#include "batch_renderer.hxx"

#include "core/graphics.hxx"
#include <algorithm>
#include <iostream>

#define BATCH2D_MAX_RECT (1000)
#define BATCH2D_MAX_VERTEX (BATCH2D_MAX_RECT * 6)
#define BATCH2D_MAX_TEXTURE 16

BatchRenderer::BatchRenderer() {
	m_vertices.reserve(BATCH2D_MAX_VERTEX);
}

BatchRenderer::~BatchRenderer() {
}

Error BatchRenderer::Init(const char *vsPath, const char *fsPath) {
	Error err = m_shader.Load(vsPath, fsPath);
	if (err != OK) {
		return err;
	}

	m_array.New();
	m_buffer.New(gfx::BufferType::VertexBuffer);

	m_array.Bind();
	m_buffer.Bind();

	m_buffer.BufferData(nullptr, sizeof(BatchVertex) * BATCH2D_MAX_VERTEX, gfx::BufferUsage::DynamicDraw);

	m_array.ResetAttributes();
	m_array.SetAttribute(0, gfx::AttributeType::Vec3);	// position
	m_array.SetAttribute(1, gfx::AttributeType::Vec2);	// uv
	m_array.SetAttribute(2, gfx::AttributeType::Vec4);	// color
	m_array.SetAttribute(3, gfx::AttributeType::Float); // draw id
	m_array.SetAttribute(4, gfx::AttributeType::Float); // texture slot
	m_array.SetAttribute(5, gfx::AttributeType::Float); // draw mode
	m_array.UploadAttributes();

	m_array.Unbind();

	if (m_blankTexture.ID() == 0) {
		unsigned char data[4] = {255, 255, 255, 255};
		Error err = m_blankTexture.Load2DUByteRGBA(data, 1, 1);
		if (err != OK) {
			std::cout << "Failed to load white texture" << std::endl;
		}
	}

	return OK;
}

void BatchRenderer::Reset() {
	m_vertices.clear();
	m_textures.clear();
	m_textureCounter = 0;
}

void BatchRenderer::PushQuad(BatchVertex vertices[4]) {

	// 1 2 3
	// 1 3 4
	for (int i = 0; i < 4; i++) {
		uint32_t textureID = static_cast<uint32_t>(vertices[i].t_id);
		if (textureID == 0) {
			textureID = m_blankTexture.ID();
		}

		if (m_textures[textureID] == 0) {
			m_textures[textureID] = ++m_textureCounter;
		}

		vertices[i].t_id = static_cast<float>(m_textures[textureID]) - 1.f;
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

void BatchRenderer::PushQuad(float x, float y, float z, float w, float h, float r, float g, float b, float a, float drawID, float textureID, float drawMode) {
	/*
		{ 0.0f, 1.0f,  0.f, 1.f}
		{ 0.0f, 0.0f,  0.f, 1.f}
		{ 1.0f, 0.0f,  0.f, 1.f}
		{ 1.0f, 1.0f,  0.f, 1.f}
		*/

	glm::vec4 points[4] = {
		{x, y + h, 0.f, 1.f},
		{x, y, 0.f, 1.f},
		{x + w, y, 0.f, 1.f},
		{x + w, y + h, 0.f, 1.f}};
	glm::vec2 uvs[4] = {
		{0.f, 1.f},
		{0.f, 0.f},
		{1.f, 0.f},
		{1.f, 1.f}};

	BatchVertex vertices[4];
	for (int i = 0; i < 4; i++) {
		vertices[i].x = points[i].x;
		vertices[i].y = points[i].y;
		vertices[i].z = z;
		vertices[i].r = r;
		vertices[i].g = g;
		vertices[i].b = b;
		vertices[i].a = a;
		vertices[i].u = uvs[i].x;
		vertices[i].v = uvs[i].y;
		vertices[i].t_id = textureID;
		vertices[i].d_id = drawID;
		vertices[i].draw_mode = drawMode;
	}

	PushQuad(vertices);
}

void BatchRenderer::DrawText(const std::string &text, Font *font, float x, float y, glm::mat4 transform, float draw_id, float scale, float maxWidth, float maxHeight) {
	glm::vec2 textSize = font->CalcTextSize(text);

	if (maxWidth > 0.f) {
		if (textSize.x > maxWidth) {
			scale = std::min(scale, maxWidth / textSize.x);
		}
	}

	if (maxHeight > 0.f) {
		if (textSize.y > maxHeight) {
			scale = std::min(scale, maxHeight / textSize.y);
		}
	}

	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++) {
		Font::Character ch = font->m_characters[*c];

		float xPos = x + ch.bearing.x * scale;
		float yPos = y - (ch.size.y - ch.bearing.y) * scale;

		float w = ch.size.x * scale;
		float h = ch.size.y * scale;

		glm::vec4 points[4] = {
			{xPos, yPos + h, 0.f, 1.f},
			{xPos, yPos, 0.f, 1.f},
			{xPos + w, yPos, 0.f, 1.f},
			{xPos + w, yPos + h, 0.f, 1.f}};

		glm::vec2 uvs[4] = {
			{0.f, 0.f},
			{0.f, 1.f},
			{1.f, 1.f},
			{1.f, 0.f}};

		BatchVertex vertices[4];
		for (int i = 0; i < 4; i++) {
			points[i] = transform * points[i];

			vertices[i].x = points[i].x;
			vertices[i].y = points[i].y;
			vertices[i].z = 0.f;
			vertices[i].r = 1.f;
			vertices[i].g = 1.f;
			vertices[i].b = 1.f;
			vertices[i].a = 1.f;
			vertices[i].u = uvs[i].x;
			vertices[i].v = uvs[i].y;
			vertices[i].t_id = ch.textureID;
			vertices[i].d_id = draw_id;
			vertices[i].draw_mode = 2.f;
		}

		PushQuad(vertices);
		x += (ch.advance >> 6) * scale;
	}
}

void BatchRenderer::End() {
	if (m_vertices.size() == 0) {
		return;
	}

	GetShader().Bind();
	m_buffer.Bind();
	m_buffer.BufferSubdata(m_vertices.data(), m_vertices.size() * sizeof(BatchVertex), 0);
	m_buffer.Unbind();

	std::vector<int> textures;
	for (const auto &[id, slot] : m_textures) {
		glActiveTexture(GL_TEXTURE0 + (slot - 1));
		glBindTexture(GL_TEXTURE_2D, id);
		textures.push_back(slot - 1);
	}

	std::sort(textures.begin(), textures.end());
	GetShader().Uniformiv("uTextures", textures);

	m_array.Bind();
	glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
	m_array.Unbind();

	Reset();
}