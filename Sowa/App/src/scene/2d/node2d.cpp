#include "node2d.hpp"
#include "debug.hpp"

#include "core/application.hpp"
#include "Core/nm_Matrix.hpp"

namespace Sowa {
Node2D::Node2D() {
	_NodeType = "Node2D";
}

void Node2D::EnterScene() {
}
void Node2D::ExitScene() {
}
void Node2D::UpdateLogic() {
}
void Node2D::UpdateDraw() {
}


const glm::mat4& Node2D::CalculateTransform() {
	auto frameID = Application::get_singleton().GetFrameCount();
	if (frameID == _LastUpdateFrameID) {
		return _LatestTransform;
	}

	Node* parent = GetParent();
	glm::mat4 baseTransform(1.f);
	if(parent != nullptr) {
		// Temporary solution
		// add Node::IsSubClassOf() later
		auto nodeType = parent->GetNodeType();
		if(nodeType == "Node2D" || nodeType == "Sprite2D" || nodeType == "Text2D") {
			Node2D* parentNode2D = dynamic_cast<Node2D*>(parent);
			if(parentNode2D != nullptr) {
				baseTransform = parentNode2D->CalculateTransform();
			}
		}
	}

	_LastUpdateFrameID = frameID;
	_LatestTransform = nmGfx::CalculateModelMatrix(_Position, _Rotation, {_Scale.x, _Scale.y}, {0.f, 0.f}, baseTransform);
	return _LatestTransform;
}
} // namespace Sowa
