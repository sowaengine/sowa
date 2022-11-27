#include "ECS/Components/UITransform/UITransform.hpp"

namespace Sowa::Component {
UITransform::UITransform()
	: m_Position(0.f, 0.f), m_Scale(1.f, 1.f), m_Size(64.f, 64.f), m_Rotation(0.f), m_ZIndex(0) {
}

UITransform::~UITransform() {
}
} // namespace Sowa::Component