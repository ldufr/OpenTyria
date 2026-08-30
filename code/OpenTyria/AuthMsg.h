#pragma once

#pragma pack(push, 1)
typedef struct AuthSrv_AccountSettings {
    uint16_t header;
    uint32_t req_id;
    uint32_t n_data;
    uint8_t  data[2048];
} AuthSrv_AccountSettings;

typedef struct AuthSrv_ComputerHash {
    uint16_t header;
    uint32_t version;
    uint8_t  hash[16];
} AuthSrv_ComputerHash;

typedef struct AuthSrv_ComputerInfo {
    uint16_t header;
    uint32_t n_username;
    uint16_t username[32];
    uint32_t n_pcname;
    uint16_t pcname[32];
} AuthSrv_ComputerInfo;

typedef struct AuthSrv_AskServerResponse {
    uint16_t header;
    uint32_t req_id;
} AuthSrv_AskServerResponse;

typedef struct AuthSrv_ClientHeartbeat {
    uint16_t header;
    uint32_t tick;
} AuthSrv_ClientHeartbeat;

typedef struct AuthSrv_PortalAccountLogin {
    uint16_t header;
    uint32_t req_id;
    uint8_t  user_id[16];
    uint8_t  session_id[16];
    uint32_t n_charname;
    uint16_t charname[20];
    uint32_t n_type;
    uint16_t type[32];
} AuthSrv_PortalAccountLogin;

typedef struct AuthSrv_HardwareInfo {
    uint16_t header;
    uint32_t n_info;
    uint8_t  info[92];
    uint8_t  hash[16];
} AuthSrv_HardwareInfo;

typedef struct AuthSrv_SetPlayerStatus {
    uint16_t header;
    uint32_t status;
} AuthSrv_SetPlayerStatus;

typedef struct AuthSrv_ChangeCharacter {
    uint16_t header;
    uint32_t req_id;
    uint32_t n_name;
    uint16_t name[20];
} AuthSrv_ChangeCharacter;

typedef struct AuthSrv_RequestGameInstance {
    uint16_t header;
    uint32_t req_id;
    uint32_t map_type;
    uint32_t map_id;
    uint32_t region;
    uint32_t district;
    uint32_t language;
} AuthSrv_RequestGameInstance;

typedef struct AuthSrv_Disconnect {
    uint16_t header;
    uint32_t h0002;
} AuthSrv_Disconnect;

typedef struct AuthSrv_AcceptEula {
    uint16_t header;
    uint8_t  value;
} AuthSrv_AcceptEula;

typedef struct AuthSrv_AddAccessKey {
    uint16_t header;
    uint32_t req_id;
    uint32_t n_key;
    uint16_t key[26];
} AuthSrv_AddAccessKey;

typedef struct AuthSrv_DeleteCharacter {
    uint16_t header;
    uint32_t req_id;
    uint32_t n_charname;
    uint16_t charname[20];
} AuthSrv_DeleteCharacter;

typedef struct AuthSrv_UpdateCharacterSettings {
    uint16_t header;
    uint32_t req_id;
    uint32_t n_charname;
    uint16_t charname[20];
    uint32_t n_extended;
    uint8_t  extended[64];
} AuthSrv_UpdateCharacterSettings;

typedef union AuthCliMsg {
    uint16_t                        header;
    uint8_t                         buffer[MSG_MAX_BUFFER_SIZE];
    AuthSrv_ClientHeartbeat         heartbeat;
    AuthSrv_ComputerHash            computer_hash;
    AuthSrv_ComputerInfo            computer_info;
    AuthSrv_AskServerResponse       ask_server_response;
    AuthSrv_PortalAccountLogin      portal_account_login;
    AuthSrv_HardwareInfo            hardware_info;
    AuthSrv_SetPlayerStatus         set_player_status;
    AuthSrv_ChangeCharacter         change_character;
    AuthSrv_RequestGameInstance     request_game_instance;
    AuthSrv_Disconnect              disconnect;
    AuthSrv_AcceptEula              accept_eula;
    AuthSrv_AddAccessKey            add_access_key;
    AuthSrv_DeleteCharacter         delete_character;
    AuthSrv_UpdateCharacterSettings update_character_settings;
} AuthCliMsg;

typedef struct AuthSrv_SessionInfo {
    uint16_t header;
    uint32_t server_salt;
    uint32_t unk0;
    uint32_t unk1;
} AuthSrv_SessionInfo;

typedef struct AuthSrv_RequestResponse {
    uint16_t header;
    uint32_t req_id;
    uint32_t status;
} AuthSrv_RequestResponse;

typedef struct AuthSrv_AccountInfo {
    uint16_t header;
    uint32_t req_id;
    uint32_t h0006;
    uint32_t h000A;
    uint8_t  h000E[8];
    uint8_t  h0016[8];
    uint8_t  account_uuid[16];
    uint8_t  character_uuid[16];
    uint32_t h003E;
    uint32_t n_unk4;
    uint8_t  unk4[200];
    uint8_t  eula_accepted;
    uint32_t unk5;
} AuthSrv_AccountInfo;

typedef struct AuthSrv_CharacterInfo {
    uint16_t header;
    uint32_t req_id;
    uint8_t  uuid[16];
    uint32_t unk0;
    uint32_t n_name;
    uint16_t name[20];
    uint32_t n_extended;
    uint8_t  extended[64];
} AuthSrv_CharacterInfo;

typedef struct AuthSrv_FriendStreamEnd {
    uint16_t header;
    uint32_t unk1;
    uint32_t unk2;
} AuthSrv_FriendStreamEnd;

typedef struct AuthSrv_ServerHeartbeat {
    uint16_t header;
    uint32_t tick;
} AuthSrv_ServerHeartbeat;

typedef struct AuthSrv_GameServerInfo {
    uint16_t header;
    uint32_t req_id;
    uint32_t map_token;
    uint32_t map_id;
    uint8_t  host[24];
    uint32_t player_token;
} AuthSrv_GameServerInfo;

typedef union AuthSrvMsg {
    uint16_t                  header;
    uint8_t                   buffer[MSG_MAX_BUFFER_SIZE];
    AuthSrv_ServerHeartbeat   heartbeat;
    AuthSrv_AccountSettings   account_settings;
    AuthSrv_SessionInfo       session_info;
    AuthSrv_RequestResponse   request_response;
    AuthSrv_AccountInfo       account_info;
    AuthSrv_CharacterInfo     character_info;
    AuthSrv_FriendStreamEnd   friend_stream_end;
    AuthSrv_GameServerInfo    game_server_info;
} AuthSrvMsg;
#pragma pack(pop)

typedef array(AuthCliMsg *) AuthCliMsgArray;
