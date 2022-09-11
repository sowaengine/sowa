#ifndef _E_TEXTURE_HPP__
#define _E_TEXTURE_HPP__

#pragma once

#include <stdint.h>
#include <string>
#include "../Resource.hpp"
#include "Core/GL/Texture.hpp"
#include <memory>

typedef uint32_t ResourceID;

namespace Ease
{

class Texture : public BaseResource
{
public:
    Texture();
    ~Texture();

    
    const std::string& GetFilepath() { return m_Filepath; }
    inline int Width() { return _texture.GetWidth(); }
    inline int Height() { return _texture.GetHeight(); }
    inline int Channels() { return _texture.GetChannels(); }

    size_t TextureID() { return _texture.ID(); }
private:
    template<typename> friend class ResourceManager;
    template<typename> friend class ResourceLoaderImpl;
    friend class Renderer;
    
    nmGfx::Texture _texture{};
    std::string m_Filepath{""};
};

}
#endif