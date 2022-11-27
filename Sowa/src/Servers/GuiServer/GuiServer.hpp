#ifndef _E_GUISERVER_HPP__
#define _E_GUISERVER_HPP__
#pragma once

#include <string>
#include <unordered_map>

#include "Core/EngineContext.hpp"
#include "Resource/Texture/Texture.hpp"
#include "Utils/Math.hpp"

class GLFWwindow;

namespace Sowa {
class Application;

enum WindowFlags {
	WindowFlags_None = 0,
	WindowFlags_NoResize = 1 << 0,
	WindowFlags_NoMove = 1 << 1,
	WindowFlags_NoBringToFrontOnFocus = 1 << 2,
	WindowFlags_NoNavFocus = 1 << 3,
	WindowFlags_NoDocking = 1 << 4,
	WindowFlags_NoTitleBar = 1 << 5,
	WindowFlags_NoCollapse = 1 << 6,
	WindowFlags_MenuBar = 1 << 7,
	WindowFlags_NoBackground = 1 << 8,
};
enum TreeFlags {
	TreeFlags_None = 0,
	TreeFlags_Leaf = 1 << 0,
	TreeFlags_SpanMinWidth = 1 << 1, // if not given, spans full width
};

enum class StyleVar {
	None = 0,
	WindowPadding,
	WindowRounding,
};

enum class GuiMouseButton {
	None = 0,
	Left,
	Right,
	Middle,
};

class GuiServer {
  public:
	void InitGui(GLFWwindow *window);

	void BeginGui();
	void EndGui();

	bool BeginWindow(const std::string &title, uint32_t flags = 0);
	void EndWindow();

	bool BeginChild(const std::string &id, int width = 0, int height = 0);
	void EndChild();

	void Text(const std::string &text);
	void TextCentered(const std::string &text);
	void TextUnformatted(const std::string &text);
	bool Button(const std::string &label, int width = 0, int height = 0);
	bool DragFloat(const std::string &label, float &f);
	bool DragFloat2(const std::string &label, float &f1, float &f2);
	bool SliderFloat(const std::string &label, float &f, float min, float max);
	bool Checkbox(const std::string &label, bool &value);
	bool InputText(const std::string &label, std::string &value);
	void Separator();
	void Image(Reference<ImageTexture> image, int width, int height);
	bool ImageButton(Reference<ImageTexture> image, int width, int height);

	void SetNextWindowPos(int x, int y);
	void SetNextWindowSize(int width, int height);

	void PushStyleVar(StyleVar var, float value);
	void PushStyleVar(StyleVar var, Vec2 value);
	void PopStyleVar(int count = 1);

	void SetupDockspace();

	bool BeginMainMenuBar();
	void EndMainMenuBar();
	bool BeginMenu(const std::string &label);
	void EndMenu();
	bool MenuItem(const std::string &label, const std::string &shortcut = {""});

	bool BeginFooter(const std::string &label);
	void EndFooter();

	void RegisterPanel(const std::string &windowTitle, const std::string &header, bool visibleByDefault = true);
	void DrawViewbar();

	bool BeginCheckerList(const std::string &id);
	void CheckerListNextItem();
	void EndCheckerList();

	bool BeginTable(const std::string &id, int columns);
	void TableNextRow();
	void TableNextColumn();
	void EndTable();

	bool BeginTree(const std::string &label, uint32_t flags = 0);
	void EndTree();

	void PushID(const std::string &id);
	void PopID();

	void DrawFrame();
	void DrawFilesystem();
	void DrawPlayButton();
	void DrawScene();

	// input
	bool IsWindowHovered();
	bool IsItemHovered();
	bool IsAnyItemHovered(); // any item hovered in current window
	bool IsMouseClicked(GuiMouseButton button);
	bool IsMousePressed(GuiMouseButton button);
	bool IsMouseDoubleClicked(GuiMouseButton button);

	void OpenContextMenu(const std::string &id);
	void CloseCurrentContextMenu();
	bool BeginContextMenu(const std::string &id);
	void EndContextMenu();

	bool Header(const std::string &label);
	void Indent(float width = 0.f);
	void Unindent(float width = 0.f);

	float GetAvailableWidth();
	float GetAvailableHeight();
	float GetTitleHeight();

	void SetScrollRatioY(float r); // 0.0 top, 1.0 bottom
	void SetFocusToNextItem();

	void SameLine();

	// returns mouse position relative to window position (0, 0) being top-left
	Vec2 GetMousePosition();

	// shows dear imgui demo window
	void ShowDemoWindow();

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

	struct PanelData {
		bool visible{false};
		std::string header{""};
	};
	std::unordered_map<std::string, PanelData> _panelViews;

	int _panelStack = 0;

	//
	Reference<Sowa::ImageTexture> _PlayTexture{nullptr};
	Reference<Sowa::ImageTexture> _StopTexture{nullptr};

	Reference<Sowa::ImageTexture> _DirectoryTexture{nullptr};
	Reference<Sowa::ImageTexture> _FileTexture{nullptr};
	std::unordered_map<std::string, Reference<Sowa::ImageTexture>> _FileTextures;
};
} // namespace Sowa

#endif // _E_GUISERVER_HPP__