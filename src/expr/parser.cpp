#include "parser.hpp"
#include <iostream>

constexpr double pi = 3.14159265358979323846;
constexpr double E = 2.718281828459045235360;

// all functions from math.h that take 1 parameter
const std::unordered_set<std::string_view> Parser::functionSet = {"sin",  "cos",  "tan",   "acos", "asin",	"atan",
																  "cosh", "sinh", "tanh",  "log",  "log10", "sqrt",
																  "ceil", "fabs", "floor", "round"};

Parser::Parser(const std::vector<Token, ArenaAllocator<Token>>& arr) : tokenArray(arr), tokenIndex(0) {
	if (!tokenArray.empty()) {
		curr = tokenArray[tokenIndex];
		parserAdvance();
	}
}

Parser::~Parser() {
}

inline void Parser::parserAdvance() {
	if (tokenIndex < tokenArray.size()) {
		curr = tokenArray[tokenIndex];
		tokenIndex++;
	} else {
		curr = tokenArray.back(); // Keep curr pointing to the last token (tkEOF) if out of bounds
	}
}

ExpressionNode* Parser::parserParseNumber() {
	double value = strtod((const char*)curr.lexme.data(), nullptr);
	parserAdvance();

	ExpressionNode* ret = nodePool.allocate(1);
	ret->type = NodeType::Number;
	ret->number = value;
	return ret;
}

ExpressionNode* Parser::parseFunctionCall() {
	ExpressionNode* ret = nodePool.allocate(1);
	ret->type = NodeType::Function;
	ret->function.name = curr.lexme; // Store the function name

	parserAdvance(); // Advance past the function name

	if (curr.type == TokenType::OpenParenthesis) {
		parserAdvance(); // Advance past the '('
		// Parse the argument (if any) within the parentheses
		ret->function.argument = parserParseExpression(Precedence::MIN);

		if (curr.type == TokenType::CloseParenthesis) {
			// the parseIdent already does this
			// parserAdvance(); // Advance past the ')'
		} else {
			// Handle error: mismatched parentheses
			ret->type = NodeType::Error;
			hasError = true;
		}
	} else {
		// Handle error: expected '(' after function name
		ret->type = NodeType::Error;
		hasError = true;
	}

	return ret;
}

ExpressionNode* Parser::parseIdent() {
	ExpressionNode* ret = nullptr;

	if (functionSet.find(curr.lexme) != functionSet.end()) {
		ret = parseFunctionCall(); // Parse as function call
	} else if (curr.lexme == "e") {
		ret = nodePool.allocate(1);
		ret->type = NodeType::Number;
		ret->number = E;
	} else if (curr.lexme == "pi") {
		ret = nodePool.allocate(1);
		ret->type = NodeType::Number;
		ret->number = pi;
	} else if (curr.lexme == "x") {
		ret = nodePool.allocate(1);
		ret->type = NodeType::Variable;
	} else {
		ret = nodePool.allocate(1);
		ret->type = NodeType::Error;
		hasError = true;
	}
	parserAdvance();
	return ret;
}

ExpressionNode* Parser::parserParsePrefixExpr() {
	ExpressionNode* ret = nullptr;

	switch (curr.type) {
	case TokenType::Ident: {
		ret = parseIdent();
		break;
	}
	case TokenType::Number: {
		ret = parserParseNumber();
		break;
	}
	case TokenType::OpenParenthesis: {
		parserAdvance();
		ret = parserParseExpression(Precedence::MIN);
		if (curr.type == TokenType::CloseParenthesis) {
			parserAdvance();
		}
		break;
	}
	case TokenType::Plus: {
		parserAdvance();
		ret = nodePool.allocate(1);
		ret->type = NodeType::Positive;
		ret->unary.operand = parserParsePrefixExpr();
		break;
	}
	case TokenType::Minus: {
		parserAdvance();
		ret = nodePool.allocate(1);
		ret->type = NodeType::Negative;
		ret->unary.operand = parserParsePrefixExpr();
		break;
	}
	default: {
		ret = nodePool.allocate(1);
		ret->type = NodeType::Error;
		hasError = true;
		return ret;
	}
	}
	// support implicit multiplication such as:
	// 5(1 + 5), which means 5*(1+5)
	// 5pi, which means 5*pi
	if (curr.type == TokenType::Number || curr.type == TokenType::Ident || curr.type == TokenType::OpenParenthesis) {
		ExpressionNode* new_ret = nodePool.allocate(1);
		new_ret->type = NodeType::Mul;
		new_ret->binary.left = ret;
		new_ret->binary.right = parserParseExpression(Precedence::Div);
		ret = new_ret;
	}
	return ret;
}

ExpressionNode* Parser::parserParseInfixExpr(Token tk, ExpressionNode* left) {
	ExpressionNode* ret = nodePool.allocate(1);

	switch (tk.type) {
	case TokenType::Plus:
		ret->type = NodeType::Add;
		break;
	case TokenType::Minus:
		ret->type = NodeType::Sub;
		break;
	case TokenType::Star:
		ret->type = NodeType::Mul;
		break;
	case TokenType::Slash:
		ret->type = NodeType::Div;
		break;
	case TokenType::Caret:
		ret->type = NodeType::Pow;
		break;
	default:
		ret->type = NodeType::Error;
		hasError = true;
		return ret;
	}
	ret->binary.left = left;
	ret->binary.right = parserParseExpression(getPrecedence(tk.type));

	/*
	// Optimization
	if (   (ret->type == NodeType::Add && ret->binary.right->type == NodeType::Number && ret->binary.right->number == 0) ||
		   (ret->type == NodeType::Add && left->type == NodeType::Number && left->number == 0.0)) {
		return left; // x + 0 => x
	} else if (
			(ret->type == NodeType::Mul && ret->binary.right->type == NodeType::Number && ret->binary.right->number == 1) ||
			(ret->type == NodeType::Mul && left->type == NodeType::Number && left->number == 1.0)) {
		return left; // x * 1 => x
	}
	*/

	return ret;
}

ExpressionNode* Parser::parserParseExpression(Precedence curr_operator_prec) {
	ExpressionNode* left = parserParsePrefixExpr();
	Token next_operator = curr;
	Precedence next_operator_prec = getPrecedence(curr.type);

	while (next_operator_prec != Precedence::MIN) {

		if (curr_operator_prec >= next_operator_prec) {
			break;
		} else /* (curr_operator_prec < next_operator_prec) */ {
			parserAdvance(); // Advance the operator

			left = parserParseInfixExpr(next_operator, left);
			next_operator = curr;
			next_operator_prec = getPrecedence(curr.type);
		}
	}

	return left;
}

void Parser::parserDebugDumpTree(ExpressionNode* node, size_t indent) {
	for (size_t i = 0; i < indent; i++)
		printf("  ");

	switch (node->type) {
	case NodeType::Error:
		printf("Error\n");
		break;

	case NodeType::Variable: {
		printf("X\n");
	} break;

	case NodeType::Number: {
		printf("%f\n", node->number);
	} break;

	case NodeType::Positive: {
		printf("Unary +:\n");
		parserDebugDumpTree(node->unary.operand, indent + 1);
	} break;

	case NodeType::Negative: {
		printf("Unary -:\n");
		parserDebugDumpTree(node->unary.operand, indent + 1);
	} break;

	case NodeType::Add: {
		printf("+:\n");
		parserDebugDumpTree(node->binary.left, indent + 1);
		parserDebugDumpTree(node->binary.right, indent + 1);
	} break;

	case NodeType::Sub: {
		printf("-:\n");
		parserDebugDumpTree(node->binary.left, indent + 1);
		parserDebugDumpTree(node->binary.right, indent + 1);
	} break;

	case NodeType::Mul: {
		printf("*:\n");
		parserDebugDumpTree(node->binary.left, indent + 1);
		parserDebugDumpTree(node->binary.right, indent + 1);
	} break;

	case NodeType::Div: {
		printf("/:\n");
		parserDebugDumpTree(node->binary.left, indent + 1);
		parserDebugDumpTree(node->binary.right, indent + 1);
	} break;

	case NodeType::Pow: {
		printf("^:\n");
		parserDebugDumpTree(node->binary.left, indent + 1);
		parserDebugDumpTree(node->binary.right, indent + 1);
	} break;
	case NodeType::Function: {
		printf("%.*s:\n", static_cast<int>(node->function.name.size()), node->function.name.data());
		parserDebugDumpTree(node->function.argument, indent + 1);
	}
	}
}