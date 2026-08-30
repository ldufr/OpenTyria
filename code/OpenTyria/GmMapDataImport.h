#pragma once

typedef enum ChunkId {
    ChunkId_Header = 0,
    ChunkId_EditorOld = 1,
    ChunkId_Terrain = 2,
    ChunkId_Zones = 3,
    ChunkId_Props = 4,
    ChunkId_Obsolete1 = 5,
    ChunkId_Water = 6,
    ChunkId_Mission = 7,
    ChunkId_Path = 8,
    ChunkId_Environment = 9,
    ChunkId_Locations = 10,
    ChunkId_Obsolete2 = 11,
    ChunkId_MapParameters = 12,
    ChunkId_Editor = 13,
    ChunkId_Collision = 14,
    ChunkId_Light = 15,
    ChunkId_Shore = 16,
} ChunkId;

typedef enum RiffType {
    RIFF_NONE  = 0,
    RIFF_MAP1  = 1,
    RIFF_MAP2  = 3,
    RIFF_TYPES = 9,
} RiffType;

typedef struct MapChunk {
    uint32_t      chunk_id;
    uint32_t      chunk_size;
    slice_uint8_t chunk_data;
} MapChunk;
typedef array(MapChunk) MapChunkArray;

typedef struct GmMapContext {
    GmPathContext path;
} GmMapContext;

void GmMapContext_Free(GmMapContext *context);

int GmImportMap(GmMapContext *context, slice_uint8_t content);
