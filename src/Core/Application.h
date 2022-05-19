#ifndef APPLICATION_H
#define APPLICATION_H

#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <unordered_map>

#include "Window.h"
#include "Ease.hpp"
#include "ECS/Scene/Scene.h"
#include "Resource/NativeModule/NativeModule.h"

namespace Ease
{
    class Application
    {
    public:
        static Application& get_singleton()
        {
            static Application app;
            return app;
        }

        void Run();

        Window& GetWindow() { return m_Window; }


        

        enum class ModuleLoadResult
        {
            SUCCESS = 0,
            UNDEFINED_MODULE,
            OUTDATED_VERSION,
        };
        /**
         * @brief   loads author.moduleName.extension
         * 
         * @param author      author name
         * @param moduleName  module name without extension
         * @param minimumVersion minimum version allowed for module
         */
        ModuleLoadResult LoadModule(const std::string& author, const std::string& moduleName, int minimumVersion);

        /**
         * @brief Adds given module to application context
         * 
         * @param name alias of module
         * @param _module module resource to add
         */
        void AddModule(const std::string& name, std::shared_ptr<NativeModule> _module);



        void StartGame();
        void UpdateGame();


        void InitModules();
        void UpdateModules();
        
    private:
        Application();
        ~Application();

        Window m_Window;

        Scene m_GameScene;
        Scene* m_pCurrentScene;

        std::unordered_map<std::string, std::shared_ptr<NativeModule>> m_Modules;

        bool m_AppRunning = false;
    };
} // namespace Ease

#endif