#ifndef SW_NEW_TREE_HXX
#define SW_NEW_TREE_HXX
#pragma once

#include <unordered_map>

#include "new_container.hxx"

class NewTree {
  public:
	NewTree();

	inline NewContainer &Root() { return m_root; }

  private:
	friend class NewContainer;
	void initContainer(NewContainer *container);
  void deinitContainer(NewContainer* container);

  private:
	NewContainer m_root;
	int m_idCounter = 0;

	std::unordered_map<int, NewContainer *> m_containers;
};

#endif // SW_NEW_TREE_HXX