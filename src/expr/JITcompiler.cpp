#include "JITcompiler.hpp"
#include "parser.hpp"
#include <cmath>
#include <iostream>
#include <tools.hpp>

#undef NDEBUG
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h> // For file writing support
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/ExecutionEngine/JITLink/JITLinkMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Transforms/Utils/BuildLibCalls.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/InstCombine/InstCombiner.h>

using namespace llvm;
using namespace llvm::orc;

JITCompiler::JITCompiler() {
}

CompiledFunction JITCompiler::compile(ExpressionNode* expr) {

	auto J = LLJITBuilder().create();
	if (!J) {
		elog("failed to create LLJITBuilder");
		return {};
	}
	auto M = createModule(expr);

	// link all libraries already linked with the parent program
	{
		auto& JD = J.get()->getMainJITDylib();
		auto& DL = J.get()->getDataLayout();
		JD.addGenerator(cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
	}

	// InstCombinePass [func] ( 1 + x - 0.5 converts to x - 0.5)
    llvm::legacy::PassManager passManager;
	passManager.add(llvm::createInstructionCombiningPass());
	passManager.add(llvm::createDeadCodeEliminationPass());

	if (J.get()->addIRModule(std::move(M))) {
		elog("failed to link module to LLJIT");
		return {};
	}
	auto evalFunc = J.get()->lookup("eval");
	if (!evalFunc) {
		elog("failed to get \"eval\" function");
		return {};	
	}
	calcFunction func = evalFunc.get().toPtr<calcFunction>();

	CompiledFunction compFunc(func, std::move(J.get()));
	return compFunc;
}

ThreadSafeModule JITCompiler::createModule(ExpressionNode* expr) {
	auto context = std::make_unique<llvm::LLVMContext>();
	funcType = FunctionType::get(Type::getDoubleTy(*context), {Type::getDoubleTy(*context)}, false);

	auto module = std::make_unique<llvm::Module>("test", *context);
	Module* M = module.get();
	Function* func = Function::Create(funcType,
						 Function::ExternalLinkage, "eval", M);

	llvm::BasicBlock* BB = llvm::BasicBlock::Create(*context, "EntryBlock", func);
	llvm::IRBuilder<> builder(BB);

	assert(func->arg_begin() != func->arg_end());
	Argument* ArgX = &*func->arg_begin(); // Get the arg
	ArgX->setName("x");

	variable = ArgX;
	builderPtr = &builder;
	contextPtr = context.get();
	modulePtr = M;
	llvm::Value* result = generateCode(expr);
	builder.CreateRet(result);
	{
		std::string llvmIR = "";
		llvm::raw_string_ostream ros(llvmIR);
		module->print(ros, nullptr, false, !PRODUCTION_BUILD);
		ilog(llvmIR, '\n');
	}

	return ThreadSafeModule(std::move(module), std::move(context));

}

llvm::Value* JITCompiler::generateCode(ExpressionNode* expr) {
	switch (expr->type) {
	case NodeType::Number:
		return llvm::ConstantFP::get(*contextPtr, llvm::APFloat(expr->number));
	case NodeType::Positive:
		return generateCode(expr->unary.operand);
	case NodeType::Negative: {
		llvm::Value* operand = generateCode(expr->unary.operand);
		return builderPtr->CreateFNeg(operand);
	}
	case NodeType::Add: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builderPtr->CreateFAdd(left, right, "addtmp");
	}
	case NodeType::Sub: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builderPtr->CreateFSub(left, right, "subtmp");
	}
	case NodeType::Mul: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builderPtr->CreateFMul(left, right, "multmp");
	}
	case NodeType::Div: {
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);
		return builderPtr->CreateFDiv(left, right, "divtmp");
	}
	case NodeType::Pow: {
		createExternalFunction("pow");
		llvm::Value* left = generateCode(expr->binary.left);
		llvm::Value* right = generateCode(expr->binary.right);

		// Check if both left and right are constants
		if (llvm::ConstantFP* leftConst = llvm::dyn_cast<llvm::ConstantFP>(left)) {
			if (llvm::ConstantFP* rightConst = llvm::dyn_cast<llvm::ConstantFP>(right)) {
				// If both are constants, calculate the result and return it as a constant
				double result =
					std::pow(leftConst->getValueAPF().convertToDouble(), rightConst->getValueAPF().convertToDouble());
				return llvm::ConstantFP::get(*contextPtr, llvm::APFloat(result));
			}
		}

		// Check if the right operand is another power expression
		if (expr->binary.left->type == NodeType::Pow) {
			// Get the base and inner exponent from the left power expression
			ExpressionNode* innerPow = expr->binary.left; // This is the left Pow
			llvm::Value* innerBase = generateCode(innerPow->binary.left);
			llvm::Value* innerExponent = generateCode(innerPow->binary.right);

			// Combine the inner exponent with the right exponent
			llvm::Value* outerExponent = right; // Use the exponent from the current Pow
			llvm::Value* newExponent = builderPtr->CreateFMul(innerExponent, outerExponent, "exponentProduct");

			CallInst* callinst = builderPtr->CreateCall(createdFunctions.at("pow"), {innerBase, newExponent}, "powtmp");
			callinst->setTailCall(true);
			return callinst;
		}
		CallInst* callinst = builderPtr->CreateCall(createdFunctions.at("pow"), {left, right}, "powtmp");
		callinst->setTailCall(true);
		return callinst;

	}
	case NodeType::Variable: {
		return variable; // Return the variable (the function's argument)
	}
	case NodeType::Function: {
		createExternalFunction(expr->function.name);
		llvm::Value* argValue = generateCode(expr->function.argument);
		CallInst* callinst = builderPtr->CreateCall(createdFunctions.at(expr->function.name), {argValue}, "funccalltmp");
		callinst->setTailCall(true);
		return callinst;
	}
	case NodeType::Error: {
		assert(0 && "ERROR WAS FOUND!, YOU PROBABLY FORGOT TO CHECK FOR IT");
		break;
	}
	default:
		return nullptr;
		break;
	}
	// llvm_unreachable();
	unreachable();
}

void JITCompiler::createExternalFunction(const std::string_view name) {
	// Check if the function has already been created
	if (createdFunctions.find(name) == createdFunctions.end()) {
		llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, modulePtr);
		func->addFnAttr(llvm::Attribute::ReadNone);
		func->addFnAttr(llvm::Attribute::NoUnwind);
		func->addFnAttr(llvm::Attribute::AlwaysInline);
		createdFunctions[name] = func; // Mark this function as created
	}
}