#include "particles/particles2d.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace sowa {
namespace {
constexpr f32 Pi = 3.14159265358979323846f;

f32 clampFinite(f32 value, f32 minimum, f32 maximum) {
	return std::clamp(std::isfinite(value) ? value : 0, minimum, maximum);
}

f32 lifetimeProgress(const Particle2D &particle) {
	return static_cast<f32>(std::clamp(particle.age / particle.lifetime, 0.0, 1.0));
}

f32 evaluateRamp(f32 start, f32 middle, f32 end, f32 midpoint, f32 progress) {
	midpoint = clampFinite(midpoint, 0.001f, 0.999f);
	if (progress < midpoint) {
		return std::lerp(start, middle, progress / midpoint);
	}
	return std::lerp(middle, end, (progress - midpoint) / (1 - midpoint));
}
} // namespace

f32 ParticleSize(const Particles2D &settings, const Particle2D &particle) {
	const f32 scale = evaluateRamp(settings.sizeStart, settings.sizeMiddle, settings.sizeEnd,
		settings.sizeMidpoint, lifetimeProgress(particle));
	return clampFinite(particle.size * scale, 0, 1000000);
}

Color ParticleColor(const Particles2D &settings, const Particle2D &particle) {
	const auto &start = settings.colorStart;
	const auto &middle = settings.colorMiddle;
	const auto &end = settings.colorEnd;
	const f32 progress = lifetimeProgress(particle);
	const f32 midpoint = settings.colorMidpoint;

	return Color(evaluateRamp(start.r, middle.r, end.r, midpoint, progress),
		evaluateRamp(start.g, middle.g, end.g, midpoint, progress),
		evaluateRamp(start.b, middle.b, end.b, midpoint, progress),
		clampFinite(evaluateRamp(start.a, middle.a, end.a, midpoint, progress), 0, 1));
}

Rect ParticleUV(const Particles2D &settings, const Particle2D &particle, bool flipVertically) {
	const i32 columns = std::clamp(settings.hFrames, 1, 256);
	const i32 rows = std::clamp(settings.vFrames, 1, 256);
	const i32 frameCount = columns * rows;

	f32 progress = lifetimeProgress(particle) * frameCount;
	if (settings.animationSpeed > 0) {
		progress = particle.age * clampFinite(settings.animationSpeed, 0, 10000);
	}
	progress += particle.frameOffset * frameCount;

	i32 frame = static_cast<i32>(std::min(progress, static_cast<f32>(frameCount - 1)));
	if (settings.animationLoop) {
		frame = static_cast<i32>(std::fmod(progress, static_cast<f32>(frameCount)));
	}

	Rect uv(static_cast<f32>(frame % columns) / columns, static_cast<f32>(frame / columns) / rows,
		1.f / columns, 1.f / rows);
	if (flipVertically) {
		uv.y = 1 - uv.y;
		uv.h = -uv.h;
	}
	return uv;
}

glm::mat4 ParticleTransform(const Particles2D &settings, const Particle2D &particle,
	const glm::mat4 &worldMatrix) {
	f32 rotation = particle.rotation * Pi / 180;
	if (settings.alignToVelocity && particle.velocity.LengthSquared() > 1e-8f) {
		rotation = std::atan2(particle.velocity.y, particle.velocity.x) - Pi * 0.5f;
	}

	const auto base = settings.localCoords ? worldMatrix : glm::mat4(1);
	const glm::vec3 position(particle.position.x, particle.position.y, 0);
	auto transform = glm::translate(base, position);
	if (!settings.localCoords && !settings.alignToVelocity) {
		transform *= particle.basis;
	}
	transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));

	if (!settings.localCoords && settings.alignToVelocity) {
		// Alignment replaces birth rotation, but keeps birth scale and mirroring.
		f32 xScale = glm::length(glm::vec2(particle.basis[0]));
		const f32 yScale = glm::length(glm::vec2(particle.basis[1]));
		if (glm::determinant(glm::mat2(particle.basis)) < 0) {
			xScale = -xScale;
		}
		transform = glm::scale(transform, glm::vec3(xScale, yScale, 1));
	}

	const f32 stretch =
		1 + particle.velocity.Length() * clampFinite(settings.velocityStretch, 0, 10);
	return glm::scale(transform, glm::vec3(1, stretch, 1));
}
} // namespace sowa
