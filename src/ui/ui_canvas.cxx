#include "ui_canvas.hxx"

#include <algorithm>
#include <iostream>

#include "core/app.hxx"
#include "core/error/error.hxx"

UICanvas::UICanvas(NewTree *tree, int id) {
	m_pTree = tree;
	m_id = id;
}

void UICanvas::Text(const std::string &text, Font *font, float scale) {
	NewContainer *container = m_pTree->GetContainerByID(m_id);
	if (nullptr == container) {
		std::cout << "Invalid Container" << std::endl;
		return;
	}

	glm::vec2 size = font->CalcTextSize(text);
	glm::vec2 maxSize(0.f, m_lineSize);

	if (size.y > m_lineSize) {
		scale = std::min(scale, m_lineSize / size.y);
	}

	if (container->Width() < (size.x * scale) + (m_padding * 2)) {
		maxSize.x = container->Width();
	} else if ((size.x * scale) + (m_padding * 2) > container->GetAvailableWidth()) {
		NewLine();
	}

	App::get().Renderer().draw_text(text, font, container->PosX() + container->CursorX() + m_padding, container->PosY() + container->CursorY() - m_lineSize - m_padding, 0.f, glm::mat4(1.f), 0.f, scale, maxSize.x, maxSize.y);
	container->Advance(size.x, 0.f);
}

void UICanvas::NewLine() {
	NewContainer *container = m_pTree->GetContainerByID(m_id);
	if (nullptr == container) {
		std::cout << "Invalid Container" << std::endl;
		return;
	}
	container->Advance(-container->CursorX(), m_lineSize + m_padding);
}