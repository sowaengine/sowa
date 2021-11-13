#include "GLFramebuffer.hpp"

#include "glad/glad.h"

#include "Ease.hpp"

namespace Ease
{

void GLFramebuffer::recreateFrameBuffer()
{
   glGenFramebuffers(1, &m_BufferID);
   glBindFramebuffer(GL_FRAMEBUFFER, m_BufferID);

   glGenTextures(1, &m_ColorAttachment);
   glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, createInfo.width, createInfo.height,0 ,GL_RGB, GL_UNSIGNED_BYTE, nullptr);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
   

   
   glGenRenderbuffers(1, &m_DepthAttachment);
   glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, createInfo.width, createInfo.height);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthAttachment);

   auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   if(fboStatus != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Framebuffer Error: " << fboStatus << std::endl;
   }

}

void GLFramebuffer::bind()
{
   glBindFramebuffer(GL_FRAMEBUFFER, m_BufferID);
}
void GLFramebuffer::unbind()
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
GLFramebuffer::~GLFramebuffer()
{
   //unbind();
   // dont delete frame buffer, it results in a segfault. check later
   //glDeleteFramebuffers(1, &m_BufferID);
   
   
}
   
} // namespace Ease