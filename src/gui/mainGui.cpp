#include "mainGui.hpp"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <iostream>
#include <chrono>

static GLFWwindow* window = nullptr;

void guiLoop() {
	auto lastRefreshTime = std::chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();



		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
	}
}

bool guiInit() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	window = glfwCreateWindow(1280, 720, "Graph calculator", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1); // Enable vsync
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}
	// enable error reporting
#ifndef NDEBUG
	enableReportGlErrors();
#endif

	return true;
}

void guiCleanup() {
	if (window) {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
}
