#pragma once

typedef enum Channel {
    Channel_Alliance  = 0,
    Channel_Allies    = 1, // coop with two groups for instance.
    Channel_Unknown2  = 2,
    Channel_All       = 3,
    Channel_Unknown4  = 4,
    Channel_Moderator = 5,
    Channel_Emote     = 6,
    Channel_Warning   = 7, // shows in the middle of the screen and does not parse <c> tags
    Channel_Unknown8  = 8,
    Channel_Guild     = 9,
    Channel_Global    = 10,
    Channel_Group     = 11,
    Channel_Trade     = 12,
    Channel_Advisory  = 13,
    Channel_Whisper   = 14,
    Channel_Count,
} Channel;

GameSrvMsg* GameSrv_BuildChatMessageCoreMsg(GameSrv *srv, slice_uint16_t data, size_t *size);
void GameSrv_SendChatMessageCore(GameSrv *srv, GameConnection *conn, slice_uint16_t data);
void GameSrv_BroadcastChatMessageCore(GameSrv *srv, slice_uint16_t data);
GameSrvMsg* GameSrv_BuildChatMessageLocalMsg(GameSrv *srv, uint16_t player_id_of_sender, Channel channel, size_t *size);
void GameSrv_SendChatMessageLocal(GameSrv *srv, GameConnection *conn, uint16_t player_id_of_sender, Channel channel);
void GameSrv_BroadcastChatMessageLocal(GameSrv *srv, uint16_t player_id_of_sender, Channel channel);
GameSrvMsg* GameSrv_BuildChatMessageServerMsg(GameSrv *srv, Channel channel, size_t *size);
void GameSrv_SendChatMessageServer(GameSrv *srv, GameConnection *conn, Channel channel);
void GameSrv_BroadcastChatMessageServer(GameSrv *srv, Channel channel);
void GameSrv_SendInvalidCommand(GameSrv *srv, uint16_t player_id);
int GameSrv_HandleChatCommand(GameSrv *srv, uint16_t player_id, slice_uint16_t msg);
int GameSrv_HandleLocalMessage(GameSrv *srv, uint16_t player_id, Channel channel, slice_uint16_t msg);
int GameSrv_HandleChatMessage(GameSrv *srv, uint16_t player_id, GameSrv_ChatMessage *msg);
