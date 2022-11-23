#ifndef _E_EDITORTHEME_HPP__
#define _E_EDITORTHEME_HPP__

#pragma once

#include "imgui-docking/imgui.h"
#include <stdint.h>

#include "../Resource.hpp"
#include "Ease.hpp"

namespace Ease {

class EditorTheme : public BaseResource {
  public:
	EditorTheme();
	~EditorTheme();

	void LoadFromStyle(ImGuiStyle style);
	ImGuiStyle &GetStyle() { return style; }

  private:
	template <typename>
	friend class ResourceManager;

	ImGuiStyle style;
};

} // namespace Ease
#endif