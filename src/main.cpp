#include <iostream>
#include "parser.hpp"
#include "eval.hpp"
#include <string>
#include <llvm/Support/TargetSelect.h>
#include "JITcompiler.hpp"
#include "mainGui.hpp"
#include <stb_image.h>
#include <fstream>

void pngToCArray(const char* filename, const char* outputFile) {
	int width, height, channels;
	unsigned char* imgData = stbi_load(filename, &width, &height, &channels, 0);

	if (!imgData) {
		std::cerr << "Failed to load image: " << filename << std::endl;
		return;
	}

	// Allocate a C array for the image data
	std::vector<unsigned char> cArray(width * height * channels);
	std::copy(imgData, imgData + (width * height * channels), cArray.data());

	// Open output file
	std::ofstream file(outputFile);
	if (!file) {
		std::cerr << "Failed to open output file: " << outputFile << std::endl;
		stbi_image_free(imgData);
		return;
	}

	// Write the C array to the file
	file << "#include <stdint.h>\n\n";
	file << "unsigned char imageArray[] = { ";
	for (size_t i = 0; i < cArray.size(); ++i) {
		file << (int)cArray[i];
		if (i < cArray.size() - 1) {
			file << ", ";
		}
	}
	file << " };\n";
	file << "unsigned int imageWidth = " << width << ";\n";
	file << "unsigned int imageHeight = " << height << ";\n";
	file.close();

	// Clean up
	stbi_image_free(imgData);
}



int main() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	
	pngToCArray(RESOURCES_PATH "icon.png", RESOURCES_PATH "icon.h");
	return 0;
	int returnCode = guiLoop();

	return returnCode;
}