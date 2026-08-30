#pragma once

#define TERM_FINAL          (0x0000)
#define TERM_INTERMEDIATE   (0x0001)
#define CONCAT_CODED        (0x0002)
#define CONCAT_LITERAL      (0x0003)
#define STRING_CHAR_FIRST   (0x0010)
#define WORD_VALUE_BASE     (0x0100)
#define WORD_BIT_MORE       (0x8000)
#define WORD_VALUE_RANGE    (WORD_BIT_MORE - WORD_VALUE_BASE)

bool GmText_IsParam(uint16_t code)
{
    return 0x101 <= code && code <= 0x10F;
}

bool GmText_IsParamSegment(uint16_t code)
{
    return (0x10A <= code && code <= 0x10C);
}

bool GmText_IsParamLiteral(uint16_t code)
{
    return (0x107 <= code && code <= 0x109);
}

bool GmText_IsParamNumeric(uint16_t code)
{
    return (0x101 <= code && code <= 0x106) || (0x10D <= code && code <= 0x10F);
}

int GmText_ValidateUserMessage(slice_uint16_t text)
{
    for (size_t idx = 0; idx < text.len; ++idx) {
        uint16_t ch = text.ptr[idx];
        if (ch == TERM_FINAL || ch == TERM_INTERMEDIATE) {
            return ERR_BAD_USER_DATA;
        }
    }
    return ERR_OK;
}

int GmText_BuildLiteral(array_uint16_t *buffer, slice_uint16_t text)
{
    int err;
    if ((err = array_add(buffer, 0x108)) != 0 || (err = array_add(buffer, 0x107)) != 0) {
        return err;
    }
    uint16_t *dst;
    if ((dst = array_push(buffer, text.len)) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }
    memcpy_u16(dst, text.ptr, text.len);
    if ((err = array_add(buffer, TERM_INTERMEDIATE)) != 0) {
        return err;
    }
    return ERR_OK;
}
