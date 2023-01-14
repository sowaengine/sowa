#ifndef __NM_GFX_MODEL_HPP__
#define __NM_GFX_MODEL_HPP__
#pragma once

#include <stdint.h>
#include <vector>
#include <unordered_map>

#include "Core/GL/nm_Buffer.hpp"
#include "nm_VertexArray.hpp"

namespace nmGfx
{
    enum class AttributeType
	{
		NONE = 0,
		FLOAT,
		VEC2,
		VEC3,
	};

    class Model
    {
        public:
            void LoadFromFile(const char* path);




            void Create();

            void SetModelData(const std::vector<float>& data);
            void SetIndexData(const std::vector<uint32_t>& data);
            void ResetAttributes();
            void SetAttribute(uint32_t slot, AttributeType type);
            void UploadAttributes();
            
            void Draw() const;
        protected:
            friend class Renderer;

            VertexArray _vao2d;
            Buffer _vbo2d;
            Buffer _ebo2d;

            std::unordered_map<uint32_t, Buffer> _attributes;
            uint32_t _indexCount = 0;
            uint32_t _vbodata_size = 0;
    };
} // namespace nmGfx



#endif // __NM_GFX_RENDERER_HPP__