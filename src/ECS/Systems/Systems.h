#ifndef SYSTEMS_H
#define SYSTEMS_H

#pragma once

#include "ECS/Scene/Scene.h"

namespace Ease::Systems
{
    // Updates all systems.
    // TODO: Make Filtered
    void ProcessAll(Ease::Scene* pScene);


    // Specific Systems
    void System_SpriteRenderer2D(Ease::Scene* pScene);
    void System_AnimatedSprite2D(Ease::Scene* pScene);
    void System_TextRenderer2D(Ease::Scene* pScene);

} // namespace Ease::Systems


#endif