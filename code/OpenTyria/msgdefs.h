#pragma once

#define MSG_MAX_BUFFER_SIZE 4096

typedef enum FieldType {
    TYPE_MSG_HEADER     = 1,
    TYPE_AGENT_ID       = 2,
    TYPE_FLOAT          = 3,
    TYPE_VECT2          = 4,
    TYPE_VECT3          = 5,
    TYPE_BYTE           = 6,
    TYPE_WORD           = 7,
    TYPE_DWORD          = 8,
    TYPE_BLOB           = 9,
    TYPE_STRING_16      = 10,
    TYPE_ARRAY_8        = 11,
    TYPE_ARRAY_16       = 12,
    TYPE_ARRAY_32       = 13,
    TYPE_NESTED_STRUCT  = 14,
} FieldType;

typedef struct MsgField MsgField;
struct MsgField {
    FieldType type;
    size_t    length;
};

typedef struct MsgFormat {
    uint32_t  header;
    size_t    count;
    MsgField *fields;
    size_t    unpack_size;
} MsgFormat;

extern MsgFormat AUTH_CMSG_FORMATS[AUTH_CMSG_COUNT];
extern MsgFormat AUTH_SMSG_FORMATS[AUTH_SMSG_COUNT];
extern MsgFormat GAME_CMSG_FORMATS[GAME_CMSG_COUNT];
extern MsgFormat GAME_SMSG_FORMATS[GAME_SMSG_COUNT];
