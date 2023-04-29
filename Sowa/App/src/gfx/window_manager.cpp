#include "window_manager.hpp"

#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace sowa {
namespace gfx {

WindowManager::WindowManager() {
    if(!glfwInit()) {
        throw std::runtime_error("Failed to initialize glfw");
    }
}
WindowManager::~WindowManager() {
    for(auto& window : m_windowHandles) {
        window.DestroyWindow();
    }
    glfwTerminate();
}

Window& WindowManager::CreateWindow() {
    Window& window = m_windowHandles.emplace_back();
    window.m_ctx = m_ctx;
    return window;
}

} // namespace gfx
} // namespace sowa
