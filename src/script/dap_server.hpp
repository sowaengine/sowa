#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"

namespace sowa {

struct DapServerEvent {
	enum class Kind {
		Connected,
		Message,
		Disconnected,
	};
	Kind kind = Kind::Message;
	// The JSON body, for Message.
	std::string message;
};

// Loopback-only DAP listener with one client; additional connections are closed.
// A dedicated libuv thread handles sockets even while the main thread is busy.
// The main thread takes framed messages with Take; Send may run on any thread.
class DapServer {
  public:
	DapServer();
	~DapServer();

	DapServer(const DapServer &) = delete;
	DapServer &operator=(const DapServer &) = delete;

	// Port 0 picks a free port (tests). Fails when the port is in use.
	bool Start(u16 port, std::string &error);
	// Flushes pending writes, closes the connection and joins the thread.
	void Stop();

	bool IsRunning() const;
	// The bound address and port, valid while running.
	std::string Address() const;
	u16 Port() const;
	bool IsConnected() const;

	// Everything received since the last call, in arrival order.
	std::vector<DapServerEvent> Take();

	// Queues one message for the client; dropped when none is connected.
	void Send(std::string json);

	// Internal: the libuv callbacks reach their state through it.
	struct Impl;

  private:
	Impl *_impl;
};

} // namespace sowa
