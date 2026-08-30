#pragma once

unsigned int get_digit(char ch)
{
    switch (ch) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return ch - '0';
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return (ch - 'a') + 10;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        return (ch - 'A') + 10;
    default:
        return 0;
    }
}

bool parse_u16(uint16_t *result, const char *input, size_t len, uint8_t base)
{
    if (len == 0) {
        return false;
    }

    bool positive = true;
    size_t idx = 0;
    if (input[0] == '+' || input[0] == '-') {
        if (input[0] == '+') {
            positive = true;
        } else {
            positive = false;
        }

        if (len == 1) {
            return false;
        }

        ++idx;
    }

    if (input[idx] == '0') {
        if (++idx == len) {
            *result = 0;
            return true;
        }

        if (base == 0) {
            if (input[idx] == 'x' || input[idx] == 'X') {
                base = 16;
                ++idx;
            } else if (input[idx] == 'b' || input[idx] == 'B') {
                base = 2;
                ++idx;
            } else if (input[idx] == 'o' || input[idx] == 'O') {
                base = 8;
                ++idx;
            } else if (input[idx] < '0' || '9' < input[idx]) {
                return false;
            } else {
                base = 10;
            }
        }
    }

    uint16_t value = 0;
    for (; idx < len; ++idx) {
        char ch = input[idx];
        uint16_t digit = (uint16_t)get_digit(ch);

        if ((digit == 0 && ch != '0') || base <= digit) {
            return false;
        }

        if (((UINT16_MAX - digit) / base) < value) {
            return false;
        }

        value = (value * base) + digit;
    }

    if (!positive && value != 0) {
        return false;
    }

    *result = value;
    return true;
}
