#pragma once
#include "lexer.hpp"
#include <vector>
#include "arena.h"

enum class Precedence {
	MIN,
	Term,
	Mult,
	Div,
	Power,
	MAX,
};
constexpr Precedence precedenceLookup[static_cast<int>(TokenType::MAX)] = {
	Precedence::MIN,   // TokenType::Error
	Precedence::MIN,   // TokenType::EOF
	Precedence::MIN,   // TokenType::Ident
	Precedence::MIN,   // TokenType::Number
	Precedence::Term,  // TokenType::Plus
	Precedence::Term,  // TokenType::Minus
	Precedence::Mult,  // TokenType::Star
	Precedence::Div,   // TokenType::Slash
	Precedence::Power, // TokenType::Caret
	Precedence::MIN,   // TokenType::OpenParenthesis
	Precedence::MIN,   // TokenType::CloseParenthesis
	Precedence::MIN,   // TokenType::Comma
};

// Utility function to access array with enum
constexpr Precedence getPrecedence(TokenType type) {
	return precedenceLookup[static_cast<int>(type)];
}

enum class NodeType {
	Error,
	Number,
	Variable,
	Positive,
	Negative,
	Add,
	Sub,
	Mul,
	Div,
	Pow
};

struct ExpressionNode {
	NodeType type;

	union {
		double number;

		struct {
			ExpressionNode* operand;
		} unary;

		struct {
			ExpressionNode* left;
			ExpressionNode* right;
		} binary;
	};
};

typedef struct Parser {
	Arena nodePool;

	Token curr{};
	Token next{};
	Lexer lexer;

	Parser(std::string& expression);
	~Parser();

	inline void parserAdvance();

	ExpressionNode* allocateExpressionNode();

	ExpressionNode* parserParseNumber();
	ExpressionNode* parseIdent();
	ExpressionNode* parserParsePrefixExpr();
	ExpressionNode* parserParseExpression(Precedence curr_operator_prec);
	ExpressionNode* parserParseInfixExpr(Token tk, ExpressionNode* left);

	static void parserDebugDumpTree(ExpressionNode* node, size_t indent);
} Parser;