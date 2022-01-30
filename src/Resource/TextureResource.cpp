#include "TextureResource.hpp"
#include <EaseGL.hpp>
	
TextureResource::TextureResource()
   : m_Filepath(prop_filepath.dataStr)
{
	prop_filepath.name = "Texture";
   prop_filepath.type = PropertyType::TEXTURE;
   
   properties.push_back(&prop_filepath);


   texture = EaseGL::Texture2D::New();
}

TextureResource::~TextureResource()
{
}


void TextureResource::SetFilepath(const std::string& path) 
{
   m_Filepath = path;
}

const std::string& TextureResource::GetFilepath() 
{
   return m_Filepath;
}


void TextureResource::PropertiesUpdated() 
{
   SetFilepath(prop_filepath.dataStr);
   texture.LoadTexture(m_Filepath.c_str());
   m_IsValid = true;
}

bool TextureResource::IsValid() 
{
   return m_IsValid;
}
