#include "eval.hpp"
#include "parser.hpp"
#include <math.h>

double evaluate(ExpressionNode* expr, double x) {
	switch (expr->type) {
	case NodeType::Number:
		return expr->number;
	case NodeType::Variable:
		return x;
	case NodeType::Positive:
		return +evaluate(expr->unary.operand, x);
	case NodeType::Negative:
		return -evaluate(expr->unary.operand, x);
	case NodeType::Add:
		return evaluate(expr->binary.left, x) + evaluate(expr->binary.right, x);
	case NodeType::Sub:
		return evaluate(expr->binary.left, x) - evaluate(expr->binary.right, x);
	case NodeType::Mul:
		return evaluate(expr->binary.left, x) * evaluate(expr->binary.right, x);
	case NodeType::Div:
		return evaluate(expr->binary.left, x) / evaluate(expr->binary.right, x);
	case NodeType::Pow:
		return pow(evaluate(expr->binary.left, x), evaluate(expr->binary.right, x));
	}
	return 0.0;
}