#ifndef VBO_ALLOCATOR_H
#define VBO_ALLOCATOR_H

#include <vector>
#include <glad/glad.h>

class VBOAllocator {
  public:
	VBOAllocator();
	~VBOAllocator();

	void reserve(size_t amount);
	GLuint allocateVBO();
	void freeVBO(GLuint& vbo);
	void cleanup();

	static constexpr size_t DEFAULT_VBO_RESERVE_AMOUNT = 10;

  private:
	std::vector<GLuint> freeList;	   // Holds freed VBOs
	std::vector<GLuint> allocatedVBOs; // Holds all allocated VBOs
};

#endif // VBO_ALLOCATOR_H
