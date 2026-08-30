#pragma once

typedef enum AgentType {
    AgentType_None   = 0,
    AgentType_Living = 1,
    AgentType_Gadget = 2,
    AgentType_Item   = 3,
} AgentType;

#define CHAR_CLASS_BASE_MASK    0xF0000000
#define CHAR_CLASS_PLAYER_BASE  0x30000000
#define CHAR_CLASS_MONSTER_BASE 0x20000000

typedef struct GmAgent {
    GmIdHeader    IdHeader;
    uint32_t      agent_id;
    GmPos         position;
    Vec2f         direction;
    GmPos         destination;
    float         rotation;
    uint32_t      model_id;
    uint32_t      load_time;
    uint32_t      health_max;
    uint32_t      energy_max;
    float         health_per_sec;
    float         energy_per_sec;
    float         health;
    float         energy;
    AgentType     agent_type;
    float         speed_base;
    float         speed;
    uint32_t      level;
    uint32_t      effects;
    uint32_t      player_team_token;
    size_t        waypoint_idx;
    WaypointArray waypoints;
} GmAgent;
typedef GmIdTable(GmAgent) GmAgentArray;

GmAgent* GameSrv_CreateAgent(GameSrv *srv);
GmAgent* GameSrv_GetAgent(GameSrv *srv, uint32_t agent_id);
GmAgent* GameSrv_GetAgentOrAbort(GameSrv *srv, uint32_t agent_id);
void     GameSrv_RemoveAgentById(GameSrv *srv, uint32_t agent_id);
GmAgent* GameSrv_GetAgentByPlayerId(GameSrv *srv, uint32_t player_id);

void GameSrv_CancelAgentMovement(GmAgent *agent);

void GameSrv_SendAgentHealthEnergy(GameSrv *srv, GameConnection *conn, GmAgent *agent);
GameSrvMsg* GameSrv_BuildAgentLevelMsg(GameSrv *srv, GmAgent *agent, size_t *size);
void GameSrv_SendAgentLevel(GameSrv *srv, GameConnection *conn, GmAgent *agent);
void GameSrv_BroadcastAgentLevel(GameSrv *srv, GmAgent *agent);
void GameSrv_SendAgentLoadTime(GameSrv *srv, GameConnection *conn, GmAgent *agent);
GameSrvMsg* GameSrv_BuildCreateAgentMsg(GameSrv *srv, GmAgent *agent, size_t *size);
void GameSrv_SendCreateAgent(GameSrv *srv, GameConnection *conn, GmAgent *agent);
void GameSrv_BroadcastCreateAgent(GameSrv *srv, GmAgent *agent);
GameSrvMsg* GameSrv_BuildAgentInitialEffectsMsg(GameSrv *srv, GmAgent *agent, size_t *size);
void GameSrv_SendAgentInitialEffects(GameSrv *srv, GameConnection *conn, GmAgent *agent);
void GameSrv_BroadcastAgentInitialEffects(GameSrv *srv, GmAgent *agent);
void GameSrv_BroadcastUpdateAgentVisualEquipment(GameSrv *srv, GmAgent *agent, GmBagArray *bags);
void GameSrv_SendUpdateControlledAgent(GameSrv *srv, GameConnection *conn, GmAgent *agent);
void GameSrv_BroadcastAgentPosition(GameSrv *srv, GmAgent *agent);
void GameSrv_BroadcastWorldSimulationTick(GameSrv *srv, uint32_t delta_ms);
void GameSrv_BroadcastAgentStopMoving(GameSrv *srv, uint32_t agent_id);
GameSrvMsg* GameSrv_BuildAgentDisplayCapeMsg(GameSrv *srv, GmAgent *agent, size_t *size);
void GameSrv_SendAgentDisplayCape(GameSrv *srv, GameConnection *conn, GmAgent *agent);
void GameSrv_BroadcastAgentDisplayCape(GameSrv *srv, GmAgent *agent);
void GameSrv_SendWorldAgents(GameSrv *srv, GameConnection *conn, GmAgent *player_agent);
void GameSrv_WorldTick(GameSrv *srv);

int GameSrv_HandleMoveToCoord(GameSrv *srv, uint16_t player_id, GameSrv_MoveToCoord *msg);
int GameSrv_HandleCancelMovement(GameSrv *srv, uint16_t player_id);
int GameSrv_HandleLastPosOnMoveCanceled(GameSrv *srv, uint16_t player_id, GameSrv_LastPosBeforeMoveCanceled *msg);
