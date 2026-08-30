#pragma once

#define array_npos ((size_t)(-1))

typedef struct array_void_t {
    size_t cap;
    size_t len;
    void  *ptr;
    int    tmp;
} array_void_t;

#define array(T)                \
union {                         \
    array_void_t base;          \
    struct {                    \
        size_t cap;             \
        size_t len;             \
        T     *ptr;             \
        int    tmp;             \
    };                          \
}

typedef array(char)      array_char_t;
typedef array(uint8_t)   array_uint8_t;
typedef array(uint16_t)  array_uint16_t;
typedef array(uint32_t)  array_uint32_t;
typedef array(size_t)    array_size_t;
typedef array(uintptr_t) array_uintptr_t;

#define array_init(a)           _array_init(&(a)->base)
#define array_free(a)           _array_free(&(a)->base);
#define array_reset(a)          _array_reset(&(a)->base);

#define array_reserve(a, s)     _array_reserve(&(a)->base, (s), sizeof(*(a)->ptr))
#define array_resize(a, s)      _array_resize(&(a)->base, (s), sizeof(*(a)->ptr))
#define array_shrink(a, s)      _array_shrink(&(a)->base, (s));
#define array_remove(a, i)      _array_remove(&(a)->base, (i), sizeof(*(a)->ptr))
#define array_insert(a, c, p)   _array_insert(&(a)->base, (c), (p), sizeof(*(a)->ptr))
#define array_copy(d, s)        _array_copy(&(d)->base, &(s)->base, sizeof(*(s)->ptr))
#define array_push(a, n)        (_array_push(&(a)->base, (n), sizeof(*(a)->ptr)) == 0 ? &(a)->ptr[(size_t)(a)->tmp] : NULL)

#define array_remove_ordered(a, i) _array_remove_range_ordered(&(a)->base, (i), 1, sizeof(*(a)->ptr))
#define array_remove_range_ordered(a, i, c) _array_remove_range_ordered(&(a)->base, (i), (c), sizeof(*(a)->ptr))

#define array_add(a, e)         ((array_reserve(a, 1) == 0) && (((a)->ptr[(a)->len++] = (e)), 1), (a)->tmp)
#define array_set(a, i, e)      (array_inside(a, i) && (((a)->ptr[(i)] = (e)), 1))
#define array_pop(a)            ((a)->ptr[(a)->len ? --(a)->len : 0])
#define array_at(a, i)          ((a)->ptr[_array_at_chk(&(a)->base, (i), __FILE__, __LINE__)])

#define array_front(a)          ((a)->ptr[0])
#define array_back(a)           ((a)->ptr[(a)->len - 1])
#define array_peek(a)           ((a)->ptr[(a)->len - 1])

#define array_size(a)           ((a)->len)
#define array_data(a)           ((a)->ptr)
#define array_capacity(a)       ((a)->cap)

#define array_inside(a, i)      (0 <= (i) && (size_t)(i) < (a)->len)
#define array_full(a)           ((a)->len == (a)->cap)
#define array_empty(a)          ((a)->len == 0)
#define array_clear(a)          ((void)((a)->len =  0))

#define array_begin(a)          ((a)->ptr)
#define array_end(a)            ((a)->ptr + (a)->len)

#define array_rbegin(a)         ((a)->ptr + (a)->len - 1)
#define array_rend(a)           ((a)->ptr - 1)

#define array_foreach(it, a) \
    for (it = array_begin(a); it != array_end(a); ++it)

#define array_foreach_reverse(it, a) \
    for (it = array_rbegin(a); it != array_rend(a); --it)

#if !defined(_MSC_VER)
# define array_for(a)           array_foreach(it, a)
# define array_for_reverse(a)   array_foreach_reverse(it, a)
#endif

void  _array_init(array_void_t *a);
void  _array_free(array_void_t *a);
void  _array_reset(array_void_t *a);

int   _array_resize(array_void_t *a, size_t len, const size_t elem_size);
void  _array_shrink(array_void_t *a, size_t len);
int   _array_reserve(array_void_t *a, size_t count, const size_t elem_size);
void  _array_remove(array_void_t *a, size_t index, const size_t elem_size);
int   _array_insert(array_void_t *a, size_t count, const void *ptr, const size_t elem_size);
int   _array_copy(array_void_t *dest, array_void_t *src, const size_t elem_size);
int   _array_push(array_void_t *a, size_t n, const size_t elem_size);
void  _array_remove_ordered(array_void_t *a, size_t index, const size_t elem_size);
void  _array_remove_range_ordered(array_void_t *a, size_t index, size_t count, const size_t elem_size);
size_t _array_at_chk(array_void_t *a, size_t index, const char *file, int line);
