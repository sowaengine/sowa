#ifndef _E_EDITORTHEME_HPP__
#define _E_EDITORTHEME_HPP__

#pragma once

#include <stdint.h>
#include "imgui-docking/imgui.h"
#include "../Resource.hpp"

typedef uint32_t ResourceID;

namespace Ease
{

class EditorTheme : public BaseResource
{
public:
    EditorTheme();
    ~EditorTheme();

    void LoadFromStyle(ImGuiStyle style);
    ImGuiStyle& GetStyle() { return style; }
private:
    template<typename> friend class ResourceManager;

    ImGuiStyle style;
};

}
#endif