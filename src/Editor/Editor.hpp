/**
 * @file Editor.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef EDITOR_H
#define EDITOR_H
#pragma once

#include <glad/glad.h>
#include <string>
#include "Scene/Scene.hpp"
#include "Scene/Entity.hpp"
#include <EaseGL.hpp>

#include "Resource/Resource.hpp"
#include <filesystem>


struct EditorData
{
	GLuint finalFramebufferTextureID;
	Scene* currentScene = nullptr;


	Entity selectedEntity;
	Resource* selectedResource = nullptr;


	float viewportDragMouseDeltaX = 0.f;
	float viewportDragMouseDeltaY = 0.f;

	bool start_stop_pressed = false; // start or stop button has just pressed
	bool useInGameCamera = false;
};

enum class Log
{
	Endl = 1,
};
class EditorOstream
{
	public:
		friend EditorOstream& operator<<(EditorOstream& os, Log command);
		friend EditorOstream& operator<<(EditorOstream& os, const std::string& str);
		friend EditorOstream& operator<<(EditorOstream& os, int num);
		

		EditorOstream() = default;
};

struct ClickData;
class Editor  
{
	private:
		Editor();

		void UpdateViewport();
		void UpdateScenePanel();
		void UpdateInspector();
		void UpdateAssets();

		// Helper functions for UpdateAssets
		ClickData DrawFolder(const std::string& name);
		ClickData DrawFile(const std::string& name);
		void DrawItem(std::filesystem::path& currentPath);

		friend class Application;
		EditorData* m_EditorData = nullptr;


		EaseGL::GLTexture m_PlayTexture;
		EaseGL::GLTexture m_StopTexture;
		EaseGL::GLTexture m_EntityIconTexture;

		struct
		{
			EaseGL::GLTexture m_Directory;
			EaseGL::GLTexture m_UnknownFile;
			EaseGL::GLTexture m_Image;
			EaseGL::GLTexture m_Text;
			EaseGL::GLTexture m_Lua;
			EaseGL::GLTexture m_Font;

		} AssetsPanelTextures;
	public:
		static Editor& get_singleton();

		void Update();

		~Editor();

		void SetStyle(int index = 0);
		
		
		EditorOstream Log;
		std::string m_ConsoleText = "";

		void SetSelectedResource(Resource* resource) { m_EditorData->selectedResource = resource; }
};

#include "EaseEngine.hpp"
#define EASE_LOG_VERSION() Editor::get_singleton().Log << "Ease Engine v" << EASE_VERSION_MAJOR << "." << EASE_VERSION_MINOR << "." << EASE_VERSION_PATCH << " by Lexographics\n" << __DATE__ << Log::Endl;




#endif