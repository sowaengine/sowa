#include "sprite2d.hpp"

#include "debug.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include "gfx/graphics.hpp"
#include "core/renderer.hpp"
#include "core/application.hpp"
#include "scene/scene.hpp"
#include "resource/resource_loader.hpp"

namespace sowa {
Sprite2D::Sprite2D() {
	_NodeType = "Sprite2D";
}

void Sprite2D::EnterScene() {
}
void Sprite2D::ExitScene() {
}
void Sprite2D::UpdateLogic() {
}
void Sprite2D::UpdateDraw() {
	if (m_texture != nullptr) {
		Graphics().Default2DShader().Bind();
		Graphics().Default2DShader().UniformTexture("uTexture", m_texture->TextureID(), 0);

		mat4 transform = CalculateTransform();
		transform = glm::scale(transform, {m_texture->Width(), m_texture->Height(), 1.f});
		Graphics().Default2DShader().UniformMat4("uModel", transform);

		Graphics().DrawQuad();
	}
	// Renderer::get_singleton().DrawTexture(CalculateTransform(), *_Texture.get(), {(float)_Texture->Width(), (float)_Texture->Height()}, ID());
}

FileBuffer Sprite2D::SaveImpl(object_type *out) {
	Sprite2D *o = reinterpret_cast<Sprite2D *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<Node2D>(out).Yaml();
	
	return FileBuffer(doc);
}

bool Sprite2D::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Sprite2D::LoadImpl is not implemented");
	return false;
}
} // namespace sowa
