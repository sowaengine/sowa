#ifndef SW_SOWA_HXX
#define SW_SOWA_HXX
#pragma once

#include <cstdint>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

template <typename T>
using ptr = T *;

template <typename T>
using ref = T &;

template <typename T>
using cptr = const T *;

template <typename T>
using cref = const T &;

#endif // SW_SOWA_HXX