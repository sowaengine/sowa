#ifndef SW_BATCH_RENDERER_HXX
#define SW_BATCH_RENDERER_HXX
#pragma once

#include <map>
#include <stdint.h>
#include <vector>

#include "core/error/error.hxx"
#include "gfx/buffer.hxx"
#include "gfx/vertex_array.hxx"
#include "math/math.hxx"
#include "resource/font/font.hxx"
#include "resource/image_texture/image_texture.hxx"
#include "shader.hxx"

struct BatchVertex {
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float u = 0.f;
	float v = 0.f;
	float r = 1.f;
	float g = 1.f;
	float b = 1.f;
	float a = 1.f;
	float d_id = 0.f;	   // draw id
	float t_id = 0.f;	   // texture id
	float draw_mode = 0.f; // 0.f -> sprite2d, 1.f -> ui container, 2.f -> text, 3.f -> hollow, 4.f -> circle hollow
	float clipRect_x = 0.f;
	float clipRect_y = 0.f;
	float clipRect_w = 1920.f;
	float clipRect_h = 1080.f;
};

class BatchRenderer {
  public:
	BatchRenderer();
	~BatchRenderer();

	ErrorCode Init(const char *vsPath, const char *fsPath);
	ErrorCode Init(unsigned char vsData[], size_t vsLength, unsigned char fsData[], size_t fsLength);

	void Reset();
	void PushQuad(BatchVertex vertices[4]);
	void PushQuad(float x, float y, float z, float w, float h, float r, float g, float b, float a, float drawID, float textureID, float drawMode = 0.f);
	void PushQuad(float x, float y, float z, float w, float h, float r, float g, float b, float a, float drawID, float textureID, rect clipRect, float drawMode = 0.f);
	void PushLine(const vec2 &p1, const vec2 &p2, float thickness, float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f, float z = 0.f);
	void draw_text(const std::string &text, Font *font, float x, float y, float z, glm::mat4 transform, float draw_id = 0.f, float scale = 1.0f, float maxWidth = 0.f, float maxHeight = 0.f, float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f, rect clipRect = rect(-1.f, -1.f, -1.f, -1.f));
	void End();

	inline Shader &GetShader() { return m_shader; }
	inline Texture &BlankTexture() { return m_blankTexture; }

  private:
	ErrorCode init();

	Shader m_shader;
	gfx::Buffer m_buffer;
	gfx::VertexArray m_array;
	std::vector<BatchVertex> m_vertices;

	// textures[textureID] = slot;
	std::map<uint32_t, uint32_t> m_textures;
	uint32_t m_textureCounter = 0;

	Texture m_blankTexture;
};

#endif // SW_BATCH_RENDERER_HXX