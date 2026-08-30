#pragma once

bool read_u16(const uint8_t *data, size_t size, uint16_t *result, size_t *consumed)
{
    size_t idx = *consumed;
    if ((idx + size) < sizeof(*result)) {
        return false;
    }

    *result = le16dec(&data[idx]);
    *consumed = idx + sizeof(*result);
    return true;
}

bool read_u32(const uint8_t *data, size_t size, uint32_t *result, size_t *consumed)
{
    size_t idx = *consumed;
    if ((idx + size) < sizeof(*result)) {
        return false;
    }

    *result = le32dec(&data[idx]);
    *consumed = idx + sizeof(*result);
    return true;
}

bool read_array_u8_exact(const uint8_t *data, size_t size, uint8_t *buffer, size_t buffer_size, size_t *consumed)
{
    size_t idx = *consumed;
    if ((idx + size) < buffer_size) {
        return false;
    }

    memcpy(buffer, &data[idx], buffer_size);
    *consumed = idx + buffer_size;
    return true;
}

void appendv(array_char_t *buffer, const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int ret = vsnprintf(NULL, 0, fmt, args);
    if (ret < 0) {
        va_end(args_copy);
        abort();
    }

    // We need to allocate one more bytes, because  of `vsnprintf`.
    // We will pop this "\0" byte later.
    char *write_ptr = array_push(buffer, (size_t)ret + 1);
    vsnprintf((char *)write_ptr, (size_t)ret + 1, fmt, args_copy);
    (void)array_pop(buffer);
    va_end(args_copy);
}

void appendf(array_char_t *buffer, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendv(buffer, fmt, args);
    va_end(args);
}
