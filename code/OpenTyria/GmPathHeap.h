#pragma once

typedef struct PathHeapNode {
    float    cost;
    uint32_t trap_id;
} PathHeapNode;
typedef array(PathHeapNode) PathHeapArray;

void PathHeapPush(PathHeapArray *heap, PathHeapNode node);
bool PathHeapPop(PathHeapArray *heap, PathHeapNode *node);
