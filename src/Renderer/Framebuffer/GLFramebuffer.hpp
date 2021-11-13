#pragma once

#include "glad/glad.h"

namespace Ease
{

struct GLFrameBufferCreateInfo
{
   uint32_t width=1280, height=720;
   uint32_t samples = 1;

   bool swapChainTarget = false;
};


class GLFramebuffer
{
public:
   void bind();
   void unbind();
   void recreateFrameBuffer();


   GLFrameBufferCreateInfo createInfo;

   GLuint getID() { return m_BufferID; }

   ~GLFramebuffer();

   GLuint getColorAttachment() { return m_ColorAttachment; }
private:
   GLuint m_BufferID;

   GLuint m_ColorAttachment;
   GLuint m_DepthAttachment;
};
   
} // namespace Ease
