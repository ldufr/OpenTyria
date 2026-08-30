#pragma once

typedef enum ItemType {
    ItemType_Salvage       = 0,
    ItemType_Leadhand      = 1,
    ItemType_Axe           = 2,
    ItemType_Bag           = 3,
    ItemType_Boots         = 4,
    ItemType_Bow           = 5,
    ItemType_Bundle        = 6,
    ItemType_Body          = 7,
    ItemType_Rune          = 8,
    ItemType_Consumable    = 9,
    ItemType_Dye           = 10,
    ItemType_Material      = 11,
    ItemType_Focus         = 12,
    ItemType_Gloves        = 13,
    ItemType_Sigil         = 14,
    ItemType_Hammer        = 15,
    ItemType_Head          = 16,
    ItemType_SalvageItem   = 17,
    ItemType_Key           = 18,
    ItemType_Legs          = 19,
    ItemType_Coins         = 20,
    ItemType_QuestItem     = 21,
    ItemType_Wand          = 22,
    ItemType_Shield        = 24,
    ItemType_Staff         = 26,
    ItemType_Sword         = 27,
    ItemType_Kit           = 29,
    ItemType_Trophy        = 30,
    ItemType_Scroll        = 31,
    ItemType_Daggers       = 32,
    ItemType_Present       = 33,
    ItemType_Minipet       = 34,
    ItemType_Scythe        = 35,
    ItemType_Spear         = 36,
    ItemType_Handbook      = 43,
    ItemType_CostumeBody   = 44,
    ItemType_CostumeHead   = 45,
} ItemType;

const char* ItemType_ToString(ItemType item_type)
{
    switch (item_type) {
    case ItemType_Salvage: return "ItemType_Salvage";
    case ItemType_Leadhand: return "ItemType_Leadhand";
    case ItemType_Axe: return "ItemType_Axe";
    case ItemType_Bag: return "ItemType_Bag";
    case ItemType_Boots: return "ItemType_Boots";
    case ItemType_Bow: return "ItemType_Bow";
    case ItemType_Bundle: return "ItemType_Bundle";
    case ItemType_Body: return "ItemType_Body";
    case ItemType_Rune: return "ItemType_Rune";
    case ItemType_Consumable: return "ItemType_Consumable";
    case ItemType_Dye: return "ItemType_Dye";
    case ItemType_Material: return "ItemType_Material";
    case ItemType_Focus: return "ItemType_Focus";
    case ItemType_Gloves: return "ItemType_Gloves";
    case ItemType_Sigil: return "ItemType_Sigil";
    case ItemType_Hammer: return "ItemType_Hammer";
    case ItemType_Head: return "ItemType_Head";
    case ItemType_SalvageItem: return "ItemType_SalvageItem";
    case ItemType_Key: return "ItemType_Key";
    case ItemType_Legs: return "ItemType_Legs";
    case ItemType_Coins: return "ItemType_Coins";
    case ItemType_QuestItem: return "ItemType_QuestItem";
    case ItemType_Wand: return "ItemType_Wand";
    case ItemType_Shield: return "ItemType_Shield";
    case ItemType_Staff: return "ItemType_Staff";
    case ItemType_Sword: return "ItemType_Sword";
    case ItemType_Kit: return "ItemType_Kit";
    case ItemType_Trophy: return "ItemType_Trophy";
    case ItemType_Scroll: return "ItemType_Scroll";
    case ItemType_Daggers: return "ItemType_Daggers";
    case ItemType_Present: return "ItemType_Present";
    case ItemType_Minipet: return "ItemType_Minipet";
    case ItemType_Scythe: return "ItemType_Scythe";
    case ItemType_Spear: return "ItemType_Spear";
    case ItemType_Handbook: return "ItemType_Handbook";
    case ItemType_CostumeBody: return "ItemType_CostumeBody";
    case ItemType_CostumeHead: return "ItemType_CostumeHead";
    default: abort();
    }
}

int ItemType_FromInt(int value, ItemType *result)
{
    switch (value) {
    case ItemType_Salvage:
    case ItemType_Leadhand:
    case ItemType_Axe:
    case ItemType_Bag:
    case ItemType_Boots:
    case ItemType_Bow:
    case ItemType_Bundle:
    case ItemType_Body:
    case ItemType_Rune:
    case ItemType_Consumable:
    case ItemType_Dye:
    case ItemType_Material:
    case ItemType_Focus:
    case ItemType_Gloves:
    case ItemType_Sigil:
    case ItemType_Hammer:
    case ItemType_Head:
    case ItemType_SalvageItem:
    case ItemType_Key:
    case ItemType_Legs:
    case ItemType_Coins:
    case ItemType_QuestItem:
    case ItemType_Wand:
    case ItemType_Shield:
    case ItemType_Staff:
    case ItemType_Sword:
    case ItemType_Kit:
    case ItemType_Trophy:
    case ItemType_Scroll:
    case ItemType_Daggers:
    case ItemType_Present:
    case ItemType_Minipet:
    case ItemType_Scythe:
    case ItemType_Spear:
    case ItemType_Handbook:
    case ItemType_CostumeBody:
    case ItemType_CostumeHead:
        *result = (ItemType)value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

typedef struct GmItem {
    GmIdHeader   IdHeader;
    uint32_t     item_id;
    uint32_t     file_id;
    ItemType     item_type;
    uint8_t      dye_tint;
    DyeColor     dye_colors;
    uint16_t     materials;
    uint8_t      unk1;
    uint32_t     flags;
    uint32_t     value;
    uint32_t     model_id;
    uint32_t     quantity;
    Profession   profession;
    struct
    {
        uint32_t size;
        uint16_t data[64];
    } name;
    struct
    {
        uint32_t size;
        uint32_t data[64];
    } modifiers;
} GmItem;
typedef GmIdTable(GmItem) GmItemArray;
typedef slice(GmItem) GmItemSlice;

GmItem* GameSrv_AllocateItem(GameSrv *srv);
void    GameSrv_FreeItemId(GameSrv *srv, uint32_t item_id);
GmItem* GameSrv_GetItemById(GameSrv *srv, uint32_t item_id);

void GameSrv_SendItemStreamCreate(GameSrv *srv, GameConnection *conn);
void GameSrv_SendCreateNamedItem(GameSrv *srv, GameConnection *conn, GmItem *item);
void GameSrv_SendCreateUnamedItem(GameSrv *srv, GameConnection *conn, GmItem *item);
void GameSrv_SendItemById(GameSrv *srv, GameConnection *conn, uint32_t item_id);
void GameSrv_SendItemsInBag(GameSrv *srv, GameConnection *conn, GmBag *bag);
void GameSrv_BroadcastPlayerEquippedItems(GameSrv *srv, GmPlayer *player);
