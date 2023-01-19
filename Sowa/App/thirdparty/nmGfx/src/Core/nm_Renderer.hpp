#ifndef __NM_GFX_RENDERER_HPP__
#define __NM_GFX_RENDERER_HPP__
#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "nm_Window.hpp"
#include "Core/GL/nm_Model.hpp"
#include "Core/nm_Shader.hpp"
#include "Core/GL/nm_Texture.hpp"
#include "Core/GL/nm_Framebuffer.hpp"
#include "Core/GL/nm_Material.hpp"

namespace nmGfx
{
    // pure virtual base class that has renderer methods
    class Renderer
    {
    public:
        void Init(int windowWidth, int windowHeight, int videoWidth, int videoHeight, const char* title, WindowFlags flags);
        
        Window& GetWindow() { return _window; }
        
        Renderer() = default;
        ~Renderer();

        /**
         * @brief Call before drawing 2d/3d layers
         * 
         */
        void ClearLayers();

        /**
         * @brief Begin 3D context, use shaders, calculate matrices
         * 
         * @param projectionMatrix
         * @param cameraTransform Camera transform (NOT view matrix)
         */
        void Begin3D(const glm::mat4 projectionMatrix, const glm::mat4 cameraTransform);
        void End3D();

        /**
         * @brief Returns id of given x, y coordinate in 3d space !Must be called within 3d context(Between Begin3D-End3D block)
         * 
         * @param x 
         * @param y 
         * @return int id
         */
        int Get3DPickID(int x, int y);

        /**
         * @brief Returns id of given x, y coordinate in 3d space. must be called outside of a context
         * 
         * @param x 
         * @param y 
         * @return int id
         */
        int Get3DPickIDSafe(int x, int y);


        /**
         * @brief Draws 3d layer on full screen
         * 
         */
        void Draw3DLayer();


        void DrawModel(const Model& model, const glm::mat4& transform, const Material& material, int drawID = 0);


        /**
         * @brief Begin 2D context, use shaders, calculate matrices (camera is on center)
         * 
         * @param cameraTransform Camera transform (NOT view matrix)
         * @param cameraCenter camera pivot point. {0.f, 0.f} -> top left, {1.f, 1.f} -> bottom right
         */
		void Begin2D(const glm::mat4 cameraTransform, const glm::vec2 &cameraCenter = {0.5f, 0.5f}, const glm::vec4& clearColor = {0.f, 0.f, 0.f, 0.f});
		void End2D();

		/**
		 * @brief Returns id of given x, y coordinate in 2d space !Must be called within 2d context(Between Begin2D-End2D block)
		 *
		 * @param x
		 * @param y
		 * @return int id
		 */
		int Get2DPickID(int x, int y);

        /**
         * @brief Returns id of given x, y coordinate in 2d space. must be called outside of a context
         * 
         * @param x 
         * @param y 
         * @return int id
         */
        int Get2DPickIDSafe(int x, int y);

        void DrawTexture(Texture* texture, const glm::mat4& transform, const glm::vec3& tint = glm::vec3(1.f), int drawID = 0);

        /**
         * @brief Draws 2d layer on full screen
         * 
         */
        void Draw2DLayer();


        struct Data3D
        {
            Framebuffer _gBuffer;

            glm::mat4 _projectionMatrix;
            glm::mat4 _viewMatrix;

            Shader _shader;

            Model _skyboxModel;
            Shader _skyboxShader;
            Texture* _skyboxTexture{nullptr};
        };
        struct Data2D
        {
            Model _model2d;
            Framebuffer _frameBuffer;

            glm::mat4 _projectionMatrix;
            glm::mat4 _viewMatrix;

            Shader _shader;
        };
        struct DataFullscreen
        {
            Model _model;
            Shader _shader;
        };

        DataFullscreen& GetDataFullscreen() { return _fullscreen; }
        Data3D& GetData3D() { return _data3d; }
        Data2D& GetData2D() { return _data2d; }
    private:
        Window _window{};

        Texture _whiteTexture;

        DataFullscreen _fullscreen;

        Data3D _data3d;
        Data2D _data2d;
    };
    
} // namespace nmGfx


#endif // __NM_GFX_RENDERER_HPP__
