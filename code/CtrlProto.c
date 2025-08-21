#pragma once

MsgField CTRL_MSG_0000[] = {
    {TYPE_MSG_HEADER, 0},
    {TYPE_DWORD, 0},
};

MsgField CTRL_MSG_0001[] = {
    {TYPE_MSG_HEADER, 0},
    {TYPE_DWORD, 0},
};

MsgFormat CTRL_MSG_FORMATS[CTRL_MSG_COUNT] = {
// header | field_count | fields | max_size
    {0, ARRAY_SIZE(CTRL_MSG_0000), CTRL_MSG_0000, 6},
    {1, ARRAY_SIZE(CTRL_MSG_0001), CTRL_MSG_0001, 6},
};
