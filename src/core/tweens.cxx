#include "tweens.hxx"
#include "utils/utils.hxx"
#include <cmath>

static float lerp(float from, float to, float t) {
	t = 1.f - std::pow(1.f - t, 3);
	return from + ((to - from) * t);
}

Tweens &Tweens::GetInstance() {
	static Tweens *tweens = new Tweens;
	return *tweens;
}

void Tweens::RegisterTween(float duration, std::function<void(float)> callback) {
	Tween t;
	t.duration = duration;
	t.elapsed = 0.f;
	t.callback = callback;

	m_tweens.push_back(t);
}

void Tweens::Poll(float dt) {
	for (size_t i = 0; i < m_tweens.size();) {
		Tween &tween = m_tweens[i];

		tween.elapsed += dt;

		float current = lerp(0.f, 1.f, tween.elapsed / tween.duration);
		tween.callback(std::min(current, 1.f));

		if (current >= 1.f) {
			m_tweens.erase(m_tweens.begin() + i);
			i--;
		}

		i++;
	}
}