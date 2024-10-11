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
