#include "displayPoints.hpp"
#include "platformInput.h"
#include "mainGui.hpp"
#include <iostream>

bool gameLogic(float deltaTime) {
	if (platform::isButtonPressedOn(platform::Button::A)) {
		std::cout << "A";
	}

	if (platform::isButtonPressedOn(platform::Button::B)) {
		if (platform::isFullScreen()) {
			platform::setFullScreen(false);
		} else {
			platform::setFullScreen(true);		
		}
	}


	return true;
}