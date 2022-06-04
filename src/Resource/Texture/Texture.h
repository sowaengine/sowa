#ifndef TEXTURE_H
#define TEXTURE_H

#pragma once

#include <stdint.h>
#include <string>
#include "raylib.h"
#include "../Resource.h"

typedef uint32_t ResourceID;

namespace Ease
{

class Texture : public BaseResource
{
public:
    Texture();
    ~Texture();

    void SetTexture(const Texture2D& tex) { m_Texture = tex; }
    Texture2D& GetTexture() { return m_Texture; }
    const std::string& GetFilepath() { return m_Filepath; }
private:
    template<typename> friend class ResourceManager;

    Texture2D m_Texture;
    std::string m_Filepath{""};
};

}
#endif