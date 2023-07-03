#ifndef SW_GFX_VERTEX_ARRAY_HXX
#define SW_GFX_VERTEX_ARRAY_HXX
#pragma once

#include "../gl.hxx"

#include <vector>

namespace gfx {

typedef int attribute_t;
namespace AttributeType {
const int Float = 0;
const int Vec2 = 2;
const int Vec3 = 3;
const int Vec4 = 4;

inline GLenum GetAttributeType(attribute_t attrib) {
	return attrib == Float	? GL_FLOAT
		   : attrib == Vec2 ? GL_FLOAT
		   : attrib == Vec3 ? GL_FLOAT
		   : attrib == Vec4 ? GL_FLOAT
							: GL_NONE;
}

inline int GetComponentCount(attribute_t attrib) {
	return attrib == Float	? 1
		   : attrib == Vec2 ? 2
		   : attrib == Vec3 ? 3
		   : attrib == Vec4 ? 4
							: 0;
}

inline int GetAttributeSize(attribute_t attrib) {
	return attrib == Float	? sizeof(GLfloat)
		   : attrib == Vec2 ? sizeof(GLfloat) * 2
		   : attrib == Vec3 ? sizeof(GLfloat) * 3
		   : attrib == Vec4 ? sizeof(GLfloat) * 4
							: 0;
}
} // namespace AttributeType

struct Attribute {
	attribute_t type;
	uint32_t slot;

	Attribute(uint32_t slot, attribute_t type) : type(type), slot(slot) {
	}

	bool operator<(const Attribute &other) const {
		return slot < other.slot;
	}
};

class VertexArray {
  public:
	VertexArray() = default;
	~VertexArray();

	void New();
	void Delete();

	void Bind();
	void Unbind();

	void ResetAttributes();
	void SetAttribute(uint32_t slot, attribute_t type);
	void UploadAttributes();
	void DisableAttributes();

	inline uint32_t SizeInBytes() const { return m_attributeSizeInBytes; }

  private:
	uint32_t m_id = 0;

	uint32_t m_attributeSizeInBytes = 0;
	std::vector<Attribute> m_attributes;
};
} // namespace gfx

#endif // SW_GFX_VERTEX_ARRAY_HXX