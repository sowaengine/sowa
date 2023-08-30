#ifndef SW_SCENE_SAVE_AS_HXX
#define SW_SCENE_SAVE_AS_HXX
#pragma once

#include "../command_interface.hxx"

class SceneSaveAsInterface : public CommandInterface {
  public:
	SceneSaveAsInterface() = default;
	SceneSaveAsInterface(std::string path);
	virtual ~SceneSaveAsInterface() = default;

  private:
};

#endif // SW_SCENE_SAVE_AS_HXX