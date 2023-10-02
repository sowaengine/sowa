#ifndef SW_ANIMATED_SPRITE_2D_HXX
#define SW_ANIMATED_SPRITE_2D_HXX
#pragma once

#include "resource/resource_type.hxx"
#include "scene/node_2d.hxx"

#include <string>

class AnimatedSprite2D : public Node2D {
  public:
	void _start() override;
	void _update() override;
	virtual ~AnimatedSprite2D() = default;

	inline RID &spritesheet() { return m_spritesheet; }
	inline float &speed() { return m_speed; }
	inline bool &playing() { return m_playing; }

	void set_animation(const std::string &name, bool autostart = true);
	void set_animation(uint32_t index, bool autostart = true);

	uint32_t get_animation_index(const std::string &name);

	void start();
	void stop();
	void pause();

  protected:
	RID m_spritesheet;
	float m_speed = 1.f;
	bool m_playing = false;

  private:
	float m_delta = 0.f;
	uint32_t m_anim_index = 0;
	uint32_t m_current_tile = 0;
};

#endif // SW_ANIMATED_SPRITE_2D_HXX