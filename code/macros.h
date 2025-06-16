#pragma once

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(expr) ((void)expr)
#endif

#define CAST_STRUCT_FROM_MEMBER(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define indexof(base, offset) (size_t)(offset - base)

#define ROL16(x, n) ((x << n) | ((x & 0xFFFF)     >> (16 - n)))
#define ROL32(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define ALIGN(v, a) (((v) + (a - 1)) & ~(a - 1))
#define ALIGN16(v)  ((v + 1) & ~1)
#define ALIGN32(v)  ((v + 3) & ~3)

#define STATIC_ASSERT(expr) static_assert(expr, #expr)
