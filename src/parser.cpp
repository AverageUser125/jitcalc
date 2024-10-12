#include "parser.hpp"
#include <cstdlib>
#include <cassert>

void Parser::parserAdvance() {
	curr = next;
	next = lexer.lexerNextToken();
}

Parser::Parser(std::string expression) : lexer(expression) {
	parserAdvance();
	parserAdvance();
}

ExpressionNode* Parser::pushNodeAndGetPointer() {
	nodes.emplace_back(); // Add a new node to the list
	return &nodes.back(); // Return the reference to the newly added node
};

ExpressionNode& Parser::parserParsePrefixExpr() {

	ExpressionNode* ret = nullptr;

	switch (curr.type) {
		case (TokenType::Number): {
			double value = std::stod(std::string(curr.lexme));
			parserAdvance();

			ret = pushNodeAndGetPointer();

			ret->type = NodeType::Number;
			ret->number = value;
			break;
		} case (TokenType::OpenParenthesis): {
			parserAdvance();
			ret = &parserParseExpression(Precedence::MIN);
			if (curr.type == TokenType::CloseParenthesis) {
				parserAdvance();
			}
			break;
		} case (TokenType::Plus): {
			parserAdvance();

			ret = pushNodeAndGetPointer();
			ret->type = NodeType::Positive;
			ret->unary.operand = &parserParsePrefixExpr();
			break;
		} case (TokenType::Minus): {
			parserAdvance();
			ret = pushNodeAndGetPointer();
			ret->type = NodeType::Negative;
			ret->unary.operand = &parserParsePrefixExpr();
			break;
		}
		default:
		{
			ret = pushNodeAndGetPointer();
			ret->type = NodeType::Error;
		}
	}

	if (curr.type == TokenType::Number || curr.type == TokenType::OpenParenthesis) {
		ExpressionNode* newRet = pushNodeAndGetPointer();
		newRet->type = NodeType::Mul;
		newRet->binary.left = ret;
		newRet->binary.right = &parserParseExpression(Precedence::Div);
	}
	return *ret;
}

ExpressionNode& Parser::parserParseExpression(Precedence curr_operator_prec) {
	ExpressionNode& left = parserParsePrefixExpr();
	Token next_operator = curr;
	Precedence next_operator_prec = getPrecedence(curr.type);

	while (next_operator_prec != Precedence::MIN) {

		if (curr_operator_prec >= next_operator_prec) {
			break;
		} else /* (curr_operator_prec < next_operator_prec) */ {
			parserAdvance(); // Advance the operator

			left = parserParseInfixExpr(next_operator, left);
			next_operator = curr;
			next_operator_prec = getPrecedence(curr.type);
		}
	}

	return left;
}

ExpressionNode& Parser::parserParseInfixExpr(Token tk, ExpressionNode& left) {
	nodes.push_back({});
	ExpressionNode& ret = *nodes.end();

	switch (tk.type) {
	case TokenType::Plus:
		ret.type = NodeType::Add;
		break;
	case TokenType::Minus:
		ret.type = NodeType::Sub;
		break;
	case TokenType::Star:
		ret.type = NodeType::Mul;
		break;
	case TokenType::Slash:
		ret.type = NodeType::Div;
		break;
	case TokenType::Caret:
		ret.type = NodeType::Pow;
		break;
	}
	ret.binary.left = &left;
	ret.binary.right = &parserParseExpression(getPrecedence(tk.type));
	return ret;
}
