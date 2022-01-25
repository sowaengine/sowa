/**
 * @file Renderer.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Renderer.hpp"  

#include <EaseGL.hpp>
#include "Resource/Project.hpp"
#include <iostream>

Renderer::Renderer()
{
}

void Renderer::Init() 
{
   static const float vertices[] = {
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f,     1.0f, 0.2f, 0.4f,  // top right
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f,     0.0f, 1.0f, 1.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,     1.0f, 0.0f, 1.0f,  // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f,     1.0f, 1.0f, 0.0f,  // top left 
   };
   static const unsigned int indices[] = {
      0, 1, 3, 
      1, 2, 3  
   };

   draw2d.VertexArray = std::make_unique<EaseGL::VertexArray>();

   draw2d.VertexBuffer = std::make_unique<EaseGL::GLBuffer>(EaseGL::GLBufferType::ARRAY_BUFFER);
   draw2d.VertexBuffer->BufferData(vertices, sizeof(vertices), EaseGL::GLBufferUsage::STATIC_DRAW);

   draw2d.IndexBuffer = std::make_unique<EaseGL::GLBuffer>(EaseGL::GLBufferType::ELEMENT_ARRAY_BUFFER);
   draw2d.IndexBuffer->BufferData(indices, sizeof(indices), EaseGL::GLBufferUsage::STATIC_DRAW);
   

   draw2d.VertexArray->SetAttribute(0, EaseGL::AttributeType::VEC3);
   draw2d.VertexArray->SetAttribute(1, EaseGL::AttributeType::VEC2);
   draw2d.VertexArray->SetAttribute(2, EaseGL::AttributeType::VEC3);
   draw2d.VertexArray->UploadAttributes();

   EaseGL::VertexArray::Unbind();


   draw2d.ShaderProgram.LoadShader(Project::get_singleton().GetAbsolutePath("shader.glsl").c_str());
}
	
Renderer::~Renderer()
{
	
}


void Renderer::Begin2D() 
{
   draw2d.VertexArray->Bind();
   draw2d.ShaderProgram.Bind();
}


void Renderer::DrawSprite(
      const glm::mat4& model,
      const glm::mat4& cameraTransform,
      const glm::mat4& proj,
      EaseGL::GLTexture* ptexture) 
{
   draw2d.ShaderProgram.Uniform("u_Model", model);
   draw2d.ShaderProgram.Uniform("u_View", glm::inverse(cameraTransform));
   draw2d.ShaderProgram.Uniform("u_Proj", proj);

   draw2d.ShaderProgram.Uniform("tex", *ptexture, 0);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}



Renderer& Renderer::get_singleton() 
{
   static Renderer renderer;
   return renderer;
}


