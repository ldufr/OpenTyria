#define RIFF_SIGNATURE "ffna"

typedef enum RiffType {
    RIFF_NONE  = 0,
    RIFF_MAP1  = 1,
    RIFF_MAP2  = 3,
    RIFF_TYPES = 9,
} RiffType;

typedef struct MapChunk {
    uint32_t      chunkId;
    uint32_t      chunkSize;
    slice_uint8_t chunkData;
} MapChunk;
typedef array(MapChunk) MapChunkArray;

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
    if (chunk1->chunkId < chunk2->chunkId) {
        return -1;
    } else if (chunk1->chunkId > chunk2->chunkId) {
        return 1;
    } else {
        return 0;
    }
}

int RiffParseChunks(const uint8_t *data, size_t size, MapChunkArray *result)
{
    if (!RiffCheckHeader(data, size))
        return 1;

    data += 5;
    size -= 5;

    while (8 <= size) {
        MapChunk chunk = {0};
        chunk.chunkId = le32dec(&data[0]);
        chunk.chunkSize = le32dec(&data[4]);
        if (size < 8 + chunk.chunkSize) {
            return 1;
        }
        chunk.chunkData.ptr = &data[8];
        chunk.chunkData.len = chunk.chunkSize;
        array_add(result, chunk);

        size = size - chunk.chunkSize - 8;
        data = data + chunk.chunkSize + 8;
    }

    qsort(result->ptr, result->len, sizeof(*result->ptr), qsort_compare_map_chunk);

    return 0;
}

MapChunk* RiffGetChunk(MapChunkArray *chunks, uint32_t chunkId)
{
    size_t left = 0;
    size_t right = chunks->len - 1;
    while (left <= right) {
        size_t half = left + ((right - left) / 2);
        uint32_t halfChunkId = chunks->ptr[half].chunkId;
        if (halfChunkId < chunkId) {
            left = half + 1;
        } else if (halfChunkId > chunkId) {
            right = half - 1;
        } else {
            return &chunks->ptr[half];
        }
    }
    return NULL;
}
