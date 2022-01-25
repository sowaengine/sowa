/**
 * @file Window.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef WINDOW_H
#define WINDOW_H
#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>

class Window  
{
	private:
		/**
		 * @brief Create a Window object
		 * Only Application class can create
		 */
		void CreateWindow(int width, int height, std::string title);
		
		friend class Application;
	
	private:
		// Window Properties
		int m_Width;
		int m_Height;
		std::string m_Title;
	public:
		// Sets Width of the window, DOES NOT update window, Call UpdateWindowSize To update it
		void SetWidth(int width) { m_Width = width; }
		// Sets Height of the window, DOES NOT update window, Call UpdateWindowSize To update it
		void SetHeight(int height) {m_Height = height;}
		// Sets Width and Height of the window, DOES NOT update window, Call UpdateWindowSize To update it
		void SetSize(int width, int height) { m_Width = width; m_Height = height; }
		// Updates window size with member variables m_Width, m_Height
		void UpdateWindowSize() const;

		// Sets Title of the window, DOES NOT update window, Call UpdateWindowTitle To update it
		void SetTitle(const std::string& title) { m_Title = title; }
		// Updates window title with member variable m_Title
		void UpdateWindowTitle() const;

		// Getter for the window width
		inline int Width() { return m_Width; }
		// Getter for the window height
		inline int Height() { return m_Height; }
		// Getter for the window title
		inline const std::string& Title() { return m_Title; }
		
	public:

		static Window& get_singleton();

		Window();
		~Window();

		operator GLFWwindow*() { return m_WindowPointer; }


		/**
		 * @brief OpenGL and GLFW call abstractions
		 **/
		// glClearColor
		void SetClearColor(float r, float g, float b, float a) const;

		// glfwPollEvents
		void PollEvents() const;

		// glClear (can be used like DEPTH_BIT | COLOR_BIT)
		void Clear(int bits) const;

		// glfwSwapBuffers
		void SwapBuffers() const;

		// glfwWindowShouldClose
		bool WindowShouldClose() const;

	private:
		GLFWwindow* m_WindowPointer = nullptr;
};
#endif