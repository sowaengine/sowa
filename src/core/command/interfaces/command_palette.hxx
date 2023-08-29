#ifndef SW_COMMAND_PALETTE_HXX
#define SW_COMMAND_PALETTE_HXX
#pragma once

#include "../command_interface.hxx"

class CommandPaletteInterface : public CommandInterface {
  public:
	CommandPaletteInterface();
	virtual ~CommandPaletteInterface() = default;

  private:
};

#endif // SW_COMMAND_PALETTE_HXX