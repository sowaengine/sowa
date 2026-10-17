#include "script/dap_session.hpp"

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/log.hpp"

namespace sowa {

using Json = nlohmann::json;

namespace {

constexpr i64 kLuaThread = 1;
constexpr std::string_view kResourcePrefix = "res://";
constexpr const char *kUncaughtFilter = "uncaught";

const char *stopReasonName(LuaStopReason reason) {
	switch (reason) {
	case LuaStopReason::Breakpoint:
		return "breakpoint";
	case LuaStopReason::Step:
		return "step";
	case LuaStopReason::Pause:
		return "pause";
	case LuaStopReason::Exception:
		return "exception";
	}
	return "pause";
}

bool isIdentifier(std::string_view text) {
	if (text.empty() || std::isdigit(static_cast<unsigned char>(text[0])) != 0) {
		return false;
	}
	return std::all_of(text.begin(), text.end(),
		[](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; });
}

std::filesystem::path normalized(const std::filesystem::path &path) {
	std::filesystem::path result = path.lexically_normal();
	// "dir/" and "dir" are the same root.
	if (!result.empty() && !result.has_filename()) {
		result = result.parent_path();
	}
	return result;
}

#ifdef _WIN32
// Drive letters and paths compare without case on Windows; VS Code may send
// "c:\..." where the engine has "C:\...".
std::string comparable(const std::filesystem::path &path) {
	std::string text = path.generic_string();
	std::transform(text.begin(), text.end(), text.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}
#else
std::string comparable(const std::filesystem::path &path) {
	return path.generic_string();
}
#endif

Json variableJson(const LuaVariable &variable) {
	return Json{{"name", variable.name}, {"value", variable.value}, {"type", variable.type},
		{"variablesReference", variable.reference}};
}

} // namespace

DapSession::DapSession(LuaDebugger &debugger, Send send, Hooks hooks)
	: _debugger(debugger), _send(std::move(send)), _hooks(std::move(hooks)) {
}

void DapSession::SetProjectRoot(std::filesystem::path root) {
	_projectRoot = normalized(root);
}

std::optional<std::string> DapSession::ToResource(std::string_view path) const {
	const std::filesystem::path &root = _localRoot.empty() ? _projectRoot : _localRoot;
	if (root.empty() || path.empty()) {
		return std::nullopt;
	}
	const std::filesystem::path file = normalized(std::filesystem::path(path));
	const std::string rootText = comparable(root);
	const std::string fileText = comparable(file);
	if (fileText.size() <= rootText.size() || fileText.compare(0, rootText.size(), rootText) != 0 ||
		fileText[rootText.size()] != '/') {
		return std::nullopt;
	}
	// The original spelling of the relative part: resources keep their case.
	return std::string(kResourcePrefix) + file.generic_string().substr(rootText.size() + 1);
}

std::string DapSession::ToPath(std::string_view resource) const {
	const std::filesystem::path &root = _localRoot.empty() ? _projectRoot : _localRoot;
	if (root.empty() || !resource.starts_with(kResourcePrefix)) {
		return {};
	}
	return (root / std::filesystem::path(resource.substr(kResourcePrefix.size()))).string();
}

void DapSession::sendEvent(std::string_view event, const std::string &bodyJson) {
	Json message{{"seq", ++_seq}, {"type", "event"}, {"event", event}};
	if (!bodyJson.empty()) {
		message["body"] = Json::parse(bodyJson);
	}
	_send(message.dump());
}

void DapSession::Handle(std::string_view text) {
	const Json request = Json::parse(text, nullptr, false);
	if (request.is_discarded() || !request.is_object() || request.value("type", "") != "request") {
		Log::Warn("Debug adapter: ignoring a malformed message");
		return;
	}
	const std::string command = request.value("command", "");
	const Json arguments = request.value("arguments", Json::object());

	Json body = Json::object();
	std::string error;
	bool sendInitialized = false;
	const bool stopped = _debugger.IsStopped();
	const auto requireStopped = [&] {
		if (!stopped) {
			error = "Lua is not stopped";
		}
		return stopped;
	};
	const auto resume = [&](LuaResumeAction action) {
		if (requireStopped() && _hooks.resume) {
			_hooks.resume(action);
		}
	};

	// A field of the wrong type throws; the request then fails rather than
	// the exception reaching the frame loop or, worse, a Lua hook.
	try {
		if (command == "initialize") {
			body = Json{{"supportsConfigurationDoneRequest", true},
				{"supportsConditionalBreakpoints", true},
				{"supportsHitConditionalBreakpoints", true}, {"supportsLogPoints", true},
				{"supportsEvaluateForHovers", true}, {"supportsSetVariable", true},
				{"exceptionBreakpointFilters",
					Json::array({Json{{"filter", kUncaughtFilter}, {"label", "Uncaught Errors"},
						{"description", "Stop on errors no script catches"},
						{"default", false}}})}};
			sendInitialized = true;
		} else if (command == "attach") {
			_attached = true;
			_configured = false;
			_localRoot.clear();
			if (const auto root = arguments.find("localRoot");
				root != arguments.end() && root->is_string() && !root->get<std::string>().empty()) {
				_localRoot = normalized(root->get<std::string>());
			}
		} else if (command == "launch") {
			error = "Sowa is debugged by attaching to a running editor or game";
		} else if (command == "setBreakpoints") {
			const std::string path = arguments.value("source", Json::object()).value("path", "");
			const std::optional<std::string> resource = ToResource(path);
			std::vector<LuaBreakpoint> breakpoints;
			Json results = Json::array();
			for (const Json &requested : arguments.value("breakpoints", Json::array())) {
				LuaBreakpoint breakpoint;
				breakpoint.line = requested.value("line", 0);
				breakpoint.condition = requested.value("condition", "");
				breakpoint.hitCondition = requested.value("hitCondition", "");
				breakpoint.logMessage = requested.value("logMessage", "");
				Json result{{"verified", resource.has_value()}, {"line", breakpoint.line}};
				if (!resource) {
					result["message"] = "Not a file in the project";
				}
				results.push_back(std::move(result));
				breakpoints.push_back(std::move(breakpoint));
			}
			if (resource) {
				_debugger.SetBreakpoints(*resource, std::move(breakpoints));
			}
			body = Json{{"breakpoints", std::move(results)}};
		} else if (command == "setExceptionBreakpoints") {
			const Json filters = arguments.value("filters", Json::array());
			_debugger.SetBreakOnError(
				std::find(filters.begin(), filters.end(), kUncaughtFilter) != filters.end());
		} else if (command == "configurationDone") {
			_configured = true;
		} else if (command == "threads") {
			body = Json{{"threads", Json::array({Json{{"id", kLuaThread}, {"name", "Lua"}}})}};
		} else if (command == "stackTrace") {
			if (requireStopped()) {
				const std::vector<LuaStackFrame> frames = _debugger.StackTrace();
				const auto start =
					static_cast<std::size_t>(std::max(0, arguments.value("startFrame", 0)));
				const int levels = arguments.value("levels", 0);
				const std::size_t end =
					levels > 0 ? std::min(frames.size(), start + static_cast<std::size_t>(levels))
							   : frames.size();
				Json list = Json::array();
				for (std::size_t i = start; i < end; ++i) {
					const LuaStackFrame &frame = frames[i];
					Json item{{"id", frame.id}, {"name", frame.name}, {"line", frame.line},
						{"column", frame.line > 0 ? 1 : 0}};
					const std::string path = ToPath(frame.source);
					if (!frame.source.empty()) {
						Json source{
							{"name", std::filesystem::path(frame.source).filename().string()}};
						if (!path.empty()) {
							source["path"] = path;
						}
						item["source"] = std::move(source);
					} else {
						item["presentationHint"] = "subtle";
					}
					list.push_back(std::move(item));
				}
				body = Json{{"stackFrames", std::move(list)}, {"totalFrames", frames.size()}};
			}
		} else if (command == "scopes") {
			if (requireStopped()) {
				Json scopes = Json::array();
				for (const LuaScope &scope : _debugger.Scopes(arguments.value("frameId", 0))) {
					scopes.push_back(
						Json{{"name", scope.name}, {"variablesReference", scope.reference},
							{"expensive", scope.name == "Globals"}});
				}
				body = Json{{"scopes", std::move(scopes)}};
			}
		} else if (command == "variables") {
			if (requireStopped()) {
				Json variables = Json::array();
				for (const LuaVariable &variable :
					_debugger.Variables(arguments.value("variablesReference", 0))) {
					variables.push_back(variableJson(variable));
				}
				body = Json{{"variables", std::move(variables)}};
			}
		} else if (command == "evaluate") {
			const std::string expression = arguments.value("expression", "");
			if (requireStopped()) {
				// Hovering must never run code: only plain names are looked up.
				if (arguments.value("context", "") == "hover" && !isIdentifier(expression)) {
					error = "Hover shows plain variables only";
				} else {
					const LuaEvalResult result =
						_debugger.Evaluate(arguments.value("frameId", 0), expression);
					if (result.success) {
						body = Json{{"result", result.value}, {"type", result.type},
							{"variablesReference", result.reference}};
					} else {
						error = result.value;
					}
				}
			}
		} else if (command == "setVariable") {
			if (requireStopped()) {
				const LuaEvalResult result =
					_debugger.SetVariable(arguments.value("variablesReference", 0),
						arguments.value("name", ""), arguments.value("value", ""));
				if (result.success) {
					body = Json{{"value", result.value}, {"type", result.type},
						{"variablesReference", result.reference}};
				} else {
					error = result.value;
				}
			}
		} else if (command == "continue") {
			resume(LuaResumeAction::Continue);
			body = Json{{"allThreadsContinued", true}};
		} else if (command == "next") {
			resume(LuaResumeAction::StepOver);
		} else if (command == "stepIn") {
			resume(LuaResumeAction::StepIn);
		} else if (command == "stepOut") {
			resume(LuaResumeAction::StepOut);
		} else if (command == "pause") {
			if (_hooks.pause) {
				_hooks.pause();
			} else {
				_debugger.RequestPause();
			}
		} else if (command == "disconnect") {
			detach();
		} else {
			error = "Unsupported request '" + command + "'";
		}
	} catch (const std::exception &exception) {
		error = std::string("Invalid request: ") + exception.what();
		body = Json::object();
		sendInitialized = false;
	}

	Json response{{"seq", ++_seq}, {"type", "response"}, {"request_seq", request.value("seq", 0)},
		{"command", command}, {"success", error.empty()}};
	if (!error.empty()) {
		response["message"] = error;
	} else if (!body.empty()) {
		response["body"] = std::move(body);
	}
	_send(response.dump());
	if (sendInitialized) {
		sendEvent("initialized", {});
	}
}

void DapSession::detach() {
	const bool wasAttached = _attached;
	_attached = false;
	_configured = false;
	_localRoot.clear();
	if (!wasAttached) {
		return;
	}
	_debugger.ClearAllBreakpoints();
	_debugger.SetBreakOnError(false);
	if (_debugger.IsStopped() && _hooks.resume) {
		_hooks.resume(LuaResumeAction::Continue);
	}
}

void DapSession::OnStopped(const LuaStopEvent &event) {
	if (!_attached) {
		return;
	}
	Json body{{"reason", stopReasonName(event.reason)}, {"threadId", kLuaThread},
		{"allThreadsStopped", true}};
	if (!event.message.empty()) {
		body["description"] = event.message;
		body["text"] = event.message;
	}
	sendEvent("stopped", body.dump());
}

void DapSession::OnContinued() {
	if (_attached) {
		sendEvent("continued",
			Json{{"threadId", kLuaThread}, {"allThreadsContinued", true}}.dump());
	}
}

void DapSession::OnOutput(std::string_view text) {
	if (_attached) {
		sendEvent("output",
			Json{{"category", "console"}, {"output", std::string(text) + "\n"}}.dump());
	}
}

void DapSession::OnConnectionClosed() {
	detach();
}

void DapSession::OnTerminated() {
	if (_attached) {
		sendEvent("terminated", {});
	}
}

} // namespace sowa
