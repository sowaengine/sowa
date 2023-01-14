#ifndef __NM_GFX_VERTEX_ARRAY_HPP__
#define __NM_GFX_VERTEX_ARRAY_HPP__

#include <stdint.h>
#include <vector>

namespace nmGfx
{
	enum class AttributeType;
	
    class VertexArray
    {
        public:
            VertexArray() = default;
            ~VertexArray();

            void Create();
            void Use() const;

            static void Unbind();

            void ResetAttributes();
			void SetAttribute(uint32_t slot, nmGfx::AttributeType type);
			void UploadAttributes();
        private:
            unsigned int _id = 0;

            // Atrributes
			struct Attribute
			{
				nmGfx::AttributeType type;
				uint32_t slot;

				bool operator<(const Attribute& other)
				{
					return slot < other.slot;
				}

				Attribute(uint32_t _slot, AttributeType _type)
					: slot(_slot), type(_type)
				{
				}
			};
			uint32_t _attributeSizeInBytes = 0;

			std::vector<Attribute> _usedAttributes;

			friend class Model;
    };
} // namespace nmGfx


#endif // __NM_GFX_GL_VERTEX_ARRAY_HPP__