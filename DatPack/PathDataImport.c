#define PATH_DATA_SIGNATURE     (0xEEFE704C)
#define PATH_DATA_VERSION       (12)
#define PATH_SEQUENCE_FAST_SAVE (-1)

#define FIRST_X_NODE_INDEX    0
#define FIRST_Y_NODE_INDEX    0x40000000
#define FIRST_SINK_NODE_INDEX 0x80000000

typedef struct PortalPairRef {
    uint32_t key;
    Portal *portal;
} PortalPairRef;

typedef struct PathDataImportContext {
    PathPlane      *plane;
    PathStaticData *static_data;
    PortalPairRef  *portal_pairs;
} PathDataImportContext;

int PathImport_CheckHeader(slice_uint8_t *data)
{
    if (data->len < 12) {
        return 1;
    }

    uint32_t signature = le32dec(&data->ptr[0]);
    uint32_t version = le32dec(&data->ptr[4]);
    uint32_t sequence = le32dec(&data->ptr[8]);

    assert(sequence != PATH_SEQUENCE_FAST_SAVE);
    if (signature != PATH_DATA_SIGNATURE || version != PATH_DATA_VERSION)
        return 1;

    slice_advance(data, 12);
    return 0;
}

int PathImport_Step2(slice_uint8_t *data)
{
    if (data->len <= 5) {
        return 1;
    }

    uint32_t size = le32dec(&data->ptr[1]);
    slice_advance(data, 5);

    if (data->len < size) {
        return 1;
    }

    slice_advance(data, size);
    return 0;
}

int ParseTaggedChunk(slice_uint8_t *data, uint8_t tag, uint32_t *outLength)
{
    if (data->len < 5)
        return 1;
    if (data->ptr[0] != tag)
        return 1;
    uint32_t length = le32dec(&data->ptr[1]);
    slice_advance(data, 5);
    if (data->len < length)
        return 1;
    *outLength = length;
    return 0;
}

int PathImport_PlaneReadCounts(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 0, &subLength)) != 0)
        return err;

    if (subLength < 32) {
        return 1;
    }

    uint32_t h000C = le32dec(&data->ptr[0]);
    uint32_t vectorCount = le32dec(&data->ptr[4]);
    uint32_t trapezoidCount = le32dec(&data->ptr[8]);
    uint32_t xNodeCount = le32dec(&data->ptr[12]);
    uint32_t yNodeCount = le32dec(&data->ptr[16]);
    uint32_t sinkNodeCount = le32dec(&data->ptr[20]);
    uint32_t portalCount = le32dec(&data->ptr[24]);
    uint32_t portalTrapsCount = le32dec(&data->ptr[28]);

    array_resize(&plane->vectors, vectorCount);
    array_resize(&plane->trapezoids, trapezoidCount);
    array_resize(&plane->xnodes, xNodeCount);
    array_resize(&plane->ynodes, yNodeCount);
    array_resize(&plane->sink_nodes, sinkNodeCount);
    array_resize(&plane->portals, portalCount);
    array_resize(&plane->portals_traps, portalTrapsCount);

    plane->h000C = h000C;

    printf("h000C          = %" PRIu32 "\n", h000C);
    printf("datVectorCount = %" PRIu32 "\n", vectorCount);
    printf("trapezoidCount = %" PRIu32 "\n", trapezoidCount);
    printf("xNodeCount     = %" PRIu32 "\n", xNodeCount);
    printf("yNodeCount     = %" PRIu32 "\n", yNodeCount);
    printf("sinkNodeCount  = %" PRIu32 "\n", sinkNodeCount);
    printf("portalCount    = %" PRIu32 "\n", portalCount);
    printf("portalsTrapsCount = %" PRIu32 "\n", portalTrapsCount);

    slice_advance(data, 32);

    return 0;
}

int PathImport_PlaneTag11(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 11, &subLength)) != 0)
        return err;

    assert(plane->h000C * 8 <= subLength);
    slice_advance(data, plane->h000C * 8);
    return 0;
}

PathTrapezoid *GetTrapHelper(PathTrapezoidArray *traps, uint32_t datIdx)
{
    if (datIdx == UINT32_MAX)
        return NULL;
    assert(datIdx < traps->len);
    return &traps->ptr[datIdx];
}

int PathImport_PlaneReadTrapezoids(PathStaticData *staticData, PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 2, &subLength)) != 0)
        return err;

    size_t size = plane->trapezoids.len * 4 * 11;
    if (size != (size_t) subLength)
        return 1;

    for (size_t idx = 0; idx < plane->trapezoids.len; ++idx) {
        uint32_t trapId0 = le32dec(&data->ptr[4 * 0]);
        uint32_t trapId1 = le32dec(&data->ptr[4 * 1]);
        uint32_t trapId2 = le32dec(&data->ptr[4 * 2]);
        uint32_t trapId3 = le32dec(&data->ptr[4 * 3]);

        PathTrapezoid *trap = &plane->trapezoids.ptr[idx];

        trap->trap_id      = staticData->traps_count++;
        trap->top_left     = GetTrapHelper(&plane->trapezoids, trapId0);
        trap->top_right    = GetTrapHelper(&plane->trapezoids, trapId1);
        trap->bottom_left  = GetTrapHelper(&plane->trapezoids, trapId2);
        trap->bottom_right = GetTrapHelper(&plane->trapezoids, trapId3);
        trap->portal_left  = le16dec(&data->ptr[(4 * 4) + 0]);
        trap->portal_right = le16dec(&data->ptr[(4 * 4) + 2]);
        trap->yt           = le32decf(&data->ptr[4 * 5]);
        trap->yb           = le32decf(&data->ptr[4 * 6]);
        trap->xtl          = le32decf(&data->ptr[4 * 7]);
        trap->xtr          = le32decf(&data->ptr[4 * 8]);
        trap->xbl          = le32decf(&data->ptr[4 * 9]);
        trap->xbr          = le32decf(&data->ptr[4 * 10]);

        slice_advance(data, 4*11);
    }

    return 0;
}

int PathImport_PlaneReadVectors(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 1, &subLength)) != 0)
        return err;

    size_t size = sizeof(*plane->vectors.ptr) * plane->vectors.len;
    if (size != (size_t) subLength)
        return 1;

    memcpy(plane->vectors.ptr, data->ptr, size);
    slice_advance(data, subLength);
    return 0;
}

int PathImport_ReadRootNode(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 3, &subLength)) != 0)
        return err;

    if (subLength < 1)
        return 1;

    uint8_t rootType = data->ptr[0];
    slice_advance(data, 1);

    switch (rootType) {
    case NodeType_XNode:
        plane->root_node = &plane->xnodes.ptr[0];
        break;
    case NodeType_YNode:
        plane->root_node = &plane->ynodes.ptr[0];
        break;
    case NodeType_Sink:
        plane->root_node = &plane->sink_nodes.ptr[0];
        break;
    default:
        return 1;
    }

    return 0;
}

Node *GetNodeFromDatId(PathPlane *plane, uint32_t nodeId)
{
    if (nodeId < FIRST_Y_NODE_INDEX) {
        uint32_t nodeIdx = nodeId - FIRST_X_NODE_INDEX;
        if (nodeIdx < plane->xnodes.len) {
            return &plane->xnodes.ptr[nodeIdx];
        }
    } else if (nodeId < FIRST_SINK_NODE_INDEX) {
        uint32_t nodeIdx = nodeId - FIRST_Y_NODE_INDEX;
        if (nodeIdx < plane->ynodes.len) {
            return &plane->ynodes.ptr[nodeIdx];
        }
    } else {
        uint32_t nodeIdx = nodeId - FIRST_SINK_NODE_INDEX;
        if (nodeIdx < plane->sink_nodes.len) {
            return &plane->sink_nodes.ptr[nodeIdx];
        }
    }
    return NULL;
}

int PathImport_ReadXNodes(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 4, &subLength)) != 0)
        return err;

    if (subLength != plane->xnodes.len * 4 * 4)
        return 1;

    for (size_t idx = 0; idx < plane->xnodes.len; ++idx) {
        uint32_t pos1Index = le32dec(&data->ptr[0]);
        uint32_t pos2Index = le32dec(&data->ptr[4]);
        uint32_t leftNodeIndex = le32dec(&data->ptr[8]);
        uint32_t rightNodeIndex = le32dec(&data->ptr[12]);
        slice_advance(data, 16);

        plane->xnodes.ptr[idx].type = NodeType_XNode;
        XNode *xnode = &plane->xnodes.ptr[idx].xnode;
        xnode->pos = plane->vectors.ptr[pos1Index];
        Vec2f pos2 = plane->vectors.ptr[pos2Index];
        xnode->dir.x = pos2.x - xnode->pos.x;
        xnode->dir.y = pos2.y - xnode->pos.y;
        xnode->left = GetNodeFromDatId(plane, leftNodeIndex);
        xnode->right = GetNodeFromDatId(plane, rightNodeIndex);
    }

    return 0;
}

int PathImport_ReadYNodes(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 5, &subLength)) != 0)
        return err;

    if (subLength != plane->ynodes.len * 4 * 3)
        return 1;

    for (size_t idx = 0; idx < plane->ynodes.len; ++idx) {
        uint32_t posIndex = le32dec(&data->ptr[0]);
        uint32_t aboveNodeIndex = le32dec(&data->ptr[4]);
        uint32_t bellowNodeIndex = le32dec(&data->ptr[8]);
        slice_advance(data, 12);

        plane->ynodes.ptr[idx].type = NodeType_YNode;
        YNode *ynode = &plane->ynodes.ptr[idx].ynode;
        ynode->pos = plane->vectors.ptr[posIndex];
        ynode->above = GetNodeFromDatId(plane, aboveNodeIndex);
        ynode->bellow = GetNodeFromDatId(plane, bellowNodeIndex);
    }

    return 0;
}

int PathImport_ReadSinkNodes(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 6, &subLength)) != 0)
        return err;

    if (subLength != plane->sink_nodes.len * 4)
        return 1;

    for (size_t idx = 0; idx < plane->sink_nodes.len; ++idx) {
        uint32_t trapIdx = le32dec(&data->ptr[0]);
        slice_advance(data, 4);

        plane->sink_nodes.ptr[idx].type = NodeType_Sink;
        plane->sink_nodes.ptr[idx].sink_node.trap = &plane->trapezoids.ptr[trapIdx];
    }

    return 0;
}

int PathImport_ReadPortalTraps(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 10, &subLength)) != 0)
        return err;

    if (subLength != plane->portals_traps.len * 4)
        return 1;

    for (uint32_t idx = 0; idx < plane->portals_traps.len; ++idx) {
        uint32_t trapIdx = le32dec(&data->ptr[0]);
        if (trapIdx < plane->trapezoids.len)
            plane->portals_traps.ptr[idx] = &plane->trapezoids.ptr[trapIdx];
        slice_advance(data, 4);
    }

    return 0;
}

int PathImport_ReadPortals(PathDataImportContext *context, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 9, &subLength)) != 0)
        return err;

    PathPlane *plane = context->plane;
    if (subLength != plane->portals.len * 9)
        return 1;

    for (size_t idx = 0; idx < plane->portals.len; ++idx) {
        uint16_t trapsCount = le16dec(&data->ptr[0]);
        uint16_t trapsIdx = le16dec(&data->ptr[2]);
        uint16_t neighborPlaneId = le16dec(&data->ptr[4]);
        uint16_t neighborSharedId = le16dec(&data->ptr[6]);
        uint8_t  flags = data->ptr[8];

        Portal *portal = &plane->portals.ptr[idx];
        portal->portal_plane_id = plane->plane_id;
        portal->neighbor_plane_id = neighborPlaneId;
        portal->flags = flags;
        portal->traps_count = trapsCount;
        portal->traps = &plane->portals_traps.ptr[trapsIdx];

        PortalPairRef pair_ref;
        pair_ref.key = ((uint32_t)portal->portal_plane_id << 16) | (uint32_t)neighborSharedId;
        pair_ref.portal = portal;
        stbds_hmputs(context->portal_pairs, pair_ref);

        ptrdiff_t jdx;
        uint32_t key = ((uint32_t)portal->neighbor_plane_id << 16) | (uint32_t)neighborSharedId;
        if ((jdx = stbds_hmgeti(context->portal_pairs, key)) != -1) {
            PortalPairRef *ref = &context->portal_pairs[(size_t)jdx];
            assert(ref->portal && !ref->portal->pair);
            portal->pair = ref->portal;
            ref->portal->pair = portal;
        }

        slice_advance(data, 9);
    }

    return 0;
}

int PathImport_ReadPlane(PathDataImportContext *context, slice_uint8_t *data)
{
    int err;

    PathPlane *plane = array_push(&context->static_data->planes, 1);
    plane->plane_id = context->static_data->planes.len - 1;
    context->plane = plane;

    if ((err = PathImport_PlaneReadCounts(plane, data)) != 0 ||
        (err = PathImport_PlaneTag11(plane, data)) != 0 ||
        (err = PathImport_PlaneReadVectors(plane, data)) != 0 ||
        (err = PathImport_PlaneReadTrapezoids(context->static_data, plane, data)) != 0 ||
        (err = PathImport_ReadRootNode(plane, data)) != 0 ||
        (err = PathImport_ReadXNodes(plane, data)) != 0 ||
        (err = PathImport_ReadYNodes(plane, data)) != 0 ||
        (err = PathImport_ReadSinkNodes(plane, data)) != 0 ||
        (err = PathImport_ReadPortalTraps(plane, data)) != 0 ||
        (err = PathImport_ReadPortals(context, data)) != 0
    )
    {
        return err;
    }

    return 0;
}

int PathImport_ReadPlanes(PathStaticData *staticData, slice_uint8_t *data)
{
    int err;

    uint32_t subLength;
    if ((err = ParseTaggedChunk(data, 8, &subLength)) != 0)
        return err;

    if (data->len < 4)
        return 1;

    uint32_t planeCount = le32dec(data->ptr);
    slice_advance(data, 4);

    printf("subLength: %" PRIu32 "\n", subLength);
    printf("planeCount: %" PRIu32 "\n", planeCount);

    PathDataImportContext context = {0};
    context.static_data = staticData;

    array_reserve(&staticData->planes, planeCount);

    for (uint32_t idx = 0; idx < planeCount; ++idx) {
        printf("Plane %" PRIu32 "\n", idx);
        if ((err = PathImport_ReadPlane(&context, data)) != 0)
            goto cleanup;
    }

cleanup:
    stbds_hmfree(context.portal_pairs);
    return err;
}

int PathDataImport(PathStaticData *staticData, slice_uint8_t data)
{
    PathImport_CheckHeader(&data);
    PathImport_Step2(&data);
    PathImport_ReadPlanes(staticData, &data);
    return 0;
}
