#ifndef SW_NEW_CONTAINER_HXX
#define SW_NEW_CONTAINER_HXX
#pragma once

#include <initializer_list>
#include <stddef.h>
#include <vector>

#include "data/color.hxx"

enum class ContainerOrientation {
	Row = 0,
	Column
};

class NewTree;
class App;

class NewContainer {
  public:
	NewContainer() = default;
	NewContainer(float size);
	~NewContainer();

	inline int ID() { return m_id; }

	// sum(ratios) == 100 -> Full size
	void SetChildren(std::initializer_list<float> ratios);
	inline void SetOrientation(ContainerOrientation orientation) { m_orientation = orientation; }
	inline ContainerOrientation GetOrientation() { return m_orientation; }
	inline NewContainer *GetParent() { return m_parent; }

	NewContainer *Child(int idx);
	int GetChildIndex(int id);
	int ChildCount();

	void DrawLayout(float x, float y, float width, float height, float z = 0.f);

	Color color = Color::FromRGB(70, 70, 70);
	float minWidth = 5.f;
	float maxWidth = 100.f;
	bool resizable = true;
	bool alwaysDraw = false;

	float Width() { return m_width; }
	float Height() { return m_height; }
	float PosX() { return m_xPos; }
	float PosY() { return m_yPos; }

	inline float CursorX() { return m_cursorX; }
	inline float CursorY() { return m_cursorY; }
	inline void Advance(float x, float y) {
		m_cursorX += x;
		m_cursorY -= y;
	}
	inline float GetAvailableWidth() { return m_width - m_cursorX; }
	inline float GetAvailableHeight() { return m_height - m_cursorY; }

  private:
	friend class NewTree;
	friend class App;

	float m_cursorX = 0.f;
	float m_cursorY = 0.f;

	NewTree *m_pTree = nullptr;

	NewContainer *getContainer(int id);

	int m_id = 0;
	float m_sizePercentage = 100.f;
	ContainerOrientation m_orientation = ContainerOrientation::Row;

	float m_xPos = 0.f;
	float m_yPos = 0.f;
	float m_width = 0.f;
	float m_height = 0.f;

	std::vector<NewContainer> m_children;
	NewContainer *m_parent = nullptr;
};

#endif // SW_NEW_CONTAINER_HXX