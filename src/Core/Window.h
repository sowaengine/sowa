#ifndef WINDOW_H
#define WINDOW_H
#pragma once


#include <string>
#include "raylib.h"
#include "imgui-docking/imgui.h"


namespace Ease
{
    class Window
    {
    public:
        Window();
        ~Window();


        /**
         * @brief Video Size : Render resolution of scene, different than window size
         * 
         * @param width 
         */
        inline void SetVideoWidth(int width) {
            m_VideoWidth = width;
            
            m_Target.texture.width = width;
        }
        /**
         * @brief Video Size : Render resolution of scene, different than window size
         * 
         * @param height 
         */
        inline void SetVideoHeight(int height) {
            m_VideoHeight = height;
            m_Target.texture.height = height;
        }
        /**
         * @brief Video Size : Render resolution of scene, different than window size
         * 
         * @param width 
         * @param height 
         */
        inline void SetVideoSize(int width, int height) {
            m_VideoWidth = width;
            m_VideoHeight = height;
            
            m_Target.texture.width = width;
            m_Target.texture.height = height;
        }

        /**
         * @brief Video Size : Render resolution of scene, different than window size
         * 
         * @return int 
         */
        inline int GetVideoWidth() { return m_VideoWidth; }
        /**
         * @brief Video Size : Render resolution of scene, different than window size
         * 
         * @return int 
         */
        inline int GetVideoHeight() { return m_VideoHeight; }
        
        inline int GetWindowWidth() { return GetScreenWidth(); }
        inline int GetWindowHeight() { return GetScreenHeight(); }

        inline RenderTexture2D& GetFinalFramebufferTexture() { return m_Target; }

        void CreateWindow(int width, int height, const std::string& title);


        void Begin();
        void End();

        bool ShouldClose();
    private:


        RenderTexture2D m_Target;
        int m_VideoWidth = 1920, m_VideoHeight = 1080;
    };
} // namespace Ease

#endif