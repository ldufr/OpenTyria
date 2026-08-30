#pragma once

#define CtrlMsgDefs \
    X(ServerReady) \
    X(PlayerLeft) \
    X(TransferConnection) \
    X(StartPlayerTransfer) \
    X(CompletePlayerTransfer) \
    X(CancelPlayerTransfer)

typedef enum CtrlMsgId {
    CtrlMsgId_None,
#define X(name) CtrlMsgId_ ## name,
    CtrlMsgDefs
#undef X
    CtrlMsgId_Count,
} CtrlMsgId;

typedef struct CtrlMsg_ServerReady {
    CtrlMsgId msg_id;
    uint32_t  server_id;
} CtrlMsg_ServerReady;

typedef struct CtrlMsg_PlayerLeft {
    CtrlMsgId msg_id;
    GmUuid    account_id;
    GmUuid    char_id;
} CtrlMsg_PlayerLeft;

typedef struct CtrlMsg_TransferConnection {
    CtrlMsgId   msg_id;
    SocketAddr  peer_addr;
    IoSource    source;
    uintptr_t   token;
    uint8_t     cipher_init_key[20];
    bool        reconnection;
    GmUuid      account_id;
    GmUuid      char_id;
} CtrlMsg_TransferConnection;

typedef struct CtrlMsg_StartPlayerTransfer {
    CtrlMsgId  msg_id;
    uint32_t   player_id;
    GmUuid     account_id;
    GmUuid     char_id;
    GmDistrict district;
} CtrlMsg_StartPlayerTransfer;

typedef struct CtrlMsg_CompletePlayerTransfer {
    CtrlMsgId  msg_id;
    uint32_t   player_id;
    GmUuid     char_id;
    SocketAddr address;
    uint32_t   map_token;
    uint32_t   player_token;
    uint16_t   map_id;
    DistrictRegion region;
} CtrlMsg_CompletePlayerTransfer;

typedef struct CtrlMsg_CancelPlayerTransfer {
    CtrlMsgId msg_id;
} CtrlMsg_CancelPlayerTransfer;

typedef union CtrlMsg {
    CtrlMsgId           msg_id;
    uint8_t             buffer[MSG_MAX_BUFFER_SIZE];
#define X(name) CtrlMsg_ ## name name;
    CtrlMsgDefs
#undef X
} CtrlMsg;
typedef array(CtrlMsg *) CtrlMsgArray;

typedef struct CtrlConnection {
    uintptr_t     token;
    IoSource      source;
    SocketAddr    peer_addr;
    array_uint8_t incoming;
    array_uint8_t outgoing;
    bool          writable;
    bool          connected;
    CtrlMsgArray  messages;
    uint32_t      server_id;
} CtrlConnection;

int  CtrlConn_Setup(CtrlConnection *conn);
void CtrlConn_Free(CtrlConnection *conn);
int  CtrlConn_FlushOutgoingBuffer(CtrlConnection *conn);
int  CtrlConn_WriteMessage(CtrlConnection *conn, CtrlMsg *msg, size_t size);
int  CtrlConn_GetMessages(CtrlConnection *conn);
int  CtrlConn_ProcessEvent(CtrlConnection *conn, Event event);

int  CtrlConn_UpdateWrite(CtrlConnection *conn);
int  CtrlConn_WriteHandshake(CtrlConnection *conn);
int  CtrlConn_SendServerReady(CtrlConnection *conn, uint32_t server_id);
int  CtrlConn_SendPlayerLeft(CtrlConnection *conn, GmUuid account_id, GmUuid char_id);
