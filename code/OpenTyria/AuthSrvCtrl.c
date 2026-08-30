#pragma once

GameSrvMetadata* AuthSrvCtrl_AddGameSrvMetadata(AuthSrv *srv, uint32_t server_id)
{
    GameSrvMetadata metadata = {0};
    metadata.server_id = server_id;
    stbds_hmputs(srv->games, metadata);
    return &srv->games[stbds_temp(srv->games - 1)];
}

GameSrvMetadata* AuthSrvCtrl_GetGameSrvMetadata(AuthSrv *srv, uint32_t server_id)
{
    ptrdiff_t idx;
    if ((idx = stbds_hmgeti(srv->games, server_id)) == -1) {
        return NULL;
    }
    return &srv->games[(size_t)idx];
}

int AuthSrvCtrl_FindCompatibleGameSrv(AuthSrv *srv, GmDistrict district, size_t *result)
{
    size_t n_games = stbds_hmlen(srv->games);
    for (size_t idx = 0; idx < n_games; ++idx) {
        GameSrvMetadata *metadata = &srv->games[idx];
        if (GmDistrict_IsCompatible(metadata->district, district)) {
            *result = idx;
            return ERR_OK;
        }
    }

    return ERR_UNSUCCESSFUL;
}

int AuthSrvCtrl_CreateServer(AuthSrv *srv, GmDistrict district, size_t *idx)
{
    uint32_t server_id;
    random_get_bytes(&srv->random, &server_id, sizeof(server_id));

    GameSrvMetadata *metadata = AuthSrvCtrl_AddGameSrvMetadata(srv, server_id);
    metadata->district = district;
    metadata->district.district_number = 1;

    GameSrv *gm;
    if ((gm = calloc(1, sizeof(*gm))) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    GameSrvSetupParams params = {0};
    params.server_id = server_id;
    params.district = metadata->district;
    params.ctrl_srv_addr = srv->internal_address;

    int err;
    if ((err = GameSrv_Setup(gm, params)) != 0) {
        free(gm);
        return err;
    }

    if ((err = GameSrv_Start(gm)) != 0) {
        log_error("Failed to start a game server");
        GameSrv_Free(gm);
        free(gm);
        return err;
    }

    *idx = ARRAY_INDEX(srv->games, metadata);
    return 0;
}

int AuthSrvCtrl_HandleServerReady(AuthSrv *srv, CtrlConnection *conn, CtrlMsg_ServerReady *msg)
{
    conn->server_id = msg->server_id;

    GameSrvMetadata *metadata = AuthSrvCtrl_GetGameSrvMetadata(srv, conn->server_id);
    metadata->ctrl_conn = conn->token;
    metadata->ready = true;

    for (size_t idx = 0; idx < metadata->pending_transfers.len; ++idx) {
        AuthTransfer transfer = metadata->pending_transfers.ptr[idx];
        AuthSrv_CompleteGameTransfer(
            srv,
            transfer,
            conn->server_id,
            metadata->district.map_id);
    }

    array_free(&metadata->pending_transfers);
    return 0;
}
