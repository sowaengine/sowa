#include "node.hpp"

namespace Sowa {
Node::Node() : _Name("Node") {}
Node::Node(const std::string &name) : _Name(name) {}
Node::~Node() {}
} // namespace Sowa
