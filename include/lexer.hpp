#pragma once

#include <string>
#include <string_view>

#undef EOF
enum class TokenType {
	Error,
	EOF,
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
	TokenType type;
	std::string_view lexme;
};

struct Lexer {
	char* start;
	char* current;

	Lexer(std::string expression);

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
};

