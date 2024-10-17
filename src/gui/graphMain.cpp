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
#include <random>
#include <chrono>
#include <cmath> // Include for std::log10 and std::floor
#include <array>
#include "arenaAllocator.hpp"

using calcFunc = std::function<double(double)>;

static constexpr float mouseSensitivity = 60;
static constexpr float scrollSensitivity = 30;

static constexpr float initialNumPoints = 100;

struct VertexBufferObject {
	GLuint vbo = 0;
	size_t dataSize = 0;
};

struct GraphEquation {
	std::string input = "";
	calcFunc func = nullptr;
	GLuint vbo = 0;
	size_t dataSize = 0;
	glm::vec3 color = {0.0f, 0.0f, 0.0f};
};


static std::array<VertexBufferObject,2> grid;
static std::vector<GraphEquation> graphEquations;

// use std::vector to allow dynamic amount of equations
static glm::vec2 origin = {0, 0};
static float scale = 1;


void generateAxisData() {
	constexpr auto roundToNearestPowerOf2 = [](float value) { return std::pow(2, std::round(std::log2(value))); };

	constexpr int desiredLines = 65;

	// Set screen-space boundaries in NDC (-1 to 1)
	constexpr float screenMinX = -1.0f;
	constexpr float screenMaxX = 1.0f;
	constexpr float screenMinY = -1.0f;
	constexpr float screenMaxY = 1.0f;

	// Convert NDC to function space, factoring in origin and scale
	const float worldMinX = origin.x + screenMinX / scale;
	const float worldMaxX = origin.x + screenMaxX / scale;
	const float worldMinY = origin.y + screenMinY / scale;
	const float worldMaxY = origin.y + screenMaxY / scale;

	// Calculate the width and height in world space
	const float size = 2 / scale;
	// Calculate world spacing based on the number of lines
	const float worldSpacing = roundToNearestPowerOf2(size / desiredLines);

	// Ensure grid aligns with the real origin (0, 0)
	float xStart = std::floor(worldMinX / worldSpacing) * worldSpacing;
	float yStart = std::floor(worldMinY / worldSpacing) * worldSpacing;

	// ensure in NDC range of [-1, 1]
	if (xStart < -1.0f) {
		xStart += worldSpacing;
	}
	if (yStart < -1.0f) {
		yStart += worldSpacing;
	}
	
	ArenaAllocator<float> tempAllocator;
	std::vector<float, ArenaAllocator<float>> verticesThick(tempAllocator);
	std::vector<float, ArenaAllocator<float>> verticesThin(tempAllocator);
	
	verticesThick.reserve(desiredLines * 0.2);
	verticesThin.reserve(desiredLines * 0.8);

	// Generate vertical lines in world space

	for (float x = xStart; x <= worldMaxX; x += worldSpacing) {
		// Convert from function space to NDC
		float ndcX = (x - origin.x) * scale;

		if (fmod(x / worldSpacing, 5) != 0) {
			verticesThin.push_back(ndcX);		// x1
			verticesThin.push_back(screenMinY); // y1
			verticesThin.push_back(ndcX);		// x2
			verticesThin.push_back(screenMaxY); // y2
		} else {
			verticesThick.push_back(ndcX);		 // x1
			verticesThick.push_back(screenMinY); // y1
			verticesThick.push_back(ndcX);		 // x2
			verticesThick.push_back(screenMaxY); // y2	
		}
	}

	// Generate horizontal lines in world space
	for (float y = yStart; y <= worldMaxY; y += worldSpacing) {
		// Correctly calculate ndcY using the origin and scale
		float ndcY = (-y + origin.y) * scale; // Use the original y value and adjust correctly
		
		if (fmod(y / worldSpacing, 5) != 0) {
			verticesThin.push_back(screenMinX); // x1
			verticesThin.push_back(ndcY);		// y1
			verticesThin.push_back(screenMaxX); // x2
			verticesThin.push_back(ndcY);		// y2
		} else {
			verticesThick.push_back(screenMinX); // x1
			verticesThick.push_back(ndcY);		 // y1
			verticesThick.push_back(screenMaxX); // x2
			verticesThick.push_back(ndcY);		 // y2
		}
	}
	grid[0].dataSize = verticesThin.size();
	grid[1].dataSize = verticesThick.size();
	
	// Transfer data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, grid[0].vbo);
	glBufferData(GL_ARRAY_BUFFER, grid[0].dataSize * sizeof(float), verticesThin.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, grid[1].vbo);
	glBufferData(GL_ARRAY_BUFFER, grid[1].dataSize * sizeof(float), verticesThick.data(), GL_STATIC_DRAW);
}

// Function to generate vertex data for the graph
void generateGraphData(GraphEquation& graph) {
	if (graph.func == nullptr) {
		return;
	}
	std::vector<float, ArenaAllocator<float>> vertexData;

	int numPoints = std::max(initialNumPoints, initialNumPoints / scale);
	float step = 2.0f / numPoints; // Step through X space from -1 to 1
	for (int j = 0; j <= numPoints; ++j) {
		float normalizedX = -1.0f + j * step;		// Generate normalized X in the range [-1, 1]
		float x = (normalizedX / scale) + origin.x; // Apply scaling (zoom) to X

		// Evaluate the function at the scaled X value
		float y = graph.func(x); // Evaluate the function

		float scaledY = (y + origin.y) * scale; // Apply scaling (zoom) to the Y value
		vertexData.push_back(normalizedX);	// Keep normalized X for OpenGL [-1, 1] range
		vertexData.push_back(scaledY);	  // Scaled Y
	}

	if (graph.vbo == 0) {
		glGenBuffers(1, &graph.vbo); // Generate the VBO only if it doesn't exist
	}
	glBindBuffer(GL_ARRAY_BUFFER, graph.vbo);
	graph.dataSize = vertexData.size();
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
	
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

bool setGraph(GraphEquation& graph, int index) {
	{
		// the tokens have a string_view to a member string of the lexer
		// therfore you cannot call the destructor on the lexer before the parser has finished
		Lexer lexer(graph.input);
		std::optional<std::vector<Token, ArenaAllocator<Token>>> tokenArrayOpt = lexer.lexerLexAllTokens();
			// lexer.lexerDebugPrintArray(*tokenArrayOpt);
	

		if (!tokenArrayOpt.has_value()) {
			return false;
		}

		Parser parser(*tokenArrayOpt);
		// lifetime of tree pointer is the same as the parser object lifetime
		ExpressionNode* tree = parser.parserParseExpression();
		// parser.parserDebugDumpTree(tree);
		if (parser.hasError) {
			return false;
		}

		JITCompiler jit;
		graph.func = jit.compile(tree); // Compile the function
	}

	graph.color = generateColor(index);

	generateGraphData(graph);	
	
	return true;
}

// Custom callback function to call setGraph when input changes
int inputTextCallback(ImGuiInputTextCallbackData* data) {
	size_t index = reinterpret_cast<size_t>(data->UserData) - 1;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		graphEquations[index].input = data->Buf;
		setGraph(graphEquations[index], index); // Update the graph with both inputs
	}
	return 0;
}

bool gameInit() {
	glClearColor(1.0f, 1.0f, 1.0f, 0.5f);

	glGenBuffers(1, &grid[0].vbo);
	glGenBuffers(1, &grid[1].vbo);

	graphEquations.resize(1);
	graphEquations[0].input = "x * x";
	graphEquations[0].func = [](double x) { return x * x; };
	graphEquations[0].color = generateColor(0);
	generateGraphData(graphEquations[0]);
	generateAxisData();
	return true;
}

bool gameLogic(float deltaTime, int w, int h) {
	glClear(GL_COLOR_BUFFER_BIT); // Clear screen

	bool shouldRecalculateEverything = false;

	#pragma region draw vertexdata
	for (int i = 0; i < grid.size(); i++) {
		glLineWidth(1.5 * i + 1);
		glColor3f(0.01f, 0.01f, 0.01f);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, grid[i].vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glDrawArrays(GL_LINES, 0, grid[i].dataSize / 2);
		glDisableVertexAttribArray(0);
	}
	// Draw graph for each function
	glLineWidth(4.0f);
	for (const auto& graph : graphEquations) {

		glBindBuffer(GL_ARRAY_BUFFER, graph.vbo);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, nullptr); // Set up vertex pointer
		// Set the color for the line
		glm::vec3 color = graph.color;
		glColor3f(color.x, color.y, color.z); // Set different color for each function
		glDrawArrays(GL_LINE_STRIP, 0, graph.dataSize / 2); // Draw the line strip
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	#pragma endregion
	#pragma region move with cursor
	// Move with cursor
	static glm::ivec2 originMousePos = {0, 0};
	static glm::vec2 originOrigin = {0, 0};

	if (platform::isRMousePressed()) {
		originOrigin = origin;
		// originMousePos = static_cast<glm::vec2>(platform::getRelMousePosition()) / glm::vec2({w, h});
		originMousePos = platform::getRelMousePosition();
	}
	if (platform::isRMouseHeld()) {
		glm::ivec2 currentMousePos = platform::getRelMousePosition();
		glm::vec2 delta = 2.0f * static_cast<glm::vec2>(originMousePos - currentMousePos); // Delta in pixels
		origin = originOrigin + (delta / scale) / glm::vec2({w, h});				// Scale and update the origin

		shouldRecalculateEverything = true;
	}

	double scrollSize = platform::getScrollSize();
	if (scrollSize != 0) {
		scale *= exp(scrollSize / scrollSensitivity);
		scale = std::clamp(scale, 0.001f, 1000.0f);
		shouldRecalculateEverything = true;
	}
#pragma endregion
    #pragma region display equations
	ImGui::Begin("Equations", nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
	for (size_t i = 0; i < graphEquations.size(); i++) {
		ImGui::InputText(("##" + std::to_string(i)).c_str(), 
			&graphEquations[i].input, 
			ImGuiInputTextFlags_CallbackEdit, inputTextCallback,
			(void*)(i + 1));
	}
	if (ImGui::Button("add equation", {100.0f, 25.0f})) {
		graphEquations.resize(graphEquations.size() + 1);
	}
	shouldRecalculateEverything |= ImGui::SliderFloat("Scale", &scale, 0.001f, 10.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginX", &origin.x, -5.0f, 5.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginY", &origin.y, -5.0f, 5.0f);
	ImGui::End();
#pragma endregion
	if (shouldRecalculateEverything) {
		generateAxisData();
		for (GraphEquation& graph : graphEquations)
			generateGraphData(graph);
	}
    #pragma region fullscreen
	if (platform::isButtonPressedOn(platform::Button::F11)) {
		if (platform::isFullScreen()) {
			platform::setFullScreen(false);
		} else {
			platform::setFullScreen(true);
		}
	}
    #pragma endregion

	#pragma region clearTerminal
	if (platform::isButtonPressedOn(platform::Button::D)) {
		platform::clearTerminal();
	}

	#pragma endregion


	arena_reset(&global_arena);
	return true;
}

void gameEnd() {
	for (const auto& graph : graphEquations) {
		if (graph.vbo != 0) {
			glDeleteBuffers(1, &graph.vbo);
		}
	}
}
