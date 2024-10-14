#include "graphMain.hpp"
#include "platformInput.h"
#include "mainGui.hpp"
#include "parser.hpp"
#include "JITcompiler.hpp"
#include <iostream>
#include <glad/glad.h>
#include <vector>
#include <imgui.h>
#include <imgui_stdlib.h>

static constexpr int mouseSensitivity = 30;

static GLuint vbo;
static std::vector<float> vertexData;
static std::string input = "x*x";
static std::function<double(double)> func;
static glm::vec2 origin = {0, 0};
static float scale = 1;

// Function to generate vertex data for the graph
void generateGraphData() {
	vertexData.clear();

	int numPoints = std::max(100.0f, 100 / scale);
	// We want X to go from -1 to 1, with scaling applied
	float step = 2.0f / numPoints; // Step through X space from -1 to 1
	for (int i = 0; i <= numPoints; ++i) {
		// Generate normalized X in the range [-1, 1]
		float normalizedX = -1.0f + i * step;

		// Apply scaling (zoom) to X and Y
		float x = (normalizedX / scale) + origin.x;

		// Evaluate the function at the scaled X value
		float y = func(x);

		// Apply scaling (zoom) to the Y value
		float scaledY = (y / scale) + origin.y;

		// Store the scaled X and Y coordinates
		vertexData.push_back(normalizedX); // Keep normalized X for OpenGL [-1, 1] range
		vertexData.push_back(scaledY);	   // Scaled Y
	}

	if (vbo == 0) {
		glGenBuffers(1, &vbo); // Generate the VBO only if it doesn't exist
	}
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
}

void drawAxis() {

}

bool setGraph(const std::string& equation) {
	Parser parser(equation);
	ExpressionNode* tree = parser.parserParseExpression();
	parser.parserDebugDumpTree(tree);
	if (parser.hasError) {
		return false;
	}
	JITCompiler jit;
	func = jit.compile(tree); // Compile the function

	generateGraphData();
	return true;
}

bool gameInit() {
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	setGraph(input);

	return true;
}

// Custom callback function to call setGraph when input changes
int inputTextCallback(ImGuiInputTextCallbackData* data) {
	// InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		// Trigger graph update whenever the text is modified
		if (data->Buf == nullptr || (*data->Buf) == '/0')
			return 0;
		setGraph(data->Buf);   // Update the graph
	}
	return 0;
}

bool gameLogic(float deltaTime) {
#pragma region init stuff
	glClear(GL_COLOR_BUFFER_BIT); //clear screen
#pragma endregion

	drawAxis();
#pragma region draw graph
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, nullptr); // Set up vertex pointer

	// Set the color for the line
	glColor3f(0.0f, 1.0f, 0.0f); // Set line color to green (R, G, B)

	glDrawArrays(GL_LINE_STRIP, 0, vertexData.size() / 2); // Draw the line strip

	glDisableClientState(GL_VERTEX_ARRAY);
#pragma endregion
	


#pragma region move with cursor

	static glm::ivec2 originMousePos = {0, 0};
	static glm::ivec2 originOrigin = {0, 0};

	if (platform::isLMousePressed()) {
		originOrigin = origin;
		originMousePos = platform::getRelMousePosition();
	}
	if (platform::isLMouseHeld()) {
		glm::ivec2 currentMousePos = platform::getRelMousePosition(); 
		origin = (int)scale * (originMousePos - currentMousePos) / mouseSensitivity + originOrigin;
	}

	double scrollSize = platform::getScrollSize();
	scale += scrollSize;

#pragma endregion
	//ImGui::ShowDemoWindow();
	ImGui::Begin("Equation", nullptr, ImGuiWindowFlags_NoTitleBar);
	ImGui::InputText("equ", &input, ImGuiInputTextFlags_CallbackEdit, inputTextCallback);


	if (ImGui::SliderFloat("Scale", &scale, 0.01, 10) || 
		ImGui::SliderFloat("OriginX", &origin.x, -5, 5) || 
		ImGui::SliderFloat("OriginY", &origin.y, -5, 5)) {
		generateGraphData();
	}
	ImGui::End();

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
	if (vbo != 0) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	return;
}