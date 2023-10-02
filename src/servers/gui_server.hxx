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
};

#endif // SW_GUI_SERVER_HXX