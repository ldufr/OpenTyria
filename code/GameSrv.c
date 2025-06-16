#pragma once

#define GAME_SRV_READ_CHUNK_SIZE 4096
#define GAME_SRV_TIME_BETWEEN_PING_MS 5000
#define GAME_SRV_MAX_TIME_BETWEEN_WORLD_TICK_MS 500

void GameConnection_Free(GameConnection *conn)
{
    array_free(&conn->incoming);
    array_free(&conn->outgoing);
    IoSource_free(&conn->source);
}

int GameConnection_FlushOutgoingBuffer(GameConnection *conn)
{
    int err;
    size_t bytes_sent;
    if ((err = sys_send(conn->source.socket, conn->outgoing.ptr, conn->outgoing.len, &bytes_sent)) != 0) {
        if (sys_would_block(err)) {
            conn->writable = false;
            return ERR_OK;
        }

        log_error("Failed to send %zu bytes, err: %d", conn->outgoing.len, err);
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

int GameSrv_Setup(GameSrv *srv)
{
    int err;

    if ((err = iocp_setup(&srv->iocp)) != 0) {
        return err;
    }

    if ((err = sys_mutex_init(&srv->mtx)) != 0) {
        return err;
    }

    if ((err = random_init_from_sys(&srv->random)) != 0) {
        return err;
    }

    const char *path = "db/database.db";
    if ((err = Db_Open(&srv->database, path)) != 0) {
        return err;
    }

    srv->creation_instance_time = sys_get_monotonic_time_ms();
    return ERR_OK;
}

void GameSrv_Free(GameSrv *srv)
{
    iocp_free(&srv->iocp);
    stbds_hmfree(srv->connections);
    array_free(&srv->connections_with_event);
    array_free(&srv->connections_to_remove);
    array_free(&srv->events);
    sys_mutex_free(&srv->mtx);
    array_free(&srv->admin_messages);
    array_free(&srv->clients);
    array_free(&srv->players);
    array_free(&srv->free_players_slots);
    array_free(&srv->items);
    array_free(&srv->free_items_slots);
    array_free(&srv->agents);
    array_free(&srv->free_agents_slots);
    array_free(&srv->text_builder);
    Db_Close(&srv->database);
}

GameConnection* GameSrv_GetConnection(GameSrv *srv, uintptr_t token)
{
    ptrdiff_t idx;
    if ((idx = stbds_hmgeti(srv->connections, token)) == -1) {
        return NULL;
    }
    return &srv->connections[(size_t)idx].value;
}

GameSrvMsg* GameSrv_BuildMsg(GameSrv *srv, uint16_t header)
{
    memset(&srv->srv_msg, 0, sizeof(srv->srv_msg));
    srv->srv_msg.header = header;
    return &srv->srv_msg;
}

array_uint16_t* GameSrv_GetTextBuilder(GameSrv *srv)
{
    array_clear(&srv->text_builder);
    return &srv->text_builder;
}

int GameConnection_SendMessage(GameConnection *conn, GameSrvMsg *msg, size_t size)
{
    int err;

    assert(msg->header < ARRAY_SIZE(GAME_SMSG_FORMATS));
    MsgFormat format = GAME_SMSG_FORMATS[msg->header];

    assert(msg->header == format.header);
    assert(size == format.unpack_size);

    size_t size_before = array_size(&conn->outgoing);

    uint8_t *dst;
    if ((dst = array_push(&conn->outgoing, MSG_MAX_BUFFER_SIZE)) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    size_t written;
    if ((err = pack_msg(format, &written, msg->buffer, size, dst, MSG_MAX_BUFFER_SIZE)) != 0) {
        log_error("Failed to serialized message %u", msg->header);
        array_shrink(&conn->outgoing, size_before);
        return err;
    }

    array_shrink(&conn->outgoing, size_before + written);
    arc4_crypt_inplace(&conn->cipher_enc, dst, written);
    return ERR_OK;
}

void GameSrv_BroadcastMessage(GameSrv *srv, GameSrvMsg *msg, size_t size)
{
    size_t n_connections = stbds_hmlen(srv->connections);
    for (size_t idx = 0; idx < n_connections; ++idx) {
        GameConnection *conn = &srv->connections[idx].value;
        GameConnection_SendMessage(conn, msg, size);
    }
}

void GameSrv_SendPing(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PING_REQUEST);
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->header));

    buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PING_REPLY);
    GameSrv_PingReply *msg = &buffer->ping_reply;
    msg->ping = 20;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendInstanceHead(GameSrv *srv, GameConnection *conn)
{
    // 0b111111 all
    // 0b111101 faction + nightfall
    // 0b111011 prof + nightfall
    // 0b110111 prof + faction
    static const uint8_t PROPHECIES_UNLOCK = 2;
    static const uint8_t FACTIONS_UNLOCK   = 4;
    static const uint8_t NIGHTFALL_UNLOCK  = 8;
    static const uint8_t DEFAULT_FLAGS     = 0x31;

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_LOAD_HEAD);
    GameSrv_InstanceHead *msg = &buffer->instance_head;
    msg->pve_unlocked_flags = DEFAULT_FLAGS | PROPHECIES_UNLOCK | FACTIONS_UNLOCK | NIGHTFALL_UNLOCK;
    msg->pvp_unlocked_flags = DEFAULT_FLAGS | PROPHECIES_UNLOCK | FACTIONS_UNLOCK | NIGHTFALL_UNLOCK;
    msg->b3 = 0;
    msg->b4 = 0;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendCharacterCreationStart(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHAR_CREATION_START);
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->header));
}

void GameSrv_SendInstancePlayerDataDone(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_PLAYER_DATA_DONE);
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->header));
}

void GameSrv_SendUnlockedPvpHeroes(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_UNLOCKED_PVP_HEROES);
    GameSrv_UnlockedPvpHeroes *msg = &buffer->unlocked_pvp_heroes;
    msg->n_bit_map = ARRAY_SIZE(msg->bit_map);
    for (size_t idx = 0; idx < ARRAY_SIZE(msg->bit_map); ++idx) {
        msg->bit_map[idx] = 0xFFFFFFFF;
    }
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendPvpItems(GameSrv *srv, GameConnection *conn)
{
    typedef struct PvpItem {
        uint16_t id;
        uint32_t n_param;
        uint32_t params[10];
    } PvpItem;

    static PvpItem items[] = {
        {1, 3, {0x24080081, 0x25300102, 0x24B80300}},
        {2, 3, {0x24080082, 0x25300104, 0x24B80B00}},
        {3, 3, {0x24080083, 0x25300106, 0x24B80400}},
        {4, 3, {0x24080084, 0x25300108, 0x24B80500}},
        {5, 3, {0x24080085, 0x2530010A, 0x24B80300}},
        {6, 3, {0x24080086, 0x2530010C, 0x24B80B00}},
        {7, 3, {0x24080087, 0x2530010E, 0x24B80400}},
        {8, 3, {0x24080088, 0x25300110, 0x24B80500}},
        {9, 3, {0x24080089, 0x25300112, 0x24B80300}},
        {10, 3, {0x2408008A, 0x25300114, 0x24B80B00}},
        {11, 3, {0x2408008B, 0x25300116, 0x24B80400}},
        {12, 3, {0x2408008C, 0x25300118, 0x24B80500}},
        {13, 3, {0x2408008D, 0x2530011A, 0x24B80300}},
        {14, 3, {0x2408008E, 0x2530011C, 0x24B80B00}},
        {15, 3, {0x2408008F, 0x2530011E, 0x24B80400}},
        {16, 3, {0x24080090, 0x25300120, 0x24B80500}},
        {17, 3, {0x24080099, 0x25300132, 0x23B8000A}},
        {18, 3, {0x2408009A, 0x25300134, 0x23B8000A}},
        {19, 3, {0x2408009B, 0x25300136, 0x23B8000A}},
        {22, 3, {0x240800AF, 0x2530015F, 0x21E80001}},
        {23, 3, {0x240800AF, 0x2530015F, 0x21E80201}},
        {24, 3, {0x240800AF, 0x2530015F, 0x21E80101}},
        {25, 3, {0x240800AF, 0x2530015F, 0x21E80301}},
        {26, 3, {0x240800B0, 0x25300161, 0x21E80401}},
        {27, 3, {0x240800B0, 0x25300161, 0x21E80501}},
        {28, 3, {0x240800B0, 0x25300161, 0x21E80701}},
        {29, 3, {0x240800B0, 0x25300161, 0x21E80601}},
        {30, 3, {0x240800B1, 0x25300163, 0x21E80C01}},
        {31, 3, {0x240800B1, 0x25300163, 0x21E80A01}},
        {32, 3, {0x240800B1, 0x25300163, 0x21E80801}},
        {33, 3, {0x240800B1, 0x25300163, 0x21E80901}},
        {34, 3, {0x240800B1, 0x25300163, 0x21E80B01}},
        {35, 3, {0x240800B2, 0x25300165, 0x21E80D01}},
        {36, 3, {0x240800B2, 0x25300165, 0x21E80E01}},
        {37, 3, {0x240800B2, 0x25300165, 0x21E80F01}},
        {38, 3, {0x240800B2, 0x25300165, 0x21E81001}},
        {39, 3, {0x240800B3, 0x25300167, 0x21E81501}},
        {40, 3, {0x240800B3, 0x25300167, 0x21E81101}},
        {41, 3, {0x240800B3, 0x25300167, 0x21E81201}},
        {42, 3, {0x240800B3, 0x25300167, 0x21E81301}},
        {43, 3, {0x240800B3, 0x25300167, 0x21E81401}},
        {44, 3, {0x240800B4, 0x25300169, 0x21E81801}},
        {45, 3, {0x240800B4, 0x25300169, 0x21E81701}},
        {46, 3, {0x240800B4, 0x25300169, 0x21E81601}},
        {47, 3, {0x240800B4, 0x25300169, 0x21E81901}},
        {48, 5, {0x240800B5, 0x2530016B, 0x21E80002, 0x2530016B, 0x20D80023}},
        {49, 5, {0x240800B5, 0x2530016B, 0x21E80202, 0x2530016B, 0x20D80023}},
        {50, 5, {0x240800B5, 0x2530016B, 0x21E80102, 0x2530016B, 0x20D80023}},
        {51, 5, {0x240800B5, 0x2530016B, 0x21E80302, 0x2530016B, 0x20D80023}},
        {52, 5, {0x240800B6, 0x2530016D, 0x21E80402, 0x2530016D, 0x20D80023}},
        {53, 5, {0x240800B6, 0x2530016D, 0x21E80502, 0x2530016D, 0x20D80023}},
        {54, 5, {0x240800B6, 0x2530016D, 0x21E80702, 0x2530016D, 0x20D80023}},
        {55, 5, {0x240800B6, 0x2530016D, 0x21E80602, 0x2530016D, 0x20D80023}},
        {56, 5, {0x240800B7, 0x2530016F, 0x21E80C02, 0x2530016F, 0x20D80023}},
        {57, 5, {0x240800B7, 0x2530016F, 0x21E80A02, 0x2530016F, 0x20D80023}},
        {58, 5, {0x240800B7, 0x2530016F, 0x21E80802, 0x2530016F, 0x20D80023}},
        {59, 5, {0x240800B7, 0x2530016F, 0x21E80902, 0x2530016F, 0x20D80023}},
        {60, 5, {0x240800B7, 0x2530016F, 0x21E80B02, 0x2530016F, 0x20D80023}},
        {61, 5, {0x240800B8, 0x25300171, 0x21E80D02, 0x25300171, 0x20D80023}},
        {62, 5, {0x240800B8, 0x25300171, 0x21E80E02, 0x25300171, 0x20D80023}},
        {63, 5, {0x240800B8, 0x25300171, 0x21E80F02, 0x25300171, 0x20D80023}},
        {64, 5, {0x240800B8, 0x25300171, 0x21E81002, 0x25300171, 0x20D80023}},
        {65, 5, {0x240800B9, 0x25300173, 0x21E81502, 0x25300173, 0x20D80023}},
        {66, 5, {0x240800B9, 0x25300173, 0x21E81102, 0x25300173, 0x20D80023}},
        {67, 5, {0x240800B9, 0x25300173, 0x21E81202, 0x25300173, 0x20D80023}},
        {68, 5, {0x240800B9, 0x25300173, 0x21E81302, 0x25300173, 0x20D80023}},
        {69, 5, {0x240800B9, 0x25300173, 0x21E81402, 0x25300173, 0x20D80023}},
        {70, 5, {0x240800BA, 0x25300175, 0x21E81802, 0x25300175, 0x20D80023}},
        {71, 5, {0x240800BA, 0x25300175, 0x21E81702, 0x25300175, 0x20D80023}},
        {72, 5, {0x240800BA, 0x25300175, 0x21E81602, 0x25300175, 0x20D80023}},
        {73, 5, {0x240800BA, 0x25300175, 0x21E81902, 0x25300175, 0x20D80023}},
        {74, 5, {0x240800BB, 0x25300177, 0x21E80003, 0x25300177, 0x20D8004B}},
        {75, 5, {0x240800BB, 0x25300177, 0x21E80203, 0x25300177, 0x20D8004B}},
        {76, 5, {0x240800BB, 0x25300177, 0x21E80103, 0x25300177, 0x20D8004B}},
        {77, 5, {0x240800BB, 0x25300177, 0x21E80303, 0x25300177, 0x20D8004B}},
        {78, 5, {0x240800BC, 0x25300179, 0x21E80403, 0x25300179, 0x20D8004B}},
        {79, 5, {0x240800BC, 0x25300179, 0x21E80503, 0x25300179, 0x20D8004B}},
        {80, 5, {0x240800BC, 0x25300179, 0x21E80703, 0x25300179, 0x20D8004B}},
        {81, 5, {0x240800BC, 0x25300179, 0x21E80603, 0x25300179, 0x20D8004B}},
        {82, 5, {0x240800BD, 0x2530017B, 0x21E80C03, 0x2530017B, 0x20D8004B}},
        {83, 5, {0x240800BD, 0x2530017B, 0x21E80A03, 0x2530017B, 0x20D8004B}},
        {84, 5, {0x240800BD, 0x2530017B, 0x21E80803, 0x2530017B, 0x20D8004B}},
        {85, 5, {0x240800BD, 0x2530017B, 0x21E80903, 0x2530017B, 0x20D8004B}},
        {86, 5, {0x240800BD, 0x2530017B, 0x21E80B03, 0x2530017B, 0x20D8004B}},
        {87, 5, {0x240800BE, 0x2530017D, 0x21E80D03, 0x2530017D, 0x20D8004B}},
        {88, 5, {0x240800BE, 0x2530017D, 0x21E80E03, 0x2530017D, 0x20D8004B}},
        {89, 5, {0x240800BE, 0x2530017D, 0x21E80F03, 0x2530017D, 0x20D8004B}},
        {90, 5, {0x240800BE, 0x2530017D, 0x21E81003, 0x2530017D, 0x20D8004B}},
        {91, 5, {0x240800BF, 0x2530017F, 0x21E81503, 0x2530017F, 0x20D8004B}},
        {92, 5, {0x240800BF, 0x2530017F, 0x21E81103, 0x2530017F, 0x20D8004B}},
        {93, 5, {0x240800BF, 0x2530017F, 0x21E81203, 0x2530017F, 0x20D8004B}},
        {94, 5, {0x240800BF, 0x2530017F, 0x21E81303, 0x2530017F, 0x20D8004B}},
        {95, 5, {0x240800BF, 0x2530017F, 0x21E81403, 0x2530017F, 0x20D8004B}},
        {96, 5, {0x240800C0, 0x25300181, 0x21E81803, 0x25300181, 0x20D8004B}},
        {97, 5, {0x240800C0, 0x25300181, 0x21E81703, 0x25300181, 0x20D8004B}},
        {98, 5, {0x240800C0, 0x25300181, 0x21E81603, 0x25300181, 0x20D8004B}},
        {99, 5, {0x240800C0, 0x25300181, 0x21E81903, 0x25300181, 0x20D8004B}},
        {100, 3, {0x24080091, 0x25300122, 0x21080005}},
        {101, 3, {0x24080092, 0x25300124, 0x246801DE}},
        {102, 3, {0x24080093, 0x25300126, 0x246801DE}},
        {103, 3, {0x24080094, 0x25300128, 0x246801E1}},
        {104, 3, {0x24080095, 0x2530012A, 0x246801E1}},
        {105, 3, {0x24080096, 0x2530012C, 0x246801E2}},
        {106, 3, {0x24080097, 0x2530012E, 0x246801E2}},
        {107, 3, {0x24080098, 0x25300130, 0x246801E2}},
        {108, 3, {0x2408009C, 0x25300138, 0x22D80005}},
        {109, 3, {0x2408009D, 0x2530013A, 0x23481E00}},
        {110, 3, {0x2408009E, 0x2530013C, 0x246801E4}},
        {111, 3, {0x2408009F, 0x2530013E, 0x246801E4}},
        {112, 3, {0x240800A0, 0x25300140, 0x246801E4}},
        {113, 3, {0x240800A1, 0x25300142, 0x246801E6}},
        {114, 3, {0x240800A2, 0x25300144, 0x246801E6}},
        {115, 5, {0x240800A3, 0x25300146, 0x25180001, 0x25300146, 0x20C80001}},
        {116, 5, {0x240800A4, 0x25300148, 0x25180001, 0x25300148, 0x20C80001}},
        {117, 5, {0x240800A5, 0x2530014A, 0x25180001, 0x2530014A, 0x20C80001}},
        {118, 5, {0x240800A6, 0x2530014C, 0x25180001, 0x2530014C, 0x20C80001}},
        {119, 5, {0x240800A7, 0x2530014E, 0x25280300, 0x2530014E, 0x20E80001}},
        {120, 5, {0x240800A8, 0x25300150, 0x25280500, 0x25300150, 0x20E80001}},
        {121, 5, {0x240800A9, 0x25300152, 0x25280500, 0x25300152, 0x20E80001}},
        {122, 5, {0x240800AA, 0x25300154, 0x25280300, 0x25300154, 0x20E80001}},
        {123, 3, {0x24080182, 0x25300305, 0x21E82C01}},
        {124, 3, {0x24080182, 0x25300305, 0x21E82B01}},
        {125, 3, {0x24080182, 0x25300305, 0x21E82901}},
        {126, 3, {0x24080182, 0x25300305, 0x21E82A01}},
        {127, 3, {0x240800C5, 0x2530018B, 0x21080005}},
        {128, 3, {0x240800C6, 0x2530018D, 0x21080005}},
        {129, 3, {0x240800C7, 0x2530018F, 0x21280007}},
        {130, 3, {0x240800C8, 0x25300191, 0x21280007}},
        {131, 3, {0x240800C9, 0x25300193, 0x21280007}},
        {132, 3, {0x240800CA, 0x25300195, 0x21280007}},
        {133, 3, {0x240800CB, 0x25300197, 0x21280007}},
        {134, 3, {0x240800CC, 0x25300199, 0x21080005}},
        {135, 3, {0x240800CD, 0x2530019B, 0x21580007}},
        {136, 3, {0x240800CE, 0x2530019D, 0x21580007}},
        {137, 3, {0x240800CF, 0x2530019F, 0x21580007}},
        {138, 3, {0x240800D0, 0x253001A1, 0x21580007}},
        {139, 3, {0x240800D1, 0x253001A3, 0x21580007}},
        {140, 3, {0x240800D2, 0x253001A5, 0x21080005}},
        {141, 3, {0x240800D3, 0x253001A7, 0x21080005}},
        {142, 3, {0x240800D9, 0x253001B3, 0x23481E00}},
        {143, 3, {0x240800DA, 0x253001B5, 0x23481E00}},
        {144, 3, {0x240800DB, 0x253001B7, 0x23481E00}},
        {145, 3, {0x240800DC, 0x253001B9, 0x23481E00}},
        {146, 3, {0x240800DD, 0x253001BB, 0x23481E00}},
        {147, 3, {0x240800DE, 0x253001BD, 0x22B80014}},
        {148, 3, {0x240800DF, 0x253001BF, 0x22B80014}},
        {149, 3, {0x240800E0, 0x253001C1, 0x22B80014}},
        {150, 3, {0x240800E1, 0x253001C3, 0x22B80014}},
        {151, 3, {0x240800E2, 0x253001C5, 0x22B80014}},
        {152, 3, {0x240800E8, 0x253001D1, 0x24181214}},
        {153, 3, {0x240800E9, 0x253001D3, 0x24181914}},
        {154, 3, {0x240800EA, 0x253001D5, 0x24181314}},
        {155, 3, {0x240800EB, 0x253001D7, 0x24181414}},
        {156, 3, {0x240800FF, 0x253001FF, 0x27E802C2}},
        {157, 3, {0x24080100, 0x25300201, 0x27E902C2}},
        {158, 3, {0x24080101, 0x25300203, 0x27EA02C2}},
        {159, 3, {0x240800FC, 0x253001F9, 0x27E802EA}},
        {160, 3, {0x240800FD, 0x253001FB, 0x27E902EA}},
        {161, 3, {0x240800FE, 0x253001FD, 0x27EA02EA}},
        {162, 3, {0x2408013B, 0x25300277, 0x21E82301}},
        {163, 3, {0x2408013B, 0x25300277, 0x21E81D01}},
        {164, 3, {0x2408013B, 0x25300277, 0x21E81E01}},
        {165, 3, {0x2408013B, 0x25300277, 0x21E81F01}},
        {166, 3, {0x2408013E, 0x2530027D, 0x21E82201}},
        {167, 3, {0x2408013E, 0x2530027D, 0x21E82101}},
        {168, 3, {0x2408013E, 0x2530027D, 0x21E82001}},
        {169, 3, {0x2408013E, 0x2530027D, 0x21E82401}},
        {170, 5, {0x2408013C, 0x25300279, 0x21E82302, 0x25300279, 0x20D80023}},
        {171, 5, {0x2408013C, 0x25300279, 0x21E81D02, 0x25300279, 0x20D80023}},
        {172, 5, {0x2408013C, 0x25300279, 0x21E81E02, 0x25300279, 0x20D80023}},
        {173, 5, {0x2408013C, 0x25300279, 0x21E81F02, 0x25300279, 0x20D80023}},
        {174, 5, {0x2408013F, 0x2530027F, 0x21E82202, 0x2530027F, 0x20D80023}},
        {175, 5, {0x2408013F, 0x2530027F, 0x21E82102, 0x2530027F, 0x20D80023}},
        {176, 5, {0x2408013F, 0x2530027F, 0x21E82002, 0x2530027F, 0x20D80023}},
        {177, 5, {0x2408013F, 0x2530027F, 0x21E82402, 0x2530027F, 0x20D80023}},
        {178, 5, {0x2408013D, 0x2530027B, 0x21E82303, 0x2530027B, 0x20D8004B}},
        {179, 5, {0x2408013D, 0x2530027B, 0x21E81D03, 0x2530027B, 0x20D8004B}},
        {180, 5, {0x2408013D, 0x2530027B, 0x21E81E03, 0x2530027B, 0x20D8004B}},
        {181, 5, {0x2408013D, 0x2530027B, 0x21E81F03, 0x2530027B, 0x20D8004B}},
        {182, 5, {0x24080140, 0x25300281, 0x21E82203, 0x25300281, 0x20D8004B}},
        {183, 5, {0x24080140, 0x25300281, 0x21E82103, 0x25300281, 0x20D8004B}},
        {184, 5, {0x24080140, 0x25300281, 0x21E82003, 0x25300281, 0x20D8004B}},
        {185, 5, {0x24080140, 0x25300281, 0x21E82403, 0x25300281, 0x20D8004B}},
        {186, 3, {0x2408012E, 0x2530025C, 0x24B80300}},
        {187, 3, {0x2408012F, 0x2530025E, 0x24B80B00}},
        {188, 3, {0x24080130, 0x25300260, 0x24B80500}},
        {189, 3, {0x24080131, 0x25300262, 0x24B80400}},
        {190, 5, {0x24080132, 0x25300264, 0x25180001, 0x25300264, 0x20C80001}},
        {191, 5, {0x24080133, 0x25300266, 0x25280300, 0x25300266, 0x20E80001}},
        {192, 3, {0x24080135, 0x2530026A, 0x246801DE}},
        {193, 3, {0x24080136, 0x2530026C, 0x246801E1}},
        {194, 3, {0x24080137, 0x2530026E, 0x246801E2}},
        {195, 3, {0x24080138, 0x25300270, 0x246801E4}},
        {196, 3, {0x24080139, 0x25300272, 0x246801E5}},
        {197, 3, {0x2408013A, 0x25300274, 0x23B8000A}},
        {198, 3, {0x24080185, 0x2530030B, 0x21E82801}},
        {199, 3, {0x24080146, 0x2530028D, 0x24181D14}},
        {200, 3, {0x24080141, 0x25300283, 0x21080005}},
        {201, 3, {0x24080143, 0x25300287, 0x21580007}},
        {202, 3, {0x24080142, 0x25300285, 0x21280007}},
        {203, 3, {0x24080144, 0x25300289, 0x22B80014}},
        {204, 3, {0x24080145, 0x2530028B, 0x23481E00}},
        {205, 3, {0x24080147, 0x2530028E, 0x246801DE}},
        {206, 3, {0x24080148, 0x25300290, 0x246801E1}},
        {207, 3, {0x24080149, 0x25300292, 0x246801E5}},
        {208, 3, {0x240800AB, 0x25300156, 0x23F81414}},
        {209, 3, {0x240800AD, 0x2530015A, 0x23F81414}},
        {210, 3, {0x240800AC, 0x25300158, 0x23F81414}},
        {211, 3, {0x240800AE, 0x2530015C, 0x23F81414}},
        {212, 3, {0x24080134, 0x25300268, 0x23F81414}},
        {213, 3, {0x24080185, 0x2530030B, 0x21E82701}},
        {214, 3, {0x24080185, 0x2530030B, 0x21E82601}},
        {215, 3, {0x24080185, 0x2530030B, 0x21E82501}},
        {216, 5, {0x24080183, 0x25300307, 0x21E82C02, 0x25300307, 0x20D80023}},
        {217, 5, {0x24080183, 0x25300307, 0x21E82B02, 0x25300307, 0x20D80023}},
        {218, 5, {0x24080183, 0x25300307, 0x21E82902, 0x25300307, 0x20D80023}},
        {219, 5, {0x24080183, 0x25300307, 0x21E82A02, 0x25300307, 0x20D80023}},
        {220, 5, {0x24080186, 0x2530030D, 0x21E82802, 0x2530030D, 0x20D80023}},
        {221, 5, {0x24080186, 0x2530030D, 0x21E82702, 0x2530030D, 0x20D80023}},
        {222, 5, {0x24080186, 0x2530030D, 0x21E82602, 0x2530030D, 0x20D80023}},
        {223, 5, {0x24080186, 0x2530030D, 0x21E82502, 0x2530030D, 0x20D80023}},
        {224, 5, {0x24080184, 0x25300309, 0x21E82C03, 0x25300309, 0x20D8004B}},
        {225, 5, {0x24080184, 0x25300309, 0x21E82B03, 0x25300309, 0x20D8004B}},
        {226, 5, {0x24080184, 0x25300309, 0x21E82903, 0x25300309, 0x20D8004B}},
        {227, 5, {0x24080184, 0x25300309, 0x21E82A03, 0x25300309, 0x20D8004B}},
        {228, 5, {0x24080187, 0x2530030F, 0x21E82803, 0x2530030F, 0x20D8004B}},
        {229, 5, {0x24080187, 0x2530030F, 0x21E82703, 0x2530030F, 0x20D8004B}},
        {230, 5, {0x24080187, 0x2530030F, 0x21E82603, 0x2530030F, 0x20D8004B}},
        {231, 5, {0x24080187, 0x2530030F, 0x21E82503, 0x2530030F, 0x20D8004B}},
        {232, 3, {0x2408016B, 0x253002D6, 0x24B80300}},
        {233, 3, {0x2408016C, 0x253002D8, 0x24B80B00}},
        {234, 5, {0x2408016F, 0x253002DE, 0x25180001, 0x253002DE, 0x20C80001}},
        {235, 5, {0x24080171, 0x253002E2, 0x25280500, 0x253002E2, 0x20E80001}},
        {236, 3, {0x24080173, 0x253002E6, 0x23F81414}},
        {237, 3, {0x24080174, 0x253002E8, 0x246801DE}},
        {238, 3, {0x24080175, 0x253002EA, 0x246801E1}},
        {239, 3, {0x24080176, 0x253002EC, 0x246801E2}},
        {240, 3, {0x24080179, 0x253002F2, 0x23B8000A}},
        {241, 3, {0x24080177, 0x253002EE, 0x246801E4}},
        {242, 3, {0x24080178, 0x253002F0, 0x246801E6}},
        {243, 3, {0x2408018D, 0x2530031B, 0x24182914}},
        {244, 3, {0x24080188, 0x25300311, 0x21080005}},
        {245, 3, {0x2408018A, 0x25300315, 0x21580007}},
        {246, 3, {0x24080189, 0x25300313, 0x21280007}},
        {247, 3, {0x2408018B, 0x25300317, 0x22B80014}},
        {248, 3, {0x2408018C, 0x25300319, 0x23481E00}},
        {249, 3, {0x2408016D, 0x253002DA, 0x24B80500}},
        {250, 3, {0x2408016E, 0x253002DC, 0x24B80400}},
        {251, 5, {0x24080170, 0x253002E0, 0x25180001, 0x253002E0, 0x20C80001}},
        {252, 5, {0x24080172, 0x253002E4, 0x25280300, 0x253002E4, 0x20E80001}},
        {253, 3, {0x2408017A, 0x253002F4, 0x23F81414}},
        {254, 3, {0x2408017B, 0x253002F6, 0x246801DE}},
        {255, 3, {0x2408017C, 0x253002F8, 0x246801E1}},
        {256, 3, {0x2408017D, 0x253002FA, 0x246801E2}},
        {257, 3, {0x24080180, 0x25300300, 0x23B8000A}},
        {258, 3, {0x2408017E, 0x253002FC, 0x246801E4}},
        {259, 3, {0x2408017F, 0x253002FE, 0x246801E5}},
        {260, 3, {0x24080181, 0x25300302, 0x246801E6}},
        {261, 3, {0x24080193, 0x25300327, 0x24182514}},
        {262, 3, {0x2408018E, 0x2530031D, 0x21080005}},
        {263, 3, {0x24080190, 0x25300321, 0x21580007}},
        {264, 3, {0x2408018F, 0x2530031F, 0x21280007}},
        {265, 3, {0x24080191, 0x25300323, 0x22B80014}},
        {266, 3, {0x24080192, 0x25300325, 0x23481E00}},
        {267, 3, {0x2408021B, 0x25300437, 0x23882D00}},
        {268, 3, {0x2408021A, 0x25300435, 0x23783C00}},
        {269, 3, {0x2408020B, 0x25300416, 0x24B80500}},
        {270, 3, {0x2408020C, 0x25300418, 0x24B80400}},
        {271, 3, {0x2408020D, 0x2530041A, 0x24B80300}},
        {272, 3, {0x2408020E, 0x2530041C, 0x24B80B00}},
        {273, 3, {0x2408020F, 0x2530041E, 0x22080A00}},
        {274, 3, {0x24080154, 0x253002A9, 0x23682D00}},
        {275, 3, {0x24080156, 0x253002AD, 0x23882D00}},
        {276, 3, {0x24080155, 0x253002AB, 0x23783C00}},
        {277, 3, {0x2408015E, 0x253202BC, 0x23A80A00}},
        {278, 3, {0x240801D9, 0x253203B2, 0x22F80005}},
        {279, 3, {0x240801DB, 0x253203B6, 0x23183207}},
        {280, 3, {0x240801DC, 0x253203B8, 0x23280007}},
        {281, 3, {0x240801DD, 0x253203BA, 0x22080A00}},
        {282, 3, {0x24080163, 0x253202C6, 0x2258000F}},
        {283, 3, {0x24080164, 0x253202C8, 0x2268000F}},
        {284, 3, {0x240801D0, 0x253203A0, 0x28580700}},
        {285, 3, {0x240801D1, 0x253203A2, 0x28580800}},
        {286, 3, {0x24080167, 0x253202CE, 0x22980014}},
        {287, 3, {0x24080168, 0x253202D0, 0x22A8000F}},
        {288, 5, {0x24080169, 0x253202D2, 0x2238000F, 0x253202D2, 0x20B80005}},
        {289, 5, {0x2408016A, 0x253202D4, 0x2238000F, 0x253202D4, 0x2018000A}},
        {290, 3, {0x240801E6, 0x253003CC, 0x26D80500}},
        {291, 3, {0x240801E5, 0x253003CA, 0x26C80001}},
        {292, 3, {0x240801E7, 0xA53003CE, 0xA158000A}},
        {293, 5, {0x240801E8, 0x807003D0, 0x80900000, 0xA53003D0, 0xA0F80A00}},
        {294, 5, {0x240801E9, 0x807003D2, 0x80B00600, 0xA53003D2, 0xA0F80A00}},
        {295, 5, {0x240801EA, 0x807003D4, 0x80C00000, 0xA53003D4, 0xA0F80A00}},
        {296, 5, {0x240801EB, 0x807003D6, 0x81100000, 0xA53003D6, 0xA0F80A00}},
        {297, 5, {0x240801DE, 0xA53003BC, 0xA158000A, 0xA53003BC, 0xA118000A}},
        {298, 5, {0x240801DF, 0xA53003BE, 0xA158000A, 0xA53003BE, 0xA118010A}},
        {299, 5, {0x240801E0, 0xA53003C0, 0xA158000A, 0xA53003C0, 0xA118020A}},
        {300, 5, {0x240801E1, 0x807003C2, 0x80900000, 0xA53003C2, 0xA0F80F00}},
        {301, 5, {0x240801E4, 0x807003C8, 0x80A00000, 0xA53003C8, 0xA0F80F00}},
        {302, 3, {0x2408020A, 0x25300414, 0x27E802A9}},
        {303, 5, {0x240801EC, 0x253003D8, 0x28680208, 0xA53003D8, 0xA0F80A00}},
        {304, 3, {0x240801EE, 0xA53003DC, 0xA118010F}},
        {305, 3, {0x240801EF, 0xA53003DE, 0xA6E80500}},
        {306, 5, {0x240801F0, 0x807003E0, 0x80B00400, 0xA53003E0, 0xA0F81400}},
        {307, 5, {0x240801F2, 0xA53003E4, 0xA128000A, 0xA53003E4, 0xA118030A}},
        {308, 5, {0x240801F3, 0xA53003E6, 0xA128000A, 0xA53003E6, 0xA1180B0A}},
        {309, 5, {0x240801F4, 0xA53003E8, 0xA128000A, 0xA53003E8, 0xA118050A}},
        {310, 5, {0x240801F5, 0xA53003EA, 0xA128000A, 0xA53003EA, 0xA118040A}},
        {311, 3, {0x240801F6, 0xA53003EC, 0xA128000A}},
        {312, 5, {0x240801F7, 0x807003EE, 0x80B00800, 0xA53003EE, 0xA0F80F00}},
        {313, 3, {0x240801F9, 0xA53003F2, 0xA7F80300}},
        {314, 5, {0x24080208, 0x25300410, 0x27E802B6, 0xA5300410, 0xA0FBEC00}},
        {315, 3, {0x24080209, 0x25300412, 0x27E802B7}},
        {316, 3, {0x240801FA, 0xA53003F4, 0xA128000A}},
        {317, 5, {0x240801FB, 0x807003F6, 0x8010110D, 0xA53003F6, 0xA1280014}},
        {318, 3, {0x240801FC, 0xA53003F8, 0xA118030F}},
        {319, 3, {0x240801FE, 0xA53003FC, 0xA118050F}},
        {320, 3, {0x240801FF, 0xA53003FE, 0xA118040F}},
        {321, 5, {0x24080201, 0x80700402, 0x80D00000, 0xA5300402, 0xA0F80A00}},
        {322, 3, {0x24080204, 0xA5300408, 0xA6F80500}},
        {323, 5, {0x24080205, 0x8070040A, 0x80B01900, 0xA530040A, 0xA0F80F00}},
        {324, 5, {0x24080206, 0x8070040C, 0x80A00000, 0xA530040C, 0xA0F80F00}},
        {325, 3, {0x240801BC, 0x25320378, 0x21980005}},
        {326, 5, {0x240801C0, 0x25320380, 0x22D8000F, 0x25320380, 0x20C80001}},
        {327, 3, {0x240801C1, 0x25320382, 0x23A80A00}},
        {328, 3, {0x240801C2, 0x25320384, 0x28281400}},
        {329, 3, {0x2408015C, 0x253202B8, 0x22D80005}},
        {330, 3, {0x240801D2, 0x253203A4, 0x20781405}},
        {331, 3, {0x240801D3, 0x253203A6, 0x20880002}},
        {332, 3, {0x240801D4, 0x253203A8, 0x20980003}},
        {333, 3, {0x240801D5, 0x253203AA, 0x20A80002}},
        {334, 3, {0x240801D6, 0x253203AC, 0x28381400}},
        {335, 3, {0x240801D7, 0x253203AE, 0x28081400}},
        {336, 5, {0x240801D8, 0x253203B0, 0x22D8000F, 0x253203B0, 0x20C80001}},
        {337, 3, {0x240801DA, 0x253203B4, 0x23083205}},
        {338, 3, {0x24080165, 0x253202CA, 0x2278320F}},
        {339, 3, {0x24080166, 0x253202CC, 0x22883214}},
        {340, 3, {0x24080218, 0x25300431, 0x23481E00}},
        {341, 3, {0x24080219, 0x25300433, 0x23682D00}},
        {342, 3, {0x2408021C, 0x25300439, 0x22080A00}},
        {343, 3, {0x24080217, 0x2530042F, 0x28081400}},
        {344, 3, {0x24080160, 0x253002C1, 0x23A80A00}},
        {345, 3, {0x2408015F, 0x253002BF, 0x28281400}},
        {346, 3, {0x24080161, 0x253002C3, 0x23481E00}},
        {347, 3, {0x24080162, 0x253002C5, 0x23682D00}},
        {348, 3, {0x24080152, 0x253002A5, 0x23882D00}},
        {349, 3, {0x24080151, 0x253002A3, 0x23783C00}},
        {350, 3, {0x24080210, 0x25300420, 0x28081400}},
        {351, 3, {0x24080153, 0x253002A7, 0x28381400}},
        {352, 3, {0x24080211, 0x25300423, 0x22D80002}},
        {353, 3, {0x24080212, 0x25300425, 0x23480A00}},
        {354, 3, {0x24080213, 0x25300427, 0x27780407}},
        {355, 3, {0x24080214, 0x25300429, 0x27780300}},
        {356, 3, {0x24080215, 0x2530042B, 0x27780801}},
        {357, 3, {0x24080216, 0x2530042D, 0x27780605}},
        {358, 3, {0x240801E2, 0xA53003C4, 0xA7480300}},
        {359, 3, {0x240801E3, 0xA53003C6, 0xA7280500}},
        {360, 3, {0x240801ED, 0xA53003DA, 0xA7380500}},
        {361, 3, {0x240801F1, 0xA53003E2, 0xA7080509}},
        {362, 3, {0x240801F8, 0xA53003F0, 0xA7280500}},
        {363, 3, {0x240801FD, 0xA53003FA, 0xA1180B0F}},
        {364, 5, {0x24080200, 0x80700400, 0x81200000, 0xA5300400, 0xA0F80A00}},
        {365, 3, {0x24080202, 0xA5300404, 0xA7180506}},
        {366, 5, {0x24080203, 0x80700406, 0x81400600, 0xA5300406, 0xA0F80A00}},
        {367, 5, {0x24080207, 0x8070040E, 0x81300000, 0xA530040E, 0xA0F80A00}},
        {368, 5, {0x240801B6, 0x2532036C, 0x21080005, 0x2532036C, 0x20B80005}},
        {369, 5, {0x240801B7, 0x2532036E, 0x21080005, 0x2532036E, 0x20D80014}},
        {370, 3, {0x240801B8, 0x25320370, 0x21280005}},
        {371, 3, {0x240801B9, 0x25320372, 0x21580005}},
        {372, 3, {0x240801BA, 0x25320374, 0x21780005}},
        {373, 3, {0x240801BB, 0x25320376, 0x21880005}},
        {374, 3, {0x240801BD, 0x2532037A, 0x21B8320A}},
        {375, 3, {0x240801BE, 0x2532037C, 0x21A83205}},
        {376, 3, {0x240801BF, 0x2532037E, 0x21C8000A}},
        {377, 3, {0x240801C3, 0xA5320386, 0xA118000A}},
        {378, 3, {0x240801C4, 0xA5320388, 0xA118030A}},
        {379, 3, {0x240801C5, 0xA532038A, 0xA1180B0A}},
        {380, 3, {0x240801C6, 0xA532038C, 0xA118040A}},
        {381, 3, {0x240801C7, 0xA532038E, 0xA118050A}},
        {382, 3, {0x240801C8, 0xA5320390, 0xA118010A}},
        {383, 3, {0x240801C9, 0xA5320392, 0xA118020A}},
        {384, 3, {0x240801CA, 0x25320394, 0x28580000}},
        {385, 3, {0x240801CB, 0x25320396, 0x28580100}},
        {386, 3, {0x240801CC, 0x25320398, 0x28580300}},
        {387, 3, {0x240801CD, 0x2532039A, 0x28580400}},
        {388, 3, {0x240801CE, 0x2532039C, 0x28580500}},
        {389, 3, {0x240801CF, 0x2532039E, 0x28580600}},
    };

    for (size_t idx = 0; idx < ARRAY_SIZE(items); ++idx) {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PVP_ITEM_ADD_UNLOCK);
        GameSrv_PvpItemAddUnlock *msg = &buffer->pvp_item_add_unlock;
        msg->item_id = items[idx].id;
        msg->n_params = items[idx].n_param;
        memcpy_u32(msg->params, items[idx].params, msg->n_params);
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PVP_ITEM_END);
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->header));
}

void GameSrv_SendAccountFeatures(GameSrv *srv, GameConnection *conn)
{
    uint16_t features[][3] = {
        {1, 10, 0},
        {99, 3, 0},
        {100, 5, 0},
        {101, 5, 5},
        {102, 2, 2},
        {111, 1, 0},
        {124, 1, 0},
        {125, 1, 0},
        {131, 1, 0},
    };

    for (size_t idx = 0; idx < ARRAY_SIZE(features); ++idx) {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ACCOUNT_FEATURE);
        GameSrv_AccountFeature *msg = &buffer->account_feature;
        msg->feature_id = features[idx][0];
        msg->param1 = features[idx][1];
        msg->param2 = features[idx][2];
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }
}

void GameSrv_SendPlayerHeroNameAndInfo(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_PLAYER_HERO_NAME_AND_INFO);
    GameSrv_PlayerHeroNameAndInfo *msg = &buffer->player_hero_name_and_info;

    msg->n_charname = 0;
    msg->h0008 = 0;
    msg->h000C = 0;
    msg->h0010 = 1000;
    msg->h0014 = 0;
    msg->h0018 = 0;
    msg->h001C = 0;

    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendUpdateCurrentMap(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_UPDATE_CURRENT_MAP);
    GameSrv_UpdateCurrentMap *msg = &buffer->update_current_map;
    msg->map_id = srv->map_id;
    msg->unk = 0;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendInstancePlayerDataStart(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_PLAYER_DATA_START);
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->header));
}

void GameSrv_SendInstanceLoadInfo(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_LOAD_INFO);
    GameSrv_InstanceInfo *msg = &buffer->instance_info;
    msg->agent = player->agent_id;
    msg->map_id = srv->map_id;
    msg->is_explorable = 0; // what to put here?
    msg->district = srv->district_number;
    msg->language = DistrictLanguage_ToInt(srv->language);
    msg->is_observer = 0;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendUpdateActiveWeaponSet(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ITEM_SET_ACTIVE_WEAPON_SET);
    GameSrv_UpdateActiveWeapon *msg = &buffer->update_active_weapon_set;
    msg->stream = 1;
    msg->slot = 0;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendGoldStorage(GameSrv *srv, GameConnection *conn)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, conn->player_id)) == NULL) {
        return;
    }

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_UPDATE_GOLD_STORAGE);
    GameSrv_UpdateGold *msg = &buffer->update_gold;
    msg->stream = 1;
    msg->gold = player->account.storage_gold;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendWeaponSlots(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ITEM_WEAPON_SET);
    GameSrv_WeaponSet *msg = &buffer->weapon_set;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        msg->stream_id = 1;
        msg->slot = slot;
        msg->leadhand = 0;
        msg->offhand = 0;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }
}

void GameSrv_SendReadyForMapSpawn(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_READY_FOR_MAP_SPAWN);
    GameSrv_ReadyForMapSpawn *msg = &buffer->ready_for_map_spawn;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendInstanceLoaded(GameSrv *srv, GameConnection *conn, GmPlayer *player)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_LOADED);
    GameSrv_InstanceLoaded *msg = &buffer->instance_loaded;
    msg->player_team_token = player->player_team_token;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendInstanceLoadFinish(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_LOAD_FINISH);
    GameConnection_SendMessage(conn, buffer, sizeof(buffer->header));
}

void GameSrv_SendManifest(GameSrv *srv, GameConnection *conn, slice_uint8_t data)
{
    GameSrv_InstanceManifestData *msg;

    size_t count = (data.len + sizeof(msg->data) - 1) / sizeof(msg->data);
    for (size_t idx = 0; idx < count; ++idx) {
        size_t offset = idx * sizeof(msg->data);
        size_t rem = data.len - offset;

        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_DATA);
        msg = &buffer->instance_manifest_data;
        msg->n_data = min_u32((uint32_t)rem, sizeof(msg->data));
        memcpy(msg->data, data.ptr + offset, msg->n_data);

        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }
}

void GameSrv_SendDownloadManifest(GameSrv *srv, GameConnection *conn)
{
    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_PHASE);
        GameSrv_InstanceManifestPhase *msg = &buffer->instance_manifest_phase;
        msg->download_phase = ManifestPhase_Phase1;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_PHASE);
        GameSrv_InstanceManifestPhase *msg = &buffer->instance_manifest_phase;
        msg->download_phase = ManifestPhase_Phase2;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_DONE);
        GameSrv_InstanceManifestDone *msg = &buffer->instance_manifest_done;
        msg->download_phase = ManifestPhase_Done;
        msg->map_id = MapId_Count;
        msg->unk = 0;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_PHASE);
        GameSrv_InstanceManifestPhase *msg = &buffer->instance_manifest_phase;
        msg->download_phase = ManifestPhase_Phase1;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_PHASE);
        GameSrv_InstanceManifestPhase *msg = &buffer->instance_manifest_phase;
        msg->download_phase = ManifestPhase_Phase2;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }

    {
        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_MANIFEST_DONE);
        GameSrv_InstanceManifestDone *msg = &buffer->instance_manifest_done;
        msg->download_phase = ManifestPhase_Phase1;
        msg->map_id = srv->map_id;
        msg->unk = 0;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));
    }
}

void GameSrv_SendInitialPackets(GameSrv *srv, GameConnection *conn)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, conn->player_id)) == NULL) {
        return;
    }

    GameSrv_SendInstanceHead(srv, conn);
    GameSrv_SendInstancePlayerDataStart(srv, conn);
    GameSrv_SendInstanceLoadPlayerName(srv, conn, player);
    GameSrv_SendInstanceLoadInfo(srv, conn, player);
}

void GameSrv_GetMessages(GameSrv *srv, GameConnection *conn)
{
    int err;
    uint16_t header;
    size_t total_consumed = 0;

    while (sizeof(header) <= (conn->incoming.len - total_consumed)) {
        const uint8_t *input = &conn->incoming.ptr[total_consumed];
        size_t size = conn->incoming.len - total_consumed;

        header = le16dec(input) & ~GAME_CMSG_MASK;
        if (ARRAY_SIZE(GAME_CMSG_FORMATS) <= header) {
            IoSource_free(&conn->source);
            array_add(&srv->connections_to_remove, conn->token);
            break;
        }

        MsgFormat format = GAME_CMSG_FORMATS[header];

        size_t consumed;

        GamePlayerMsg *msg = array_push(&srv->player_messages, 1);
        msg->player_id = conn->player_id;

        if ((err = unpack_msg(format, &consumed, input, size, msg->msg.buffer, sizeof(msg->msg.buffer))) != 0) {
            array_pop(&srv->player_messages);

            if (err != ERR_NOT_ENOUGH_DATA) {
                log_warn("Received invalid message from client %04" PRIXPTR, conn->token);
                IoSource_free(&conn->source);
                array_add(&srv->connections_to_remove, conn->token);
            }

            break;
        }

        total_consumed += consumed;
    }

    array_remove_range_ordered(&conn->incoming, 0, total_consumed);
}

void GameSrv_PeerDisconnected(GameSrv *srv, GameConnection *conn)
{
    GameSrv_GetMessages(srv, conn);
    IoSource_free(&conn->source);
    array_add(&srv->connections_to_remove, conn->token);

    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, conn->player_id)) != NULL) {
        player->conn_token = 0;
    }
}

void GameSrv_ProcessEvent(GameSrv *srv, Event event)
{
    UNREFERENCED_PARAMETER(srv);

    int err;

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, event.token)) == NULL) {
        return;
    }

    if ((event.flags & IOCPF_WRITE) != 0) {
        conn->writable = true;
    }

    if ((event.flags & IOCPF_READ) != 0) {
        for (;;) {
            uint8_t *buffer;

            const size_t size_at_starts = conn->incoming.len;
            if ((buffer = array_push(&conn->incoming, GAME_SRV_READ_CHUNK_SIZE)) == NULL) {
                log_error("Out of memory will reading from socket");
                break;
            }

            size_t bytes;
            if ((err = sys_recv(conn->source.socket, buffer, GAME_SRV_READ_CHUNK_SIZE, &bytes)) != 0) {
                array_shrink(&conn->incoming, size_at_starts);
                if (!sys_would_block(err)) {
                    log_error(
                        "Error while reading %04" PRIXPTR ", err: %d",
                        conn->source.socket,
                        err
                    );

                    GameSrv_PeerDisconnected(srv, conn);
                }

                break;
            }

            uint8_t *ptr = &array_at(&conn->incoming, size_at_starts);
            arc4_crypt_inplace(&conn->cipher_dec, ptr, bytes);
            array_shrink(&conn->incoming, size_at_starts + bytes);

            if (bytes == 0) {
                GameSrv_PeerDisconnected(srv, conn);
                break;
            }
        }

        GameSrv_GetMessages(srv, conn);
    }
}

void GameSrv_Poll(GameSrv *srv)
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

void GameSrv_CreatePlayerAgent(GameSrv *srv, GmPlayer *player)
{
    GmAgent *agent = GameSrv_CreateAgent(srv);
    agent->position.x = -9067.f;
    agent->position.y = 13218.f;
    agent->direction.x = 1.f;
    agent->direction.y = 0.f;
    agent->model_id = CHAR_CLASS_PLAYER_BASE | player->player_id;
    agent->agent_type = AgentType_Living;
    agent->speed_base = 288.f;
    agent->health_max = 400;
    agent->energy_max = 25;
    agent->player_team_token = player->player_team_token;

    player->agent_id = agent->agent_id;
}

void GameSrv_CreatePlayerParty(GameSrv *srv, GmPlayer *player)
{
    GmParty *party = GameSrv_CreateParty(srv);
    GmParty_AddPlayer(party, player->agent_id, player->player_id);
    player->party_id = party->party_id;
}

void GameSrv_LoadPlayerFromDatabase(GameSrv *srv, GmPlayer *player)
{
    int err;

    if (uuid_is_null(&player->char_id)) {
        if ((err = Db_GetAccount(
                &srv->database,
                player->account_id,
                &player->account)) != 0)
        {
            log_error("Could't load the character for this user from database");
            return;
        }
    } else {
        if ((err = Db_GetCharacterAndAccount(
                &srv->database,
                player->account_id,
                player->char_id,
                &player->account,
                &player->character)) != 0)
        {
            log_error("Could't load the character for this user from database");
            return;
        }

        if ((err = Profession_FromInt(player->character.primary_profession, &player->primary_profession)) != 0) {
            log_error("Invalid profession value %u received from database", player->character.primary_profession);
            player->primary_profession = Profession_None;
        }
    }

    size_t count;
    DbBag bags[BagModelId_Count];
    if ((err = Db_CharacterBags(&srv->database, player->account_id, player->char_id, bags, ARRAY_SIZE(bags), &count)) != 0) {
        // We should shutdown this player with a server error.
        log_error("Could't load the bags for this user from database");
        return;
    }

    for (size_t idx = 0; idx < count; ++idx) {
        BagType bag_type;
        BagModelId bag_model_id;
        if ((err = BagModelId_FromInt((int)bags[idx].bag_model_id, &bag_model_id)) != 0 ||
            (err = BagType_FromInt((int)bags[idx].bag_type, &bag_type)) != 0)
        {
            log_error(
                "Invalid bag at idx %zu, model id: %u, bag type: %u",
                bags[idx].bag_model_id,
                bags[idx].bag_type
            );
            return;
        }

        GmBag *bag = &player->bags.bags[bag_model_id];
        bag->bag_id = ++srv->next_bag_id;
        bag->bag_model_id = bag_model_id;
        bag->bag_type = bags[idx].bag_type;
        bag->slot_count = bags[idx].slot_count;
        memset(bag->items, 0, sizeof(bag->items));

        if (bag_model_id == BagModelId_Backpack) {
            GmItem* backpack_item = GameSrv_AllocateItem(srv);
            if (backpack_item != NULL) {
                bag->bag_item_id = backpack_item->item_id;
                *backpack_item = g_Backpack;
                backpack_item->item_id = bag->bag_item_id;
            }
        }
    }

    DbItemArray items = {0};
    if ((err = Db_GetItems(&srv->database, player->account_id, player->char_id, &items)) != 0) {
        log_error("Couldn't load the items from database");
        return;
    }

    for (size_t idx = 0; idx < items.len; ++idx) {
        DbItem *dbitem = &items.ptr[idx];

        BagModelId bag_model_id;
        if ((err = BagModelId_FromInt(dbitem->bag_model_id, &bag_model_id)) != 0) {
            log_warn("Invalid bag model id %u from databasae", dbitem->bag_model_id);
            continue;
        }

        GmBag *bag = &player->bags.bags[bag_model_id];
        if (bag->bag_id == 0) {
            log_warn("Got an dbitem for a bag that isn't existing for the user");
            continue;
        }

        if (bag->slot_count < dbitem->slot) {
            log_warn(
                "Got an dbitem in bag %s at slot %u, but max slot is %u",
                BagModelId_ToString(bag_model_id),
                dbitem->slot,
                bag->slot_count
            );
            continue;
        }

        GmItem *gmitem = GameSrv_AllocateItem(srv);
        if (gmitem == NULL) {
            log_error("Failed to allocate a GmItem");
            continue;
        }

        assert(gmitem->item_id != 0);
        gmitem->file_id = dbitem->file_id;
        gmitem->flags = dbitem->flags;
        gmitem->model_id = dbitem->model_id;
        gmitem->quantity = dbitem->quantity;
        gmitem->dye_tint = dbitem->dye_tint;

        if ((err = Profession_FromInt(dbitem->profession, &gmitem->profession)) != 0 ||
            (err = ItemType_FromInt(dbitem->item_type, &gmitem->item_type)) != 0 ||
            (err = DyeColor_FromInt(dbitem->dye_colors, &gmitem->dye_colors)) != 0)
        {
            log_error("Failed to parse enum from the database");
            GameSrv_FreeItemId(srv, gmitem->item_id);
            continue;
        }

        bag->items[dbitem->slot] = gmitem->item_id;
    }
}

int GameSrv_CreatePlayerBags(GameSrv *srv, GmPlayer *player)
{
    int err;

    size_t count = 0;
    DbBag bags[BagModelId_Count] = {0};
    DbItemArray items = {0};

    for (size_t idx = 0; idx < BagModelId_Count; ++idx) {
        GmBag *bag = &player->bags.bags[idx];
        if (bag->bag_id == 0) {
            continue;
        }

        if (GmBag_IsVolatile(bag->bag_model_id)) {
            continue;
        }

        DbBag *dst = &bags[count++];
        dst->account_id = player->account_id;
        dst->char_id = player->char_id;
        dst->bag_model_id = bag->bag_model_id;
        dst->bag_type = bag->bag_type;
        dst->slot_count = bag->slot_count;

        for (uint8_t slot = 0; slot < bag->slot_count; ++slot) {
            uint32_t item_id = bag->items[slot];
            if (item_id == 0) {
                continue;
            }

            GmItem *item;
            if ((item = GameSrv_GetItemById(srv, item_id)) == NULL) {
                continue;
            }

            DbItem *dbitem = array_push(&items, 1);
            dbitem->account_id = player->account_id;
            dbitem->char_id = player->char_id;
            dbitem->bag_model_id = bag->bag_model_id;
            dbitem->slot = slot;
            dbitem->file_id = item->file_id;
            dbitem->model_id = item->model_id;
            dbitem->item_type = item->item_type;
            dbitem->dye_tint = item->dye_tint;
            dbitem->dye_colors = item->dye_colors;
            dbitem->quantity = cast_u16(item->quantity);
            dbitem->flags = item->flags;
            dbitem->profession = item->profession;
        }
    }

    if ((err = Db_CreateBags(&srv->database, bags, count)) != 0) {
        log_error("Failed to create %zu bags", count);
        array_free(&items);
        return err;
    }

    if ((err = Db_CreateItems(&srv->database, items.ptr, items.len)) != 0) {
        log_error("Failed to create %zu items", items.len);
        array_free(&items);
        return err;
    }

    array_free(&items);
    return ERR_OK;
}

void GameSrv_HandleTransferUserCmd(GameSrv *srv, AdminMsg_TransferUser *msg)
{
    GameConnection conn = {0};
    conn.token = msg->token;
    conn.source = msg->source;
    arc4_setup(&conn.cipher_enc, msg->cipher_init_key, sizeof(msg->cipher_init_key));
    conn.cipher_dec = conn.cipher_enc;

    int err;
    if ((err = iocp_register(&srv->iocp, &conn.source, conn.token, IOCPF_READ | IOCPF_WRITE)) != 0) {
        log_error("Failed to register a transfered user");
    }

    if (!msg->reconnection) {
        GmPlayer *player = GameSrv_CreatePlayer(srv, msg->token, msg->account_id, msg->char_id);
        conn.player_id = player->player_id;

        player->player_team_token = 0xBAADF00D;
        GameSrv_LoadPlayerFromDatabase(srv, player);
        GameSrv_CreateDefaultBags(srv, player);
        GameSrv_CreatePlayerAgent(srv, player);
        GameSrv_CreatePlayerParty(srv, player);
    } else {
        abort();
    }

    switch (srv->map_type) {
    case MapType_CharacterCreation:
        GameSrv_SendInstanceHead(srv, &conn);
        GameSrv_SendCharacterCreationStart(srv, &conn);
        break;
    case MapType_MainTown:
    case MapType_MainExplorable:
    case MapType_GuildHall:
    case MapType_MissionOutpost:
    case MapType_MissionExplorable:
    case MapType_ArenaOutpost:
    case MapType_ArenaExplorable:
    case MapType_HeroesAscentOutpost:
        GameSrv_SendInitialPackets(srv, &conn);
        break;
    default:
        abort();
    }

    // @TODO: Send all initial packets?
    stbds_hmput(srv->connections, conn.token, conn);
}

void GameSrv_ProcessInternalMessages(GameSrv *srv)
{
    sys_mutex_lock(&srv->mtx);
    AdminMsgArray messages = srv->admin_messages;
    memset(&srv->admin_messages, 0, sizeof(srv->admin_messages));
    sys_mutex_unlock(&srv->mtx);

    for (size_t idx = 0; idx < messages.len; ++idx) {
        AdminMsg *msg = &messages.ptr[idx];
        switch (msg->cmd) {
        case AdminCmd_Quit:
            srv->quit_signaled = true;
            break;
        case AdminCmd_TransferUser:
            GameSrv_HandleTransferUserCmd(srv, &msg->transfer_user);
            break;
        default:
            break;
        }
    }
}

int GameSrv_HandleInstanceLoadRequestSpawn(GameSrv *srv, size_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Unknown player");
        return ERR_OK;
    }

    GmAgent *agent;
    if ((agent = GameSrv_GetAgent(srv, player->agent_id)) == NULL) {
        log_error("Unknown agent %u", player->agent_id);
        return ERR_OK;
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        log_error("Failed to get player connection");
        return ERR_OK;
    }

    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INSTANCE_LOAD_SPAWN_POINT);
    GameSrv_SpawnPoint *msg = &buffer->spawn_point;
    msg->map_file_id = 0x345CC;
    msg->position.x = -9130;
    msg->position.y = 11105;
    msg->plane = 0;
    msg->unk0 = 0;
    msg->is_cinematic = 0;
    memset(msg->unk1, 0, sizeof(msg->unk1));
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));

    return ERR_OK;
}

int GameSrv_HandleInstanceLoadRequestPlayers(GameSrv *srv, size_t player_id, GameSrv_RequestPlayers *msg)
{
    UNREFERENCED_PARAMETER(msg);

    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Unknown player");
        return ERR_OK;
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        log_error("Failed to get player connection");
        return ERR_OK;
    }

    GmAgent *agent;
    if ((agent = GameSrv_GetAgent(srv, player->agent_id)) == NULL) {
        log_error("Unknown agent");
        return ERR_OK;
    }

    agent->load_time = srv->current_instance_time;

    GameSrv_SendUpdatePvpUnlockedSkills(srv, conn, player);
    GameSrv_SendUnlockedPvpHeroes(srv, conn);
    GameSrv_SendPvpItems(srv, conn);
    GameSrv_SendAccountFeatures(srv, conn);
    GameSrv_SendUnlockedMaps(srv, conn, player);
    // GAME_SMSG_VANQUISH_PROGRESS
    GameSrv_SendInstanceLoaded(srv, conn, player);
    // RecvPacket (1DBB5790): 123, 0x7B, unknown
    // RecvPacket (1DBB5790): 124, 0x7C, unknown
    GameSrv_SendSkillsAndAttributes(srv, conn, player);
    // GameSrv_SendCartographyData
    GameSrv_SendPlayerFactions(srv, conn, player);
    // GameSrv_SendPlayerTitles
    GameSrv_SendPlayerAttributes(srv, conn, player);
    GameSrv_SendAgentLoadTime(srv, conn, agent);
    GameSrv_BroadcastUpdatePlayerInfo(srv, player);
    GameSrv_SendPlayerProfession(srv, conn, player);
    // GAME_SMSG_TITLE_RANK_DISPLAY

    GameSrv_SendWorldAgents(srv, conn, agent);
    GameSrv_BroadcastCreateAgent(srv, agent);
    GameSrv_SendAgentHealthEnergy(srv, conn, agent);
    GameSrv_BroadcastAgentLevel(srv, agent);
    GameSrv_BroadcastAgentInitialEffects(srv, agent);
    GameSrv_BroadcastPlayerEquippedItems(srv, player);
    GameSrv_BroadcastUpdateAgentVisualEquipment(srv, agent, &player->bags);
    GameSrv_BroadcastAgentDisplayCape(srv, agent);
    GameSrv_SendUpdatePlayerAgent(srv, conn, agent);
    GameSrv_SendPlayerParty(srv, conn, player->party_id);
    GameSrv_SendUpdatePlayerParty(srv, conn, player->party_id);

    GameSrv_SendInstanceLoadFinish(srv, conn);

    return ERR_OK;
}

int GameSrv_HandleInstanceLoadRequestItems(GameSrv *srv, size_t player_id, GameSrv_RequestItems *msg)
{
    UNREFERENCED_PARAMETER(msg);

    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Unknown player");
        return ERR_OK;
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        return ERR_OK;
    }

    GameSrv_SendItemStreamCreate(srv, conn);
    GameSrv_SendUpdateActiveWeaponSet(srv, conn);
    GameSrv_SendInventory(srv, conn, conn->player_id);
    GameSrv_SendWeaponSlots(srv, conn);
    GameSrv_SendGoldStorage(srv, conn);
    // GameSrv_SendQuests(conn);
    GameSrv_SendPlayerMaxFactions(srv, conn, player);
    GameSrv_SendHardModeUnlocked(srv, conn);
    GameSrv_SendPlayerHeroNameAndInfo(srv, conn);
    GameSrv_SendUpdateCurrentMap(srv, conn);
    GameSrv_SendReadyForMapSpawn(srv, conn);
    GameSrv_SendDownloadManifest(srv, conn);

    return ERR_OK;
}

int GameSrv_HandleCharCreationRequestPlayer(GameSrv *srv, size_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Unknown player");
        return ERR_OK;
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        return ERR_OK;
    }

    player->primary_profession = Profession_Warrior;

    GameSrv_SendInstancePlayerDataStart(srv, conn);
    GameSrv_SendItemStreamCreate(srv, conn);
    GameSrv_SendInventory(srv, conn, conn->player_id);
    GameSrv_SendUpdateActiveWeaponSet(srv, conn);
    GameSrv_SendWeaponSlots(srv, conn);
    GameSrv_SendGoldStorage(srv, conn);
    GameSrv_SendPlayerMaxFactions(srv, conn, player);
    GameSrv_SendSkillsAndAttributes(srv, conn, player);
    GameSrv_SendPlayerFactions(srv, conn, player);
    GameSrv_SendPlayerHealthEnergy(srv, conn, player);
    GameSrv_SendInstancePlayerDataDone(srv, conn);

    return ERR_OK;
}

int GameSrv_HandleCharCreationRequestArmors(GameSrv *srv, size_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Unknown player");
        return ERR_OK;
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        return ERR_OK;
    }

    GameSrv_SendUpdatePvpUnlockedSkills(srv, conn, player);
    GameSrv_SendUnlockedPvpHeroes(srv, conn);
    GameSrv_SendPvpItems(srv, conn);
    GameSrv_SendAccountFeatures(srv, conn);
    return ERR_OK;
}

int GetBagSlotForItemType(ItemType item_type, EquippedItemSlot *result)
{
    switch (item_type) {
    case ItemType_Leadhand:
    case ItemType_Axe:
    case ItemType_Bow:
    case ItemType_Hammer:
    case ItemType_Wand:
    case ItemType_Staff:
    case ItemType_Sword:
    case ItemType_Daggers:
    case ItemType_Scythe:
    case ItemType_Spear:
        *result = EquippedItemSlot_Weapon;
        return ERR_OK;
    case ItemType_Focus:
    case ItemType_Shield:
        *result = EquippedItemSlot_OffHand;
        return ERR_OK;
    case ItemType_Body:
        *result = EquippedItemSlot_Body;
        return ERR_OK;
    case ItemType_Boots:
        *result = EquippedItemSlot_Boots;
        return ERR_OK;
    case ItemType_Gloves:
        *result = EquippedItemSlot_Gloves;
        return ERR_OK;
    case ItemType_Head:
        *result = EquippedItemSlot_Head;
        return ERR_OK;
    case ItemType_Legs:
        *result = EquippedItemSlot_Legs;
        return ERR_OK;
    case ItemType_CostumeBody:
        *result = EquippedItemSlot_CostumeBody;
        return ERR_OK;
    case ItemType_CostumeHead:
        *result = EquippedItemSlot_CostumeHead;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

int GameSrv_HandleCharCreationChangeProf(GameSrv *srv, size_t player_id, GameSrv_CharCreationChangeProf *msg)
{
    int err;

    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        return ERR_SERVER_ERROR;
    }

    if ((err = Profession_FromInt(msg->profession, &player->primary_profession)) != 0) {
        log_warn("Invalid `Profession` value %u", msg->profession);
        return err;
    }

    if ((err = Campaign_FromInt(msg->campaign, &player->char_creation_campaign)) != 0) {
        log_warn("Invalid `Campaign` value %u", msg->campaign);
        return err;
    }

    GmItemSlice items = GetDefaultEquipments(player->char_creation_campaign, player->primary_profession);
    GmBag *bag = &player->bags.equipped_items;
    GameSrv_FreeBagItems(srv, player, bag);

    for (size_t idx = 0; idx < items.len; ++idx) {
        const GmItem *item_def = &items.ptr[idx];
        EquippedItemSlot item_slot;
        if ((err = GetBagSlotForItemType(item_def->item_type, &item_slot)) != 0) {
            log_warn(
                "Failed to get an item slot for the item of type %s (%d)",
                ItemType_ToString(item_def->item_type),
                item_def->item_type
            );
            continue;
        }
        GmItem *item = GameSrv_AllocateItem(srv);
        uint32_t item_id = item->item_id;

        // This will override the `item_id` to 0, but we restore it. 
        *item = *item_def;
        item->item_id = item_id;

        GmBag_SetItem(bag, item_slot, item_id);
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        return ERR_OK;
    }

    GameSrv_SendUpdatePveUnlockedSkills(srv, conn, player);
    GameSrv_SendPlayerProfession(srv, conn, player);
    GameSrv_SendBagItems(srv, conn, &player->bags.equipped_items);
    return ERR_OK;
}

int GameSrv_HandleChangeEquippedItemColor(GameSrv *srv, size_t player_id, GameSrv_ChangeEquippedItemColor *msg)
{
    UNREFERENCED_PARAMETER(srv);
    UNREFERENCED_PARAMETER(player_id);
    UNREFERENCED_PARAMETER(msg);
    return ERR_OK;
}

int GameSrv_HandleCharCreationConfirm(GameSrv *srv, size_t player_id, GameSrv_CharCreationConfirm *msg)
{
    int err;

    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        return ERR_OK;
    }

    Appearance app;
    memcpy(&app, msg->config, 4);

    random_get_bytes(&srv->random, &player->char_id, sizeof(player->char_id));

    DbCharacter character = {0};
    character.char_id = player->char_id;
    character.account_id = player->account_id;
    character.last_outpost = MapId_KamadanJewelOfIstanOutpost;
    character.sex = cast_u8(app.sex);
    character.height = cast_u8(app.height);
    character.skin_color = cast_u8(app.skin_color);
    character.hair_color = cast_u8(app.hair_color);
    character.face_style = cast_u8(app.face_style);
    character.hair_style = cast_u8(app.hair_style);
    character.race = cast_u8(app.race);
    character.campaign = player->char_creation_campaign;
    character.level = 1;
    character.is_pvp = player->char_creation_campaign == Campaign_None;
    character.helm_status = HelmStatus_Show;
    character.primary_profession = cast_u8(app.primary_profession);
    // @Cleanup: Add the unlocked profession and unlocked map
    character.unlocked_professions = 1 << app.primary_profession;

    GmBag *bag = &player->bags.equipped_items;
    assert(EquippedItemSlot_Count <= bag->slot_count);
    for (uint8_t idx = EquippedItemSlot_Body; idx <= EquippedItemSlot_Gloves; ++idx) {
        if (bag->items[idx] == 0) {
            continue;
        }

        GmItem *item;
        if ((item = GameSrv_GetItemById(srv, bag->items[idx])) == NULL) {
            log_warn("Invalid item id %u in equiped item", bag->items[idx]);
            continue;
        }

        switch (idx) {
        case EquippedItemSlot_Body:
            character.file_id_body = item->file_id & 0xFFFF;
            character.colors_body = item->dye_colors;
            break;
        case EquippedItemSlot_Legs:
            character.file_id_legs = item->file_id & 0xFFFF;
            character.colors_legs = item->dye_colors;
            break;
        case EquippedItemSlot_Head:
            character.file_id_head = item->file_id & 0xFFFF;
            character.colors_head = item->dye_colors;
            break;
        case EquippedItemSlot_Boots:
            character.file_id_boots = item->file_id & 0xFFFF;
            character.colors_boots = item->dye_colors;
            break;
        case EquippedItemSlot_Gloves:
            character.file_id_gloves = item->file_id & 0xFFFF;
            character.colors_gloves = item->dye_colors;
            break;
        }
    }

    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, player->conn_token)) == NULL) {
        return ERR_OK;
    }

    GameSrvMsg *buffer;
    if ((err = Db_CreateCharacter(
            &srv->database,
            &character,
            msg->n_name, msg->name)
        ) != 0
    ) {
        log_error("Failed to insert character in database");

        buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHAR_CREATION_ERROR);
        GameSrv_CharCreationError *msg2 = &buffer->char_creation_error;
        msg2->error_code = GM_ERROR_CHARACTER_NAME_ALREADY_EXIST;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg2));
        return ERR_OK;
    }

    // save bags
    if ((err = GameSrv_CreatePlayerBags(srv, player)) != 0) {
        log_error("Failed to create the player bags in database");
    }

    buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CHAR_CREATION_SUCCESS);
    GameSrv_CharCreationSuccess *result = &buffer->char_creation_success;
    uuid_enc_le(result->char_id, &player->char_id);
    STATIC_ASSERT(sizeof(result->n_name) <= sizeof(msg->n_name));
    result->n_name = msg->n_name;
    memcpy_u16(result->name, msg->name, msg->n_name);
    result->idk = 0x2211; // what is that?
    CharacterSettings settings = CharacterSettings_FromDbCharacter(&player->character);
    STATIC_ASSERT(sizeof(settings) <= sizeof(result->settings));
    result->n_settings = sizeof(settings);
    memcpy(result->settings, &settings, sizeof(settings));

    return ERR_OK;
}

void GameSrv_RemoveConnection(GameSrv *srv, uintptr_t token)
{
    GameConnection *conn;
    if ((conn = GameSrv_GetConnection(srv, token)) != NULL) {
        GameConnection_Free(conn);
    }
    stbds_hmdel(srv->connections, token);
}

void GameSrv_HandleDisconnect(GameSrv *srv, size_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) != NULL) {
        GameSrv_RemoveConnection(srv, player->conn_token);

        if (player->agent_id != 0) {
            // what more should be deleted?
            GameSrv_RemoveAgentById(srv, player->agent_id);
        }

        GameSrv_RemovePlayer(srv, player_id);
    }
}

int GameSrv_ProcessPlayerMessage(GameSrv *srv, uint16_t player_id, GameCliMsg *msg)
{
    int err = ERR_OK;
    switch (msg->header) {
    case GAME_CMSG_DISCONNECT:
        GameSrv_HandleDisconnect(srv, player_id);
        log_info("GAME_CMSG_DISCONNECT");
        err = ERR_OK;
        break;
    case GAME_CMSG_PING_REPLY:
        break;
    case GAME_CMSG_INSTANCE_LOAD_REQUEST_SPAWN:
        err = GameSrv_HandleInstanceLoadRequestSpawn(srv, player_id);
        break;
    case GAME_CMSG_INSTANCE_LOAD_REQUEST_PLAYERS:
        log_info("GAME_CMSG_INSTANCE_LOAD_REQUEST_PLAYERS");
        err = GameSrv_HandleInstanceLoadRequestPlayers(srv, player_id, &msg->request_players);
        break;
    case GAME_CMSG_INSTANCE_LOAD_REQUEST_ITEMS:
        log_info("GAME_CMSG_INSTANCE_LOAD_REQUEST_ITEMS");
        err = GameSrv_HandleInstanceLoadRequestItems(srv, player_id, &msg->request_items);
        break;
    case GAME_CMSG_CHAR_CREATION_REQUEST_PLAYER:
        err = GameSrv_HandleCharCreationRequestPlayer(srv, player_id);
        break;
    case GAME_CMSG_CHAR_CREATION_REQUEST_ARMORS:
        err = GameSrv_HandleCharCreationRequestArmors(srv, player_id);
        break;
    case GAME_CMSG_CHAR_CREATION_CHANGE_PROF:
        err = GameSrv_HandleCharCreationChangeProf(srv, player_id, &msg->char_creation_change_prof);
        break;
    case GAME_CMSG_CHANGE_EQUIPPED_ITEM_COLOR:
        err = GameSrv_HandleChangeEquippedItemColor(srv, player_id, &msg->change_equipped_item_color);
        break;
    case GAME_CMSG_CHAR_CREATION_CONFIRM:
        err = GameSrv_HandleCharCreationConfirm(srv, player_id, &msg->char_creation_confirm);
        break;
    case GAME_CMSG_SEND_CHAT_MESSAGE:
        err = GameSrv_HandleChatMessage(srv, player_id, &msg->chat_message);
        break;
    case GAME_CMSG_MOVE_TO_COORD:
        err = GameSrv_HandleMoveToCoord(srv, player_id, &msg->move_to_coord);
        break;
    case GAME_CMSG_CANCEL_MOVEMENT:
        err = GameSrv_HandleCancelMovement(srv, player_id);
        break;
    case GAME_CMSG_LAST_POS_BEFORE_MOVE_CANCELED:
        err = GameSrv_HandleLastPosOnMoveCanceled(srv, player_id, &msg->last_pos_before_move_canceled);
        break;
    default:
        log_warn(
            "Unhandled GameSrv packet with header %" PRIu16 " (0x%" PRIX16 ")",
            msg->header & ~GAME_CMSG_MASK,
            msg->header & ~GAME_CMSG_MASK
        );
        err = ERR_BAD_USER_DATA;
    }
    return err;
}

void GameSrv_Update(GameSrv *srv)
{
    int err;

    uint64_t current_time = sys_get_monotonic_time_ms();
    srv->current_frame_time = current_time;
    srv->current_instance_time = (uint32_t)(srv->creation_instance_time - current_time);

    GameSrv_ProcessInternalMessages(srv);
    GameSrv_Poll(srv);

    for (size_t idx = 0; idx < srv->events.len; ++idx) {
        Event event = array_at(&srv->events, idx);
        GameSrv_ProcessEvent(srv, event);
        array_add(&srv->connections_with_event, event.token);
    }

    GamePlayerMsgArray msgs = srv->player_messages;
    for (size_t idx = 0; idx < msgs.len; ++idx) {
        GamePlayerMsg *msg = &msgs.ptr[idx];
        GameSrv_ProcessPlayerMessage(srv, msg->player_id, &msg->msg);
    }
    array_clear(&srv->player_messages);

    size_t n_connections = stbds_hmlen(srv->connections);
    if (GAME_SRV_TIME_BETWEEN_PING_MS <= (current_time - srv->last_ping_request)) {
        srv->last_ping_request = current_time;
        for (size_t idx = 0; idx < n_connections; ++idx) {
            GameConnection *conn = &srv->connections[idx].value;
            GameSrv_SendPing(srv, conn);
        }
    }

    if (GAME_SRV_MAX_TIME_BETWEEN_WORLD_TICK_MS <= (current_time - srv->last_world_tick)) {
        GameSrv_WorldTick(srv);
    }

    for (size_t idx = 0; idx < n_connections; ++idx) {
        GameConnection *conn = &srv->connections[idx].value;
        if (array_empty(&conn->outgoing)) {
            continue;
        }

        if ((err = GameConnection_FlushOutgoingBuffer(conn)) != 0) {
            array_add(&srv->connections_to_remove, conn->token);
        }
    }

    for (size_t idx = 0; idx < srv->connections_to_remove.len; ++idx) {
        uintptr_t token = array_at(&srv->connections_to_remove, idx);
        GameSrv_RemoveConnection(srv, token);
    }
    array_clear(&srv->connections_to_remove);

    for (size_t idx = 0; idx < srv->connections_with_event.len; ++idx) {
        GameConnection *conn;
        uintptr_t token = array_at(&srv->connections_with_event, idx);
        if ((conn = GameSrv_GetConnection(srv, token)) == NULL) {
            continue;
        }

        int flags = IOCPF_READ;
        if (!conn->writable) {
            flags |= IOCPF_WRITE;
        }

        if ((err = iocp_reregister(&srv->iocp, &conn->source, flags)) != 0) {
            log_error("Failed to re-register game connection %04" PRIXPTR ", err: %d", token, err);
        }
    }
    array_clear(&srv->connections_with_event);
}

void GameSrv_ThreadEntry(void *param)
{
    GameSrv *srv = (GameSrv *)param;
    while (!srv->quit_signaled) {
        GameSrv_Update(srv);
    }
}

int GameSrv_Start(GameSrv *srv)
{
    int err;
    if ((err = sys_thread_create(&srv->thread, GameSrv_ThreadEntry, srv)) != 0) {
        return err;
    }
    return ERR_OK;
}

void GameSrv_SendAdminMsg(GameSrv *srv, AdminMsg *msg)
{
    sys_mutex_lock(&srv->mtx);
    AdminMsg *dst = array_push(&srv->admin_messages, 1);
    *dst = *msg;
    sys_mutex_unlock(&srv->mtx);
}
