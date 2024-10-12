#include "lexer.hpp"
#include "defines.hpp"
#include <string_view>
#include <ctype.h>
#include <iostream>

#pragma region helperFunction
[[nodiscard]] inline Token Lexer::lexerMakeToken(TokenType type) {
	return {type, std::string_view(start, static_cast<size_t>(current - start))};
}


Token Lexer::lexerPeek() {
	const char* temp1 = start;
	const char* temp2 = current;
	Token tk = lexerNextToken();
	start = temp1;
	current = temp2;
	return tk;
}

#pragma endregion

#pragma region majorFunctions

Lexer::Lexer(const std::string& expression)
{
	start = expression.c_str();
	current = start;
}

Token Lexer::lexerNextToken() {
	// lexerAdvanceTillConditionFail(std::isspace);
	start = current;
	if (*current == '\0')
		return lexerMakeToken(TokenType::EOF);

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
			}else {
				current++;
				return lexerMakeToken(TokenType::Error);
			}
	}
	unreachable();
}

std::string lexerDebugGetTokenTypeName(TokenType type) {
	switch (type) {
	case TokenType::Error:
		return "Error";
	case TokenType::EOF:
		return "EOF";
	case TokenType::Number:
		return "Number Literal";
	case TokenType::Ident:
		return "Identifier";
	case TokenType::Plus:
		return "+";
	case TokenType::Minus:
		return "-";
	case TokenType::Star:
		return "*";
	case TokenType::Slash:
		return "/";
	case TokenType::Caret:
		return "^";
	case TokenType::OpenParenthesis:
		return "(";
	case TokenType::CloseParenthesis:
		return ")";
	}
	return "unreachable";
}

void Lexer::lexerDebugPrintToken(Token token) {
	std::cout << lexerDebugGetTokenTypeName(token.type) << "  " << token.lexme << '\n';
}

#pragma endregion