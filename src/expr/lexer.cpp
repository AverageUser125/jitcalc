#include "lexer.hpp"
#include "defines.hpp"
#include <string_view>
#include <ctype.h>
#include <iostream>
#include <algorithm>

#pragma region helperFunction
[[nodiscard]] inline Token Lexer::lexerMakeToken(TokenType type) {
	return {type, std::string_view(start, static_cast<size_t>(current - start))};
}

#pragma endregion

#pragma region majorFunctions

Lexer::Lexer(const std::string& expression) {
	cleanExpression.reserve(expression.size());
	for (char c : expression) {
		if (!std::isspace(c)) {
			cleanExpression += c;
		}
	}
	start = cleanExpression.c_str();
	current = start;
}

std::optional<std::vector<Token>> Lexer::lexerLexAllTokens() {
	std::vector<Token> tokenArray;
	tokenArray.reserve(cleanExpression.size()); // overkill, but still okay

	Token tk;
	do {
		tk = lexerNextToken();
		tokenArray.push_back(tk);
	} while (tk.type != TokenType::tkEOF);
	if (parenthesesBalance != 0) {
		return std::nullopt;
	}

	return tokenArray;
}

Token Lexer::lexerNextToken() {
	// lexerAdvanceTillConditionFail(std::isspace);
	start = current;
	if (*current == '\0') {
		return lexerMakeToken(TokenType::tkEOF);
	}

	current++;
	char currentChar = current[-1];
	switch (currentChar){
		case '(':
			parenthesesBalance++;
			return lexerMakeToken(TokenType::OpenParenthesis);
		case ')':
			parenthesesBalance--;
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
			lexerAdvanceTillConditionFail(static_cast<int(*)(int)>(std::isdigit));

			if (*current == '.') {
				current++;
				lexerAdvanceTillConditionFail(static_cast<int(*)(int)>(std::isdigit));
			}
			return lexerMakeToken(TokenType::Number);
		}

		default:
			if (std::isalpha(currentChar)) {
				lexerAdvanceTillConditionFail(static_cast<int(*)(int)>(std::isdigit), static_cast<int(*)(int)>(std::isalpha));
				return lexerMakeToken(TokenType::Ident);
			}else {
				current++;
				return lexerMakeToken(TokenType::Error);
			}
	}
	unreachable();
}

std::string Lexer::lexerDebugGetTokenTypeName(TokenType type) {
	switch (type) {
	case TokenType::Error:
		return "Error";
	case TokenType::tkEOF:
		return "tkEOF";
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

void Lexer::lexerDebugPrintArray(const std::vector<Token>& tokenArray) {
	for (const Token& tk : tokenArray) {
		lexerDebugPrintToken(tk);
	}
}

void Lexer::lexerDebugPrintToken(Token token) {
	std::cout << lexerDebugGetTokenTypeName(token.type) << "  " << token.lexme << '\n';
}

#pragma endregion