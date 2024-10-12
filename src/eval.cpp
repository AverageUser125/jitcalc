#include "eval.hpp"
#include "parser.hpp"
#include <math.h>

double evaluate(ExpressionNode* expr) {
	switch (expr->type) {
	case NodeType::Number:
		return expr->number;
	case NodeType::Positive:
		return +evaluate(expr->unary.operand);
	case NodeType::Negative:
		return -evaluate(expr->unary.operand);
	case NodeType::Add:
		return evaluate(expr->binary.left) + evaluate(expr->binary.right);
	case NodeType::Sub:
		return evaluate(expr->binary.left) - evaluate(expr->binary.right);
	case NodeType::Mul:
		return evaluate(expr->binary.left) * evaluate(expr->binary.right);
	case NodeType::Div:
		return evaluate(expr->binary.left) / evaluate(expr->binary.right);
	case NodeType::Pow:
		return pow(evaluate(expr->binary.left), evaluate(expr->binary.right));
	}
	return 0.0;
}