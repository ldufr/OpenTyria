#pragma once

#define DbSessionColumnsDef \
    X(GmUuid, user_id) \
    X(GmUuid, session_id) \
    X(int64_t, created_at) \
    X(int64_t, updated_at) \
    X(GmUuid, account_id) \

typedef struct DbSession {
    #define X(T, N) T N;
    DbSessionColumnsDef
    #undef X
} DbSession;

typedef enum DbSessionCols {
    #define X(T, N) DbSessionCols_ ## N,
    DbSessionColumnsDef
    #undef X
} DbSessionCols;

static const char *DbSessionColsName[] = {
    #define X(T, N) #N,
    DbSessionColumnsDef
    #undef X
};

#define DbAccountColumnsDef \
    X(GmUuid, account_id) \
    X(int64_t, created_at) \
    X(int64_t, updated_at) \
    X(bool, eula_accepted) \
    X(GmUuid, current_char_id) \
    X(uint16_t, current_territory) \
    X(uint16_t, storage_gold) \
    X(uint32_t, balthazar_points_max) \
    X(uint32_t, balthazar_points_amount) \
    X(uint32_t, balthazar_points_total) \
    X(uint32_t, kurzick_points_max) \
    X(uint32_t, kurzick_points_amount) \
    X(uint32_t, kurzick_points_total) \
    X(uint32_t, luxon_points_max) \
    X(uint32_t, luxon_points_amount) \
    X(uint32_t, luxon_points_total) \
    X(uint32_t, imperial_points_max) \
    X(uint32_t, imperial_points_amount) \
    X(uint32_t, imperial_points_total) \

typedef struct DbAccount {
    #define X(T, N) T N;
    DbAccountColumnsDef
    #undef X
} DbAccount;

typedef enum DbAccountCols {
    #define X(T, N) DbAccountCols_ ## N,
    DbAccountColumnsDef
    #undef X
} DbAccountCols;

static const char *DbAccountColsName[] = {
    #define X(T, N) #N,
    DbAccountColumnsDef
    #undef X
};

#define DbCharacterColumnsDef \
    X(GmUuid, char_id) \
    X(int64_t, created_at) \
    X(int64_t, updated_at) \
    X(GmUuid, account_id) \
    X(static_array(uint16_t, 20), charname) \
    X(uint16_t, last_outpost) \
    X(GmUuid, last_guild_hall_id) \
    X(uint8_t, sex) \
    X(uint8_t, height) \
    X(uint8_t, skin_color) \
    X(uint8_t, hair_color) \
    X(uint8_t, face_style) \
    X(uint8_t, hair_style) \
    X(uint8_t, race) \
    X(uint8_t, campaign) \
    X(uint8_t, level) \
    X(uint8_t, is_pvp) \
    X(uint8_t, helm_status) \
    X(uint32_t, skill_points) \
    X(uint32_t, skill_points_total) \
    X(uint32_t, experience) \
    X(uint16_t, gold) \
    X(uint8_t, active_weapon_set) \
    X(uint8_t, primary_profession) \
    X(uint8_t, secondary_profession) \
    X(Bitmap1024, unlocked_skills) \
    X(Bitmap256, unlocked_maps) \
    X(Bitmap256, completed_missions_nm) \
    X(Bitmap256, completed_bonuses_nm) \
    X(Bitmap256, completed_missions_hm) \
    X(Bitmap256, completed_bonuses_hm) \
    X(uint32_t, unlocked_professions) \
    X(uint32_t, skill1) \
    X(uint32_t, skill2) \
    X(uint32_t, skill3) \
    X(uint32_t, skill4) \
    X(uint32_t, skill5) \
    X(uint32_t, skill6) \
    X(uint32_t, skill7) \
    X(uint32_t, skill8) \
    X(uint16_t, file_id_body) \
    X(uint16_t, file_id_legs) \
    X(uint16_t, file_id_head) \
    X(uint16_t, file_id_boots) \
    X(uint16_t, file_id_gloves) \
    X(uint16_t, colors_body) \
    X(uint16_t, colors_legs) \
    X(uint16_t, colors_head) \
    X(uint16_t, colors_boots) \
    X(uint16_t, colors_gloves) \

typedef struct DbCharacter {
    #define X(T, N) T N;
    DbCharacterColumnsDef
    #undef X
} DbCharacter;

typedef enum DbCharacterCols {
    #define X(T, N) DbCharacterCols_ ## N,
    DbCharacterColumnsDef
    #undef X
    DbCharacterCols_Count,
} DbCharacterCols;

static const char *DbCharacterColsName[] = {
    #define X(T, N) #N,
    DbCharacterColumnsDef
    #undef X
};

typedef array(DbCharacter) DbCharacterArray;

#define DbBagColumnsDef \
    X(GmUuid, account_id) \
    X(GmUuid, char_id) \
    X(int64_t, created_at) \
    X(int64_t, updated_at) \
    X(uint8_t, bag_model_id) \
    X(uint8_t, bag_type) \
    X(uint8_t, slot_count) \

typedef struct DbBag {
    #define X(T, N) T N;
    DbBagColumnsDef
    #undef X
} DbBag;

typedef enum DbBagCols {
    #define X(T, N) DbBagCols_ ## N,
    DbBagColumnsDef
    #undef X
} DbBagCols;

static const char *DbBagColsName[] = {
    #define X(T, N) #N,
    DbBagColumnsDef
    #undef X
};

typedef array(DbBag) DbBagArray;

#define DbItemColumnsDef \
    X(GmUuid, account_id) \
    X(GmUuid, char_id) \
    X(uint8_t, bag_model_id) \
    X(uint16_t, slot) \
    X(int64_t, created_at) \
    X(int64_t, updated_at) \
    X(uint16_t, quantity) \
    X(uint8_t, dye_tint) \
    X(uint8_t, dye_colors) \
    X(uint8_t, item_type) \
    X(uint8_t, profession) \
    X(uint32_t, model_id) \
    X(uint32_t, file_id) \
    X(uint32_t, flags) \

typedef struct DbItem {
    #define X(T, N) T N;
    DbItemColumnsDef
    #undef X
} DbItem;

typedef enum DbItemCols {
    #define X(T, N) DbItemCols_ ## N,
    DbItemColumnsDef
    #undef X
} DbItemCols;

static const char *DbItemColsName[] = {
    #define X(T, N) #N,
    DbItemColumnsDef
    #undef X
};

typedef array(DbItem) DbItemArray;
