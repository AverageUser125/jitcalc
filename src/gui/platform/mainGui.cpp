#include "displayPoints.hpp"
#include "GLFW/glfw3.h"
#include "mainGui.hpp"
#include "opterPlatformFunctions.hpp"
#include "platformInput.h"
#include <chrono>
#include <defines.hpp>
#include <fstream>
#include <glad/errorReporting.hpp>
#include <glad/glad.h>
#include <iostream>
#if PLATFORM_WIN
#define WIN32_LEAN_AN_MEAN
#define NOMINMAX
#include <Windows.h>
#endif
#include <cstdio>
static GLFWwindow* window = nullptr;
static bool currentFullScreen = 0;
static bool fullScreen = 0;
bool windowFocus = true;

constexpr int WINDOW_DEFAULT_WIDTH = 1280;
constexpr int WINDOW_DEFAULT_HEIGHT = 720;

#pragma region glfwcallbacks

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if ((action == GLFW_REPEAT || action == GLFW_PRESS) && key == GLFW_KEY_BACKSPACE) {
		platform::internal::addToTypedInput(8);
	}

	bool state = 0;

	if (action == GLFW_PRESS) {
		state = 1;
	} else if (action == GLFW_RELEASE) {
		state = 0;
	} else {
		return;
	}

	if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
		int index = key - GLFW_KEY_A;
		platform::internal::setButtonState(platform::Button::A + index, state);
	} else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
		int index = key - GLFW_KEY_0;
		platform::internal::setButtonState(platform::Button::NR0 + index, state);
	} else if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25) {
		int index = key - GLFW_KEY_F1;
		platform::internal::setButtonState(platform::Button::F1 + index, state);
	} else {
		//special keys
		//GLFW_KEY_SPACE, GLFW_KEY_ENTER, GLFW_KEY_ESCAPE, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT

		if (key == GLFW_KEY_SPACE) {
			platform::internal::setButtonState(platform::Button::Space, state);
		} else if (key == GLFW_KEY_ENTER) {
			platform::internal::setButtonState(platform::Button::Enter, state);
		} else if (key == GLFW_KEY_ESCAPE) {
			platform::internal::setButtonState(platform::Button::Escape, state);
		} else if (key == GLFW_KEY_UP) {
			platform::internal::setButtonState(platform::Button::Up, state);
		} else if (key == GLFW_KEY_DOWN) {
			platform::internal::setButtonState(platform::Button::Down, state);
		} else if (key == GLFW_KEY_LEFT) {
			platform::internal::setButtonState(platform::Button::Left, state);
		} else if (key == GLFW_KEY_RIGHT) {
			platform::internal::setButtonState(platform::Button::Right, state);
		} else if (key == GLFW_KEY_LEFT_CONTROL) {
			platform::internal::setButtonState(platform::Button::LeftCtrl, state);
		} else if (key == GLFW_KEY_TAB) {
			platform::internal::setButtonState(platform::Button::Tab, state);
		} else if (key == GLFW_KEY_LEFT_SHIFT) {
			platform::internal::setButtonState(platform::Button::LeftShift, state);
		} else if (key == GLFW_KEY_LEFT_ALT) {
			platform::internal::setButtonState(platform::Button::LeftAlt, state);
		}
	}
};

void mouseCallback(GLFWwindow* window, int key, int action, int mods) {
	bool state = 0;

	if (action == GLFW_PRESS) {
		state = 1;
	} else if (action == GLFW_RELEASE) {
		state = 0;
	} else {
		return;
	}

	if (key == GLFW_MOUSE_BUTTON_LEFT) {
		platform::internal::setLeftMouseState(state);
	} else if (key == GLFW_MOUSE_BUTTON_RIGHT) {
		platform::internal::setRightMouseState(state);
	}
}

void windowFocusCallback(GLFWwindow* window, int focused) {
	if (focused) {
		windowFocus = 1;
	} else {
		windowFocus = 0;
		//if you not capture the release event when the window loses focus,
		//the buttons will stay pressed
		platform::internal::resetInputsToZero();
	}
}

void windowSizeCallback(GLFWwindow* window, int x, int y) {
	platform::internal::resetInputsToZero();
}

static int mouseMovedFlag = 0;

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
	mouseMovedFlag = 1;
}

void characterCallback(GLFWwindow* window, unsigned int codepoint) {
	if (codepoint < 127) {
		platform::internal::addToTypedInput(codepoint);
	}
}

#pragma endregion


#pragma region platform functions

namespace platform
{

void setRelMousePosition(int x, int y) {
	glfwSetCursorPos(window, x, y);
}

bool isFullScreen() {
	return fullScreen;
}

void setFullScreen(bool f) {
	fullScreen = f;
}

ivec2 getFrameBufferSize() {
	int x = 0;
	int y = 0;
	glfwGetFramebufferSize(window, &x, &y);
	return {x, y};
}

ivec2 getRelMousePosition() {
	double x = 0, y = 0;
	glfwGetCursorPos(window, &x, &y);
	return {(int)x, (int)y};
}

ivec2 getWindowSize() {
	int x = 0;
	int y = 0;
	glfwGetWindowSize(window, &x, &y);
	return {x, y};
}

//todo test
void showMouse(bool show) {
	if (show) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	} else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
}

bool isFocused() {

	return windowFocus;
}

bool mouseMoved() {
	return mouseMovedFlag;
}

bool writeEntireFile(const char* name, void* buffer, size_t size) {
	std::ofstream f(name, std::ios::binary);

	if (!f.is_open()) {
		return 0;
	}

	f.write((char*)buffer, size);

	f.close();

	return 1;
}

bool readEntireFile(const char* name, void* buffer, size_t size) {
	std::ifstream f(name, std::ios::binary);

	if (!f.is_open()) {
		return 0;
	}

	f.read((char*)buffer, size);

	f.close();

	return 1;
}

}; // namespace platform

#pragma endregion

void guiLoop() {
	auto stop = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

#pragma region deltaTime

		//long newTime = clock();
		//float deltaTime = (float)(newTime - lastTime) / CLOCKS_PER_SEC;
		//lastTime = clock();
		auto start = std::chrono::high_resolution_clock::now();

		float deltaTime = (std::chrono::duration_cast<std::chrono::nanoseconds>(start - stop)).count() / 1000000000.0;
		stop = std::chrono::high_resolution_clock::now();

		float augmentedDeltaTime = deltaTime;
		if (augmentedDeltaTime > 1.f / 10) {
			augmentedDeltaTime = 1.f / 10;
		}

#pragma endregion
		
#pragma region fullscreen

		if (platform::isFocused() && currentFullScreen != fullScreen) {
			static int lastW = WINDOW_DEFAULT_WIDTH;
			static int lastH = WINDOW_DEFAULT_HEIGHT;
			static int lastPosX = 0;
			static int lastPosY = 0;

			if (fullScreen) {
				lastW = WINDOW_DEFAULT_WIDTH;
				lastH = WINDOW_DEFAULT_HEIGHT;

				//glfwWindowHint(GLFW_DECORATED, NULL); // Remove the border and titlebar..
				glfwGetWindowPos(window, &lastPosX, &lastPosY);


				//auto monitor = glfwGetPrimaryMonitor();
				auto monitor = getCurrentMonitor(window);


				const GLFWvidmode* mode = glfwGetVideoMode(monitor);

				// switch to full screen
				glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

				currentFullScreen = 1;

			} else {
				//glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); //
				glfwSetWindowMonitor(window, nullptr, lastPosX, lastPosY, lastW, lastH, 0);

				currentFullScreen = 0;
			}
		}

#pragma endregion

#pragma region reset flags

		mouseMovedFlag = 0;
		platform::internal::updateAllButtons(deltaTime);
		platform::internal::resetTypedInput();

#pragma endregion

		if (!gameLogic(augmentedDeltaTime)) {
			return;
		}

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
	}
}

bool guiInit() {

#if PLATFORM_WIN
#if PRODUCTION_BUILD == 0
	AllocConsole();
	(void)freopen("conin$", "r", stdin);
	(void)freopen("conout$", "w", stdout);
	(void)freopen("conout$", "w", stderr);
	std::cout.sync_with_stdio();
#endif
#endif

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(1280, 720, "Graph calculator", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1); // Enable vsync

	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetWindowFocusCallback(window, windowFocusCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	glfwSetCursorPosCallback(window, cursorPositionCallback);
	glfwSetCharCallback(window, characterCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}
	// enable error reporting
	enableReportGlErrors();

	return true;
}

void guiCleanup() {
	if (window) {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
}
