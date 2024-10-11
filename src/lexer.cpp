#include "lexer.hpp"
#include "defines.hpp"
#include <string_view>
#include <ctype.h>

#pragma region helperFunction
inline Token Lexer::lexerMakeToken(TokenType type) {
	return {type, std::string_view(start, static_cast<size_t>(current - start))};
}
#pragma endregion

#pragma region majorFunctions
void Lexer::lexerInit(std::string expression) {
	start = expression.data();
	current = start;
}

Token Lexer::lexerNextToken() {
	lexerAdvanceTillConditionFail(std::isspace);
	start = current;
	if (*current == '\0')
		lexerMakeToken(TokenType::EOF);

	current++;
	char currentChar = current[-1];
	switch (currentChar){
		case '(':
			return lexerMakeToken(TokenType::OpenParenthesis);
		case ')':
			return lexerMakeToken(TokenType::CloseParenthesis);
		case '+':
			return lexerMakeToken(TokenType::Plus);
		case '-':
			return lexerMakeToken(TokenType::Minus);
		case '*':
			return lexerMakeToken(TokenType::Star);
		case '/':
			return lexerMakeToken(TokenType::Slash);
		case '^':
			return lexerMakeToken(TokenType::Caret);

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			lexerAdvanceTillConditionFail(std::isdigit);

			if (*current == '.') {
				current++;
				lexerAdvanceTillConditionFail(std::isdigit);
			}
			return lexerMakeToken(TokenType::Number);
		}

		default:
			if (std::isalpha(currentChar)) {
				lexerAdvanceTillConditionFail(std::isdigit, std::isalpha);
				return lexerMakeToken(TokenType::Ident);
			} else {
				current++;
				return lexerMakeToken(TokenType::Error);
			}
	}
	unreachable();
}

#pragma endregion