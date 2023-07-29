#include "new_tree.hxx"

#include <iostream>

#include "ui_canvas.hxx"

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

UICanvas NewTree::Canvas(int id) {
	return UICanvas(this, id);
}