#ifndef TEXTURERESOURCE_H
#define TEXTURERESOURCE_H
#pragma once

#include <string>
#include "Resource.hpp"
#include <EaseGL.hpp>

class TextureResource : public Resource
{
	private:
		friend class Scene;
		
		std::string& m_Filepath;
		EaseGL::GLTexture texture;

		ResourceProperty prop_filepath;
		bool m_IsValid = false;

		void PropertiesUpdated() override;
	public:

		bool IsValid();

		EaseGL::GLTexture* GetTexturePtr() { return &texture; }

		TextureResource();
		~TextureResource();

		void SetFilepath(const std::string& path);
		const std::string& GetFilepath();
};
#endif