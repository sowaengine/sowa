#include <doctest/doctest.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <uv.h>

#include "script/dap_framing.hpp"
#include "script/dap_server.hpp"

using namespace sowa;

namespace {

bool waitFor(const std::function<bool()> &condition, const std::function<void()> &step = {}) {
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (std::chrono::steady_clock::now() < deadline) {
		if (step) {
			step();
		}
		if (condition()) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return condition();
}

// A TCP client on its own libuv loop, driven by the test thread.
struct TestClient {
	uv_loop_t loop{};
	uv_tcp_t tcp{};
	uv_connect_t connect{};
	int status = 1;
	bool closed = false;
	std::string received;

	TestClient() {
		uv_loop_init(&loop);
		uv_tcp_init(&loop, &tcp);
		tcp.data = this;
		connect.data = this;
	}

	~TestClient() {
		Close();
		uv_run(&loop, UV_RUN_DEFAULT);
		uv_loop_close(&loop);
	}

	void Pump() {
		uv_run(&loop, UV_RUN_NOWAIT);
	}

	bool Connect(u16 port) {
		sockaddr_in address{};
		uv_ip4_addr("127.0.0.1", port, &address);
		uv_tcp_connect(&connect, &tcp, reinterpret_cast<const sockaddr *>(&address),
			[](uv_connect_t *request, int result) {
				auto *client = static_cast<TestClient *>(request->data);
				client->status = result;
				if (result == 0) {
					uv_read_start(
						reinterpret_cast<uv_stream_t *>(&client->tcp),
						[](uv_handle_t *, size_t size, uv_buf_t *buf) {
							buf->base = new char[size];
							buf->len = static_cast<decltype(buf->len)>(size);
						},
						[](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
							auto *self = static_cast<TestClient *>(stream->data);
							if (nread > 0) {
								self->received.append(buf->base, static_cast<std::size_t>(nread));
							} else if (nread < 0) {
								self->closed = true;
							}
							delete[] buf->base;
						});
				}
			});
		return waitFor([this] { return status != 1; }, [this] { Pump(); }) && status == 0;
	}

	void Write(std::string data) {
		struct Request {
			uv_write_t write{};
			std::string data;
		};

		auto *request = new Request{{}, std::move(data)};
		uv_buf_t buf =
			uv_buf_init(request->data.data(), static_cast<unsigned>(request->data.size()));
		uv_write(&request->write, reinterpret_cast<uv_stream_t *>(&tcp), &buf, 1,
			[](uv_write_t *write, int) { delete reinterpret_cast<Request *>(write); });
		Pump();
	}

	void Close() {
		if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(&tcp))) {
			uv_close(reinterpret_cast<uv_handle_t *>(&tcp), nullptr);
		}
		Pump();
	}
};

// Collects server events until condition holds.
struct EventLog {
	DapServer &server;
	std::vector<DapServerEvent> events;

	bool WaitFor(const std::function<bool(const std::vector<DapServerEvent> &)> &condition,
		TestClient *client = nullptr) {
		return waitFor([&] {
			if (client != nullptr) {
				client->Pump();
			}
			for (DapServerEvent &event : server.Take()) {
				events.push_back(std::move(event));
			}
			return condition(events);
		});
	}

	std::size_t Count(DapServerEvent::Kind kind) const {
		std::size_t count = 0;
		for (const DapServerEvent &event : events) {
			count += event.kind == kind ? 1 : 0;
		}
		return count;
	}
};

} // namespace

TEST_CASE("The debug adapter listens on the loopback address only") {
	DapServer server;
	std::string error;
	REQUIRE(server.Start(0, error));
	CHECK(server.IsRunning());
	CHECK(server.Address() == "127.0.0.1");
	CHECK(server.Port() != 0);
	server.Stop();
	CHECK_FALSE(server.IsRunning());
}

TEST_CASE("Messages are framed both ways") {
	DapServer server;
	std::string error;
	REQUIRE(server.Start(0, error));
	EventLog log{server};
	TestClient client;
	REQUIRE(client.Connect(server.Port()));
	REQUIRE(log.WaitFor([](const auto &events) { return !events.empty(); }, &client));
	CHECK(log.events[0].kind == DapServerEvent::Kind::Connected);
	CHECK(server.IsConnected());

	const std::string request = R"({"seq":1,"type":"request","command":"initialize"})";
	const std::string framed = DapFraming::Encode(request);
	client.Write(framed.substr(0, 10));
	client.Write(framed.substr(10));
	REQUIRE(log.WaitFor([](const auto &events) { return events.size() == 2; }, &client));
	CHECK(log.events[1].kind == DapServerEvent::Kind::Message);
	CHECK(log.events[1].message == request);

	server.Send(R"({"type":"event","event":"initialized"})");
	REQUIRE(waitFor([&] { return !client.received.empty(); }, [&] { client.Pump(); }));
	CHECK(client.received == DapFraming::Encode(R"({"type":"event","event":"initialized"})"));
}

TEST_CASE("A second client is refused while one is connected") {
	DapServer server;
	std::string error;
	REQUIRE(server.Start(0, error));
	EventLog log{server};
	TestClient first;
	REQUIRE(first.Connect(server.Port()));
	REQUIRE(log.WaitFor([](const auto &events) { return !events.empty(); }, &first));

	TestClient second;
	REQUIRE(second.Connect(server.Port()));
	CHECK(waitFor([&] { return second.closed; }, [&] { second.Pump(); }));
	CHECK(server.IsConnected());
	CHECK(log.Count(DapServerEvent::Kind::Connected) == 1);
}

TEST_CASE("A client that leaves can be replaced") {
	DapServer server;
	std::string error;
	REQUIRE(server.Start(0, error));
	EventLog log{server};
	{
		TestClient client;
		REQUIRE(client.Connect(server.Port()));
		REQUIRE(log.WaitFor([](const auto &events) { return !events.empty(); }, &client));
		client.Close();
	}
	REQUIRE(log.WaitFor([](const auto &events) { return events.size() == 2; }));
	CHECK(log.events[1].kind == DapServerEvent::Kind::Disconnected);
	CHECK_FALSE(server.IsConnected());

	TestClient again;
	REQUIRE(again.Connect(server.Port()));
	REQUIRE(log.WaitFor([](const auto &events) { return events.size() == 3; }, &again));
	CHECK(log.events[2].kind == DapServerEvent::Kind::Connected);
}

TEST_CASE("Garbage framing drops the connection") {
	DapServer server;
	std::string error;
	REQUIRE(server.Start(0, error));
	EventLog log{server};
	TestClient client;
	REQUIRE(client.Connect(server.Port()));
	client.Write("hello\r\n\r\n");
	REQUIRE(log.WaitFor(
		[](const auto &events) {
			return !events.empty() && events.back().kind == DapServerEvent::Kind::Disconnected;
		},
		&client));
	CHECK(waitFor([&] { return client.closed; }, [&] { client.Pump(); }));
}

TEST_CASE("A port in use is reported") {
	DapServer first;
	std::string error;
	REQUIRE(first.Start(0, error));
	DapServer second;
	CHECK_FALSE(second.Start(first.Port(), error));
	CHECK_FALSE(error.empty());
	CHECK_FALSE(second.IsRunning());
}

TEST_CASE("Stop delivers queued messages before closing") {
	DapServer server;
	std::string error;
	REQUIRE(server.Start(0, error));
	EventLog log{server};
	TestClient client;
	REQUIRE(client.Connect(server.Port()));
	REQUIRE(log.WaitFor([](const auto &events) { return !events.empty(); }, &client));

	// Larger than a socket buffer, so it cannot leave in one write: closing
	// without waiting for the queued write would cut it short.
	const std::string big = "\"" + std::string(4 * 1024 * 1024, 'x') + "\"";
	server.Send(big);
	server.Send(R"({"type":"event","event":"terminated"})");
	std::thread stopper([&server] { server.Stop(); });
	const bool closed = waitFor([&] { return client.closed; }, [&] { client.Pump(); });
	stopper.join();
	REQUIRE(closed);
	CHECK(client.received ==
		  DapFraming::Encode(big) + DapFraming::Encode(R"({"type":"event","event":"terminated"})"));
}
