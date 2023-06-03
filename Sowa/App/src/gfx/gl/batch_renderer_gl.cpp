#include "batch_renderer_gl.hpp"

#include "./glfuncs.hpp"
#include "./graphics_gl.hpp"
#include "./texture_gl.hpp"
#include "core/stats.hpp"

#define BATCH2D_MAX_VERTEX 1000 * 6 // 1000 rect
#define BATCH2D_MAX_TEXTURE 16

namespace sowa {
namespace gfx {
void BatchRendererGL::Init(const std::string &vsource, const std::string &fsource) {
	m_shader.New(vsource, fsource);

	m_array.New();
	m_buffer.New(BufferType::VertexBuffer);
	m_array.Bind();
	m_buffer.Bind();

	m_buffer.BufferData(nullptr, sizeof(BatchVertex) * BATCH2D_MAX_VERTEX, BufferUsage::DynamicDraw);

	m_array.ResetAttributes();
	m_array.SetAttribute(0, GLAttributeType::Vec3);	 // position
	m_array.SetAttribute(1, GLAttributeType::Vec4);	 // color
	m_array.SetAttribute(2, GLAttributeType::Vec2);	 // uv
	m_array.SetAttribute(3, GLAttributeType::Float); // texture
	m_array.SetAttribute(4, GLAttributeType::Float); // drawID
	m_array.SetAttribute(5, GLAttributeType::Float); // drawMode
	m_array.UploadAttributes();

	m_array.Unbind();
}

void BatchRendererGL::Reset() {
	m_vertices.clear();
	m_textures.clear();
	m_textureCounter = 0;
}
void BatchRendererGL::PushQuad(BatchVertex vertices[4]) {
	static GLTexture whiteTexture;
	if (whiteTexture.ID() == 0) {
		unsigned char data[4] = {255, 255, 255, 255};
		whiteTexture.Load2DFromData(data, 1, 1, GLDataType::UByte, GLTextureFormat::Rgba);
	}
	// 1 2 3
	// 1 3 4

	for (int i = 0; i < 4; i++) {
		uint32_t textureId = static_cast<uint32_t>(vertices[i].textureId);
		if (textureId == 0) {
			textureId = whiteTexture.ID();
		}

		if (m_textures[textureId] == 0) {
			m_textures[textureId] = ++m_textureCounter;
		}
		vertices[i].textureId = static_cast<float>(m_textures[textureId]) - 1.f;
	}

	m_vertices.push_back(vertices[0]);
	m_vertices.push_back(vertices[1]);
	m_vertices.push_back(vertices[2]);

	m_vertices.push_back(vertices[0]);
	m_vertices.push_back(vertices[2]);
	m_vertices.push_back(vertices[3]);

	if (m_textures.size() >= BATCH2D_MAX_TEXTURE || m_vertices.size() >= BATCH2D_MAX_VERTEX) {
		End();
	}
}

void BatchRendererGL::PushLine(glm::vec2 p1, glm::vec2 p2, float thickness, glm::vec4 color, uint32_t id) {
	float rot = atan2(p1.y - p2.y, p1.x - p2.x) * 180 / 3.141592653;

	glm::vec2 sub = {p2.x - p1.x, p2.y - p1.y};
	float len = sqrt((sub.x * sub.x) + (sub.y * sub.y));

	mat4 mat = CalculateModelMatrix({p1.x, p1.y, 0.f}, {0.f, 0.f, rot}, {len, thickness, 1.f}, {-len / 2.f, 0.f, 0.f}, mat4(1.f));

	glm::vec4 points[4] = {
		{-0.5f, +0.5f, 0.f, 1.f},
		{-0.5f, -0.5f, 0.f, 1.f},
		{+0.5f, -0.5f, 0.f, 1.f},
		{+0.5f, +0.5f, 0.f, 1.f}};
	glm::vec2 uvs[4] = {
		{0.f, 1.f},
		{0.f, 0.f},
		{1.f, 0.f},
		{1.f, 1.f}};

	gfx::BatchVertex vertices[4];
	for (int i = 0; i < 4; i++) {
		points[i] = mat * points[i];

		vertices[i].x = points[i].x;
		vertices[i].y = points[i].y;
		vertices[i].z = 0.f;
		vertices[i].r = color.r;
		vertices[i].g = color.g;
		vertices[i].b = color.b;
		vertices[i].a = color.a;
		vertices[i].uvX = uvs[i].x;
		vertices[i].uvY = uvs[i].y;
		vertices[i].textureId = 0.f;
		vertices[i].id = id;
	}
	PushQuad(vertices);
}

void BatchRendererGL::End() {
	if (m_vertices.size() == 0) {
		return;
	}

	Shader().Bind();

	m_buffer.Bind();
	m_buffer.BufferSubdata(m_vertices.data(), m_vertices.size() * sizeof(BatchVertex), 0);
	m_buffer.Unbind();

	// m_textures[id] = slot + 1;
	std::vector<int> textures;
	for (const auto &[id, slot] : m_textures) {
		GL().activeTexture(slot - 1);
		GL().bindTexture(GLTextureType::Texture2D, id);
		textures.push_back(slot - 1);
	}
	std::sort(textures.begin(), textures.end());
	Shader().UniformIntVector("uTextures", textures);

	m_array.Bind();
	GL().drawArrays(GLDrawMode::Triangles, 0, m_vertices.size());
	Stats::Instance().Batch2DDrawCall();
	m_array.Unbind();

	Reset();
}
} // namespace gfx
} // namespace sowa
