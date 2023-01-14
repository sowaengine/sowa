#include "nm_Window.hpp"
#include <stdexcept>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace nmGfx
{
    // Mouse Button
    static int MouseButton_ToGLFW(Window::MouseButton button);
    static Window::MouseButton MouseButton_ToEnum(int button);

    // Mouse Press State
    static int MousePressState_ToGLFW(Window::MousePressState state);
    static Window::MousePressState MousePressState_ToEnum(int state);

    // Modifier Keys
    static int ModifierKeys_ToGLFW(Window::ModifierKeys mods);
    static Window::ModifierKeys ModifierKeys_ToEnum(int mods);

    // Cursor Mode
    static int CursorMode_ToGLFW(Window::CursorMode mode);
    static Window::CursorMode CursorMode_ToEnum(int mode);

    // Key
    static int Key_ToGLFW(Window::Key key);
    static Window::Key Key_ToEnum(int key);
};

static void error_callback(int error, const char* description)
{
#ifdef NMGFX_PRINT_MESSAGES
    fprintf(stderr, "Error: %s\n", description);
#endif
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

namespace nmGfx
{
    Window::Window(int windowWidth, int windowHeight, int videoWidth, int videoHeight, const char* title, WindowFlags flags)
        : _videoWidth(videoWidth), _videoHeight(videoHeight)
    {
        if(!glfwInit())
            throw std::runtime_error("Failed to initialize glfw");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwSetErrorCallback(error_callback);

        _pWindow = glfwCreateWindow(windowWidth, windowHeight, title, NULL, NULL);
        if(!_pWindow)
            throw std::runtime_error("Failed to initialize window");
        
        glfwMakeContextCurrent(_pWindow);
        gladLoadGL();
        
        // glfwSetKeyCallback(_pWindow, key_callback);
        // glfwSetMouseButtonCallback(_pWindow, mouse_button_callback);
        // glfwSetFramebufferSizeCallback(_pWindow, framebuffer_size_callback);

        glfwSwapInterval(1);

        if(flags & WindowFlags::FULLSCREEN)
        {
            SetFullscreen(true);
        }
    }

    void Window::SetFullscreen(bool fullscreen)
    {
        if(fullscreen)
        {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            glfwGetWindowPos(_pWindow, &_fullscreen_oldX, &_fullscreen_oldY);
            glfwGetWindowSize(_pWindow, &_fullscreen_oldW, &_fullscreen_oldH);
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);

            _fullscreen_oldRefresh = mode->refreshRate;
            glfwSetWindowMonitor(_pWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            _fullscreen = true;
        }
        else
        {
            glfwSetWindowMonitor(_pWindow, nullptr, _fullscreen_oldX, _fullscreen_oldY, _fullscreen_oldW, _fullscreen_oldH, _fullscreen_oldRefresh);
            _fullscreen = false;
        }
    }

    bool Window::ShouldClose()
    {
        if(_pWindow == nullptr)
#ifdef NMGFX_PRINT_MESSAGES
            return printf("Can't call ShouldClose, Window was null\n") && 0;
#else
            return 0;
#endif
        return glfwWindowShouldClose(_pWindow);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers(_pWindow);
    }

    int Window::GetWindowMousePosX()
    {
        double x, y;
        glfwGetCursorPos(_pWindow, &x, &y);
        return (int)x;
    }

    int Window::GetWindowMousePosY()
    {
        double x, y;
        glfwGetCursorPos(_pWindow, &x, &y);
        return (int)y;
    }

    int Window::GetVideoMousePosX()
    {
        double posX = (double)GetWindowMousePosX();
        posX *= (double)GetVideoWidth() / (double)GetWindowWidth();
        return (int)posX;
    }

    int Window::GetVideoMousePosY()
    {
        double posY = (double)GetWindowMousePosY();
        posY *= (double)GetVideoHeight() / (double)GetWindowHeight();
        return (int)posY;
    }
    


    int Window::GetWindowHeight()
    {
        int w, h;
        glfwGetWindowSize(_pWindow, &w, &h);
        return h;
    }

    int Window::GetWindowWidth()
    {
        int w, h;
        glfwGetWindowSize(_pWindow, &w, &h);
        return w;
    }

    void Window::UnbindFramebuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, GetWindowWidth(), GetWindowHeight());
    }


    /* Input */
    bool Window::IsMouseButtonPressed(MouseButton button)
    {
        return glfwGetMouseButton(_pWindow, MouseButton_ToGLFW(button)) == GLFW_PRESS;
    }

    void Window::SetCursorMode(CursorMode mode)
    {
        glfwSetInputMode(_pWindow, GLFW_CURSOR, CursorMode_ToGLFW(mode));
    }

    bool Window::IsWindowHovered()
    {
        return glfwGetWindowAttrib(_pWindow, GLFW_HOVERED);
    }

    bool Window::IsKeyPressed(Key key)
    {
        return glfwGetKey(_pWindow, Key_ToGLFW(key));
    }



    // void Window::SetClearColor(float r, float g, float b, float a /*= 1.0f*/)
    // {
    //     glClearColor(r, g, b, a);
    // }

    // void Window::Clear(uint32_t bits /*= ClearBit::COLOR*/)
    // {
    //     GLbitfield flags = 0;
    //     if(bits & ClearBit::COLOR)
    //         flags |= GL_COLOR_BUFFER_BIT;
    //     if(bits & ClearBit::DEPTH)
    //         flags |= GL_DEPTH_BUFFER_BIT;
    //     
    //     glClear(flags);
    // };

    // convert enum to GLFW
    int MouseButton_ToGLFW(Window::MouseButton button)
    {
        return button == Window::MouseButton::LEFT ? GLFW_MOUSE_BUTTON_LEFT
            : button == Window::MouseButton::MIDDLE ? GLFW_MOUSE_BUTTON_MIDDLE
            : button == Window::MouseButton::RIGHT ? GLFW_MOUSE_BUTTON_RIGHT
            : GLFW_MOUSE_BUTTON_LEFT;
    }
    Window::MouseButton MouseButton_ToEnum(int button)
    {
        return button == GLFW_MOUSE_BUTTON_LEFT ? Window::MouseButton::LEFT
            : button == GLFW_MOUSE_BUTTON_MIDDLE ? Window::MouseButton::MIDDLE
            : button == GLFW_MOUSE_BUTTON_RIGHT ? Window::MouseButton::RIGHT
            : Window::MouseButton::LEFT;
    }

    int MousePressState_ToGLFW(Window::MousePressState state)
    {
        return state == Window::MousePressState::CLICKED ? GLFW_PRESS
            : state == Window::MousePressState::RELEASED ? GLFW_RELEASE
            : GLFW_RELEASE;
    }
    Window::MousePressState MousePressState_ToEnum(int state)
    {
        return state == GLFW_PRESS ? Window::MousePressState::CLICKED
            : state == GLFW_RELEASE ? Window::MousePressState::RELEASED
            : Window::MousePressState::RELEASED;
    }

    int ModifierKeys_ToGLFW(Window::ModifierKeys mods)
    {
        int value = 0;
		if (mods & Window::ModifierKeys::ModifierKeys_MOD_CONTROL)
			value |= GLFW_MOD_CONTROL;
		if (mods & Window::ModifierKeys::ModifierKeys_MOD_SHIFT)
			value |= GLFW_MOD_SHIFT;
		if (mods & Window::ModifierKeys::ModifierKeys_MOD_ALT)
			value |= GLFW_MOD_ALT;
		if (mods & Window::ModifierKeys::ModifierKeys_MOD_SUPER)
			value |= GLFW_MOD_SUPER;
		if (mods & Window::ModifierKeys::ModifierKeys_MOD_CAPS_LOCK)
			value |= GLFW_MOD_CAPS_LOCK;
		if (mods & Window::ModifierKeys::ModifierKeys_MOD_NUM_LOCK)
			value |= GLFW_MOD_NUM_LOCK;
        return value;
    }
    Window::ModifierKeys ModifierKeys_ToEnum(int mods)
    {
        int value = 0;
        if(mods & GLFW_MOD_CONTROL)
			value |= Window::ModifierKeys::ModifierKeys_MOD_CONTROL;
		if (mods & GLFW_MOD_SHIFT)
			value |= Window::ModifierKeys::ModifierKeys_MOD_SHIFT;
		if (mods & GLFW_MOD_ALT)
			value |= Window::ModifierKeys::ModifierKeys_MOD_ALT;
		if (mods & GLFW_MOD_SUPER)
			value |= Window::ModifierKeys::ModifierKeys_MOD_SUPER;
		if (mods & GLFW_MOD_CAPS_LOCK)
			value |= Window::ModifierKeys::ModifierKeys_MOD_CAPS_LOCK;
		if (mods & GLFW_MOD_NUM_LOCK)
			value |= Window::ModifierKeys::ModifierKeys_MOD_NUM_LOCK;
		return (Window::ModifierKeys)value;
    }

    int CursorMode_ToGLFW(Window::CursorMode mode)
    {
        return mode == Window::CursorMode::NORMAL ? GLFW_CURSOR_NORMAL
            : mode == Window::CursorMode::HIDDEN ? GLFW_CURSOR_HIDDEN
            : mode == Window::CursorMode::DISABLED ? GLFW_CURSOR_DISABLED
            : GLFW_CURSOR_NORMAL;
    }
    Window::CursorMode CursorMode_ToEnum(int mode)
    {
        return mode == GLFW_CURSOR_NORMAL ? Window::CursorMode::NORMAL
            : mode == GLFW_CURSOR_HIDDEN ? Window::CursorMode::HIDDEN
            : mode == GLFW_CURSOR_DISABLED ? Window::CursorMode::DISABLED
            : Window::CursorMode::NORMAL;
    }

    int Key_ToGLFW(Window::Key key)
    {
        return (int)key;
    }
    Window::Key Key_ToEnum(int key)
    {
        return Window::Key(key);
    }

} // namespace nmGfx
