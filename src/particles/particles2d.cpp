#include "particles/particles2d.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"

namespace sowa {
namespace {
constexpr f64 FixedTimeStep = 1.0 / 60.0;
constexpr f32 Pi = 3.14159265358979323846f;

f32 finite(f32 value, f32 fallback = 0) {
	return std::isfinite(value) ? value : fallback;
}

f32 bounded(f32 value, f32 low, f32 high) {
	return std::clamp(finite(value), low, high);
}

Vector2 safeVector(Vector2 value) {
	return {bounded(value.x, -100000, 100000), bounded(value.y, -100000, 100000)};
}

f32 randomUnit(ParticleRuntime2D &runtime) {
	// Explicit integer generator avoids implementation-dependent distributions.
	runtime.randomState ^= runtime.randomState << 13;
	runtime.randomState ^= runtime.randomState >> 17;
	runtime.randomState ^= runtime.randomState << 5;
	return static_cast<f32>(runtime.randomState >> 8) / 16777216.f;
}

f32 randomRange(ParticleRuntime2D &runtime, Vector2 limits) {
	return limits.x + (limits.y - limits.x) * randomUnit(runtime);
}

Vector2 transformVector(const glm::mat4 &matrix, Vector2 value, f32 w) {
	const auto transformed = matrix * glm::vec4(value.x, value.y, 0, w);
	return {transformed.x, transformed.y};
}

Particles2D validatedSettings(const Particles2D &source) {
	Particles2D settings = source;
	settings.amount = std::clamp(settings.amount, 0, MaxParticles2D);
	settings.emissionRate = bounded(settings.emissionRate, 0, 100000);
	settings.burstCount = std::clamp(settings.burstCount, 0, MaxParticles2D);
	settings.lifetime = bounded(settings.lifetime, 0.001f, 3600);
	settings.lifetimeRandomness = bounded(settings.lifetimeRandomness, 0, 1);
	settings.duration = bounded(settings.duration, static_cast<f32>(FixedTimeStep), 3600);
	settings.direction = safeVector(settings.direction).Normalized();
	if (settings.direction.LengthSquared() == 0) {
		settings.direction = {0, 1};
	}
	settings.spread = bounded(settings.spread, 0, 180);
	settings.emissionSize = safeVector(settings.emissionSize);
	settings.emissionRadius = bounded(settings.emissionRadius, 0, 100000);
	settings.emissionInnerRadius =
		bounded(settings.emissionInnerRadius, 0, settings.emissionRadius);
	settings.initialVelocity = safeVector(settings.initialVelocity);
	settings.initialSize = {bounded(settings.initialSize.x, 0, 100000),
		bounded(settings.initialSize.y, 0, 100000)};
	settings.initialRotation = safeVector(settings.initialRotation);
	settings.angularVelocity = safeVector(settings.angularVelocity);
	settings.gravity = safeVector(settings.gravity);
	settings.linearAcceleration = bounded(settings.linearAcceleration, -100000, 100000);
	settings.radialAcceleration = bounded(settings.radialAcceleration, -100000, 100000);
	settings.tangentialAcceleration = bounded(settings.tangentialAcceleration, -100000, 100000);
	settings.damping = bounded(settings.damping, 0, 10000);
	settings.turbulenceStrength = bounded(settings.turbulenceStrength, 0, 100000);
	settings.turbulenceFrequency = bounded(settings.turbulenceFrequency, 0, 100);
	settings.turbulenceSpeed = bounded(settings.turbulenceSpeed, -1000, 1000);
	settings.collisionY = finite(settings.collisionY);
	settings.collisionBounce = bounded(settings.collisionBounce, 0, 1);
	settings.collisionFriction = bounded(settings.collisionFriction, 0, 1);
	return settings;
}

bool advanceParticle(const Particles2D &settings, Particle2D &particle, f32 delta) {
	particle.age += delta;
	if (particle.age >= particle.lifetime) {
		return false;
	}
	const Vector2 radial = (particle.position - particle.origin).Normalized();
	const Vector2 tangent(-radial.y, radial.x);
	Vector2 acceleration = settings.gravity;
	acceleration += particle.velocity.Normalized() * settings.linearAcceleration;
	acceleration += radial * settings.radialAcceleration;
	acceleration += tangent * settings.tangentialAcceleration;
	const f32 time = particle.age * settings.turbulenceSpeed + particle.phase;
	acceleration += Vector2(std::sin(particle.position.y * settings.turbulenceFrequency + time),
						std::cos(particle.position.x * settings.turbulenceFrequency + time)) *
					settings.turbulenceStrength;
	particle.velocity += acceleration * delta;
	particle.velocity *= std::exp(-settings.damping * delta);
	particle.position += particle.velocity * delta;
	particle.rotation += particle.angularVelocity * delta;
	if (settings.collisionEnabled && particle.position.y < settings.collisionY &&
		particle.velocity.y < 0) {
		particle.position.y = settings.collisionY;
		particle.velocity.y *= -settings.collisionBounce;
		particle.velocity.x *= 1 - settings.collisionFriction;
	}
	return std::isfinite(particle.position.x) && std::isfinite(particle.position.y) &&
		   std::isfinite(particle.velocity.x) && std::isfinite(particle.velocity.y);
}

Vector2 sampleEmissionPosition(const Particles2D &settings, ParticleRuntime2D &runtime) {
	Vector2 offset;
	switch (settings.emissionShape) {
	case ParticleEmissionShape::Circle:
	case ParticleEmissionShape::Ring: {
		const f32 angle = randomUnit(runtime) * 2 * Pi;
		const f32 inner = settings.emissionShape == ParticleEmissionShape::Ring
							  ? settings.emissionInnerRadius
							  : 0;
		const f32 radius = std::sqrt(std::lerp(inner * inner,
			settings.emissionRadius * settings.emissionRadius, randomUnit(runtime)));
		offset = Vector2(std::cos(angle), std::sin(angle)) * radius;
		break;
	}
	case ParticleEmissionShape::Rectangle:
		offset =
			Vector2(randomUnit(runtime) - 0.5f, randomUnit(runtime) - 0.5f) * settings.emissionSize;
		break;
	case ParticleEmissionShape::Line:
		offset = settings.emissionSize * (randomUnit(runtime) - 0.5f);
		break;
	default:
		break;
	}
	return offset;
}

void emitParticle(const Particles2D &settings, ParticleRuntime2D &runtime, const glm::mat4 &matrix,
	Vector2 inheritedVelocity, f32 age) {
	if (runtime.particles.size() >= static_cast<size_t>(settings.amount)) {
		return;
	}
	Particle2D particle;
	const Vector2 offset = sampleEmissionPosition(settings, runtime);
	const f32 angle = std::atan2(settings.direction.y, settings.direction.x) +
					  (randomUnit(runtime) * 2 - 1) * settings.spread * Pi / 180;
	particle.velocity =
		Vector2(std::cos(angle), std::sin(angle)) * randomRange(runtime, settings.initialVelocity);
	particle.position = offset;
	if (!settings.localCoords) {
		particle.origin = transformVector(matrix, {}, 1);
		particle.position = transformVector(matrix, offset, 1);
		particle.velocity = transformVector(matrix, particle.velocity, 0) + inheritedVelocity;
		particle.basis = matrix;
		particle.basis[3] = glm::vec4(0, 0, 0, 1);
	}
	particle.lifetime = std::max(0.001f,
		settings.lifetime * (1 - randomUnit(runtime) * settings.lifetimeRandomness));
	particle.size = randomRange(runtime, settings.initialSize);
	particle.rotation = randomRange(runtime, settings.initialRotation);
	particle.angularVelocity = randomRange(runtime, settings.angularVelocity);
	particle.phase = randomUnit(runtime) * 2 * Pi;
	particle.frameOffset = settings.randomFrame ? randomUnit(runtime) : 0;
	if (advanceParticle(settings, particle, age)) {
		runtime.particles.push_back(particle);
	}
}

void simulateStep(const Particles2D &settings, ParticleRuntime2D &runtime, const glm::mat4 &matrix,
	Vector2 inherited) {
	std::erase_if(runtime.particles, [&](Particle2D &particle) {
		return !advanceParticle(settings, particle, static_cast<f32>(FixedTimeStep));
	});
	if (!settings.isEmitting || runtime.finished) {
		return;
	}
	f64 remaining = FixedTimeStep;
	if (runtime.delayRemaining > 0) {
		const f64 delay = std::min(remaining, runtime.delayRemaining);
		runtime.delayRemaining -= delay;
		remaining -= delay;
	}
	// Split only when this step crosses a cycle boundary.
	while (remaining > 1e-10 && !runtime.finished) {
		if (!runtime.cycleStarted) {
			runtime.cycleStarted = true;
			for (i32 i = 0; i < settings.burstCount; ++i) {
				emitParticle(settings, runtime, matrix, inherited, static_cast<f32>(remaining));
			}
		}
		const f64 segment = std::min(remaining,
			std::max(0.0, static_cast<f64>(settings.duration) - runtime.cycleTime));
		// Keep fractional births so low rates do not depend on frame rate.
		const f64 emission = runtime.emissionCarry + segment * settings.emissionRate;
		const i32 count = static_cast<i32>(std::floor(emission + 1e-9));
		for (i32 i = 0; i < count; ++i) {
			const f64 birth = (i + 1 - runtime.emissionCarry) / settings.emissionRate;
			emitParticle(settings, runtime, matrix, inherited,
				static_cast<f32>(std::max(0.0, remaining - birth)));
		}
		runtime.emissionCarry = std::max(0.0, emission - count);
		runtime.cycleTime += segment;
		remaining -= segment;
		if (runtime.cycleTime >= settings.duration - 1e-9) {
			runtime.cycleTime = 0;
			runtime.cycleStarted = false;
			if (settings.oneShot) {
				runtime.finished = true;
			}
		}
	}
}
} // namespace

void ParticleRuntime2D::Reset() {
	*this = ParticleRuntime2D{};
}

void Particles2D::Restart() {
	runtime.Reset();
}

void Particles2D::SetEmitting(bool emitting) {
	isEmitting = emitting;
	if (emitting && runtime.finished) {
		runtime.finished = false;
		runtime.cycleStarted = false;
		runtime.cycleTime = 0;
	}
}

void UpdateParticles2D(Particles2D &emitter, const glm::mat4 &matrix, f64 delta) {
	if (!std::isfinite(delta) || delta < 0) {
		return;
	}
	for (i32 col = 0; col < 4; ++col) {
		for (i32 row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[col][row])) {
				return;
			}
		}
	}
	auto &runtime = emitter.runtime;
	const auto settings = validatedSettings(emitter);
	const Vector2 position = transformVector(matrix, {}, 1);
	if (!runtime.initialized || runtime.seed != settings.seed ||
		runtime.localCoords != settings.localCoords) {
		runtime.Reset();
		runtime.initialized = true;
		runtime.seed = settings.seed;
		runtime.randomState = settings.seed == 0 ? 1 : settings.seed;
		runtime.localCoords = settings.localCoords;
		runtime.previousPosition = position;
		runtime.delayRemaining = bounded(settings.startDelay, 0, 3600);
		runtime.particles.reserve(static_cast<size_t>(settings.amount));
		runtime.wasEmitting = settings.isEmitting;
		if (settings.isEmitting) {
			const i32 steps = static_cast<i32>(bounded(settings.preprocess, 0, 10) / FixedTimeStep);
			for (i32 i = 0; i < steps; ++i) {
				simulateStep(settings, runtime, matrix, {});
			}
		}
	}
	if (runtime.particles.size() > static_cast<size_t>(settings.amount)) {
		runtime.particles.resize(settings.amount);
	}
	if (settings.isEmitting && !runtime.wasEmitting && runtime.finished) {
		runtime.finished = false;
		runtime.cycleStarted = false;
		runtime.cycleTime = 0;
	}
	runtime.wasEmitting = settings.isEmitting;
	Vector2 inherited;
	if (delta > 0) {
		const Vector2 velocity = (position - runtime.previousPosition) / static_cast<f32>(delta);
		inherited = safeVector(velocity * bounded(settings.inheritVelocity, 0, 10));
	}
	runtime.previousPosition = position;
	// Bound catch-up work after stalls. All normal frames use identical 60 Hz
	// steps, independently of viewport rendering or editor/game frame rate.
	runtime.accumulator += std::min(delta * bounded(settings.speedScale, 0, 100), 1.0);
	const i32 steps = static_cast<i32>((runtime.accumulator + 1e-9) / FixedTimeStep);
	for (i32 i = 0; i < steps; ++i) {
		simulateStep(settings, runtime, matrix, inherited);
	}
	runtime.accumulator = std::max(0.0, runtime.accumulator - steps * FixedTimeStep);
}

void RegisterParticles2D(World &world) {
	auto builder = world.RegisterComponent<Particles2D>("Particles2D");
	if (!builder) {
		return;
	}
	builder.Field("is_emitting", &Particles2D::isEmitting);
	builder.Field("amount", &Particles2D::amount);
	builder.Field("emissionRate", &Particles2D::emissionRate);
	builder.Field("burstCount", &Particles2D::burstCount);
	builder.Field("lifetime", &Particles2D::lifetime);
	builder.Field("lifetimeRandomness", &Particles2D::lifetimeRandomness);
	builder.Field("oneShot", &Particles2D::oneShot);
	builder.Field("duration", &Particles2D::duration);
	builder.Field("startDelay", &Particles2D::startDelay);
	builder.Field("preprocess", &Particles2D::preprocess);
	builder.Field("speedScale", &Particles2D::speedScale);
	builder.Field("seed", &Particles2D::seed);
	builder.Field("localCoords", &Particles2D::localCoords);
	builder.Field("emissionShape", &Particles2D::emissionShape);
	builder.Field("emissionSize", &Particles2D::emissionSize);
	builder.Field("emissionRadius", &Particles2D::emissionRadius);
	builder.Field("emissionInnerRadius", &Particles2D::emissionInnerRadius);
	builder.Field("direction", &Particles2D::direction);
	builder.Field("spread", &Particles2D::spread);
	builder.Field("initialVelocity", &Particles2D::initialVelocity);
	builder.Field("inheritVelocity", &Particles2D::inheritVelocity);
	builder.Field("gravity", &Particles2D::gravity);
	builder.Field("linearAcceleration", &Particles2D::linearAcceleration);
	builder.Field("radialAcceleration", &Particles2D::radialAcceleration);
	builder.Field("tangentialAcceleration", &Particles2D::tangentialAcceleration);
	builder.Field("damping", &Particles2D::damping);
	builder.Field("turbulenceStrength", &Particles2D::turbulenceStrength);
	builder.Field("turbulenceFrequency", &Particles2D::turbulenceFrequency);
	builder.Field("turbulenceSpeed", &Particles2D::turbulenceSpeed);
	builder.Field("initialRotation", &Particles2D::initialRotation);
	builder.Field("angularVelocity", &Particles2D::angularVelocity);
	builder.Field("alignToVelocity", &Particles2D::alignToVelocity);
	builder.Field("velocityStretch", &Particles2D::velocityStretch);
	builder.Field("initialSize", &Particles2D::initialSize);
	builder.Field("sizeStart", &Particles2D::sizeStart);
	builder.Field("sizeMiddle", &Particles2D::sizeMiddle);
	builder.Field("sizeEnd", &Particles2D::sizeEnd);
	builder.Field("sizeMidpoint", &Particles2D::sizeMidpoint);
	builder.Field("colorStart", &Particles2D::colorStart);
	builder.Field("colorMiddle", &Particles2D::colorMiddle);
	builder.Field("colorEnd", &Particles2D::colorEnd);
	builder.Field("colorMidpoint", &Particles2D::colorMidpoint);
	builder.Field("drawShape", &Particles2D::drawShape);
	builder.Field("blendMode", &Particles2D::blendMode);
	builder.ResourceField("texture", &Particles2D::texture, "ImageTexture");
	builder.Field("hFrames", &Particles2D::hFrames);
	builder.Field("vFrames", &Particles2D::vFrames);
	builder.Field("animationSpeed", &Particles2D::animationSpeed);
	builder.Field("animationLoop", &Particles2D::animationLoop);
	builder.Field("randomFrame", &Particles2D::randomFrame);
	builder.Field("collisionEnabled", &Particles2D::collisionEnabled);
	builder.Field("collisionY", &Particles2D::collisionY);
	builder.Field("collisionBounce", &Particles2D::collisionBounce);
	builder.Field("collisionFriction", &Particles2D::collisionFriction);

	world.Flecs()
		.system<Particles2D, const Transform2D>("SimulateParticles2D")
		.kind(flecs::PreStore)
		.each([](flecs::entity entity, Particles2D &emitter, const Transform2D &transform) {
			UpdateParticles2D(emitter, transform.worldMatrix, entity.world().delta_time());
		});
}

std::size_t CountLiveParticles2D(World &world) {
	std::size_t count = 0;
	world.Flecs().each(
		[&count](const Particles2D &emitter) { count += emitter.runtime.particles.size(); });
	return count;
}

} // namespace sowa
