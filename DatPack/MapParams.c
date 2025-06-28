#define MAP_PARAMS_SIGNATURE 0x5943EEEF
#define MAP_PARAMS_VERSION   2

typedef struct MapParams {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
} MapParams;

int MapParamsImport_ConsumeHeader(slice_uint8_t *data)
{
    if (data->len < 5)
        return 1;
    if (le32dec(data->ptr) != MAP_PARAMS_SIGNATURE)
        return 1;
    if (data->ptr[4] != MAP_PARAMS_VERSION)
        return 1;
    slice_advance(data, 5);
    return 0;
}

int MapParamsImport(MapParams *params, slice_uint8_t data)
{
    int err;
    if ((err = MapParamsImport_ConsumeHeader(&data)) != 0)
        return err;

    params->min_x = le32decf(&data.ptr[0]);
    params->min_y = le32decf(&data.ptr[4]);
    params->max_x = le32decf(&data.ptr[8]);
    params->max_y = le32decf(&data.ptr[12]);

    return 0;
}
