#include "displayPoints.hpp"
#include "platformInput.h"
#include <iostream>

bool gameLogic(float deltaTime) {
	if (platform::isButtonPressedOn(platform::Button::A)) {
		std::cout << "A";
	}

	return true;
}