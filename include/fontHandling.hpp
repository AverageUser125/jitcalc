#pragma once
#include <imstb_truetype.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string.h>
#include <fstream>
#include <cstring>
#include <vector>

using Color4f = glm::vec4;
using Rect = glm::vec4;
typedef glm::vec2 Position2D;
typedef glm::vec4 Texture_Coords;

//this is how the library should load textures by default.
#define GL2D_DEFAULT_TEXTURE_LOAD_MODE_PIXELATED false
#define GL2D_DEFAULT_TEXTURE_LOAD_MODE_USE_MIPMAPS true
#define GL2D_DefaultTextureCoords (glm::vec4{0, 1, 1, 0})
#define GL2D_OPNEGL_SHADER_VERSION "#version 330"
#define GL2D_OPNEGL_SHADER_PRECISION "precision highp float;"

void gldInit();

#pragma region shader program
struct ShaderProgram {
	GLuint id = 0;
	int u_sampler = 0;

	void bind() {
		glUseProgram(id);
	};

	void clear() {
		glDeleteProgram(id);
		*this = {};
	}
};

void validateProgram(GLuint id);
GLuint loadShader(const char* source, GLenum shaderType);
ShaderProgram createShaderProgram(const char* vertex, const char* fragment);
#pragma endregion

#pragma region texture
struct Texture {
	GLuint id = 0;

	Texture(){};

	explicit Texture(const char* file, bool pixelated = GL2D_DEFAULT_TEXTURE_LOAD_MODE_PIXELATED,
					 bool useMipMaps = GL2D_DEFAULT_TEXTURE_LOAD_MODE_USE_MIPMAPS) {
	}

	//returns the texture dimensions
	glm::ivec2 GetSize();

	//Note: This function expects a buffer of bytes in GL_RGBA format
	void createFromBuffer(const char* image_data, const int width, const int height,
						  bool pixelated = GL2D_DEFAULT_TEXTURE_LOAD_MODE_PIXELATED,
						  bool useMipMaps = GL2D_DEFAULT_TEXTURE_LOAD_MODE_USE_MIPMAPS);

	//used internally. It creates a 1by1 white texture
	void create1PxSquare(const char* b = 0);

	//returns how much memory does the texture take (bytes),
	//used for allocating your buffer when using readTextureData
	//you can also optionally get the width and the height of the texture using outSize
	size_t getMemorySize(int mipLevel = 0, glm::ivec2* outSize = 0);

	//reads the texture data back into RAM, you need to specify
	//the buffer to read into yourself, allocate it using
	//getMemorySize to know the size in bytes.
	//The data will be in RGBA format, one byte each component
	void readTextureData(void* buffer, int mipLevel = 0);

	//reads the texture data back into RAM
	//The data will be in RGBA format, one byte each component
	//You can also optionally get the width and the height of the texture using outSize
	std::vector<unsigned char> readTextureData(int mipLevel = 0, glm::ivec2* outSize = 0);

	void bind(const unsigned int sample = 0);
	void unbind();

	void cleanup();
};
#pragma endregion

#pragma region font
struct Font {
	Texture texture = {};
	glm::ivec2 size = {};
	std::vector<stbtt_packedchar> packedCharsBuffer{};
	float max_height = 0.f;

	Font() {
	}

	explicit Font(const char* file) {
		createFromFile(file);
	}

	void createFromTTF(const unsigned char* ttf_data, const size_t ttf_data_size);
	void createFromFile(const char* file);

	void cleanup();
};

stbtt_aligned_quad fontGetGlyphQuad(const Font& font, const char c);

#pragma endregion

#pragma region framebuffer

struct FrameBuffer {
	FrameBuffer(){};

	explicit FrameBuffer(unsigned int w, unsigned int h) {
		create(w, h);
	};

	GLuint fbo = 0;
	Texture texture = {};

	void create(unsigned int w, unsigned int h);
	void resize(unsigned int w, unsigned int h);

	//clears resources
	void cleanup();

	//clears colors
	void clear();
};

#pragma endregion

#pragma region renderer

void enableNecessaryGLFeatures();

enum Renderer2DBufferType{
	quadPositions,
	quadColors,
	texturePositions,

	bufferSize
};


struct Renderer2D {
	Renderer2D(){};

	//feel free to delete this lines but you probably don't want to copy the renderer from a place to another
	Renderer2D(Renderer2D& other) = delete;
	Renderer2D(Renderer2D&& other) = delete;
	Renderer2D operator=(Renderer2D other) = delete;
	Renderer2D operator=(Renderer2D& other) = delete;
	Renderer2D operator=(Renderer2D&& other) = delete;

	//creates the renderer
	//fbo is the default frame buffer, 0 means drawing to the screen.
	//Quad count is the reserved quad capacity for drawing.
	//If the capacity is exceded it will be extended but this will cost performance.
	void create(GLuint fbo = 0, size_t quadCount = 1'000);

	//Clears the object alocated resources but
	//does not clear resources allocated by user like textures, fonts and fbos!
	void cleanup();

	ShaderProgram currentShader = {};

	GLuint defaultFBO = 0;

	GLuint vao = {};

	//4 elements each component
	std::vector<glm::vec2> spritePositions;
	std::vector<glm::vec4> spriteColors;
	std::vector<glm::vec2> texturePositions;
	std::vector<Texture> spriteTextures;

	GLuint buffers[Renderer2DBufferType::bufferSize] = {};

	//window metrics, should be up to date at all times
	int windowW = 1;
	int windowH = 1;

	void updateWindowMetrics(int w, int h) {
		//windowW = w;
		//windowH = h;
	}

	//clears the things that are to be drawn when calling flush
	inline void clearDrawData() {
		spritePositions.clear();
		spriteColors.clear();
		texturePositions.clear();
		spriteTextures.clear();
	}

	void flush(bool clearDrawData = true);

	glm::vec2 getTextSize(const char* text, const Font font, const float size = 1.5f, const float spacing = 4,
						  const float line_space = 3);

	// The origin will be the bottom left corner since it represents the line for the text to be drawn
	//Pacing and lineSpace are influenced by size
	//todo the function should returns the size of the text drawn also refactor
	void renderText(glm::vec2 position, const char* text, const Font font, const Color4f color, const float size = 1.5f,
					const float spacing = 4, const float line_space = 3, const glm::vec2 relativeCenter = {-0.5, -0.5},
					const glm::vec2 absoluteCenter = {0.0f, 0.0f},
					const Color4f ShadowColor = {0.1, 0.1, 0.1, 1}, const Color4f LightColor = {});

	void renderRectangle(const Rect transforms, const Texture texture, const Color4f colors[4],
									 const glm::vec4 textureCoords);

	inline void renderRectangle(const Rect transforms, const Texture texture, const Color4f colors = {1, 1, 1, 1},
								const glm::vec4 textureCoords = GL2D_DefaultTextureCoords) {
		Color4f c[4] = {colors, colors, colors, colors};
		renderRectangle(transforms, texture, c, textureCoords);
	}

	inline void renderRectangle(const Rect transforms, const Texture texture, const Color4f colors = {1, 1, 1, 1}) {
		const Color4f c[4] = {colors, colors, colors, colors};
		renderRectangle(transforms, texture, c, GL2D_DefaultTextureCoords);
	}
};

void internalFlush(Renderer2D& renderer, bool clearDrawData);

#pragma endregion