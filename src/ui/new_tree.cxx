#include "new_tree.hxx"

NewTree::NewTree() {
	initContainer(&m_root);
}

void NewTree::initContainer(NewContainer *container) {
	container->m_id = ++m_idCounter;
	container->m_pTree = this;
	m_containers[m_idCounter] = container;
}

void NewTree::deinitContainer(NewContainer *container) {
	m_containers[container->m_id] = nullptr;
}