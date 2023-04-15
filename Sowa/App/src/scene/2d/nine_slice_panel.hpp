#ifndef _E_NINEPATCHRECT_HPP__
#define _E_NINEPATCHRECT_HPP__

#include "scene/2d/node2d.hpp"
#include "scene/node.hpp"
#include "utils/math.hpp"

#include "resource/texture/image_texture.hpp"
namespace Sowa {
class NineSlicePanel : public Node2D {
  public:
	NineSlicePanel();

	static std::string Typename() { return "Sowa::NineSlicePanel"; }

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline float& MarginLeft() { return m_MarginLeft; }
	inline float& MarginRight() { return m_MarginRight; }
	inline float& MarginTop() { return m_MarginTop; }
	inline float& MarginBottom() { return m_MarginBottom; }

	inline Vector2& Size() { return m_Size; }

	inline Reference<ImageTexture>& Texture() { return _Texture; }

	static FileBuffer SaveImpl(ObjectType* out);
	static bool LoadImpl(ObjectType* out, const FileBuffer& buf);

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

	Vector2 m_Size;
};
} // namespace Sowa

#endif // _E_SPRITE2D_HPP__