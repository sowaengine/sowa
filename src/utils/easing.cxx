#include "utils.hxx"

float Utils::LerpMap(float t, Easing easing) {
	if (easing == Easing::LINEAR) {
		return t;
	}
	if (easing == Easing::SINE_EASE_IN) {
		return 1.f - std::cos((t * M_PI) * 0.5f);
	}
	if (easing == Easing::SINE_EASE_OUT) {
		return std::sin((t * M_PI) * 0.5f);
	}
	if (easing == Easing::SINE_EASE_IN_OUT) {
		return -(std::cos(M_PI * t) - 1.f) * 0.5f;
	}
	if (easing == Easing::CUBIC_EASE_IN) {
		return t * t * t;
	}
	if (easing == Easing::CUBIC_EASE_OUT) {
		return 1.f - ((1.f - t) * (1.f - t) * (1.f - t));
	}
	if (easing == Easing::CUBIC_EASE_IN_OUT) {
		return t < 0.5f ? 4 * t * t * t : 1.f - ((-2.f * t + 2) * (-2.f * t + 2) * (-2.f * t + 2)) * 0.5f;
	}
	if (easing == Easing::QUINT_IN) {
		return std::pow(t, 5);
	}
	if (easing == Easing::QUINT_OUT) {
		return 1.f - std::pow(1.f - t, 5);
	}
	if (easing == Easing::QUINT_IN_OUT) {
		return t < 0.5 ? 16 * t * t * t * t * t : 1 - std::pow(-2 * t + 2, 5) * 0.5f;
	}
	if (easing == Easing::CIRC_IN) {
		return 1.f - std::sqrt(1.f - std::pow(t, 2));
	}
	if (easing == Easing::CIRC_OUT) {
		return std::sqrt(1.f - std::pow(t - 1.f, 2));
	}
	if (easing == Easing::CIRC_IN_OUT) {
		return t < 0.5f ? (1.f - std::sqrt(1.f - std::pow(2 * t, 2))) * 0.5f : (std::sqrt(1.f - std::pow(-2.f * t + 2.f, 2)) + 1.f) * 0.5f;
	}
	if (easing == Easing::ELASTIC_IN) {
		return t == 0.f ? 0.f : t == 1.f ? 1.f
										 : -std::pow(2, 10 * t - 10.f) * std::sin((t * 10.f - 10.75f) * (2 * M_PI) * 0.33f);
	}
	if (easing == Easing::ELASTIC_OUT) {
		return t == 0.f ? 0.f : t == 1 ? 1.f
									   : std::pow(2, -10.f * t) * std::sin((t * 10.f - 0.75f) * (2 * M_PI) * 0.33f) + 1.f;
	}
	if (easing == Easing::ELASTIC_IN_OUT) {
		return t == 0.f ? 0.f : t == 1.f ? 1.f
							: t < 0.5f	 ? -(std::pow(2, 20 * t - 10.f) * std::sin((20 * t - 11.125) * (2 * M_PI) / 4.5)) * 0.5f
										 : (std::pow(2, -20 * t + 10) * std::sin((20 * t - 11.125) * (2 * M_PI) / 4.5)) * 0.5f + 1;
	}
	if (easing == Easing::QUAD_IN) {
		return t * t;
	}
	if (easing == Easing::QUAD_OUT) {
		return 1.f - (1.f - t) * (1.f - t);
	}
	if (easing == Easing::QUAD_IN_OUT) {
		return t < 0.5f ? 2 * t * t : 1.f - std::pow(-2.f * t + 2.f, 2) * 0.5f;
	}
	if (easing == Easing::QUART_IN) {
		return t * t * t * t;
	}
	if (easing == Easing::QUART_OUT) {
		return 1.f - std::pow(1.f - t, 4);
	}
	if (easing == Easing::QUART_IN_OUT) {
		return t < 0.5f ? 8 * t * t * t * t : 1.f - std::pow(-2.f * t + 2, 4) * 0.5f;
	}
	if (easing == Easing::EXPO_IN) {
		return t == 0.f ? 0 : std::pow(2, 10 * t - 10);
	}
	if (easing == Easing::EXPO_OUT) {
		return t == 1.f ? 1.f : 1.f - std::pow(2, -10 * t);
	}
	if (easing == Easing::EXPO_IN_OUT) {
		return t == 0.f ? 0.f : t == 1.f ? 1.f
							: t < 0.5f	 ? std::pow(2, 20 * t - 10) * 0.5f
										 : (2 - std::pow(2, -20 * t + 10)) * 0.5f;
	}
	if (easing == Easing::BACK_IN) {
		float c1 = 1.70158;
		float c3 = c1 + 1;

		return c3 * t * t * t - c1 * t * t;
	}
	if (easing == Easing::BACK_OUT) {
		float c1 = 1.70158;
		float c3 = c1 + 1;

		return 1.f + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2);
	}
	if (easing == Easing::BACK_IN_OUT) {
		float c1 = 1.70158;
		float c2 = c1 * 1.525;

		return t < 0.5f ? (std::pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) * 0.5f : (std::pow(2 * 2 - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) * 0.5f;
	}
	if (easing == Easing::BOUNCE_IN) {
		return 1.f - LerpMap(1.f - t, Easing::BOUNCE_OUT);
	}
	if (easing == Easing::BOUNCE_OUT) {
		float scaledTime = t / 1;

		if (scaledTime < (1 / 2.75)) {

			return 7.5625 * scaledTime * scaledTime;

		} else if (scaledTime < (2 / 2.75)) {

			float scaledTime2 = scaledTime - (1.5 / 2.75);
			return (7.5625 * scaledTime2 * scaledTime2) + 0.75;

		} else if (scaledTime < (2.5 / 2.75)) {

			float scaledTime2 = scaledTime - (2.25 / 2.75);
			return (7.5625 * scaledTime2 * scaledTime2) + 0.9375;

		} else {

			float scaledTime2 = scaledTime - (2.625 / 2.75);
			return (7.5625 * scaledTime2 * scaledTime2) + 0.984375;
		}
	}
	if (easing == Easing::BOUNCE_IN_OUT) {
		return t < 0.5f ? (1.f - LerpMap(2 * t, Easing::BOUNCE_OUT)) * 0.5f
						: (LerpMap((t * 2) - 1, Easing::BOUNCE_OUT) * 0.5) + 0.5;
	}

	Utils::Error("Unreachable - {}:{}", __FILE__, __LINE__);
	return t;
}

float Utils::Lerp(float from, float to, float t, Easing easing) {
	t = LerpMap(t, easing);
	return from + ((to - from) * t);
}