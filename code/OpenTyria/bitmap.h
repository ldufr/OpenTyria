#pragma once

#define BITS_PER_UINT32 (sizeof(uint32_t) * 8)
#define Bitmap(nr) \
    struct { uint32_t bitmap[((nr + BITS_PER_UINT32 - 1) / BITS_PER_UINT32)]; }

typedef Bitmap(256)  Bitmap256;
typedef Bitmap(1024) Bitmap1024;

#define bitmap_set_bit(bm, bp)   _bitmap_set_bit((bm).bitmap, sizeof((bm).bitmap), (bp))
#define bitmap_clear_bit(bm, bp) _bitmap_clear_bit((bm).bitmap, sizeof((bm).bitmap), (bp))
#define bitmap_test_bit(bm, bp)  _bitmap_test_bit((bm).bitmap, sizeof((bm).bitmap), (bp))
#define bitmap_clear(bm)         _bitmap_clear((bm).bitmap, sizeof((bm).bitmap))
#define bitmap_length(bm)        _bitmap_length((bm).bitmap, sizeof((bm).bitmap))

#define bitmap_write_u32(buf, size, bm) _bitmap_write_u32(buf, size, bm, sizeof((bm).bitmap))

void _bitmap_set_bit(uint32_t *bitmap, size_t length, int pos)
{
    size_t idx = pos / BITS_PER_UINT32;
    pos = pos & (BITS_PER_UINT32 - 1);
    assert(idx < length);
    bitmap[idx] = 1 << pos;
}

void _bitmap_clear_bit(uint32_t *bitmap, size_t length, int pos)
{
    size_t idx = pos / BITS_PER_UINT32;
    pos = pos & (BITS_PER_UINT32 - 1);
    if (idx < length) {
        bitmap[idx] = ~(1 << pos);
    }
}

bool _bitmap_test_bit(const uint32_t *bitmap, size_t length, int pos)
{
    size_t idx = pos / BITS_PER_UINT32;
    pos = pos & (BITS_PER_UINT32 - 1);
    if (idx < length) {
        return (bitmap[idx] & (1 << pos)) != 0;
    } else {
        return false;
    }
}

void _bitmap_clear(uint32_t *bitmap, size_t length)
{
    memset(bitmap, 0, length * sizeof(*bitmap));
}

size_t _bitmap_bitlen(uint32_t val)
{
    uint32_t mask = 1;
    for (size_t idx = 0; idx < BITS_PER_UINT32; ++idx) {
        if ((val & mask) != 0) {
            return idx;
        }
        mask <<= 1;
    }
    return 0;
}

size_t _bitmap_length(const uint32_t *bitmap, size_t length)
{
    for (size_t idx = length - 1; idx < length; ++idx) {
        if (bitmap[idx] != 0) {
            return (idx * BITS_PER_UINT32) + _bitmap_bitlen(bitmap[idx]);
        }
    }
    return 0;
}

void _bitmap_write_u32(uint32_t *buffer, size_t size, const uint32_t *bitmap, size_t length)
{
    assert(size <= length);
    memcpy_u32(buffer, bitmap, _bitmap_length(bitmap, length));
}
