#pragma once

#define UUID_NODE_LEN   6
#define UUID_STRING_LEN 36

typedef struct GmUuid {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t  clock_seq_hi_and_reserved;
    uint8_t  clock_seq_low;
    uint8_t  node[UUID_NODE_LEN];
} GmUuid;

static const GmUuid null_uuid = {0};

#define uuid_equals(a, b) (uuid_cmp(a, b) == 0)

bool uuid_is_null(const GmUuid *u)
{
    return !memcmp(u, &null_uuid, sizeof(*u));
}

int uuid_cmp(const GmUuid *a, const GmUuid *b)
{
    if (a == b) return 1;
    if (a == NULL) return uuid_is_null(b);
    if (b == NULL) return uuid_is_null(a);
    return memcmp(a, b, sizeof(GmUuid));
}

void uuid_copy(GmUuid *dest, const GmUuid *src)
{
    if (!dest || (dest == src))
        return;
    if (src) {
        memcpy(dest, src, sizeof(GmUuid));
    } else {
        memcpy(dest, &null_uuid, sizeof(GmUuid));
    }
}

void uuid_clear(GmUuid *u)
{
    if (!u) return;
    memset(u, 0, sizeof(*u));
}

uint16_t uuid_hash(const GmUuid *u)
{
    return (u ? (u->time_low & 0xffff) : 0);
}

void uuid_fprint(FILE *stream, const GmUuid *u)
{
    u = u ? u : &null_uuid;
    fprintf(stream, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        u->time_low, u->time_mid, u->time_hi_and_version,
        u->clock_seq_hi_and_reserved, u->clock_seq_low, u->node[0],
        u->node[1], u->node[2], u->node[3], u->node[4], u->node[5]);
}

void uuid_snprint(char *s, size_t n, const GmUuid *u)
{
    u = u ? u : &null_uuid;
    snprintf(s, n, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        u->time_low, u->time_mid, u->time_hi_and_version,
        u->clock_seq_hi_and_reserved, u->clock_seq_low, u->node[0],
        u->node[1], u->node[2], u->node[3], u->node[4], u->node[5]);
}

bool uuid_parse(GmUuid *u, const char *str, size_t len)
{
    // This check means that the `sscanf` is safe.
    if (len != sizeof("AABBCCDD-AABB-AABB-AABB-AABBCCDDEEFF") - 1) {
        return false;
    }
    int ret = sscanf(
        str,
        "%08" SCNx32 "-%04" SCNx16 "-%04" SCNx16 "-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        &u->time_low, &u->time_mid, &u->time_hi_and_version,
        &u->clock_seq_hi_and_reserved, &u->clock_seq_low, &u->node[0],
        &u->node[1], &u->node[2], &u->node[3], &u->node[4], &u->node[5]);
    return ret == 11;
}

void uuid_enc_le(void *buf, const GmUuid *u)
{
    uint8_t *b = (uint8_t *)buf;
    le32enc(b + 0, u->time_low);
    le16enc(b + 4, u->time_mid);
    le16enc(b + 6, u->time_hi_and_version);
    b[8] = u->clock_seq_hi_and_reserved;
    b[9] = u->clock_seq_low;
    for (int i = 0; i < UUID_NODE_LEN; ++i)
        b[10 + i] = u->node[i];
}

void uuid_dec_le(GmUuid *u, const void *buf)
{
    const uint8_t *b = (const uint8_t *)buf;
    u->time_low = le32dec(b);
    u->time_mid = le16dec(b + 4);
    u->time_hi_and_version = le16dec(b + 6);
    u->clock_seq_hi_and_reserved = b[8];
    u->clock_seq_low = b[9];
    for (int i = 0; i < UUID_NODE_LEN; ++i)
        u->node[i] = b[10 + i];
}

void uuid_enc_be(void *buf, const GmUuid *u)
{
    uint8_t *b = (uint8_t *)buf;
    be32enc(b + 0, u->time_low);
    be16enc(b + 4, u->time_mid);
    be16enc(b + 6, u->time_hi_and_version);
    b[8] = u->clock_seq_hi_and_reserved;
    b[9] = u->clock_seq_low;
    for (int i = 0; i < UUID_NODE_LEN; ++i)
        b[10 + i] = u->node[i];
}

void uuid_dec_be(GmUuid *u, const void *buf)
{
    const uint8_t *b = (const uint8_t *)buf;
    u->time_low = be32dec(b);
    u->time_mid = be16dec(b + 4);
    u->time_hi_and_version = be16dec(b + 6);
    u->clock_seq_hi_and_reserved = b[8];
    u->clock_seq_low = b[9];
    for (int i = 0; i < UUID_NODE_LEN; ++i)
        u->node[i] = b[10 + i];
}
