#include "graphMain.hpp"
#include "platformInput.h"
#include "mainGui.hpp"
#include "parser.hpp"
#include "JITcompiler.hpp"
#include <iostream>

bool gameInit() {

}

bool gameLogic(float deltaTime) {
	if (platform::isButtonPressedOn(platform::Button::A)) {
		// x ^ (0.5) / (x ^ 2) causes crash (negative x)
		// x ^ (0.5) / (2 ) doesn't crash
		std::string input = "x * x";

		// std::getline(std::cin, input);
		Parser parser(input);
		ExpressionNode* tree = parser.parserParseExpression(Precedence::MIN);
		parser.parserDebugDumpTree(tree, 0);

		JITCompiler jit;
		auto func = jit.compile(tree);

		for (double i = -1; i <= 2; i++) {
			std::cout << func(i) << '\n';
		}
	}

	if (platform::isButtonPressedOn(platform::Button::F11)) {
		if (platform::isFullScreen()) {
			platform::setFullScreen(false);
		} else {
			platform::setFullScreen(true);		
		}
	}


	return true;
}


bool gameEnd() {

}