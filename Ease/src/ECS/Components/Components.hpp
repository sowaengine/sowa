#ifndef _E_COMPONENTS_HPP__
#define _E_COMPONENTS_HPP__

#include "AnimatedSprite2D/AnimatedSprite2D.hpp"
#include "Button/Button.hpp"
#include "Camera2D/Camera2D.hpp"
#include "Common/Common.hpp"
#include "Group/Group.hpp"
#include "NinePatchRect/NinePatchRect.hpp"
#include "PhysicsBody2D/PhysicsBody2D.hpp"
#include "Sprite2D/Sprite2D.hpp"
#include "Text2D/Text2D.hpp"
#include "Transform2D/Transform2D.hpp"
#include "UITransform/UITransform.hpp"

namespace Ease::Component {
enum Component_ : uint32_t {
	Component_None = 0,
	Component_Common,
	Component_Transform2D,
	Component_Sprite2D,
	Component_Camera2D,
};
}

#endif // _E_COMPONENTS_HPP__