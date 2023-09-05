#ifndef SW_TWEENS_HXX
#define SW_TWEENS_HXX
#pragma once

#include <functional>
#include <vector>

class Tweens {
  public:
	static Tweens &GetInstance();

	void RegisterTween(float duration, std::function<void(float)> callback);
	void Poll(float dt);

  private:
	struct Tween {
		float duration = 0.f;
		float elapsed = 0.f;
		std::function<void(float)> callback;
	};
	std::vector<Tween> m_tweens;
};

#endif // SW_TWEENS_HXX