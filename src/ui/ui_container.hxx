#ifndef SW_UI_CONTAINER_HXX
#define SW_UI_CONTAINER_HXX
#pragma once

#include "core/graphics.hxx"
#include "data/color.hxx"
#include "data/margin.hxx"
#include "data/ui_size.hxx"
#include "layout.h"
#include "servers/rendering_server.hxx"
#include <string>
#include <vector>

enum class FlexDirection {
	Stack,
	Row,
	Column
};

enum class Anchor {
	Left = 0,
	Top,
	Right,
	Bottom,
	HFill,
	VFill,
	HCenter,
	VCenter,
	Center,
	Fill
};

enum class LayoutModel {
	Free = 0,
	Flex
};

enum class Wrap {
	NoWrap = 0,
	Wrap
};

enum class JustifyContent {
	Start = 0,
	Middle,
	End,
	Justify
};

class UIContainer {
  public:
	UIContainer();

	inline std::vector<UIContainer> &Children() { return m_children; }

	std::string name = "";
	FlexDirection flexDirection = FlexDirection::Stack;
	Wrap wrap = Wrap::NoWrap;
	LayoutModel layoutModel = LayoutModel::Free;
	JustifyContent justifyContent = JustifyContent::Middle;
	Anchor anchor = Anchor::Center;
	Margin margin = Margin::All(0.f);
	Padding padding = Padding::All(0.f);
	Color backgroundColor = Color::FromRGBFloat(0.2, 0.1, 0.7);
	bool visible = true;
	bool active = true;
	UISize width = "128px";
	UISize height = "128px";
	int id = 0xFF;
	CursorMode cursorMode = CursorMode::Normal;

  private:
	friend class UITree;
	friend class App;

	float m_uitree_x = 0.f;
	float m_uitree_y = 0.f;
	float m_uitree_w = 0.f;
	float m_uitree_h = 0.f;
	lay_id m_uitree_item = LAY_INVALID_ID;

	std::vector<UIContainer> m_children;
	Model m_model;
};

#endif // SW_UI_CONTAINER_HXX