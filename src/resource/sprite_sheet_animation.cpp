#include "resource/sprite_sheet_animation.hpp"

#include <algorithm>
#include <cmath>

#include "core/log.hpp"
#include "core/serialize/archive.hpp"

namespace sowa {

f32 SpriteAnimation::Duration() const {
	if (!std::isfinite(framesPerSecond) || framesPerSecond <= 0.f || frames.empty()) {
		return 0.f;
	}
	return static_cast<f32>(frames.size()) / framesPerSecond;
}

Rect SpriteAnimation::CellRect(i32 cell, Vector2 textureSize) const {
	if (columns <= 0 || rows <= 0 || paddingX < 0 || paddingY < 0 || spacingX < 0 || spacingY < 0 ||
		cell < 0 || static_cast<i64>(cell) >= static_cast<i64>(columns) * rows ||
		!std::isfinite(textureSize.x) || !std::isfinite(textureSize.y)) {
		return {};
	}
	const f32 width = (textureSize.x - 2. * paddingX - double(columns - 1) * spacingX) / columns;
	const f32 height = (textureSize.y - 2. * paddingY - double(rows - 1) * spacingY) / rows;
	if (width <= 0 || height <= 0) {
		return {};
	}
	return {paddingX + (cell % columns) * (width + spacingX),
		paddingY + (cell / columns) * (height + spacingY), width, height};
}

Rect SpriteAnimation::FrameUV(i32 index, Vector2 textureSize, bool flipVertically) const {
	if (index < 0 || index >= FrameCount()) {
		return {};
	}
	const Rect cell = CellRect(frames[index], textureSize);
	if (cell.w <= 0 || cell.h <= 0) {
		return {};
	}
	const f32 x = cell.x / textureSize.x;
	const f32 y = cell.y / textureSize.y;
	const f32 w = cell.w / textureSize.x;
	const f32 h = cell.h / textureSize.y;
	return flipVertically ? Rect(x, 1.f - y - h, w, h) : Rect(x, y + h, w, -h);
}

bool SpriteAnimation::IsValid() const {
	return texture.IsValid() && columns > 0 && rows > 0 && paddingX >= 0 && paddingY >= 0 &&
		   spacingX >= 0 && spacingY >= 0 && !frames.empty() &&
		   std::all_of(frames.begin(), frames.end(), [&](i32 cell) {
			   return cell >= 0 && static_cast<i64>(cell) < static_cast<i64>(columns) * rows;
		   });
}

SpriteAnimation *SpriteSheetAnimation::Find(const std::string &name) {
	const auto it = _animations.find(name);
	return it != _animations.end() ? &it->second : nullptr;
}

const SpriteAnimation *SpriteSheetAnimation::Find(const std::string &name) const {
	const auto it = _animations.find(name);
	return it != _animations.end() ? &it->second : nullptr;
}

bool SpriteSheetAnimation::Set(const std::string &name, const SpriteAnimation &animation) {
	if (name.empty()) {
		Log::Error("SpriteSheetAnimation: an animation needs a name");
		return false;
	}

	_animations[name] = animation;
	return true;
}

bool SpriteSheetAnimation::Remove(const std::string &name) {
	const auto it = _animations.find(name);
	if (it == _animations.end()) {
		return false;
	}

	_animations.erase(it);
	return true;
}

bool SpriteSheetAnimation::Has(const std::string &name) const {
	return _animations.find(name) != _animations.end();
}

void SpriteSheetAnimation::Clear() {
	_animations.clear();
}

std::vector<std::string> SpriteSheetAnimation::Names() const {
	std::vector<std::string> names;
	names.reserve(_animations.size());
	for (const auto &pair : _animations) {
		names.push_back(pair.first);
	}
	return names;
}

void SpriteSheetAnimation::Serialize(Archive &archive) {
	// Use a list of named records so one Archive::List callback handles saving and loading.
	struct Entry {
		std::string name;
		SpriteAnimation animation;
	};

	std::vector<Entry> entries;
	if (archive.IsSaving()) {
		entries.reserve(_animations.size());
		for (const auto &pair : _animations) {
			entries.push_back(Entry{pair.first, pair.second});
		}
	}

	archive.List("animations", entries, [](Archive &element, Entry &entry) {
		element.Field("name", entry.name);
		element.Field("texture", entry.animation.texture.value);
		element.Field("columns", entry.animation.columns);
		element.Field("rows", entry.animation.rows);
		// Older scenes used the same padding and spacing on both axes.
		if (element.IsLoading()) {
			i32 padding = 0, spacing = 0;
			element.Field("padding", padding);
			element.Field("spacing", spacing);
			entry.animation.paddingX = entry.animation.paddingY = padding;
			entry.animation.spacingX = entry.animation.spacingY = spacing;
		}
		element.Field("paddingX", entry.animation.paddingX);
		element.Field("paddingY", entry.animation.paddingY);
		element.Field("spacingX", entry.animation.spacingX);
		element.Field("spacingY", entry.animation.spacingY);
		element.Field("framesPerSecond", entry.animation.framesPerSecond);
		element.Field("loop", entry.animation.loop);
		element.List("frames", entry.animation.frames,
			[](Archive &frame, i32 &cell) { frame.Value(cell); });
	});

	if (archive.IsLoading()) {
		_animations.clear();
		for (Entry &entry : entries) {
			// Set refuses an empty name, so a malformed record is dropped
			// rather than becoming an animation nothing can look up.
			if (!Set(entry.name, entry.animation)) {
				archive.AddError("animations", "an animation record has no name");
			}
		}
	}
}

} // namespace sowa
