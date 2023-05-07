#ifndef _E_NODE2D_HPP__
#define _E_NODE2D_HPP__

#include "math/math.hpp"
#include "scene/node.hpp"

#include "glm/glm.hpp"

namespace sowa {
class Node2D : public Node {
  public:
	Node2D();

	static std::string Typename() { return "sowa::Node2D"; }

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline vec2 &Position() { return _Position; }
	inline double &Rotation() { return _Rotation; }
	inline vec2 &Scale() { return _Scale; }

	static FileBuffer SaveImpl(object_type *out);
	static bool LoadImpl(object_type *out, const FileBuffer &buf);

  protected:
	friend class Application;

	vec2 _Position{0.f, 0.f};
	double _Rotation{0.0};
	vec2 _Scale{1.f, 1.f};

	const glm::mat4 &CalculateTransform();

  private:
	glm::mat4 _LatestTransform{1.f};
	uint64_t _LastUpdateFrameID{24};
};
} // namespace sowa

#endif // _E_NODE2D_HPP__