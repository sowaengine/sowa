#include "node2d.hpp"
#include "debug.hpp"

#include "core/application.hpp"
#include "Core/nm_Matrix.hpp"

#include "core/node_db.hpp"

namespace sowa {
Node2D::Node2D() {
	_NodeType = "Node2D";
	m_type = Typename();
}

void Node2D::Bind() {
	NodeFactory factory;
	factory.construct = []() -> Node* {
		Node* node = Allocator<Node2D>::Get().allocate(1);
		new (node) Node2D;

		return node;
	};

	factory.destruct = [](Node* node) {
		Allocator<Node2D>::Get().deallocate(reinterpret_cast<Node2D *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("Node2D", "Node", factory);

	Serializer::get_singleton().RegisterSerializer(Node2D::Typename(), SerializeImpl(Node2D::SaveImpl, Node2D::LoadImpl));
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

FileBuffer Node2D::SaveImpl(object_type *out) {
	Node2D *o = reinterpret_cast<Node2D *>(out);

	YAML::Node doc = Serializer::get_singleton().SaveWithType<Node>(out).Yaml();
	doc["Position"] = Serializer::get_singleton().Save(&o->Position()).Yaml();
	doc["Rotation"] = o->Rotation();
	doc["Scale"] = Serializer::get_singleton().Save(&o->Scale()).Yaml();
	return FileBuffer(doc);
}

bool Node2D::LoadImpl(object_type *out, const FileBuffer &buf) {
	Debug::Error("Node2D::LoadImpl is not implemented");
	return false;
}

} // namespace sowa
