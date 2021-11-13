#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "glad/glad.h"

#include "Ease.hpp"
#include "Renderer/Shader/Shader.hpp"

namespace Ease
{

struct Vertex
{
   glm::vec3 position;
   glm::vec2 texCoords;
};

class Model
{
public:
   void setVertices(const std::vector<Vertex>& vertices);
   void setIndices(const std::vector<uint32_t>& indices);

   void setShader(Shader* shader);

// recreates model with m_Vertices, m_Indices
   void recreateModel();


private:
   friend class GLRenderer;

   std::vector<Vertex> m_Vertices;
   std::vector<uint32_t> m_Indices;

   GLuint m_VertexBuffer;
   GLuint m_IndexBuffer;
   Shader* m_Shader;
};
   
} // namespace Ease
