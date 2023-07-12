#include "new_tree.hxx"

#include <iostream>

NewTree::NewTree() {
	initContainer(&m_root);
}

void NewTree::initContainer(NewContainer *container) {
	container->m_id = ++m_idCounter;
	container->m_pTree = this;
}

NewContainer *NewTree::GetContainerByID(int id) {
	return m_root.getContainer(id);
}