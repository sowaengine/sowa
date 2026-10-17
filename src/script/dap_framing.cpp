#include "script/dap_framing.hpp"

#include <cctype>
#include <charconv>

#include <fmt/format.h>

namespace sowa {

namespace {

constexpr std::string_view kHeaderEnd = "\r\n\r\n";
constexpr std::string_view kContentLength = "content-length:";
// A header block or body larger than this is not a debugger talking.
constexpr std::size_t kMaxMessage = 64 * 1024 * 1024;

bool startsWithNoCase(std::string_view text, std::string_view prefix) {
	if (text.size() < prefix.size()) {
		return false;
	}
	for (std::size_t i = 0; i < prefix.size(); ++i) {
		if (std::tolower(static_cast<unsigned char>(text[i])) != prefix[i]) {
			return false;
		}
	}
	return true;
}

} // namespace

void DapFraming::Feed(std::string_view bytes) {
	if (!_failed) {
		_buffer.append(bytes);
	}
}

std::optional<std::string> DapFraming::Next() {
	if (_failed) {
		return std::nullopt;
	}
	if (!_inBody) {
		const std::size_t end = _buffer.find(kHeaderEnd);
		if (end == std::string::npos) {
			if (_buffer.size() > kMaxMessage) {
				_failed = true;
			}
			return std::nullopt;
		}
		std::optional<std::size_t> length;
		std::string_view headers(_buffer.data(), end);
		while (!headers.empty()) {
			const std::size_t lineEnd = headers.find("\r\n");
			const std::string_view line = headers.substr(0, lineEnd);
			if (startsWithNoCase(line, kContentLength)) {
				std::string_view value = line.substr(kContentLength.size());
				while (!value.empty() && value.front() == ' ') {
					value.remove_prefix(1);
				}
				std::size_t parsed = 0;
				const auto result =
					std::from_chars(value.data(), value.data() + value.size(), parsed);
				if (result.ec == std::errc() && result.ptr == value.data() + value.size()) {
					length = parsed;
				}
			}
			if (lineEnd == std::string_view::npos) {
				break;
			}
			headers.remove_prefix(lineEnd + 2);
		}
		if (!length || *length > kMaxMessage) {
			_failed = true;
			return std::nullopt;
		}
		_buffer.erase(0, end + kHeaderEnd.size());
		_bodyLength = *length;
		_inBody = true;
	}
	if (_buffer.size() < _bodyLength) {
		return std::nullopt;
	}
	std::string body = _buffer.substr(0, _bodyLength);
	_buffer.erase(0, _bodyLength);
	_inBody = false;
	return body;
}

std::string DapFraming::Encode(std::string_view json) {
	return fmt::format("Content-Length: {}\r\n\r\n{}", json.size(), json);
}

} // namespace sowa
