#pragma once

namespace sowa {

enum class EcsError {
	Ok,
	// Duplicate registration is rejected, including identical layouts.
	DuplicateName,
	// ReplaceDataDescribedFields was asked to reshape a native component,
	// or a type that was never data-described.
	NotDataDescribed,
	// Structural ECS work (layout replace) from inside a system / Progress.
	Deferred,
};

template <typename T> struct EcsResult {
	T value{};
	EcsError error = EcsError::Ok;

	explicit operator bool() const {
		return error == EcsError::Ok;
	}
};

} // namespace sowa
