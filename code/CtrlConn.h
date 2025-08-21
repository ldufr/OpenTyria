#pragma once

#pragma pack(push, 1)
typedef struct CTRL_CMSG_VERSION {
    uint32_t header;
} CTRL_CMSG_VERSION;

typedef struct CtrlMsgServerReady {
    uint16_t msg_id;
    uint32_t server_id;
} CtrlMsgServerReady;

typedef struct CtrlMsgPlayerLeaved {
    uint16_t msg_id;
    uint32_t server_id;
    GmUuid   account_id;
} CtrlMsgPlayerLeaved;

/*
typedef struct CtrlMsgAddPlayer {
    uint32_t server_id;
} CtrlMsgAddPlayer;

typedef struct CtrlMsgKickPlayer {
    uint16_t msg_id;
    GmUuid   player_id;
} CtrlMsgKickPlayer;
*/

typedef union CtrlMsg {
    uint16_t            msg_id;
    uint8_t             buffer[MSG_MAX_BUFFER_SIZE];
    CtrlMsgServerReady  server_ready;
    CtrlMsgPlayerLeaved player_leaved;
} CtrlMsg;
#pragma pack(pop)

typedef array(CtrlMsg *) CtrlMsgArray;

typedef enum CtrlMsgId {
    CtrlMsgId_None,
    CtrlMsgId_ServerReady,
    CtrlMsgId_PlayerLeaved,
} CtrlMsgId;

typedef struct CtrlConn {
    uintptr_t     token;
    IoSource      source;
    SocketAddr    peer_addr;
    array_uint8_t incoming;
    array_uint8_t outgoing;
    bool          writable;
    bool          connected;
    CtrlMsgArray  messages;
} CtrlConn;

int  CtrlConn_Setup(CtrlConn *conn);
void CtrlConn_Free(CtrlConn *conn);
int  CtrlConn_FlushOutgoingBuffer(CtrlConn *conn);
int  CtrlConn_WriteMessage(CtrlConn *conn, CtrlMsg *msg, size_t size);
int  CtrlConn_GetMessages(CtrlConn *conn);
int  CtrlConn_ProcessEvent(CtrlConn *conn, Event event);

int  CtrlConn_UpdateWrite(CtrlConn *conn);
int  CtrlConn_WriteHandshake(CtrlConn *conn);
int  CtrlConn_NotifyServerReady(CtrlConn *conn);
int  CtrlConn_SendPlayerLeaved(CtrlConn *conn);
