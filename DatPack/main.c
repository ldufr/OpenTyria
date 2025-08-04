void MyWriteFile(const char *dir, int idx, const uint8_t* data, size_t len)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%d", dir, idx);
    FILE *fout = fopen(filename, "wb");
    if (fout)
    {
        if (fwrite(data, len, 1, fout) != 1) {
            fprintf(stderr, "Failed to write out file %d\n", idx);
        }
        fclose(fout);
    }
}

int exec_subcommand_toc(FileArchive *archive)
{
    for (size_t idx = 0; idx < archive->fileNames.len; ++idx) {
        MFTFileName filename = archive->fileNames.ptr[idx];
        printf("MFTFileName[%zu].FileId    = %" PRIu32 " (0x%" PRIX32 ")\n", idx, filename.FileId, filename.FileId);
        printf("MFTFileName[%zu].FileIndex = %" PRIu32 " (0x%" PRIX32 ")\n", idx, filename.FileIndex, filename.FileIndex);
        printf("\n");
    }

    return 0;
}

int exec_subcommand_list(FileArchive *archive)
{
    MFTEntryArray entries = archive->fileEntries;
    for (int idx = 0; idx < entries.len; ++idx) {
        MFTEntry ME = entries.ptr[idx];

        const char *flagsStr = "<no flags>";
        if (ME.flags == (FLAG_ENTRY_USED | FLAG_FIRST_STREAM)) {
            flagsStr = "FLAG_ENTRY_USED | FLAG_FIRST_STREAM";
        } else if (ME.flags == FLAG_ENTRY_USED) {
            flagsStr = "FLAG_ENTRY_USED";
        } else if (ME.flags == FLAG_FIRST_STREAM) {
            flagsStr = "FLAG_FIRST_STREAM";
        }

        printf("MFTEntry[%2d].offset     = %" PRIu64 " (0x%" PRIX64 ")\n", idx, ME.offset, ME.offset);
        printf("MFTEntry[%2d].size       = %" PRIu32 " (0x%" PRIX32 ")\n", idx, ME.size, ME.size);
        printf("MFTEntry[%2d].compressed = %" PRIu16 " (0x%" PRIX16 ")\n", idx, ME.compressed, ME.compressed);
        printf("MFTEntry[%2d].flags      = %" PRIu8  " (0x%" PRIX8 ") | %s\n", idx, ME.flags, ME.flags, flagsStr);
        printf("MFTEntry[%2d].stream     = %" PRIu8  " (0x%" PRIX8 ")\n", idx, ME.stream, ME.stream);
        printf("MFTEntry[%2d].nextStream = %" PRIu32 " (0x%" PRIX32 ")\n", idx, ME.nextStream, ME.nextStream);
        printf("MFTEntry[%2d].checksum   = %" PRIu32 " (0x%" PRIX32 ")\n", idx, ME.checksum, ME.checksum);
        printf("\n");
    }

    return 0;
}

int exec_subcommand_extract(FileArchive *archive)
{
    int err;

    array_uint8_t fileContent = {0};
    MFTEntryArray entries = archive->fileEntries;
    for (int idx = 0; idx < entries.len; ++idx) {
        MFTEntry ME = entries.ptr[idx];

        if (options.verbose) {
            printf("MFTEntry[%2d].offset     = %" PRIu64 " (0x%" PRIX64 ")\n", idx, ME.offset, ME.offset);
            printf("MFTEntry[%2d].size       = %" PRIu32 " (0x%" PRIX32 ")\n", idx, ME.size, ME.size);
            printf("MFTEntry[%2d].compressed = %" PRIu16 " (0x%" PRIX16 ")\n", idx, ME.compressed, ME.compressed);
            printf("MFTEntry[%2d].flags      = %" PRIu8  " (0x%" PRIX8 ")\n", idx, ME.flags, ME.flags);
            printf("MFTEntry[%2d].stream     = %" PRIu8  " (0x%" PRIX8 ")\n", idx, ME.stream, ME.stream);
            printf("MFTEntry[%2d].nextStream = %" PRIu32 " (0x%" PRIX32 ")\n", idx, ME.nextStream, ME.nextStream);
            printf("MFTEntry[%2d].checksum   = %" PRIu32 " (0x%" PRIX32 ")\n", idx, ME.checksum, ME.checksum);
        }

        if (ME.size == 0 || ME.offset == 0)
            continue;

        array_clear(&fileContent);
        uint8_t *buffer = array_push(&fileContent, ME.size);

        if ((err = archive_read_at(archive, ME.offset, buffer, ME.size)) != 0) {
            fprintf(stderr, "[%d] Failed to read the compressed data\n", idx);
            continue;
        }

        if (ME.compressed && 4 <= fileContent.len) {
            uint32_t decompressed_len = le32dec(&buffer[fileContent.len - 4]);

            if (options.verbose) {
                printf("[%d] decompressed_len: %" PRIu32 " (0x%" PRIX32 ")\n", idx, decompressed_len, decompressed_len);
            }

            array_uint8_t result = {0};
            if (!DecompressFile(fileContent.ptr, fileContent.len - 4, decompressed_len, &result)) {
                fprintf(stderr, "[%d] Failed to decompress file\n", idx);
                array_free(&result);
                continue;
            }

            array_free(&fileContent);
            fileContent = result;
        }

        MyWriteFile(options.extract_dir, idx, fileContent.ptr, fileContent.len);
    }

    return 0;
}

int exec_subcommand_find(FileArchive *archive)
{
    int err;

    assert(options.file_id.set);
    uint32_t fileIdx;
    if (!archive_find_file_by_id(archive, options.file_id.val, &fileIdx)) {
        fprintf(stderr, "Couldn't find the file for file id %" PRIu32 "\n", options.file_id.val);
        return 1;
    }

    printf("File index is: %" PRIu32 "\n", fileIdx);

    array_uint8_t fileContent = {0};
    if ((err = archive_read_file(archive, fileIdx, &fileContent)) != 0)
        return err;

    RiffType riffType = RiffParseAndGetType(fileContent.ptr, fileContent.len);
    if (riffType != RIFF_MAP1 && riffType != RIFF_MAP2) {
        printf("Invalid riff type\n");
        return 1;
    }

    MapChunkArray chunks = {0};
    if ((err = RiffParseChunks(fileContent.ptr, fileContent.len, &chunks)) != 0)
        return err;

    uint8_t stage = 2;
    uint32_t chunkId = ((uint32_t)stage << STAGE_SHIFT) | (uint32_t) ChunkId_Path;
    MapChunk *chunk = RiffGetChunk(&chunks, chunkId);
    printf("%p:%zu\n", chunk->chunkData.ptr, chunk->chunkData.len);

    PathContext context = {0};
    PathDataImport(&context.static_data, chunk->chunkData);

    chunkId = ((uint32_t)stage << STAGE_SHIFT) | (uint32_t) ChunkId_MapParameters;
    chunk = RiffGetChunk(&chunks, chunkId);
    printf("%p:%zu\n", chunk->chunkData.ptr, chunk->chunkData.len);

    MapParams params = {0};
    if ((err = MapParamsImport(&params, chunk->chunkData)) != 0)
        return err;

    float width = params.max_x - params.min_x;
    float height = params.max_y - params.min_y;
    float win_height = 920;
    float win_width = (width / height) * win_height;

    float tx = win_width / width;
    float ty = win_height / height;
    float ox = -params.min_x;
    float oy = -params.min_y;

    printf("%f, %f\n", width, height);

#if 0
    WaypointArray waypoints = {0};
    GmPos gmStartPos = (GmPos) { -5706.0845, 9395.6445, 0 };
    GmPos gmEndPos   = (GmPos) { -7621.4482, 9156.0986, 0 };

    array_clear(&waypoints);
    if (!PathFinding(&context, gmStartPos, gmEndPos, &waypoints)) {
        printf("PathFinding failed\n");
    }
#else
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(win_width, win_height, "trapezoids");
    Font fontBm = LoadFontEx("C:\\Windows\\Fonts\\consola.ttf", 20, 0, 250);

    Vector2 startPos = GetMousePosition();

    int     nextSelect = 0;
    GmPos   gmStartPos = {0};
    GmPos   gmEndPos = {0};
    WaypointArray waypoints = {0};

    while (!WindowShouldClose())
    {
        Vector2 mousePos = GetMousePosition();

        BeginDrawing();
        ClearBackground(SKYBLUE);

    #if 0
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (nextSelect == 0) {
                gmStartPos.plane = 0;
                gmStartPos.x = (mousePos.x / tx) - ox;
                gmStartPos.y = ((win_height - mousePos.y) / ty) - oy;
                printf("Start pos %f, %f\n", gmStartPos.x, gmStartPos.y);
            } else {
                gmEndPos.plane = 0;
                gmEndPos.x = (mousePos.x / tx) - ox;
                gmEndPos.y = ((win_height - mousePos.y) / ty) - oy;
                printf("End pos %f, %f\n", gmEndPos.x, gmEndPos.y);
            }

            nextSelect ^= 1;

            // gmStartPos = (GmPos) { -425.2458, -1599.9950, 0 };
            // gmEndPos   = (GmPos) { 899.0714, -1193.5079, 0 };

            array_clear(&waypoints);
            if (!PathFinding(&context, gmStartPos, gmEndPos, &waypoints)) {
                printf("PathFinding failed\n");
            }
        }
    #else
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            gmStartPos.plane = 0;
            gmStartPos.x = (mousePos.x / tx) - ox;
            gmStartPos.y = ((win_height - mousePos.y) / ty) - oy;
            printf("Start pos %f, %f\n", gmStartPos.x, gmStartPos.y);
        }

        gmEndPos.plane = 0;
        gmEndPos.x = (mousePos.x / tx) - ox;
        gmEndPos.y = ((win_height - mousePos.y) / ty) - oy;

        array_clear(&waypoints);
        if (!PathFinding(&context, gmStartPos, gmEndPos, &waypoints)) {
            // printf("PathFinding failed\n");
        }
    #endif

        for (size_t planeIdx = 0; planeIdx < context.static_data.planes.len; ++planeIdx) {
            PathPlane *plane = &context.static_data.planes.ptr[planeIdx];
            for (size_t trapIdx = 0; trapIdx < plane->trapezoids.len; ++trapIdx) {
                PathTrapezoid *trap = &plane->trapezoids.ptr[trapIdx];
                Vector2 v1 = { tx * (trap->xbl + ox), win_height - (ty * (trap->yb + oy)) };
                Vector2 v2 = { tx * (trap->xtl + ox), win_height - (ty * (trap->yt + oy)) };
                Vector2 v3 = { tx * (trap->xtr + ox), win_height - (ty * (trap->yt + oy)) };
                Vector2 v4 = { tx * (trap->xbr + ox), win_height - (ty * (trap->yb + oy)) };
                DrawTriangle(v3, v2, v1, RAYWHITE);
                DrawTriangle(v4, v3, v1, RAYWHITE);
            }
        }

#if 0
        for (size_t idx = 1; idx < waypoints.len; ++idx) {
            Waypoint prev = waypoints.ptr[idx - 1];
            Waypoint curr = waypoints.ptr[idx];

            Vector2 p1 = { tx * (prev.pos.x + ox), win_height - ty * (prev.pos.y + oy) };
            Vector2 p2 = { tx * (curr.pos.x + ox), win_height - ty * (curr.pos.y + oy) };
            DrawLineV(p1, p2, BLUE);
        }
#else
        for (size_t idx = 0; idx < waypoints.len; ++idx) {
            Waypoint wp = waypoints.ptr[idx];

            Vector2 p1 = { tx * (wp.pos.x + ox), win_height - (ty * (wp.pos.y + oy)) };
            DrawCircle(p1.x, p1.y, 3, BLUE);
        }
#endif

        float x = (gmStartPos.x + ox) * tx;
        float y = (gmStartPos.y + oy) * ty;
        DrawCircle(x, win_height - y, 2.f, GREEN);

        x = (gmEndPos.x + ox) * tx;
        y = (gmEndPos.y + oy) * ty;
        DrawCircle(x, win_height - y, 2.f, ORANGE);

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Start: (%.3f, %.3f)", gmStartPos.x, gmStartPos.y);
        DrawTextEx(fontBm, buffer, (Vector2) { 10, 10 }, (float)fontBm.baseSize, 2, BLACK);
        snprintf(buffer, sizeof(buffer), "End:   (%.3f, %.3f)", gmEndPos.x, gmEndPos.y);
        DrawTextEx(fontBm, buffer, (Vector2) { 10, 10 + fontBm.baseSize }, (float)fontBm.baseSize, 2, BLACK);

        EndDrawing();
    }
#endif

    return 0;
}

int main(int argc, char **argv)
{
    int err;

    parse_command_args(argc, argv);

    FileArchive archive = {0};
    if ((err = archive_open(&archive, options.archive)) != 0) {
        fprintf(stderr, "Failed to open '%s'\n", options.archive);
        return 1;
    }

    switch (options.subcommand) {
    case SubCommand_Toc:
        return exec_subcommand_toc(&archive);
    case SubCommand_List:
        return exec_subcommand_list(&archive);
    case SubCommand_Extract:
        return exec_subcommand_extract(&archive);
    case SubCommand_Find:
        return exec_subcommand_find(&archive);
    default:
        return 1;
    }

    return 0;
}
