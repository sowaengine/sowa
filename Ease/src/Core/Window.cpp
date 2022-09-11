#include "Core/Window.hpp"
#include "Ease.hpp"
#include <iostream>

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "ECS/Components/Common/Common.hpp"
#include "ECS/Entity/Entity.hpp"

namespace Ease {
Window::Window() {}
Window::~Window() {
}

bool Window::ShouldClose() {
	return _windowHandle->ShouldClose();
}

#ifndef EASE_EDITOR
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
} // namespace Ease