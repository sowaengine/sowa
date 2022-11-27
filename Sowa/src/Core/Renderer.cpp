#include "Core/Renderer.hpp"
#include "Resource/Texture/Texture.hpp"

#include "Core/Application.hpp"
#include "Core/nm_Matrix.hpp"
#include "nmGfx/src/Core/nm_Renderer.hpp"
#include <vector>

namespace Sowa {
Renderer::Renderer() {
}

Renderer::~Renderer() {
}

void Renderer::DrawTexture(const glm::vec2 &position, const glm::vec2 &scale, float zIndex, float rotation, Sowa::ImageTexture &texture, uint32_t id /*= 0*/) {
	Application::get_singleton()._renderer->DrawTexture(&texture._texture, nmGfx::CalculateModelMatrix({position.x, position.y, zIndex}, {0.f, 0.f, rotation}, {scale.x, scale.y, 1.f}), {1.f, 1.f, 1.f}, id);
}

} // namespace Sowa