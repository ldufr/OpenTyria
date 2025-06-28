int read_bytes(slice_uint8_t *input, uint8_t *result, size_t length)
{
    if (input->len < length) {
        return ERR_NOT_ENOUGH_DATA;
    } else {
        memcpy(result, input->ptr, length);
        input->ptr += length;
        input->len -= length;
        return 0;
    }
}

int read_le_uint32(slice_uint8_t *input, uint32_t *result)
{
    if (input->len < sizeof(*result)) {
        return ERR_NOT_ENOUGH_DATA;
    } else {
        *result = le32dec(input->ptr);
        input->ptr += sizeof(*result);
        input->len -= sizeof(*result);
        return 0;
    }
}

int read_le_uint64(slice_uint8_t *input, uint64_t *result)
{
    if (input->len < sizeof(*result)) {
        return ERR_NOT_ENOUGH_DATA;
    } else {
        *result = le32dec(input->ptr);
        input->ptr += sizeof(*result);
        input->len -= sizeof(*result);
        return 0;
    }
}
