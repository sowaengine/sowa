#ifndef _E_INPUT_HPP__
#define _E_INPUT_HPP__
#pragma once

#include "Utils/Math.hpp"

namespace Ease::Input {
// Returns mouse pos in screen relative to center. center would be 0, 0
// Ease::Vec2 GetMousePos();

// Returns mouse pos in 2d game world. bottom right would be {VideoWidth / 2, VideoHeight / 2}
// Ease::Vec2 GetGameMousePos();

enum class Button {
	LEFT = 0,
	RIGHT = 1,
	MIDDLE = 2,
};
// bool IsMouseButtonDown(Button button);
// bool IsMouseButtonClicked(Button button);
// bool IsMouseButtonReleased(Button button);
} // namespace Ease::Input

#endif