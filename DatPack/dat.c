#define DAT_MAGIC "3AN\x1A"

#define TYPE_SHIFT  24
#define STAGE_SHIFT 28

static const char *s_typeText[] = {
    "Data",
    "Dependencies"
};

typedef enum MapType {
    MAP_TYPE_EDIT = 1,
    MAP_TYPES = 4,
} MapType;

typedef enum MapStage {
    MAP_STAGE_RAW,
    MAP_STAGE_STRIP,
    MAP_STAGE_BLOATED,
    MAP_STAGES
} Stage;

static const char *s_stageText[] = {
    "Raw",
    "Stripped",
    "Bloated",
};

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

typedef struct DatHeader {
    uint8_t  Magic[4];
    uint32_t HeaderSize;
    uint32_t BlockSize;
    uint32_t checksum;
    uint64_t MFTOffset;
    uint32_t MFTSize;
    uint32_t Flags;
} DatHeader;

typedef struct MFTHeader {
    uint32_t fileId;
    uint32_t h0004;
    uint32_t h0008;
    uint32_t entryCount;
    uint32_t h0010;
    uint32_t h0014;
} MFTHeader;

#define INDEX_DESCRIPTOR 0
#define INDEX_TOC        1
#define INDEX_MFT        3
#define INDEX_FIRST_FILE 0x10

#define FLAG_ENTRY_USED   1
#define FLAG_FIRST_STREAM 2

typedef struct MFTEntry {
    uint64_t offset;
    uint32_t size;
    uint16_t compressed; // 0=stored, 8=compress (maybe compress level)
    uint8_t  flags;
    uint8_t  stream;
    uint32_t nextStream;
    uint32_t checksum;
} MFTEntry;
static_assert(sizeof(MFTEntry) == 24, "sizeof(MFTEntry) == 24");
typedef array(MFTEntry) MFTEntryArray;

typedef struct MFTFileName {
    uint32_t FileId;
    uint32_t FileIndex;
} MFTFileName;
typedef array(MFTFileName) MFTFileNameArray;

typedef struct FileArchive {
    FILE            *handle;
    DatHeader        header;
    MFTHeader        fileHeader;
    MFTEntryArray    fileEntries;
    MFTFileNameArray fileNames;
} FileArchive;

int check_header_magic(uint8_t *magic)
{
    if (memcmp(magic, "3AN\x1A", 4) == 0)
        return 0;
    return ERR_BAD_USER_DATA;
}

int read_header(FILE *fp, DatHeader *result)
{
    int err;

    uint8_t buffer[sizeof(*result)];
    if ((err = fread(buffer, sizeof(buffer), 1, fp)) != 1) {
        return ERR_BAD_USER_DATA;
    }

    slice_uint8_t slice = { buffer, sizeof(buffer) };
    if ((err = read_bytes(&slice, result->Magic, sizeof(result->Magic))) != 0 ||
        (err = check_header_magic(result->Magic)) != 0 ||
        (err = read_le_uint32(&slice, &result->HeaderSize)) != 0 ||
        (err = read_le_uint32(&slice, &result->BlockSize)) != 0 ||
        (err = read_le_uint32(&slice, &result->checksum)) != 0 ||
        (err = read_le_uint64(&slice, &result->MFTOffset)) != 0 ||
        (err = read_le_uint32(&slice, &result->MFTSize)) != 0 ||
        (err = read_le_uint32(&slice, &result->Flags)) != 0
    ) {
        return ERR_BAD_USER_DATA;
    }

    assert(slice.len == 0);
    return 0;
}

#ifdef _WIN32
int file_seek(FILE *fp, int64_t offset, int origin)
{
    return _fseeki64(fp, offset, origin);
}
#else
int file_seek(FILE *fp, int64_t offset, int origin)
{
    return fseek(fp, offset, origin);
}
#endif

int archive_read(FileArchive *archive, void* buffer, size_t size)
{
    if (fread(buffer, size, 1, archive->handle) != 1) {
        fprintf(stderr, "Failed to read %zu bytes\n", size);
        return 1;
    }
    return 0;
}

int archive_read_at(FileArchive *archive, int64_t offset, void* buffer, size_t size)
{
    if (file_seek(archive->handle, offset, SEEK_SET) == -1) {
        fprintf(stderr, "Failed to move the file cursor to (%" PRIi64 ")\n", offset);
        return 1;
    }

    if (fread(buffer, size, 1, archive->handle) != 1) {
        fprintf(stderr, "Failed to read %zu bytes at offset %" PRIi64 "\n", size, offset);
        return 1;
    }

    return 0;
}

int qsort_compare_filename(const void *p1, const void *p2)
{
    const MFTFileName *file1 = p1;
    const MFTFileName *file2 = p2;
    if (file1->FileId < file2->FileId)
        return -1;
    else if (file1->FileId > file2->FileId)
        return 1;
    else
        return 0;
}

int archive_open(FileArchive *archive, const char *path)
{
    int err;

    if ((archive->handle = fopen(options.archive, "rb")) == NULL) {
        fprintf(stderr, "Failed to open '%s'\n", options.archive);
        return 1;
    }

    if ((err = read_header(archive->handle, &archive->header)) != 0) {
        return err;
    }

    if ((err = archive_read_at(archive, archive->header.MFTOffset, &archive->fileHeader, sizeof(archive->fileHeader))) != 0) {
        return err;
    }

    size_t fileCount = archive->fileHeader.entryCount;
    MFTEntry* entries = array_push(&archive->fileEntries, fileCount);
    if ((err = archive_read(archive, entries, fileCount * sizeof(*entries))) != 0) {
        return err;
    }

    MFTEntry toc = entries[INDEX_TOC];
    size_t count = toc.size / sizeof(MFTFileName);
    MFTFileName *filenames = array_push(&archive->fileNames, count);
    if ((err = archive_read_at(archive, toc.offset, filenames, toc.size)) != 0) {
        return err;
    }

    qsort(filenames, count, sizeof(MFTFileName), qsort_compare_filename);
    return 0;
}

bool archive_find_file_by_id(FileArchive *archive, uint32_t fileId, uint32_t *fileIdx)
{
    MFTFileName *filename = archive->fileNames.ptr;
    size_t left = 0;
    size_t right = archive->fileNames.len - 1;

    while (left <= right) {
        size_t halfIdx = left + (right - left) / 2;
        if (filename[halfIdx].FileId < fileId) {
            left = halfIdx + 1;
        } else if (filename[halfIdx].FileId > fileId) {
            right = halfIdx - 1;
        } else {
            *fileIdx = filename[halfIdx].FileIndex - 1;
            return true;
        }
    }

    return false;
}

int archive_read_file(FileArchive *archive, uint32_t fileIdx, array_uint8_t *fileContent)
{
    int err;

    if (archive->fileEntries.len <= fileIdx) {
        return 1;
    }

    MFTEntry fileEntry = archive->fileEntries.ptr[fileIdx];

    array_clear(fileContent);
    uint8_t *buffer = array_push(fileContent, fileEntry.size);
    if ((err = archive_read_at(archive, fileEntry.offset, buffer, fileEntry.size)) != 0) {
        fprintf(stderr, "[file:%d] Failed to read the compressed data\n", fileIdx);
        return err;
    }

    if (fileEntry.compressed) {
        if (fileContent->len < 4) {
            fprintf(stderr, "[file:%d] Not enough bytes to get the decrypted file size\n", fileIdx);
            return 1;
        }

        uint32_t decompressedSize = le32dec(&buffer[fileContent->len - 4]);
        array_uint8_t decompressedContent = {0};
        if (!DecompressFile(fileContent->ptr, fileContent->len - 4, decompressedSize, &decompressedContent)) {
            fprintf(stderr, "[%d] Failed to decompress file\n", fileIdx);
            array_free(&decompressedContent);
            return 1;
        }

        array_free(fileContent);
        *fileContent = decompressedContent;
    }

    return 0;
}
