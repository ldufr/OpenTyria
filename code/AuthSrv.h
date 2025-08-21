#pragma once

#define AUTH_CMSG_VERSION_HEADER 0xC0400
#define GAME_CMSG_VERSION_HEADER 0xC0500
#define CTRL_CMSG_VERSION_HEADER 0xC0FFF

#define CMSG_CLIENT_SEED_HEADER  0x4200
#define SMSG_SERVER_SEED_HEADER  0x1601

extern mbedtls_mpi prime_modulus;
extern mbedtls_mpi server_private;

typedef enum ConnState {
    ConnState_AwaitHello,
    ConnState_AwaitKeyExchange,
    ConnState_AwaitDone,
} ConnState;

typedef enum ConnType {
    ConnType_None,
    ConnType_Auth,
    ConnType_Game,
    ConnType_Ctrl,
} ConnType;

typedef struct Connection {
    uintptr_t             token;
    IoSource              source;
    SocketAddr            peer_addr;
    uint8_t               incoming[256];
    size_t                n_incoming;
    bool                  writable;
    uint8_t               master_secret[20];
    ConnState             state;
    ConnType              type;
    union
    {
        AUTH_CMSG_VERSION auth_version;
        GAME_CMSG_VERSION game_version;
        CTRL_CMSG_VERSION ctrl_version;
    };
} Connection;

typedef struct AuthConnection {
    uintptr_t        token;
    IoSource         source;
    SocketAddr       peer_addr;
    array_uint8_t    incoming;
    array_uint8_t    outgoing;
    bool             writable;
    arc4_context     cipher_enc;
    arc4_context     cipher_dec;
    AuthCliMsgArray  messages;
    AuthSrvMsg       srv_msg;
    GmUuid           user_id;
    GmUuid           session_id;
    GmUuid           account_id;
    size_t           n_username;
    uint16_t         username[32];
    size_t           n_pcname;
    uint16_t         pcname[32];
    GmUuid           computer_hash;
    uint32_t         server_salt;
    PlayerStatus     player_status;
    bool             connected;
    DbAccount        account;
    DbCharacterArray characters;
    size_t           selected_character_idx;
} AuthConnection;

void AuthConnection_Free(AuthConnection *conn);
int  AuthConnection_FlushOutgoingBuffer(AuthConnection *conn);
int  AuthConnection_SendMessage(AuthConnection *conn, AuthSrvMsg *msg, size_t size);

typedef enum IoObjectType {
    IoObjectType_None,
    IoObjectType_Listener,
    IoObjectType_Connection,
    IoObjectType_AuthConnection,
    IoObjectType_CtrlConnection,
} IoObjectType;

typedef struct IoObject {
    IoObjectType type;
    union
    {
        IoSource       listener;
        Connection     connection;
        AuthConnection auth_connection;
        CtrlConn       ctrl_connection;
    };
} IoObject;

typedef struct IoObjectMap {
    uintptr_t key;
    IoObject  value;
} IoObjectMap;

typedef struct ConnectedAccountInfo {
    GmUuid      key; // account id
    GmUuid      char_id;
    uint32_t    map_token;
    uintptr_t   auth_conn_token;
} ConnectedAccountInfo;

typedef struct GameSrvMeta {
    uintptr_t ctrl_conn;
    bool      ready;
    
} GameSrvMeta;

typedef struct AuthSrv {
    Iocp                     iocp;
    uintptr_t                last_token_issued;
    IoObjectMap             *objects;
    array_uintptr_t          objects_with_event;
    array_uintptr_t          objects_to_remove;
    ArrayEvent               events;
    mbedtls_chacha20_context random;
    Database                 database;
    GameSrvArray             game_servers;
    ConnectedAccountInfo    *connected_accounts;
    SocketAddr               internal_address;
} AuthSrv;

int  AuthSrv_Setup(AuthSrv *srv);
void AuthSrv_Free(AuthSrv *srv);
int  AuthSrv_Bind(AuthSrv *srv, const char *addr, size_t addr_len);
void AuthSrv_Update(AuthSrv *srv);
