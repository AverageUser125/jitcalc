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
#include <chrono>

using calcFunc = std::function<double(double)>;

static constexpr float mouseSensitivity = 60;
static constexpr float scrollSensitivity = 30;

static constexpr int amount = 3;
static constexpr float initialNumPoints = 100;

static std::vector<GLuint> vbos(amount);						  // Store two VBOs
static std::vector<std::vector<float>> vertexData(amount);				 // Store vertex data for two functions
static std::vector<std::string> inputs = {"x*x", "x^3-0.1", "x"}; // Input for the equations
static std::vector<calcFunc> funcs(amount);								  // Store two functions
static std::vector<glm::vec3> colors(amount);
static glm::vec2 origin = {0, 0};
static float scale = 1;

// Function to generate vertex data for the graph
void generateGraphData() {
	for (size_t i = 0; i < funcs.size(); ++i) {
		if (funcs[i] == nullptr) {
			break;
		}
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

float getDoubleRand() {
	std::mt19937_64 rng;
	// initialize the random number generator with time-dependent seed
	uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};
	rng.seed(ss);
	std::uniform_real_distribution<double> unif(0, 1);
	return unif(rng);
}

// TODO: reconsider the entire method of this function
glm::vec3 generateColor(int index) {
	static float rndStart = getDoubleRand();
	// Helper lambda to convert HSV to RGB
	static const auto hsvToRgb = [](float h, float s, float v) -> glm::vec3 {
		float c = v * s;
		float x = c * (1 - std::fabs(fmod(h / 60.0f, 2) - 1));
		float m = v - c;

		float r = 0, g = 0, b = 0;

		if (h >= 0 && h < 60) {
			r = c;
			g = x;
			b = 0;
		} else if (h >= 60 && h < 120) {
			r = x;
			g = c;
			b = 0;
		} else if (h >= 120 && h < 180) {
			r = 0;
			g = c;
			b = x;
		} else if (h >= 180 && h < 240) {
			r = 0;
			g = x;
			b = c;
		} else if (h >= 240 && h < 300) {
			r = x;
			g = 0;
			b = c;
		} else if (h >= 300 && h < 360) {
			r = c;
			g = 0;
			b = x;
		}

		return glm::vec3(r + m, g + m, b + m);
	};

	// Simple hash function to generate consistent pseudo-random values based on the index
	static const auto hash = [](int idx) -> float {
		const int prime = 31;							   // A small prime number
		return fmod((idx * prime * rndStart), 1.0f); // Modulo 1 to get a float between 0 and 1
	};

	// Generate hue using hash based on the index
	float hue = 360.0f * hash(index);

	// For even indexes, generate a hue opposite to the previous index
	if (index > 0 && index % 2 == 0) {
		float previousHue = 360.0f * hash(index - 1); // Get the hue for the previous index
		hue = fmod(previousHue + 180.0f, 360.0f);	  // Opposite hue
	}

	// Full saturation and value for fully saturated colors
	float saturation = 1.0f;
	float value = 1.0f;

	// Return the RGB color generated from the HSV values
	return hsvToRgb(hue, saturation, value);
}

bool setGraph(const std::vector<std::string>& equations) {
	for (size_t i = 0; i < equations.size(); ++i) {
		Parser parser(equations[i]);
		ExpressionNode* tree = parser.parserParseExpression();
		parser.parserDebugDumpTree(tree);
		if (parser.hasError) {
			return false;
		}

		JITCompiler jit;
		funcs[i] = jit.compile(tree); // Compile the function

		colors[i] = generateColor(i);
	}

	generateGraphData();
	return true;
}

// Custom callback function to call setGraph when input changes
int inputTextCallback(ImGuiInputTextCallbackData* data) {
	size_t index = reinterpret_cast<size_t>(data->UserData) - 1;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		// Trigger graph update whenever the text is modified
		std::vector<std::string> inputsClone = inputs;
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
		glm::vec3 color = colors[i];
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
		origin = static_cast<glm::vec2>(originMousePos - currentMousePos) / mouseSensitivity + originOrigin;
		shouldRecalculateEverything = true;
	}

	double scrollSize = platform::getScrollSize();
	if (scrollSize != 0) {
		scale -= scrollSize / scrollSensitivity;
		scale = std::clamp(scale, 0.001f, 1000.0f);
		shouldRecalculateEverything = true;
	}
#pragma endregion

#pragma region display equations
	ImGui::Begin("Equations", nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
	for (size_t i = 0; i < inputs.size(); i++) {
		ImGui::InputText(("##" + std::to_string(i)).c_str(), 
			&inputs[i], 
			ImGuiInputTextFlags_CallbackEdit, inputTextCallback,
			(void*)(i + 1));
	}
	if (ImGui::Button("add equation", {100.0f, 25.0f})) {
		inputs.resize(inputs.size() + 1);
		funcs.resize(funcs.size() + 1);
		vbos.resize(vbos.size() + 1);
		vertexData.resize(vertexData.size() + 1);
		colors.resize(colors.size() + 1);
	}
	shouldRecalculateEverything |= ImGui::SliderFloat("Scale", &scale, 0.001f, 10.0f);
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
