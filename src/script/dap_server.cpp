#include "script/dap_server.hpp"

#include <atomic>
#include <mutex>
#include <thread>
#include <utility>

#include <uv.h>

#include "script/dap_framing.hpp"

namespace sowa {

struct DapServer::Impl {
	uv_loop_t loop{};
	uv_tcp_t server{};
	uv_async_t wake{};
	// Loop-thread only.
	uv_tcp_t *client = nullptr;
	DapFraming framing;

	std::thread thread;
	std::mutex mutex;
	std::vector<DapServerEvent> inbox;
	std::vector<std::string> outbox;
	bool stopping = false;

	std::atomic<bool> running{false};
	std::atomic<bool> connected{false};
	u16 port = 0;
	std::string address;

	void push(DapServerEvent event) {
		std::lock_guard lock(mutex);
		inbox.push_back(std::move(event));
	}

	void closeClient() {
		if (client == nullptr) {
			return;
		}
		uv_read_stop(reinterpret_cast<uv_stream_t *>(client));
		uv_close(reinterpret_cast<uv_handle_t *>(client),
			[](uv_handle_t *handle) { delete reinterpret_cast<uv_tcp_t *>(handle); });
		client = nullptr;
		framing = {};
		connected = false;
		push({DapServerEvent::Kind::Disconnected, {}});
	}
};

namespace {

struct WriteRequest {
	uv_write_t request{};
	std::string data;
};

DapServer::Impl *implOf(uv_handle_t *handle) {
	return static_cast<DapServer::Impl *>(handle->data);
}

void onAlloc(uv_handle_t *, size_t suggested, uv_buf_t *buf) {
	buf->base = new char[suggested];
	buf->len = static_cast<decltype(buf->len)>(suggested);
}

void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
	DapServer::Impl *impl = implOf(reinterpret_cast<uv_handle_t *>(stream));
	if (nread > 0) {
		impl->framing.Feed(std::string_view(buf->base, static_cast<std::size_t>(nread)));
		while (std::optional<std::string> message = impl->framing.Next()) {
			impl->push({DapServerEvent::Kind::Message, std::move(*message)});
		}
		if (impl->framing.Failed()) {
			impl->closeClient();
		}
	} else if (nread < 0) {
		impl->closeClient();
	}
	delete[] buf->base;
}

void onConnection(uv_stream_t *server, int status) {
	DapServer::Impl *impl = implOf(reinterpret_cast<uv_handle_t *>(server));
	if (status < 0) {
		return;
	}
	auto *client = new uv_tcp_t;
	uv_tcp_init(&impl->loop, client);
	client->data = impl;
	if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client)) != 0 ||
		impl->client != nullptr) {
		// One debugger at a time: a second connection is closed at once.
		uv_close(reinterpret_cast<uv_handle_t *>(client),
			[](uv_handle_t *handle) { delete reinterpret_cast<uv_tcp_t *>(handle); });
		return;
	}
	impl->client = client;
	impl->connected = true;
	impl->push({DapServerEvent::Kind::Connected, {}});
	uv_read_start(reinterpret_cast<uv_stream_t *>(client), onAlloc, onRead);
}

void onWake(uv_async_t *async) {
	DapServer::Impl *impl = implOf(reinterpret_cast<uv_handle_t *>(async));
	std::vector<std::string> outgoing;
	bool stopping = false;
	{
		std::lock_guard lock(impl->mutex);
		outgoing.swap(impl->outbox);
		stopping = impl->stopping;
	}
	for (std::string &data : outgoing) {
		if (impl->client == nullptr) {
			break;
		}
		auto *write = new WriteRequest;
		write->data = std::move(data);
		uv_buf_t buf = uv_buf_init(write->data.data(), static_cast<unsigned>(write->data.size()));
		const int result =
			uv_write(&write->request, reinterpret_cast<uv_stream_t *>(impl->client), &buf, 1,
				[](uv_write_t *request, int) { delete reinterpret_cast<WriteRequest *>(request); });
		if (result != 0) {
			delete write;
		}
	}
	if (!stopping) {
		return;
	}
	if (impl->client != nullptr) {
		// Shutdown waits for the queued writes (a final "terminated" event)
		// before the connection closes.
		uv_tcp_t *client = impl->client;
		impl->client = nullptr;
		impl->connected = false;
		uv_read_stop(reinterpret_cast<uv_stream_t *>(client));
		auto *shutdown = new uv_shutdown_t;
		if (uv_shutdown(shutdown, reinterpret_cast<uv_stream_t *>(client),
				[](uv_shutdown_t *request, int) {
					uv_close(reinterpret_cast<uv_handle_t *>(request->handle),
						[](uv_handle_t *handle) { delete reinterpret_cast<uv_tcp_t *>(handle); });
					delete request;
				}) != 0) {
			delete shutdown;
			uv_close(reinterpret_cast<uv_handle_t *>(client),
				[](uv_handle_t *handle) { delete reinterpret_cast<uv_tcp_t *>(handle); });
		}
	}
	uv_close(reinterpret_cast<uv_handle_t *>(&impl->server), nullptr);
	uv_close(reinterpret_cast<uv_handle_t *>(&impl->wake), nullptr);
}

} // namespace

DapServer::DapServer() : _impl(new Impl) {
}

DapServer::~DapServer() {
	Stop();
	delete _impl;
}

bool DapServer::Start(u16 port, std::string &error) {
	if (_impl->running) {
		return true;
	}
	Impl &impl = *_impl;
	uv_loop_init(&impl.loop);
	uv_tcp_init(&impl.loop, &impl.server);
	impl.server.data = &impl;

	sockaddr_in address{};
	uv_ip4_addr("127.0.0.1", port, &address);
	int result = uv_tcp_bind(&impl.server, reinterpret_cast<const sockaddr *>(&address), 0);
	if (result == 0) {
		result = uv_listen(reinterpret_cast<uv_stream_t *>(&impl.server), 1, onConnection);
	}
	if (result != 0) {
		error = uv_strerror(result);
		uv_close(reinterpret_cast<uv_handle_t *>(&impl.server), nullptr);
		uv_run(&impl.loop, UV_RUN_DEFAULT);
		uv_loop_close(&impl.loop);
		return false;
	}

	sockaddr_storage bound{};
	int length = sizeof(bound);
	uv_tcp_getsockname(&impl.server, reinterpret_cast<sockaddr *>(&bound), &length);
	impl.port = ntohs(reinterpret_cast<const sockaddr_in *>(&bound)->sin_port);
	char name[64] = {};
	uv_ip4_name(reinterpret_cast<const sockaddr_in *>(&bound), name, sizeof(name));
	impl.address = name;

	uv_async_init(&impl.loop, &impl.wake, onWake);
	impl.wake.data = &impl;
	{
		std::lock_guard lock(impl.mutex);
		impl.stopping = false;
		impl.inbox.clear();
		impl.outbox.clear();
	}
	impl.running = true;
	impl.thread = std::thread([&impl] { uv_run(&impl.loop, UV_RUN_DEFAULT); });
	return true;
}

void DapServer::Stop() {
	if (!_impl->running) {
		return;
	}
	{
		std::lock_guard lock(_impl->mutex);
		_impl->stopping = true;
	}
	uv_async_send(&_impl->wake);
	_impl->thread.join();
	uv_loop_close(&_impl->loop);
	_impl->running = false;
	_impl->connected = false;
}

bool DapServer::IsRunning() const {
	return _impl->running;
}

std::string DapServer::Address() const {
	return _impl->address;
}

u16 DapServer::Port() const {
	return _impl->port;
}

bool DapServer::IsConnected() const {
	return _impl->connected;
}

std::vector<DapServerEvent> DapServer::Take() {
	std::lock_guard lock(_impl->mutex);
	return std::exchange(_impl->inbox, {});
}

void DapServer::Send(std::string json) {
	if (!_impl->running) {
		return;
	}
	{
		std::lock_guard lock(_impl->mutex);
		_impl->outbox.push_back(DapFraming::Encode(json));
	}
	uv_async_send(&_impl->wake);
}

} // namespace sowa
