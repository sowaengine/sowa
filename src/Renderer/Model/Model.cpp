#include "Model.hpp"

#include "glad/glad.h"
#include "Renderer/gl/glRenderer.hpp"

#include <glm/glm.hpp>

namespace Ease
{

void Model::setVertices(const std::vector<Vertex>& vertices)
{
   m_Vertices = vertices;
}

void Model::setIndices(const std::vector<uint32_t>& indices)
{
   m_Indices = indices;
}

void Model::setShader(Shader* shader)
{
   m_Shader = shader;
}

void Model::recreateModel()
{
   /** 
    * !Clear old buffers here
    */

   // Generate Vertex Buffer
   glGenBuffers(1, &m_VertexBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*m_Vertices.size(), m_Vertices.data(), GL_STATIC_DRAW );

   // Attributes
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);// (const void*)offsetof(Vertex, position) );

   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texCoords));
   

   // Generate index array
   glGenBuffers(1, &m_IndexBuffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*m_Indices.size(), m_Indices.data(), GL_STATIC_DRAW);
}

   
} // namespace Ease
