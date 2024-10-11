#include "lexer.hpp"
#include <string_view>
#include <ctype.h>

#pragma region helperFunction
constexpr bool Lexer::lexerBound() {
	return *current == '\0';
}
constexpr char Lexer::lexerPeek() {
	return *current;
}
constexpr char Lexer::lexerAdvance() {
	current++;
	return current[-1];
}
constexpr void Lexer::lexerSkipWhitespace() {
	while (true) {
		char c = lexerPeek();
		if (std::isspace(c)) {
			lexerAdvance();
		} else {
			return;
		}
	}
}

inline void Lexer::lexerAdvanceTillConditionFail(bool (*func(char))) {
	while (func(*current)) {
		current++;
	}
}

inline Token Lexer::lexerMakeToken(TokenType type) {
	return {type, std::string_view(start, static_cast<size_t>(current - start))};
}
#pragma endregion

#pragma region tokenizerFunctions

Token Lexer::lexerNumber() {
	lexerAdvanceTillConditionFail(std::isdigit);

	if (lexerPeek() == '.') {
		lexerAdvance();
		lexerAdvanceTillConditionFail(std::isdigit);
	}

	return lexerMakeToken(TokenType::Number);
}

Token Lexer::lexerIdentifier() {
	lexerAdvanceTillConditionFail(std::isdigit, std::isalpha);
	return lexerMakeToken(TokenType::Ident);
}

#pragma endregion

void lexerInit(Lexer* lexer, std::string expression) {
}

Token lexerNextToken(Lexer* lexer) {
	return Token();
}
