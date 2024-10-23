#include "vboAllocator.hpp"
#include "arenaAllocator.hpp"

VBOAllocator::VBOAllocator() {
}

VBOAllocator::~VBOAllocator() {
	cleanup();
}

void VBOAllocator::reserve(size_t amount) {
	size_t initialSize = freeList.size();
	freeList.resize(freeList.size() + amount);
	glGenBuffers(amount, freeList.data() + initialSize);
}

GLuint VBOAllocator::allocateVBO() {
	GLuint vbo;

	if (!freeList.empty()) {
		vbo = freeList.back();
		freeList.pop_back();
	} else {
		glGenBuffers(1, &vbo);
	}

	allocatedVBOs.push_back(vbo);
	return vbo;
}

void VBOAllocator::freeVBO(GLuint& vbo) {
	auto it = std::find(allocatedVBOs.begin(), allocatedVBOs.end(), vbo);
	if (it != allocatedVBOs.end()) {
		allocatedVBOs.erase(it);
		freeList.push_back(vbo);
	}
	vbo = 0;
}

void VBOAllocator::cleanup() {
	// Delete all allocated VBOs
	glDeleteBuffers(allocatedVBOs.size(), allocatedVBOs.data());
	glDeleteBuffers(freeList.size(), freeList.data());
	allocatedVBOs.clear();
	freeList.clear();
}
