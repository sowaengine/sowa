/**
 * @file Application.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef APPLICATION_H
#define APPLICATION_H
#pragma once

#include <vector>
#include <memory>
#include "Resource/Project.hpp"

#include "Scene/Scene.hpp"

#ifdef EASE_EDITOR
	#include "Editor/Editor.hpp"
#endif


#include <EaseGL.hpp>
/**
 * @brief Can only have one instance
 */
class Application  
{
	private:
		Application();

		void CreateFinalFramebuffer();
		EaseGL::Shader m_FinalFramebufferShader;
		EaseGL::Framebuffer m_FinalFramebuffer;
		EaseGL::Framebuffer m_EditorFramebuffer; // framebuffer that is used on editor 'Viewport' tab

		EditorData m_EditorData;

		Scene* m_CurrentScene = nullptr;


		std::unique_ptr<EaseGL::VertexArray> m_FramebufferVertexArray;
#ifndef EASE_EDITOR
		bool m_Running = true; // game automatically starts on non-editor builds
#else
		bool m_Running = false;
#endif

	public:

		void Run();
		void Start();
		void Stop();

		bool IsRunning() { return m_Running; }
		
		static Application& get_singleton();

		~Application();
};
#endif