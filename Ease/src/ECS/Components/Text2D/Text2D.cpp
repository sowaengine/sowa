#include "ECS/Components/Text2D/Text2D.hpp"

namespace Ease::Component {
Text2D::Text2D()
	: m_Text(""), m_Color({255.f, 255.f, 255.f, 255.f}), m_FontSize(64.f) {
}

Text2D::~Text2D() {
}
} // namespace Ease::Component