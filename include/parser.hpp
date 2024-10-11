#pragma once
#include "lexer.hpp"

enum class Precedence {
	MIN,
	Term,
	Mult,
	Div,
	Power,
	MAX,
};
const static Precedence precedenceLookup[static_cast<int>(TokenType::MAX)] = {
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


enum class NodeType {
	Error,
	Number,
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
		size_t number;

		struct {
			ExpressionNode* operand;
		} unary;

		struct {
			ExpressionNode* left;
			ExpressionNode* right;
		} binary;
	};
};

/*
typedef struct Parser {
	M_Pool node_pool;

	Token curr;
	Token next;
	Lexer lexer;


} Parser;
*/