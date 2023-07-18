#include "new_container.hxx"

#include <iostream>
#include <stddef.h>

#include "core/app.hxx"
#include "new_tree.hxx"

NewContainer::NewContainer(float size) {
	m_sizePercentage = size;
}

NewContainer::~NewContainer() {
}

void NewContainer::SetChildren(std::initializer_list<float> ratios) {
	m_children.clear();

	for (float f : ratios) {
		NewContainer &child = m_children.emplace_back();
		child.m_sizePercentage = f;
		child.m_parent = this;
		m_pTree->initContainer(&child);
	}
}

NewContainer *NewContainer::Child(int idx) {
	if (static_cast<size_t>(idx) >= m_children.size()) {
		return nullptr;
	}

	return &m_children[idx];
}

int NewContainer::GetChildIndex(int id) {
	size_t index = 0;

	for (NewContainer &child : m_children) {
		if (child.ID() == id) {
			return index;
		}
		index++;
	}

	return -1;
}

int NewContainer::ChildCount() {
	return static_cast<int>(m_children.size());
}

void NewContainer::DrawLayout(float x, float y, float width, float height, float z) {
	m_xPos = x;
	m_yPos = y;
	m_width = width;
	m_height = height;

	float padding = 0.f;
	if (alwaysDraw || m_children.size() == 0) {
		Color c = color;
		if (App::GetInstance().HoveredItem() == ID()) {
			// c = Color::FromRGB(255, 255, 255);
		}

		App::GetInstance().Renderer().PushQuad(
			x + padding,
			y + padding,
			z,
			width - (padding * 2),
			height - (padding * 2),
			c.r, c.g, c.b, c.a,
			static_cast<float>(m_id),
			0.f, 1.f);
	}

	float xCursor = x;
	float yCursor = y;

	for (auto &child : m_children) {
		float w = 0.f;
		float h = 0.f;

		if (m_orientation == ContainerOrientation::Row) {
			w = width * child.m_sizePercentage / 100.f;
			h = height;
		} else {
			w = width;
			h = height * child.m_sizePercentage / 100.f;
		}

		child.DrawLayout(xCursor, yCursor, w, h, z + 0.1f);

		if (m_orientation == ContainerOrientation::Row) {
			xCursor += w;
		} else {
			yCursor += h;
		}
	}
}

NewContainer *NewContainer::getContainer(int id) {
	for (auto &child : m_children) {
		if (child.ID() == id) {
			return &child;
		}

		NewContainer *container = child.getContainer(id);
		if (nullptr != container) {
			return container;
		}
	}

	return nullptr;
}