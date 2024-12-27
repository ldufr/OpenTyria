#pragma once

bool slice_u16_equals_ascii_lit(slice_uint16_t s1, const char *s2, size_t len)
{
    if (s1.len != len) {
        return false;
    }

    for (size_t idx = 0; idx < s1.len; ++idx) {
        if (s1.ptr[idx] != (uint16_t) s2[idx]) {
            return false;
        }
    }

    return true;
}

GameSrvMsg* GameSrv_BuildChatMessageCoreMsg(GameSrv *srv, slice_uint16_t data, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHAT_MESSAGE_CORE);
    GameSrv_ChatMessageCore *msg = &buffer->chat_message_core;

    assert(data.len <= sizeof(msg->msg_buf));
    if (sizeof(msg->msg_buf) < data.len) {
        log_error(
            "This function should be called with at most %u codepoints, but %u were given",
            sizeof(msg->msg_buf),
            data.len
        );
        return NULL;
    }

    msg->msg_len = (uint16_t) data.len;
    memcpy_u16(msg->msg_buf, data.ptr, msg->msg_len);
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendChatMessageCore(GameSrv *srv, GameConnection *conn, slice_uint16_t data)
{
    size_t count = (data.len + (CHAT_MESSAGE_FRAGMENT_MAX_LENGTH - 1)) / CHAT_MESSAGE_FRAGMENT_MAX_LENGTH;
    for (size_t idx = 0; idx < count; ++idx) {
        size_t offset = idx * CHAT_MESSAGE_FRAGMENT_MAX_LENGTH;
        slice_uint16_t frag;
        frag.ptr = data.ptr + offset;
        frag.len = min_size_t(data.len - offset, CHAT_MESSAGE_FRAGMENT_MAX_LENGTH);

        size_t size;
        GameSrvMsg *buffer = GameSrv_BuildChatMessageCoreMsg(srv, data, &size);
        GameConnection_SendMessage(conn, buffer, size);
    }
}

void GameSrv_BroadcastChatMessageCore(GameSrv *srv, slice_uint16_t data)
{
    size_t count = (data.len + (CHAT_MESSAGE_FRAGMENT_MAX_LENGTH - 1)) / CHAT_MESSAGE_FRAGMENT_MAX_LENGTH;
    for (size_t idx = 0; idx < count; ++idx) {
        size_t offset = idx * CHAT_MESSAGE_FRAGMENT_MAX_LENGTH;
        slice_uint16_t frag;
        frag.ptr = data.ptr + offset;
        frag.len = min_size_t(data.len - offset, CHAT_MESSAGE_FRAGMENT_MAX_LENGTH);

        size_t size;
        GameSrvMsg *buffer = GameSrv_BuildChatMessageCoreMsg(srv, data, &size);
        GameSrv_BroadcastMessage(srv, buffer, size);
    }
}

GameSrvMsg* GameSrv_BuildChatMessageLocalMsg(GameSrv *srv, uint16_t player_id_of_sender, Channel channel, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHAT_MESSAGE_LOCAL);
    GameSrv_ChatMessageLocal *msg = &buffer->chat_message_local;
    msg->player_id_of_sender = player_id_of_sender;
    msg->channel = channel;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendChatMessageLocal(GameSrv *srv, GameConnection *conn, uint16_t player_id_of_sender, Channel channel)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildChatMessageLocalMsg(srv, player_id_of_sender, channel, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastChatMessageLocal(GameSrv *srv, uint16_t player_id_of_sender, Channel channel)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildChatMessageLocalMsg(srv, player_id_of_sender, channel, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

GameSrvMsg* GameSrv_BuildChatMessageServerMsg(GameSrv *srv, Channel channel, size_t *size)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHAT_MESSAGE_SERVER);
    GameSrv_ChatMessageServer *msg = &buffer->chat_message_server;
    msg->channel = channel;
    *size = sizeof(*msg);
    return buffer;
}

void GameSrv_SendChatMessageServer(GameSrv *srv, GameConnection *conn, Channel channel)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildChatMessageServerMsg(srv, channel, &size);
    GameConnection_SendMessage(conn, buffer, size);
}

void GameSrv_BroadcastChatMessageServer(GameSrv *srv, Channel channel)
{
    size_t size;
    GameSrvMsg *buffer = GameSrv_BuildChatMessageServerMsg(srv, channel, &size);
    GameSrv_BroadcastMessage(srv, buffer, size);
}

void GameSrv_SendInvalidCommand(GameSrv *srv, uint16_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Failed to get player struct");
        return;
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        log_error("Failed to get player connection");
        return;
    }

    slice_uint16_t msg;
    msg.ptr = L"Unknown command";
    msg.len = (sizeof(L"Unknown command") - 1) / 2;

    array_uint16_t *builder = GameSrv_GetTextBuilder(srv);
    GmText_BuildLiteral(builder, msg);
    msg.ptr = builder->ptr;
    msg.len = builder->len;

    GameSrv_BroadcastChatMessageCore(srv, msg);
    GameSrv_SendChatMessageServer(srv, conn, Channel_Global);

    array_clear(builder);
}

int GameSrv_HandleChatCommand(GameSrv *srv, uint16_t player_id, slice_uint16_t msg)
{
    if (slice_u16_equals_ascii_lit(msg, "stuck", sizeof("stuck") - 1)) {
        GmAgent *agent;
        if ((agent = GameSrv_GetAgentByPlayerId(srv, player_id)) != NULL) {
            GameSrv_BroadcastAgentPosition(srv, agent);
        } else {
            log_error("Player id %u doesn't have an agent", player_id);
        }
    } else {
        GameSrv_SendInvalidCommand(srv, player_id);
    }

    return ERR_OK;
}

int GameSrv_HandleLocalMessage(GameSrv *srv, uint16_t player_id, Channel channel, slice_uint16_t msg)
{
    int err;
    if ((err = GmText_ValidateUserMessage(msg)) != 0) {
        log_warn("User %u sent an invalid message", player_id);
        return err;
    }

    array_uint16_t *builder = GameSrv_GetTextBuilder(srv);
    GmText_BuildLiteral(builder, msg);

    msg.ptr = builder->ptr;
    msg.len = builder->len;
    GameSrv_BroadcastChatMessageCore(srv, msg);
    GameSrv_BroadcastChatMessageLocal(srv, player_id, channel);

    array_clear(builder);
    return ERR_OK;
}

int GameSrv_HandleChatMessage(GameSrv *srv, uint16_t player_id, GameSrv_ChatMessage *msg)
{
    if (msg->message_len == 0) {
        return 0;
    }

    slice_uint16_t data = {0};
    data.ptr = msg->message_buf + 1;
    data.len = msg->message_len - 1;

    switch (msg->message_buf[0]) {
    case '!': return GameSrv_HandleLocalMessage(srv, player_id, Channel_All, data);
    case '@':
        break;
    case '#': return GameSrv_HandleLocalMessage(srv, player_id, Channel_Group, data);
    case '$': return GameSrv_HandleLocalMessage(srv, player_id, Channel_Trade, data);
    case '%':
        break;
    case '"':
        break;
    case '/': return GameSrv_HandleChatCommand(srv, player_id, data);
    default:
        log_error("Player id %u tried to send a message on channel '%c' (%u)", player_id, data.ptr[0], data.ptr[0]);
        return ERR_BAD_USER_DATA;
    }

    return 0;
}
