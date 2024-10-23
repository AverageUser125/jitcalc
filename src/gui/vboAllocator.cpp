#include "vboAllocator.hpp"
#include "arenaAllocator.hpp"
#include <algorithm>

VBOAllocator::VBOAllocator() {
}

VBOAllocator::~VBOAllocator() {
	// DO NOT CALL cleanup() here!
	// it will cause a segfault since opengl is no longer
	// working at the after math of the window
}

void VBOAllocator::reserve(size_t amount) {
	assert(freeList.empty());
	freeList.resize(amount);
	glGenBuffers(amount, freeList.data());
}

GLuint VBOAllocator::allocateVBO() {
	GLuint vbo = 0;

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
