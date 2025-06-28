/*
void push_heap_at(PathPrioQ* heap, size_t idx)
{
    assert(idx < heap->len);

    PathNode *nodes = heap->ptr;
    PathNode pathNode = nodes[idx];

    size_t hole = idx;
    while (0 < hole) {
        size_t parentIdx = (hole - 1) >> 1;
        if (nodes[parentIdx].value < pathNode.value)
            break;
        nodes[hole] = nodes[parentIdx];
        hole = parentIdx;
    }

    nodes[hole] = pathNode;
}

void push_heap(PathPrioQ* heap, PathNode pathNode)
{
    array_add(heap, pathNode);
    if (heap->len < 2) {
        return;
    }

    push_heap_at(heap, heap->len - 1);
}

void pop_heap(PathPrioQ *heap)
{
    if (heap->len < 2)
        return;

    size_t idx = 0, hole = 0, count = heap->len - 1;
    PathNode *nodes = heap->ptr;

    PathNode lastNode = nodes[count];
    nodes[count] = nodes[0];

    // This store the last time a parent must have two childs.
    size_t lastParentOfFullDepthIdx = count >> 1;
    while (idx < lastParentOfFullDepthIdx) {
        idx = (idx * 2) + 2;
        if (nodes[idx - 1].value < nodes[idx].value)
            --idx;
        if (lastNode.value <= nodes[idx].value)
            break;
        nodes[hole] = nodes[idx];
        hole = idx;
    }

    if (idx == lastParentOfFullDepthIdx && (count % 2) == 0) {
        if (nodes[count - 1].value < lastNode.value) {
            nodes[hole] = nodes[count - 1];
            hole = count - 1;
        }
    }

    nodes[hole] = lastNode;
}

void PathFinding(PathContext *pathContext, GamePos srcPos, GamePos dstPos, ArrayWaypoint *waypoints)
{
    PathFindTrapezoid(pathContext, srcPos);

    // visit
    PathPrioQ prioq = {0};
}
*/
