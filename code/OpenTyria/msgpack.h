int unpack_helper(
    MsgField *fields, size_t n_fields, size_t *out_consumed,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer);

int unpack_msg(
    MsgFormat format, size_t *consumed,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer);

int pack_helper(
    MsgField *fields, size_t n_fields, size_t *out_written,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer);

int pack_msg(
    MsgFormat format, size_t *written,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer);
