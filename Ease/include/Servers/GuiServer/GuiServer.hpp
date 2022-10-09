#ifndef _E_GUISERVER_HPP__
#define _E_GUISERVER_HPP__
#pragma once

#include <string>

#include "Core/EngineContext.hpp"
#include "Utils/Math.hpp"

class GLFWwindow;

namespace Ease {
class Application;

enum WindowFlags {
	WindowFlags_None = 0,
	WindowFlags_NoResize = 1 << 0,
	WindowFlags_NoMove = 1 << 1,
	WindowFlags_NoBringToFrontOnFocus = 1 << 2,
};
enum class StyleVar {
	None = 0,
	WindowPadding,
	WindowRounding,
};

class GuiServer {
  public:
	void InitGui(GLFWwindow *window);

	void BeginGui();
	void EndGui();

	void BeginWindow(const std::string &title, uint32_t flags = 0);
	void EndWindow();

	void Text(const std::string &text);
	bool Button(const std::string &label, int width = 0, int height = 0);
	bool DragFloat(const std::string &label, float &f);

	void SetNextWindowPos(int x, int y);
	void SetNextWindowSize(int width, int height);

	void PushStyleVar(StyleVar var, float value);
	void PushStyleVar(StyleVar var, Vec2 value);
	void PopStyleVar(int count = 1);

	GuiServer(const GuiServer &) = delete;
	GuiServer(const GuiServer &&) = delete;
	GuiServer &operator=(const GuiServer &) = delete;
	GuiServer &operator=(const GuiServer &&) = delete;

  protected:
	friend class Application;
	GuiServer(Application *app, EngineContext &ctx);
	~GuiServer();

	static GuiServer *CreateServer(Application *app, EngineContext &ctx) {
		return new GuiServer(app, ctx);
	}

	Application *_Application{nullptr};
	EngineContext &_Context;
};
} // namespace Ease

#endif // _E_GUISERVER_HPP__