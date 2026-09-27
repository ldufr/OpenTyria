#pragma once

bool IsValidBag(GmBag *bag)
{
    return bag->bag_id != 0;
}

void GmBag_InitBackpack(GmBag *bag, uint16_t bag_id)
{
    assert(!IsValidBag(bag));
    bag->bag_id = bag_id;
    bag->bag_model_id = BagModelId_Backpack;
    bag->bag_type = BagType_Bag;
    bag->slot_count = 20;
}

void GmBag_InitUnclaimedItems(GmBag *bag, uint16_t bag_id)
{
    assert(!IsValidBag(bag));
    bag->bag_id = bag_id;
    bag->bag_model_id = BagModelId_UnclaimedItems;
    bag->bag_type = BagType_NotCollected;
    bag->slot_count = 12;
}

void GmBag_InitEquippedItems(GmBag *bag, uint16_t bag_id)
{
    assert(!IsValidBag(bag));
    bag->bag_id = bag_id;
    bag->bag_model_id = BagModelId_EquippedItems;
    bag->bag_type = BagType_Equipped;
    bag->slot_count = 9;
}

void GmBag_InitMaterialStorage(GmBag *bag, uint16_t bag_id)
{
    assert(!IsValidBag(bag));
    bag->bag_id = bag_id;
    bag->bag_model_id = BagModelId_MaterialStorage;
    bag->bag_type = BagType_MatStorage;
    bag->slot_count = 41;
}

void GmBag_InitStorage(GmBagArray *bags, BagModelId model_id, uint16_t bag_id)
{
    assert(!IsValidBag(&bags->bags[model_id]));
    assert(BagModelId_Storage1 <= model_id && model_id <= BagModelId_StorageAnniversary);
    bags->bags[model_id].bag_id = bag_id;
    bags->bags[model_id].bag_model_id = model_id;
    bags->bags[model_id].bag_type = BagType_Storage;
    bags->bags[model_id].slot_count = 25;
}

void GmBag_SetItem(GmBag *bag, size_t slot, uint32_t item_id)
{
    assert(slot < ARRAY_SIZE(bag->items));
    assert(bag->items[slot] == 0);
    bag->items[slot] = item_id;
}

bool GmBag_TryAddToBag(GmBag *bag, uint32_t item_id)
{
    for (size_t idx = 0; idx < bag->slot_count; ++idx) {
        if (bag->items[idx] != 0) {
            bag->items[idx] = item_id;
            return true;
        }
    }
    return false;
}

bool GmBag_IsVolatile(BagModelId model_id)
{
    return model_id == BagModelId_UnclaimedItems;
}

bool GmBag_FindItem(GmBagArray *bags, uint32_t item_id, GmBag **out_bag, uint8_t *out_slot)
{
    for (BagModelId bag_id = BagModelId_Backpack; bag_id <= BagModelId_Bag2; ++bag_id) {
        GmBag *bag = &bags->bags[bag_id];
        if (!IsValidBag(bag))
            continue;
        for (uint8_t slot = 0; slot < bag->slot_count; ++slot) {
            if (bag->items[slot] == item_id) {
                *out_bag = bag;
                *out_slot = slot;
                return true;
            }
        }
    }
    return false;
}

void GameSrv_FreeBagItems(GameSrv *srv, GmPlayer *player, GmBag *bag)
{
    GameConnection *conn = GameSrv_GetConnection(srv, player->conn_token);

    for (size_t idx = 0; idx < bag->slot_count; ++idx) {
        uint32_t item_id = bag->items[idx];
        if (item_id == 0) {
            continue;
        }

        if (conn != NULL) {
            GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ITEM_REMOVE);
            GameSrv_ItemRemove *msg = &buffer->item_remove;
            msg->stream_id = 1;
            msg->item_id = item_id;

            GameConnection_SendMessage(conn, buffer, sizeof(*msg));
        }

        GameSrv_FreeItemId(srv, item_id);
    }
    memset(bag->items, 0, sizeof(bag->items));
}

void GameSrv_CreateDefaultBags(GameSrv *srv, GmPlayer *player)
{
    if (!IsValidBag(&player->bags.backpack)) {
        GmBag_InitBackpack(&player->bags.backpack, ++srv->next_bag_id);
    }

    if (!IsValidBag(&player->bags.material_storage)) {
        GmBag_InitMaterialStorage(&player->bags.material_storage, ++srv->next_bag_id);
    }

    if (!IsValidBag(&player->bags.storage1)) {
        GmBag_InitStorage(&player->bags, BagModelId_Storage1, ++srv->next_bag_id);
    }

    if (!IsValidBag(&player->bags.storage2)) {
        GmBag_InitStorage(&player->bags, BagModelId_Storage2, ++srv->next_bag_id);
    }

    if (!IsValidBag(&player->bags.equipped_items)) {
        GmBag_InitEquippedItems(&player->bags.equipped_items, ++srv->next_bag_id);
    }

    GmBag_InitUnclaimedItems(&player->bags.unclaimed_items, ++srv->next_bag_id);

    // Testing code, add 100 cupcakes in the first free spot in the inventory.
    for (BagModelId bag_id = BagModelId_Backpack; bag_id <= BagModelId_Bag2; ++bag_id) {
        GmBag *bag = &player->bags.bags[bag_id];
        if (!IsValidBag(bag))
            continue;
        for (uint8_t slot_id = 0; slot_id < bag->slot_count; ++slot_id) {
            if (bag->items[slot_id] == 0) {
                GmItem *item = GameSrv_AllocateItem(srv);
                item->file_id = 0x47C34; // cupcake
                item->item_type = ItemType_Consumable;
                item->flags = 0x21080201;
                item->model_id = 22269;
                item->quantity = 100;

                // uint16_t name[] = L"\x108\x107Cupcake\x1\0";
                // memcpy(item->name.data, name, sizeof(name));
                // item->name.size = ARRAY_SIZE(name);

                bag->items[slot_id] = item->item_id;
                goto quit_loop;
            }
        }
    }

quit_loop:;
}

void GameSrv_SendBagItems(GameSrv *srv, GameConnection *conn, GmBag *bag)
{
    for (size_t idx = 0; idx < bag->slot_count; ++idx) {
        uint32_t item_id = bag->items[idx];
        if (item_id == 0) {
            continue;
        }

        GmItem *item;
        if ((item = GameSrv_GetItemById(srv, item_id)) == NULL) {
            log_warn("Player has non-existing item %" PRIu32 " in his inventory", item_id);
            continue;
        }

        GameSrv_SendCreateNamedItem(srv, conn, item);

        if (item->profession != Profession_None) {
            GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ITEM_SET_PROFESSION);
            GameSrv_ItemSetProfession *msg = &buffer->item_set_profession;
            msg->item_id = item->item_id;
            msg->profession = item->profession;
            GameConnection_SendMessage(conn, buffer, sizeof(*msg));
        }

        {
            GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_ITEM_MOVED_TO_LOCATION);
            GameSrv_ItemMoveToLocation *msg = &buffer->item_move_to_location;
            msg->stream_id = 1;
            msg->item_id = item->item_id;
            msg->bag_id = bag->bag_id;
            msg->slot = (uint8_t)idx;
            GameConnection_SendMessage(conn, buffer, sizeof(*msg));
        }
    }
}

void GameSrv_SendInventory(GameSrv *srv, GameConnection *conn, uint32_t player_id)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        return;
    }

    for (size_t idx = 0; idx < ARRAY_SIZE(player->bags.bags); ++idx) {
        GmBag bag = player->bags.bags[idx];
        if (bag.bag_id == 0) {
            continue;
        }

        if (bag.bag_item_id != 0) {
            GameSrv_SendItemById(srv, conn, bag.bag_item_id);
        }

        GameSrvMsg *buffer = GameSrv_BuildMsg(srv, GAME_SMSG_INVENTORY_CREATE_BAG);
        GameSrv_InventoryCreateBag *msg = &buffer->inventory_create_bag;
        msg->stream_id = 1;
        msg->bag_type = bag.bag_type;
        msg->bag_model_id = bag.bag_model_id;
        msg->bag_id = bag.bag_id;
        msg->slot_count = bag.slot_count;
        msg->assoc_item_id = bag.bag_item_id;
        GameConnection_SendMessage(conn, buffer, sizeof(*msg));

        GameSrv_SendBagItems(srv, conn, &bag);
    }
}

int GameSrv_HandleDropItem(GameSrv *srv, uint16_t player_id, GameSrv_DropItem *msg)
{
    GmPlayer *player;
    if ((player = GameSrv_GetPlayer(srv, player_id)) == NULL) {
        log_error("Unknow player with player id %u", player_id);
        return ERR_SERVER_ERROR;
    }

    GmAgent *player_agent;
    if ((player_agent = GameSrv_GetAgent(srv, player->agent_id)) == NULL) {
        log_error("Unknow agent with player id %u", player_id);
        return ERR_SERVER_ERROR;
    }

    if (msg->item_id == 0) {
        log_warn("Can't drop item with id 0");
        return ERR_OK;
    }

    GmBag *bag;
    uint8_t bag_slot;
    if (!GmBag_FindItem(&player->bags, msg->item_id, &bag, &bag_slot)) {
        log_warn("Can't find item with id %lu in player inventory", msg->item_id);
        return ERR_OK;
    }

    // Find item
    GmItem *item;
    if ((item = GameSrv_GetItemById(srv, msg->item_id)) == NULL) {
        log_warn("Can't find item with id %lu", msg->item_id);
        return ERR_OK;
    }

    if (item->quantity < (uint32_t) msg->count) {
        log_warn("Can't drop item %lu, drop count: %u, capacity: %" PRIu32, msg->item_id, msg->count, item->quantity);
        return ERR_OK;
    }

    if (msg->count < item->quantity) {
        // GmItem *new_item = GameSrv_AllocateItem(srv);
        log_warn("TODO: we must still implement this");
        return ERR_OK;
    }

    GmAgent *item_agent = GameSrv_CreateAgent(srv);
    item_agent->model_id = item->item_id;
    item_agent->agent_type = AgentType_Item;
    item_agent->position.v2 = Vec2fAdd(player_agent->position.v2, (Vec2f) { 30.f, 30.f });
    item_agent->position.plane = player_agent->position.plane;
    item_agent->direction.x = 1.f;
    GameSrv_BroadcastCreateAgent(srv, item_agent);

    return ERR_OK;
}
