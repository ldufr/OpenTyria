#pragma once

void random_init(mbedtls_chacha20_context *ctx, const uint8_t key[32])
{
    mbedtls_chacha20_init(ctx);
    if (mbedtls_chacha20_setkey(ctx, key) != 0 ||
        mbedtls_chacha20_starts(ctx, (const uint8_t *)"OpenTyria-Srv", 0)) {
        // This isn't possible, the signature is only to support a common interface.
        abort();
    }
}

int random_init_from_sys(mbedtls_chacha20_context *ctx)
{
    int err;

    unsigned char random_key[32];
    if ((err = sys_getrandom(random_key, 32)) != 0) {
        log_error("Failed to get %zu random bytes", sizeof(random_key));
        return err;
    }

    random_init(ctx, random_key);
    return ERR_OK;
}

void random_get_bytes(mbedtls_chacha20_context *ctx, void *buffer, size_t size)
{
    static uint8_t ZEROES[512];

    for (size_t generated = 0; generated < size; ) {
        size_t to_generate = min_size_t(size, sizeof(ZEROES));
        if (mbedtls_chacha20_update(ctx, to_generate, ZEROES, buffer) != 0) {
            abort(); // Isn't possible to reach `mbedtls_chacha20_update` can't fail.
        }
        generated += to_generate;
    }
}

uint32_t random_uint32(mbedtls_chacha20_context *ctx)
{
    uint32_t result;
    random_get_bytes(ctx, &result, sizeof(result));
    return result;
}
