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
#include <glm/glm.hpp>
#include <array>

Renderer::Renderer()
{
}

void Renderer::Init() 
{
   SetupBatch2D();

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



const int MAX_QUAD_PER_BATCH = 1000;
void Renderer::SetupBatch2D() 
{
   glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &batch2d.max_texture_units);
   
   batch2d.VertexArray = std::make_unique<EaseGL::VertexArray>();
   batch2d.VertexBufferObject = std::make_unique<EaseGL::GLBuffer>(EaseGL::GLBufferType::ARRAY_BUFFER);
   batch2d.IndexBuffer = std::make_unique<EaseGL::GLBuffer>(EaseGL::GLBufferType::ELEMENT_ARRAY_BUFFER);

   batch2d.ShaderProgram.LoadShader(Project::get_singleton().GetAbsolutePath("batch2d.glsl").c_str());


   std::array<uint32_t, MAX_QUAD_PER_BATCH * 6> indices;
   uint32_t offset = 0;
   for(int i=0; i<MAX_QUAD_PER_BATCH; i+=6)
   {
      indices[i+0] = offset + 0;
      indices[i+1] = offset + 1;
      indices[i+2] = offset + 3;

      indices[i+3] = offset + 1;
      indices[i+4] = offset + 2;
      indices[i+5] = offset + 3;

      offset += 4;
   }
   batch2d.IndexBuffer->BufferData(indices.data(), indices.size() * sizeof(uint32_t), EaseGL::GLBufferUsage::STATIC_DRAW);
   for(int i=0; i<100; i++)
   {
      std::cout << indices[i] << std::endl;
   }

   batch2d.VertexArray->SetAttribute(0, EaseGL::AttributeType::VEC2);
   batch2d.VertexArray->SetAttribute(1, EaseGL::AttributeType::VEC2);
   batch2d.VertexArray->SetAttribute(2, EaseGL::AttributeType::FLOAT);
   batch2d.VertexArray->UploadAttributes();

   batch2d.VertexBufferObject->BufferData(nullptr, MAX_QUAD_PER_BATCH * 4 * sizeof(Batch2D::Vertex), EaseGL::GLBufferUsage::DYNAMIC_DRAW);

   EaseGL::VertexArray::Unbind();

   

   batch2d.vertexBuffer = new Batch2D::Vertex[MAX_QUAD_PER_BATCH];
}


void Renderer::BeginBatch2D()
{
   batch2d.VertexArray->Bind();
   batch2d.ShaderProgram.Bind();
   batch2d.UsedTextures.clear();


   batch2d.vertexCount = 0;
   batch2d.quadCount = 0;
   batch2d.vertexCurrent = batch2d.vertexBuffer;
   batch2d.currentTextureSlot = 0;
}

void Renderer::DrawBatch2D(const glm::mat4& model, const glm::mat4& cameraTransform, const glm::mat4& proj, EaseGL::GLTexture* ptexture)
{
   constexpr glm::vec2 textureCoords[] = { { 1.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f } };
   constexpr glm::vec4 vertexPositions[] = {
      {  0.5f,  0.5f, 0.0f, 1.0f },
		{  0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f,  0.5f, 0.0f, 1.0f }
   };


   if(batch2d.quadCount >= MAX_QUAD_PER_BATCH || batch2d.UsedTextures.size() >= batch2d.max_texture_units)
      EndBatch2D();
   
   float textureSlot = -1.f;
   for(auto tex : batch2d.UsedTextures)
   {
      if(tex.first == (GLuint)*ptexture)
      {
         textureSlot = tex.second;
      }
   }
   if(textureSlot < 0)
   {
      batch2d.ShaderProgram.Uniform("u_Textures", *ptexture, batch2d.currentTextureSlot);
      batch2d.UsedTextures[(GLuint)*ptexture] = batch2d.currentTextureSlot;
      textureSlot = batch2d.currentTextureSlot;
      batch2d.currentTextureSlot++;
   }
   // move proj and view to shader
   for(int i=0; i<4; i++)
   {
      glm::mat4 view = glm::inverse(cameraTransform);
      batch2d.vertexCurrent->position = proj * view * model * vertexPositions[i];
      batch2d.vertexCurrent->texCoords = textureCoords[i];
      batch2d.vertexCurrent->texID = textureSlot;
      batch2d.vertexCurrent++;
   }
   
   batch2d.vertexCount += 6;
   batch2d.quadCount++;
}


void Renderer::EndBatch2D()
{
   uint32_t dataSize = (uint32_t)((uint8_t*)batch2d.vertexCurrent - (uint8_t*)batch2d.vertexBuffer);
   batch2d.VertexBufferObject->BufferSubData(batch2d.vertexBuffer,0 ,dataSize);

   glDrawElements(GL_TRIANGLES, batch2d.quadCount*6, GL_UNSIGNED_INT, 0);

   batch2d.UsedTextures.clear();
   batch2d.vertexCount = 0;
   batch2d.quadCount = 0;
   batch2d.vertexCurrent = batch2d.vertexBuffer;
   batch2d.currentTextureSlot = 0;

   EaseGL::VertexArray::Unbind();
}