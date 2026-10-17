#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "script/dap_framing.hpp"

using namespace sowa;

TEST_CASE("DapFraming splits framed messages however the bytes arrive") {
	const std::string first = R"({"seq":1,"type":"request","command":"initialize"})";
	const std::string second = R"({"seq":2})";
	const std::string stream = DapFraming::Encode(first) + DapFraming::Encode(second);

	SUBCASE("all at once") {
		DapFraming framing;
		framing.Feed(stream);
		CHECK(framing.Next() == first);
		CHECK(framing.Next() == second);
		CHECK_FALSE(framing.Next().has_value());
	}

	SUBCASE("one byte at a time") {
		DapFraming framing;
		std::vector<std::string> messages;
		for (const char c : stream) {
			framing.Feed(std::string_view(&c, 1));
			while (auto message = framing.Next()) {
				messages.push_back(*message);
			}
		}
		CHECK(messages == std::vector<std::string>{first, second});
	}
}

TEST_CASE("DapFraming reads Content-Length among other headers, in any case") {
	DapFraming framing;
	framing.Feed("Content-Type: application/vscode-jsonrpc\r\ncontent-length: 2\r\n\r\n{}");
	CHECK(framing.Next() == "{}");
	CHECK_FALSE(framing.Failed());
}

TEST_CASE("DapFraming fails on a header block without a length") {
	DapFraming framing;
	framing.Feed("Content-Type: x\r\n\r\n{}");
	CHECK_FALSE(framing.Next().has_value());
	CHECK(framing.Failed());

	DapFraming bad;
	bad.Feed("Content-Length: twelve\r\n\r\n{}");
	CHECK_FALSE(bad.Next().has_value());
	CHECK(bad.Failed());

	DapFraming trailing;
	trailing.Feed("Content-Length: 2x\r\n\r\n{}");
	CHECK_FALSE(trailing.Next().has_value());
	CHECK(trailing.Failed());
}

TEST_CASE("DapFraming::Encode prefixes the byte length") {
	CHECK(DapFraming::Encode("{\"a\":\"\xC3\xA9\"}") ==
		  "Content-Length: 10\r\n\r\n{\"a\":\"\xC3\xA9\"}");
}
