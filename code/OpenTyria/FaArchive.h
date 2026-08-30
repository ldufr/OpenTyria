#pragma once

#define FILE_ARCHIVE_MAGIC ((uint32_t)'\x1ANA3')
#define FILE_ARCHIVE_TYPE_SHIFT  24
#define FILE_ARCHIVE_STAGE_SHIFT 28

#define INDEX_DESCRIPTOR 0
#define INDEX_TOC        1
#define INDEX_MFT        3
#define INDEX_FIRST_FILE 0x10

#define FLAG_ENTRY_USED   1
#define FLAG_FIRST_STREAM 2

#pragma pack(push, 1)
typedef struct FaHeader {
    uint32_t magic;
    uint32_t header_size;
    uint32_t block_size;
    uint32_t checksum;
    uint64_t mft_offset;
    uint32_t mft_size;
    uint32_t flags;
} FaHeader;

typedef struct MFTHeader {
    uint32_t file_id;
    uint32_t h0004;
    uint32_t h0008;
    uint32_t entry_count;
    uint32_t h0010;
    uint32_t h0014;
} MFTHeader;

typedef struct MFTEntry {
    uint64_t offset;
    uint32_t size;
    uint16_t compressed; // 0=stored, 8=compress (maybe compress level)
    uint8_t  flags;
    uint8_t  stream;
    uint32_t next_stream;
    uint32_t checksum;
} MFTEntry;
static_assert(sizeof(MFTEntry) == 24, "sizeof(MFTEntry) == 24");

typedef struct MFTFileName {
    uint32_t file_id;
    uint32_t file_index;
} MFTFileName;
#pragma pack(pop)

typedef array(MFTEntry)    MFTEntryArray;
typedef array(MFTFileName) MFTFileNameArray;

typedef struct MFTFileHash {
    uint32_t key;
    uint32_t file_index;
} MFTFileHash;

typedef struct FileArchive {
    FILE            *handle;
    uint64_t         file_size;
    FaHeader         header;
    MFTHeader        file_header;
    MFTEntryArray    file_entries;
    MFTFileHash     *file_hash_table;
} FileArchive;

int  FaOpen(FileArchive *archive, const char *path);
void FaClose(FileArchive *archive);
int  FaGetEntry(FileArchive *archive, uint32_t file_hash, MFTEntry *result);
int  FaGetFile(FileArchive *archive, uint32_t file_hash, array_uint8_t *result);
