#ifndef SW_COMMAND_INTERFACE_HXX
#define SW_COMMAND_INTERFACE_HXX
#pragma once

#include <functional>
#include <string>
#include <vector>

struct CommandOption {
	std::string label;
	std::string description;
	std::function<void()> action;
};

class CommandInterface {
  public:
	virtual ~CommandInterface() = default;

	std::vector<CommandOption> options;
	int currentIndex = 0;

  private:
};

#endif // SW_COMMAND_INTERFACE_HXX