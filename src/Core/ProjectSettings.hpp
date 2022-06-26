#ifndef _E_PROJECTSETTINGS_HPP__
#define _E_PROJECTSETTINGS_HPP__

#pragma once

#include <string>
#include <filesystem>

namespace Ease
{
    class ProjectSettings
    {
    public:
        static ProjectSettings& get_singleton()
        {
            static ProjectSettings settings;
            return settings;
        }

        struct {
            int WindowWidth = 1280;
            int WindowHeight = 720;

            int VideoWidth = 1920;
            int VideoHeight = 1280;

            bool Fullscreen = false;
        } _window;
        struct {
            std::string Name = "Ease Engine";
            std::string Description = "";
            std::string MainScene = "";
        } _application;

        bool LoadProject(const char* path);
        bool SaveProject();

        std::filesystem::path projectpath{""};
    private:
        ProjectSettings();
        ~ProjectSettings();

    };
} // namespace Ease

#endif