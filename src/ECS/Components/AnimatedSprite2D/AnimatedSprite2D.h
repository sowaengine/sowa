#ifndef ANIMATEDSPRITE2D_H
#define ANIMATEDSPRITE2D_H

#pragma once
#include <stdint.h>
#include <vector>
#include <unordered_map>

namespace Ease
{
    struct Animation
    {
        int hFrames;
        int vFrames;
        int SelectedRow;
        int frameCount;
        int startFrame;
        uint32_t TextureID;
    };
} // namespace Ease


namespace Ease::Component
{
    class AnimatedSprite2D
    {
    public:
        AnimatedSprite2D();
        ~AnimatedSprite2D();

        void Step(float ms);

        
        void SetAnimation(const std::string& name, const Animation& animation) { m_Animations[name] = animation; }
        Ease::Animation& GetAnimation(const std::string& name) { return m_Animations[name]; }
        std::unordered_map<std::string, Ease::Animation>& Animations() { return m_Animations; }

        int& CurrentFrame() { return m_CurrentFrame; }
        int& FPS() { return m_FPS; }
        std::string& SelectedAnimation() { return m_SelectedAnimation; }


        uint32_t GetCurrentTexture() { return GetAnimation(m_SelectedAnimation).TextureID; }
    private:
        int m_CurrentFrame;
        int m_FPS;
        std::string m_SelectedAnimation = "";
        std::unordered_map<std::string, Ease::Animation> m_Animations;


        float m_TicksPast;
    };
}

#endif