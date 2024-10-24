#include "graphMain.hpp"
#include <GLFW/glfw3.h>
#include "mainGui.hpp"
#include "opterPlatformFunctions.hpp"
#include "platformInput.h"
#include <chrono>
#include <defines.hpp>
#include <fstream>
#include <glad/errorReporting.hpp>
#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imguiThemes.h>
#include "windowIcon.h"
#include "tools.hpp"

#if PLATFORM_WIN
#define WIN32_LEAN_AN_MEAN
#define NOMINMAX
#include <Windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
using DwmSetWindowAttributeType = HRESULT(__stdcall*)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute,
													  DWORD cbAttribute);
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif
static GLFWwindow* wind = nullptr;
static bool currentFullScreen = 0;
static bool fullScreen = 0;
static bool windowFocus = true;
static double scrollSize = 0.0;

constexpr int WINDOW_DEFAULT_WIDTH = 1280;
constexpr int WINDOW_DEFAULT_HEIGHT = 1280;

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
	if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED)) {
		glfwRestoreWindow(window);
	}
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

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	scrollSize = yoffset;
}

#pragma endregion

#pragma region platform functions

namespace platform
{
#if PRODUCTION_BUILD == 0
#if PLATFORM_WIN == 1
void clearTerminal() {
	COORD topLeft = {0, 0};
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
	FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
							   screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
	SetConsoleCursorPosition(console, topLeft);
}
#elif PLATFORM_LINUX || PLATFORM_MAC
void clearTerminal() {
	std::cout << "\x1B[2J\x1B[H";
}
#endif
#else
void clearTerminal() {
	// does nothing
}
#endif

void setRelMousePosition(int x, int y) {
	glfwSetCursorPos(wind, x, y);
}

double getScrollSize() {
	return scrollSize;
}

bool isFullScreen() {
	return fullScreen;
}

void setFullScreen(bool f) {
	fullScreen = f;
}

glm::ivec2 getFrameBufferSize() {
	int x = 0;
	int y = 0;
	glfwGetFramebufferSize(wind, &x, &y);
	return {x, y};
}

glm::ivec2 getRelMousePosition() {
	double x = 0, y = 0;
	glfwGetCursorPos(wind, &x, &y);
	return {(int)floor(x), (int)floor(y)};
}

glm::ivec2 getWindowSize() {
	int x = 0;
	int y = 0;
	glfwGetWindowSize(wind, &x, &y);
	return {x, y};
}

//todo test
void showMouse(bool show) {
	if (show) {
		glfwSetInputMode(wind, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	} else {
		glfwSetInputMode(wind, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
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

int guiLoop() {
#pragma region windows console
#if PLATFORM_WIN
#if PRODUCTION_BUILD == 0
	AllocConsole();
	(void)freopen("conin$", "r", stdin);
	(void)freopen("conout$", "w", stdout);
	(void)freopen("conout$", "w", stderr);
	std::cout.sync_with_stdio();
#endif
#endif
#pragma endregion
	int result = EXIT_SUCCESS;
#pragma region glfw and glad init
	permaAssertComment(glfwInit(), "GLFW has failed to initialize");

	glfwWindowHint(GLFW_SAMPLES, 4);
#if PLATFORM_MAC
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
#if PRODUCTION_BUILD == 0
	glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
#endif
	wind = glfwCreateWindow(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT, "Graph calculator", nullptr, nullptr);
	if (!wind) {
		elog("Failed to create GLFW window");
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwSetWindowAspectRatio(wind, 1, 1);
#if PLATFORM_WIN
	// set window decoration to black
	HMODULE dwamapidll = LoadLibraryW(L"Dwmapi.dll");
	if (dwamapidll) {
		DwmSetWindowAttributeType fnDwmSetWindowAttribute =
			(DwmSetWindowAttributeType)GetProcAddress(dwamapidll, "DwmSetWindowAttribute");
		if (fnDwmSetWindowAttribute) {
			HWND hwnd = glfwGetWin32Window(wind);
			assert(hwnd != nullptr);
			BOOL value = TRUE;
			fnDwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
			glog("successfully called DwmSetWindowAttribute");
		} else {
			wlog("failed to get DwmSetWindowAttribute from Dwmapi.dll");
		}
		FreeLibrary(dwamapidll);
	} else {
		wlog("failed to lode Dwampi.dll");
	}
#endif
	glfwMakeContextCurrent(wind);
	glfwSwapInterval(1);

	glfwSetKeyCallback(wind, keyCallback);
	glfwSetMouseButtonCallback(wind, mouseCallback);
	glfwSetWindowFocusCallback(wind, windowFocusCallback);
	glfwSetWindowSizeCallback(wind, windowSizeCallback);
	glfwSetCursorPosCallback(wind, cursorPositionCallback);
	glfwSetCharCallback(wind, characterCallback);
	glfwSetScrollCallback(wind, scrollCallback);

	const GLFWimage windownIconImage{windowIconHeight, windowIconWidth, const_cast<unsigned char*>(windowIcon)};
	glfwSetWindowIcon(wind, 1, &windownIconImage);

	permaAssertComment(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

#if PRODUCTION_BUILD == 0
	// enable error reporting
	enableReportGlErrors();
#endif
#pragma endregion

#pragma region imgui init
	ImGui::CreateContext();

	imguiThemes::embraceTheDarkness();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplGlfw_InitForOpenGL(wind, true);
	ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion

	auto stop = std::chrono::high_resolution_clock::now();

	if (!gameInit()) {
		wlog("gameInit has failed, exiting");
		result = EXIT_FAILURE;
		goto defer;
	}

	while (!glfwWindowShouldClose(wind)) {

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
#pragma region imgui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
#pragma endregion

		int display_w, display_h;
		glfwGetFramebufferSize(wind, &display_w, &display_h);

		if (!gameLogic(augmentedDeltaTime, display_w, display_h)) {
			return EXIT_SUCCESS;
		}

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
				glfwGetWindowPos(wind, &lastPosX, &lastPosY);


				//auto monitor = glfwGetPrimaryMonitor();
				auto monitor = getCurrentMonitor(wind);


				const GLFWvidmode* mode = glfwGetVideoMode(monitor);

				// switch to full screen
				glfwSetWindowMonitor(wind, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

				currentFullScreen = 1;

			} else {
				//glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); //
				glfwSetWindowMonitor(wind, nullptr, lastPosX, lastPosY, lastW, lastH, 0);

				currentFullScreen = 0;
			}
		}

#pragma endregion
#pragma region reset flags
		scrollSize = 0;
		mouseMovedFlag = 0;
		platform::internal::updateAllButtons(deltaTime);
		platform::internal::resetTypedInput();

#pragma endregion

		// Rendering
		ImGui::Render();
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(wind);
		glfwPollEvents();
	}

#pragma region cleanup

	gameEnd();

defer:
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	if (wind) {
		glfwDestroyWindow(wind);
		glfwTerminate();
	}
#pragma endregion
	return result;
}
