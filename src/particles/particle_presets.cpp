#include "particles/particles2d.hpp"

namespace sowa {

Particles2D MakeParticlePreset(ParticlePreset preset) {
	Particles2D emitter;
	emitter.emissionShape = ParticleEmissionShape::Circle;
	emitter.emissionRadius = 10;
	switch (preset) {
	case ParticlePreset::Smoke:
		emitter.emissionRate = 24;
		emitter.lifetime = 3;
		emitter.preprocess = 2;
		emitter.initialVelocity = {15, 30};
		emitter.initialSize = {16, 24};
		emitter.sizeStart = 0.4f;
		emitter.sizeMiddle = 1.5f;
		emitter.sizeEnd = 2.5f;
		emitter.colorStart = {0.3f, 0.3f, 0.32f, 0};
		emitter.colorMiddle = {0.5f, 0.5f, 0.52f, 0.5f};
		emitter.colorEnd = {0.65f, 0.65f, 0.67f, 0};
		emitter.colorMidpoint = 0.2f;
		emitter.turbulenceStrength = 15;
		emitter.damping = 0.2f;
		emitter.angularVelocity = {-30, 30};
		break;
	case ParticlePreset::Flame:
		emitter.emissionRate = 100;
		emitter.lifetime = 0.8f;
		emitter.preprocess = 1;
		emitter.initialVelocity = {35, 70};
		emitter.initialSize = {14, 24};
		emitter.gravity = {0, 35};
		emitter.turbulenceStrength = 20;
		emitter.sizeStart = 0.8f;
		emitter.sizeMiddle = 0.6f;
		emitter.sizeEnd = 0;
		emitter.colorStart = {1, 0.95f, 0.5f, 0.9f};
		emitter.colorMiddle = {1, 0.28f, 0.025f, 0.8f};
		emitter.colorEnd = {0.6f, 0.04f, 0.01f, 0};
		emitter.blendMode = ParticleBlendMode::Additive;
		break;
	case ParticlePreset::Water:
		emitter.amount = 512;
		emitter.emissionRadius = 3;
		emitter.emissionRate = 120;
		emitter.lifetime = 2;
		emitter.initialVelocity = {130, 190};
		emitter.spread = 12;
		emitter.gravity = {0, -220};
		emitter.initialSize = {3, 6};
		emitter.sizeStart = 1;
		emitter.sizeMiddle = 1;
		emitter.sizeEnd = 0.5f;
		emitter.colorStart = {0.5f, 0.8f, 1, 0.9f};
		emitter.colorMiddle = {0.15f, 0.5f, 1, 0.8f};
		emitter.colorEnd = {0.1f, 0.4f, 1, 0};
		emitter.drawShape = ParticleDrawShape::Circle;
		emitter.alignToVelocity = true;
		emitter.velocityStretch = 0.005f;
		break;
	case ParticlePreset::Sparks:
		emitter.emissionRate = 40;
		emitter.lifetime = 1.2f;
		emitter.lifetimeRandomness = 0.5f;
		emitter.initialVelocity = {80, 180};
		emitter.spread = 45;
		emitter.gravity = {0, -120};
		emitter.initialSize = {2, 4};
		emitter.alignToVelocity = true;
		emitter.velocityStretch = 0.02f;
		emitter.colorStart = {1, 1, 0.7f, 1};
		emitter.colorMiddle = {1, 0.4f, 0.03f, 1};
		emitter.colorEnd = {1, 0.05f, 0, 0};
		emitter.blendMode = ParticleBlendMode::Additive;
		break;
	case ParticlePreset::Explosion:
		emitter.emissionRate = 0;
		emitter.burstCount = 160;
		emitter.oneShot = true;
		emitter.duration = 0.1f;
		emitter.lifetime = 1;
		emitter.lifetimeRandomness = 0.6f;
		emitter.spread = 180;
		emitter.initialVelocity = {50, 240};
		emitter.initialSize = {8, 18};
		emitter.damping = 2;
		emitter.colorStart = {1, 1, 0.6f, 1};
		emitter.colorMiddle = {1, 0.25f, 0.02f, 0.8f};
		emitter.colorEnd = {0.25f, 0.04f, 0.01f, 0};
		emitter.blendMode = ParticleBlendMode::Additive;
		break;
	case ParticlePreset::Snow:
		emitter.amount = 1024;
		emitter.emissionShape = ParticleEmissionShape::Line;
		emitter.emissionSize = {600, 0};
		emitter.emissionRate = 30;
		emitter.lifetime = 8;
		emitter.preprocess = 4;
		emitter.direction = {0, -1};
		emitter.initialVelocity = {20, 40};
		emitter.spread = 10;
		emitter.initialSize = {2, 5};
		emitter.sizeEnd = 1;
		emitter.turbulenceStrength = 8;
		emitter.colorStart = {1, 1, 1, 0};
		emitter.colorMiddle = {1, 1, 1, 0.9f};
		emitter.colorEnd = {1, 1, 1, 0};
		emitter.colorMidpoint = 0.1f;
		break;
	}
	return emitter;
}
} // namespace sowa
