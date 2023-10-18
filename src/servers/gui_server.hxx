#ifndef SW_GUI_SERVER_HXX
#define SW_GUI_SERVER_HXX
#pragma once

#include "math/math.hxx"

class GuiServer {
  public:
	static GuiServer &get();

	void Initialize();
	void Begin();
	void End();
	void Update();

  private:
	friend class InputServer;

	rect m_viewport_rect;
	bool on_viewport = false;

	size_t scene_rclick_selected_node = 0;
};

#endif // SW_GUI_SERVER_HXX