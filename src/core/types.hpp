#pragma once

#include <cstdint>
#include <memory>

namespace sowa {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

template <typename T> using Ref = std::shared_ptr<T>;

template <typename T, typename... Args> Ref<T> MakeRef(Args &&...args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T> using Scope = std::unique_ptr<T>;

template <typename T, typename... Args> Scope<T> MakeScope(Args &&...args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace sowa
