#include "node2d.hpp"
#include "debug.hpp"

namespace Sowa {
void Node2D::EnterScene() {
	Debug::Log("Node 2D '{}' entered tree", Name());
}
void Node2D::ExitScene() {
	Debug::Log("Node 2D '{}' exited tree", Name());
}
void Node2D::UpdateLogic() {
	Debug::Log("Node 2D '{}' updated logic", Name());
}
void Node2D::UpdateDraw() {
	Debug::Log("Node 2D '{}' updated drwa", Name());
}
} // namespace Sowa
