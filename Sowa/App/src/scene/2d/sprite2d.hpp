#ifndef _E_SPRITE2D_HPP__
#define _E_SPRITE2D_HPP__

#include "scene/node.hpp"
#include "math/math.hpp"
#include "scene/2d/node2d.hpp"

#include "resource/texture/image_texture.hpp"

namespace sowa {
class Sprite2D : public Node2D {
  public:
	Sprite2D();

	static std::string Typename() { return "sowa::Sprite2D"; }

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline Reference<ImageTexture>& Texture() { return m_texture; }

	static FileBuffer SaveImpl(object_type* out);
	static bool LoadImpl(object_type* out, const FileBuffer& buf);

  private:
	friend class Application;
	Reference<ImageTexture> m_texture;
};
} // namespace sowa

#endif // _E_SPRITE2D_HPP__