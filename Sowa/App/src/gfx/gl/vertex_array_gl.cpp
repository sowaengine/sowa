#include "vertex_array_gl.hpp"

#include <algorithm>

#include "gfx/gl/glfuncs.hpp"
#include "glad/glad.h"


namespace sowa
{
namespace gfx
{
    static uint32_t GetGLAttributeSize(GLAttributeType type)
    {
        return type == GLAttributeType::Float ? sizeof(GLfloat)
            : type == GLAttributeType::Vec2  ? sizeof(GLfloat) * 2
            : type == GLAttributeType::Vec3  ? sizeof(GLfloat) * 3
            : type == GLAttributeType::Vec4  ? sizeof(GLfloat) * 4
            : 0; 
    }
    static uint32_t GetGLAttributeElementCount(GLAttributeType type)
    {
        return type == GLAttributeType::Float ? 1
            : type == GLAttributeType::Vec2  ? 2
            : type == GLAttributeType::Vec3  ? 3
            : type == GLAttributeType::Vec4  ? 4
            : 0; 
    }


    GLVertexArray::~GLVertexArray() {
        Delete();
    }

	void GLVertexArray::New() {
        if(m_id != 0) {
            Delete();
        }

        GL().genVertexArray(m_id);
    }

    void GLVertexArray::Delete() {
        if(m_id != 0) {
            GL().deleteVertexArray(m_id);
        }

        m_id = 0;
    }

	void GLVertexArray::Bind() {
        GL().bindVertexArray(m_id);
    }
	void GLVertexArray::Unbind() {
        GL().bindVertexArray(0);
    }

	void GLVertexArray::ResetAttributes() {
        Bind();

        for(const GLAttribute& attribute : m_usedAttributes) {
            GL().disableVertexAttribArray(attribute.slot);
        }

        m_attributeSizeInBytes = 0;
        m_usedAttributes.clear();
    }
	void GLVertexArray::SetAttribute(uint32_t slot, GLAttributeType type) {
        m_usedAttributes.emplace_back(slot, type);
        m_attributeSizeInBytes += GetGLAttributeSize(type);
    }
	void GLVertexArray::UploadAttributes() {
        Bind();
        std::sort(m_usedAttributes.begin(), m_usedAttributes.end());

        uint64_t usedBytes = 0;
        for(const GLAttribute& attribute : m_usedAttributes) {
            GL().vertexAttribPointer(
                attribute.slot,
                GetGLAttributeElementCount(attribute.type),
                attribute.type,
                false,
                m_attributeSizeInBytes,
                usedBytes
            );
            GL().enableVertexAttribArray(attribute.slot);

            usedBytes += GetGLAttributeSize(attribute.type);
        }
    }
} // namespace gfx
} // namespace sowa
