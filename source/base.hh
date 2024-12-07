#pragma once

#include <stdint.h>

using f32 = float;
using f64 = double;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using usize = size_t;

#if defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__gnu_linux__)
#define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_MAC 1
#else
#error "Unsupported OS"
#endif

#if defined(__cpp_lib_modules) && defined(USE_IMPORT_STD)
#define IMPORT_STD
#endif
