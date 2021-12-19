#include "Debug.hpp"
#include "SystemDriver.hpp"
#include "entt/entt.hpp"

#include "Components.hpp"

#include "Renderer/VertexArray.hpp"
#include "Renderer/VertexBuffer.hpp"
#include "Renderer/Shader.hpp"

#include "Global.hpp"

#include "Resource/Texture/Texture.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace Ease
{

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
//uniform mat4 u_MVP;

out vec3 color;
out vec2 texCoords;
void main()
{
  gl_Position = proj * view * model * vec4(aPos, 1.0f);

  color = aColor;
  texCoords = aTexCoords;
})";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 texCoords;

uniform sampler2D u_Texture;
void main()
{
   FragColor = texture(u_Texture, texCoords);
   //FragColor = vec4(texCoords, 1.0f, 1.0f);
   if(FragColor.a < 0.1)
      discard;
})";

GLfloat vertices[] =
{
   // POSITIONS                   COLOR(not used)          TEXCOORDS
   -0.5f,  0.5f, 0.f,           0.51f, 0.7f, 0.2f,        0.0f,  1.0f,
    0.5f,  0.5f, 0.f,           0.12f, 0.2f, 0.6f,        1.0f,  1.0f,
    0.5f, -0.5f, 0.f,           0.25f, 0.6f, 0.7f,        1.0f,  0.0f,
   -0.5f, -0.5f, 0.f,           0.92f, 0.1f, 0.1f,        0.0f,  0.0f,
};

GLuint indices[] =
{
   0, 1, 2,
   2, 3, 0
};

struct Vertex
{
   float position[3];
   float color[3];
   float uvcoords[2];
};



glm::mat4 view_mat_2d = glm::mat4(1.0f);
glm::mat4 proj_mat = glm::mat4(1.0f);


SystemDriver::SystemDriver(entt::registry* registry)
{
   m_pRegistry = registry;


   view_mat_2d = glm::mat4(1.0f);
   view_mat_2d = glm::translate(view_mat_2d, glm::vec3(0.f, 0.0f, -10.f));

   proj_mat = glm::perspective(glm::radians(45.f), 16.f/9.f, 0.1f, 100.f);
   proj_mat = glm::ortho(0.f, 1280.f, 0.f, 720.f, 0.1f, 100.f);
   proj_mat = glm::ortho(-640.f, 640.f, -360.f, 360.f, 0.1f, 100.f);



// SHADER
   m_Shader.CreateShader("shader.glsl");


// VERTEX ARRAY
   m_VertexArray = VertexArray::New();
   m_VertexArray->Bind();


// VERTEX BUFFERS
   m_PositionBuffer = VertexBuffer::New();
   m_PositionBuffer->Bind();
   m_PositionBuffer->BufferData(vertices, sizeof(vertices), GL_STATIC_DRAW);
   m_VertexArray->SetAttribPointer(0, 3, GL_FLOAT, false, sizeof(float) * 8, (const void*)0); // (const void*)offsetof(Vertex, position)
   m_VertexArray->SetAttribPointer(1, 3, GL_FLOAT, false, sizeof(float) * 8, (const void*)(3*sizeof(float)) );
   m_VertexArray->SetAttribPointer(2, 2, GL_FLOAT, false, sizeof(float) * 8, (const void*)(6*sizeof(float)) );

// INDEX BUFFER
   m_IndexBuffer = IndexBuffer::New();
   m_IndexBuffer->Bind();
   m_IndexBuffer->BufferData(indices, sizeof(indices), GL_STATIC_DRAW);
}


void SystemDriver::UpdateAll()
{
   { // Find current Camera2D
      //view_mat_2d = glm::mat4(1.0f);
      auto view = m_pRegistry->view<Component::Transform2D, Component::Camera2D>();
      for(auto entity: view) {
         auto& cam = view.get<Component::Camera2D>(entity);
         auto& tc = view.get<Component::Transform2D>(entity);

         if(cam.current)
         {
            view_mat_2d = glm::inverse(tc.Transform(1.f, 1.f, 10.f));

            break;
         }
      }
   }

   
   { // Render Sprite2D with Transform2D
      auto view = m_pRegistry->view<Component::Transform2D, Component::Sprite2D>();
      //glUseProgram(m_ShaderProgram);
      m_Shader.Bind();
      for(auto entity: view) {
         auto& tc = view.get<Component::Transform2D>(entity);
         auto& spritec = view.get<Component::Sprite2D>(entity);

         if(spritec.texture == nullptr) continue;
         //glViewport(0, 0, 1280, 720);

         glm::mat4 model_mat = glm::mat4(1.0f);
         model_mat = glm::translate(model_mat, {tc.position.x, tc.position.y, 0.f});
         model_mat = glm::rotate(model_mat, glm::radians(tc.rotation), {0.f, 0.f, 1.f});
         model_mat = glm::scale(model_mat, { spritec.texture->Width() * tc.scale.x, spritec.texture->Height() * tc.scale.y, 1.f});

         ////glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "model"), 1, GL_FALSE, &model_mat[0][0]);
         //glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "view"), 1, GL_FALSE,  &view_mat_2d[0][0]);
         //glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "proj"), 1, GL_FALSE,  &proj_mat[0][0]);
         //glUniformMatrix4fv(glGetUniformLocation(m_ShaderProgram, "model"), 1, GL_FALSE,  &tc.Transform(spritec.texture->Width(), spritec.texture->Height())[0][0]);
         m_Shader.BindUniform("view", view_mat_2d);
         m_Shader.BindUniform("proj", proj_mat);
         m_Shader.BindUniform("model", tc.Transform(spritec.texture->Width(), spritec.texture->Height()));


         spritec.texture->Bind(0);
         //glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_Texture"), 0);
         m_Shader.BindUniform("u_Texture", 0);

         
         m_VertexArray->Bind();
         glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      }
   }
}




} // namespace Ease
