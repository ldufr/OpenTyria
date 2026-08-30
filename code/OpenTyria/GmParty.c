#pragma once

void GameSrv_FastDeleteParty(GameSrv *srv, uint32_t party_id)
{
    assert(party_id < srv->parties.size);
    srv->parties.buffer[party_id].party_id = 0;
    GmIdFree(&srv->parties.base, party_id, sizeof(srv->parties.buffer[0]));
}

GmParty* GameSrv_CreateParty(GameSrv *srv)
{
    uint32_t party_id = GmIdAllocate(&srv->parties.base, sizeof(srv->parties.buffer[0]));

    if ((uint32_t)UINT16_MAX < party_id) {
        GameSrv_FastDeleteParty(srv, party_id);
        return NULL;
    }

    GmParty *party = &srv->parties.buffer[party_id];
    memset(party, 0, sizeof(*party));
    party->party_id = cast_u16(party_id);
    return party;
}

GmParty* GameSrv_GetParty(GameSrv *srv, uint16_t party_id)
{
    if (srv->parties.size <= party_id) {
        return NULL;
    }

    GmParty *party = &srv->parties.buffer[party_id];
    if (party->party_id != party_id) {
        return NULL;
    }

    return party;
}

void GmParty_AddPlayer(GmParty *party, uint32_t agent_id, uint16_t player_id)
{
    GmPartyPlayer *pp = array_push(&party->players, 1);
    pp->agent_id = agent_id;
    pp->player_id = player_id;
    pp->ready = true;
    pp->connected = true;
}

void GameSrv_SendCreateParty(GameSrv *srv, GameConnection *conn, uint16_t party_id)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PARTY_CREATE);
    GameSrv_CreatePartyMsg *msg = &buffer->create_party;
    msg->party_id = party_id;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendAddPartyPlayer(GameSrv *srv, GameConnection *conn, uint16_t party_id, GmPartyPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PARTY_PLAYER_ADD);
    GameSrv_AddPartyPlayer *msg = &buffer->add_party_player;
    msg->party_id = party_id;
    msg->player_id = player->player_id;
    msg->is_loaded = player->connected;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendAddPartyHero(GameSrv *srv, GameConnection *conn, uint16_t party_id, GmPartyHero *hero)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PARTY_HERO_ADD);
    GameSrv_AddPartyHero *msg = &buffer->add_party_hero;
    msg->party_id = party_id;
    msg->owner_player_id = hero->owner_player_id;
    msg->agent_id = cast_u16(hero->agent_id);
    msg->hero_id = cast_u8(hero->hero_id);
    msg->level = cast_u8(hero->level);
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendPartyMemberStreamEnd(GameSrv *srv, GameConnection *conn, uint16_t party_id)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PARTY_MEMBER_STREAM_END);
    GameSrv_PartyMemberStreamEnd *msg = &buffer->party_member_stream_end;
    msg->party_id = party_id;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendUpdatePlayerParty(GameSrv *srv, GameConnection *conn, uint16_t party_id)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHARACTER_UDPATE_PARTY);
    GameSrv_UpdatePlayerParty *msg = &buffer->update_player_party;
    msg->party_id = party_id;
    msg->unknown = 1;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendPlayerParty(GameSrv *srv, GameConnection *conn, uint16_t party_id)
{
    GmParty *party;
    if ((party = GameSrv_GetParty(srv, party_id)) == NULL) {
        log_error("Invalid party id %u", party_id);
        return;
    }

    GameSrv_SendCreateParty(srv, conn, party_id);

    for (size_t idx = 0; idx < party->players.len; ++idx) {
        GmPartyPlayer *player = &party->players.ptr[idx];
        GameSrv_SendAddPartyPlayer(srv, conn, party_id, player);
        // @Cleanup: send player tick?
    }

    for (size_t idx = 0; idx < party->heroes.len; ++idx) {
        GmPartyHero *hero = &party->heroes.ptr[idx];
        GameSrv_SendAddPartyHero(srv, conn, party_id, hero);
    }

    // for (size_t idx = 0; idx < party->henchmans.len; ++idx) {
    // }

    GameSrv_SendPartyMemberStreamEnd(srv, conn, party_id);
}
