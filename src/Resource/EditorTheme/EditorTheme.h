#ifndef EDITORTHEME_H
#define EDITORTHEME_H

#pragma once

#include <stdint.h>
#include "imgui-docking/imgui.h"
#include "../Resource.h"

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