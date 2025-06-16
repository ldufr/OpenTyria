#pragma once

#define static_array(T, N) struct { size_t len; T buf[N]; }

#define memcpy_literal(dst, src) memcpy(dst, src, sizeof(src) - 1)

void memcpy_u16(uint16_t *dst, const uint16_t *src, size_t count)
{
    memcpy(dst, src, count * sizeof(uint16_t));
}

void memcpy_u32(uint32_t *dst, const uint32_t *src, size_t count)
{
    memcpy(dst, src, count * sizeof(uint32_t));
}

int memcmp_u16(const uint16_t *left, const uint16_t *right, size_t count)
{
    return memcmp(left, right, count * sizeof(uint16_t));
}

bool memeq_u16(uint16_t *dst, const uint16_t *src, size_t count)
{
    return memcmp_u16(dst, src, count) == 0;
}

size_t max_size_t(size_t left, size_t right)
{
    return left < right ? right : left;
}

size_t min_size_t(size_t left, size_t right)
{
    return left < right ? left : right;
}

uint32_t max_u32(uint32_t left, uint32_t right)
{
    return left < right ? right : left;
}

int64_t max_i64(int64_t left, int64_t right)
{
    return left < right ? right : left;
}

uint32_t min_u32(uint32_t left, uint32_t right)
{
    return left < right ? left : right;
}

uint8_t cast_u8(uint32_t value)
{
    if (UINT8_MAX < value) {
        abort();
    }
    return (uint8_t) value;
}

uint16_t cast_u16(uint32_t value)
{
    if (UINT16_MAX < value) {
        abort();
    }
    return (uint16_t) value;
}

bool s16_from_ascii(uint16_t *dst, size_t size, const char *src)
{
    size_t len = strlen(src);
    if (size <= len + 1) {
        return false;
    }
    for (size_t idx = 0; idx < len; ++idx) {
        if ((src[idx] & 0x80) != 0) {
            return false;
        }
        dst[idx] = src[idx];
    }
    dst[len] = 0;
    return true;
}

bool s16_to_ascii(char *dst, size_t size, const uint16_t *src, size_t srclen)
{
    if ((size + 1) < srclen) {
        return false;
    }

    for (size_t idx = 0; idx < srclen; ++idx) {
        if (0x7F < src[idx]) {
            return false;
        }
        dst[idx] = (char)(src[idx] & 0x7F);
    }
    dst[srclen] = 0;
    return true;
}

void copy_u32_safe_or_abort(uint32_t *dst, size_t dstlen, const uint32_t *src, size_t srclen)
{
    if (dstlen < srclen) {
        abort();
    }

    memcpy_u32(dst, src, srclen);
}

float lerpf(float p1, float p2, float t)
{
    return t * p1 + (1 - t) * p2;
}

float clampf(float val, float min, float max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

uint32_t u32min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}
