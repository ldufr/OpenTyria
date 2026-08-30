#pragma once

int FaSeek(FILE *fp, int64_t offset, int origin)
{
#ifdef _WIN32
    return _fseeki64(fp, offset, origin);
#else
    return fseek(fp, offset, origin);
#endif
}

int FaTell(FILE *fp, uint64_t *result)
{
    int64_t ret;
#ifdef _WIN32
    ret = _ftelli64(fp);
#else
    ret = ftell(fp);
#endif
    if (ret < 0) {
        return ERR_UNSUCCESSFUL;
    } else {
        *result = (int64_t) ret;
        return 0;
    }
}

int FaGetSize(FileArchive *archive)
{
    int err;

    fpos_t pos;
    if ((err = fgetpos(archive->handle, &pos)) != 0) {
        log_error("fgetpos failed");
        return ERR_UNSUCCESSFUL;
    }

    if ((err = FaSeek(archive->handle, 0, SEEK_END)) != 0) {
        goto cleanup;
    }

    err = FaTell(archive->handle, &archive->file_size);

cleanup:
    (void) fsetpos(archive->handle, &pos);
    return err;
}

int FaRead(FileArchive *archive, void *buffer, size_t size)
{
    if (fread(buffer, size, 1, archive->handle) != 1) {
        log_error("Failed to read %zu bytes, err: %d\n", size, ferror(archive->handle));
        return ERR_UNSUCCESSFUL;
    }

    return 0;
}

int FaReadAt(FileArchive *archive, int64_t offset, void* buffer, size_t size)
{
    int err;

    if ((err = FaSeek(archive->handle, offset, SEEK_SET)) != 0) {
        return err;
    }

    if (fread(buffer, size, 1, archive->handle) != 1) {
        log_error(
            "Failed to read %zu bytes at offset %" PRIi64 ", err: %d",
            size,
            offset,
            ferror(archive->handle)
        );
        return ERR_UNSUCCESSFUL;
    }

    return 0;
}

int FaOpen(FileArchive *archive, const char *path)
{
    int err;

    if ((archive->handle = fopen(path, "rb")) == NULL) {
        log_error("Couldn't open '%s', err: %d", path, sys_errno());
        return ERR_UNSUCCESSFUL;
    }

    if ((err = FaGetSize(archive)) != 0) {
        return ERR_UNSUCCESSFUL;
    }

    if (fread(&archive->header, sizeof(archive->header), 1, archive->handle) != 1) {
        log_error("Failed to read archive header, err: %d", sys_errno());
        return ERR_UNSUCCESSFUL;
    }

    uint32_t magic = le32toh(archive->header.magic);
    if (magic != FILE_ARCHIVE_MAGIC) {
        log_error("Invalid file archive header (expected 0x%" PRIX32 ", got 0x" PRIX32 ")", FILE_ARCHIVE_MAGIC, magic);
        return ERR_UNSUCCESSFUL;
    }

    uint64_t offset = le64toh(archive->header.mft_offset);
    if ((err = FaReadAt(archive, offset, &archive->file_header, sizeof(archive->file_header))) != 0) {
        return err;
    }

    uint32_t entry_count = le32toh(archive->file_header.entry_count);
    MFTEntry* entries = array_push(&archive->file_entries, entry_count);
    if ((err = FaRead(archive, entries, entry_count * sizeof(*entries))) != 0) {
        return err;
    }

    MFTEntry toc = entries[INDEX_TOC];
    size_t count = le32toh(toc.size) / sizeof(MFTFileName);

    MFTFileNameArray filenames = {0};
    MFTFileName *buffer = array_push(&filenames, count);
    err = FaReadAt(archive, le32toh(toc.offset), buffer, le32toh(toc.size));
    if (err == 0) {
        for (size_t idx = 0; idx < count; ++idx) {
            MFTFileHash value;
            value.key = le32toh(buffer[idx].file_id);
            value.file_index = le32toh(buffer[idx].file_index);
            stbds_hmputs(archive->file_hash_table, value);
        }
    }
    array_free(&filenames);

    return 0;
}

void FaClose(FileArchive *archive)
{
    if (archive->handle != NULL) {
        fclose(archive->handle);
        archive->handle = NULL;
    }

    array_free(&archive->file_entries);
    stbds_hmfree(archive->file_hash_table);
}

int FaGetEntry(FileArchive *archive, uint32_t file_hash, MFTEntry *result)
{
    ptrdiff_t idx;
    if ((idx = stbds_hmgeti(archive->file_hash_table, file_hash)) == -1) {
        return ERR_UNSUCCESSFUL;
    }

    uint32_t file_idx = archive->file_hash_table[idx].file_index;
    if (file_idx == 0) {
        return ERR_UNSUCCESSFUL;
    }

    *result = array_at(&archive->file_entries, file_idx);
    return 0;
}

int FaGetFile(FileArchive *archive, uint32_t file_hash, array_uint8_t *result)
{
    int err;

    ptrdiff_t idx;
    if ((idx = stbds_hmgeti(archive->file_hash_table, file_hash)) == -1) {
        return ERR_UNSUCCESSFUL;
    }

    uint32_t file_idx = archive->file_hash_table[idx].file_index;
    if (file_idx == 0) {
        return ERR_UNSUCCESSFUL;
    }

    --file_idx;
    MFTEntry entry = archive->file_entries.ptr[file_idx];

    array_clear(result);
    uint8_t *buffer = array_push(result, entry.size);
    if ((err = FaReadAt(archive, entry.offset, buffer, entry.size)) != 0) {
        return err;
    }

    if (entry.compressed != 0) {
        if (result->len < 4) {
            log_error("[file:%" PRIu32 "] Not enough bytes to get the decrypted file size\n", file_idx);
            return ERR_UNSUCCESSFUL;
        }

        uint32_t decompressed_size = le32dec(&buffer[result->len - 4]);

        array_uint8_t content = {0};
        if ((err = FaDecompress(buffer, result->len - 4, decompressed_size, &content)) != 0) {
            log_error("[%" PRIu32 "] Failed to decompress file\n", file_idx);
            array_free(&content);
            return err;
        }

        array_free(result);
        *result = content;
    }

    return 0;
}
