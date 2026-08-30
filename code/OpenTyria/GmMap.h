#pragma once

// 0  = main town
// 1  = main explorable
// 2  = guild hall
// 3  = outpost/mission outpost
// 4  = mission explorable
// 5  = arena outpost
// 6  = arena explorable
// 7  = HA outpost
// 11 = character creation

typedef enum ManifestPhase {
    ManifestPhase_Phase1 = 0,
    ManifestPhase_Phase2 = 1,
    ManifestPhase_Done   = 2,
    ManifestPhase_Count
} ManifestPhase;

typedef enum MapType {
    MapType_MainTown = 0,
    MapType_MainExplorable = 1,
    MapType_GuildHall = 2,
    MapType_MissionOutpost = 3,
    MapType_MissionExplorable = 4,
    MapType_ArenaOutpost = 5,
    MapType_ArenaExplorable = 6,
    MapType_HeroesAscentOutpost = 7,
    MapType_CharacterCreation = 11,
} MapType;

int MapType_FromInt(MapType *result, int value)
{
    switch (value) {
    case MapType_MainTown:
    case MapType_MainExplorable:
    case MapType_GuildHall:
    case MapType_MissionOutpost:
    case MapType_MissionExplorable:
    case MapType_ArenaOutpost:
    case MapType_ArenaExplorable:
    case MapType_HeroesAscentOutpost:
    case MapType_CharacterCreation:
        *result = (MapType) value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

typedef enum DistrictRegion {
    DistrictRegion_International    = -2,
    DistrictRegion_America          = 0,
    DistrictRegion_Korea            = 1,
    DistrictRegion_Europe           = 2,
    DistrictRegion_China            = 3,
    DistrictRegion_Japanese         = 4,
} DistrictRegion;

int DistrictRegion_FromInt(DistrictRegion *result, int value)
{
    switch (value) {
    case DistrictRegion_International:
    case DistrictRegion_America:
    case DistrictRegion_Korea:
    case DistrictRegion_Europe:
    case DistrictRegion_China:
    case DistrictRegion_Japanese:
        *result = (DistrictRegion) value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

typedef enum DistrictLanguage {
    DistrictLanguage_Default            = 0,
    DistrictLanguage_English            = 0,
    DistrictLanguage_Korean             = 1,
    DistrictLanguage_French             = 2,
    DistrictLanguage_German             = 3,
    DistrictLanguage_Italian            = 4,
    DistrictLanguage_Spanish            = 5,
    DistrictLanguage_TraditionalChinese = 6,
    DistrictLanguage_Japanese           = 8,
    DistrictLanguage_Polish             = 9,
    DistrictLanguage_Russian            = 10,
    DistrictLanguage_BorkBrokBork       = 17,
    DistrictLanguage_Unknown            = UINT32_MAX,
} DistrictLanguage;

int DistrictLanguage_FromInt(DistrictLanguage *result, int value)
{
    switch (value) {
    case DistrictLanguage_English:
    case DistrictLanguage_Korean:
    case DistrictLanguage_French:
    case DistrictLanguage_German:
    case DistrictLanguage_Italian:
    case DistrictLanguage_Spanish:
    case DistrictLanguage_TraditionalChinese:
    case DistrictLanguage_Japanese:
    case DistrictLanguage_Polish:
    case DistrictLanguage_Russian:
    case DistrictLanguage_BorkBrokBork:
    case DistrictLanguage_Unknown:
        *result = (DistrictLanguage) value;
        return ERR_OK;
    default:
        return ERR_UNSUCCESSFUL;
    }
}

uint8_t DistrictLanguage_ToInt(DistrictLanguage language)
{
    switch (language) {
    case DistrictLanguage_English:
    case DistrictLanguage_Korean:
    case DistrictLanguage_French:
    case DistrictLanguage_German:
    case DistrictLanguage_Italian:
    case DistrictLanguage_Spanish:
    case DistrictLanguage_TraditionalChinese:
    case DistrictLanguage_Japanese:
    case DistrictLanguage_Polish:
    case DistrictLanguage_Russian:
    case DistrictLanguage_BorkBrokBork:
    case DistrictLanguage_Unknown:
        return (uint8_t)language;
    default:
        abort();
    }
}

typedef enum RegionType {
    RegionType_AllianceBattle,
    RegionType_Arena,
    RegionType_ExplorableZone,
    RegionType_GuildBattleArea,
    RegionType_GuildHall,
    RegionType_MissionOutpost,
    RegionType_CooperativeMission,
    RegionType_CompetitiveMission,
    RegionType_EliteMission,
    RegionType_Challenge,
    RegionType_Outpost,
    RegionType_ZaishenBattle,
    RegionType_HeroesAscent,
    RegionType_City,
    RegionType_MissionArea,
    RegionType_HeroBattleOutpost,
    RegionType_HeroBattleArea,
    RegionType_EotnMission,
    RegionType_Dungeon,
    RegionType_Marketplace,
    RegionType_Unknown,
    RegionType_DevRegion,
} RegionType;

typedef enum Region {
    Region_Kryta,
    Region_Maguuma,
    Region_Ascalon,
    Region_NorthernShiverpeaks,
    Region_HeroesAscent,
    Region_CrystalDesert,
    Region_FissureOfWoe,
    Region_Presearing,
    Region_Kaineng,
    Region_Kurzick,
    Region_Luxon,
    Region_ShingJea,
    Region_Kourna,
    Region_Vaabi,
    Region_Desolation,
    Region_Istan,
    Region_DomainOfAnguish,
    Region_TarnishedCoast,
    Region_DepthsOfTyria,
    Region_FarShiverpeaks,
    Region_CharrHomelands,
    Region_BattleIslands,
    Region_TheBattleOfJahai,
    Region_TheFlightNorth,
    Region_TheTenguAccords,
    Region_TheRiseOfTheWhiteMantle,
    Region_Swat,
    Region_DevRegion,
} Region;

typedef struct GmDistrict {
    uint16_t         map_id;
    DistrictRegion   region;
    DistrictLanguage language;
    MapType          map_type;
    uint32_t         district_number;
} GmDistrict;
