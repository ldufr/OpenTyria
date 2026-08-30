void copy_array_from_le16dec(uint16_t *dst, const uint8_t *src, size_t count)
{
    for (size_t idx = 0; idx < count; ++idx) {
        dst[idx] = le16dec(&src[idx * 2]);
    }
}

void copy_array_from_le32dec(uint32_t *dst, const uint8_t *src, size_t count)
{
    for (size_t idx = 0; idx < count; ++idx) {
        dst[idx] = le32dec(&src[idx * 4]);
    }
}

int unpack_helper(
    MsgField *fields, size_t n_fields, size_t *out_consumed,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer)
{
    size_t   wpos = 0;
    size_t   consumed = 0;
    uint32_t array_size;
    size_t   required_size;
    uint16_t value_u16;
    uint32_t value_u32[3];

    for (size_t idx = 0; idx < n_fields; ++idx) {
        MsgField field = fields[idx];
        switch (field.type) {
        case TYPE_MSG_HEADER:
        case TYPE_WORD:
            if ((n_buffer - wpos) < sizeof(uint16_t) ||
                (n_input - consumed) < sizeof(uint16_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            value_u16 = le16dec(&input[consumed]);
            memcpy(&buffer[wpos], &value_u16, sizeof(value_u16));
            wpos += sizeof(uint16_t);
            consumed += sizeof(uint16_t);
            break;
        case TYPE_DWORD:
        case TYPE_FLOAT:
        case TYPE_AGENT_ID:
            if ((n_buffer - wpos) < sizeof(uint32_t) ||
                (n_input - consumed) < sizeof(uint32_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            value_u32[0] = le32dec(&input[consumed]);
            memcpy(&buffer[wpos], &value_u32[0], sizeof(value_u32[0]));
            wpos += sizeof(uint32_t);
            consumed += sizeof(uint32_t);
            break;
        case TYPE_VECT2:
            if ((n_buffer - wpos) < sizeof(Vec2f) ||
                (n_input - consumed) < sizeof(Vec2f)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            STATIC_ASSERT(sizeof(Vec2f) <= sizeof(value_u32));
            value_u32[0] = le32dec(&input[consumed + 0]);
            value_u32[1] = le32dec(&input[consumed + 4]);
            memcpy(&buffer[wpos], value_u32, sizeof(Vec2f));
            wpos += sizeof(Vec2f);
            consumed += sizeof(Vec2f);
            break;
        case TYPE_VECT3:
            if ((n_buffer - wpos) < sizeof(Vec3f) ||
                (n_input - consumed) < sizeof(Vec3f)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            STATIC_ASSERT(sizeof(Vec3f) <= sizeof(value_u32));
            value_u32[0] = le32dec(&input[consumed + 0]);
            value_u32[1] = le32dec(&input[consumed + 4]);
            value_u32[2] = le32dec(&input[consumed + 8]);
            memcpy(&buffer[wpos], value_u32, sizeof(Vec3f));
            wpos += sizeof(Vec3f);
            consumed += sizeof(Vec3f);
            break;
        case TYPE_BYTE:
            if ((n_buffer - wpos) < sizeof(uint8_t) ||
                (n_input - consumed) < sizeof(uint8_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&buffer[wpos], &input[consumed], sizeof(uint8_t));
            wpos += sizeof(uint8_t);
            consumed += sizeof(uint8_t);
            break;
        case TYPE_BLOB:
            if ((n_buffer - wpos) < field.length ||
                (n_input - consumed) < field.length) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&buffer[wpos], &input[consumed], field.length);
            wpos += field.length;
            consumed += field.length;
            break;
        case TYPE_ARRAY_16:
        case TYPE_STRING_16:
            if ((n_buffer - wpos) < sizeof(uint32_t) ||
                (n_input - consumed) < sizeof(uint16_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            array_size = le16dec(&input[consumed]);
            if (field.length < array_size) {
                return ERR_BAD_USER_DATA;
            }

            memcpy(&buffer[wpos], &array_size, sizeof(array_size));
            wpos += sizeof(uint32_t);
            consumed += sizeof(uint16_t);

            required_size = array_size * sizeof(uint16_t);
            if ((n_buffer - wpos) < required_size ||
                (n_input - consumed) < required_size) {
                return ERR_NOT_ENOUGH_DATA;
            }
            copy_array_from_le16dec((uint16_t *)&buffer[wpos], &input[consumed], array_size);
            wpos += field.length * sizeof(uint16_t);
            consumed += required_size;
            break;
        case TYPE_ARRAY_8:
            if ((n_buffer - wpos) < sizeof(uint32_t) ||
                (n_input - consumed) < sizeof(uint16_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            array_size = le16dec(&input[consumed]);
            if (field.length < array_size) {
                return ERR_BAD_USER_DATA;
            }

            memcpy(&buffer[wpos], &array_size, sizeof(array_size));
            wpos += sizeof(uint32_t);
            consumed += sizeof(uint16_t);

            required_size = array_size * sizeof(uint8_t);
            if ((n_buffer - wpos) < required_size ||
                (n_input - consumed) < required_size) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&buffer[wpos], &input[consumed], required_size);
            wpos += field.length * sizeof(uint8_t);
            consumed += required_size;
            break;
        case TYPE_ARRAY_32:
            if ((n_buffer - wpos) < sizeof(uint32_t) ||
                (n_input - consumed) < sizeof(uint16_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            array_size = le16dec(&input[consumed]);
            if (field.length < array_size) {
                return ERR_BAD_USER_DATA;
            }

            memcpy(&buffer[wpos], &array_size, sizeof(array_size));
            wpos += sizeof(uint32_t);
            consumed += sizeof(uint16_t);

            required_size = array_size * sizeof(uint32_t);
            if ((n_buffer - wpos) < required_size ||
                (n_input - consumed) < required_size) {
                return ERR_NOT_ENOUGH_DATA;
            }
            copy_array_from_le32dec((uint32_t *)&buffer[wpos], &input[consumed], array_size);
            wpos += field.length * sizeof(uint32_t);
            consumed += required_size;
            break;
        case TYPE_NESTED_STRUCT:
            if ((n_buffer - wpos) < sizeof(uint32_t) ||
                (n_input - consumed) < sizeof(uint8_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            array_size = input[consumed];
            if (field.length < array_size) {
                return ERR_BAD_USER_DATA;
            }

            memcpy(&buffer[wpos], &array_size, sizeof(array_size));
            wpos += sizeof(uint32_t);
            consumed += sizeof(uint8_t);

            for (size_t jdx = 0; jdx < array_size; ++jdx) {
                size_t next_field = idx + 1;
                size_t sub_consumed;
                int err = unpack_helper(
                    &fields[next_field], n_fields - next_field, &sub_consumed,
                    &input[consumed], n_input - consumed,
                    &buffer[wpos], n_buffer - wpos);
                if (err != 0) {
                    return err;
                }
                consumed += sub_consumed;
            }

            idx = n_fields;
            break;
        default:
            abort();
        }
    }

    *out_consumed = consumed;
    return 0;
}

int unpack_msg(
    MsgFormat format, size_t *consumed,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer)
{
    memset(buffer, 0, format.unpack_size);
    return unpack_helper(format.fields, format.count, consumed, input, n_input, buffer, n_buffer);
}

void copy_array_to_le16enc(uint8_t *dst, const uint16_t *src, size_t count)
{
    for (size_t idx = 0; idx < count; ++idx) {
        le16enc(&dst[idx * 2], src[idx]);
    }
}

void copy_array_to_le32enc(uint8_t *dst, const uint32_t *src, size_t count)
{
    for (size_t idx = 0; idx < count; ++idx) {
        le32enc(&dst[idx * 4], src[idx]);
    }
}

int pack_helper(
    MsgField *fields, size_t n_fields, size_t *out_written,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer)
{
    size_t   wpos = 0;
    size_t   consumed = 0;
    uint32_t array_size;
    size_t   required_size;
    uint16_t value_u16;
    uint32_t value_u32[3];

    for (size_t idx = 0; idx < n_fields; ++idx) {
        MsgField field = fields[idx];
        switch (field.type) {
        case TYPE_MSG_HEADER:
        case TYPE_WORD:
            if ((n_buffer - wpos) < sizeof(uint16_t) ||
                (n_input - consumed) < sizeof(uint16_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&value_u16, &input[consumed], sizeof(value_u16));
            le16enc(&buffer[wpos], value_u16);
            wpos += sizeof(uint16_t);
            consumed += sizeof(uint16_t);
            break;
        case TYPE_DWORD:
        case TYPE_FLOAT:
        case TYPE_AGENT_ID:
            if ((n_buffer - wpos) < sizeof(uint32_t) ||
                (n_input - consumed) < sizeof(uint32_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&value_u32[0], &input[consumed], sizeof(value_u32[0]));
            le32enc(&buffer[wpos], value_u32[0]);
            wpos += sizeof(uint32_t);
            consumed += sizeof(uint32_t);
            break;
        case TYPE_VECT2:
            if ((n_buffer - wpos) < sizeof(Vec2f) ||
                (n_input - consumed) < sizeof(Vec2f)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            STATIC_ASSERT(sizeof(Vec2f) <= sizeof(value_u32));
            memcpy(value_u32, &input[consumed], sizeof(Vec2f));
            le32enc(&buffer[wpos + 0], value_u32[0]);
            le32enc(&buffer[wpos + 4], value_u32[1]);
            wpos += sizeof(Vec2f);
            consumed += sizeof(Vec2f);
            break;
        case TYPE_VECT3:
            if ((n_buffer - wpos) < sizeof(Vec3f) ||
                (n_input - consumed) < sizeof(Vec3f)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            STATIC_ASSERT(sizeof(Vec3f) <= sizeof(value_u32));
            memcpy(value_u32, &input[consumed], sizeof(Vec3f));
            le32enc(&buffer[wpos + 0], value_u32[0]);
            le32enc(&buffer[wpos + 4], value_u32[1]);
            le32enc(&buffer[wpos + 8], value_u32[2]);
            wpos += sizeof(Vec3f);
            consumed += sizeof(Vec3f);
            break;
        case TYPE_BYTE:
            if ((n_buffer - wpos) < sizeof(uint8_t) ||
                (n_input - consumed) < sizeof(uint8_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&buffer[wpos], &input[consumed], sizeof(uint8_t));
            wpos += sizeof(uint8_t);
            consumed += sizeof(uint8_t);
            break;
        case TYPE_BLOB:
            if ((n_buffer - wpos) < field.length ||
                (n_input - consumed) < field.length) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&buffer[wpos], &input[consumed], field.length);
            wpos += field.length;
            consumed += field.length;
            break;
        case TYPE_ARRAY_16:
        case TYPE_STRING_16:
            if ((n_buffer - wpos) < sizeof(uint16_t) ||
                (n_input - consumed) < sizeof(uint32_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            memcpy(&array_size, &input[consumed], sizeof(array_size));
            if (field.length < array_size) {
                log_error("Can't write an array of %" PRIu32 " elements, maximum is %" PRIu32, array_size, field.length);
                return ERR_UNSUCCESSFUL;
            }

            le16enc(&buffer[wpos], (uint16_t)(array_size & 0xffff));
            wpos += sizeof(uint16_t);
            consumed += sizeof(uint32_t);

            required_size = array_size * sizeof(uint16_t);
            if ((n_buffer - wpos) < required_size ||
                (n_input - consumed) < required_size) {
                return ERR_NOT_ENOUGH_DATA;
            }

            copy_array_to_le16enc(&buffer[wpos], (const uint16_t *)&input[consumed], array_size);
            wpos += required_size;
            consumed += field.length * sizeof(uint16_t);
            break;
        case TYPE_ARRAY_8:
            if ((n_buffer - wpos) < sizeof(uint16_t) ||
                (n_input - consumed) < sizeof(uint32_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            memcpy(&array_size, &input[consumed], sizeof(array_size));
            if (field.length < array_size) {
                log_error("Can't write an array of %" PRIu32 " elements, maximum is %" PRIu32, array_size, field.length);
                return ERR_UNSUCCESSFUL;
            }
            le16enc(&buffer[wpos], (uint16_t)(array_size & 0xffff));
            wpos += sizeof(uint16_t);
            consumed += sizeof(uint32_t);

            required_size = array_size * sizeof(uint8_t);
            if ((n_buffer - wpos) < required_size ||
                (n_input - consumed) < required_size) {
                return ERR_NOT_ENOUGH_DATA;
            }
            memcpy(&buffer[wpos], &input[consumed], required_size);
            wpos += required_size;
            consumed += field.length * sizeof(uint8_t);
            break;
        case TYPE_ARRAY_32:
            if ((n_buffer - wpos) < sizeof(uint16_t) ||
                (n_input - consumed) < sizeof(uint32_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            memcpy(&array_size, &input[consumed], sizeof(array_size));
            if (field.length < array_size) {
                log_error("Can't write an array of %" PRIu32 " elements, maximum is %" PRIu32, array_size, field.length);
                return ERR_UNSUCCESSFUL;
            }
            le16enc(&buffer[wpos], (uint16_t)(array_size & 0xffff));
            wpos += sizeof(uint16_t);
            consumed += sizeof(uint32_t);

            required_size = array_size * sizeof(uint32_t);
            if ((n_buffer - wpos) < required_size ||
                (n_input - consumed) < required_size) {
                return ERR_NOT_ENOUGH_DATA;
            }
            copy_array_to_le32enc(&buffer[wpos], (const uint32_t *)&input[consumed], array_size);
            wpos += required_size;
            consumed += field.length * sizeof(uint32_t);
            break;
        case TYPE_NESTED_STRUCT:
            if ((n_buffer - wpos) < sizeof(uint8_t) ||
                (n_input - consumed) < sizeof(uint32_t)) {
                return ERR_NOT_ENOUGH_DATA;
            }

            memcpy(&array_size, &input[consumed], sizeof(array_size));
            if (field.length < array_size) {
                log_error("Can't write an array of %" PRIu32 " elements, maximum is %" PRIu32, array_size, field.length);
                return ERR_UNSUCCESSFUL;
            }
            buffer[wpos] = (uint8_t)(array_size & 0xff);
            wpos += sizeof(uint8_t);
            consumed += sizeof(uint32_t);

            for (size_t jdx = 0; jdx < array_size; ++jdx) {
                size_t next_field = idx + 1;
                size_t sub_written;
                int err = unpack_helper(
                    &fields[next_field], n_fields - next_field, &sub_written,
                    &input[consumed], n_input - consumed,
                    &buffer[wpos], n_buffer - wpos);
                if (err != 0) {
                    return err;
                }

                wpos += sub_written;
            }

            idx = n_fields;
            break;
        default:
            abort();
        }
    }

    *out_written = wpos;
    return 0;
}

int pack_msg(
    MsgFormat format, size_t *written,
    const uint8_t *input, size_t n_input,
    uint8_t *buffer, size_t n_buffer)
{
    return pack_helper(format.fields, format.count, written, input, n_input, buffer, n_buffer);
}
