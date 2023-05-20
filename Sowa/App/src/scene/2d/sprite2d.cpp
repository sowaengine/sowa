#include "sprite2d.hpp"

#include "debug.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "gfx/graphics.hpp"
#include "core/renderer.hpp"
#include "core/application.hpp"
#include "scene/scene.hpp"
#include "resource/resource_loader.hpp"
#include "core/node_db.hpp"

namespace sowa {
Sprite2D::Sprite2D() {
	_NodeType = "Sprite2D";
}

void Sprite2D::Bind() {
	NodeFactory factory;
	factory.construct = []() -> Node* {
		Node* node = Allocator<Sprite2D>::Get().allocate(1);
		new (node) Sprite2D;

		return node;
	};

	factory.destruct = [](Node* node) {
		Allocator<Sprite2D>::Get().deallocate(reinterpret_cast<Sprite2D *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("Sprite2D", "Node2D", factory);

	Serializer::get_singleton().RegisterSerializer(Sprite2D::Typename(), SerializeImpl(Sprite2D::SaveImpl, Sprite2D::LoadImpl));

	NodeDB::Instance().RegisterAttribute<std::string>("Sprite2D", "texture.path", [](Node* node) -> std::string {
		Debug::Error("Impelement Sprite2D Attribute getter texture.path");
		return "";
	}, [](Node* node, std::string path) {
		Sprite2D* sprite = dynamic_cast<Sprite2D*>(node);
		if(nullptr != sprite) {
			sprite->Texture() = ResourceLoader::get_singleton().LoadResource<ImageTexture>(path);
		}
	});
}

void Sprite2D::EnterScene() {
}
void Sprite2D::ExitScene() {
}
void Sprite2D::UpdateLogic() {
}
void Sprite2D::UpdateDraw() {
	if (m_texture != nullptr) {
		const float outlineWidth = 10.f; // todo: make outline width fixed size (proportional to camera zoom)
		mat4 nodeTransform = CalculateTransform();
		mat4 transform = glm::scale(nodeTransform, {m_texture->Width(), m_texture->Height(), 1.f});

		for(int i=0; i<8; i++) {
			glm::vec3 offset = glm::rotateZ(glm::vec3{outlineWidth, 0.f, 0.f}, glm::radians(i * 45.f));
			mat4 newTransform = glm::translate(nodeTransform, offset);
			newTransform = glm::scale(newTransform, {m_texture->Width(), m_texture->Height(), 1.f});

			glm::vec4 points[4] = {
				{-0.5f, +0.5f, 0.f, 1.f},
				{-0.5f, -0.5f, 0.f, 1.f},
				{+0.5f, -0.5f, 0.f, 1.f},
				{+0.5f, +0.5f, 0.f, 1.f}
			};
			glm::vec2 uvs[4] = {
				{0.f, 1.f},
				{0.f, 0.f},
				{1.f, 0.f},
				{1.f, 1.f}
			};

			gfx::BatchVertex vertices[4];
			for(int i=0; i<4; i++) {
				points[i] = newTransform * points[i];

				vertices[i].x = points[i].x;
				vertices[i].y = points[i].y;
				vertices[i].z = 0.f;
				vertices[i].r = 0.2f;
				vertices[i].g = 0.68f;
				vertices[i].b = 0.81f;
				vertices[i].a = 1.f;
				vertices[i].uvX = uvs[i].x;
				vertices[i].uvY = uvs[i].y;
				vertices[i].textureId = 0.f;
			}
			Graphics().Batch2DPushQuad(vertices);

			Graphics().DrawQuad();
		}

		glm::vec4 points[4] = {
			{-0.5f, +0.5f, 0.f, 1.f},
			{-0.5f, -0.5f, 0.f, 1.f},
			{+0.5f, -0.5f, 0.f, 1.f},
			{+0.5f, +0.5f, 0.f, 1.f}
		};
		glm::vec2 uvs[4] = {
			{0.f, 1.f},
			{0.f, 0.f},
			{1.f, 0.f},
			{1.f, 1.f}
		};

		gfx::BatchVertex vertices[4];
		for(int i=0; i<4; i++) {
			points[i] = transform * points[i];

			vertices[i].x = points[i].x;
			vertices[i].y = points[i].y;
			vertices[i].z = 0.f;
			vertices[i].r = 1.f;
			vertices[i].g = 1.f;
			vertices[i].b = 1.f;
			vertices[i].a = 1.f;
			vertices[i].uvX = uvs[i].x;
			vertices[i].uvY = uvs[i].y;
			vertices[i].textureId = static_cast<float>(m_texture->TextureID());
			vertices[i].id = static_cast<float>(ID());
		}
		Graphics().Batch2DPushQuad(vertices);

		// Graphics().Default2DShader().Bind();
		// Graphics().Default2DShader().UniformTexture("uTexture", m_texture->TextureID(), 0);
		// Application::get_singleton().BindProjectionUniform(Graphics().Default2DShader(), "uProj");

		// Graphics().Default2DShader().UniformMat4("uModel", transform);

		// Graphics().DrawQuad();
	}
}

FileBuffer Sprite2D::SaveImpl(object_type *out) {
	Sprite2D *o = reinterpret_cast<Sprite2D *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<Sprite2D>(out).Yaml();
	
	return FileBuffer(doc);
}

bool Sprite2D::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Sprite2D::LoadImpl is not implemented");
	return false;
}
} // namespace sowa
