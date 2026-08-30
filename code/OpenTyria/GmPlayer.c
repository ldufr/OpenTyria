#pragma once

GmPlayer* GameSrv_CreatePlayer(
    GameSrv *srv,
    uintptr_t token,
    GmUuid account_id,
    GmUuid char_id)
{
    uint32_t player_id = GmIdAllocate(&srv->players.base, sizeof(srv->players.buffer[0]));

    GmPlayer *player = &srv->players.buffer[player_id];
    memset(player, 0, sizeof(*player));
    player->player_id = cast_u16(player_id);
    player->conn_token = token;
    player->account_id = account_id;
    player->char_id = char_id;

    return player;
}

void GameSrv_RemovePlayer(GameSrv *srv, uint32_t player_id)
{
    int err;

    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_warn("Tried to remove the player %zu which doesn't exist", player_id);
        return;
    }

    GameSrv_CloseConnection(srv, player->conn_token);
    player->conn_token = 0;

    // We remove the player id, ensuring we won't broadcast to this player
    // without releasing the memory yet.
    player->player_id = 0;

    if (player->agent_id != 0) {
        GameSrv_RemoveAgentById(srv, player->agent_id);
    }

    GameSrv_BroadcastPlayerDestroy(srv, player_id);

    if ((err = CtrlConn_SendPlayerLeft(&srv->ctrl_conn, player->account_id, player->char_id)) != 0) {
        log_error("Couldn't send 'PlayerLeft' on control channel");
    }

    GmIdFree(&srv->players.base, player_id, sizeof(srv->players.buffer[0]));
}

GmPlayer* GameSrv_GetPlayer(GameSrv *srv, uint32_t player_id)
{
    if ((player_id < srv->players.size) && (srv->players.buffer[player_id].player_id == player_id)) {
        return &srv->players.buffer[player_id];
    } else {
        return NULL;
    }
}

void GameSrv_SendHardModeUnlocked(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_HARD_MODE_UNLOCKED);
    GameSrv_HardModeUnlocked *msg = &buffer->hard_mode_unlocked;
    msg->hard_mode_unlocked = true;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendPlayerMaxFactions(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHARACTER_FACTION_MAX_KURZICK);
    buffer->kurzick_max.max_faction = player->account.kurzick_points_max;
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->kurzick_max));

    buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHARACTER_FACTION_MAX_LUXON);
    buffer->luxon_max.max_faction = player->account.luxon_points_max;
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->luxon_max));

    buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHARACTER_FACTION_MAX_BALTHAZAR);
    buffer->balthazar_max.max_faction = player->account.balthazar_points_max;
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->balthazar_max));

    buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHARACTER_FACTION_MAX_IMPERIAL);
    buffer->imperial_max.max_faction = player->account.imperial_points_max;
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->imperial_max));
}

void GameSrv_SendPlayerFactions(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHARACTER_UPDATE_FACTIONS);
    GameSrv_UpdatePlayerFactions *msg = &buffer->update_player_factions;
    msg->experience = player->character.experience;
    msg->current_kurzick = player->account.kurzick_points_amount;
    msg->total_earned_kurzick = player->account.kurzick_points_total;
    msg->current_luxon = player->account.luxon_points_amount;
    msg->total_earned_luxon = player->account.luxon_points_total;
    msg->current_imperial = player->account.imperial_points_amount;
    msg->total_earned_imperial = player->account.imperial_points_total;
    msg->unk_faction4 = 0;
    msg->unk_faction5 = 0;
    msg->level = 0;
    msg->morale = 0;
    msg->current_balth = player->account.balthazar_points_amount;
    msg->total_earned_balth = player->account.balthazar_points_total;
    msg->current_skill_points = player->character.skill_points;
    msg->total_earned_skill_points = player->character.skill_points_total;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendInstanceLoadPlayerName(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    assert(!uuid_is_null(&player->character.char_id));
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_LOAD_PLAYER_NAME);
    GameSrv_InstancePlayerName *msg = &buffer->instance_player_name;
    STATIC_ASSERT(ARRAY_SIZE(msg->name) <= ARRAY_SIZE(player->character.charname.buf));
    msg->n_name = (uint32_t) player->character.charname.len;
    memcpy_u16(msg->name, player->character.charname.buf, msg->n_name);
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendAgentAttributePoints(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_UPDATE_ATTRIBUTE_POINTS);
    GameSrv_UpdateAgentAttributePoints *msg = &buffer->update_agent_attribute_points;
    msg->agent_id = player->agent_id;
    msg->unused_points = 50;
    msg->used_points = 50;

    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendPlayerProfession(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_UPDATE_PROFESSION);
    GameSrv_UpdateProfession *msg = &buffer->update_profession;
    msg->agent_id = player->agent_id;
    msg->primary_profession = player->primary_profession;
    msg->is_pvp = false; // @Cleanup: use the character settings to fill this value
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendUnlockedProfessions(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_UPDATE_UNLOCKED_PROFESSIONS);
    GameSrv_UnlockedProfession *msg = &buffer->unlocked_profession;
    msg->agent_id = player->agent_id;
    msg->unlocked = player->character.unlocked_professions;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendSkillbarUpdate(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_SKILLBAR_UPDATE);
    GameSrv_SkillbarUpdate *msg = &buffer->skillbar_update;
    msg->agent_id = player->agent_id;
    msg->n_skills = 8;
    msg->skills[0] = player->character.skill1;
    msg->skills[1] = player->character.skill2;
    msg->skills[2] = player->character.skill3;
    msg->skills[3] = player->character.skill4;
    msg->skills[4] = player->character.skill5;
    msg->skills[5] = player->character.skill6;
    msg->skills[6] = player->character.skill7;
    msg->skills[7] = player->character.skill8;
    msg->n_pvp_masks = 8;
    msg->unk1 = 1;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendSkillsAndAttributes(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrv_SendAgentAttributePoints(srv, conn, player);
    GameSrv_SendPlayerProfession(srv, conn, player);
    GameSrv_SendUnlockedProfessions(srv, conn, player);
    GameSrv_SendSkillbarUpdate(srv, conn, player);
    // GameSrv_SendCartographyData
    GameSrv_SendUpdateUnlockedSkills(srv, conn, player);
}

void GameSrv_SendPlayerHealthEnergy(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_PROPERTY_UPDATE_INT);
        GameSrv_UpdateAgentIntProperty *msg = &buffer->update_agent_int_property;
        msg->agent_id = player->agent_id;
        msg->prop_id = AgentProperty_Energy;
        msg->value = 20;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
        msg->prop_id = AgentProperty_Health;
        msg->value = 100;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
        msg->prop_id = AgentProperty_Value64;
        msg->value = 0;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_PROPERTY_UPDATE_FLOAT);
        GameSrv_UpdateAgentFloatProperty *msg = &buffer->update_agent_float_property;
        msg->agent_id = player->agent_id;
        msg->prop_id = AgentProperty_EnergyRegen;
        msg->value = 0.033f;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }
}

void GameSrv_SendUnlockedMaps(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    DbCharacter *ch = &player->character;

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_MAP_UPDATE_UNLOCKED_LIST);
    GameSrv_UnlockedMaps *msg = &buffer->unlocked_maps;

    STATIC_ASSERT(sizeof(ch->completed_missions_nm.bitmap) <= sizeof(msg->completed_missions_nm_buf));
    msg->completed_missions_nm_len = (uint32_t) bitmap_length(ch->completed_missions_nm);
    memcpy_u32(msg->completed_missions_nm_buf, ch->completed_missions_nm.bitmap, msg->completed_missions_nm_len);

    STATIC_ASSERT(sizeof(ch->completed_bonuses_nm.bitmap) <= sizeof(msg->completed_bonuses_nm_buf));
    msg->completed_bonuses_nm_len = (uint32_t) bitmap_length(ch->completed_bonuses_nm);
    memcpy_u32(msg->completed_bonuses_nm_buf, ch->completed_bonuses_nm.bitmap, sizeof(msg->completed_bonuses_nm_buf));

    STATIC_ASSERT(sizeof(ch->completed_missions_hm.bitmap) <= sizeof(msg->completed_missions_hm_buf));
    msg->completed_missions_hm_len = (uint32_t) bitmap_length(ch->completed_missions_hm);
    memcpy_u32(msg->completed_missions_hm_buf, ch->completed_missions_hm.bitmap, sizeof(msg->completed_missions_hm_buf));

    STATIC_ASSERT(sizeof(ch->completed_bonuses_hm.bitmap) <= sizeof(msg->completed_bonuses_hm_buf));
    msg->completed_bonuses_hm_len = (uint32_t) bitmap_length(ch->completed_bonuses_hm);
    memcpy_u32(msg->completed_bonuses_hm_buf, ch->completed_bonuses_hm.bitmap, sizeof(msg->completed_bonuses_hm_buf));

    STATIC_ASSERT(sizeof(ch->unlocked_maps.bitmap) <= sizeof(msg->unlocked_maps_buf));
    msg->unlocked_maps_len = (uint32_t) bitmap_length(ch->unlocked_maps);
    memcpy_u32(msg->unlocked_maps_buf, ch->unlocked_maps.bitmap, sizeof(msg->unlocked_maps_buf));

    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendUpdatePvpUnlockedSkills(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    DbCharacter *ch = &player->character;

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PVP_UPDATE_UNLOCKED_SKILLS);
    GameSrv_UpdatePvpUnlockedSkills *msg = &buffer->update_pvp_unlocked_skills;

    STATIC_ASSERT(sizeof(ch->unlocked_skills.bitmap) <= sizeof(msg->unlocked_skills_buf));
    msg->unlocked_skills_len = (uint32_t) bitmap_length(ch->unlocked_skills);
    memcpy_u32(msg->unlocked_skills_buf, ch->unlocked_skills.bitmap, msg->unlocked_skills_len);
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendUpdateUnlockedSkills(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    DbCharacter *ch = &player->character;

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_UPDATE_UNLOCKED_SKILLS);
    GameSrv_UpdatePveUnlockedSkills *msg = &buffer->update_pve_unlocked_skills;

    STATIC_ASSERT(sizeof(ch->unlocked_skills.bitmap) <= sizeof(msg->unlocked_skills_buf));
    msg->unlocked_skills_len = (uint32_t) bitmap_length(ch->unlocked_skills);
    memcpy_u32(msg->unlocked_skills_buf, ch->unlocked_skills.bitmap, msg->unlocked_skills_len);
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendPlayerAttributes(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_AGENT_UPDATE_ATTRIBUTES);
    GameSrv_UpdateAgentAttributes *msg = &buffer->update_agent_attributes;
    msg->agent_id = player->agent_id;
    msg->data_len = Attribute_Count;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

GameSrvMsg* GameSrv_BuildPlayerCreate(GameSrv *srv, GmPlayer *player, size_t *size)
{
    Appearance appearance = {0};
    appearance.sex = player->character.sex;
    appearance.height = player->character.height;
    appearance.skin_color = player->character.skin_color;
    appearance.hair_color = player->character.hair_color;
    appearance.face_style = player->character.face_style;
    appearance.primary_profession = player->character.primary_profession;
    appearance.hair_style = player->character.hair_style;
    appearance.race = player->character.race;

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_CREATE);
    GameSrv_PlayerCreate *msg = &buffer->player_create;
    msg->player_id = player->player_id;
    msg->agent_id = player->agent_id;
    memcpy(&msg->appearance, &appearance, sizeof(msg->appearance));
    msg->unk0 = 0;
    msg->unk1 = 0;
    msg->unk2 = 0;
    STATIC_ASSERT(sizeof(player->character.charname.buf) <= sizeof(msg->name_buf));
    msg->name_len = (uint16_t) player->character.charname.len;
    memcpy_u16(msg->name_buf, player->character.charname.buf, msg->name_len);
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendPlayerCreate(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildPlayerCreate(srv, player, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastPlayerCreate(GameSrv *srv, GmPlayer *player)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildPlayerCreate(srv, player, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

void GameSrv_BroadcastPlayerDestroy(GameSrv *srv, uint32_t player_id)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_DESTROY);
    GameSrv_PlayerDestroy *msg = &buffer->player_destroy;
    msg->player_id = player_id;
    GameSrv_BroadcastMessage(srv, buffer, sizeof(*msg));
}

GameSrvMsg* GameSrv_BuildUpdatePlayerPartySize(GameSrv *srv, GmPlayer *player, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_UPDATE_PARTY_SIZE);
    GameSrv_UpdatePlayerPartySize *msg = &buffer->update_player_party_size;
    msg->player_id = player->player_id;
    msg->party_size = 1;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendUpdatePlayerPartySize(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildUpdatePlayerPartySize(srv, player, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastUpdatePlayerPartySize(GameSrv *srv, GmPlayer *player)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildUpdatePlayerPartySize(srv, player, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

GameSrvMsg* GameSrv_BuildAddPlayerToPlayerParty(GameSrv *srv, GmPlayer *player, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_UPDATE_PARTY);
    GameSrv_AddPlayerToPlayerParty *msg = &buffer->add_player_to_player_party;
    // @Cleanup:
    // When a player isn't in any party, we add this player to it's own party.
    msg->player_id = player->player_id;
    msg->party_leader_player_id = player->player_id;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendAddPlayerToPlayerParty(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAddPlayerToPlayerParty(srv, player, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastAddPlayerToPlayerParty(GameSrv *srv, GmPlayer *player)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildAddPlayerToPlayerParty(srv, player, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}
