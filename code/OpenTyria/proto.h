#pragma once

typedef struct _MSG_CLIENT_SEED {
    uint16_t header;    // 0x4200
    uint8_t  seed[64];
} MSG_CLIENT_SEED;

typedef struct _MSG_SERVER_SEED {
    uint16_t header;    // 0x1601
    uint8_t  seed[20];
} MSG_SERVER_SEED;

typedef struct _AUTH_CMSG_VERSION {
    uint32_t header;    // 0xC0400
    uint32_t version;
    uint32_t h0008;     // 1
    uint32_t h000C;     // 4
} AUTH_CMSG_VERSION;

typedef struct _GAME_CMSG_VERSION {
    uint32_t header;    // 0xC0500
    uint32_t version;
    uint32_t h0008;     // 1
    uint32_t map_token;
    uint32_t map_id;
    uint32_t player_token;
    uint8_t  account_uuid[16];
    uint8_t  character_uuid[16];
    uint32_t h0038;     // 0
    uint32_t h003C;     // 0
} GAME_CMSG_VERSION;

typedef struct CTRL_CMSG_VERSION {
    uint32_t header;
} CTRL_CMSG_VERSION;
