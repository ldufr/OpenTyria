#pragma once

#define GM_BAG_MAX_SLOT 41

typedef enum BagType {
    BagType_Bag          = 1,
    BagType_Equipped     = 2,
    BagType_NotCollected = 3,
    BagType_Storage      = 4,
    BagType_MatStorage   = 5,
} BagType;

int BagType_FromInt(int value, BagType *result)
{
    switch (value) {
    case BagType_Bag:
    case BagType_Equipped:
    case BagType_NotCollected:
    case BagType_Storage:
    case BagType_MatStorage:
        *result = (BagType)value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

typedef enum BagModelId {
    BagModelId_Backpack            = 0,
    BagModelId_BeltPouch           = 1,
    BagModelId_Bag1                = 2,
    BagModelId_Bag2                = 3,
    BagModelId_EquipmentPack       = 4,
    BagModelId_MaterialStorage     = 5,
    BagModelId_UnclaimedItems      = 6,
    BagModelId_Storage1            = 7,
    BagModelId_Storage2            = 8,
    BagModelId_Storage3            = 9,
    BagModelId_Storage4            = 10,
    BagModelId_Storage5            = 11,
    BagModelId_Storage6            = 12,
    BagModelId_Storage7            = 13,
    BagModelId_Storage8            = 14,
    BagModelId_Storage9            = 15,
    BagModelId_Storage10           = 16,
    BagModelId_Storage11           = 17,
    BagModelId_Storage12           = 18,
    BagModelId_Storage13           = 19,
    BagModelId_StorageAnniversary  = 20,
    BagModelId_EquippedItems       = 21,

    BagModelId_Count,
    BagModelId_Invalid             = -1,
} BagModelId;

int BagModelId_FromInt(int value, BagModelId *result)
{
    switch (value) {
    case BagModelId_Backpack:
    case BagModelId_BeltPouch:
    case BagModelId_Bag1:
    case BagModelId_Bag2:
    case BagModelId_EquipmentPack:
    case BagModelId_MaterialStorage:
    case BagModelId_UnclaimedItems:
    case BagModelId_Storage1:
    case BagModelId_Storage2:
    case BagModelId_Storage3:
    case BagModelId_Storage4:
    case BagModelId_Storage5:
    case BagModelId_Storage6:
    case BagModelId_Storage7:
    case BagModelId_Storage8:
    case BagModelId_Storage9:
    case BagModelId_Storage10:
    case BagModelId_Storage11:
    case BagModelId_Storage12:
    case BagModelId_Storage13:
    case BagModelId_StorageAnniversary:
    case BagModelId_EquippedItems:
        *result = (BagModelId)value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

const char* BagModelId_ToString(BagModelId bag_model_id)
{
    switch (bag_model_id) {
    case BagModelId_Backpack: return "BagModelId_Backpack";
    case BagModelId_BeltPouch: return "BagModelId_BeltPouch";
    case BagModelId_Bag1: return "BagModelId_Bag1";
    case BagModelId_Bag2: return "BagModelId_Bag2";
    case BagModelId_EquipmentPack: return "BagModelId_EquipmentPack";
    case BagModelId_MaterialStorage: return "BagModelId_MaterialStorage";
    case BagModelId_UnclaimedItems: return "BagModelId_UnclaimedItems";
    case BagModelId_Storage1: return "BagModelId_Storage1";
    case BagModelId_Storage2: return "BagModelId_Storage2";
    case BagModelId_Storage3: return "BagModelId_Storage3";
    case BagModelId_Storage4: return "BagModelId_Storage4";
    case BagModelId_Storage5: return "BagModelId_Storage5";
    case BagModelId_Storage6: return "BagModelId_Storage6";
    case BagModelId_Storage7: return "BagModelId_Storage7";
    case BagModelId_Storage8: return "BagModelId_Storage8";
    case BagModelId_Storage9: return "BagModelId_Storage9";
    case BagModelId_Storage10: return "BagModelId_Storage10";
    case BagModelId_Storage11: return "BagModelId_Storage11";
    case BagModelId_Storage12: return "BagModelId_Storage12";
    case BagModelId_Storage13: return "BagModelId_Storage13";
    case BagModelId_StorageAnniversary: return "BagModelId_StorageAnniversary";
    case BagModelId_EquippedItems: return "BagModelId_EquippedItems";
    default: abort();
    }
}

typedef enum EquippedItemSlot {
    EquippedItemSlot_Weapon      = 0,
    EquippedItemSlot_OffHand     = 1,
    EquippedItemSlot_Body        = 2,
    EquippedItemSlot_Legs        = 3,
    EquippedItemSlot_Head        = 4,
    EquippedItemSlot_Boots       = 5,
    EquippedItemSlot_Gloves      = 6,
    EquippedItemSlot_CostumeBody = 7,
    EquippedItemSlot_CostumeHead = 8,

    EquippedItemSlot_Count
} EquippedItemSlot;

typedef struct GmBag {
    uint16_t       bag_id;
    BagModelId     bag_model_id;
    BagType        bag_type;
    uint32_t       bag_item_id;
    uint8_t        slot_count;
    uint32_t       items[GM_BAG_MAX_SLOT];
} GmBag;

typedef union GmBagArray {
    struct {
        GmBag backpack;
        GmBag belt_pouch;
        GmBag bag1;
        GmBag bag2;
        GmBag equipment_pack;
        GmBag material_storage;
        GmBag unclaimed_items;
        GmBag storage1;
        GmBag storage2;
        GmBag storage3;
        GmBag storage4;
        GmBag storage5;
        GmBag storage6;
        GmBag storage7;
        GmBag storage8;
        GmBag storage9;
        GmBag storage10;
        GmBag storage11;
        GmBag storage12;
        GmBag storage13;
        GmBag storage_anniversary;
        GmBag equipped_items;
    };
    GmBag bags[BagModelId_Count];
} GmBagArray;

void GmBag_InitBackpack(GmBag *bag, uint16_t bag_id);
void GmBag_InitUnclaimedItems(GmBag *bag, uint16_t bag_id);
void GmBag_InitEquippedItems(GmBag *bag, uint16_t bag_id);
void GmBag_InitMaterialStorage(GmBag *bag, uint16_t bag_id);
void GmBag_InitStorage(GmBagArray *bags, BagModelId model_id, uint16_t bag_id);
void GmBag_SetItem(GmBag *bag, size_t slot, uint32_t item_id);
bool GmBag_TryAddToBag(GmBag *bag, uint32_t item_id);
bool GmBag_IsVolatile(BagModelId model_id);

void GameSrv_FreeBagItems(GameSrv *srv, GmPlayer *player, GmBag *bag);
void GameSrv_CreateDefaultBags(GameSrv *srv, GmPlayer *player);
void GameSrv_SendBagItems(GameSrv *srv, GameConnection *conn, GmBag *bag);
void GameSrv_SendInventory(GameSrv *srv, GameConnection *conn, uint32_t player_id);
