#ifndef SW_VERTEX_ARRAY_GL_HPP_
#define SW_VERTEX_ARRAY_GL_HPP_

#include <cstdint>
#include <vector>

namespace sowa {
namespace gfx {

enum class GLAttributeType {
    None = 0,
    Float,
    Vec2,
    Vec3,
    Vec4,
};

struct GLAttribute {
	GLAttributeType type;
	uint32_t slot;

	bool operator<(const GLAttribute &other) const {
		return slot < other.slot;
	}

	GLAttribute(uint32_t slot_, GLAttributeType type_)
		: slot(slot_), type(type_) {}
};

class GLVertexArray {
  public:
	GLVertexArray() = default;
	~GLVertexArray();

	void New();
    void Delete();

	void Bind();
	void Unbind();

	void ResetAttributes();
	void SetAttribute(uint32_t slot, GLAttributeType type);
	void UploadAttributes();

  private:
	uint32_t m_id;

    uint32_t m_attributeSizeInBytes = 0;
    std::vector<GLAttribute> m_usedAttributes;

    friend class GLMesh;
};

} // namespace gfx
} // namespace sowa

#endif // SW_VERTEX_ARRAY_GL_HPP_