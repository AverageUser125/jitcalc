#include "fontHandling.hpp"
#include "tools.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>
#undef STB_TRUETYPE_IMPLEMENTATION

#pragma region shader code
static ShaderProgram defaultShader = {};
static const char* defaultVertexShader =
	GL2D_OPNEGL_SHADER_VERSION "\n"
	GL2D_OPNEGL_SHADER_PRECISION "\n"
	"in vec2 quad_positions;\n"
	"in vec4 quad_colors;\n"
	"in vec2 texturePositions;\n"
	"out vec4 v_color;\n"
	"out vec2 v_texture;\n"
	"out vec2 v_positions;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vec4(quad_positions, 0, 1);\n"
	"	v_color = quad_colors;\n"
	"	v_texture = texturePositions;\n"
	"	v_positions = gl_Position.xy;\n"
	"}\n";

static const char* defaultFragmentShader =
	GL2D_OPNEGL_SHADER_VERSION "\n"
	GL2D_OPNEGL_SHADER_PRECISION "\n"
	"out vec4 color;\n"
	"in vec4 v_color;\n"
	"in vec2 v_texture;\n"
	"uniform sampler2D u_sampler;\n"
	"void main()\n"
	"{\n"
	"    color = v_color * texture2D(u_sampler, v_texture);\n"
	"}\n";
#pragma endregion
#pragma region utils

void gldInit() {
	enableNecessaryGLFeatures();
	defaultShader = createShaderProgram(defaultVertexShader, defaultFragmentShader);
}

stbtt_aligned_quad fontGetGlyphQuad(const Font& font, const char c) {
	stbtt_aligned_quad quad = {0};

	float x = 0;
	float y = 0;

	stbtt_GetPackedQuad(font.packedCharsBuffer, font.size.x, font.size.y, c - ' ', &x, &y, &quad, 1);

	return quad;
}

float positionToScreenCoordsX(const float position, float w) {
	return (position / w) * 2 - 1;
}

float positionToScreenCoordsY(const float position, float h) {
	return -((-position / h) * 2 - 1);
}
#pragma endregion
#pragma region shader program

void validateProgram(GLuint id) {
	int info = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &info);

	if (info != GL_TRUE) {
		int l = 0;

		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &l);

		std::string message;
		message.resize(l);

		glGetProgramInfoLog(id, l, &l, message.data());

		elog(message);

	}

	glValidateProgram(id);
}

GLuint loadShader(const char* source, GLenum shaderType) {
	GLuint id = glCreateShader(shaderType);

	glShaderSource(id, 1, &source, 0);
	glCompileShader(id);

	int result = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (!result) {
		int l = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &l);

		std::string message;
		message.resize(l + 1);

		glGetShaderInfoLog(id, l, &l, message.data());

		elog(message);
	}

	return id;
}

ShaderProgram createShaderProgram(const char* vertex, const char* fragment) {
	ShaderProgram shader = {0};

	const GLuint vertexId = loadShader(vertex, GL_VERTEX_SHADER);
	const GLuint fragmentId = loadShader(fragment, GL_FRAGMENT_SHADER);

	shader.id = glCreateProgram();
	glAttachShader(shader.id, vertexId);
	glAttachShader(shader.id, fragmentId);

	glBindAttribLocation(shader.id, 0, "quad_positions");
	glBindAttribLocation(shader.id, 1, "quad_colors");
	glBindAttribLocation(shader.id, 2, "texturePositions");

	glLinkProgram(shader.id);

	glDeleteShader(vertexId);
	glDeleteShader(fragmentId);

	validateProgram(shader.id);

	shader.u_sampler = glGetUniformLocation(shader.id, "u_sampler");

	return shader;
}

#pragma endregion
#pragma region texture

glm::ivec2 Texture::GetSize() {
	glm::ivec2 s;
	glBindTexture(GL_TEXTURE_2D, id);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &s.x);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &s.y);
	return s;
}

void Texture::createFromBuffer(const char* image_data, const int width, const int height, bool pixelated,
							   bool useMipMaps) {
	GLuint id = 0;

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	if (pixelated) {
		if (useMipMaps) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	} else {
		if (useMipMaps) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	glGenerateMipmap(GL_TEXTURE_2D);


	this->id = id;
}

void Texture::create1PxSquare(const char* b) {
	if (b == nullptr) {
		const unsigned char buff[] = {0xff, 0xff, 0xff, 0xff};

		createFromBuffer((char*)buff, 1, 1);
	} else {
		createFromBuffer(b, 1, 1);
	}
}

void Texture::createFromFileData(const unsigned char* image_file_data, const size_t image_file_size, bool pixelated,
								 bool useMipMaps) {
	/*
	stbi_set_flip_vertically_on_load(true);

	int width = 0;
	int height = 0;
	int channels = 0;

	const unsigned char* decodedImage =
		stbi_load_from_memory(image_file_data, (int)image_file_size, &width, &height, &channels, 4);

	createFromBuffer((const char*)decodedImage, width, height, pixelated, useMipMaps);

	STBI_FREE(decodedImage);
	*/
}

void Texture::createFromFileDataWithPixelPadding(const unsigned char* image_file_data, const size_t image_file_size,
												 int blockSize, bool pixelated, bool useMipMaps) {
	/*
	stbi_set_flip_vertically_on_load(true);

	int width = 0;
	int height = 0;
	int channels = 0;

	const unsigned char* decodedImage =
		stbi_load_from_memory(image_file_data, (int)image_file_size, &width, &height, &channels, 4);

	int newW = width + ((width * 2) / blockSize);
	int newH = height + ((height * 2) / blockSize);

	auto getOld = [decodedImage, width](int x, int y, int c) -> const unsigned char {
		return decodedImage[4 * (x + (y * width)) + c];
	};


	unsigned char* newData = new unsigned char[newW * newH * 4]{};

	auto getNew = [newData, newW](int x, int y, int c) { return &newData[4 * (x + (y * newW)) + c]; };

	int newDataCursor = 0;
	int dataCursor = 0;

	//first copy data
	for (int y = 0; y < newH; y++) {
		int yNo = 0;
		if ((y == 0 || y == newH - 1 || ((y) % (blockSize + 2)) == 0 || ((y + 1) % (blockSize + 2)) == 0)) {
			yNo = 1;
		}

		for (int x = 0; x < newW; x++) {
			if (yNo ||

				((x == 0 || x == newW - 1 || (x % (blockSize + 2)) == 0 || ((x + 1) % (blockSize + 2)) == 0))

			) {
				newData[newDataCursor++] = 0;
				newData[newDataCursor++] = 0;
				newData[newDataCursor++] = 0;
				newData[newDataCursor++] = 0;
			} else {
				newData[newDataCursor++] = decodedImage[dataCursor++];
				newData[newDataCursor++] = decodedImage[dataCursor++];
				newData[newDataCursor++] = decodedImage[dataCursor++];
				newData[newDataCursor++] = decodedImage[dataCursor++];
			}
		}
	}

	//then add margins


	for (int x = 1; x < newW - 1; x++) {
		//copy on left
		if (x == 1 || (x % (blockSize + 2)) == 1) {
			for (int y = 0; y < newH; y++) {
				*getNew(x - 1, y, 0) = *getNew(x, y, 0);
				*getNew(x - 1, y, 1) = *getNew(x, y, 1);
				*getNew(x - 1, y, 2) = *getNew(x, y, 2);
				*getNew(x - 1, y, 3) = *getNew(x, y, 3);
			}

		} else //copy on rigght
			if (x == newW - 2 || (x % (blockSize + 2)) == blockSize) {
				for (int y = 0; y < newH; y++) {
					*getNew(x + 1, y, 0) = *getNew(x, y, 0);
					*getNew(x + 1, y, 1) = *getNew(x, y, 1);
					*getNew(x + 1, y, 2) = *getNew(x, y, 2);
					*getNew(x + 1, y, 3) = *getNew(x, y, 3);
				}
			}
	}

	for (int y = 1; y < newH - 1; y++) {
		if (y == 1 || (y % (blockSize + 2)) == 1) {
			for (int x = 0; x < newW; x++) {
				*getNew(x, y - 1, 0) = *getNew(x, y, 0);
				*getNew(x, y - 1, 1) = *getNew(x, y, 1);
				*getNew(x, y - 1, 2) = *getNew(x, y, 2);
				*getNew(x, y - 1, 3) = *getNew(x, y, 3);
			}
		} else if (y == newH - 2 || (y % (blockSize + 2)) == blockSize) {
			for (int x = 0; x < newW; x++) {
				*getNew(x, y + 1, 0) = *getNew(x, y, 0);
				*getNew(x, y + 1, 1) = *getNew(x, y, 1);
				*getNew(x, y + 1, 2) = *getNew(x, y, 2);
				*getNew(x, y + 1, 3) = *getNew(x, y, 3);
			}
		}
	}

	createFromBuffer((const char*)newData, newW, newH, pixelated, useMipMaps);

	STBI_FREE(decodedImage);
	delete[] newData;
	*/
}

void Texture::loadFromFile(const char* fileName, bool pixelated, bool useMipMaps) {
	std::ifstream file(fileName, std::ios::binary);

	if (!file.is_open()) {
		elog("error opening: ", fileName, " ,for creating font");
		return;
	}

	int fileSize = 0;
	file.seekg(0, std::ios::end);
	fileSize = (int)file.tellg();
	file.seekg(0, std::ios::beg);
	unsigned char* fileData = new unsigned char[fileSize];
	file.read((char*)fileData, fileSize);
	file.close();

	createFromFileData(fileData, fileSize, pixelated, useMipMaps);

	delete[] fileData;
}

void Texture::loadFromFileWithPixelPadding(const char* fileName, int blockSize, bool pixelated, bool useMipMaps) {
	std::ifstream file(fileName, std::ios::binary);

	if (!file.is_open()) {
		elog("error opening: ", fileName, " ,for creating font");
		return;
	}

	int fileSize = 0;
	file.seekg(0, std::ios::end);
	fileSize = (int)file.tellg();
	file.seekg(0, std::ios::beg);
	unsigned char* fileData = new unsigned char[fileSize];
	file.read((char*)fileData, fileSize);
	file.close();

	createFromFileDataWithPixelPadding(fileData, fileSize, blockSize, pixelated, useMipMaps);

	delete[] fileData;
}

size_t Texture::getMemorySize(int mipLevel, glm::ivec2* outSize) {
	glBindTexture(GL_TEXTURE_2D, id);

	glm::ivec2 stub = {};

	if (!outSize) {
		outSize = &stub;
	}

	glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_WIDTH, &outSize->x);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_HEIGHT, &outSize->y);

	glBindTexture(GL_TEXTURE_2D, 0);

	return outSize->x * outSize->y * 4;
}

void Texture::readTextureData(void* buffer, int mipLevel) {
	glBindTexture(GL_TEXTURE_2D, id);
	glGetTexImage(GL_TEXTURE_2D, mipLevel, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
}

std::vector<unsigned char> Texture::readTextureData(int mipLevel, glm::ivec2* outSize) {
	glBindTexture(GL_TEXTURE_2D, id);

	glm::ivec2 stub = {};

	if (!outSize) {
		outSize = &stub;
	}

	glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_WIDTH, &outSize->x);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_HEIGHT, &outSize->y);

	std::vector<unsigned char> data;
	data.resize(outSize->x * outSize->y * 4);
	glGetTexImage(GL_TEXTURE_2D, mipLevel, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

	glBindTexture(GL_TEXTURE_2D, 0);

	return data;
}

void Texture::bind(const unsigned int sample) {
	glActiveTexture(GL_TEXTURE0 + sample);
	glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::cleanup() {
	glDeleteTextures(1, &id);
	*this = {};
}

#pragma endregion
#pragma region font
void Font::createFromTTF(const unsigned char* ttf_data, const size_t ttf_data_size) {
	size.x = 2000, size.y = 2000, max_height = 0, packedCharsBufferSize = ('~' - ' ');

	//STB TrueType will give us a one channel buffer of the font that we then convert to RGBA for OpenGL
	const size_t fontMonochromeBufferSize = size.x * size.y;
	const size_t fontRgbaBufferSize = size.x * size.y * 4;

	unsigned char* fontMonochromeBuffer = new unsigned char[fontMonochromeBufferSize];
	unsigned char* fontRgbaBuffer = new unsigned char[fontRgbaBufferSize];

	packedCharsBuffer = new stbtt_packedchar[packedCharsBufferSize]{};

	stbtt_pack_context stbtt_context;
	stbtt_PackBegin(&stbtt_context, fontMonochromeBuffer, size.x, size.y, 0, 2, NULL);
	stbtt_PackSetOversampling(&stbtt_context, 2, 2);
	stbtt_PackFontRange(&stbtt_context, ttf_data, 0, 65, ' ', '~' - ' ', packedCharsBuffer);
	stbtt_PackEnd(&stbtt_context);

	for (int i = 0; i < fontMonochromeBufferSize; i++) {

		fontRgbaBuffer[(i * 4)] = fontMonochromeBuffer[i];
		fontRgbaBuffer[(i * 4) + 1] = fontMonochromeBuffer[i];
		fontRgbaBuffer[(i * 4) + 2] = fontMonochromeBuffer[i];

		if (fontMonochromeBuffer[i] > 1) {
			fontRgbaBuffer[(i * 4) + 3] = 255;
		} else {
			fontRgbaBuffer[(i * 4) + 3] = 0;
		}
	}

	//Init texture
	{
		glGenTextures(1, &texture.id);
		glBindTexture(GL_TEXTURE_2D, texture.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, fontRgbaBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	delete[] fontMonochromeBuffer;
	delete[] fontRgbaBuffer;

	for (char c = ' '; c <= '~'; c++) {
		Font* fontptr = this;
		const stbtt_aligned_quad q = fontGetGlyphQuad(*fontptr, c);
		const float m = q.y1 - q.y0;

		if (m > max_height && m < 1.e+8f) {
			max_height = m;
		}
	}
}

void Font::createFromFile(const char* file) {
	std::ifstream fileFont(file, std::ios::binary);

	if (!fileFont.is_open()) {
		elog("error opening: ", file, " ,for creating font");
		return;
	}

	int fileSize = 0;
	fileFont.seekg(0, std::ios::end);
	fileSize = (int)fileFont.tellg();
	fileFont.seekg(0, std::ios::beg);
	unsigned char* fileData = new unsigned char[fileSize];
	fileFont.read((char*)fileData, fileSize);
	fileFont.close();

	createFromTTF(fileData, fileSize);

	delete[] fileData;
}

void Font::cleanup() {
	texture.cleanup();
	*this = {};
}

#pragma endregion
#pragma region framebuffer

void FrameBuffer::create(unsigned int w, unsigned int h) {
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.id, 0);

	//glDrawBuffer(GL_COLOR_ATTACHMENT0); //todo why is this commented out ?

	//glGenTextures(1, &depthtTexture);
	//glBindTexture(GL_TEXTURE_2D, depthtTexture);

	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtTexture, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::resize(unsigned int w, unsigned int h) {
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//glBindTexture(GL_TEXTURE_2D, depthtTexture);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void FrameBuffer::cleanup() {
	if (fbo) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}

	if (texture.id) {
		glDeleteTextures(1, &texture.id);
		texture = {};
	}

	//glDeleteTextures(1, &depthtTexture);
	//depthtTexture = 0;
}

void FrameBuffer::clear() {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	//glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClearColor(0, 0, 0, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#pragma endregion
#pragma region renderer

void enableNecessaryGLFeatures() {
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2D::create(GLuint fbo, size_t quadCount) {
	
	currentShader = defaultShader;

	defaultFBO = fbo;

	clearDrawData();
	spritePositions.reserve(quadCount * 6);
	spriteColors.reserve(quadCount * 6);
	texturePositions.reserve(quadCount * 6);
	spriteTextures.reserve(quadCount);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(Renderer2DBufferType::bufferSize, buffers);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Renderer2DBufferType::quadPositions]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Renderer2DBufferType::quadColors]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Renderer2DBufferType::texturePositions]);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindVertexArray(0);
}

void Renderer2D::cleanup() {
	glDeleteVertexArrays(1, &vao);

	internalPostProcessFlip = 0;
}

void Renderer2D::renderRectangle(const Rect transforms, const Texture texture, const Color4f colors[4],
								 const glm::vec2 origin, const float rotation, const glm::vec4 textureCoords) {
	glm::vec2 newOrigin{};
	newOrigin.x = origin.x + transforms.x + (transforms.z / 2);
	newOrigin.y = origin.y + transforms.y + (transforms.w / 2);
	renderRectangleAbsRotation(transforms, texture, colors, newOrigin, rotation, textureCoords);
}

void Renderer2D::renderRectangleAbsRotation(const Rect transforms, const Texture texture, const Color4f colors[4],
												  const glm::vec2 origin, const float rotation,
												  const glm::vec4 textureCoords) {
	debugAssertComment(texture.id != 0, "Invalid texture");
	Texture textureCopy = texture;

	//We need to flip texture_transforms.y
	const float transformsY = transforms.y * -1;

	glm::vec2 v1 = {transforms.x, transformsY};
	glm::vec2 v2 = {transforms.x, transformsY - transforms.w};
	glm::vec2 v3 = {transforms.x + transforms.z, transformsY - transforms.w};
	glm::vec2 v4 = {transforms.x + transforms.z, transformsY};

	v1.x = positionToScreenCoordsX(v1.x, (float)windowW);
	v2.x = positionToScreenCoordsX(v2.x, (float)windowW);
	v3.x = positionToScreenCoordsX(v3.x, (float)windowW);
	v4.x = positionToScreenCoordsX(v4.x, (float)windowW);
	v1.y = positionToScreenCoordsY(v1.y, (float)windowH);
	v2.y = positionToScreenCoordsY(v2.y, (float)windowH);
	v3.y = positionToScreenCoordsY(v3.y, (float)windowH);
	v4.y = positionToScreenCoordsY(v4.y, (float)windowH);

	spritePositions.push_back(glm::vec2{v1.x, v1.y});
	spritePositions.push_back(glm::vec2{v2.x, v2.y});
	spritePositions.push_back(glm::vec2{v4.x, v4.y});

	spritePositions.push_back(glm::vec2{v2.x, v2.y});
	spritePositions.push_back(glm::vec2{v3.x, v3.y});
	spritePositions.push_back(glm::vec2{v4.x, v4.y});

	spriteColors.push_back(colors[0]);
	spriteColors.push_back(colors[1]);
	spriteColors.push_back(colors[3]);
	spriteColors.push_back(colors[1]);
	spriteColors.push_back(colors[2]);
	spriteColors.push_back(colors[3]);

	texturePositions.push_back(glm::vec2{textureCoords.x, textureCoords.y}); //1
	texturePositions.push_back(glm::vec2{textureCoords.x, textureCoords.w}); //2
	texturePositions.push_back(glm::vec2{textureCoords.z, textureCoords.y}); //4
	texturePositions.push_back(glm::vec2{textureCoords.x, textureCoords.w}); //2
	texturePositions.push_back(glm::vec2{textureCoords.z, textureCoords.w}); //3
	texturePositions.push_back(glm::vec2{textureCoords.z, textureCoords.y}); //4

	spriteTextures.push_back(textureCopy);
}

glm::vec2 Renderer2D::getTextSize(const char* text, const Font font, const float size, const float spacing,
								  const float line_space) {
	debugAssertComment(font.texture.id != 0, "Missing font");

	glm::vec2 position = {};

	const int text_length = (int)strlen(text);
	Rect rectangle = {};
	rectangle.x = position.x;
	float linePositionY = position.y;

	//This is the y position we render at because it advances when we encounter newlines
	float maxPos = 0;
	float maxPosY = 0;
	float bonusY = 0;

	for (int i = 0; i < text_length; i++) {
		if (text[i] == '\n') {
			rectangle.x = position.x;
			linePositionY += (font.max_height + line_space) * size;
			bonusY += (font.max_height + line_space) * size;
			maxPosY = 0;
		} else if (text[i] == '\t') {
			const stbtt_aligned_quad quad = fontGetGlyphQuad(font, '_');
			auto x = quad.x1 - quad.x0;

			rectangle.x += x * size * 3 + spacing * size;
		} else if (text[i] == ' ') {
			const stbtt_aligned_quad quad = fontGetGlyphQuad(font, '_');
			auto x = quad.x1 - quad.x0;

			rectangle.x += x * size + spacing * size;
		} else if (text[i] >= ' ' && text[i] <= '~') {
			const stbtt_aligned_quad quad = fontGetGlyphQuad(font, text[i]);

			rectangle.z = quad.x1 - quad.x0;
			rectangle.w = quad.y1 - quad.y0;

			rectangle.z *= size;
			rectangle.w *= size;

			rectangle.y = linePositionY + quad.y0 * size;

			rectangle.x += rectangle.z + spacing * size;

			maxPosY = std::max(maxPosY, rectangle.y);
			maxPos = std::max(maxPos, rectangle.x);
		}
	}

	maxPos = std::max(maxPos, rectangle.x);
	maxPosY = std::max(maxPosY, rectangle.y);

	float paddX = maxPos;

	float paddY = maxPosY;

	paddY += font.max_height * size + bonusY;

	return glm::vec2{paddX, paddY};
}

void Renderer2D::renderText(glm::vec2 position, const char* text, const Font font, const Color4f color, const float size,
					const float spacing, const float line_space, bool showInCenter,
					const Color4f ShadowColor, const Color4f LightColor) {
	debugAssertComment(font.texture.id != 0, "Missing font");

	const int text_length = (int)strlen(text);
	glm::vec4 rectangle{};
	rectangle.x = position.x;
	float linePositionY = position.y;

	if (showInCenter) {
		auto textSize = getTextSize(text, font, size, spacing, line_space);

		position.x -= textSize.x / 2.f;
		position.y += textSize.y / 2.f;
	}

	rectangle = {};
	rectangle.x = position.x;

	//This is the y position we render at because it advances when we encounter newlines
	linePositionY = position.y;

	for (int i = 0; i < text_length; i++) {
		if (text[i] == '\n') {
			rectangle.x = position.x;
			linePositionY += (font.max_height + line_space) * size;
		} else if (text[i] == '\t') {
			const stbtt_aligned_quad quad = fontGetGlyphQuad(font, '_');
			auto x = quad.x1 - quad.x0;

			rectangle.x += x * size * 3 + spacing * size;
		} else if (text[i] == ' ') {
			const stbtt_aligned_quad quad = fontGetGlyphQuad(font, '_');
			auto x = quad.x1 - quad.x0;
			rectangle.x += x * size + spacing * size;
		} else if (text[i] >= ' ' && text[i] <= '~') {

			const stbtt_aligned_quad quad = fontGetGlyphQuad(font, text[i]);

			rectangle.z = quad.x1 - quad.x0;
			rectangle.w = quad.y1 - quad.y0;

			rectangle.z *= size;
			rectangle.w *= size;

			//rectangle.y = linePositionY - rectangle.w;
			rectangle.y = linePositionY + quad.y0 * size;

			glm::vec4 colorData[4] = {color, color, color, color};

			if (ShadowColor.w) {
				glm::vec2 pos = {-5, 3};
				pos *= size;
				renderRectangle({rectangle.x + pos.x, rectangle.y + pos.y, rectangle.z, rectangle.w}, font.texture,
								ShadowColor, glm::vec2{0, 0}, 0, glm::vec4{quad.s0, quad.t0, quad.s1, quad.t1});
			}

			renderRectangle(rectangle, font.texture, colorData, glm::vec2{0, 0}, 0,
							glm::vec4{quad.s0, quad.t0, quad.s1, quad.t1});

			if (LightColor.w) {
				glm::vec2 pos = {-2, 1}; 
				pos *= size;
				renderRectangle({rectangle.x + pos.x, rectangle.y + pos.y, rectangle.z, rectangle.w}, font.texture,
								LightColor, glm::vec2{0, 0}, 0, glm::vec4{quad.s0, quad.t0, quad.s1, quad.t1});
			}


			rectangle.x += rectangle.z + spacing * size;
		}
	}
}

void Renderer2D::flush(bool clearDrawData) {
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
	internalFlush(*this, clearDrawData);
}

void internalFlush(Renderer2D& renderer, bool clearDrawData) {
	debugAssertComment(renderer.vao, "Renderer not initialized. Have you forgotten to call gl2d::Renderer2D::create() ?");

	if (renderer.windowH == 0 || renderer.windowW == 0) {
		if (clearDrawData) {
			renderer.clearDrawData();
		}

		return;
	}

	if (renderer.spriteTextures.empty()) {
		return;
	}

	permaAssertComment(renderer.windowH > 0 && renderer.windowW > 0,
					   "Negative Window sized ave you forgotten to call updateWindowMetrics(w, h)");


	glUseProgram(renderer.currentShader.id);

	glUniform1i(renderer.currentShader.u_sampler, 0);
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer.buffers[Renderer2DBufferType::quadPositions]);
	glBufferData(GL_ARRAY_BUFFER, renderer.spritePositions.size() * sizeof(glm::vec2), renderer.spritePositions.data(),
				 GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, renderer.buffers[Renderer2DBufferType::quadColors]);
	glBufferData(GL_ARRAY_BUFFER, renderer.spriteColors.size() * sizeof(glm::vec4), renderer.spriteColors.data(),
				 GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, renderer.buffers[Renderer2DBufferType::texturePositions]);
	glBufferData(GL_ARRAY_BUFFER, renderer.texturePositions.size() * sizeof(glm::vec2),
				 renderer.texturePositions.data(), GL_STREAM_DRAW);


	glBindVertexArray(renderer.vao);

	//Instance render the textures
	{
		const int size = renderer.spriteTextures.size();
		int pos = 0;
		unsigned int id = renderer.spriteTextures[0].id;

		renderer.spriteTextures[0].bind();

		for (int i = 1; i < size; i++) {
			if (renderer.spriteTextures[i].id != id) {
				glDrawArrays(GL_TRIANGLES, pos * 6, 6 * (i - pos));

				pos = i;
				id = renderer.spriteTextures[i].id;

				renderer.spriteTextures[i].bind();
			}
		}

		glDrawArrays(GL_TRIANGLES, pos * 6, 6 * (size - pos));

		glBindVertexArray(0);
	}

	if (clearDrawData) {
		renderer.clearDrawData();
	}
	glUseProgram(0);
}

#pragma endregion