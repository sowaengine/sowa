#include "sprite2d.hpp"

#include "debug.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

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
		const float outlineWidth = 3.f;
		mat4 transform = CalculateTransform();
		transform = glm::scale(transform, {m_texture->Width(), m_texture->Height(), 1.f});

		Application::get_singleton().BindViewUniform(Graphics().DefaultSolidColorShader(), "uProj");
		Application::get_singleton().BindViewUniform(Graphics().Default2DShader(), "uView");

		Graphics().DefaultSolidColorShader().Bind();
		Application::get_singleton().BindProjectionUniform(Graphics().DefaultSolidColorShader(), "uProj");
		Graphics().DefaultSolidColorShader().UniformTexture("uTexture", m_texture->TextureID(), 0);
		Graphics().DefaultSolidColorShader().UniformVec4("uColor", {0.2f, 0.68f, 0.81f, 1.0f});
		for(int i=0; i<8; i++) {
			glm::vec3 offset = glm::rotateZ(glm::vec3{outlineWidth, 0.f, 0.f}, glm::radians(i * 45.f));
			mat4 newTransform = glm::translate(CalculateTransform(), offset);
			newTransform = glm::scale(newTransform, {m_texture->Width(), m_texture->Height(), 1.f});

			Graphics().DefaultSolidColorShader().UniformMat4("uModel", newTransform);

			Graphics().DrawQuad();
		}

		Graphics().Default2DShader().Bind();
		Graphics().Default2DShader().UniformTexture("uTexture", m_texture->TextureID(), 0);
		Application::get_singleton().BindProjectionUniform(Graphics().Default2DShader(), "uProj");

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
