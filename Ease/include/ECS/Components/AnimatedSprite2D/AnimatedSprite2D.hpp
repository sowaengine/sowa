#ifndef _E_ANIMATEDSPRITE2D_HPP__
#define _E_ANIMATEDSPRITE2D_HPP__

#pragma once
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include "Resource/SpriteSheetAnimation/SpriteSheetAnimation.hpp"


namespace Ease::Component
{
    class AnimatedSprite2D
    {
    public:
        AnimatedSprite2D();
        ~AnimatedSprite2D();

        void Step(float ms);

        
        void SetAnimation(const std::string& name, ResourceID animation);
        ResourceID GetAnimation(const std::string& name) { return m_Animations[name]; }
        std::map<std::string, ResourceID>& Animations() { return m_Animations; }

        int& CurrentFrame() { return m_CurrentFrame; }
        int& FPS() { return m_FPS; }
        const std::string& GetSelectedAnimationName() { return m_SelectedAnimationName; }
        ResourceID GetSelectedAnimation() { return m_SelectedAnimation; }

        void SelectAnimation(const std::string& name);


        ResourceID GetCurrentTexture();
    private:
        int m_CurrentFrame;
        int m_FPS;
        ResourceID m_SelectedAnimation;
        std::string m_SelectedAnimationName = "";
        std::map<std::string, ResourceID> m_Animations;


        float m_TicksPast;
    };
}

#endif