#include "tweens.hxx"
#include "utils/utils.hxx"
#include <cmath>

Tweens &Tweens::GetInstance() {
	static Tweens *tweens = new Tweens;
	return *tweens;
}

void Tweens::RegisterTween(float duration, std::function<void(float)> callback, Utils::Easing easing) {
	Tween t;
	t.duration = std::max(duration, 0.01f);
	t.elapsed = 0.f;
	t.callback = callback;
	t.easing = easing;

	m_tweens.push_back(t);
}

void Tweens::Poll(float dt) {
	for (size_t i = 0; i < m_tweens.size();) {
		Tween &tween = m_tweens[i];

		tween.elapsed += dt;

		float t = tween.elapsed / tween.duration;
		t = std::max(t, 0.f);
		t = std::min(t, 1.f);
		float current = Utils::Lerp(0.f, 1.f, t, tween.easing);
		tween.callback(std::min(current, 1.f));

		if (t >= 1.f) {
			m_tweens.erase(m_tweens.begin() + i);
			i--;
		}

		i++;
	}
}