#pragma once

// This is way too nice, but we are not implemented a real server anyway...
#define MAXIMUM_ALLOWED_CORRECTION 100.f

bool GameSrv_AgentBaseClassIsPlayer(GmAgent *agent)
{
    return (agent->model_id & CHAR_CLASS_BASE_MASK) == CHAR_CLASS_PLAYER_BASE;
}

GmAgent* GameSrv_CreateAgent(GameSrv *srv)
{
    uint32_t agent_id = GmIdAllocate(&srv->agents.base, sizeof(srv->agents.buffer[0]));
    GmAgent *agent = &srv->agents.buffer[agent_id];
    memset(agent, 0, sizeof(*agent));
    agent->agent_id = agent_id;
    return agent;
}

GmAgent* GameSrv_GetAgent(GameSrv *srv, uint32_t agent_id)
{
    if (srv->agents.size <= agent_id) {
        return NULL;
    }
    GmAgent *agent = &srv->agents.buffer[agent_id];
    if (agent->agent_id != agent_id) {
        return NULL;
    }
    return agent;
}

GmAgent* GameSrv_GetAgentOrAbort(GameSrv *srv, uint32_t agent_id)
{
    GmAgent *agent;
    if ((agent = GameSrv_GetAgent(srv, agent_id)) == NULL) {
        abort();
    }
    return agent;
}

void GameSrv_RemoveAgentById(GameSrv *srv, uint32_t agent_id)
{
    GmAgent *agent;
    if ((agent = GameSrv_GetAgent(srv, agent_id)) == NULL) {
        return;
    }

    agent->agent_id = 0;
    array_reset(&agent->waypoints);
    GmIdFree(&srv->agents.base, agent_id, sizeof(srv->agents.buffer[0]));

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_WORLD_REMOVE_AGENT);
    GameSrv_AgentRemove *msg = &buffer->agent_remove;
    msg->agent_id = agent_id;

    GameSrv_BroadcastMessage(srv, buffer, sizeof(*msg));
}

GmAgent* GameSrv_GetAgentByPlayerId(GameSrv *srv, uint32_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        return NULL;
    }

    GmAgent *agent;
    if ((agent = GameSrv_GetAgent(srv, player->agent_id)) == NULL) {
        return NULL;
    }

    return agent;
}

void GameSrv_CancelAgentMovement(GmAgent *agent)
{
    agent->destination = agent->position;
    agent->speed = 0.f;
}

void GameSrv_SendAgentHealthEnergy(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_PROPERTY_UPDATE_INT);
        GameSrv_UpdateAgentIntProperty *msg = &buffer->update_agent_int_property;
        msg->agent_id = agent->agent_id;
        msg->prop_id = AgentProperty_Energy;
        msg->value = agent->energy_max;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
        msg->prop_id = AgentProperty_Health;
        msg->value = agent->health_max;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_PROPERTY_UPDATE_FLOAT);
        GameSrv_UpdateAgentFloatProperty *msg = &buffer->update_agent_float_property;
        msg->agent_id = agent->agent_id;
        msg->prop_id = AgentProperty_EnergyRegen;
        msg->value = agent->energy_per_sec;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }
}

GameSrvMsg* GameSrv_BuildAgentLevelMsg(GameSrv *srv, GmAgent *agent, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_PROPERTY_UPDATE_INT);
    GameSrv_UpdateAgentIntProperty *msg = &buffer->update_agent_int_property;
    msg->agent_id = agent->agent_id;
    msg->prop_id = AgentProperty_PublicLevel;
    msg->value = agent->level;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendAgentLevel(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAgentLevelMsg(srv, agent, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastAgentLevel(GameSrv *srv, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAgentLevelMsg(srv, agent, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

void GameSrv_SendAgentLoadTime(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_WORLD_UPDATE_LOAD_TIME);
    GameSrv_AgentLoadTime *msg = &buffer->agent_load_time;
    msg->load_time = agent->load_time;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

GameSrvMsg* GameSrv_BuildCreateAgentMsg(GameSrv *srv, GmAgent *agent, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_WORLD_CREATE_AGENT);
    GameSrv_CreateAgentMsg *msg = &buffer->create_agent;
    msg->agent_id = agent->agent_id;
    msg->model_id = agent->model_id;
    msg->agent_type = agent->agent_type;
    msg->h000B = 5;
    msg->position = agent->position.v2;
    msg->plane = agent->position.plane;
    msg->direction = agent->direction;
    msg->h001E = 1;
    msg->speed_base = agent->speed_base;
    msg->h0023 = 1.f;
    msg->h0027 = 0x41400000;
    msg->player_team_token = agent->player_team_token;
    msg->h003B = 0;
    msg->h004B.x = HUGE_VALF;
    msg->h004B.y = HUGE_VALF;
    msg->h0059.x = HUGE_VALF;
    msg->h0059.y = HUGE_VALF;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendCreateAgent(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildCreateAgentMsg(srv, agent, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastCreateAgent(GameSrv *srv, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildCreateAgentMsg(srv, agent, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

GameSrvMsg* GameSrv_BuildAgentInitialEffectsMsg(GameSrv *srv, GmAgent *agent, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_INITIAL_EFFECTS);
    GameSrv_InitialAgentEffects *msg = &buffer->initial_agent_effects;
    msg->agent_id = agent->agent_id;
    msg->effects = agent->effects;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendAgentInitialEffects(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAgentInitialEffectsMsg(srv, agent, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastAgentInitialEffects(GameSrv *srv, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAgentInitialEffectsMsg(srv, agent, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

GameSrvMsg* GameSrv_BuildUpdateAgentVisualEquipment(GameSrv *srv, GmAgent *agent, GmBagArray *bags, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_UPDATE_AGENT_VISUAL_EQUIPMENT);
    GameSrv_UpdateAgentVisualEquipment *msg = &buffer->update_agent_visual_equipment;

    msg->agent_id = agent->agent_id;
    if (bags->equipped_items.slot_count <= EquippedItemSlot_Count) {
        uint32_t *items = bags->equipped_items.items;

        // @Cleanup: We really only want to show the weapon in an explorable zone.
        msg->weapon_item_id = items[EquippedItemSlot_Weapon];
        msg->offhand_item_id = items[EquippedItemSlot_OffHand];
        msg->body_item_id = items[EquippedItemSlot_Body];
        msg->boots_item_id = items[EquippedItemSlot_Legs];
        msg->legs_item_id = items[EquippedItemSlot_Head];
        msg->gloves_item_id = items[EquippedItemSlot_Boots];
        msg->head_item_id = items[EquippedItemSlot_Gloves];
        msg->costume_head_item_id = items[EquippedItemSlot_CostumeBody];
        msg->costume_body_item_id = items[EquippedItemSlot_CostumeHead];
    } else {
        log_error("Expected %u item in equipped item bag, but found %u", EquippedItemSlot_Count, bags->equipped_items.slot_count);
    }

    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendUpdateAgentVisualEquipment(GameSrv *srv, GameConnection *conn, GmAgent *agent, GmBagArray *bags)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildUpdateAgentVisualEquipment(srv, agent, bags, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastUpdateAgentVisualEquipment(GameSrv *srv, GmAgent *agent, GmBagArray *bags)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildUpdateAgentVisualEquipment(srv, agent, bags, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

void GameSrv_SendUpdateControlledAgent(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_WORLD_UPDATE_CONTROLLED_AGENT);
    GameSrv_UpdatePlayerAgent *msg = &buffer->update_player_agent;
    msg->agent_id = agent->agent_id;
    msg->unk0 = 3;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_BroadcastAgentPosition(GameSrv *srv, GmAgent *agent)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_UPDATE_POSITION);
    GameSrv_UpdateAgentPostion *msg = &buffer->update_agent_position;
    msg->agent_id = agent->agent_id;
    msg->position = agent->position.v2;
    msg->plane = agent->position.plane;

    GameSrv_BroadcastMessage(srv, buffer, sizeof(*msg));
}

void GameSrv_BroadcastWorldSimulationTick(GameSrv *srv, uint32_t delta_ms)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_WORLD_SIMULATION_TICK);
    GameSrv_UpdateWorldSimulationTick *msg = &buffer->world_simulation_tick;
    assert(0 < delta_ms);
    msg->delta_ms = delta_ms;
    GameSrv_BroadcastMessage(srv, buffer, sizeof(*msg));
}

void GameSrv_BroadcastAgentStopMoving(GameSrv *srv, uint32_t agent_id)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_STOP_MOVING);
    GameSrv_AgentStopMoving *msg = &buffer->agent_stop_moving;
    msg->agent_id = agent_id;
    GameSrv_BroadcastMessage(srv, buffer, sizeof(*msg));
}

GameSrvMsg* GameSrv_BuildAgentDisplayCapeMsg(GameSrv *srv, GmAgent *agent, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_DISPLAY_CAPE);
    GameSrv_AgentDisplayCape *msg = &buffer->agent_display_cape;
    msg->agent_id = agent->agent_id;
    msg->status = 1; // @TODO: find this enum
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendAgentDisplayCape(GameSrv *srv, GameConnection *conn, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAgentDisplayCapeMsg(srv, agent, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastAgentDisplayCape(GameSrv *srv, GmAgent *agent)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAgentDisplayCapeMsg(srv, agent, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

void GameSrv_SendWorldAgents(GameSrv *srv, GameConnection *conn, GmAgent *player_agent)
{
    for (size_t idx = 1; idx < srv->agents.size; ++idx) {
        GmAgent *agent = &srv->agents.buffer[idx];
        if (agent->agent_id == 0 || agent->agent_id == player_agent->agent_id) {
            continue;
        }

        GmPlayer *player = NULL;
        if (GameSrv_AgentBaseClassIsPlayer(agent)) {
            if ((player = GameSrv_GetPlayer(srv, agent->model_id & ~CHAR_CLASS_BASE_MASK)) != NULL) {
                GameSrv_SendPlayerCreate(srv, conn, player);
                GameSrv_SendUpdatePlayerPartySize(srv, conn, player);
                GameSrv_BroadcastAddPlayerToPlayerParty(srv, player);
            } else {
                log_error("Couldn't find player %u for agent %u");
            }
        }


        GameSrv_SendCreateAgent(srv, conn, agent);
        GameSrv_SendAgentLevel(srv, conn, agent);
        GameSrv_SendAgentInitialEffects(srv, conn, agent);
        if (player != NULL) {
            GameSrv_SendItemsInBag(srv, conn, &player->bags.equipped_items);
            GameSrv_SendUpdateAgentVisualEquipment(srv, conn, agent, &player->bags);
        }
        GameSrv_SendAgentDisplayCape(srv, conn, agent);
    }
}

void GameSrv_BroadcastAgentMoveToPoint(GameSrv *srv, GmAgent *agent)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_MOVE_TO_POINT);
    GameSrv_MoveAgentToPoint *msg = &buffer->move_agent_to_point;
    msg->agent_id = agent->agent_id;
    msg->dest.x = agent->destination.x;
    msg->dest.y = agent->destination.y;
    msg->plane = agent->destination.plane;
    msg->current_plane = agent->position.plane;

    GameSrv_BroadcastMessage(srv, buffer, sizeof(*msg));
}

void GameSrv_UpdateAgentDestination(GameSrv *srv, GmAgent *agent, GmPos destination)
{
    GameSrv_WorldTick(srv);

    agent->speed = agent->speed_base;
    agent->destination = destination;
    float dx = destination.x - agent->position.x;
    float dy = destination.y - agent->position.y;
    float norm = sqrtf(dx * dx + dy * dy);
    agent->direction.x = dx / norm;
    agent->direction.y = dy / norm;
    agent->rotation = atan2f(agent->direction.y, agent->direction.x);

    GameSrv_BroadcastAgentMoveToPoint(srv, agent);
    // GameSrv_BroadcastAgentRotation(srv, agent);
}

int GameSrv_HandleMoveToCoord(GameSrv *srv, uint16_t player_id, GameSrv_MoveToCoord *msg)
{
    GmAgent *agent;
    if ((agent = GameSrv_GetAgentByPlayerId(srv, player_id)) == NULL) {
        log_error("Unknow player with player id %u", player_id);
        return ERR_SERVER_ERROR;
    }

    GmPos dest = {0};
    dest.v2 = msg->pos;
    dest.plane = (uint16_t) msg->plane;

    array_clear(&agent->waypoints);
    agent->waypoint_idx = 0;
    if (PathFinding(&srv->map_context.path, agent->position, dest, &agent->waypoints)) {
        if (0 < agent->waypoints.len) {
            dest = agent->waypoints.ptr[agent->waypoint_idx++].pos;
            GameSrv_UpdateAgentDestination(srv, agent, dest);
        }
    } else {
        log_warn("PathFinding failed");
        GameSrv_WorldTick(srv);
        GameSrv_CancelAgentMovement(agent);
        GameSrv_BroadcastAgentPosition(srv, agent);
    }

    return ERR_OK;
}

void GameSrv_UpdateAgentMovement(GameSrv *srv, GmAgent *agent, float delta)
{
    if (agent->speed == 0.f) {
        return;
    }

    float dist = delta * agent->speed;
    float dist_to_dest = Vec2f_Dist2(agent->position.v2, agent->destination.v2);

    if (dist_to_dest <= dist) {
        dist -= dist_to_dest;

        agent->position = agent->destination;
        if (agent->waypoint_idx < agent->waypoints.len) {
            GmPos next_pos = agent->waypoints.ptr[agent->waypoint_idx++].pos;
            GameSrv_UpdateAgentDestination(srv, agent, next_pos);
        } else {
            dist = 0.f;
        }
    }

    if (0.f < dist) {
        float dx = agent->direction.x * dist;
        float dy = agent->direction.y * dist;
        agent->position.x += dx;
        agent->position.y += dy;
    }

    if ((agent->position.plane == agent->destination.plane) &&
        Vec2fEqual(agent->position.v2, agent->destination.v2))
    {
        agent->destination = (GmPos) {0};
        agent->speed = 0.f;
        agent->waypoint_idx = 0;
        array_clear(&agent->waypoints);
    }
}

void GameSrv_WorldTick(GameSrv *srv)
{
    assert(srv->last_world_tick <= srv->current_frame_time);
    if (srv->current_frame_time <= srv->last_world_tick) {
        return;
    }

    int64_t delta_time = srv->current_frame_time - srv->last_world_tick;
    float delta = (float)delta_time / 1000.f;

    GameSrv_BroadcastWorldSimulationTick(srv, (uint32_t) delta_time);
    srv->last_world_tick = srv->current_frame_time;

    GmAgentArray agents = srv->agents;
    for (size_t idx = 0; idx < agents.size; ++idx) {
        GmAgent *agent = &agents.buffer[idx];
        if (agent->agent_id != idx) {
            continue;
        }

        GameSrv_UpdateAgentMovement(srv, agent, delta);

        float health_diff = delta * agent->health_per_sec;
        agent->health = clampf(agent->health + health_diff, 0.f, 1.f);
        // @TODO: Check if the agent is dead!!!

        float energy_diff = delta * agent->energy_per_sec;
        agent->energy = clampf(agent->energy + energy_diff, 0.f, 1.f);
    }
}

int GameSrv_HandleCancelMovement(GameSrv *srv, uint16_t player_id)
{
    GmAgent *agent;
    if ((agent = GameSrv_GetAgentByPlayerId(srv, player_id)) == NULL) {
        log_error("Unknow player with player id %u", player_id);
        return ERR_SERVER_ERROR;
    }

    GameSrv_WorldTick(srv);
    GameSrv_CancelAgentMovement(agent);
    GameSrv_BroadcastAgentStopMoving(srv, agent->agent_id);
    return ERR_OK;
}

int GameSrv_HandleLastPosOnMoveCanceled(GameSrv *srv, uint16_t player_id, GameSrv_LastPosBeforeMoveCanceled *msg)
{
    GmAgent *agent;
    if ((agent = GameSrv_GetAgentByPlayerId(srv, player_id)) == NULL) {
        log_error("Unknow player with player id %u", player_id);
        return ERR_SERVER_ERROR;
    }

    if (msg->plane != agent->position.plane || MAXIMUM_ALLOWED_CORRECTION < Vec2f_Dist2(agent->position.v2, msg->pos)) {
        log_warn("Player %u tried to correct the position by more than allowed", player_id);
        return ERR_OK;
    }

    agent->position.x = msg->pos.x;
    agent->position.y = msg->pos.y;
    agent->position.plane = (uint16_t) msg->plane;
    return ERR_OK;
}
