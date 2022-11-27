#include "Core/Window.hpp"
#include "Sowa.hpp"

#include "GLFW/glfw3.h"
#include <iostream>

#include "Resource/Texture/Texture.hpp"

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "ECS/Components/Common/Common.hpp"
#include "ECS/Entity/Entity.hpp"
#include "nmGfx/src/Core/nm_Window.hpp"

namespace Sowa {
Window::Window() {}
Window::~Window() {
}

int Window::GetVideoWidth() { return _windowHandle->GetVideoWidth(); }
int Window::GetVideoHeight() { return _windowHandle->GetVideoHeight(); }

int Window::GetWindowWidth() { return _windowHandle->GetWindowWidth(); }
int Window::GetWindowHeight() { return _windowHandle->GetWindowHeight(); }

bool Window::ShouldClose() {
	return _windowHandle->ShouldClose();
}

bool Window::SetWindowIcon(Reference<ImageTexture> icon) {
	if (icon == nullptr) {
		Debug::Error("Invalid icon on Window::SetWindowIcon");
		return false;
	}

	if (icon->Channels() != 4) {
		Debug::Error("Window::SetWindowIcon : invalid format, texture must have 4 channels. (given texture has {})", icon->Channels());
		return false;
	}

	GLFWimage image[1];
	image[0].width = icon->Width();
	image[0].height = icon->Height();
	image[0].pixels = icon->Pixels();

	if (image[0].pixels != nullptr) {
		glfwSetWindowIcon(_windowHandle->GetGLFWwindow(), 1, image);
		return true;
	} else {
		Debug::Error("Invalid icon on Window::SetWindowIcon");
		return false;
	}
}

#ifndef SW_EDITOR
int Window::GetBlackbarWidth() {
	float scale = MIN((float)GetWindowWidth() / GetVideoWidth(), (float)GetWindowHeight() / GetVideoHeight());
	return (GetWindowWidth() - ((float)GetVideoWidth() * scale)) * 0.5f;
}
int Window::GetBlackbarHeight() {
	float scale = MIN((float)GetWindowWidth() / GetVideoWidth(), (float)GetWindowHeight() / GetVideoHeight());
	return (GetWindowHeight() - ((float)GetVideoHeight() * scale)) * 0.5f;
}
#else
int Window::GetBlackbarWidth() {
	return m_Editor_BlackbarWidth;
}
int Window::GetBlackbarHeight() {
	return m_Editor_BlackbarHeight;
}
#endif
} // namespace Sowa