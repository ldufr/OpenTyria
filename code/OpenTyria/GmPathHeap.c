#pragma once

void PathHeapPush(PathHeapArray *heap, PathHeapNode node)
{
    array_add(heap, node);
    if (heap->len < 2) {
        return;
    }

    PathHeapNode *nodes = heap->ptr;

    size_t hole = heap->len - 1;
    while (0 < hole) {
        size_t parent_idx = (hole - 1) >> 1;
        if (nodes[parent_idx].cost < node.cost)
            break;
        nodes[hole] = nodes[parent_idx];
        hole = parent_idx;
    }

    nodes[hole] = node;
}

bool PathHeapPop(PathHeapArray *heap, PathHeapNode *node)
{
    if (heap->len == 0)
        return false;

    size_t idx = 0, hole = 0, len = --heap->len;
    PathHeapNode *nodes = heap->ptr;

    PathHeapNode last_node = nodes[len];
    *node = nodes[0];

    if (len == 0) {
        return true;
    }

    // This store the last time a parent must have two childs.
    const size_t last_full_depth_idx = len >> 1;
    while (idx < last_full_depth_idx) {
        idx = (idx * 2) + 2;
        // Find the child with the lowest value.
        if (nodes[idx - 1].cost < nodes[idx].cost)
            --idx;
        if (last_node.cost <= nodes[idx].cost)
            break;
        nodes[hole] = nodes[idx];
        hole = idx;
    }

    if (idx == last_full_depth_idx && (len % 2) == 0) {
        if (nodes[len - 1].cost < last_node.cost) {
            nodes[hole] = nodes[len - 1];
            hole = len - 1;
        }
    }

    nodes[hole] = last_node;
    return true;
}
