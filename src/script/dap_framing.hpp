#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace sowa {

// DAP framing: headers end with CRLF CRLF and Content-Length gives the JSON byte
// count. Feed accepts partial socket data; Next returns complete messages.
class DapFraming {
  public:
	void Feed(std::string_view bytes);

	// The next complete message body, if one has arrived.
	std::optional<std::string> Next();

	// A header without a valid Content-Length was received. The stream
	// cannot be resynchronised; the connection should be dropped.
	bool Failed() const {
		return _failed;
	}

	static std::string Encode(std::string_view json);

  private:
	std::string _buffer;
	std::size_t _bodyLength = 0;
	bool _inBody = false;
	bool _failed = false;
};

} // namespace sowa
