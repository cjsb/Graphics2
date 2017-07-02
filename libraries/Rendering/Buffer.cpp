#include "Buffer.h"

Buffer::Buffer() {
	glGenBuffers(1, &m_bufferHandle);
}

void Buffer::del() {
	glDeleteBuffers(1, &m_bufferHandle);
	util::getGLerror(__LINE__, __FUNCTION__);
}

GLuint Buffer::getHandle() {
	return m_bufferHandle;
}

void Buffer::bind() {
	glBindBuffer(m_target, m_bufferHandle);
}

void Buffer::bindBase(unsigned int binding) {
	glBindBufferBase(m_target, binding, m_bufferHandle);
}