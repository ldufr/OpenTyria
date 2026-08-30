#pragma once

GmMapConfig s_KamadanJewelOfIstanOutpost = {
    .map_id = MapId_KamadanJewelOfIstanOutpost,
    .map_file_id = 0x345CC,
    .n_spawnpoints = 1,
    .spawnpoints = {
        { -9067.f, 13218.f, 0 },
    },
};

GmMapConfig s_KainengCenterOutpost = {
    .map_id = MapId_KainengCenterOutpost,
    .map_file_id = 0x265F7,
};

GmMapConfig s_LionsArchOutpost = {
    .map_id = MapId_LionsArchOutpost,
    .map_file_id = 352808,
};

GmMapConfig s_DomainOfAnguish = {
    .map_id = MapId_DomainOfAnguish,
    .map_file_id = 219215,
};

GmMapConfig s_SparkflySwamp = {
    .map_id = MapId_SparkflySwamp,
    .map_file_id = 287493,
};

GmMapConfig s_LornarsPass = {
    .map_id = MapId_LornarsPass,
    .map_file_id = 46594,
};

GmMapConfig* GetMapConfigForMapId(MapId map_id)
{
    switch (map_id) {
    case MapId_KamadanJewelOfIstanOutpost:
        return &s_KamadanJewelOfIstanOutpost;
    case MapId_KainengCenterOutpost:
        return &s_KainengCenterOutpost;
    case MapId_LionsArchOutpost:
        return &s_LionsArchOutpost;
    case MapId_DomainOfAnguish:
        return &s_DomainOfAnguish;
    case MapId_SparkflySwamp:
        return &s_SparkflySwamp;
    case MapId_LornarsPass:
        return &s_LornarsPass;
    default:
        return NULL;
    }
}
