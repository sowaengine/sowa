#include "nm_VertexArray.hpp"
#include "glad/glad.h"
#include <algorithm>
#include "nm_Model.hpp"

namespace nmGfx
{
    void VertexArray::Create()
    {
        glGenVertexArrays(1, &_id);
    }

    VertexArray::~VertexArray()
    {
        glDeleteVertexArrays(1, &_id);
    }

    void VertexArray::Use() const
    {
        glBindVertexArray(_id);
    }

    // static
    void VertexArray::Unbind()
    {
        glBindVertexArray(0);
    }


    static uint32_t GetGLAttributeSize(AttributeType type)
    {
        return type == AttributeType::FLOAT ? sizeof(GLfloat)
            : type == AttributeType::VEC2  ? sizeof(GLfloat) * 2
            : type == AttributeType::VEC3  ? sizeof(GLfloat) * 3
            : 0; 
    }
    static uint32_t GetGLAttributeElementCount(AttributeType type)
    {
        return type == AttributeType::FLOAT ? 1
            : type == AttributeType::VEC2  ? 2
            : type == AttributeType::VEC3  ? 3
            : 0; 
    }
    static GLenum GetGLAttributeType(AttributeType type)
    {
        return type == AttributeType::FLOAT ? GL_FLOAT
            : type == AttributeType::VEC2  ? GL_FLOAT
            : type == AttributeType::VEC3  ? GL_FLOAT
            : GL_NONE; 
    }

    void VertexArray::ResetAttributes()
    {
        Use();

        for (const auto& attribute : _usedAttributes) {
            glDisableVertexAttribArray(attribute.slot);
        }
        _usedAttributes.clear();
    }

    void VertexArray::SetAttribute(uint32_t slot, AttributeType type) 
    {
        _usedAttributes.emplace_back(slot, type);
        _attributeSizeInBytes += GetGLAttributeSize(type);
    }

    void VertexArray::UploadAttributes()
    {
        Use();
        std::sort(_usedAttributes.begin(), _usedAttributes.end());
        
        uint64_t usedBytes = 0;
        for(const auto& attribute : _usedAttributes)
        {
            glVertexAttribPointer(
                attribute.slot,                               // slot
                GetGLAttributeElementCount(attribute.type),  // size (element count)
                GetGLAttributeType(attribute.type),         // type
                GL_FALSE,                                  // normalized
                _attributeSizeInBytes,                   // stride
                (const void*)usedBytes                   // pointer
                );
            glEnableVertexAttribArray(attribute.slot);

            usedBytes += GetGLAttributeSize(attribute.type);
        }
    }
} // namespace nmGfx
