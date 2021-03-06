#pragma once
#include "Texture.h"

class FrameBuffer
{
public:
    explicit FrameBuffer(const std::vector<std::shared_ptr<Texture>>& rendertargets, const bool useDepthStencil = true, const GLenum renderbufferFormat = GL_DEPTH24_STENCIL8, int samples = 0);

    // CAUTION: This constructor is only for rendering exclusively to a depth attachment
    FrameBuffer(GLenum attachmentType, const Texture& depthAttachment);

    FrameBuffer(const int width, const int height, const bool useDepthStencil = true, const GLenum renderbufferFormat = GL_DEPTH24_STENCIL8);
    ~FrameBuffer();

    void bind() const;
    void unbind() const;

    GLuint getName() const;

private:
    void attachDepthStencil(const int width, const int height, const GLenum renderbufferFormat);
    void attachDepthStencilMultiSample(int width, int height, int samples, GLenum renderbufferFormat);
    GLuint m_name;
    GLuint m_rbo;
};
