#ifndef _E_APPLICATION_HPP__
#define _E_APPLICATION_HPP__

#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <unordered_map>

#include "Window.hpp"
#include "Ease.hpp"
#include "ECS/Scene/Scene.hpp"
#include "Resource/NativeModule/NativeModule.hpp"

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

        void Log(const std::string& message); // Logs given string to console (And Editor Console on editor builds)
        

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
        std::shared_ptr<NativeModule> GetModule(const std::string& name);

        void AddNativeBehaviour(const std::string& name, NativeBehaviourFactory* behaviour);
        Ease::NativeBehaviourFactory* GetFactory(const std::string& name);


        void StartGame();
        void UpdateGame();
        void StopGame();
        Ease::Scene* GetCurrentScene() { return m_pCurrentScene; }


        void InitModules();
        void UpdateModules();
        void Modules_OnImGuiRender();
        
    private:
        Application();
        ~Application();

        Window m_Window;

        Scene m_GameScene;
        Scene* m_pCurrentScene;

        std::unordered_map<std::string, std::shared_ptr<NativeModule>> m_Modules;
        std::unordered_map<std::string, NativeBehaviourFactory*> m_NativeBehaviours;

        bool m_AppRunning = false;
    };
} // namespace Ease

#endif