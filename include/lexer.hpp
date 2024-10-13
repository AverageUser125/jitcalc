#pragma once

#include <string>
#include <string_view>

enum class TokenType {
	Error,
	tkEOF,
	Ident,
	Number,
	Plus,
	Minus,
	Star,
	Slash,
	Caret,
	OpenParenthesis ,
	CloseParenthesis,
	Comma,
	MAX
};
struct Token {
	TokenType type{};
	std::string_view lexme;
};

struct Lexer {
	std::string cleanExpression;
	const char* start = nullptr;
	const char* current = nullptr;

	Lexer(const std::string& expression);

	inline Token lexerMakeToken(TokenType type);	
	// Template function that accepts multiple conditions and checks them in the loop
	template <typename... Conditions> inline void lexerAdvanceTillConditionFail(Conditions... conditions) {
		// Lambda to check all conditions
		auto allConditionsPass = [&](char c) -> bool {
			return (... && conditions(c)); // Fold expression to apply all conditions
		};

		// Advance lexer until one condition fails
		while (allConditionsPass(*current)) {
			current++;
		}
	}

	Token lexerNextToken();

	static void lexerDebugPrintToken(Token token);
};

