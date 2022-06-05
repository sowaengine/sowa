#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#pragma once

#include <string>

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
        } _application;

        bool LoadProject(const char* path);
        bool SaveProject(const char* path);

        std::string projectpath{""};
    private:
        ProjectSettings();
        ~ProjectSettings();

    };
} // namespace Ease

#endif