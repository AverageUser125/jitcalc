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
#include <glm/gtc/type_ptr.hpp>
#include "vboAllocator.hpp"
#include "tools.hpp"

#pragma region defines

struct GLBufferInfo {
	GLuint id = 0;
	size_t amount = 0;
};

struct GraphEquation {
	std::string input = "";
	CompiledFunction func{};
	GLBufferInfo vboObj;
	glm::vec3 color = {0.0f, 0.0f, 0.0f};
};

#pragma endregion
#pragma region constants
// https://en.wikipedia.org/wiki/Golden_angle
// https://stackoverflow.com/questions/43044/algorithm-to-randomly-generate-an-aesthetically-pleasing-color-palette
static constexpr float goldenAngle = 137.50776405f;

static constexpr float mouseSensitivity = 60;
static constexpr float scrollSensitivity = 30;

static constexpr float initialNumPoints = 100;
static constexpr float graphThreshold = 0.01f;
#pragma endregion
#pragma region shader source
static const char* const vertexShaderSource =
	"#version 330 core\n"
	"layout(location = 0) in vec2 position;\n"
	"void main() {\n"
	"    gl_Position = vec4(position, 0.0, 1.0);\n"
	"}\n";
static const char* const geometryShaderSource =
	"#version 330 core\n"
	"layout(lines) in;\n"
	"layout(triangle_strip, max_vertices = 4) out;\n"
	"uniform float lineThickness; // Uniform line thickness\n"
	"void main() {\n"
	"    vec2 p0 = gl_in[0].gl_Position.xy;\n"
	"    vec2 p1 = gl_in[1].gl_Position.xy;\n"
	"    vec2 lineDir = normalize(p1 - p0);\n"
	"    vec2 lineNormal = vec2(-lineDir.y, lineDir.x);\n"
	"    vec2 offset = lineNormal * lineThickness * 0.5;\n"
	"    gl_Position = vec4(p0 - offset, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(p0 + offset, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(p1 - offset, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"    gl_Position = vec4(p1 + offset, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n";
static const char* const fragmentShaderSource = 
	"#version 330 core\n"
	"uniform vec4 lineColor; \n"
	"out vec4 fragColor;\n"
	"void main() {\n"
	"    fragColor = lineColor;\n"
	"}\n";
#pragma endregion
#pragma region globals
static GLint lineThicknessUniform = 0;
static GLint lineColorUniform = 0;
static GLuint shaderProgram = 0;

static std::array<GLBufferInfo, 3> gridVaos{};
static GLuint gridVbo = 0;

static std::vector<GraphEquation> graphEquations{};

static VBOAllocator vboAllocator{};

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
	const float invWorldSpacing = 1.0f / worldSpacing;

	// Ensure grid aligns with the real origin (0, 0)
	float xStart = std::floor(worldMinX * invWorldSpacing) * worldSpacing;
	float yStart = std::floor(worldMinY * invWorldSpacing) * worldSpacing;

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

	verticesMedium.reserve(desiredLines * 0.2 * 2 + 2);
	verticesThin.reserve(desiredLines * 0.8 * 2 + 2);

	// Generate vertical lines in world space

	for (float x = xStart; x <= worldMaxX; x += worldSpacing) {
		// Convert from function space to NDC
		float ndcX = (x - origin.x) * scale;

		if (x == 0) {
			verticesThick[0] = ndcX;
			verticesThick[1] = screenMinY;
			verticesThick[2] = ndcX;
			verticesThick[3] = screenMaxY;
		} else if (static_cast<int>(x * invWorldSpacing) % 4 != 0) {
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
		} else if (static_cast<int>(y * invWorldSpacing) % 4 != 0) {
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

	for (int i = 0; i < gridVaos.size(); ++i) {
		glBindVertexArray(gridVaos[i].id);
		glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
		gridVaos[i].amount = vaoData[i].first; // Set the amount
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(vaoData[i].second * sizeof(float)));
		glEnableVertexAttribArray(0);
	}
	glBindVertexArray(0);
}

void clearGraphData(GLBufferInfo& vboObject) {
	// for the case where you created an empty graph and immediatly removed it
	if (vboObject.id != 0) {
		glBindBuffer(GL_ARRAY_BUFFER, vboObject.id);
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
	}
	vboObject.amount = 0;
}

void generateGraphData(const CompiledFunction& func, GLBufferInfo& vboObject,
					   std::vector<glm::vec2, ArenaAllocator<glm::vec2>>& vertexData) {
	if (func == nullptr) {
		return;
	}
	size_t targetNumPoints = static_cast<size_t>(initialNumPoints / std::sqrt(scale));
	vertexData.reserve(targetNumPoints);
	vertexData.clear();

	float step = 2.0f / targetNumPoints;

	float prevX = -1.0f;
	float prevY = func(prevX / scale + origin.x);

	for (int j = 0; j <= targetNumPoints; ++j) {
		float normalizedX = -1.0f + j * step;
		float x = (normalizedX / scale) + origin.x;

		float y = func(x);

		float deltaY = std::abs(y - prevY);

		// If deltaY is large, reduce the step size to add more points for better precision
		if (deltaY > graphThreshold) {
			float refinedStep = step / 10.0f;
			for (float refinedX = prevX + refinedStep; refinedX < normalizedX; refinedX += refinedStep) {
				float refinedFuncX = (refinedX / scale) + origin.x;
				float refinedY = func(refinedFuncX);
				float refinedScaledY = (refinedY + origin.y) * scale;
				vertexData.push_back({refinedX, refinedScaledY});
			}
		}

		float scaledY = (y + origin.y) * scale;
		vertexData.push_back({normalizedX, scaledY});

		prevX = normalizedX;
		prevY = y;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vboObject.id);
	vboObject.amount = vertexData.size();
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(glm::vec2), vertexData.data(), GL_STATIC_DRAW);
}


void generateGraphData(const CompiledFunction& func, GLBufferInfo& vboObject) {
	std::vector<glm::vec2, ArenaAllocator<glm::vec2>> vertexData;
	generateGraphData(func, vboObject, vertexData);
}

#pragma endregion
#pragma region color gen

float getFloatRand() {
	std::mt19937_64 rng;
	// initialize the random number generator with time-dependent seed
	uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};
	rng.seed(ss);
	std::uniform_real_distribution<float> unif(0, 1);
	return unif(rng);
}

// TODO: reconsider the entire method of this function
glm::vec3 generateColor() {
	static int index = 0;
	static float rndStart = fmod(getFloatRand(), 360.0f);
	// Helper lambda to convert HSV to RGB
	static constexpr auto hsvToRgb = [](const float h, const float s, const float v) -> glm::vec3 {
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

	float hue = rndStart;
	hue += index * goldenAngle;
	hue = fmod(hue, 360.0f);

	index++;
	return hsvToRgb(hue, 0.5f, 0.95f);
}

#pragma endregion
#pragma region set function and color

bool setGraph(GraphEquation& graph) {

	if (graph.vboObj.id == 0) {
		graph.vboObj.id = vboAllocator.allocateVBO();
	}

	if (graph.input.empty()) {
		graph.func = nullptr;
		clearGraphData(graph.vboObj);
		return true;
	}

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
		graph.func = jit.compile(tree);
	}

	if (graph.color.x == 0.0f && graph.color.y == 0.0f && graph.color.z == 0.0f) {
		graph.color = generateColor();
	}
	generateGraphData(graph.func, graph.vboObj);

	return true;
}

void removeGraph(int index) {
	assert(!graphEquations.empty());
	assert(0 <= index && index < graphEquations.size());

	GraphEquation& graph = graphEquations[index];

	graph.func = nullptr;
	clearGraphData(graph.vboObj);
	vboAllocator.freeVBO(graph.vboObj.id);
	graphEquations.erase(graphEquations.begin() + index);
}

#pragma endregion
#pragma region imGui callbacks

// Custom callback function to call setGraph when input changes
int inputTextCallback(ImGuiInputTextCallbackData* data) {
	size_t index = reinterpret_cast<size_t>(data->UserData) - 1;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
		graphEquations[index].input = data->Buf;
		setGraph(graphEquations[index]); // Update the graph with both inputs
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
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));		   // Red button color
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0f)); // Darker red when hovered
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // Even darker red when pressed
		if (ImGui::Button(("X##" + std::to_string(i)).c_str())) {
			removeGraph(i);
		}
		ImGui::PopStyleColor(3);
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
		std::vector<glm::vec2, ArenaAllocator<glm::vec2>> vertexData;
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
	
	vboAllocator.reserve(VBOAllocator::DEFAULT_VBO_RESERVE_AMOUNT);
	gridVbo = vboAllocator.allocateVBO();

	for (auto& gridVao : gridVaos) {
		glGenVertexArrays(1, &gridVao.id);
	}

	graphEquations.resize(1);
	GraphEquation& firstGraph = graphEquations[0];
	firstGraph.input = "x*x";
	firstGraph.color = generateColor();
	firstGraph.func = [](double x) { return x * x; };
	firstGraph.vboObj.id = vboAllocator.allocateVBO();
	generateGraphData(firstGraph.func, firstGraph.vboObj);
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
	
	vboAllocator.cleanup();
	for (const auto& gridVao : gridVaos) {
		if (gridVao.id != 0) {
			glDeleteVertexArrays(1, &gridVao.id);
		}
	}

	glDeleteProgram(shaderProgram);
	*/
}

#pragma endregion