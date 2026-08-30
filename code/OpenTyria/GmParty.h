#pragma once

typedef enum Difficulty {
    Difficulty_Normal = 0,
    Difficulty_Hard   = 1,
} Difficulty;

typedef struct GmPartyPlayer {
    uint32_t agent_id;
    uint16_t player_id;
    bool     ready;
    bool     connected;
} GmPartyPlayer;

typedef struct GmPartyHench {
    uint32_t   agent_id;
    uint32_t   hench_id;
    uint32_t   level;
    Profession profession;
} GmPartyHench;

typedef struct GmPartyHero {
    uint32_t agent_id;
    uint16_t owner_player_id;
    uint32_t hero_id;
    uint32_t level;
} GmPartyHero;

typedef array(GmPartyHero)   GmPartyHeroArray;
typedef array(GmPartyPlayer) GmPartyPlayerArray;
typedef array(GmPartyHench)  GmPartyHenchArray;

typedef struct GmParty {
    GmIdHeader         IdHeader;
    uint16_t           party_id;
    Difficulty         difficulty;
    uint8_t            player_max;
    uint8_t            player_count;
    GmPartyPlayerArray players;
    GmPartyHeroArray   heroes;
    GmPartyHenchArray  henchmans;
} GmParty;
typedef GmIdTable(GmParty) GmPartyArray;

GmParty* GameSrv_CreateParty(GameSrv *srv);
GmParty* GameSrv_GetParty(GameSrv *srv, uint16_t party_id);
void GmParty_AddPlayer(GmParty *party, uint32_t agent_id, uint16_t player_id);

void GameSrv_SendCreateParty(GameSrv *srv, GameConnection *conn, uint16_t party_id);
void GameSrv_SendAddPartyPlayer(GameSrv *srv, GameConnection *conn, uint16_t party_id, GmPartyPlayer *player);
void GameSrv_SendAddPartyHero(GameSrv *srv, GameConnection *conn, uint16_t party_id, GmPartyHero *hero);
void GameSrv_SendPartyMemberStreamEnd(GameSrv *srv, GameConnection *conn, uint16_t party_id);
void GameSrv_SendUpdatePlayerParty(GameSrv *srv, GameConnection *conn, uint16_t party_id);
void GameSrv_SendPlayerParty(GameSrv *srv, GameConnection *conn, uint16_t party_id);
