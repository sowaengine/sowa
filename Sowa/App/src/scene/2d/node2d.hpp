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
	static void Bind();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

	inline vec2 &Position() { return m_position; }
	inline double &Rotation() { return m_rotation; }
	inline vec2 &Scale() { return m_scale; }
	inline uint16_t &ZIndex() { return m_zIndex; }

	static FileBuffer SaveImpl(object_type *out);
	static bool LoadImpl(object_type *out, const FileBuffer &buf);

  protected:
	friend class Application;

	vec2 m_position = vec2{0.f, 0.f};
	double m_rotation = 0.0;
	vec2 m_scale = vec2{1.f, 1.f};
	uint16_t m_zIndex = 0;

	const glm::mat4 &CalculateTransform();

  private:
	glm::mat4 _LatestTransform{1.f};
	uint64_t _LastUpdateFrameID{24};
};
} // namespace sowa

#endif // _E_NODE2D_HPP__