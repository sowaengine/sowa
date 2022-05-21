#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#pragma once

#include <unordered_map>
#include <memory>
#include "Resource/Texture/Texture.h"
#include "Resource/NativeModule/NativeModule.h"
#include "Resource/EditorTheme/EditorTheme.h"

typedef uint32_t ResourceID;

namespace Ease
{
    template<typename T>
    class ResourceManager
    {
    public:
        ResourceManager() {}
        ~ResourceManager() {}

        static ResourceManager<T>& GetLoader();
        

        std::shared_ptr<T> LoadResource(const char* path);
        bool SaveResource(const char* path, std::shared_ptr<T> resource);
        bool DeleteResource(ResourceID id)
        {
            std::shared_ptr<T> res = m_Resources[id];
            m_Resources[id] = nullptr;
            delete res;
            return true;
        }

        std::shared_ptr<T>& GetResource(ResourceID id)
        {
            return m_Resources[id];
        }
    private:
        std::unordered_map<ResourceID, std::shared_ptr<T>> m_Resources;
    };


    template<>
    std::shared_ptr<Ease::Texture> ResourceManager<Ease::Texture>::LoadResource(const char* path);
    template<>
    ResourceManager<Ease::Texture>& ResourceManager<Ease::Texture>::GetLoader();
    
    template<>
    std::shared_ptr<Ease::NativeModule> ResourceManager<Ease::NativeModule>::LoadResource(const char* path);
    template<>
    ResourceManager<Ease::NativeModule>& ResourceManager<Ease::NativeModule>::GetLoader();

    template<>
    std::shared_ptr<Ease::EditorTheme> ResourceManager<Ease::EditorTheme>::LoadResource(const char* path);
    template<>
    bool ResourceManager<Ease::EditorTheme>::SaveResource(const char* path, std::shared_ptr<Ease::EditorTheme> resource);
    template<>
    ResourceManager<Ease::EditorTheme>& ResourceManager<Ease::EditorTheme>::GetLoader();
} // namespace Ease

#endif