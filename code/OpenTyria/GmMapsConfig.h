#pragma once

#define GM_MAP_CONFIG_MAX_SPAWNPOINTS 10

typedef struct GmMapConfig {
    MapId     map_id;
    uint32_t  map_file_id;
    uint32_t  n_spawnpoints;
    GmPos     spawnpoints[GM_MAP_CONFIG_MAX_SPAWNPOINTS];
} GmMapConfig;

GmMapConfig* GetMapConfigForMapId(MapId map_id);
