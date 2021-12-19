/**
 * @file SystemDriver.hpp
 * @author Lexographics
 * @brief class file that updates all component groups
 * @version 0.1
 * @date 2021-12-16
 */
#pragma once

#include "entt/entt.hpp"
#include "glad/glad.h"

#include "Renderer/VertexArray.hpp"
#include "Renderer/Shader.hpp"
#include "Resource/ResourceManager/ResourceManager.hpp"

namespace Ease
{
   
class SystemDriver
{
   public:
      SystemDriver(entt::registry* registry);

      // Updates all component Groups
      void UpdateAll();
   private:
      entt::registry* m_pRegistry = nullptr;


      GLuint m_VertexShader = 0;
      GLuint m_FragmentShader = 0;
      GLuint m_ShaderProgram = 0;
      Shader m_Shader;
      std::shared_ptr<VertexArray> m_VertexArray;
      std::shared_ptr<VertexBuffer> m_PositionBuffer;
      std::shared_ptr<VertexBuffer> m_ColorBuffer;
      std::shared_ptr<IndexBuffer> m_IndexBuffer;

      std::shared_ptr<Texture> tex;
};

} // namespace Ease
