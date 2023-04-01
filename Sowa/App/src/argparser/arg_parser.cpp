#include "arg_parser.hpp"
#include <iostream>
#include <string>

bool strStartsWith(std::string text, std::string with) {
	// text "he" can not start with "hello"
	if (text.size() < with.size()) {
		return false;
	}
	for (size_t i = 0; i < with.size(); i++) {
		if (text[i] != with[i]) {
			return false;
		}
	}

	return true;
}

ArgOptionValue::ArgOptionValue() : value("") {
}
ArgOptionValue::ArgOptionValue(std::string value) : value(value) {
}
ArgOptionValue::ArgOptionValue(int value) : value(std::to_string(value)) {
}
ArgOptionValue::ArgOptionValue(double value) : value(std::to_string(value)) {
}
ArgOptionValue::ArgOptionValue(bool value) : value(value ? "1" : "0") {
}

ArgOption::ArgOption() : key(""), value(ArgOptionValue()) {
}
ArgOption::ArgOption(std::string key, ArgOptionValue value) : key(key), value(value) {
}

ArgParser::ArgParser(int argc, char const **argv) {
	std::vector<std::string> args{};
	args.reserve(argc);

	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	for (const auto arg : args) {
		if (strStartsWith(arg, "--")) {
			size_t offset = arg.find('=');
			if (offset == std::string::npos) {
				m_Flags.push_back(arg.substr(2));
			} else {
				std::string key = arg.substr(0, offset);
				std::string value = arg.substr(offset + 1);

				auto opt = ArgOption();
				opt.key = key.substr(2);
				opt.value.value = value;
				m_Options.push_back(opt);
			}
		} else if (strStartsWith(arg, "-")) {
			for (size_t i = 1; i < arg.size(); i++) {
				m_Flags.push_back(std::string(arg.data() + i, 1));
			}
		} else {
			m_Params.push_back(arg);
		}
	}
}

ArgOptionValue ArgParser::GetOption(std::string key, ArgOptionValue fallback) {
	for (auto opt : m_Options) {
		if (opt.key == key) {
			return opt.value;
		}
	}
	return fallback;
}

bool ArgParser::HasFlag(std::string flag) {
	for (auto f : m_Flags) {
		if (f == flag) {
			return true;
		}
	}
	return false;
}