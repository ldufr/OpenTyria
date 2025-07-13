#pragma once

typedef struct Vec2f {
    float x;
    float y;
} Vec2f;
typedef array(Vec2f) Vec2fArray;

typedef struct Vec3f {
    float x;
    float y;
    float z;
} Vec3f;

typedef struct GmPos {
    float    x;
    float    y;
    uint16_t plane;
} GmPos;
typedef slice(GmPos) GmPosSlice;

typedef struct GameSrv GameSrv;
typedef struct GameConnection GameConnection;

typedef struct GmAgent GmAgent;
typedef struct GmBag GmBag;
typedef struct GmItem GmItem;
typedef struct GmPlayer GmPlayer;

typedef struct GameSrv_ChatMessage GameSrv_ChatMessage;
