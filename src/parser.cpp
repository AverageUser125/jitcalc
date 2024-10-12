#include "parser.hpp"
#include <cstdlib>
#include <cassert>
#include "arena.h"

void Parser::parserAdvance() {
	curr = next;
	next = lexer.lexerNextToken();
}

Parser::Parser(std::string& expression) : lexer(expression), nodePool({}) {
	parserAdvance();
	parserAdvance();
	arena_init(&nodePool);
}

Parser::~Parser() {
	arena_free(&nodePool);
}

ExpressionNode* Parser::allocateExpressionNode() {
	return (ExpressionNode*)arena_alloc(&nodePool, sizeof(ExpressionNode));
};

ExpressionNode& Parser::parserParsePrefixExpr() {

	ExpressionNode* ret = nullptr;

	switch (curr.type) {
		case (TokenType::Number): {
			double value = std::stod(curr.lexme.data(), nullptr);
			parserAdvance();

			ret = allocateExpressionNode();
			ret->type = NodeType::Number;
			ret->number = value;
			break;
		} case (TokenType::OpenParenthesis): {
			parserAdvance();
			ret = &parserParseExpression(Precedence::MIN);
			if (curr.type == TokenType::CloseParenthesis) {
				parserAdvance();
			}
			break;
		} case (TokenType::Plus): {
			parserAdvance();
			ret = allocateExpressionNode();
			ret->type = NodeType::Positive;
			ret->unary.operand = &parserParsePrefixExpr();
			break;
		} case (TokenType::Minus): {
			parserAdvance();
			ret = allocateExpressionNode();
			ret->type = NodeType::Negative;
			ret->unary.operand = &parserParsePrefixExpr();
			break;
		}
		default:
		{
			ret = allocateExpressionNode();
			ret->type = NodeType::Error;
		}
	}

	if (0 || curr.type == TokenType::Number || curr.type == TokenType::OpenParenthesis) {
		ExpressionNode* newRet = allocateExpressionNode();
		newRet->type = NodeType::Mul;
		newRet->binary.left = ret;
		newRet->binary.right = &parserParseExpression(Precedence::Div);
	}
	return *ret;
}

ExpressionNode& Parser::parserParseExpression(Precedence curr_operator_prec) {
	ExpressionNode& left = parserParsePrefixExpr();
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

ExpressionNode& Parser::parserParseInfixExpr(Token tk, ExpressionNode& left) {
	ExpressionNode& ret = *allocateExpressionNode();

	switch (tk.type) {
	case TokenType::Plus:
		ret.type = NodeType::Add;
		break;
	case TokenType::Minus:
		ret.type = NodeType::Sub;
		break;
	case TokenType::Star:
		ret.type = NodeType::Mul;
		break;
	case TokenType::Slash:
		ret.type = NodeType::Div;
		break;
	case TokenType::Caret:
		ret.type = NodeType::Pow;
		break;
	}
	ret.binary.left = &left;
	ret.binary.right = &parserParseExpression(getPrecedence(tk.type));
	return ret;
}

void Parser::parserDebugDumpTree(ExpressionNode* node, size_t indent) {
	for (size_t i = 0; i < indent; i++)
		printf("  ");

	switch (node->type) {
	case NodeType::Error:
		printf("Error\n");
		break;

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
	}
}