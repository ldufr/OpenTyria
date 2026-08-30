#pragma once

#ifndef ARRAY_MIN_cap
# define ARRAY_MIN_cap 8
#endif

#define ARRAY_ALLOC_ZERO 1

size_t array_max(size_t left, size_t right)
{
    return right < left ? left : right;
}

void *msdn_realloc(void *addr, size_t size)
{
    if (addr != NULL) {
        return realloc(addr, size);
    } else {
        return malloc(size);
    }
}

void _array_init(array_void_t *array)
{
    assert(array);
    array->cap = 0;
    array->len = 0;
    array->ptr = NULL;
}

void _array_free(array_void_t *array)
{
    array->len = 0;
    array->cap = 0;
    if (array->ptr) {
        free(array->ptr);
    }
    array->ptr = NULL;
}

void _array_reset(array_void_t *array)
{
    _array_free(array);
    _array_init(array);
}

int _array_grow_to(array_void_t *array, size_t new_cap, const size_t elem_size)
{
    assert(array && elem_size != 0);

    new_cap = array_max(new_cap, ARRAY_MIN_cap);
    size_t new_size = new_cap * elem_size;
    assert((new_size / elem_size) == new_cap);

    void *ptr;
    if ((ptr = msdn_realloc(array->ptr, new_size)) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    array->ptr = ptr;
#ifdef ARRAY_ALLOC_ZERO
    if (array->cap <= new_cap) {
        char *b = (char *)array->ptr + (array->cap * elem_size);
        memset(b, 0, (new_cap - array->cap) * elem_size);
    }
#endif

    array->len = (array->len > new_cap) ? new_cap : array->len;
    array->cap = new_cap;
    return ERR_OK;
}

int _array_resize(array_void_t *array, size_t size, const size_t elem_size)
{
    assert(array && elem_size != 0);

    int err;
    if ((array->cap < size) && (err = _array_reserve(array, size, elem_size)) != 0) {
        return err;
    }

    array->len = size;
    return ERR_OK;
}

void _array_shrink(array_void_t *array, size_t size)
{
    assert(size <= array->len);
    array->len = size;
}

int _array_reserve(array_void_t *array, size_t count, const size_t elem_size)
{
    assert(array && elem_size != 0);
    if (array->cap - array->len < count) {
        size_t new_cap = array->cap * 2;
        if (new_cap < array->len + count)
            new_cap = array->len + count;
        array->tmp = _array_grow_to(array, new_cap, elem_size);
        return array->tmp;
    }

    array->tmp = ERR_OK;
    return array->tmp;
}

void _array_remove(array_void_t *array, size_t index, const size_t elem_size)
{
    assert(array && elem_size != 0);

    if (array->len <= index)
        return;

    size_t new_size = array->len - 1;
    if (index < new_size) {
        // if it's not the last element, we swap with the last.
        char *dst = (char *)array->ptr + (index * elem_size);
        char *src = (char *)array->ptr + (new_size * elem_size);
        memcpy(dst, src, elem_size);
    }
    array->len = new_size;
}

int _array_insert(array_void_t *array, size_t count, const void *ptr, const size_t elem_size)
{
    assert(array && elem_size != 0);
    if (!count) {
        return ERR_OK;
    }

    int err;
    if ((err = _array_reserve(array, count, elem_size)) != 0)
        return err;
    assert(array->len + count <= array->cap);

    char *buff = (char *)array->ptr + (array->len * elem_size);
    memcpy(buff, ptr, count * elem_size);
    array->len += count;
    return ERR_OK;
}

int _array_copy(array_void_t *dest, array_void_t *src, const size_t elem_size)
{
    array_clear(dest);
    return _array_insert(dest, src->len, src->ptr, elem_size);
}

int _array_push(array_void_t *array, size_t n, const size_t elem_size)
{
    int err;
    if ((err = _array_reserve(array, n, elem_size)) != 0)
        return err;
    array->tmp = (int)array->len;
    array->len += n;
    return ERR_OK;
}

void _array_remove_ordered(array_void_t *array, size_t i, const size_t elem_size)
{
    assert(array && elem_size != 0);
    if (array->len <= i)
        return;
    size_t rem = array->len - i;
    if (rem) {
        char *ptr = (char *)array->ptr + (i * elem_size);
        memmove(ptr, ptr + elem_size, rem * elem_size);
    }
    array->len -= 1;
}

void _array_remove_range_ordered(array_void_t *array, size_t index, size_t count, const size_t elem_size)
{
    assert(array && elem_size != 0);
    if (array->len <= index)
        return;
    if (array->len < (index + count))
        count = array->len - index;
    size_t rem = array->len - count;
    if (rem) {
        char *ptr = (char *)array->ptr + (index * elem_size);
        memmove(ptr, ptr + (count * elem_size), rem * elem_size);
    }
    array->len -= count;
}

size_t _array_at_chk(array_void_t *a, size_t index, const char *file, int line)
{
    if (!(index < a->len)) {
        fprintf(stderr, "Assertion failed: index (%zu) < a->len (%zu), file %s, line %d\n", index, a->len, file, line);
        abort();
    }
    return index;
}
