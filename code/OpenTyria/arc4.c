#pragma once

void arc4_init(arc4_context *ctx)
{
    memset(ctx, 0, sizeof(arc4_context));
}

void arc4_free(arc4_context *ctx)
{
    memset(ctx, 0, sizeof(arc4_context));
}

void arc4_setup(arc4_context *ctx, const uint8_t *key, size_t keylen)
{
    int idx, j, a;
    unsigned int k;
    uint8_t *m;

    ctx->x = 0;
    ctx->y = 0;
    m = ctx->m;

    for (idx = 0; idx < 256; idx++ )
        m[idx] = (uint8_t)idx;

    j = k = 0;

    for (idx = 0; idx < 256; idx++, k++)
    {
        if(k >= keylen) {
            k = 0;
        }

        a = m[idx];
        j = (j + a + key[k]) & 0xFF;
        m[idx] = m[j];
        m[j] = (uint8_t)a;
    }
}

void arc4_crypt(arc4_context *ctx, const uint8_t *input, uint8_t *output, size_t length)
{
    int x, y, a, b;
    size_t idx;
    uint8_t *m;

    x = ctx->x;
    y = ctx->y;
    m = ctx->m;

    for (idx = 0; idx < length; idx++)
    {
        x = (x + 1) & 0xFF; a = m[x];
        y = (y + a) & 0xFF; b = m[y];

        m[x] = (uint8_t)b;
        m[y] = (uint8_t)a;

        output[idx] = (uint8_t)(input[idx] ^ m[(uint8_t)(a + b)]);
    }

    ctx->x = x;
    ctx->y = y;
}

void arc4_crypt_inplace(arc4_context *ctx, uint8_t *buffer, size_t length)
{
    int x, y, a, b;
    size_t idx;
    uint8_t *m;

    x = ctx->x;
    y = ctx->y;
    m = ctx->m;

    for (idx = 0; idx < length; idx++)
    {
        x = (x + 1) & 0xFF; a = m[x];
        y = (y + a) & 0xFF; b = m[y];

        m[x] = (uint8_t)b;
        m[y] = (uint8_t)a;

        buffer[idx] = (uint8_t)(buffer[idx] ^ m[(uint8_t)(a + b)]);
    }

    ctx->x = x;
    ctx->y = y;
}
