#include "arenaAllocator.hpp"
#include "graphMain.hpp"
#include "JITcompiler.hpp"
#include "mainGui.hpp"
#include "parser.hpp"
#include "platformInput.h"
#include <array>
#include <chrono>
#include <cmath> // Include for std::log10 and std::floor
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iostream>
#include <random>
#include <vector>

#pragma region defines

static constexpr float mouseSensitivity = 60;
static constexpr float scrollSensitivity = 30;

static constexpr float initialNumPoints = 100;

struct GLBufferInfo {
	GLuint id = 0;
	size_t amount = 0;
};

struct GraphEquation {
	std::string input = "";
	calcFunction func = nullptr;
	GLBufferInfo vboObj;
	glm::vec3 color = {0.0f, 0.0f, 0.0f};
};

#pragma endregion
#pragma region shader source
static const char* const vertexShaderSource =
	"#version 330 core\n"
	"\n"
	"layout(location = 0) in vec2 position; // Vertex position\n"
	"\n"
	"void main() {\n"
	"    gl_Position = vec4(position, 0.0, 1.0); // Pass through to clip space\n"
	"}\n";

static const char* const geometryShaderSource =
	"#version 330 core\n"
	"\n"
	"layout(lines) in; // Input: lines\n"
	"layout(triangle_strip, max_vertices = 4) out; // Output: quad vertices\n"
	"\n"
	"uniform float lineThickness; // Uniform line thickness\n"
	"\n"
	"void main() {\n"
	"    // Get the positions of the input line vertices\n"
	"    vec2 p0 = gl_in[0].gl_Position.xy; // First vertex position\n"
	"    vec2 p1 = gl_in[1].gl_Position.xy; // Second vertex position\n"
	"\n"
	"    // Calculate the line direction and normal\n"
	"    vec2 lineDir = normalize(p1 - p0);\n"
	"    vec2 lineNormal = vec2(-lineDir.y, lineDir.x); // Perpendicular vector\n"
	"\n"
	"    // Offset positions to create the quad\n"
	"    vec2 offset = lineNormal * lineThickness * 0.5;\n"
	"\n"
	"    // Emit vertices for the quad\n"
	"    gl_Position = vec4(p0 - offset, 0.0, 1.0); // Bottom-left\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = vec4(p0 + offset, 0.0, 1.0); // Top-left\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = vec4(p1 - offset, 0.0, 1.0); // Bottom-right\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = vec4(p1 + offset, 0.0, 1.0); // Top-right\n"
	"    EmitVertex();\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";
static const char* const fragmentShaderSource = "#version 330 core\n"
												"\n"
												"uniform vec4 lineColor; // Color uniform\n"
												"out vec4 fragColor;     // Output color\n"
												"\n"
												"void main() {\n"
												"    fragColor = lineColor; // Set the fragment color\n"
												"}\n";

#pragma endregion
#pragma region globals
static GLint lineThicknessUniform;
static GLint lineColorUniform;
static GLuint shaderProgram;
static std::array<GLBufferInfo, 3> gridVaos;
static GLuint gridVbo;
static std::vector<GraphEquation> graphEquations;

// use std::vector to allow dynamic amount of equations
static glm::vec2 origin = {0, 0};
static float scale = 1;

#pragma endregion
#pragma region generate VBOS

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

	std::vector<float, ArenaAllocator<float>> verticesThin;
	std::vector<float, ArenaAllocator<float>> verticesMedium;
	std::array<float, 8> verticesThick{};

	verticesMedium.reserve(desiredLines * 0.2);
	verticesThin.reserve(desiredLines * 0.8);

	// Generate vertical lines in world space

	for (float x = xStart; x <= worldMaxX; x += worldSpacing) {
		// Convert from function space to NDC
		float ndcX = (x - origin.x) * scale;

		if (x == 0) {
			verticesThick[0] = ndcX;
			verticesThick[1] = screenMinY;
			verticesThick[2] = ndcX;
			verticesThick[3] = screenMaxY;
		} else if (static_cast<int>(x / worldSpacing) % 5 != 0) {
			verticesThin.push_back(ndcX);		// x1
			verticesThin.push_back(screenMinY); // y1
			verticesThin.push_back(ndcX);		// x2
			verticesThin.push_back(screenMaxY); // y2
		} else {
			verticesMedium.push_back(ndcX);		  // x1
			verticesMedium.push_back(screenMinY); // y1
			verticesMedium.push_back(ndcX);		  // x2
			verticesMedium.push_back(screenMaxY); // y2
		}
	}

	// Generate horizontal lines in world space
	for (float y = yStart; y <= worldMaxY; y += worldSpacing) {
		// Correctly calculate ndcY using the origin and scale
		float ndcY = (-y + origin.y) * scale; // Use the original y value and adjust correctly

		if (y == 0) {
			verticesThick[4] = screenMinX;
			verticesThick[5] = ndcY;
			verticesThick[6] = screenMaxX;
			verticesThick[7] = ndcY;
		} else if (static_cast<int>(y / worldSpacing) % 5 != 0) {
			verticesThin.push_back(screenMinX); // x1
			verticesThin.push_back(ndcY);		// y1
			verticesThin.push_back(screenMaxX); // x2
			verticesThin.push_back(ndcY);		// y2
		} else {
			verticesMedium.push_back(screenMinX); // x1
			verticesMedium.push_back(ndcY);		  // y1
			verticesMedium.push_back(screenMaxX); // x2
			verticesMedium.push_back(ndcY);		  // y2
		}
	} 
	
	const size_t thinSize = verticesThin.size() * sizeof(float);
	const size_t mediumSize = verticesMedium.size() * sizeof(float);
	const size_t thickSize = verticesThick.size() * sizeof(float);
	const size_t totalSize = thinSize + mediumSize + thickSize;

	// Create buffer and allocate space for all vertices
	glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
	glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

	// Upload each part directly into the buffer
	size_t offset = 0;
	
	assert(!verticesThin.empty());
	glBufferSubData(GL_ARRAY_BUFFER, offset, thinSize, verticesThin.data());
	offset += thinSize;

	assert(!verticesMedium.empty());
	glBufferSubData(GL_ARRAY_BUFFER, offset, mediumSize, verticesMedium.data());
	offset += mediumSize;
	
	assert(!verticesThick.empty());
	glBufferSubData(GL_ARRAY_BUFFER, offset, thickSize, verticesThick.data());

	// Data for VAOs (amount of lines and offset in floats)
	const std::array<std::pair<size_t, size_t>, 3> vaoData = {
		{{verticesThin.size() / 2, 0},
		 {verticesMedium.size() / 2, verticesThin.size()},
		 {verticesThick.size() / 2, verticesThin.size() + verticesMedium.size()}}};
	static_assert(gridVaos.size() == vaoData.size());

	// Iterate over VAOs and set them up
	for (int i = 0; i < gridVaos.size(); ++i) {
		glBindVertexArray(gridVaos[i].id);
		glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
		gridVaos[i].amount = vaoData[i].first; // Set the amount
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(vaoData[i].second * sizeof(float)));
		glEnableVertexAttribArray(0);
	}
	glBindVertexArray(0);
}

// Function to generate vertex data for the graph
void generateGraphData(const calcFunction& func, GLBufferInfo& vboObject,
					   std::vector<float, ArenaAllocator<float>>& vertexData,
					   int numPoints = static_cast<int>(initialNumPoints / std::sqrt(scale))) {
	vertexData.reserve(numPoints);
	if (func == nullptr) {
		return;
	}
	vertexData.clear();
	// for small scale(zoom out) this needs more precision
	// but just doing / scale gives 10000 vertexes, so no

	float step = 2.0f / numPoints; // Step through X space from -1 to 1

	for (int j = 0; j <= numPoints; ++j) {
		float normalizedX = -1.0f + j * step;		// Generate normalized X in the range [-1, 1]
		float x = (normalizedX / scale) + origin.x; // Apply scaling (zoom) to X

		// Evaluate the function at the scaled X value
		float y = func(x); // Evaluate the function

		float scaledY = (y + origin.y) * scale; // Apply scaling (zoom) to the Y value
		vertexData.push_back(normalizedX);		// Keep normalized X for OpenGL [-1, 1] range
		vertexData.push_back(scaledY);			// Scaled Y
	}

	if (vboObject.id == 0) {
		glGenBuffers(1, &vboObject.id); // Generate the VBO only if it doesn't exist
	}
	glBindBuffer(GL_ARRAY_BUFFER, vboObject.id);
	vboObject.amount = vertexData.size() / 2;
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
}

#pragma endregion
#pragma region color gen

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
		const int prime = 31;						 // A small prime number
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

#pragma endregion
#pragma region set function and color

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
	std::vector<float, ArenaAllocator<float>> vertexData;
	generateGraphData(graph.func, graph.vboObj, vertexData);

	return true;
}

#pragma endregion
#pragma region imGui callbacks

// Custom callback function to call setGraph when input changes
int inputTextCallback(ImGuiInputTextCallbackData* data) {
	size_t index = reinterpret_cast<size_t>(data->UserData) - 1;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		graphEquations[index].input = data->Buf;
		setGraph(graphEquations[index], index); // Update the graph with both inputs
	}
	return 0;
}

#pragma endregion
#pragma region mainSuff

bool gameLogic(float deltaTime, int w, int h) {
	glClear(GL_COLOR_BUFFER_BIT); // Clear screen

	bool shouldRecalculateEverything = false;

#pragma region draw grid using shader
	glUniform4f(lineColorUniform, 0.1f, 0.1f, 0.1f, 1.0f);
	for (int i = 0; i < 3; i++) {
		glUniform1f(lineThicknessUniform, (2 * i + 1) / 1000.0f);
		glEnableVertexAttribArray(0);
		glBindVertexArray(gridVaos[i].id);
		glDrawArrays(GL_LINES, 0, gridVaos[i].amount);
		glDisableVertexAttribArray(0);
	}
#pragma endregion
#pragma region draw graphs
	// Draw graph for each function
	glUniform1f(lineThicknessUniform, 8.0f / 1000.0f);
	for (const auto& graph : graphEquations) {

		glBindBuffer(GL_ARRAY_BUFFER, graph.vboObj.id);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, nullptr); // Set up vertex pointer
		glUniform4f(lineColorUniform, graph.color.x, graph.color.y, graph.color.z,
					1.0f);									 // Set different color for each function
		glDrawArrays(GL_LINE_STRIP, 0, graph.vboObj.amount); // Draw the line strip
		glDisableClientState(GL_VERTEX_ARRAY);
	}
// glUseProgram(0);
#pragma endregion
#pragma region display equations widget
	ImGui::Begin("Equations", nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
	for (size_t i = 0; i < graphEquations.size(); i++) {
		ImGui::InputText(("##" + std::to_string(i)).c_str(), &graphEquations[i].input, ImGuiInputTextFlags_CallbackEdit,
						 inputTextCallback, (void*)(i + 1));
	}
	if (ImGui::Button("add equation", {100.0f, 25.0f})) {
		graphEquations.resize(graphEquations.size() + 1);
	}
	shouldRecalculateEverything |= ImGui::SliderFloat("Scale", &scale, 0.001f, 10.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginX", &origin.x, -5.0f, 5.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginY", &origin.y, -5.0f, 5.0f);
	ImGui::End();
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
		origin = originOrigin + (delta / scale) / glm::vec2({w, h}); // Scale and update the origin

		shouldRecalculateEverything = true;
	}

	double scrollSize = platform::getScrollSize();
	if (scrollSize != 0) {
		scale *= exp(scrollSize / scrollSensitivity);
		scale = std::clamp(scale, 0.001f, 1000.0f);
		shouldRecalculateEverything = true;
	}

#pragma endregion
	// reset early
	if (shouldRecalculateEverything) {
		generateAxisData();
		arena_reset(&global_arena); // early reset cause this requires alot of vertexes
		std::vector<float, ArenaAllocator<float>> vertexData;
		for (GraphEquation& graph : graphEquations) {
			generateGraphData(graph.func, graph.vboObj, vertexData);
		}
	}

#pragma region fullscreen
/*
	if (platform::isButtonPressedOn(platform::Button::F11)) {
		if (platform::isFullScreen()) {
			platform::setFullScreen(false);
		} else {
			platform::setFullScreen(true);
		}
	}
	*/
#pragma endregion

#pragma region clearTerminal
	if (platform::isButtonPressedOn(platform::Button::D)) {
		platform::clearTerminal();
	}

#pragma endregion


	arena_reset(&global_arena);
	return true;
}

bool gameInit() {
	glClearColor(1.0f, 1.0f, 1.0f, 0.5f);

	glGenBuffers(1, &gridVbo);
	for (auto& gridVao : gridVaos) {
		glGenVertexArrays(1, &gridVao.id);
	}
#pragma region shader init
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(geometryShader, 1, &geometryShaderSource, nullptr);
	glCompileShader(geometryShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, geometryShader);
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);

	lineThicknessUniform = glGetUniformLocation(shaderProgram, "lineThickness");
	lineColorUniform = glGetUniformLocation(shaderProgram, "lineColor");
#pragma endregion

	graphEquations.resize(1);
	graphEquations[0].input = "x * x";
	graphEquations[0].func = [](double x) { return x * x; };
	graphEquations[0].color = generateColor(0);
	std::vector<float, ArenaAllocator<float>> vertexData;
	generateGraphData(graphEquations[0].func, graphEquations[0].vboObj, vertexData);
	generateAxisData();

	glUseProgram(shaderProgram);
	return true;
}

void gameEnd() {

	// there is no reasone to free all of these since the OS does this for us
	// It is just here just incase
	return;

	/*
	glUseProgram(0);
	for (const auto& graph : graphEquations) {
		if (graph.vboObj.id != 0) {
			glDeleteBuffers(1, &graph.vboObj.id);
		}
	}
	for (const auto& gridVao : gridVaos) {
		if (gridVao.id != 0) {
			glDeleteVertexArrays(1, &gridVao.id);
		}
	}
	glDeleteBuffers(1, &gridVbo);

	glDeleteProgram(shaderProgram);
	*/
}

#pragma endregion