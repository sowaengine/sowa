#ifndef SW_SCRIPT_SERVER_HPP_
#define SW_SCRIPT_SERVER_HPP_

#include "core/engine_context.hpp"
#include "stlpch.hpp"

namespace sowa {
    class Application;
}

namespace sowa {
class script_server {
  public:
	static script_server &Of(sowa::EngineContext *context);

  private:
	sowa::EngineContext &_Context;

  private:
	friend class sowa::Application;
	script_server(sowa::EngineContext &ctx);
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