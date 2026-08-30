#pragma once

void random_init(mbedtls_chacha20_context *ctx, const uint8_t key[32]);
int  random_init_from_sys(mbedtls_chacha20_context *ctx);
void random_get_bytes(mbedtls_chacha20_context *ctx, void *buffer, size_t size);
uint32_t random_uint32(mbedtls_chacha20_context *ctx);

