#ifndef _E_NODE2D_HPP__
#define _E_NODE2D_HPP__

#include "scene/node.hpp"
#include "utils/math.hpp"

#include "glm/glm.hpp"

namespace Sowa {
class Node2D : public Node {
  public:
	Node2D();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline Vector2& Position() { return _Position; }
	inline double& Rotation() { return _Rotation; }
	inline Vector2& Scale() { return _Scale; }

  protected:
	Vector2 _Position{0.f, 0.f};
	double _Rotation{0.0};
	Vector2 _Scale{1.f, 1.f};

	const glm::mat4& CalculateTransform();

  private:
	glm::mat4 _LatestTransform{1.f};
	uint64_t _LastUpdateFrameID{24};
};
} // namespace Sowa

#endif // _E_NODE2D_HPP__