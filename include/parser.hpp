#pragma once
#include "lexer.hpp"
#include <vector>
#include "arenaAllocator.hpp"

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
	Precedence::MIN,   // TokenType::tkEOF
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
	ArenaAllocator<ExpressionNode> nodePool;
	bool hasError = false;

	Token curr{};
	Token next{};
	const std::vector<Token, ArenaAllocator<Token>>& tokenArray;
	size_t tokenIndex = 0;

	Parser(const std::vector<Token, ArenaAllocator<Token>>& arr);
	~Parser();

	inline void parserAdvance();

	ExpressionNode* parserParseNumber();
	ExpressionNode* parseIdent();
	ExpressionNode* parserParsePrefixExpr();
	ExpressionNode* parserParseExpression(Precedence curr_operator_prec = Precedence::MIN);
	ExpressionNode* parserParseInfixExpr(Token tk, ExpressionNode* left);

	static void parserDebugDumpTree(ExpressionNode* node, size_t indent = 0);
} Parser;