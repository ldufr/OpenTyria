#pragma once

GmItem* GameSrv_AllocateItem(GameSrv *srv)
{
    uint32_t item_id = GmIdAllocate(&srv->items.base, sizeof(srv->items.buffer[0]));
    GmItem *item = &srv->items.buffer[item_id];
    memset(item, 0, sizeof(*item));
    item->item_id = item_id;
    return item;
}

void GameSrv_FreeItemId(GameSrv *srv, uint32_t item_id)
{
    assert(item_id != 0);

    if (srv->items.size <= item_id) {
        log_warn("Can't free item id %" PRIu32 ", when the maximum is %zu", item_id, srv->items.size);
        return;
    }

    srv->items.buffer[item_id].item_id = 0;
    GmIdFree(&srv->items.base, item_id, sizeof(srv->items.buffer[0]));
}

GmItem* GameSrv_GetItemById(GameSrv *srv, uint32_t item_id)
{
    if (srv->items.size <= item_id) {
        return NULL;
    }
    GmItem *result = &srv->items.buffer[item_id];
    assert(!result->IdHeader.freed && result->item_id != 0);
    return result;
}

void GameSrv_SendItemStreamCreate(GameSrv *srv, GameConnection *conn)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ITEM_STREAM_CREATE);
    GameSrv_ItemStreamCreate *msg = &buffer->item_stream_create;
    msg->stream_id = 1;
    msg->is_hero = 0;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendCreateNamedItem(GameSrv *srv, GameConnection *conn, GmItem *item)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CREATE_NAMED_ITEM);
    GameSrv_CreateNamedItem *msg = &buffer->create_named_item;
    msg->item_id = item->item_id;
    msg->file_id = item->file_id;
    msg->item_type = item->item_type;
    msg->dye_tint = item->dye_tint;
    msg->dye_colors = item->dye_colors;
    msg->materials = item->materials;
    msg->unk1 = item->unk1;
    msg->flags = item->flags;
    msg->value = item->value;
    msg->model_id = item->model_id;
    msg->quantity = item->quantity;
    msg->n_name = item->name.size;
    STATIC_ASSERT(ARRAY_SIZE(msg->name) <= ARRAY_SIZE(item->name.data));
    memcpy_u16(msg->name, item->name.data, item->name.size);
    msg->n_modifiers = item->modifiers.size;
    STATIC_ASSERT(ARRAY_SIZE(msg->modifiers) <= ARRAY_SIZE(item->modifiers.data));
    memcpy_u32(msg->modifiers, item->modifiers.data, item->modifiers.size);
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendCreateUnamedItem(GameSrv *srv, GameConnection *conn, GmItem *item)
{
    GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_CREATE_UNNAMED_ITEM);
    GameSrv_CreateUnamedItem *msg = &buffer->create_unnamed_item;
    msg->item_id = item->item_id;
    msg->file_id = item->file_id;
    msg->item_type = item->item_type;
    msg->dye_tint = item->dye_tint;
    msg->dye_colors = item->dye_colors;
    msg->materials = item->materials;
    msg->unk1 = item->unk1;
    msg->flags = item->flags;
    msg->value = item->value;
    GameConnection_SendMessage(conn, buffer, sizeof(*msg));
}

void GameSrv_SendItemById(GameSrv *srv, GameConnection *conn, uint32_t item_id)
{
    GmItem *item;
    if ((item = GameSrv_GetItemById(srv, item_id)) == NULL) {
        log_error("Item %u doesn't exist", item_id);
        return;
    }

    GameSrv_SendCreateNamedItem(srv, conn, item);
    // @TODO: send customization and other stuff
}

void GameSrv_SendItemsInBag(GameSrv *srv, GameConnection *conn, GmBag *bag)
{
    for (uint8_t idx = 0; idx < bag->slot_count; ++idx) {
        uint32_t item_id = bag->items[idx];
        if (item_id == 0) {
            continue;
        }

        GmItem *item;
        if ((item = GameSrv_GetItemById(srv, item_id)) == NULL) {
            log_error("Item %u doesn't exist", item_id);
            continue;
        }

        GameSrv_SendCreateUnamedItem(srv, conn, item);
    }
}

void GameSrv_BroadcastPlayerEquippedItems(GameSrv *srv, GmPlayer *player)
{
    size_t count = 0;
    GmItem *equipped_items[EquippedItemSlot_Count];

    GmBag *bag = &player->bags.equipped_items;
    for (uint8_t idx = 0; idx < bag->slot_count; ++idx) {
        uint32_t item_id = bag->items[idx];
        if (item_id == 0) {
            continue;
        }

        GmItem *item;
        if ((item = GameSrv_GetItemById(srv, item_id)) == NULL) {
            log_error("Item %u doesn't exist", item_id);
        }

        equipped_items[count++] = item;
    }

    size_t n_connections = stbds_hmlen(srv->connections);
    for (size_t idx = 0; idx < n_connections; ++idx) {
        GameConnection *conn = &srv->connections[idx].value;
        if (conn->token == player->conn_token) {
            continue;
        }

        for (size_t item_idx = 0; item_idx < count; ++item_idx) {
            GameSrv_SendCreateUnamedItem(srv, conn, equipped_items[item_idx]);
        }
    }
}
