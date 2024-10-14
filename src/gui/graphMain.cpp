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
#include <array>
#include <random>

using calcFunc = std::function<double(double)>;

static constexpr float mouseSensitivity = 60;
static constexpr float scrollSensitivity = 30;

static constexpr int amount = 3;
static constexpr float initialNumPoints = 100;

static std::array<GLuint, amount> vbos;						   // Store two VBOs
static std::array<std::vector<float>, amount> vertexData;	   // Store vertex data for two functions
static std::array<std::string, amount> inputs = {"x*x", "x^3-0.1", "x"}; // Input for the equations
static std::array<calcFunc, amount> funcs;							// Store two functions
static glm::vec2 origin = {0, 0};
static float scale = 1;

// Function to generate vertex data for the graph
void generateGraphData() {
	for (size_t i = 0; i < funcs.size(); ++i) {
		vertexData[i].clear(); // Clear vertex data for the current function

		int numPoints = std::max(initialNumPoints, initialNumPoints / scale);
		float step = 2.0f / numPoints; // Step through X space from -1 to 1
		for (int j = 0; j <= numPoints; ++j) {
			float normalizedX = -1.0f + j * step;		// Generate normalized X in the range [-1, 1]
			float x = (normalizedX / scale) + origin.x; // Apply scaling (zoom) to X

			// Evaluate the function at the scaled X value
			float y = funcs[i](x); // Evaluate the function

			float scaledY = (y * scale) + origin.y; // Apply scaling (zoom) to the Y value
			vertexData[i].push_back(normalizedX);	// Keep normalized X for OpenGL [-1, 1] range
			vertexData[i].push_back(scaledY);	  // Scaled Y
		}

		if (vbos[i] == 0) {
			glGenBuffers(1, &vbos[i]); // Generate the VBO only if it doesn't exist
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
		glBufferData(GL_ARRAY_BUFFER, vertexData[i].size() * sizeof(float), vertexData[i].data(), GL_STATIC_DRAW);
	}
}

void drawAxis() {
	// Axis drawing logic can be added here
}

bool setGraph(const std::array<std::string, amount>& equations) {
	for (size_t i = 0; i < equations.size(); ++i) {
		Parser parser(equations[i]);
		ExpressionNode* tree = parser.parserParseExpression();
		parser.parserDebugDumpTree(tree);
		if (parser.hasError) {
			return false;
		}

		JITCompiler jit;
		funcs[i] = jit.compile(tree); // Compile the function
	}

	generateGraphData();
	return true;
}

// Custom callback function to call setGraph when input changes
int inputTextCallback(ImGuiInputTextCallbackData* data) {
	size_t index = reinterpret_cast<size_t>(data->UserData) - 1;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		// Trigger graph update whenever the text is modified
		std::array<std::string, inputs.size()> inputsClone = inputs;
		inputsClone[index] = data->Buf;
		setGraph(inputsClone); // Update the graph with both inputs
	}
	return 0;
}

bool gameInit() {
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	glClearColor(0.01f, 0.01f, 0.01f, 0.1f);
	setGraph(inputs); // Initial call with both inputs

	return true;
}

glm::vec3 getColor(size_t index) {
	static size_t currentTime = time(nullptr);
	std::mt19937 generator(index + currentTime);									// Mersenne Twister PRNG seeded with the index
	std::uniform_real_distribution<float> distribution(0.0f, 1.0f); // Distribution between 0 and 1

	// Ensure saturation is maximum
	float s = 1.0f;
	float v = 1.0f;
	float h = distribution(generator);
	// Convert HSV to RGB
	glm::vec3 rgb(0.0f);
	int i = static_cast<int>(h * 6); // Sector 0 to 5
	float f = h * 6 - i;			 // Factor in the current sector
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);

	i = i % 6; // Wrap the sector index

	// Use glm::vec3 operations to set the RGB values based on the sector
	switch (i) {
	case 0:
		rgb = glm::vec3(v, t, p);
		break; // Red
	case 1:
		rgb = glm::vec3(q, v, p);
		break; // Yellow
	case 2:
		rgb = glm::vec3(p, v, t);
		break; // Green
	case 3:
		rgb = glm::vec3(p, q, v);
		break; // Cyan
	case 4:
		rgb = glm::vec3(t, p, v);
		break; // Blue
	case 5:
		rgb = glm::vec3(v, p, q);
		break; // Magenta
	}

	return rgb; // Return the RGB color
}

bool gameLogic(float deltaTime) {
	glClear(GL_COLOR_BUFFER_BIT); // Clear screen

	bool shouldRecalculateEverything = false;
	drawAxis();

	#pragma region draw vertexdata
	// Draw graph for each function
	for (size_t i = 0; i < vertexData.size(); ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, nullptr); // Set up vertex pointer
		
		// Set the color for the line
		glm::vec3 color = getColor(i);
		glColor3f(color.x, color.y, color.z); // Set different color for each function

		glDrawArrays(GL_LINE_STRIP, 0, vertexData[i].size() / 2); // Draw the line strip

		glDisableClientState(GL_VERTEX_ARRAY);
	}
	#pragma endregion
	#pragma region move with cursor
	// Move with cursor
	static glm::ivec2 originMousePos = {0, 0};
	static glm::vec2 originOrigin = {0, 0};

	if (platform::isRMousePressed()) {
		originOrigin = origin;
		originMousePos = platform::getRelMousePosition();
	}
	if (platform::isRMouseHeld()) {
		glm::ivec2 currentMousePos = platform::getRelMousePosition();
		origin = scale * static_cast<glm::vec2>(originMousePos - currentMousePos) / mouseSensitivity + originOrigin;
		shouldRecalculateEverything = true;
	}

	double scrollSize = platform::getScrollSize();
	if (scrollSize != 0) {
		scale -= scrollSize / scrollSensitivity;
		scale = std::clamp(scale, 0.01f, 1000.0f);
		shouldRecalculateEverything = true;
	}
#pragma endregion

#pragma region display equations
	ImGui::Begin("Equations", nullptr, ImGuiWindowFlags_NoTitleBar);
	for (size_t i = 0; i < inputs.size(); i++) {
			ImGui::InputText(
				std::to_string(i).c_str(), 
				&inputs[i], 
				ImGuiInputTextFlags_CallbackEdit, inputTextCallback,
				(void*)(i + 1));
	}
	shouldRecalculateEverything |= ImGui::SliderFloat("Scale", &scale, 0.01f, 10.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginX", &origin.x, -5.0f, 5.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginY", &origin.y, -5.0f, 5.0f);
#pragma endregion
	if (shouldRecalculateEverything) {
		generateGraphData();
	}
	ImGui::End();
    #pragma region fullscreen
	if (platform::isButtonPressedOn(platform::Button::F11)) {
		if (platform::isFullScreen()) {
			platform::setFullScreen(false);
		} else {
			platform::setFullScreen(true);
		}
	}
    #pragma endregion

	return true;
}

void gameEnd() {
	for (auto& vbo : vbos) {
		if (vbo != 0) {
			glDeleteBuffers(1, &vbo);
			vbo = 0;
		}
	}
}
