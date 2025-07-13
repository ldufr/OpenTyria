#pragma once

typedef struct slice_void_t {
    size_t       len;
    const void  *ptr;
} slice_void_t;

#define slice(T)           \
union {                    \
    slice_void_t base;     \
    struct {               \
        size_t   len;      \
        const T *ptr;      \
    };                     \
}

typedef slice(char)      slice_char_t;
typedef slice(uint8_t)   slice_uint8_t;
typedef slice(uint16_t)  slice_uint16_t;
typedef slice(uint32_t)  slice_uint32_t;
typedef slice(size_t)    slice_size_t;
typedef slice(uintptr_t) slice_uintptr_t;

#define slice_advance(slice, count) _slice_advance(&(slice)->base, count, sizeof(*(slice)->ptr))

void _slice_advance(slice_void_t *slice, size_t count, size_t elem_size)
{
    slice->ptr = (const char *) slice->ptr + (count * elem_size);
    slice->len -= count;
}

typedef struct slice_mut_void_t {
    size_t len;
    void  *ptr;
} slice_mut_void_t;

#define slice_mut(T)       \
union {                    \
    slice_mut_void_t base; \
    struct {               \
        size_t  len;       \
        T      *ptr;       \
    };                     \
}

typedef slice_mut(char)      slice_mut_char_t;
typedef slice_mut(uint8_t)   slice_mut_uint8_t;
typedef slice_mut(uint16_t)  slice_mut_uint16_t;
typedef slice_mut(uint32_t)  slice_mut_uint32_t;
typedef slice_mut(size_t)    slice_mut_size_t;
typedef slice_mut(uintptr_t) slice_mut_uintptr_t;

#define slice_u8_from_lit(lit) { .len = sizeof(lit) - 1, .ptr = (const uint8_t *)lit }
