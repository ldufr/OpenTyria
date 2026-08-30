#pragma once

#define TitleIdDefs \
    X(TitleId_Hero, 0) \
    X(TitleId_TyrianCarto, 1) \
    X(TitleId_CanthanCarto, 2) \
    X(TitleId_Gladiator, 3) \
    X(TitleId_Champion, 4) \
    X(TitleId_Kurzick, 5) \
    X(TitleId_Luxon, 6) \
    X(TitleId_Drunkard, 7) \
    X(TitleId_Deprecated_SkillHunter, 8) \
    X(TitleId_Survivor, 9) \
    X(TitleId_KoaBD, 10) \
    X(TitleId_Deprecated_TreasureHunter, 11) \
    X(TitleId_Deprecated_Wisdom, 12) \
    X(TitleId_ProtectorTyria, 13) \
    X(TitleId_ProtectorCantha, 14) \
    X(TitleId_Lucky, 15) \
    X(TitleId_Unlucky, 16) \
    X(TitleId_Sunspear, 17) \
    X(TitleId_ElonianCarto, 18) \
    X(TitleId_ProtectorElona, 19) \
    X(TitleId_Lightbringer, 20) \
    X(TitleId_LDoA, 21) \
    X(TitleId_Commander, 22) \
    X(TitleId_Gamer, 23) \
    X(TitleId_SkillHunterTyria, 24) \
    X(TitleId_VanquisherTyria, 25) \
    X(TitleId_SkillHunterCantha, 26) \
    X(TitleId_VanquisherCantha, 27) \
    X(TitleId_SkillHunterElona, 28) \
    X(TitleId_VanquisherElona, 29) \
    X(TitleId_LegendaryCarto, 30) \
    X(TitleId_LegendaryGuardian, 31) \
    X(TitleId_LegendarySkillHunter, 32) \
    X(TitleId_LegendaryVanquisher, 33) \
    X(TitleId_Sweets, 34) \
    X(TitleId_GuardianTyria, 35) \
    X(TitleId_GuardianCantha, 36) \
    X(TitleId_GuardianElona, 37) \
    X(TitleId_Asuran, 38) \
    X(TitleId_Deldrimor, 39) \
    X(TitleId_Vanguard, 40) \
    X(TitleId_Norn, 41) \
    X(TitleId_MasterOfTheNorth, 42) \
    X(TitleId_Party, 43) \
    X(TitleId_Zaishen, 44) \
    X(TitleId_TreasureHunter, 45) \
    X(TitleId_Wisdom, 46) \
    X(TitleId_Codex, 47) \

typedef enum TitleId {
    #define X(name, val) name = val,
    TitleIdDefs
    #undef X
    Title_Count,
} TitleId;

const char* Title_ToString(TitleId title_id)
{
    switch (title_id) {
    #define X(name, val) case name: return #name;
    TitleIdDefs
    #undef X
    default: abort();
    }
}

int Title_FromInt(int title_id, TitleId *result)
{
    switch (title_id) {
    #define X(name, val) case name: break;
    TitleIdDefs
    #undef X
    default:
        return ERR_UNSUCCESSFUL;
    }
    *result = (TitleId) title_id;
    return ERR_OK;
}

typedef enum TitleType {
    TitleType_Points = 0,
    TitleType_Cartographer = 1,
    TitleType_Missions = 2,
    TitleType_Deprecated = 3,
    TitleType_Account = 4,
} TitleType;

typedef struct TitleRank {
    uint32_t rank_id;
    uint32_t current_rank;
    uint32_t current_rank_points;
    uint32_t next_rank;
    uint32_t next_rank_points;
} TitleRank;

typedef slice(TitleRank) TitleRankSlice;

typedef struct TitleConstData {
    TitleId        title_id;
    TitleType      title_type;
    TitleRankSlice ranks;
    uint32_t       point_name_len;
    uint16_t       point_name_buf[8];
    uint32_t       description_len;
    uint16_t       description_buf[8];
} TitleConstData;

typedef struct Title {
    TitleId  title_id;
    uint32_t current_points;
    uint32_t current_rank;
    TitleConstData *const_data;
} Title;
