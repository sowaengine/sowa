#ifndef SW_RENDER_LAYER_HXX
#define SW_RENDER_LAYER_HXX
#pragma once

#include <map>
#include <stdint.h>

namespace RenderLayerTargetType {
const int Vec4 = 0;
const int Int = 1;
} // namespace RenderLayerTargetType
typedef int renderlayer_type_t;

struct RenderLayerTarget {
	renderlayer_type_t type;
	uint32_t textureID;
};

class RenderLayer {
  public:
	RenderLayer() = default;
	~RenderLayer();

	void Create(int width, int height);
	void Resize(int width, int height);
	void Delete();

	void SetTarget(uint32_t slot, renderlayer_type_t type);

	void Bind();
	void Unbind();

	void Clear(float r, float g, float b, float a);
	int GetTargetTextureID(int slot);

	int ReadAttachmentInt(int slot, int x, int y);

	inline int Width() { return m_width; }
	inline int Height() { return m_height; }

  private:
	std::map<uint32_t, RenderLayerTarget> m_targets;
	int m_width = 0;
	int m_height = 0;

	uint32_t m_id = 0;
	uint32_t m_rbo = 0;
};

#endif // SW_RENDER_LAYER_HXX