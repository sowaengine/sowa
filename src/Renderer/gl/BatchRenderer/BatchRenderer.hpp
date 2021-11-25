/**
 * @file BatchRenderer.hpp
 * @author Lexographics
 * @brief OpenGL batch renderer for 2D rendering
 * @version 0.1
 * @date 2021-11-16
 */
#pragma once

#include "glm/glm.hpp"
#include "Renderer/Texture/Texture.hpp"

namespace Ease
{
   
class BatchRenderer
{
public:
/**
 * @brief Usage: call begin2D -> call submitQuad for every sprite -> call flush2D
 */

   static void initialize();
   

   /* --------------- 2D --------------- */
   static void begin2D();
   static void submitQuad(glm::vec3 location, float rotation, glm::vec2 scale, const Texture& texture, glm::vec4 color = {1.f, 1.f, 1.f, 1.f});
   static void flush2D();


private:
   
};


} // namespace Ease