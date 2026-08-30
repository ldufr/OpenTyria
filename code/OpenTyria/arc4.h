#pragma once

typedef struct arc4_context {
    int x;
    int y;
    uint8_t m[256];
} arc4_context;

void arc4_init(arc4_context *ctx);
void arc4_free(arc4_context *ctx);

void arc4_setup(arc4_context *ctx, const uint8_t *key, size_t keylen);
void arc4_crypt(arc4_context *ctx, const uint8_t *input, uint8_t *output, size_t length);
void arc4_crypt_inplace(arc4_context *ctx, uint8_t *buffer, size_t length);
