#ifndef SW_GFX_WINDOW_MANAGER_HPP_
#define SW_GFX_WINDOW_MANAGER_HPP_

#include <vector>

#define GLFW_INCLUDE_NONE
#include "window.hpp"

#include "core/engine_context.hpp"

namespace sowa {
class Application;
namespace gfx {

class WindowManager {
  public:
    Window& CreateWindow();

	  WindowManager();
    ~WindowManager();
  private:
	friend class sowa::Application;
    sowa::EngineContext* m_ctx;
    std::vector<Window> m_windowHandles{};
};

} // namespace gfx
} // namespace sowa

#endif // SW_GFX_WINDOW_MANAGER_HPP_