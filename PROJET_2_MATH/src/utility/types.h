#pragma once

#include <stdint.h>
#include <stdio.h>

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

#define Kilobytes(Amount) ((Amount) * 1024)
#define Megabytes(Amount) (Kilobytes(Amount) * 1024)
#define Gigabytes(Amount) (Megabytes(Amount) * 1024)

#define SCAST(Type, From) static_cast<Type>(From)
#define RCAST(Type, From) reinterpret_cast<Type>(From)
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#include <cstdio>
#ifdef _DEBUG
#define ASSERT(condition, message, ...)                                                   \
    do {                                                                                    \
        if (!(condition)) {                                                                 \
            fprintf(stderr, "Assertion failed: (%s), function %s, file %s, line %d.\n",     \
                    #condition, __FUNCTION__, __FILE__, __LINE__);                          \
            fprintf(stderr, "Message: " message "\n", ##__VA_ARGS__);                        \
            __debugbreak();                                                                 \
        }                                                                                   \
    } while (0)
#else
#define ASSERT(condition, message, ...) do { } while (0)
#endif

#define SAFE_RELEASE(x) { if(x) { (x)->Release(); (x) = nullptr; } }

constexpr auto MAX_VECTORS = 100;
constexpr auto SHARED_OBJECT_DATA_SLOT = 0;
constexpr auto OBJECT_DATA_SLOT = 1;
constexpr auto INSTANCE_DATA_SLOT = 0;
constexpr auto MAX_OBJECTS = 1000;
