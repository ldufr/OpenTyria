#pragma once

#pragma pack(push, 1)
typedef struct GameSrv_PingReply {
    uint16_t header;
    uint32_t ping;
} GameSrv_PingReply;

typedef struct GameSrv_InstanceHead {
    uint16_t header;
    uint8_t  pve_unlocked_flags;
    uint8_t  pvp_unlocked_flags;
    uint8_t  b3;
    uint8_t  b4;
} GameSrv_InstanceHead;

typedef struct GameSrv_InstancePlayerName {
    uint16_t header;
    uint32_t n_name;
    uint16_t name[20];
} GameSrv_InstancePlayerName;

typedef struct GameSrv_InstanceInfo {
    uint16_t header;
    uint32_t agent;
    uint16_t map_id;
    uint8_t  is_explorable;
    uint32_t district;
    uint8_t  language;
    uint8_t  is_observer;
} GameSrv_InstanceInfo;

typedef struct GameSrv_InstanceLoaded {
    uint16_t header;
    uint32_t player_team_token;
} GameSrv_InstanceLoaded;

typedef struct GameSrv_ReadyForMapSpawn {
    uint16_t header;
    uint32_t unk;
} GameSrv_ReadyForMapSpawn;

typedef struct GameSrv_ItemStreamCreate {
    uint16_t header;
    uint16_t stream_id;
    uint8_t  is_hero;
} GameSrv_ItemStreamCreate;

typedef struct GameSrv_UpdateGold {
    uint16_t header;
    uint16_t stream;
    uint32_t gold;
} GameSrv_UpdateGold;

typedef struct GameSrv_WeaponSet {
    uint16_t header;
    uint16_t stream_id;
    uint8_t  slot;
    uint32_t leadhand;
    uint32_t offhand;
} GameSrv_WeaponSet;

typedef struct GameSrv_LoadSpawnPoint {
    uint16_t header;
    uint32_t map_file;
    Vec2f    position;
    uint16_t plane;
    uint8_t  unk0;
    uint8_t  is_cinematic;
    uint8_t  unknow[8];
} GameSrv_LoadSpawnPoint;

typedef struct GameSrv_RequestPlayers {
    uint16_t header;
    uint8_t  data[16];
    uint8_t  h0003;
} GameSrv_RequestPlayers;

typedef struct GameSrv_RequestItems {
    uint16_t header;
    uint8_t  unk0;
    uint8_t  unk1;
} GameSrv_RequestItems;

typedef struct GameSrv_CharCreationChangeProf {
    uint16_t header;
    uint8_t  campaign;
    uint8_t  profession;
} GameSrv_CharCreationChangeProf;

typedef struct GameSrv_ChangeEquippedItemColor {
    uint16_t header;
    uint8_t  equipped_item_slot;
    uint8_t  dye_color;
} GameSrv_ChangeEquippedItemColor;

typedef struct GameSrv_CharCreationConfirm {
    uint16_t header;
    uint32_t n_name;
    uint16_t name[20];
    uint8_t  config[8];
    uint32_t flags; // See CharModeFlags
} GameSrv_CharCreationConfirm;

typedef struct GameSrv_ChatMessage {
    uint16_t header;
    uint32_t agent_id;
    uint32_t message_len;
    uint16_t message_buf[138];
} GameSrv_ChatMessage;

typedef struct GameSrv_UpdateProfession {
    uint16_t header;
    uint32_t agent_id;
    uint8_t  primary_profession;
    uint8_t  secondary_profession;
    uint8_t  is_pvp;
} GameSrv_UpdateProfession;

typedef struct GameSrv_UnlockedProf {
    uint16_t header;
    uint32_t agent_id;
    uint32_t unlocked;
} GameSrv_UnlockedProfession;

typedef struct GameSrv_UpdateActiveWeapon {
    uint16_t header;
    uint16_t stream;
    uint8_t  slot;
} GameSrv_UpdateActiveWeapon;

typedef struct GameSrv_InventoryCreateBag {
    uint16_t header;
    uint16_t stream_id;
    uint8_t  bag_type;
    uint8_t  bag_model_id;
    uint16_t bag_id;
    uint8_t  slot_count;
    uint32_t assoc_item_id;
} GameSrv_InventoryCreateBag;

typedef struct GameSrv_UpdatePvpUnlockedSkills {
    uint16_t header;
    uint32_t unlocked_skills_len;
    uint32_t unlocked_skills_buf[128];
} GameSrv_UpdatePvpUnlockedSkills;

typedef struct GameSrv_UpdatePveUnlockedSkills {
    uint16_t header;
    uint32_t unlocked_skills_len;
    uint32_t unlocked_skills_buf[128];
} GameSrv_UpdatePveUnlockedSkills;

typedef struct GameSrv_UnlockedPvpHeroes {
    uint16_t header;
    uint32_t n_bit_map;
    uint32_t bit_map[8];
} GameSrv_UnlockedPvpHeroes;

typedef struct GameSrv_PvpItemAddUnlock {
    uint16_t header;
    uint16_t item_id;
    uint32_t n_params;
    uint32_t params[64];
} GameSrv_PvpItemAddUnlock;

typedef struct GameSrv_AccountFeature {
    uint16_t header;
    uint16_t feature_id;
    uint16_t param1;
    uint16_t param2;
} GameSrv_AccountFeature;

typedef struct GameSrv_KurzickMax {
    uint16_t header;
    uint32_t max_faction;
} GameSrv_KurzickMax;

typedef struct GameSrv_LuxonMax {
    uint16_t header;
    uint32_t max_faction;
} GameSrv_LuxonMax;

typedef struct GameSrv_BalthazarMax {
    uint16_t header;
    uint32_t max_faction;
} GameSrv_BalthazarMax;

typedef struct GameSrv_ImperialMax {
    uint16_t header;
    uint32_t max_faction;
} GameSrv_ImperialMax;

typedef struct GameSrv_SkillbarUpdate {
    uint16_t header;
    uint32_t agent_id;
    uint32_t n_skills;
    uint32_t skills[8];
    uint32_t n_pvp_masks;
    uint32_t pvp_masks[8];
    uint8_t  unk1;
} GameSrv_SkillbarUpdate;

typedef struct GameSrv_UpdateAgentIntProperty {
    uint16_t header;
    uint32_t prop_id;
    uint32_t agent_id;
    uint32_t value;
} GameSrv_UpdateAgentIntProperty;

typedef struct GameSrv_UpdateAgentFloatProperty {
    uint16_t header;
    uint32_t prop_id;
    uint32_t agent_id;
    float    value;
} GameSrv_UpdateAgentFloatProperty;

typedef struct GameSrv_UpdatePlayerFactions {
    uint16_t header;
    uint32_t experience;
    uint32_t current_kurzick;
    uint32_t total_earned_kurzick;
    uint32_t current_luxon;
    uint32_t total_earned_luxon;
    uint32_t current_imperial;
    uint32_t total_earned_imperial;
    uint32_t unk_faction4;
    uint32_t unk_faction5;
    uint32_t level;
    uint32_t morale;
    uint32_t current_balth;
    uint32_t total_earned_balth;
    uint32_t current_skill_points;
    uint32_t total_earned_skill_points;
} GameSrv_UpdatePlayerFactions;

typedef struct GameSrv_CreateNamedItem {
    uint16_t header;
    uint32_t item_id;
    uint32_t file_id;
    uint8_t  item_type;
    uint8_t  dye_tint;
    uint16_t dye_colors;
    uint16_t materials;
    uint8_t  unk1;
    uint32_t flags; // interaction
    uint32_t value;
    uint32_t model_id;
    uint32_t quantity;
    uint32_t n_name;
    uint16_t name[64];
    uint32_t n_modifiers;
    uint32_t modifiers[64];
} GameSrv_CreateNamedItem;

typedef struct GameSrv_CreateUnamedItem {
    uint16_t header;
    uint32_t item_id;
    uint32_t file_id;
    uint8_t  item_type;
    uint8_t  dye_tint;
    uint16_t dye_colors;
    uint16_t materials;
    uint8_t  unk1;
    uint32_t flags; // interaction
    uint32_t value;
} GameSrv_CreateUnamedItem;

typedef struct GameSrv_ItemRemove {
    uint16_t header;
    uint16_t stream_id;
    uint32_t item_id;
} GameSrv_ItemRemove;

typedef struct GameSrv_ItemSetProfession {
    uint16_t header;
    uint32_t item_id;
    uint8_t  profession;
} GameSrv_ItemSetProfession;

typedef struct GameSrv_ItemMoveToLocation {
    uint16_t header;
    uint16_t stream_id;
    uint32_t item_id;
    uint16_t bag_id;
    uint8_t  slot;
} GameSrv_ItemMoveToLocation;

typedef struct GameSrv_CharCreationError {
    uint16_t header;
    uint32_t error_code;
} GameSrv_CharCreationError;

typedef struct GameSrv_CharCreationSuccess {
    uint16_t header;
    uint8_t  char_id[16];
    uint32_t n_name;
    uint16_t name[20];
    uint16_t idk;
    uint32_t n_settings;
    uint8_t  settings[1024];
} GameSrv_CharCreationSuccess;

typedef struct GameSrv_PlayerHeroNameAndInfo {
    uint16_t header;
    uint32_t n_charname;
    uint16_t charname[32];
    uint32_t h0008; // 76
    uint32_t h000C; // 76
    uint32_t h0010; // 1083
    uint32_t h0014; // 0
    uint32_t h0018; // 0
    uint32_t h001C; // 2
} GameSrv_PlayerHeroNameAndInfo;

typedef struct GameSrv_HardModeUnlocked {
    uint16_t header;
    uint8_t  hard_mode_unlocked;
} GameSrv_HardModeUnlocked;

typedef struct GameSrv_UpdateCurrentMap {
    uint16_t header;
    uint16_t map_id;
    uint8_t  unk;
} GameSrv_UpdateCurrentMap;

typedef struct GameSrv_InstanceManifestData {
    uint16_t header;
    uint32_t n_data;
    uint8_t  data[1024];
} GameSrv_InstanceManifestData;

typedef struct GameSrv_InstanceManifestDone {
    uint16_t header;
    uint8_t  download_phase;
    uint16_t map_id;
    uint32_t unk;
} GameSrv_InstanceManifestDone;

typedef struct GameSrv_InstanceManifestPhase {
    uint16_t header;
    uint8_t  download_phase;
} GameSrv_InstanceManifestPhase;

typedef struct GameSrv_SpawnPoint {
    uint16_t header;
    uint32_t map_file_id;
    Vec2f    position;
    uint16_t plane;
    uint8_t  unk0;
    uint8_t  is_cinematic;
    uint8_t  unk1[8];
} GameSrv_SpawnPoint;

typedef struct GameSrv_UnlockedMaps {
    uint16_t header;
    uint32_t completed_missions_nm_len;
    uint32_t completed_missions_nm_buf[32];
    uint32_t completed_bonuses_nm_len;
    uint32_t completed_bonuses_nm_buf[32];
    uint32_t completed_missions_hm_len;
    uint32_t completed_missions_hm_buf[32];
    uint32_t completed_bonuses_hm_len;
    uint32_t completed_bonuses_hm_buf[32];
    uint32_t unlocked_maps_len;
    uint32_t unlocked_maps_buf[32];
} GameSrv_UnlockedMaps;

typedef struct GameSrv_UpdateAgentAttributePoints {
    uint16_t header;
    uint32_t agent_id;
    uint8_t  unused_points;
    uint8_t  used_points;
} GameSrv_UpdateAgentAttributePoints;

typedef struct GameSrv_UpdateAgentAttributes {
    uint16_t header;
    uint32_t agent_id;
    uint32_t data_len;
    uint32_t data_buf[48];
} GameSrv_UpdateAgentAttributes;

typedef struct GameSrv_AgentLoadTime {
    uint16_t header;
    uint32_t load_time;
} GameSrv_AgentLoadTime;

typedef struct GameSrv_CreateAgentMsg {
    uint16_t header;
    uint32_t agent_id;
    uint32_t model_id;   // ho byte: 2=npc, 3=player
    uint8_t  agent_type; // 1=living, 2=gadget, 4=item
    uint8_t  h000B;
    Vec2f    position;
    uint16_t plane;
    Vec2f    direction;
    uint8_t  h001E;
    float    speed_base;
    float    h0023;
    uint32_t h0027;
    uint32_t player_team_token;
    uint32_t h002F;
    uint32_t h0033;
    uint32_t h0037;
    uint32_t h003B;
    uint32_t h003F;
    Vec2f    h0043;
    Vec2f    h004B;
    uint16_t h0053;
    uint32_t h0055;
    Vec2f    h0059;
    uint16_t h0061;
} GameSrv_CreateAgentMsg;

typedef struct GameSrv_PlayerCreate {
    uint16_t header;
    uint32_t player_id;
    uint32_t agent_id;
    uint32_t appearance;
    uint8_t  unk0;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t name_len;
    uint16_t name_buf[32];
} GameSrv_PlayerCreate;

typedef struct GameSrv_PlayerDestroy {
    uint16_t header;
    uint32_t player_id;
} GameSrv_PlayerDestroy;

typedef struct GameSrv_InitialAgentEffects {
    uint16_t header;
    uint32_t agent_id;
    uint32_t effects;
} GameSrv_InitialAgentEffects;

typedef struct GameSrv_AgentRemove {
    uint16_t header;
    uint32_t agent_id;
} GameSrv_AgentRemove;

typedef struct GameSrv_UpdatePlayerAgent {
    uint16_t header;
    uint32_t agent_id;
    uint32_t unk0;
} GameSrv_UpdatePlayerAgent;

typedef struct GameSrv_CreatePartyMsg {
    uint16_t header;
    uint16_t party_id;
} GameSrv_CreatePartyMsg;

typedef struct GameSrv_AddPartyPlayer {
    uint16_t header;
    uint16_t party_id;
    uint16_t player_id;
    uint8_t  is_loaded;
} GameSrv_AddPartyPlayer;

typedef struct GameSrv_RemovePartyPlayer {
    uint16_t header;
    uint16_t party_id;
    uint16_t player_id;
} GameSrv_RemovePartyPlayer;

typedef struct GameSrv_AddPartyHero {
    uint16_t header;
    uint16_t party_id;
    uint16_t owner_player_id;
    uint16_t agent_id;
    uint8_t  hero_id;
    uint8_t  level;
} GameSrv_AddPartyHero;

typedef struct GameSrv_RemovePartyHero {
    uint16_t header;
    uint16_t party_id;
    uint16_t player_id;
    uint16_t agent_id;
} GameSrv_RemovePartyHero;

typedef struct GameSrv_PartyMemberStreamEnd {
    uint16_t header;
    uint16_t party_id;
} GameSrv_PartyMemberStreamEnd;

typedef struct GameSrv_UpdatePlayerParty {
    uint16_t header;
    uint16_t party_id;
    uint8_t  unknown;
} GameSrv_UpdatePlayerParty;

typedef struct GameSrv_CreateEquipmentItem {
    uint16_t header;
    uint32_t item_id;
    uint32_t model_id;
    uint8_t  item_type;
    uint8_t  dye_tint; // not sure
    uint16_t color;
    uint16_t unk1;
    uint8_t  campaign;
    uint32_t flags;
    uint32_t value;
} GameSrv_CreateEquipmentItem;

typedef struct GameSrv_UpdateAgentVisualEquipment {
    uint16_t header;
    uint32_t agent_id;
    uint32_t weapon_item_id;
    uint32_t offhand_item_id;
    uint32_t body_item_id;
    uint32_t boots_item_id;
    uint32_t legs_item_id;
    uint32_t gloves_item_id;
    uint32_t head_item_id;
    uint32_t costume_head_item_id;
    uint32_t costume_body_item_id;
} GameSrv_UpdateAgentVisualEquipment;

typedef struct GameSrv_UpdateWorldSimulationTick {
    uint16_t header;
    uint32_t delta_ms;
} GameSrv_UpdateWorldSimulationTick;

typedef struct GameSrv_UpdateAgentPostion {
    uint16_t header;
    uint32_t agent_id;
    Vec2f    position;
    uint16_t plane;
} GameSrv_UpdateAgentPostion;

typedef struct GameSrv_MoveToCoord {
    uint16_t header;
    Vec2f    pos;
    uint32_t plane;
} GameSrv_MoveToCoord;

typedef struct GameSrv_MoveAgentToPoint {
    uint16_t header;
    uint32_t agent_id;
    Vec2f    dest;
    uint16_t plane;
    uint16_t current_plane;
} GameSrv_MoveAgentToPoint;

typedef struct GameSrv_UpdateAgentRotation {
    uint16_t header;
    uint32_t agent_id;
    uint32_t sin;
    uint32_t cos;
} GameSrv_UpdateAgentRotation;

typedef struct GameSrv_AgentDisplayCape {
    uint16_t header;
    uint32_t agent_id;
    uint8_t  status;
} GameSrv_AgentDisplayCape;

typedef struct GameSrv_AgentStopMoving {
    uint16_t header;
    uint32_t agent_id;
} GameSrv_AgentStopMoving;

typedef struct GameSrv_LastPosBeforeMoveCanceled {
    uint16_t header;
    Vec2f    pos;
    uint32_t plane;
} GameSrv_LastPosBeforeMoveCanceled;

typedef struct GameSrv_UpdatePlayerPartySize {
    uint16_t header;
    uint16_t player_id;
    uint8_t  party_size;
} GameSrv_UpdatePlayerPartySize;

typedef struct GameSrv_AddPlayerToPlayerParty {
    uint16_t header;
    uint16_t player_id;
    uint16_t party_leader_player_id;
} GameSrv_AddPlayerToPlayerParty;

#define CHAT_MESSAGE_FRAGMENT_MAX_LENGTH 122
typedef struct GameSrv_ChatMessageCore {
    uint16_t header;
    uint32_t msg_len;
    uint16_t msg_buf[CHAT_MESSAGE_FRAGMENT_MAX_LENGTH];
} GameSrv_ChatMessageCore;

typedef struct GameSrv_ChatMessageLocal {
    uint16_t header;
    uint16_t player_id_of_sender;
    uint8_t  channel;
} GameSrv_ChatMessageLocal;

typedef struct GameSrv_ChatMessageGlobal {
    uint16_t header;
    uint8_t  channel;
    uint32_t sender_len;
    uint16_t sender_buf[32];
    uint32_t tag_len;
    uint16_t tag_buf[6];
} GameSrv_ChatMessageGlobal;

typedef struct GameSrv_ChatMessageWhisper {
    uint16_t header;
    uint32_t unk0;
    uint32_t sender_len;
    uint16_t sender_buf[20];
    uint32_t msg_len;
    uint16_t msg_buf[256];
} GameSrv_ChatMessageWhisper;

typedef struct GameSrv_ChatMessageServer {
    uint16_t header;
    uint16_t some_id;
    uint8_t  channel;
} GameSrv_ChatMessageServer;

typedef struct GameSrv_TransferGameServerInfo {
    uint16_t header;
    uint8_t  host[24];
    uint32_t map_token;
    int8_t   region;
    uint16_t map_id;
    uint8_t  is_explorable;
    uint32_t player_token;
    uint8_t  h00F;
} GameSrv_TransferGameServerInfo;

typedef union GameCliMsg {
    uint16_t                             header;
    uint8_t                              buffer[MSG_MAX_BUFFER_SIZE];
    GameSrv_LoadSpawnPoint               load_spawn_point;
    GameSrv_RequestPlayers               request_players;
    GameSrv_RequestItems                 request_items;
    GameSrv_CharCreationChangeProf       char_creation_change_prof;
    GameSrv_ChangeEquippedItemColor      change_equipped_item_color;
    GameSrv_CharCreationConfirm          char_creation_confirm;
    GameSrv_ChatMessage                  chat_message;
    GameSrv_MoveToCoord                  move_to_coord;
    GameSrv_LastPosBeforeMoveCanceled    last_pos_before_move_canceled;
} GameCliMsg;

typedef union GameSrvMsg {
    uint16_t                             header;
    uint8_t                              buffer[MSG_MAX_BUFFER_SIZE];
    GameSrv_PingReply                    ping_reply;
    GameSrv_InstanceHead                 instance_head;
    GameSrv_InstancePlayerName           instance_player_name;
    GameSrv_InstanceInfo                 instance_info;
    GameSrv_ReadyForMapSpawn             ready_for_map_spawn;
    GameSrv_ItemStreamCreate             item_stream_create;
    GameSrv_UpdateGold                   update_gold;
    GameSrv_WeaponSet                    weapon_set;
    GameSrv_LoadSpawnPoint               load_spawn_point;
    GameSrv_UpdateProfession             update_profession;
    GameSrv_UnlockedProfession           unlocked_profession;
    GameSrv_UpdateActiveWeapon           update_active_weapon_set;
    GameSrv_InventoryCreateBag           inventory_create_bag;
    GameSrv_UpdatePvpUnlockedSkills      update_pvp_unlocked_skills;
    GameSrv_UpdatePveUnlockedSkills      update_pve_unlocked_skills;
    GameSrv_UnlockedPvpHeroes            unlocked_pvp_heroes;
    GameSrv_PvpItemAddUnlock             pvp_item_add_unlock;
    GameSrv_AccountFeature               account_feature;
    GameSrv_KurzickMax                   kurzick_max;
    GameSrv_LuxonMax                     luxon_max;
    GameSrv_BalthazarMax                 balthazar_max;
    GameSrv_ImperialMax                  imperial_max;
    GameSrv_SkillbarUpdate               skillbar_update;
    GameSrv_UpdateAgentIntProperty       update_agent_int_property;
    GameSrv_UpdateAgentFloatProperty     update_agent_float_property;
    GameSrv_UpdatePlayerFactions         update_player_factions;
    GameSrv_CreateNamedItem              create_named_item;
    GameSrv_CreateUnamedItem             create_unnamed_item;
    GameSrv_ItemRemove                   item_remove;
    GameSrv_ItemSetProfession            item_set_profession;
    GameSrv_ItemMoveToLocation           item_move_to_location;
    GameSrv_CharCreationError            char_creation_error;
    GameSrv_CharCreationSuccess          char_creation_success;
    GameSrv_PlayerHeroNameAndInfo        player_hero_name_and_info;
    GameSrv_HardModeUnlocked             hard_mode_unlocked;
    GameSrv_UpdateCurrentMap             update_current_map;
    GameSrv_InstanceManifestData         instance_manifest_data;
    GameSrv_InstanceManifestDone         instance_manifest_done;
    GameSrv_InstanceManifestPhase        instance_manifest_phase;
    GameSrv_SpawnPoint                   spawn_point;
    GameSrv_InstanceLoaded               instance_loaded;
    GameSrv_UnlockedMaps                 unlocked_maps;
    GameSrv_UpdateAgentAttributePoints   update_agent_attribute_points;
    GameSrv_UpdateAgentAttributes        update_agent_attributes;
    GameSrv_AgentLoadTime                agent_load_time;
    GameSrv_CreateAgentMsg               create_agent;
    GameSrv_PlayerCreate                 player_create;
    GameSrv_PlayerDestroy                player_destroy;
    GameSrv_InitialAgentEffects          initial_agent_effects;
    GameSrv_AgentRemove                  agent_remove;
    GameSrv_UpdatePlayerAgent            update_player_agent;
    GameSrv_CreatePartyMsg               create_party;
    GameSrv_AddPartyPlayer               add_party_player;
    GameSrv_RemovePartyPlayer            remove_party_player;
    GameSrv_AddPartyHero                 add_party_hero;
    GameSrv_RemovePartyHero              remove_party_hero;
    GameSrv_PartyMemberStreamEnd         party_member_stream_end;
    GameSrv_UpdatePlayerParty            update_player_party;
    GameSrv_UpdateAgentVisualEquipment   update_agent_visual_equipment;
    GameSrv_CreateEquipmentItem          create_equipment_item;
    GameSrv_UpdateWorldSimulationTick    world_simulation_tick;
    GameSrv_UpdateAgentPostion           update_agent_position;
    GameSrv_MoveAgentToPoint             move_agent_to_point;
    GameSrv_AgentStopMoving              agent_stop_moving;
    GameSrv_UpdateAgentRotation          update_agent_rotation;
    GameSrv_AgentDisplayCape             agent_display_cape;
    GameSrv_UpdatePlayerPartySize        update_player_party_size;
    GameSrv_AddPlayerToPlayerParty       add_player_to_player_party;
    GameSrv_ChatMessageCore              chat_message_core;
    GameSrv_ChatMessageLocal             chat_message_local;
    GameSrv_ChatMessageGlobal            chat_message_global;
    GameSrv_ChatMessageWhisper           chat_message_whisper;
    GameSrv_ChatMessageServer            chat_message_server;
    GameSrv_TransferGameServerInfo       transfer_game_server_info;
} GameSrvMsg;
#pragma pack(pop)
