#ifndef _E_RENDERER_HPP__
#define _E_RENDERER_HPP__

#pragma once

#include <glm/glm.hpp>
#include <string>
#include "Resource/Texture/Texture.hpp"

namespace Ease
{

class Renderer
{
public:
    static Renderer& get_singleton()
    {
        static Renderer renderer;
        return renderer;
    }


    void DrawQuad(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, Texture& texture, const glm::vec2& uv1, const glm::vec2& uv2);
    void DrawQuadColor(const glm::vec2& center, const glm::vec2& size, float rotation, Color color, const glm::vec2& offset = glm::vec2{0.f, 0.f});
    void DrawCircleColor(const glm::vec2& center, float radius, float rotation, Color color, const glm::vec2& offset = glm::vec2{0.f, 0.f});
    void DrawText(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color);
private:
    Renderer();
    ~Renderer();
};

}
#endif