#include "Debug.hpp"

namespace Debug {

Ease::Event<void(std::string)> &OnPrint() {
	static Ease::Event<void(std::string)> _onPrint;

	return _onPrint;
}

static CallStack s_StackTrace;

FuncEntry::FuncEntry() : _FuncName("unnamed_func"), _File(""), _Line(0) {}
FuncEntry::FuncEntry(const std::string &funcName, const std::string &file, int line)
	: _FuncName(funcName), _File(file), _Line(line) {}
FuncEntry::~FuncEntry() {}

FuncEntryRegister::FuncEntryRegister(const std::string &funcName, const std::string &file, int line) {
	s_StackTrace.emplace(FuncEntry(funcName, file, line));
}
FuncEntryRegister::~FuncEntryRegister() {
	if (s_StackTrace.size() > 0)
		s_StackTrace.pop();
}

void DumpStacktrace() {
	CallStack stack = s_StackTrace;
	Debug::Info("Dumping Stacktrace!");
	while (!stack.empty()) {
		auto &entry = stack.top();
		Debug::Info("\t[{}(), {}:{}]", entry._FuncName, entry._File, entry._Line);
		stack.pop();
	}
	Debug::Info("");
}
} // namespace Debug
