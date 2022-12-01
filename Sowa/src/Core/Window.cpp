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
/* -- Input -- */
Vec2 Window::GetMousePosition() {
	double x, y;
	glfwGetCursorPos(_windowHandle->GetGLFWwindow(), &x, &y);

	// x -= GetWindowWidth() * 0.5f;
	// y -= GetWindowHeight() * 0.5f;

#ifndef SW_EDITOR
	x -= GetBlackbarWidth();
	y -= GetBlackbarHeight();
#else
	Vec2 scale;
	scale.x = (float)GetWindowWidth() / (float)Editor_GetWindowWidth();
	scale.y = (float)GetWindowHeight() / (float)Editor_GetWindowHeight();

	x -= Editor_GetWindowPosX();
	y -= Editor_GetWindowPosY();
	x *= scale.x;
	y *= scale.y;
#endif

	return Vec2(x, y);
}

Vec2 Window::GetGameMousePosition() {

	Vec2 scale;
#ifndef SW_EDITOR
	scale.x = (float)GetVideoWidth() / ((float)GetWindowWidth() - (GetBlackbarWidth() * 2));
	scale.y = (float)GetVideoHeight() / ((float)GetWindowHeight() - (GetBlackbarHeight() * 2));
#else

	scale.x = (float)GetVideoWidth() / (float)GetWindowWidth();
	scale.y = (float)GetVideoHeight() / (float)GetWindowHeight();
#endif

	Vec2 mousePos = GetMousePosition();
	mousePos.x *= scale.x;
	mousePos.y *= scale.y;

	return mousePos;
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