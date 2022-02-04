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
#include <unordered_map>

class Renderer  
{
	private:
		struct {
			std::unique_ptr<EaseGL::VertexArray> VertexArray;
			std::unique_ptr<EaseGL::GLBuffer> VertexBuffer;
			std::unique_ptr<EaseGL::GLBuffer> IndexBuffer;
			EaseGL::Shader ShaderProgram;
		} draw2d;


		void SetupBatch2D();
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


		
		struct Batch2D {
			struct Vertex
			{
				glm::vec2 position;
				glm::vec2 texCoords;
				float texID;
			};

			std::unique_ptr<EaseGL::VertexArray> VertexArray;

			std::unique_ptr<EaseGL::GLBuffer> VertexBufferObject;
			std::unique_ptr<EaseGL::GLBuffer> IndexBuffer;

			EaseGL::Shader ShaderProgram;
			
			// First -> Texture ID,      Second -> Its bound slot (-1 if not bound)
			std::unordered_map<uint32_t, int> UsedTextures;
			int currentTextureSlot = 0;

			
			uint32_t vertexCount;
			Vertex* vertexBuffer;
			Vertex* vertexCurrent;
			uint32_t quadCount;
			

			int max_texture_units = 8;

		} batch2d;
		void BeginBatch2D();
		void DrawBatch2D(const glm::mat4& model, const glm::mat4& cameraTransform, const glm::mat4& proj, EaseGL::GLTexture* ptexture);
		void EndBatch2D();
};
#endif