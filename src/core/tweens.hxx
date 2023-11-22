#ifndef SW_TWEENS_HXX
#define SW_TWEENS_HXX
#pragma once

#include <functional>
#include <vector>

#include "utils/utils.hxx"

class Tweens {
  public:
	static Tweens &get();

	void clear();
	void RegisterTween(float duration, std::function<void(float)> callback, std::function<void()> finished, Utils::Easing easing = Utils::Easing::SINE_EASE_IN);
	void Poll(float dt);

  private:
	struct Tween {
		float duration = 0.f;
		float elapsed = 0.f;
		std::function<void(float)> callback;
		std::function<void()> finished;
		Utils::Easing easing;
	};
	std::vector<Tween> m_tweens;
};

#endif // SW_TWEENS_HXX