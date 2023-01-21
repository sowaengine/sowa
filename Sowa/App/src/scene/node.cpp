#include "node.hpp"
#include "scene.hpp"

namespace Sowa {
Node::Node() : _Name("Node") {}
Node::Node(const std::string &name) : _Name(name) {}
Node::~Node() {
	if (IsValid()) {
	}
}
} // namespace Sowa
