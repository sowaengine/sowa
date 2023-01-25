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

  protected:
	Vector2 _Position{0.f, 0.f};
	double _Rotation{0.0};
	Vector2 _Scale{1.f, 1.f};
};
} // namespace Sowa

#endif // _E_NODE2D_HPP__