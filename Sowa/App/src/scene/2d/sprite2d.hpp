#ifndef _E_SPRITE2D_HPP__
#define _E_SPRITE2D_HPP__

#include "scene/node.hpp"
#include "utils/math.hpp"
#include "scene/2d/node2d.hpp"

#include "resource/texture/texture.hpp"

namespace Sowa {
class Sprite2D : public Node2D {
  public:
	Sprite2D();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

  private:
	friend class Application;
	Reference<ImageTexture> _Texture;
};
} // namespace Sowa

#endif // _E_SPRITE2D_HPP__