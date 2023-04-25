#ifndef SW_SCRIPT_SERVER_HPP_
#define SW_SCRIPT_SERVER_HPP_

#include "core/engine_context.hpp"
#include "stlpch.hpp"

namespace Sowa {
    class Application;
}

namespace sowa {
class script_server {
  public:
	static script_server &Of(Sowa::EngineContext *context);

  private:
	Sowa::EngineContext &_Context;

  private:
	friend class Sowa::Application;
	script_server(Sowa::EngineContext &ctx);
	~script_server();

    void init();

  public:
	script_server(const script_server &) = delete;
	script_server(const script_server &&) = delete;
	script_server &operator=(const script_server &) = delete;
	script_server &operator=(const script_server &&) = delete;
};
} // namespace sowa

#endif // SW_SCRIPT_SERVER_HPP_