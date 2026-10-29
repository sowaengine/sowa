#include "animation/animation.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

#include "core/log.hpp"
#include "core/serialize/archive.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {
namespace {
const std::vector<EnumConstant> types = {{"Bool", 0}, {"I32", 1}, {"U32", 2}, {"F32", 3},
	{"F64", 4}, {"String", 5}, {"EntityRef", 6}, {"Vector2", 7}, {"Vector3", 8}, {"Color", 9},
	{"Enum", 10}, {"ResourceRef", 11}};
const std::vector<EnumConstant> interpolations = {{"Step", 0}, {"Linear", 1}, {"Smooth", 2},
	{"AngleDegrees", 3}};
} // namespace

AnimationValue DefaultAnimationValue(FieldType type) {
	switch (type) {
	case FieldType::Bool:
		return false;
	case FieldType::I32:
	case FieldType::Enum:
		return i32(0);
	case FieldType::U32:
		return u32(0);
	case FieldType::F32:
		return f32(0);
	case FieldType::F64:
		return f64(0);
	case FieldType::String:
	case FieldType::EntityRef:
		return std::string();
	case FieldType::Vector2:
		return Vector2(0, 0);
	case FieldType::Vector3:
		return Vector3(0, 0, 0);
	case FieldType::Color:
		return Color();
	case FieldType::ResourceRef:
		return ResourceID();
	}
	return 0.f;
}

namespace {
bool finiteValue(const AnimationValue &value) {
	return std::visit(
		[](const auto &v) {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_floating_point_v<T>) {
				return std::isfinite(v);
			} else if constexpr (std::is_same_v<T, Vector2>) {
				return std::isfinite(v.x) && std::isfinite(v.y);
			} else if constexpr (std::is_same_v<T, Vector3>) {
				return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
			} else if constexpr (std::is_same_v<T, Color>) {
				return std::isfinite(v.r) && std::isfinite(v.g) && std::isfinite(v.b) &&
					   std::isfinite(v.a);
			} else {
				return true;
			}
		},
		value);
}
} // namespace

bool AnimationValueMatches(FieldType type, const AnimationValue &value) {
	return static_cast<int>(type) >= 0 &&
		   static_cast<int>(type) <= static_cast<int>(FieldType::ResourceRef) &&
		   value.index() == DefaultAnimationValue(type).index() && finiteValue(value);
}

AnimationValue InterpolateAnimationValues(const AnimationValue &a, const AnimationValue &b,
	f64 weight, KeyInterpolation interpolation) {
	if (a.index() != b.index()) {
		return b;
	}
	if (std::isnan(weight)) {
		return a;
	}
	weight = std::clamp(weight, 0., 1.);
	if (interpolation == KeyInterpolation::Step) {
		return weight < 1 ? a : b;
	}
	if (interpolation == KeyInterpolation::Smooth) {
		weight = weight * weight * (3 - 2 * weight);
	}
	auto lerp = [&](f64 x, f64 y) { return x + (y - x) * weight; };
	return std::visit(
		[&](const auto &x) -> AnimationValue {
			using T = std::decay_t<decltype(x)>;
			const auto &y = std::get<T>(b);
			if constexpr (std::is_floating_point_v<T>) {
				if (interpolation == KeyInterpolation::AngleDegrees) {
					return T(x + std::remainder(f64(y) - x, 360.) * weight);
				}
				return T(lerp(x, y));
			} else if constexpr (std::is_same_v<T, i32> || std::is_same_v<T, u32>) {
				return T(std::round(lerp(x, y)));
			} else if constexpr (std::is_same_v<T, Vector2>) {
				return Vector2(lerp(x.x, y.x), lerp(x.y, y.y));
			} else if constexpr (std::is_same_v<T, Vector3>) {
				return Vector3(lerp(x.x, y.x), lerp(x.y, y.y), lerp(x.z, y.z));
			} else if constexpr (std::is_same_v<T, Color>) {
				return Color(lerp(x.r, y.r), lerp(x.g, y.g), lerp(x.b, y.b), lerp(x.a, y.a));
			} else {
				return weight < .5 ? a : b;
			}
		},
		a);
}

u64 AnimationTrack::Insert(f64 time, AnimationValue value, KeyInterpolation interpolation) {
	if (!std::isfinite(time) || time < 0 || !AnimationValueMatches(binding.type, value)) {
		return 0;
	}
	for (auto &key : _keys) {
		if (key.time == time) {
			key.value = std::move(value);
			key.interpolation = interpolation;
			return key.id;
		}
	}
	const u64 id = _nextId++;
	_keys.push_back({id, time, std::move(value), interpolation});
	std::sort(_keys.begin(), _keys.end(), [](auto &a, auto &b) { return a.time < b.time; });
	return id;
}

bool AnimationTrack::Update(u64 id, f64 time, AnimationValue value,
	KeyInterpolation interpolation) {
	if (!std::isfinite(time) || time < 0 || !AnimationValueMatches(binding.type, value)) {
		return false;
	}
	for (auto &key : _keys) {
		if (key.id != id && key.time == time) {
			return false;
		}
	}
	for (auto &key : _keys) {
		if (key.id == id) {
			key.time = time;
			key.value = std::move(value);
			key.interpolation = interpolation;
			std::sort(_keys.begin(), _keys.end(), [](auto &a, auto &b) { return a.time < b.time; });
			return true;
		}
	}
	return false;
}

bool AnimationTrack::Remove(u64 id) {
	return std::erase_if(_keys, [&](auto &k) { return k.id == id; }) != 0;
}

std::optional<AnimationValue> AnimationTrack::Sample(f64 time) const {
	if (_keys.empty() || !std::isfinite(time)) {
		return {};
	}
	auto right = std::upper_bound(_keys.begin(), _keys.end(), time,
		[](f64 t, const auto &k) { return t < k.time; });
	if (right == _keys.begin()) {
		return right->value;
	}
	const auto &left = *std::prev(right);
	if (right == _keys.end()) {
		return left.value;
	}
	auto mode = left.interpolation;
	if (binding.type == FieldType::Bool || binding.type == FieldType::String ||
		binding.type == FieldType::Enum || binding.type == FieldType::EntityRef ||
		binding.type == FieldType::ResourceRef) {
		mode = KeyInterpolation::Step;
	}
	return InterpolateAnimationValues(left.value, right->value,
		(time - left.time) / (right->time - left.time), mode);
}

void AnimationTrack::Serialize(Archive &a) {
	a.Field("path", binding.path);
	a.Field("component", binding.component);
	a.Field("property", binding.property);
	a.EnumField("type", binding.type, types);
	auto keys = _keys;
	a.List("keys", keys, [&](Archive &k, AnimationKey &key) {
		k.Field("time", key.time);
		k.EnumField("interpolation", key.interpolation, interpolations);
		if (k.IsLoading()) {
			key.value = DefaultAnimationValue(binding.type);
		}
		std::visit(
			[&](auto &value) {
				using T = std::decay_t<decltype(value)>;
				if constexpr (std::is_same_v<T, ResourceID>) {
					k.ResourceField("value", value.value);
				} else {
					k.Field("value", value);
				}
			},
			key.value);
	});
	if (a.IsLoading()) {
		_keys.clear();
		_nextId = 1;
		for (auto &k : keys) {
			if (!Insert(k.time, k.value, k.interpolation)) {
				a.AddError("keys", "invalid key time or value");
			}
		}
	}
}

const AnimationClip *AnimationLibrary::FindClip(const std::string &name) const {
	for (auto &clip : clips) {
		if (clip.name == name) {
			return &clip;
		}
	}
	return nullptr;
}

void AnimationLibrary::Serialize(Archive &a) {
	a.List("clips", clips, [](Archive &c, AnimationClip &clip) {
		c.Field("name", clip.name);
		c.Field("duration", clip.duration);
		c.Field("loop", clip.loop);
		c.List("tracks", clip.tracks,
			[](Archive &t, AnimationTrack &track) { track.Serialize(t); });
		c.List("calls", clip.calls, [](Archive &k, AnimationCall &call) {
			k.Field("time", call.time);
			k.Field("path", call.path);
			k.Field("component", call.component);
			k.Field("method", call.method);
		});
		if (!std::isfinite(clip.duration) || clip.duration <= 0) {
			c.AddError("duration", "must be positive and finite");
			clip.duration = 1;
		}
	});
}

} // namespace sowa
