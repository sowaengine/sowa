#ifndef SW_BATCH_RENDERER_HXX
#define SW_BATCH_RENDERER_HXX
#pragma once

#include <map>
#include <stdint.h>
#include <vector>

#include "core/error/error.hxx"
#include "gfx/buffer.hxx"
#include "gfx/vertex_array.hxx"
#include "shader.hxx"
#include "texture.hxx"

struct BatchVertex {
	float x;
	float y;
	float z;
	float u;
	float v;
	float r;
	float g;
	float b;
	float a;
	float d_id; // draw id
	float t_id; // texture id
};

class BatchRenderer {
  public:
	BatchRenderer();
	~BatchRenderer();

	Error Init(const char *vsPath, const char *fsPath);

	void Reset();
	void PushQuad(BatchVertex vertices[4]);
	void End();

	inline Shader &GetShader() { return m_shader; }
	inline Texture &BlankTexture() { return m_blankTexture; }

  private:
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