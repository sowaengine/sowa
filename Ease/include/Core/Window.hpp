#ifndef _E_WINDOW_HPP__
#define _E_WINDOW_HPP__
#pragma once


#include <string>
#include "raylib.h"


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
            m_MousePickingTarget.texture.width = width;
        }
        /**
         * @brief Video Size : Render resolution of scene, different than window size
         * 
         * @param height 
         */
        inline void SetVideoHeight(int height) {
            m_VideoHeight = height;

            m_Target.texture.height = height;
            m_MousePickingTarget.texture.height = height;
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
            m_MousePickingTarget.texture.width = width;
            m_MousePickingTarget.texture.height = height;
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
        inline RenderTexture2D& GetMousePickingTexture() { return m_MousePickingTarget; }

        void CreateWindow(int width, int height, const std::string& title);


        void Begin();
        void End();
        void BeginPicking();
        void EndPicking();

        bool ShouldClose();

        int GetBlackbarWidth();
        int GetBlackbarHeight();

        #ifdef EASE_EDITOR
    public:
        void Editor_SetBlackbarWidth(int width) { m_Editor_BlackbarWidth = width; }
        void Editor_SetBlackbarHeight(int height) { m_Editor_BlackbarHeight = height; }

        void Editor_SetWindowWidth(int width) { m_Editor_WindowWidth = width; }
        void Editor_SetWindowHeight(int height) { m_Editor_WindowHeight = height; }

        inline int Editor_GetWindowWidth() { return m_Editor_WindowWidth; }
        inline int Editor_GetWindowHeight() { return m_Editor_WindowHeight; }

        void Editor_SetWindowPos(int x, int y) { m_Editor_WindowPosX = x; m_Editor_WindowPosY = y; }
        int Editor_GetWindowPosX() { return m_Editor_WindowPosX; }
        int Editor_GetWindowPosY() { return m_Editor_WindowPosY; }
    private:
        int m_Editor_BlackbarWidth;
        int m_Editor_BlackbarHeight;

        int m_Editor_WindowWidth;
        int m_Editor_WindowHeight;

        int m_Editor_WindowPosX;
        int m_Editor_WindowPosY;
        #endif
    private:
        RenderTexture2D m_MousePickingTarget;
        RenderTexture2D m_Target;
        int m_VideoWidth = 1920, m_VideoHeight = 1080;
    };
} // namespace Ease

#endif