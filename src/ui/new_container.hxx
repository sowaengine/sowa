#ifndef SW_NEW_CONTAINER_HXX
#define SW_NEW_CONTAINER_HXX
#pragma once

#include <initializer_list>
#include <vector>

#include "data/color.hxx"

enum class ContainerOrientation {
	Row = 0,
	Column
};

class NewTree;
class NewContainer {
  public:
	NewContainer() = default;
	NewContainer(float size);
	~NewContainer();

	inline int ID() { return m_id; }

	// sum(ratios) == 100 -> Full size
	void SetChildren(std::initializer_list<float> ratios);
	inline void SetOrientation(ContainerOrientation orientation) { m_orientation = orientation; }
	NewContainer *Child(int idx);

	void DrawLayout(float x, float y, float width, float height, float z = 0.f);

	Color color = Color::FromRGB(45, 45, 45);

  private:
	friend class NewTree;

	NewTree *m_pTree = nullptr;

	NewContainer *getContainer(int id);

	int m_id = 0;
	float m_sizePercentage = 100.f;
	ContainerOrientation m_orientation = ContainerOrientation::Row;

	std::vector<NewContainer> m_children;
	NewContainer *m_parent = nullptr;
};

#endif // SW_NEW_CONTAINER_HXX