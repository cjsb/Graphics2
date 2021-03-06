#include "Buffer.h"

#include "Utils/UtilCollection.h"
#include <iostream>

Buffer::Buffer(GLenum target) : m_target(target)
{
    glCreateBuffers(1, &m_bufferHandle);
}

Buffer::~Buffer()
{
    if (glfwGetCurrentContext() != nullptr)
    {
        glDeleteBuffers(1, &m_bufferHandle);
    }
    if constexpr (util::debugmode)
    {
        util::getGLerror(__LINE__, __FUNCTION__);
        std::cout << "buffer destructor called" << std::endl;
    }
}

GLuint Buffer::getHandle() const
{
    return m_bufferHandle;
}

GLenum Buffer::getTarget() const
{
    return m_target;
}

size_t Buffer::getTypeSize() const
{
    return m_typeSize;
}

void Buffer::bindBase(unsigned int binding) const
{
    glBindBufferBase(m_target, binding, m_bufferHandle);
}

void Buffer::bindBase(BufferBindings::Binding binding) const
{
    glBindBufferBase(m_target, static_cast<int>(binding), m_bufferHandle);
}

void Buffer::unmapBuffer() const
{
    glUnmapNamedBuffer(m_bufferHandle);
}
