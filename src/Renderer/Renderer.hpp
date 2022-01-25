/**
 * @file Renderer.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef RENDERER_H
#define RENDERER_H
#pragma once

#include <EaseGL.hpp>
#include <memory>

class Renderer  
{
	private:
		struct {
			std::unique_ptr<EaseGL::VertexArray> VertexArray;
			std::unique_ptr<EaseGL::GLBuffer> VertexBuffer;
			std::unique_ptr<EaseGL::GLBuffer> IndexBuffer;
			EaseGL::Shader ShaderProgram;
		} draw2d;

	public:

		static Renderer& get_singleton();
		void Init();

		Renderer();
		~Renderer();


		void Begin2D();
		/**
		 * @brief 
		 * 
		 * @param model Transform of object in model space
		 * @param cameraTransform Transform of camera
		 * @param proj Projection Matrix
		 * @param ptexture Pointer to GLTexture object for sprite
		 */
		void DrawSprite(const glm::mat4& model, const glm::mat4& cameraTransform, const glm::mat4& proj, EaseGL::GLTexture* ptexture);

		// returns the shader used in 2d sprite rendering
		EaseGL::Shader& Get2DShader() { return draw2d.ShaderProgram; }
};
#endif