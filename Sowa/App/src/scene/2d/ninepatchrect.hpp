#ifndef _E_NINEPATCHRECT_HPP__
#define _E_NINEPATCHRECT_HPP__

#include "scene/2d/node2d.hpp"
#include "scene/node.hpp"
#include "utils/math.hpp"

#include "resource/texture/ninepatch_texture.hpp"

namespace Sowa {
class NinePatchRect : public Node2D {
  public:
	NinePatchRect();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline Reference<NinePatchTexture>& Texture() { return _Texture; }

  private:
	friend class Application;
	Reference<NinePatchTexture> _Texture;
};
} // namespace Sowa

#endif // _E_SPRITE2D_HPP__