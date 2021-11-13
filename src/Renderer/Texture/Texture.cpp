#include "Texture.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "glad/glad.h"
#include "Renderer/gl/glRenderer.hpp"
#include "stb/stb_image.h"

#include "yaml-cpp/yaml.h"

#include "Utils/File.hpp"


namespace Ease
{



static void loadTextureImportSettings(YAML::Node& data, TextureImportProperties& props)
{
   if(data["min_filter"])
      props.min_filter = data["min_filter"].as<uint32_t>(props.min_filter);
   if(data["mag_filter"])
      props.mag_filter = data["mag_filter"].as<uint32_t>(props.mag_filter);
   if(data["wrap_s"])
      props.wrap_s = data["wrap_s"].as<uint32_t>(props.wrap_s);
   if(data["wrap_t"])
      props.wrap_t = data["wrap_t"].as<uint32_t>(props.wrap_t);
}

static void saveDefaultTextureImportSettings(const std::string& filepath)
{
   /**
    *  / -> *
    *  . -> ?
    * file/path.png = file*path?png
    */
   std::string path = filepath;
   std::replace(path.begin(), path.end(), '/', '*');
   std::replace(path.begin(), path.end(), '.', '?');
   path = File::getPath_Head("res") + ".settings/" + path;
   std::cout << "Saved Default Texture Settings to: " << path << std::endl;

   TextureImportProperties props;
   
   YAML::Emitter out;

   out << YAML::Comment("Import settings");
   out << YAML::Newline;

   out << YAML::BeginMap;
      out << YAML::Key << "min_filter" << YAML::Value << props.min_filter;
      out << YAML::Key << "mag_filter" << YAML::Value << props.mag_filter;
      out << YAML::Key << "wrap_s" << YAML::Value << props.wrap_s;
      out << YAML::Key << "wrap_t" << YAML::Value << props.wrap_t;
   out << YAML::EndMap;

   
   std::ofstream fout(path);
   fout << out.c_str();
}


void Texture::recreateTexture()
{
   File::FileProperties fileProps = File::resolveFilepath(m_FilePath);
   m_FilePath = fileProps.fullPath;


   std::string path = fileProps.body;
   std::replace(path.begin(), path.end(), '/', '*');
   std::replace(path.begin(), path.end(), '.', '?');
   path = File::getPath_Head("res") + ".settings/" + path;
   
   
   std::ifstream stream(path);
   std::stringstream strStream;
   strStream << stream.rdbuf();

   YAML::Node data = YAML::Load(strStream.str());
   TextureImportProperties props;
   loadTextureImportSettings(data, props);

   if(data.size() == 0) {
      saveDefaultTextureImportSettings( std::filesystem::relative( fileProps.fullPath, File::getPath_Head("res")));
   } else
   {
      std::cout << "Loaded default settings from: " << path << std::endl;
   }

   /** 
    * !Clear old texture here
    */
   //m_FilePath = "res/textures/ship.png";
   auto* pixels = stbi_load(m_FilePath.c_str(), &m_Width, &m_Height, &m_Channels, 4);

   if(!pixels)
   {
      std::cout << "Failed loading texture: " << m_FilePath << std::endl;
      return;
   }  

   glGenTextures(1, &m_TextureID);
   glBindTexture(GL_TEXTURE_2D, m_TextureID);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, props.min_filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, props.mag_filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, props.wrap_s);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, props.wrap_t);


   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
   glBindTexture(GL_TEXTURE_2D, 0);

   stbi_image_free(pixels);
}

void Texture::bind(unsigned int slot/* = 0*/) const
{
   glActiveTexture(GL_TEXTURE0+slot);
   glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

   
} // namespace Ease
