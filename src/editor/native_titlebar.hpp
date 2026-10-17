#pragma once

struct GLFWwindow;

namespace sowa::editor {

// Native decorations stay under OS control. Other platforms retain their
// standard title bar and use the shared gradient in the editor's menu strip.
#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
void SetNativeTitlebarStyle(GLFWwindow *window, bool enabled);
#else
inline void SetNativeTitlebarStyle(GLFWwindow *, bool) {
}
#endif

} // namespace sowa::editor
