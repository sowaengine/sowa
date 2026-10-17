#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

#include "core/reflection/field_type.hpp"
#include "math/rect.hpp"
#include "resource/resource_id.hpp"

namespace sowa {
class World;

enum class ParticleEmissionShape {
	Point,
	Circle,
	Ring,
	Rectangle,
	Line,
};
SOWA_REFLECT_ENUM(ParticleEmissionShape, {"Point", ParticleEmissionShape::Point},
	{"Circle", ParticleEmissionShape::Circle}, {"Ring", ParticleEmissionShape::Ring},
	{"Rectangle", ParticleEmissionShape::Rectangle}, {"Line", ParticleEmissionShape::Line});

enum class ParticleDrawShape {
	SoftCircle,
	Circle,
	Quad,
};
SOWA_REFLECT_ENUM(ParticleDrawShape, {"SoftCircle", ParticleDrawShape::SoftCircle},
	{"Circle", ParticleDrawShape::Circle}, {"Quad", ParticleDrawShape::Quad});

enum class ParticleBlendMode {
	Alpha,
	Additive,
};
SOWA_REFLECT_ENUM(ParticleBlendMode, {"Alpha", ParticleBlendMode::Alpha},
	{"Additive", ParticleBlendMode::Additive});

enum class ParticlePreset {
	Smoke,
	Flame,
	Water,
	Sparks,
	Explosion,
	Snow,
};

struct Particle2D {
	Vector2 position;
	Vector2 velocity;
	Vector2 origin;
	// World-space particles retain the emitter's scale/rotation at birth.
	glm::mat4 basis{1.f};
	f64 age = 0;
	f32 lifetime = 1;
	f32 size = 1;
	f32 rotation = 0;
	f32 angularVelocity = 0;
	f32 phase = 0;
	f32 frameOffset = 0;
};

// Runtime is never reflected or serialized. Copies (including play snapshots,
// duplicates and undo payloads) start fresh; ECS storage moves preserve it.
struct ParticleRuntime2D {
	ParticleRuntime2D() = default;

	ParticleRuntime2D(const ParticleRuntime2D &) {
	}

	ParticleRuntime2D &operator=(const ParticleRuntime2D &) {
		Reset();
		return *this;
	}

	ParticleRuntime2D(ParticleRuntime2D &&) noexcept = default;
	ParticleRuntime2D &operator=(ParticleRuntime2D &&) noexcept = default;
	void Reset();

	std::vector<Particle2D> particles;
	f64 accumulator = 0;
	f64 cycleTime = 0;
	f64 emissionCarry = 0;
	f64 delayRemaining = 0;
	u32 randomState = 1;
	u32 seed = 1;
	bool initialized = false;
	bool wasEmitting = false;
	bool finished = false;
	bool cycleStarted = false;
	bool localCoords = false;
	Vector2 previousPosition;
};

// CPU-simulated 2D visual effects. Distances are pixels, time is seconds and
// angles are degrees. Vector2 ranges are (minimum, maximum). All settings are
// reflected, so the same component works in scenes, the inspector and Lua.
struct Particles2D {
	bool isEmitting = true;
	i32 amount = 256; // maximum live particles, not the emission rate
	f32 emissionRate = 60;
	i32 burstCount = 0; // emitted at the beginning of each duration cycle
	f32 lifetime = 2;
	f32 lifetimeRandomness = 0;
	bool oneShot = false;
	f32 duration = 1;
	f32 startDelay = 0;
	f32 preprocess = 0;
	f32 speedScale = 1;
	u32 seed = 1;
	bool localCoords = false;

	ParticleEmissionShape emissionShape = ParticleEmissionShape::Point;
	Vector2 emissionSize{32, 32}; // rectangle full size; line endpoints +/- size/2
	f32 emissionRadius = 16;
	f32 emissionInnerRadius = 8;
	Vector2 direction{0, 1};
	f32 spread = 20; // half-angle of the emission cone
	Vector2 initialVelocity{30, 60};
	f32 inheritVelocity = 0;
	Vector2 gravity{0, 0};
	f32 linearAcceleration = 0;
	f32 radialAcceleration = 0;
	f32 tangentialAcceleration = 0;
	f32 damping = 0;
	f32 turbulenceStrength = 0;
	f32 turbulenceFrequency = 0.03f;
	f32 turbulenceSpeed = 1;
	Vector2 initialRotation{0, 360};
	Vector2 angularVelocity{0, 0};
	bool alignToVelocity = false;
	f32 velocityStretch = 0;

	Vector2 initialSize{8, 16};
	// Piecewise linear lifetime ramps, with an adjustable middle key time.
	f32 sizeStart = 1;
	f32 sizeMiddle = 1;
	f32 sizeEnd = 0;
	f32 sizeMidpoint = 0.5f;
	Color colorStart{1, 1, 1, 1};
	Color colorMiddle{1, 1, 1, 1};
	Color colorEnd{1, 1, 1, 0};
	f32 colorMidpoint = 0.5f;
	ParticleDrawShape drawShape = ParticleDrawShape::SoftCircle;
	ParticleBlendMode blendMode = ParticleBlendMode::Alpha;
	ResourceID texture; // optional ImageTexture overrides the procedural shape
	i32 hFrames = 1;
	i32 vFrames = 1;
	f32 animationSpeed = 0; // frames/sec; zero traverses the atlas over lifetime
	bool animationLoop = false;
	bool randomFrame = false;

	// Optional horizontal collision plane in simulation coordinates. This is
	// for visual droplets/debris, independent of gameplay rigid bodies.
	bool collisionEnabled = false;
	f32 collisionY = 0;
	f32 collisionBounce = 0.5f;
	f32 collisionFriction = 0.1f;

	ParticleRuntime2D runtime;
	void SetEmitting(bool emitting);
	void Restart(); // preserves authored settings, including isEmitting

	bool IsEmitting() const {
		return isEmitting && !runtime.finished;
	}

	bool IsFinished() const {
		return runtime.finished && runtime.particles.empty();
	}
};

constexpr i32 MaxParticles2D = 16384;
void RegisterParticles2D(World &world);
void UpdateParticles2D(Particles2D &emitter, const glm::mat4 &worldMatrix, f64 delta);
// Number of live particles across all emitters, for profiling.
std::size_t CountLiveParticles2D(World &world);
Particles2D MakeParticlePreset(ParticlePreset preset);
f32 ParticleSize(const Particles2D &emitter, const Particle2D &particle);
Color ParticleColor(const Particles2D &emitter, const Particle2D &particle);
Rect ParticleUV(const Particles2D &emitter, const Particle2D &particle,
	bool flipVertically = false);
glm::mat4 ParticleTransform(const Particles2D &emitter, const Particle2D &particle,
	const glm::mat4 &worldMatrix);
} // namespace sowa
