#pragma once

#define RIFF_SIGNATURE "ffna"

#define PATH_DATA_SIGNATURE     (0xEEFE704C)
#define PATH_DATA_VERSION       (12)
#define PATH_SEQUENCE_FAST_SAVE (-1)

#define FIRST_X_NODE_INDEX    0
#define FIRST_Y_NODE_INDEX    0x40000000
#define FIRST_SINK_NODE_INDEX 0x80000000

#define TYPE_SHIFT  24
#define STAGE_SHIFT 28

typedef struct PortalPairRef {
    uint32_t key;
    Portal *portal;
} PortalPairRef;

typedef struct PathDataImportContext {
    PathPlane      *plane;
    PathStaticData *static_data;
    PortalPairRef  *portal_pairs;
} PathDataImportContext;

bool RiffCheckHeader(const uint8_t *data, size_t size)
{
    return (5 <= size && memcmp(data, RIFF_SIGNATURE, 4) == 0 && data[4] < RIFF_TYPES);
}

RiffType RiffParseAndGetType(const uint8_t *data, size_t size)
{
    if (!RiffCheckHeader(data, size))
        return RIFF_NONE;
    return (RiffType) data[4];
}

int qsort_compare_map_chunk(const void *p1, const void *p2)
{
    MapChunk *chunk1 = (MapChunk *) p1;
    MapChunk *chunk2 = (MapChunk *) p2;
    if (chunk1->chunk_id < chunk2->chunk_id) {
        return -1;
    } else if (chunk1->chunk_id > chunk2->chunk_id) {
        return ERR_UNSUCCESSFUL;
    } else {
        return 0;
    }
}

int RiffParseChunks(const uint8_t *data, size_t size, MapChunkArray *result)
{
    if (!RiffCheckHeader(data, size))
        return ERR_UNSUCCESSFUL;

    data += 5;
    size -= 5;

    while (8 <= size) {
        MapChunk chunk = {0};
        chunk.chunk_id = le32dec(&data[0]);
        chunk.chunk_size = le32dec(&data[4]);
        if (size < 8 + chunk.chunk_size) {
            return ERR_UNSUCCESSFUL;
        }
        chunk.chunk_data.ptr = &data[8];
        chunk.chunk_data.len = chunk.chunk_size;
        array_add(result, chunk);

        size = size - chunk.chunk_size - 8;
        data = data + chunk.chunk_size + 8;
    }

    qsort(result->ptr, result->len, sizeof(*result->ptr), qsort_compare_map_chunk);
    return 0;
}

MapChunk* RiffGetChunk(MapChunkArray *chunks, uint32_t chunk_id)
{
    size_t left = 0;
    size_t right = chunks->len - 1;
    while (left <= right) {
        size_t half = left + ((right - left) / 2);
        uint32_t half_chunk_id = chunks->ptr[half].chunk_id;
        if (half_chunk_id < chunk_id) {
            left = half + 1;
        } else if (half_chunk_id > chunk_id) {
            right = half - 1;
        } else {
            return &chunks->ptr[half];
        }
    }
    return NULL;
}

int PathImport_CheckHeader(slice_uint8_t *data)
{
    if (data->len < 12) {
        return ERR_UNSUCCESSFUL;
    }

    uint32_t signature = le32dec(&data->ptr[0]);
    uint32_t version = le32dec(&data->ptr[4]);
    uint32_t sequence = le32dec(&data->ptr[8]);

    assert(sequence != PATH_SEQUENCE_FAST_SAVE);
    if (signature != PATH_DATA_SIGNATURE || version != PATH_DATA_VERSION)
        return ERR_UNSUCCESSFUL;

    slice_advance(data, 12);
    return 0;
}

int PathImport_Step2(slice_uint8_t *data)
{
    if (data->len <= 5) {
        return ERR_UNSUCCESSFUL;
    }

    uint32_t size = le32dec(&data->ptr[1]);
    slice_advance(data, 5);

    if (data->len < size) {
        return ERR_UNSUCCESSFUL;
    }

    slice_advance(data, size);
    return 0;
}

int ParseTaggedChunk(slice_uint8_t *data, uint8_t tag, uint32_t *outLength)
{
    if (data->len < 5)
        return ERR_UNSUCCESSFUL;
    if (data->ptr[0] != tag)
        return ERR_UNSUCCESSFUL;
    uint32_t length = le32dec(&data->ptr[1]);
    slice_advance(data, 5);
    if (data->len < length)
        return ERR_UNSUCCESSFUL;
    *outLength = length;
    return 0;
}

int PathImport_PlaneReadCounts(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 0, &sub_length)) != 0)
        return err;

    if (sub_length < 32) {
        return ERR_UNSUCCESSFUL;
    }

    uint32_t h000C = le32dec(&data->ptr[0]);
    uint32_t vector_count = le32dec(&data->ptr[4]);
    uint32_t traps_count = le32dec(&data->ptr[8]);
    uint32_t xnode_count = le32dec(&data->ptr[12]);
    uint32_t ynode_count = le32dec(&data->ptr[16]);
    uint32_t sink_node_count = le32dec(&data->ptr[20]);
    uint32_t portal_count = le32dec(&data->ptr[24]);
    uint32_t portal_traps_count = le32dec(&data->ptr[28]);

    array_resize(&plane->vectors, vector_count);
    array_resize(&plane->trapezoids, traps_count);
    array_resize(&plane->xnodes, xnode_count);
    array_resize(&plane->ynodes, ynode_count);
    array_resize(&plane->sink_nodes, sink_node_count);
    array_resize(&plane->portals, portal_count);
    array_resize(&plane->portals_traps, portal_traps_count);

    plane->h000C = h000C;
    slice_advance(data, 32);
    return 0;
}

int PathImport_PlaneTag11(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 11, &sub_length)) != 0)
        return err;

    assert(plane->h000C * 8 <= sub_length);
    slice_advance(data, plane->h000C * 8);
    return 0;
}

PathTrapezoid *GetTrapHelper(PathTrapezoidArray *traps, uint32_t dat_idx)
{
    if (dat_idx == UINT32_MAX)
        return NULL;
    assert(dat_idx < traps->len);
    return &traps->ptr[dat_idx];
}

int PathImport_PlaneReadTrapezoids(PathStaticData *static_data, PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 2, &sub_length)) != 0)
        return err;

    size_t size = plane->trapezoids.len * 4 * 11;
    if (size != (size_t) sub_length)
        return ERR_UNSUCCESSFUL;

    for (size_t idx = 0; idx < plane->trapezoids.len; ++idx) {
        uint32_t trapId0 = le32dec(&data->ptr[4 * 0]);
        uint32_t trapId1 = le32dec(&data->ptr[4 * 1]);
        uint32_t trapId2 = le32dec(&data->ptr[4 * 2]);
        uint32_t trapId3 = le32dec(&data->ptr[4 * 3]);

        PathTrapezoid *trap = &plane->trapezoids.ptr[idx];

        trap->trap_id      = static_data->traps_count++;
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

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 1, &sub_length)) != 0)
        return err;

    size_t size = sizeof(*plane->vectors.ptr) * plane->vectors.len;
    if (size != (size_t) sub_length)
        return ERR_UNSUCCESSFUL;

    memcpy(plane->vectors.ptr, data->ptr, size);
    slice_advance(data, sub_length);
    return 0;
}

int PathImport_ReadRootNode(PathPlane *plane, slice_uint8_t *data)
{
    int err;

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 3, &sub_length)) != 0)
        return err;

    if (sub_length < 1)
        return ERR_UNSUCCESSFUL;


    uint8_t root_type = data->ptr[0];
    slice_advance(data, 1);

    switch (root_type) {
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
        return ERR_UNSUCCESSFUL;
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

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 4, &sub_length)) != 0)
        return err;

    if (sub_length != plane->xnodes.len * 4 * 4)
        return ERR_UNSUCCESSFUL;

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

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 5, &sub_length)) != 0)
        return err;

    if (sub_length != plane->ynodes.len * 4 * 3)
        return ERR_UNSUCCESSFUL;

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

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 6, &sub_length)) != 0)
        return err;

    if (sub_length != plane->sink_nodes.len * 4)
        return ERR_UNSUCCESSFUL;

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

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 10, &sub_length)) != 0)
        return err;

    if (sub_length != plane->portals_traps.len * 4)
        return ERR_UNSUCCESSFUL;

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

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 9, &sub_length)) != 0)
        return err;

    PathPlane *plane = context->plane;
    if (sub_length != plane->portals.len * 9)
        return ERR_UNSUCCESSFUL;

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
    plane->plane_id = (uint16_t) (context->static_data->planes.len - 1);
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

int PathImport_ReadPlanes(PathStaticData *static_data, slice_uint8_t *data)
{
    int err;

    uint32_t sub_length;
    if ((err = ParseTaggedChunk(data, 8, &sub_length)) != 0)
        return err;

    if (data->len < 4)
        return ERR_UNSUCCESSFUL;

    uint32_t planeCount = le32dec(data->ptr);
    slice_advance(data, 4);

    PathDataImportContext context = {0};
    context.static_data = static_data;

    array_reserve(&static_data->planes, planeCount);

    for (uint32_t idx = 0; idx < planeCount; ++idx) {
        if ((err = PathImport_ReadPlane(&context, data)) != 0)
            goto cleanup;
    }

cleanup:
    stbds_hmfree(context.portal_pairs);
    return err;
}

int PathDataImport(PathStaticData *static_data, slice_uint8_t data)
{
    int err;

    if ((err = PathImport_CheckHeader(&data)) != 0 ||
        (err = PathImport_Step2(&data)) != 0 ||
        (err = PathImport_ReadPlanes(static_data, &data)) != 0)
    {
        return err;
    }

    return 0;
}

void GmMapContext_Free(GmMapContext *context)
{
    PathStaticData *static_data = &context->path.static_data;
    for (size_t idx = 0; idx < static_data->planes.len; ++idx) {
        PathPlane *plane = &static_data->planes.ptr[idx];
        array_free(&plane->vectors);
        array_free(&plane->trapezoids);
        array_free(&plane->sink_nodes);
        array_free(&plane->xnodes);
        array_free(&plane->ynodes);
        array_free(&plane->portals);
        array_free(&plane->portals_traps);
    }

    array_free(&static_data->planes);
    array_free(&context->path.nodes);
    array_free(&context->path.prioq);
    array_free(&context->path.steps);
}

int GmImportMap(GmMapContext *context, slice_uint8_t content)
{
    int err;

    RiffType riff_type = RiffParseAndGetType(content.ptr, content.len);
    if (riff_type != RIFF_MAP1 && riff_type != RIFF_MAP2) {
        log_error("Invalid riff type %d", (int) riff_type);
        return ERR_UNSUCCESSFUL;
    }

    MapChunkArray chunks = {0};
    if ((err = RiffParseChunks(content.ptr, content.len, &chunks)) != 0)
        return err;

    MapChunk *chunk;

    uint8_t stage = 2;
    uint32_t chunk_id = ((uint32_t)stage << STAGE_SHIFT) | (uint32_t) ChunkId_Path;

    if ((chunk = RiffGetChunk(&chunks, chunk_id)) == NULL) {
        err = ERR_UNSUCCESSFUL;
        goto cleanup;
    }

    if ((err = PathDataImport(&context->path.static_data, chunk->chunk_data)) != 0) {
        goto cleanup;
    }

    /*
    chunk_id = ((uint32_t) stage << STAGE_SHIFT) | (uint32_t) ChunkId_MapParameters;
    if ((chunk = RiffGetChunk(&chunks, chunk_id)) == NULL) {
        err = ERR_UNSUCCESSFUL;
        goto cleanup;
    }

    MapParams params = {0};
    if ((err = MapParamsImport(&params, chunk->chunk_data)) != 0)
        return err;
    */

cleanup:
    array_free(&chunks);
    return err;
}
