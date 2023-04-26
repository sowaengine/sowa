#ifndef _E_NINEPATCHRECT_HPP__
#define _E_NINEPATCHRECT_HPP__

#include "scene/2d/node2d.hpp"
#include "scene/node.hpp"
#include "math/math.hpp"

#include "resource/texture/image_texture.hpp"
namespace sowa {
class NineSlicePanel : public Node2D {
  public:
	NineSlicePanel();

	static std::string Typename() { return "sowa::NineSlicePanel"; }

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline float& MarginLeft() { return m_MarginLeft; }
	inline float& MarginRight() { return m_MarginRight; }
	inline float& MarginTop() { return m_MarginTop; }
	inline float& MarginBottom() { return m_MarginBottom; }

	inline vec2& Size() { return m_Size; }

	inline Reference<ImageTexture>& Texture() { return _Texture; }

	static FileBuffer SaveImpl(object_type* out);
	static bool LoadImpl(object_type* out, const FileBuffer& buf);

  private:
	friend class Application;
	Reference<ImageTexture> _Texture;

	/**
	 * Margin in pixels
	*/
	float m_MarginLeft;
	float m_MarginRight;
	float m_MarginTop;
	float m_MarginBottom;

	vec2 m_Size;
};
} // namespace sowa

#endif // _E_SPRITE2D_HPP__