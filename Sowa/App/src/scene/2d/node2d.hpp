#ifndef _E_NODE2D_HPP__
#define _E_NODE2D_HPP__

#include "scene/node.hpp"
#include "utils/math.hpp"

namespace Sowa {
class Node2D : public Node {
  public:
	Node2D();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

  private:
	Vec2 _Position{0.f, 0.f};
	float _Rotation{0.f};
	Vec2 _Scale{1.f, 1.f};
};
} // namespace Sowa

#endif // _E_NODE2D_HPP__