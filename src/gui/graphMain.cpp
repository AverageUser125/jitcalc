#include "graphMain.hpp"
#include "platformInput.h"
#include "mainGui.hpp"
#include "parser.hpp"
#include "JITcompiler.hpp"
#include <iostream>
#include <glad/glad.h>
#include <vector>
#include <imgui.h>

GLuint vbo;
std::vector<float> vertexData;

// Function to generate vertex data for the graph
void generateGraphData(const std::function<double(double)>& func, float startX, float endX, int numPoints) {
	vertexData.clear();
	float step = (endX - startX) / numPoints;

	for (int i = 0; i <= numPoints; ++i) {
		float x = startX + i * step;
		float y = func(x); // Call the JIT-compiled function
		vertexData.push_back(x);
		vertexData.push_back(y);
	}
}

void setupVBO() {
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
}

void drawGraph() {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, nullptr); // Set up vertex pointer

	// Set the color for the line
	glColor3f(0.0f, 1.0f, 0.0f); // Set line color to green (R, G, B)

	glDrawArrays(GL_LINE_STRIP, 0, vertexData.size() / 2); // Draw the line strip

	glDisableClientState(GL_VERTEX_ARRAY);
}

std::function<double(double)> func;

bool gameInit() {
	return true;
	std::string input; // Example function

	std::getline(std::cin, input);

	Parser parser(input);
	ExpressionNode* tree = parser.parserParseExpression();
	parser.parserDebugDumpTree(tree);

	JITCompiler jit;
	func = jit.compile(tree); // Compile the function

	generateGraphData(func, -2.0f, 2.0f, 100); // Generate points from -2 to 2 with 100 points
	setupVBO();

	return true;
}

bool gameLogic(float deltaTime) {
#pragma region init stuff
	int w = 0;
	int h = 0;
	w = platform::getFrameBufferSizeX(); //window w
	h = platform::getFrameBufferSizeY(); //window h

	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT); //clear screen
#pragma endregion

	ImGui::ShowDemoWindow();

	if (platform::isButtonPressedOn(platform::Button::F11)) {
		if (platform::isFullScreen()) {
			platform::setFullScreen(false);
		} else {
			platform::setFullScreen(true);		
		}
	}

	return true;
}


void gameEnd() {
	return;
}