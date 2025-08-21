#define IPV4_MAX_LENGTH sizeof("123.123.123.123:65535")
#define IPV6_MAX_LENGTH sizeof("[e9c2:e2e3:ea8b:9f36:e151:9b65:b631:c87d]:65535")
#define AUTH_READ_CHUNK_SIZE 4096

mbedtls_mpi prime_modulus;
mbedtls_mpi server_private;

typedef struct RequestGameInstanceParams {
    uint16_t         map_id;
    MapType          map_type;
    DistrictRegion   region;
    DistrictLanguage language;
    uint32_t         district_number;
} RequestGameInstanceParams;

void arc4_hash(const uint8_t *key, uint8_t *digest)
{
    uint32_t A = 0x67452301;
    uint32_t B = 0xEFCDAB89;
    uint32_t C = 0x98BADCFE;
    uint32_t D = 0x10325476;
    uint32_t E = 0xC3D2E1F0;
    uint32_t F;

    uint32_t input[5];
    input[0] = le32dec(&key[0]);
    input[1] = le32dec(&key[4]);
    input[2] = le32dec(&key[8]);
    input[3] = le32dec(&key[12]);
    input[4] = le32dec(&key[16]);

    F = (D ^ (B & (C ^ D)));
    E += input[0] + ROL32(A, 5) + F + 0x5A827999;
    B = ROL32(B, 30);

    F = (C ^ (A & (B ^ C)));
    D += input[1] + ROL32(E, 5) + F + 0x5A827999;
    A = ROL32(A, 30);

    F = (B ^ (E & (A ^ B)));
    C += input[2] + ROL32(D, 5) + F + 0x5A827999;
    E = ROL32(E, 30);

    F = (B ^ C ^ D);
    E += input[3] + ROL32(A, 5) + F + 0x6ED9EBA1;
    B = ROL32(B, 30);

    F = (A ^ B ^ C);
    D += input[4] + ROL32(E, 5) + F + 0x6ED9EBA1;
    A = ROL32(A, 30);

    le32enc(&digest[0], input[0] + A);
    le32enc(&digest[4], input[1] + B);
    le32enc(&digest[8], input[2] + C);
    le32enc(&digest[12], input[3] + D);
    le32enc(&digest[16], input[4] + E);
}

void Connection_Free(Connection *conn)
{
    IoSource_free(&conn->source);
}

void Connection_RemoveIncoming(Connection *conn, size_t count)
{
    if (count < conn->n_incoming) {
        size_t rem = conn->n_incoming - count;
        memmove(conn->incoming, &conn->incoming[count], rem);
        conn->n_incoming = rem;
    } else {
        conn->n_incoming = 0;
    }
}

int Connection_ReadVersion(AuthSrv *srv, Connection *conn)
{
    size_t consumed = 0;
    const uint8_t *data = conn->incoming;
    size_t size = conn->n_incoming;

    uint32_t header;
    if (!read_u32(data, size, &header, &consumed)) {
        return ERR_WOULDBLOCK;
    }

    if (header == AUTH_CMSG_VERSION_HEADER) {
        if (size < sizeof(AUTH_CMSG_VERSION)) {
            return ERR_WOULDBLOCK;
        }

        AUTH_CMSG_VERSION version;
        version.header = header;
        if (!read_u32(data, size, &version.version, &consumed) ||
            !read_u32(data, size, &version.h0008, &consumed) ||
            !read_u32(data, size, &version.h000C, &consumed))
        {
            return ERR_WOULDBLOCK;
        }

        Connection_RemoveIncoming(conn, consumed);
        conn->state = ConnState_AwaitKeyExchange;
        conn->type = ConnType_Auth;
        conn->auth_version = version;
        return ERR_OK;
    } else if (header == GAME_CMSG_VERSION_HEADER) {
        if (size < sizeof(GAME_CMSG_VERSION)) {
            return ERR_WOULDBLOCK;
        }

        GAME_CMSG_VERSION version;
        version.header = header;
        if (!read_u32(data, size, &version.version, &consumed) ||
            !read_u32(data, size, &version.h0008, &consumed) ||
            !read_u32(data, size, &version.map_token, &consumed) ||
            !read_u32(data, size, &version.map_id, &consumed) ||
            !read_u32(data, size, &version.player_token, &consumed) ||
            !read_array_u8_exact(data, size, version.account_uuid, sizeof(version.account_uuid), &consumed) ||
            !read_array_u8_exact(data, size, version.character_uuid, sizeof(version.character_uuid), &consumed) ||
            !read_u32(data, size, &version.h0038, &consumed) ||
            !read_u32(data, size, &version.h003C, &consumed))
        {
            return ERR_WOULDBLOCK;
        }

        GmUuid account_id;
        uuid_dec_le(&account_id, version.account_uuid);
        GmUuid char_id;
        uuid_dec_le(&char_id, version.character_uuid);

        size_t idx;
        for (idx = 0; idx < srv->game_servers.len; ++idx) {
            if (srv->game_servers.ptr[idx]->map_token == version.map_token) {
                break;
            }
        }

        if (idx == srv->game_servers.len) {
            log_error("Player tried to connect to a map that doesn't exist");
            return ERR_BAD_USER_DATA;
        }

        GameSrv *gm = srv->game_servers.ptr[idx];
        for (idx = 0; idx < gm->clients.len; ++idx) {
            if (gm->clients.ptr[idx].player_token == version.player_token) {
                break;
            }
        }

        if (idx == gm->clients.len) {
            log_error("Player tried to connect to a map he doesn't have a slot for");
            return ERR_BAD_USER_DATA;
        }

        Connection_RemoveIncoming(conn, consumed);
        conn->state = ConnState_AwaitKeyExchange;
        conn->type = ConnType_Game;
        conn->game_version = version;
        return ERR_OK;
    } else if (header == CTRL_CMSG_VERSION_HEADER) {
        if (size < sizeof(CTRL_CMSG_VERSION)) {
            return ERR_WOULDBLOCK;
        }

        CTRL_CMSG_VERSION version;
        version.header = header;

        Connection_RemoveIncoming(conn, consumed);
        conn->state = ConnState_AwaitDone;
        conn->type = ConnType_Ctrl;
        conn->ctrl_version = version;
        return ERR_OK;
    } else {
        return ERR_BAD_USER_DATA;
    }
}

int Connection_DoKeyExchange(Connection *conn)
{
    int err;

    size_t consumed = 0;
    const uint8_t *data = conn->incoming;
    size_t size = conn->n_incoming;

    MSG_CLIENT_SEED client_seed;
    if (!read_u16(data, size, &client_seed.header, &consumed) ||
        !read_array_u8_exact(data, size, client_seed.seed, sizeof(client_seed.seed), &consumed))
    {
        return ERR_WOULDBLOCK;
    }

    if (client_seed.header != CMSG_CLIENT_SEED_HEADER) {
        return ERR_BAD_USER_DATA;
    }

    mbedtls_mpi client_public;
    mbedtls_mpi_init(&client_public);
    if ((err = mbedtls_mpi_read_binary_le(&client_public, client_seed.seed, sizeof(client_seed.seed))) != 0) {
        log_error("mbedtls_mpi_read_binary_le failed on client public key, err: %d", err);
        mbedtls_mpi_free(&client_public);
        return ERR_UNSUCCESSFUL;
    }

    mbedtls_mpi shared_secret;
    mbedtls_mpi_init(&shared_secret);
    if ((err = mbedtls_mpi_exp_mod(&shared_secret, &client_public, &server_private, &prime_modulus, NULL)) != 0) {
        log_error("mbedtls_mpi_exp_mod failed, couldn't compute shared secret, err: %d", err);
        mbedtls_mpi_free(&client_public);
        mbedtls_mpi_free(&shared_secret);
        return ERR_UNSUCCESSFUL;
    }

    uint8_t shared_secret_bytes[64];
    err = mbedtls_mpi_write_binary_le(&shared_secret, shared_secret_bytes, sizeof(shared_secret_bytes));
    mbedtls_mpi_free(&client_public);
    mbedtls_mpi_free(&shared_secret);

    if (err != 0) {
        log_error("mbedtls_mpi_write_binary_le failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }

    if ((err = sys_getrandom(conn->master_secret, sizeof(conn->master_secret))) != 0) {
        log_error("sys_getrandom failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }

    MSG_SERVER_SEED server_seed;
    STATIC_ASSERT(sizeof(conn->master_secret) == sizeof(server_seed.seed));
    server_seed.header = SMSG_SERVER_SEED_HEADER;
    for (size_t idx = 0; idx < sizeof(server_seed.seed); ++idx) {
        server_seed.seed[idx] = conn->master_secret[idx] ^ shared_secret_bytes[idx];
    }

    // @Cleanup: Only works on little endian machine.
    size_t bytes_sent;
    const uint8_t *bytes = (const uint8_t *)&server_seed;
    if ((err = sys_send(conn->source.socket, bytes, sizeof(server_seed), &bytes_sent)) != 0) {
        log_error("Failed to send %zu bytes, err: %d", sizeof(server_seed), err);
        return ERR_UNSUCCESSFUL;
    }

    if (bytes_sent != sizeof(server_seed)) {
        log_error("Failed to send %zu bytes, only %zu were sent", sizeof(server_seed), bytes_sent);
        return ERR_UNSUCCESSFUL;
    }

    conn->state = ConnState_AwaitDone;
    return ERR_OK;
}

void AuthConnection_Free(AuthConnection *conn)
{
    IoSource_free(&conn->source);
    array_free(&conn->incoming);
    array_free(&conn->outgoing);
    arc4_free(&conn->cipher_enc);
    arc4_free(&conn->cipher_dec);
    array_free(&conn->messages);
    array_free(&conn->characters);
}

AuthSrvMsg* AuthConnection_BuildMsg(AuthConnection *conn, uint16_t header)
{
    memset(&conn->srv_msg, 0, sizeof(conn->srv_msg));
    conn->srv_msg.header = header;
    return &conn->srv_msg;
}

int AuthConnection_FlushOutgoingBuffer(AuthConnection *conn)
{
    int err;
    size_t bytes_sent;
    if ((err = sys_send(conn->source.socket, conn->outgoing.ptr, conn->outgoing.len, &bytes_sent)) != 0) {
        if (sys_would_block(err)) {
            conn->writable = false;
            return ERR_OK;
        }

        log_error("Failed to send %zu bytes, err: %d", bytes_sent, err);
        return ERR_UNSUCCESSFUL;
    }

    if (bytes_sent != conn->outgoing.len) {
        conn->writable = false;
        array_remove_range_ordered(&conn->outgoing, 0, bytes_sent);
    } else {
        array_clear(&conn->outgoing);
    }

    return ERR_OK;
}

int AuthConnection_SendMessage(AuthConnection *conn, AuthSrvMsg *msg, size_t size)
{
    int err;

    assert(msg->header < ARRAY_SIZE(AUTH_SMSG_FORMATS));
    MsgFormat format = AUTH_SMSG_FORMATS[msg->header];

    size_t size_before = array_size(&conn->outgoing);

    uint8_t *dst;
    if ((dst = array_push(&conn->outgoing, MSG_MAX_BUFFER_SIZE)) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    size_t written;
    if ((err = pack_msg(format, &written, msg->buffer, size, dst, MSG_MAX_BUFFER_SIZE)) != 0) {
        array_shrink(&conn->outgoing, size_before);
        return err;
    }

    array_shrink(&conn->outgoing, size_before + written);
    arc4_crypt_inplace(&conn->cipher_enc, dst, written);
    return ERR_OK;
}

int AuthSrv_Setup(AuthSrv *srv)
{
    int err;

    unsigned char random_key[32];
    if ((err = sys_getrandom(random_key, 32)) != 0) {
        log_error("Failed to get %zu random bytes", sizeof(random_key));
        return err;
    }

    memset(srv, 0, sizeof(*srv));

    if ((err = iocp_setup(&srv->iocp)) != 0) {
        return err;
    }

    // @Cleanup: How to move that out.
    const char *path = "db/database.db";
    if ((err = Db_Open(&srv->database, path)) != 0) {
        iocp_free(&srv->iocp);
        return ERR_UNSUCCESSFUL;
    }

    random_init(&srv->random, random_key);
    return 0;
}

void AuthSrv_Free(AuthSrv *srv)
{
    iocp_free(&srv->iocp);
    stbds_hmfree(srv->objects);
    array_free(&srv->objects_with_event);
    array_free(&srv->objects_to_remove);
    array_free(&srv->events);
    mbedtls_chacha20_free(&srv->random);
    Db_Close(&srv->database);
    array_free(&srv->game_servers);
    stbds_hmfree(srv->connected_accounts);
}

bool AuthSrv_IssueNextToken(AuthSrv *srv, uintptr_t *result)
{
    if (++srv->last_token_issued == UINTPTR_MAX) {
        log_error("Can't issue any more token, all values were exhausted");
        return false;
    }

    *result = srv->last_token_issued;
    return true;
}

IoObject *AuthSrv_GetObject(AuthSrv *srv, uintptr_t token)
{
    ptrdiff_t idx;
    if ((idx = stbds_hmgeti(srv->objects, token)) == -1) {
        return NULL;
    }
    return &srv->objects[(size_t)idx].value;
}

ConnectedAccountInfo* AuthSrv_GetConnectedAccount(AuthSrv *srv, GmUuid account_id)
{
    ptrdiff_t idx;
    if ((idx = stbds_hmgeti(srv->connected_accounts, account_id)) == -1) {
        return NULL;
    }
    return &srv->connected_accounts[(size_t)idx];
}

void AuthSrv_DelConnectedAccount(AuthSrv *srv, GmUuid account_id)
{
    stbds_hmdel(srv->connected_accounts, account_id);
}

ConnectedAccountInfo* AuthSrv_AddConnectedAccount(AuthSrv *srv, GmUuid account_id)
{
    ConnectedAccountInfo info = {0};
    info.key = account_id;
    stbds_hmputs(srv->connected_accounts, info);
    return &srv->connected_accounts[stbds_temp(srv->connected_accounts - 1)];
}

void AuthSrv_FreeAuthConnectionByToken(AuthSrv *srv, uintptr_t token)
{
    IoObject *object;
    if ((object = AuthSrv_GetObject(srv, token)) == NULL) {
        return;
    }

    switch (object->type) {
    case IoObjectType_AuthConnection:
        IoSource_reset(&object->auth_connection.source);
        array_clear(&object->auth_connection.outgoing);
        array_add(&srv->objects_to_remove, token);
        break;
    case IoObjectType_CtrlConnection:
        IoSource_reset(&object->ctrl_connection.source);
        array_clear(&object->ctrl_connection.outgoing);
        array_add(&srv->objects_to_remove, token);
        break;
    default:
        abort();
    }
}

void AuthSrv_CloseAuthConnection(AuthSrv *srv, AuthConnection *conn)
{
    IoSource_reset(&conn->source);
    array_clear(&conn->outgoing);
    array_add(&srv->objects_to_remove, conn->token);

    ConnectedAccountInfo *connected;
    if ((connected = AuthSrv_GetConnectedAccount(srv, conn->account_id)) == NULL) {
        return;
    }

    connected->auth_conn_token = 0;
    if (connected->map_token == 0) {
        AuthSrv_DelConnectedAccount(srv, conn->account_id);
    }
}

void AuthSrv_CloseCtrlConnection(AuthSrv *srv, CtrlConn *conn)
{
    IoSource_reset(&conn->source);
    array_clear(&conn->outgoing);
    array_add(&srv->objects_to_remove, conn->token);
}

void AuthSrv_GetMessages(AuthSrv *srv, AuthConnection *conn)
{
    int err;
    uint16_t header;
    size_t total_consumed = 0;

    while (sizeof(header) <= (conn->incoming.len - total_consumed)) {
        const uint8_t *input = &conn->incoming.ptr[total_consumed];
        size_t size = conn->incoming.len - total_consumed;

        header = le16dec(input) & ~AUTH_CMSG_MASK;
        if (ARRAY_SIZE(AUTH_CMSG_FORMATS) <= header) {
            AuthSrv_CloseAuthConnection(srv, conn);
            break;
        }

        MsgFormat format = AUTH_CMSG_FORMATS[header];

        size_t consumed;
        AuthCliMsg *msg = malloc(sizeof(*msg));
        if ((err = unpack_msg(format, &consumed, input, size, msg->buffer, sizeof(msg->buffer))) != 0) {
            free(msg);

            if (err != ERR_NOT_ENOUGH_DATA) {
                log_warn("Received invalid message from client %04" PRIXPTR, conn->token);
                AuthSrv_CloseAuthConnection(srv, conn);
            }

            break;
        }

        array_add(&conn->messages, msg);
        total_consumed += consumed;
    }

    array_remove_range_ordered(&conn->incoming, 0, total_consumed);
}

void AuthSrv_PeerDisconnected(AuthSrv *srv, AuthConnection *conn)
{
    size_t count = array_size(&conn->messages);
    if (count != 0 && array_at(&conn->messages, count - 1) == NULL) {
        log_warn("Tried to peer disconnect twice");
        return;
    }

    AuthSrv_GetMessages(srv, conn);
    array_add(&conn->messages, NULL);
    AuthSrv_CloseAuthConnection(srv, conn);
}

void AuthSrv_SendSessionInfo(AuthSrv *srv, AuthConnection *conn)
{
    random_get_bytes(&srv->random, &conn->server_salt, sizeof(conn->server_salt));
    AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_SESSION_INFO);
    msg->session_info.server_salt = conn->server_salt;

    AuthConnection_SendMessage(conn, msg, sizeof(msg->session_info));
}

void AuthSrv_SendRequestResponse(AuthConnection *conn, uint32_t req_id, uint32_t status)
{
    AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_REQUEST_RESPONSE);
    msg->request_response.req_id = req_id;
    msg->request_response.status = status;

    AuthConnection_SendMessage(conn, msg, sizeof(msg->request_response));
}

void AuthSrv_SendAccountInfo(AuthConnection *conn, uint32_t req_id)
{
    static const uint8_t features[] = "\x01\x00\x04\x00\x57\x00\x01\x00";
    //                                           ^^ Number of slots. (2 are unaccounted for)

    AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_ACCOUNT_INFO);
    msg->account_info.req_id = req_id;
    msg->account_info.h0006 = 0;
    msg->account_info.h000A = 4;
    memcpy_literal(msg->account_info.h000E, "\x3F\x00\x00\x00\x00\x00\x00\x00");
    memcpy_literal(msg->account_info.h0016, "\x80\x3F\x42\x00\x83\x00\x0C\x02"); // control which campaigns are owned.
    uuid_enc_le(msg->account_info.account_uuid, &conn->account_id);
    if (conn->characters.len != 0) {
        uuid_enc_le(msg->account_info.character_uuid, &conn->characters.ptr[0].char_id);
    }
    msg->account_info.h003E = 8;
    msg->account_info.n_unk4 = sizeof(features) - 1;
    memcpy(msg->account_info.unk4, features, msg->account_info.n_unk4);
    msg->account_info.eula_accepted = 24;
    msg->account_info.unk5 = 3;

    AuthConnection_SendMessage(conn, msg, sizeof(msg->account_info));
}

void AuthSrv_SendCharactersInfo(AuthConnection *conn, uint32_t req_id)
{
    DbCharacterArray characters = conn->characters; 
    for (size_t idx = 0; idx < characters.len; ++idx) {
        DbCharacter *ch = &characters.ptr[idx];

        AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_CHARACTER_INFO);
        msg->character_info.req_id = req_id;
        uuid_enc_le(msg->character_info.uuid, &ch->char_id);
        // msg.character_info.unk0 = ;
        msg->character_info.n_name = (uint32_t)ch->charname.len;
        STATIC_ASSERT(sizeof(msg->character_info.name) == sizeof(ch->charname.buf));
        memcpy_u16(msg->character_info.name, ch->charname.buf, ch->charname.len);

        CharacterSettings settings = CharacterSettings_FromDbCharacter(ch);
        STATIC_ASSERT(sizeof(settings) <= sizeof(msg->character_info.extended));
        msg->character_info.n_extended = sizeof(settings);
        memcpy(msg->character_info.extended, &settings, sizeof(settings));
        AuthConnection_SendMessage(conn, msg, sizeof(msg->character_info));
    }
}

void AuthSrv_SendAccountSettings(AuthConnection *conn, uint32_t req_id)
{
    static const uint8_t settings[] = "\x08\x00\x66\x21\x00\x10\x38\x43\x7F\xDE\x68\x8E\xF5\x93\xFF\xFF\x00\xC1\x3F\x00\x00\x00\x08\x42\x01\x5E\x00\x02\x00\x80\x33\x18\x00\x81\x03\x00\x00\x00\x4B\x0A\x00\x02\x00\x1B\x00\xD7\x00\x42\x00\x42\x00\x73\x0F\x00\x07\x0C\x09\x0B\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x00\x00\x76\x14\x00\x08\x00\x00\x00\xEF\x03\x01\x00\x00\x08\x80\xDC\x00\xDF\xAA\x0B\x20\x00\x02\x01\x77\xF0\x01\x10\x3B\x38\x03\xC0\xFF\x7D\x99\x01\xFB\xBF\xFA\xBB\xFF\x00\x00\x00\x0D\x00\x00\xA1\x01\x00\x00\xCB\x01\x53\xA4\x01\x96\x00\x00\x96\x03\xEF\x00\x29\x02\xF8\x00\x26\x02\x51\x6D\x01\x2D\x02\x6C\x35\x59\x2F\x00\x1C\x02\x00\xBD\x51\xD8\x00\xB0\x01\x48\x0C\x51\xE1\x00\xC3\x01\x3A\x0C\x51\x82\x00\x00\x01\x57\x0D\x06\xC8\x02\x5E\x03\xDE\x00\x1D\x02\x60\x00\x00\x00\x00\x06\x15\x06\x54\x01\x0C\x02\xDE\x00\x06\x5E\x01\xC1\x01\xFB\xFF\x1A\x02\x06\x05\x03\xFF\x01\xA3\x00\x77\x02\x06\xF7\x01\x44\x02\x94\x00\xA0\x02\x46\xF1\x00\x00\x00\x42\x00\x27\xF3\xF0\x6F\x00\x8C\x02\x06\x9A\x00\x19\x02\x27\x00\x1C\x02\x46\xDE\x02\x00\x00\x59\x00\x60\x00\x00\x00\x00\x06\x6C\x03\x8E\x02\x29\x00\x81\x03\x06\x24\x03\x00\x00\xED\x00\x00\x00\x06\x03\x05\x00\x00\xE2\x00\x00\x00\x06\x02\x01\x13\x03\x2F\x00\x2F\x03\x31\x57\xB5\xB7\x00\x25\x00\x01\x10\x01\x12\x02\x22\xFF\x14\x00\x06\x86\x03\x00\x00\x0F\x01\x00\x00\x27\x1F\xFA\x90\x00\xC8\x00\x06\xEB\x03\xEE\x01\xB0\x00\x84\x02\x60\x00\x00\x00\x00\x05\x99\x00\xF4\x01\xB0\x00\x64\x00\x47\x1F\x01\xDE\x01\x4B\x64\x60\x00\x00\x00\x00\x06\xA8\x02\x1F\x02\x84\x00\x22\x02\x60\x00\x00\x00\x00\x06\xD5\x04\x00\x00\xB0\x01\x00\x00\x06\xD7\x04\x00\x00\x26\x02\x00\x00\x06\xD6\x04\x00\x00\x9A\x02\x00\x00\x06\x20\x01\x11\x02\x2D\x00\x52\x02\x60\x00\x00\x00\x00\x06\xB8\x03\xD2\x01\x64\x00\x71\x02\x60\x00\x00\x00\x00\x06\xB8\x03\xEA\x01\x64\x00\x38\x02\x05\xDD\x00\xD2\x01\x82\x00\xD0\x00\x47\xEE\x00\x2C\x01\x70\xFA\x60\x00\x00\x00\x00\x06\x76\x04\x00\x00\xFA\x00\x00\x00\x46\x29\x03\x00\x00\x5A\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x46\x22\x02\x00\x00\x3B\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x06\x35\x04\x00\x00\x97\x00\x00\x00\x06\x23\x01\x00\x00\x7B\x01\x00\x00\x06\x26\x01\x00\x00\xDB\x00\x00\x00\x06\x2E\x01\x00\x00\xA9\x01\x00\x00\x06\x24\x01\x00\x00\x3C\x02\x00\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x60\x00\x00\x00\x00\x0D\x11\x02\x5A\x00\xD6\x01\x1C\x00";
    const uint32_t n_settings = sizeof(settings) - 1;

    AuthSrv_AccountSettings *msg = &AuthConnection_BuildMsg(conn, AUTH_SMSG_ACCOUNT_SETTINGS)->account_settings;
    msg->req_id = req_id;
    STATIC_ASSERT(sizeof(settings) <= sizeof(msg->data));
    msg->n_data = n_settings;
    memcpy(msg->data, settings, n_settings);

    AuthConnection_SendMessage(conn, &conn->srv_msg, sizeof(*msg));
}

void AuthSrv_SendFriendStreamEnd(AuthConnection *conn, uint32_t req_id)
{
    AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_FRIEND_STREAM_END);
    msg->friend_stream_end.unk1 = req_id;
    msg->friend_stream_end.unk2 = req_id;
    AuthConnection_SendMessage(conn, msg, sizeof(msg->friend_stream_end));
}

void AuthSrv_SendHeartbeat(AuthConnection *conn)
{
    AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_HEARTBEAT);
    msg->heartbeat.tick = 16;
    AuthConnection_SendMessage(conn, msg, sizeof(msg->heartbeat));
}

void AuthSrv_SendGameServerInfo(
    AuthConnection *conn,
    uint32_t   req_id,
    SocketAddr address,
    uint32_t   map_token,
    uint32_t   player_token,
    uint32_t   map_id)
{
    AuthSrvMsg *msg = AuthConnection_BuildMsg(conn, AUTH_SMSG_GAME_SERVER_INFO);
    msg->game_server_info.req_id = req_id;
    msg->game_server_info.map_token = map_token;
    msg->game_server_info.map_id = map_id;
    msg->game_server_info.player_token = player_token;

    switch (address.af)
    {
    case AddressFamily_V4:
        le16enc(&msg->game_server_info.host[0], AF_INET);
        be16enc(&msg->game_server_info.host[2], address.v4.port);
        memcpy(&msg->game_server_info.host[4], address.v4.bytes, sizeof(address.v4.bytes));
        break;
    case AddressFamily_V6:
        le16enc(&msg->game_server_info.host[0], AF_INET6);
        be16enc(&msg->game_server_info.host[2], address.v6.port);
        memcpy(&msg->game_server_info.host[4], address.v6.bytes, sizeof(address.v6.bytes));
        break;
    default:
        abort();
    }

    AuthConnection_SendMessage(conn, msg, sizeof(msg->game_server_info));
}

int AuthSrv_HandlePortalAccountLogin(AuthSrv *srv, AuthConnection *conn, AuthCliMsg *cli_msg)
{
    int err;

    assert(cli_msg->header == AUTH_CMSG_PORTAL_ACCOUNT_LOGIN);
    AuthSrv_PortalAccountLogin *msg = &cli_msg->portal_account_login;

    GmUuid session_id, user_id;
    uuid_dec_le(&user_id, msg->user_id);
    uuid_dec_le(&session_id, msg->session_id);

    DbSession session;
    if ((err = Db_GetSession(&srv->database, user_id, session_id, &session)) != 0) {
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_AUTH_ERROR);
        return ERR_OK;
    }

    ConnectedAccountInfo *connected_account;
    if ((connected_account = AuthSrv_GetConnectedAccount(srv, session.account_id)) != NULL) {
        AuthSrv_FreeAuthConnectionByToken(srv, connected_account->auth_conn_token);
        connected_account->auth_conn_token = conn->token;
    } else {
        connected_account = AuthSrv_AddConnectedAccount(srv, session.account_id);
        connected_account->auth_conn_token = conn->token;
    }

    conn->user_id = user_id;
    conn->session_id = session_id;
    conn->account_id = session.account_id;
    conn->connected = true;

    if ((err = Db_GetAccount(&srv->database, conn->account_id, &conn->account)) != 0) {
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_NETWORK_ERROR);
        return ERR_OK;
    }
    
    array_clear(&conn->characters);
    if ((err = Db_GetCharacters(&srv->database, conn->account_id, &conn->characters)) != 0) {
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_NETWORK_ERROR);
        return ERR_OK;
    }

    for (size_t idx = 0; idx < conn->characters.len; ++idx) {
        DbCharacter *ch = &conn->characters.ptr[idx];
        if (uuid_equals(&ch->char_id, &conn->account.current_char_id)) {
            conn->selected_character_idx = idx;
            break;
        }
    }

    AuthSrv_SendCharactersInfo(conn, msg->req_id);
    AuthSrv_SendAccountSettings(conn, msg->req_id);
    AuthSrv_SendFriendStreamEnd(conn, msg->req_id);
    AuthSrv_SendAccountInfo(conn, msg->req_id);
    AuthSrv_SendRequestResponse(conn, msg->req_id, 0);
    return ERR_OK;
}

int AuthSrv_HandleAskServerResponse(AuthConnection *conn, AuthCliMsg *msg)
{
    assert(msg->header == AUTH_CMSG_ASK_SERVER_RESPONSE);
    AuthSrv_SendRequestResponse(conn, msg->ask_server_response.req_id, 0);
    return ERR_OK;
}

int AuthSrv_HandleSetPlayerStatus(AuthConnection *conn, AuthCliMsg *msg)
{
    assert(msg->header == AUTH_CMSG_SET_PLAYER_STATUS);
    switch (msg->set_player_status.status) {
    case PlayerStatus_Offline:
    case PlayerStatus_Online:
    case PlayerStatus_DND:
    case PlayerStatus_Away:
    case PlayerStatus_Blank:
        conn->player_status = msg->set_player_status.status;
        return ERR_OK;
        break;
    default:
        log_warn(
            "Client %04" PRIXPTR ", sent an invalid status %" PRIu32,
            conn->token,
            msg->set_player_status.status
        );
        return ERR_BAD_USER_DATA;
    }
}

int AuthSrv_HandleChangePlayCharacter(AuthConnection *conn, AuthCliMsg *msg)
{
    assert(msg->header == AUTH_CMSG_CHANGE_PLAY_CHARACTER);

    if (!conn->connected) {
        AuthSrv_SendRequestResponse(conn, msg->change_character.req_id, GM_ERROR_DISCONNECTED);
        return ERR_OK;
    }

    DbCharacterArray characters = conn->characters;
    for (size_t idx = 0; characters.len; ++idx) {
        DbCharacter *ch = &characters.ptr[idx];
        if (ch->charname.len == msg->change_character.n_name &&
            memcmp_u16(ch->charname.buf, msg->change_character.name, ch->charname.len) == 0)
        {
            AuthSrv_SendRequestResponse(conn, msg->change_character.req_id, 0);
            return ERR_OK;
        }
    }

    AuthSrv_SendRequestResponse(conn, msg->change_character.req_id, GM_ERROR_NETWORK_ERROR);
    return ERR_OK;
}

int AuthSrv_FindCompatibleGameSrv(AuthSrv *srv, RequestGameInstanceParams params, size_t *result)
{
    GameSrvArray game_servers = srv->game_servers;
    for (size_t idx = 0; idx < game_servers.len; ++idx) {
        GameSrv *gm = game_servers.ptr[idx];
        if (gm->map_id == params.map_id && gm->map_type == params.map_type &&
            gm->region == params.region && gm->language == params.language &&
            (params.district_number == 0 || gm->district_number == params.district_number))
        {
            *result = idx;
            return ERR_OK;
        }
    }
    return ERR_UNSUCCESSFUL;
}

int AuthSrv_AllocGameServer(AuthSrv *srv, RequestGameInstanceParams params, size_t *result)
{
    GameSrv *gm;
    if ((gm = calloc(1, sizeof(*gm))) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    int err;
    if ((err = GameSrv_Setup(gm)) != 0) {
        free(gm);
        return err;
    }

    random_get_bytes(&srv->random, &gm->map_token, sizeof(gm->map_token));
    gm->map_id = params.map_id;
    gm->region = params.region;
    gm->language = params.language;
    gm->district_number = params.district_number;
    gm->map_type = params.map_type;
    gm->ctrl_conn.peer_addr = srv->internal_address;

    if ((err = GameSrv_Start(gm)) != 0) {
        log_error("Failed to start a game server");
        GameSrv_Free(gm);
        free(gm);
        return err;
    }

    array_add(&srv->game_servers, gm);
    *result = array_size(&srv->game_servers) - 1;
    return ERR_OK;
}

int AuthSrv_HandleRequestGameInstance(AuthSrv *srv, AuthConnection *conn, AuthCliMsg *cli_msg)
{
    int err;
    assert(cli_msg->header == AUTH_CMSG_REQUEST_GAME_INSTANCE);
    AuthSrv_RequestGameInstance *msg = &cli_msg->request_game_instance;

    if (!conn->connected) {
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_DISCONNECTED);
        return ERR_OK;
    }

    RequestGameInstanceParams req = {0};
    if ((err = MapType_FromInt(&req.map_type, msg->map_type)) != 0 ||
        (err = DistrictRegion_FromInt(&req.region, msg->region)) != 0 ||
        (err = DistrictLanguage_FromInt(&req.language, msg->language)) != 0)
    {
        log_error("Receive invalid client data from %04" PRIXPTR, conn->token);
        return ERR_BAD_USER_DATA;
    }

    req.map_id = cast_u16(msg->map_id);
    req.district_number = msg->district;

    if (conn->characters.len != 0 && !(conn->selected_character_idx < conn->characters.len)) {
        log_error("Client tried to join a server without a selected character");
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_NETWORK_ERROR);
        return ERR_OK;
    }

    GmUuid char_id = {0};
    if (conn->selected_character_idx < conn->characters.len) {
        char_id = conn->characters.ptr[conn->selected_character_idx].char_id;
    }

    // @Cleanup:
    // Ensure the client isn't on an other game server.

    size_t idx;
    if (AuthSrv_FindCompatibleGameSrv(srv, req, &idx) != 0) {
        if (AuthSrv_AllocGameServer(srv, req, &idx) != 0) {
            log_error("Couldn't allocate an appropriate game server");
            AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_NETWORK_ERROR);
            return ERR_OK;
        }
    }

    GameSrv *gm = srv->game_servers.ptr[idx];
    GameClient *cli = array_push(&gm->clients, 1);
    memset(cli, 0, sizeof(*cli));
    random_get_bytes(&srv->random, &cli->player_token, sizeof(cli->player_token));
    cli->account_id = conn->account_id;
    cli->char_id = char_id;

    SocketAddr address = SocketAddr_LocalHostV4();
    AuthSrv_SendGameServerInfo(conn, msg->req_id,
        address, gm->map_token, cli->player_token, gm->map_id);
    AuthSrv_SendRequestResponse(conn, msg->req_id, 0);
    return ERR_OK;
}

int AuthSrv_HandleClientHeartbeat(AuthConnection *conn, AuthCliMsg *cli_msg)
{
    assert(cli_msg->header == AUTH_CMSG_HEARTBEAT);
    AuthSrv_SendHeartbeat(conn);
    return ERR_OK;
}

int AuthSrv_HandleAcceptEula(AuthConnection *conn, AuthCliMsg *cli_msg)
{
    UNREFERENCED_PARAMETER(conn);
    assert(cli_msg->header == AUTH_CMSG_ACCEPT_EULA);
    return ERR_OK;
}

int AuthSrv_HandleAddAccessKey(AuthConnection *conn, AuthCliMsg *cli_msg)
{
    assert(cli_msg->header == AUTH_CMSG_ADD_ACCESS_KEY);
    AuthSrv_AddAccessKey *msg = &cli_msg->add_access_key;

    char buffer[sizeof(msg->key) + 1];
    if (!s16_to_ascii(buffer, sizeof(buffer), msg->key, msg->n_key)) {
        buffer[0] = 0;
    }

    log_info("Adding access key: '%s'", buffer);
    AuthSrv_SendRequestResponse(conn, msg->req_id, 0);
    return ERR_OK;
}

int AuthSrv_HandleDeleteCharacter(AuthSrv *srv, AuthConnection *conn, AuthSrv_DeleteCharacter *msg)
{
    int err;

    size_t idx;
    for (idx = 0; idx < conn->characters.len; ++idx) {
        DbCharacter *ch = &conn->characters.ptr[idx];
        if (msg->n_charname != ch->charname.len) {
            continue;
        }

        if (memeq_u16(msg->charname, ch->charname.buf, ch->charname.len)) {
            break;
        }
    }

    if (idx == conn->characters.len) {
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_INVALID_CHARNAME);
        return ERR_OK;
    }

    DbCharacter *ch = &conn->characters.ptr[idx];
    if ((err = Db_DeleteCharacter(&srv->database, conn->account_id, ch->char_id)) != 0) {
        AuthSrv_SendRequestResponse(conn, msg->req_id, GM_ERROR_NETWORK_ERROR);
        return ERR_OK;
    }

    AuthSrv_SendRequestResponse(conn, msg->req_id, 0);
    return ERR_OK;
}

int AuthSrv_Bind(AuthSrv *srv, const char *addr, size_t addr_len)
{
    int err;

    SocketAddr bind_addr;
    if (!parse_addr(&bind_addr, addr, addr_len)) {
        return ERR_UNSUCCESSFUL;
    }

    struct sockaddr sa;
    SocketAddr_WriteSocketAddrStorage(&sa, &bind_addr);

    uintptr_t fd;
    if (!create_nonblocking_socket(&fd, sa.sa_family)) {
        return ERR_UNSUCCESSFUL;
    }

#if !PLATFORM_WINDOWS
    if ((err = sys_set_reuseaddr(fd, true)) != 0) {
        sys_closesocket(fd);
        return err;
    }
#endif

    if ((err = sys_bind(fd, &sa, sizeof(sa))) != 0) {
        log_error("Failed to bind to '%.*s', err: %d", (int) addr_len, addr, err);
        sys_closesocket(fd);
        return ERR_UNSUCCESSFUL;
    }

    if ((err = sys_listen(fd, 256)) != 0) {
        log_error("Failed to listen to '%.*s', err: %d", (int) addr_len, addr, err);
        sys_closesocket(fd);
        return ERR_UNSUCCESSFUL;
    }

    uintptr_t token;
    if (!AuthSrv_IssueNextToken(srv, &token)) {
        log_error("Can't issue a new token");
        sys_closesocket(fd);
        return ERR_UNSUCCESSFUL;
    }


    IoObject obj = {IoObjectType_Listener};
    IoSource *source = &obj.listener;
    IoSource_setup(source, fd);

    if ((err = iocp_register(&srv->iocp, source, token, IOCPF_READ)) != 0) {
        sys_closesocket(fd);
        return ERR_UNSUCCESSFUL;
    }

    log_info("AuthSrv listen to '%.*s'", (int) addr_len, addr);
    stbds_hmput(srv->objects, token, obj);
    return ERR_OK;
}

void AuthSrv_ProcessListenerEvent(AuthSrv *srv, IoSource *listener)
{
    int err;

    for (;;) {
        uintptr_t conn_socket;
        struct sockaddr_storage addr;
        int addrlen = sizeof(addr);
        if ((err = sys_accept(&conn_socket, listener->socket, (struct sockaddr *)&addr, &addrlen)) != 0) {
            if (!sys_would_block(err)) {
                log_error("sys_accept failed, err: %d", err);
            }
            break;
        }

        uintptr_t conn_token;
        if (!AuthSrv_IssueNextToken(srv, &conn_token)) {
            sys_closesocket(conn_socket);
            break;
        }

        IoObject obj = {IoObjectType_Connection};
        Connection *conn = &obj.connection;
        SocketAddr_FromSocketAddrStorage(&conn->peer_addr, &addr);
        conn->token = conn_token;

        char buffer[128];
        if (snprint_sockaddr(buffer, sizeof(buffer), (struct sockaddr *) &addr, (size_t)addrlen)) {
            log_info("New connection %04" PRIXPTR " accepted with address '%s'", conn_token, buffer);
        } else {
            log_info("New connection %04" PRIXPTR " accepted, but can't stringify the address", conn_token);
        }

        IoSource_setup(&conn->source, conn_socket);
        if ((err = iocp_register(&srv->iocp, &conn->source, conn_token, IOCPF_READ | IOCPF_WRITE)) != 0) {
            log_error("Failed to register connection with token %04" PRIXPTR, conn_token);
        }

        stbds_hmput(srv->objects, conn_token, obj);
    }
}

void AuthSrv_DoPostHandshake(AuthSrv *srv, Connection *conn)
{
    int err;
    if (conn->type == ConnType_Auth) {
        IoObject obj = { .type = IoObjectType_AuthConnection };
        AuthConnection *dst = &obj.auth_connection;

        dst->token = conn->token;
        dst->source = conn->source;
        uint8_t derived_key[20];
        arc4_hash(conn->master_secret, derived_key);
        arc4_init(&dst->cipher_enc);
        arc4_setup(&dst->cipher_enc, derived_key, sizeof(derived_key));
        dst->cipher_dec = dst->cipher_enc;
        dst->peer_addr = conn->peer_addr;

        log_info("Accepted Auth connection %04" PRIXPTR, dst->token);
        stbds_hmput(srv->objects, dst->token, obj);
    } else if (conn->type == ConnType_Game) {
        size_t idx;
        for (idx = 0; idx < srv->game_servers.len; ++idx) {
            if (srv->game_servers.ptr[idx]->map_token == conn->game_version.map_token) {
                break;
            }
        }

        uintptr_t token = conn->token;
        if (idx == srv->game_servers.len) {
            Connection_Free(conn);
            stbds_hmdel(srv->objects, token);
            return;
        }

        GameSrv *gm = srv->game_servers.ptr[idx];
        for (idx = 0; idx < gm->clients.len; ++idx) {
            if (gm->clients.ptr[idx].player_token == conn->game_version.player_token) {
                break;
            }
        }

        if (idx == gm->clients.len) {
            Connection_Free(conn);
            stbds_hmdel(srv->objects, token);
            return;
        }

        GameClient *cli = &gm->clients.ptr[idx];

        if ((err = iocp_deregister(&srv->iocp, &conn->source)) != 0) {
            log_error("Failed to de-register socket when transferring connection to a game server");
            Connection_Free(conn);
            stbds_hmdel(srv->objects, token);
            return;
        }

        AdminMsg msg = {AdminCmd_TransferUser};
        msg.transfer_user.peer_addr = conn->peer_addr;
        msg.transfer_user.source = IoSource_take(&conn->source);
        msg.transfer_user.token = conn->token;
        arc4_hash(conn->master_secret, msg.transfer_user.cipher_init_key);
        msg.transfer_user.account_id = cli->account_id;
        msg.transfer_user.char_id = cli->char_id;
        GameSrv_SendAdminMsg(gm, &msg);

        stbds_hmdel(srv->objects, token);
    } else if (conn->type == ConnType_Ctrl) {
        IoObject obj = { .type = IoObjectType_CtrlConnection };
        CtrlConn *dst = &obj.ctrl_connection;

        dst->token = conn->token;
        dst->source = conn->source;
        dst->peer_addr = conn->peer_addr;

        if (conn->n_incoming != 0) {
            uint8_t *buffer;
            if ((buffer = array_push(&dst->incoming, conn->n_incoming)) == NULL) {
                log_error("out of memory");
                abort();
            }

            memcpy(buffer, conn->incoming, conn->n_incoming);
            if ((err = CtrlConn_GetMessages(dst)) != 0) {
                log_error("CtrlConn_GetMessages failed, err: %d", err);
                abort();
            }
        }

        log_info("Accepted Ctrl connection %04" PRIXPTR, dst->token);
        stbds_hmput(srv->objects, dst->token, obj);
    } else {
        abort();
    }
}

void AuthSrv_DoHandshake(AuthSrv *srv, Connection *conn)
{
    int err;
    do {
        switch (conn->state) {
        case ConnState_AwaitHello:
            err = Connection_ReadVersion(srv, conn);
            break;
        case ConnState_AwaitKeyExchange:
            err = Connection_DoKeyExchange(conn);
            break;
        case ConnState_AwaitDone:
            AuthSrv_DoPostHandshake(srv, conn);
            err = ERR_WOULDBLOCK;
            break;
        default:
            abort();
        }
    } while (err == ERR_OK);

    if (err != ERR_WOULDBLOCK) {
        Connection_Free(conn);
        stbds_hmdel(srv->objects, conn->token);
    }
}

void AuthSrv_ProcessConnectionEvent(AuthSrv *srv, Connection *conn, Event event)
{
    int err;

    if ((event.flags & IOCPF_WRITE) != 0) {
        conn->writable = true;
    }

    if ((event.flags & IOCPF_READ) != 0) {
        uint8_t *buffer = &conn->incoming[conn->n_incoming];
        size_t   avail  = sizeof(conn->incoming) - conn->n_incoming;

        size_t bytes;
        if ((err = sys_recv(conn->source.socket, buffer, avail, &bytes)) != 0) {
            if (!sys_would_block(err)) {
                log_error(
                    "Error while reading %04" PRIXPTR ", err: %d",
                    conn->source.socket,
                    err
                );

                // Deleting the objects from the map invalidate the pointer.
                Connection_Free(conn);
                stbds_hmdel(srv->objects, conn->token);
                return;
            }
        } else if (bytes == 0) {
            Connection_Free(conn);
            stbds_hmdel(srv->objects, conn->token);
        } else {
            conn->n_incoming += bytes;
            AuthSrv_DoHandshake(srv, conn);
        }
    }
}

void AuthSrv_ProcessAuthConnectionEvent(AuthSrv *srv, AuthConnection *conn, Event event)
{
    int err;

    if ((event.flags & IOCPF_WRITE) != 0) {
        conn->writable = true;
    }

    if ((event.flags & IOCPF_READ) != 0) {
        for (;;) {
            uint8_t *buffer;

            const size_t size_at_starts = conn->incoming.len;
            if ((buffer = array_push(&conn->incoming, AUTH_READ_CHUNK_SIZE)) == NULL) {
                log_error("Out of memory will reading from socket");
                break;
            }

            size_t bytes;
            if ((err = sys_recv(conn->source.socket, buffer, AUTH_READ_CHUNK_SIZE, &bytes)) != 0) {
                array_shrink(&conn->incoming, size_at_starts);
                if (!sys_would_block(err)) {
                    log_error(
                        "Error while reading %04" PRIXPTR ", err: %d",
                        conn->source.socket,
                        err
                    );

                    AuthSrv_PeerDisconnected(srv, conn);
                }

                break;
            }

            uint8_t *ptr = &array_at(&conn->incoming, size_at_starts);
            arc4_crypt_inplace(&conn->cipher_dec, ptr, bytes);
            array_shrink(&conn->incoming, size_at_starts + bytes);

            if (bytes == 0) {
                AuthSrv_PeerDisconnected(srv, conn);
                break;
            }
        }

        AuthSrv_GetMessages(srv, conn);
    }

    for (size_t idx = 0; idx < conn->messages.len; ++idx) {
        AuthCliMsg *msg = conn->messages.ptr[idx];

        if (msg == NULL) {
            break;
        }

        err = ERR_OK;
        switch (msg->header) {
        case AUTH_CMSG_SEND_COMPUTER_INFO:
            conn->n_username = msg->computer_info.n_username;
            memcpy_u16(conn->username, msg->computer_info.username, conn->n_username);
            conn->n_pcname = msg->computer_info.n_pcname;
            memcpy_u16(conn->pcname, msg->computer_info.pcname, conn->n_pcname);
            break;
        case AUTH_CMSG_SEND_COMPUTER_HASH:
            uuid_dec_le(&conn->computer_hash, msg->computer_hash.hash);
            AuthSrv_SendSessionInfo(srv, conn);
            break;
        case AUTH_CMSG_PORTAL_ACCOUNT_LOGIN:
            err = AuthSrv_HandlePortalAccountLogin(srv, conn, msg);
            break;
        case AUTH_CMSG_ASK_SERVER_RESPONSE:
            log_info("AUTH_CMSG_ASK_SERVER_RESPONSE");
            err = AuthSrv_HandleAskServerResponse(conn, msg);
            break;
        case AUTH_CMSG_SET_PLAYER_STATUS:
            err = AuthSrv_HandleSetPlayerStatus(conn, msg);
            break;
        case AUTH_CMSG_CHANGE_PLAY_CHARACTER:
            err = AuthSrv_HandleChangePlayCharacter(conn, msg);
            break;
        case AUTH_CMSG_REQUEST_GAME_INSTANCE:
            err = AuthSrv_HandleRequestGameInstance(srv,conn, msg);
            break;
        case AUTH_CMSG_HEARTBEAT:
            err = AuthSrv_HandleClientHeartbeat(conn, msg);
            break;
        case AUTH_CMSG_DISCONNECT:
            log_info("Received disconnect from %" PRIXPTR, conn->token);
            err = ERR_UNSUCCESSFUL;
            break;
        case AUTH_CMSG_ACCEPT_EULA:
            err = AuthSrv_HandleAcceptEula(conn, msg);
            break;
        case AUTH_CMSG_ADD_ACCESS_KEY:
            err = AuthSrv_HandleAddAccessKey(conn, msg);
            break;
        case AUTH_CMSG_DELETE_CHARACTER:
            err = AuthSrv_HandleDeleteCharacter(srv, conn, &msg->delete_character);
            break;
        default:
            log_warn(
                "Unhandled AuthSrv packet with header %" PRIu16 " (0x%" PRIX16 ")",
                msg->header & ~AUTH_CMSG_MASK,
                msg->header & ~AUTH_CMSG_MASK
            );
        }

        if (err != ERR_OK) {
            AuthSrv_CloseAuthConnection(srv, conn);
            break;
        }
    }

    for (size_t idx = 0; idx < conn->messages.len; ++idx) {
        AuthCliMsg *msg = conn->messages.ptr[idx];
        free(msg);
    }

    array_clear(&conn->messages);
}

void AuthSrv_ProcessCtrlConnectionEvent(AuthSrv *srv, CtrlConn *conn, Event event)
{
    int err;

    if ((err = CtrlConn_ProcessEvent(conn, event)) != 0) {
        log_error("CtrlConn_ProcessEvent failed, err: %d", err);
    }

    for (size_t idx = 0; idx < conn->messages.len; ++idx) {
        CtrlMsg *msg = conn->messages.ptr[idx];

        if (msg == NULL) {
            array_add(&srv->objects_to_remove, conn->token);
            break;
        }

        err = ERR_OK;
        switch (msg->msg_id) {
        case CtrlMsgId_ServerReady:
            err = AuthSrvCtrl_ProcessServerReady(srv, msg);
            break;
        case CtrlMsgId_PlayerLeaved:
            break;
        default:
            err = ERR_UNSUCCESSFUL;
            log_error(
                "Unhandled Ctrl packet with header %" PRIu16 " (0x%" PRIX16 ")",
                msg->msg_id,
                msg->msg_id
            );
        }
    }
}

void AuthSrv_ProcessEvent(AuthSrv *srv, Event event)
{
    IoObject *obj;

    if ((obj = AuthSrv_GetObject(srv, event.token)) == NULL) {
        return;
    }

    switch (obj->type) {
    case IoObjectType_Listener:
        AuthSrv_ProcessListenerEvent(srv, &obj->listener);
        break;
    case IoObjectType_Connection:
        AuthSrv_ProcessConnectionEvent(srv, &obj->connection, event);
        break;
    case IoObjectType_AuthConnection:
        AuthSrv_ProcessAuthConnectionEvent(srv, &obj->auth_connection, event);
        break;
    case IoObjectType_CtrlConnection:
        AuthSrv_ProcessCtrlConnectionEvent(srv, &obj->ctrl_connection, event);
        break;
    default:
        abort();
    }
}

void AuthSrv_Poll(AuthSrv *srv)
{
    int err;

    array_clear(&srv->events);
    while ((err = iocp_poll(&srv->iocp, &srv->events, 16)) != 0) {
        if (err == ERR_TIMEOUT) {
            break;
        }
        log_error("Failed to run 'iocp_poll', err: %d", err);
        abort();
    }
}

void AuthSrv_Update(AuthSrv *srv)
{
    int err;

    AuthSrv_Poll(srv);

    for (size_t idx = 0; idx < srv->events.len; ++idx) {
        Event event = array_at(&srv->events, idx);
        AuthSrv_ProcessEvent(srv, event);
        array_add(&srv->objects_with_event, event.token);
    }

    size_t n_objects = stbds_hmlen(srv->objects);
    for (size_t idx = 0; idx < n_objects; ++idx) {
        IoObject *obj = &srv->objects[idx].value;
        switch (obj->type) {
        case IoObjectType_AuthConnection:
            if (array_empty(&obj->auth_connection.outgoing)) {
                break;
            }
            if ((err = AuthConnection_FlushOutgoingBuffer(&obj->auth_connection)) != 0) {
                AuthSrv_CloseAuthConnection(srv, &obj->auth_connection);
            }
            break;
        case IoObjectType_CtrlConnection:
            if (array_empty(&obj->ctrl_connection.outgoing)) {
                break;
            }
            if ((err = CtrlConn_FlushOutgoingBuffer(&obj->ctrl_connection)) != 0) {
                // CtrlConn_CloseConnection(srv, &obj->ctrl_connection);
            }
            break;
        default:
            break;
        }
    }

    for (size_t idx = 0; idx < srv->objects_to_remove.len; ++idx) {
        uintptr_t token = array_at(&srv->objects_to_remove, idx);
        stbds_hmdel(srv->objects, token);
    }

    array_clear(&srv->objects_to_remove);

    for (size_t idx = 0; idx < srv->objects_with_event.len; ++idx) {
        IoObject *obj;
        uintptr_t token = array_at(&srv->objects_with_event, idx);
        if ((obj = AuthSrv_GetObject(srv, token)) == NULL) {
            continue;
        }

        int flags = IOCPF_READ;
        switch (obj->type) {
        case IoObjectType_Listener:
            if ((err = iocp_reregister(&srv->iocp, &obj->listener, IOCPF_READ)) != 0) {
                log_error("Failed to re-register listener %04" PRIXPTR ", err: %d", token, err);
            }
            break;
        case IoObjectType_Connection:
            if (!obj->connection.writable) {
                flags |= IOCPF_WRITE;
            }
            if ((err = iocp_reregister(&srv->iocp, &obj->connection.source, flags)) != 0) {
                log_error("Couldn't re-register connection %04" PRIXPTR ", err: %d", token, err);
            }
            break;
        case IoObjectType_AuthConnection:
            if (!obj->auth_connection.writable) {
                flags |= IOCPF_WRITE;
            }
            if ((err = iocp_reregister(&srv->iocp, &obj->auth_connection.source, flags)) != 0) {
                log_error("Couldn't re-register connection %04" PRIXPTR ", err: %d", token, err);
            }
            break;
        case IoObjectType_CtrlConnection:
            if (!obj->ctrl_connection.writable) {
                flags |= IOCPF_WRITE;
            }
            if ((err = iocp_reregister(&srv->iocp, &obj->ctrl_connection.source, flags)) != 0) {
                log_error("Couldn't re-register connection %04" PRIXPTR ", err: %d", token, err);
            }
            break;
        default:
            abort();
        }
    }

    array_clear(&srv->objects_with_event);
}
