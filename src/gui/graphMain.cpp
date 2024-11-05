#include "arenaAllocator.hpp"
#include "compilerPipeline.hpp"
#include "fontHandling.hpp"
#include "graphMain.hpp"
#include "mainGui.hpp"
#include "platformInput.h"
#include "tools.hpp"
#include "vboAllocator.hpp"
#include <array>
#include <cmath> // Include for std::log10 and std::floor
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include "tools.hpp"
#include "RobotoMono.h"
#include <vector>
#include <imgui_decomp.h>

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

std::string formatFloat(double num) {
	// Handle zero case
	if (num == 0.0)
		return "0";

	// Get the absolute value
	double absNum = std::abs(num);
	std::ostringstream oss;

	// Convert to string to manipulate
	std::string strNum = std::to_string(absNum);

	// Remove trailing zeros
	size_t end = strNum.find_last_not_of('0');
	if (end == std::string::npos) {
		strNum = "0";
	} else {
		size_t decimalPos = strNum.find('.');
		if (decimalPos != std::string::npos && end > decimalPos) {
			strNum = strNum.substr(0, end + 1);
		} else {
			strNum = strNum.substr(0, end + 1);
		}
	}

	// Determine the number of decimal places
	size_t decimalPos = strNum.find('.');
	size_t decimalPlaces = (decimalPos != std::string::npos) ? strNum.length() - decimalPos - 1 : 0;

	// Convert to scientific notation if necessary
	if (absNum >= 1e6 || absNum < 1e-6) { // Adjust the threshold as needed
		int power = static_cast<int>(std::floor(std::log10(absNum)));
		double mantissa = absNum / std::pow(10.0, power);
		oss << std::fixed << std::setprecision(1) << mantissa << "*10^" << power;
	} else {
		oss << std::fixed << std::setprecision(decimalPlaces);
		oss << absNum; // Output the number directly
	}

	// Return the formatted string, adding negative sign if necessary
	return (num < 0) ? "-" + oss.str() : oss.str();
}

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

static Renderer2D renderer;
static Font font;

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
			renderer.renderText({ndcX, screenMinY}, "Y", font, {0.0f, 0, 0, 1.0f}, 0.00075f, 0.1f, 2.0f,
								{0.25f, 0.75f}, {-5 / 1000.0f, 0.0f}, {}, {});
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
			renderer.renderText({ndcX, -origin.y * scale}, formatFloat(x).c_str(), font,
								{0.0f, 0, 0, 1.0f}, 0.00065f, 0.1f, 2.0f, {-1, 0}, {}, {}, {});
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
			renderer.renderText({screenMaxX, -ndcY}, "X", font, {0.0f, 0, 0, 1.0f}, 0.0007f, 0.1f, 2.0f,
								{ -1.125f, -0.25f } ,{0.0f, 2 * 5 / 1000.0f}, {}, {});
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
			renderer.renderText({-origin.x * scale, -ndcY}, formatFloat(-y).c_str(), font,
								{0.0f, 0, 0, 1.0f}, 0.00075f, 0.1f, 2.0f, {-1, 0}, {}, {}, {});
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
	glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);

	size_t offset = 0;

	const auto setupVertexData = [&offset](auto& gridVaos, const auto& vertices,
												size_t index) {
		if (vertices.empty()) {
			return;
		}
		glBufferSubData(GL_ARRAY_BUFFER, offset, vertices.size() * sizeof(float), vertices.data());
		gridVaos[index].amount = vertices.size() / 2;
		glBindVertexArray(gridVaos[index].id); // Bind the VAO
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(offset)); // Set pointer
		offset += vertices.size() * sizeof(float);											 // Update offset
	};

	setupVertexData(gridVaos, verticesThin, 0);
	setupVertexData(gridVaos, verticesMedium, 1);
	setupVertexData(gridVaos, verticesThick, 2);

	// Clean up state
	glDisableVertexAttribArray(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
					   std::vector<glm::vec2, ArenaAllocator<glm::vec2>>& vertexData,
					   size_t targetNumPoints) {
	if (func == nullptr) {
		return;
	}
	// the real amount is this + 2
	permaAssertComment(targetNumPoints != 0, "Tried to display zero points for a graph");

	vertexData.reserve(targetNumPoints);
	vertexData.clear();

	float step = 2.0f / (targetNumPoints - 1);
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
}


void generateAllGraphs() {
	std::vector<glm::vec2, ArenaAllocator<glm::vec2>> buffer;
	int targetNumPoints = static_cast<size_t>(initialNumPoints / std::sqrt(scale));

	for (GraphEquation& graph : graphEquations) {
		generateGraphData(graph.func, graph.vboObj, buffer, targetNumPoints);
		glBindBuffer(GL_ARRAY_BUFFER, graph.vboObj.id);
		graph.vboObj.amount = buffer.size();
		glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(glm::vec2), buffer.data(), GL_STATIC_DRAW);
	}
}

#pragma endregion
#pragma region color gen

// TODO: reconsider the entire method of this function
glm::vec3 generateColor() {
	static int index = 0;
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

	float hue = index * goldenAngle;
	hue = fmod(hue, 360.0f);

	index++;
	return hsvToRgb(hue, 0.5f, 0.95f);
}

#pragma endregion
#pragma region set function and color

bool setGraph(int index) {
	GraphEquation& graph = graphEquations[index];

	if (graph.vboObj.id == 0) {
		graph.vboObj.id = vboAllocator.allocateVBO();
	}

	if (graph.input.empty()) {
		graph.func = nullptr;
		clearGraphData(graph.vboObj);
		return true;
	}

	{
		auto funcOpt = compileFromSource(graph.input);
		if (!funcOpt.has_value()) {
			return false;
		}
		graph.func = std::move(funcOpt.value());
	}

	if (graph.color.x == 0.0f && graph.color.y == 0.0f && graph.color.z == 0.0f) {
		graph.color = generateColor();
	}

	std::vector<glm::vec2, ArenaAllocator<glm::vec2>> vertexData;
	size_t targetNumPoints = static_cast<size_t>(initialNumPoints / std::sqrt(scale));
	generateGraphData(graph.func, graph.vboObj, vertexData, targetNumPoints);

	glBindBuffer(GL_ARRAY_BUFFER, graph.vboObj.id);
	graph.vboObj.amount = vertexData.size();
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(glm::vec2), vertexData.data(), GL_STATIC_DRAW);

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

#pragma endregion
#pragma region mainSuff

bool gameLogic(float deltaTime, int w, int h) {
	glClear(GL_COLOR_BUFFER_BIT); // Clear screen
	bool shouldRecalculateEverything = false;

#pragma region draw grid using shader
	glUseProgram(shaderProgram);
	glUniform4f(lineColorUniform, 0.1f, 0.1f, 0.1f, 1.0f);
	for (size_t i = 0; i < gridVaos.size(); ++i) {
		glUniform1f(lineThicknessUniform, (2 * i + 1) / 1000.0f);
		glBindVertexArray(gridVaos[i].id);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_LINES, 0, gridVaos[i].amount);
		glDisableVertexAttribArray(0);
	}
	glBindVertexArray(0); // Clean up
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
	glUseProgram(0);
#pragma endregion
#pragma region display equations widget
	ImGui::Begin("Equations", nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);
	for (size_t i = 0; i < graphEquations.size(); i++) {
		if(ImGui::InputText(("##" + std::to_string(i)).c_str(), &graphEquations[i].input)) {
			setGraph(i);
		}
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
	shouldRecalculateEverything |= ImGui::SliderFloat("Scale", &scale, 0.00001f, 100000.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginX", &origin.x, -50.0f, 50.0f);
	shouldRecalculateEverything |= ImGui::SliderFloat("OriginY", &origin.y, -50.0f, 50.0f);
	ImGui::End();
#pragma endregion

#pragma region move with cursor
	// Move with cursor
	static glm::ivec2 originMousePos = {0, 0};
	static glm::vec2 originOrigin = {0, 0};

	if (platform::isMMousePressed()) {
		originOrigin = origin;
		// originMousePos = static_cast<glm::vec2>(platform::getRelMousePosition()) / glm::vec2({w, h});
		originMousePos = platform::getRelMousePosition();
	}
	if (platform::isMMouseHeld()) {
		glm::ivec2 currentMousePos = platform::getRelMousePosition();
		glm::vec2 delta = 2.0f * static_cast<glm::vec2>(originMousePos - currentMousePos); // Delta in pixels
		origin = originOrigin + (delta / scale) / glm::vec2({w, h}); // Scale and update the origin

		shouldRecalculateEverything = true;
	}
	double scrollSize = platform::getScrollSize();
	if (scrollSize != 0) {
		scale *= exp(scrollSize / scrollSensitivity);
		// scale = std::clamp(scale, 0.001f, 1000.0f);
		shouldRecalculateEverything = true;
	}

#pragma endregion
	// reset early
	if (shouldRecalculateEverything) {
		renderer.flush();
		generateAxisData();
		arena_reset(&global_arena); // early reset cause this requires alot of memory
		generateAllGraphs();
	} else {
		renderer.flush(false);
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

	gldInit();
	renderer.create();
	{
		int length = stb_decompress_length((const unsigned char*)RobotoMono_compressed_data);
		unsigned char* data = (unsigned char*)arena_alloc(&global_arena, length);
		stb_decompress(data, (const unsigned char*)RobotoMono_compressed_data, RobotoMono_compressed_size);
		font.createFromTTF(data, length);


		ImGuiIO& io = ImGui::GetIO();
		ImFont imguiFont{};
		ImFontConfig font_cfg = ImFontConfig();
		font_cfg.FontDataOwnedByAtlas = false;
		font_cfg.FontData = data;
		font_cfg.SizePixels = 15.0f;
		font_cfg.FontDataSize = length;
		io.Fonts->AddFont(&font_cfg);
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
	generateAllGraphs();

	generateAxisData();

	arena_reset(&global_arena);
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