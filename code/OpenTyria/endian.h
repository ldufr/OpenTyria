#pragma once

#ifdef _WIN32
# define bswap16(x)  _byteswap_ushort(x)
# define bswap32(x)  _byteswap_ulong(x)
# define bswap64(x)  _byteswap_uint64(x)
#else
#include <byteswap.h>
# define bswap16(x)  bswap_16(x)
# define bswap32(x)  bswap_32(x)
# define bswap64(x)  bswap_64(x)
#endif

/*
 * Host to big endian, host to little endian, big endian to host, and little
 * endian to host byte order functions.
 */
#ifndef htobe16
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define htobe16(x)  bswap16((x))
#define htobe32(x)  bswap32((x))
#define htobe64(x)  bswap64((x))
#define htole16(x)  ((uint16_t)(x))
#define htole32(x)  ((uint32_t)(x))
#define htole64(x)  ((uint64_t)(x))

#define be16toh(x)  bswap16((x))
#define be32toh(x)  bswap32((x))
#define be64toh(x)  bswap64((x))
#define le16toh(x)  ((uint16_t)(x))
#define le32toh(x)  ((uint32_t)(x))
#define le64toh(x)  ((uint64_t)(x))
#else /* _BYTE_ORDER != _LITTLE_ENDIAN */
#define htobe16(x)  ((uint16_t)(x))
#define htobe32(x)  ((uint32_t)(x))
#define htobe64(x)  ((uint64_t)(x))
#define htole16(x)  bswap16((x))
#define htole32(x)  bswap32((x))
#define htole64(x)  bswap64((x))

#define be16toh(x)  ((uint16_t)(x))
#define be32toh(x)  ((uint32_t)(x))
#define be64toh(x)  ((uint64_t)(x))
#define le16toh(x)  bswap16((x))
#define le32toh(x)  bswap32((x))
#define le64toh(x)  bswap64((x))
#endif /* _BYTE_ORDER == _LITTLE_ENDIAN */
#endif

uint16_t be16dec(const void *pp)
{
    const uint8_t *p = (const uint8_t *)pp;

    return ((p[0] << 8) | p[1]);
}

uint32_t be32dec(const void *pp)
{
    const uint8_t *p = (const uint8_t *)pp;

    return (((unsigned)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

uint64_t be64dec(const void *pp)
{
    const uint8_t *p = (const uint8_t *)pp;

    return (((uint64_t)be32dec(p) << 32) | be32dec(p + 4));
}

uint16_t le16dec(const void *pp)
{
    const uint8_t *p = (const uint8_t *)pp;

    return ((p[1] << 8) | p[0]);
}

uint32_t le32dec(const void *pp)
{
    const uint8_t *p = (const uint8_t *)pp;

    return (((unsigned)p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

float le32decf(const void *pp)
{
    uint32_t num = le32dec(pp);
    float res;
    memcpy(&res, &num, sizeof(res));
    return res;
}

uint64_t le64dec(const void *pp)
{
    const uint8_t *p = (const uint8_t *)pp;

    return (((uint64_t)le32dec(p + 4) << 32) | le32dec(p));
}

void be16enc(void *pp, uint16_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = (u >> 8) & 0xff;
    p[1] = u & 0xff;
}

void be32enc(void *pp, uint32_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = (u >> 24) & 0xff;
    p[1] = (u >> 16) & 0xff;
    p[2] = (u >> 8) & 0xff;
    p[3] = u & 0xff;
}

void be64enc(void *pp, uint64_t u)
{
    uint8_t *p = (uint8_t *)pp;

    be32enc(p, (uint32_t)(u >> 32));
    be32enc(p + 4, (uint32_t)(u & 0xffffffff));
}

void le16enc(void *pp, uint16_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = u & 0xff;
    p[1] = (u >> 8) & 0xff;
}

void le32enc(void *pp, uint32_t u)
{
    uint8_t *p = (uint8_t *)pp;

    p[0] = u & 0xff;
    p[1] = (u >> 8) & 0xff;
    p[2] = (u >> 16) & 0xff;
    p[3] = (u >> 24) & 0xff;
}

void le64enc(void *pp, uint64_t u)
{
    uint8_t *p = (uint8_t *)pp;

    le32enc(p, (uint32_t)(u & 0xffffffff));
    le32enc(p + 4, (uint32_t)(u >> 32));
}
