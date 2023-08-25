#ifndef SW_UI_CANVAS_HXX
#define SW_UI_CANVAS_HXX
#pragma once

#include <string>

#include "new_tree.hxx"
#include "resource/font/font.hxx"

class UICanvas {
  public:
	UICanvas() = delete;

	UICanvas(NewTree *tree, int id);

	void Text(const std::string &text, Font *font, float scale = 1.f);
	void NewLine();

  private:
	NewTree *m_pTree = nullptr;
	int m_id = 0;

	float m_lineSize = 14.f;
	float m_padding = 2.f;
};

#endif // SW_UI_CANVAS_HXX