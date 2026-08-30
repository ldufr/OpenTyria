#pragma once

typedef struct GmIdHeader {
    uint32_t free_next;
    bool     freed;
} GmIdHeader;

typedef struct GmIdTableBase {
    void    *buffer;
    uint32_t size;
    uint32_t free_head;
    uint32_t free_tail;
} GmIdTableBase;

#define GmIdTable(T) \
union {                         \
    GmIdTableBase base;         \
    struct {                    \
        T       *buffer;        \
        uint32_t size;          \
        uint32_t free_head;     \
        uint32_t free_tail;     \
    };                          \
}

GmIdHeader* GmIdTableAt(GmIdTableBase *table, uint32_t idx, size_t elem_size)
{
    return (GmIdHeader *)((char *)table->buffer + idx * elem_size);
}

void GmIdFree(GmIdTableBase *table, uint32_t idx, size_t elem_size)
{
    assert(idx < table->size);
    GmIdHeader *header = GmIdTableAt(table, idx, elem_size);
    assert(!header->freed);
    header->freed = true;

    // We always update the index to the next free element. If nothing was
    // queued, it's 0, which is the correct next index.
    if ((header->free_next = table->free_tail) == 0) {
        // If the queue was empty, we also update the index of the head, that
        // is the next  element to dequeue.
        table->free_head = idx;
    } else {
        // If the queue was empty, we need to update the index of the next
        // free element of the previously last element.
        GmIdTableAt(table, table->free_tail, elem_size)->free_next = idx;
    }

    table->free_tail = idx;
}

void GmIdTableInit(GmIdTableBase *table, size_t elem_size)
{
    if (elem_size < sizeof(size_t)) {
        log_error("Minimum element size for a 'GmIdTable' is %zu, got %zu", sizeof(size_t), elem_size);
        abort();
    }

    uint32_t size = 64;
    if ((table->buffer = calloc(1, size * elem_size)) == NULL) {
        log_error("Failed create a table array");
        abort();
    }

    table->size = size;
    for (uint32_t idx = 1; idx < size; ++idx) {
        GmIdFree(table, idx, elem_size);
    }
}

void GmIdTableFree(GmIdTableBase *table)
{
    free(table->buffer);
    memset(table, 0, sizeof(*table));
}

uint32_t GmIdAllocate(GmIdTableBase *table, size_t elem_size)
{
    if (!table->buffer) {
        GmIdTableInit(table, elem_size);
    }

    if (table->free_head == 0) {
        assert(table->size != 0);
        uint32_t new_size = table->size * 2;
        if ((table->buffer = realloc(table->buffer, new_size * elem_size)) == NULL) {
            log_error("Failed to allocate new id");
            abort();
        }

        memset((char *)table->buffer + (table->size * elem_size), 0, table->size);
        for (uint32_t idx = table->size; idx < new_size - 1; ++idx) {
            size_t *free_next_ptr = (size_t *)((char *)table->buffer + idx * elem_size);
            *free_next_ptr = idx + 1;
        }

        table->free_head = table->size;
        table->free_tail = new_size - 1;
        table->size = new_size;
    }

    assert(table->free_head != 0);
    uint32_t new_id = table->free_head;

    // If we removed the last element of the table, we have to update the index
    // of the last element as well.
    GmIdHeader *header = GmIdTableAt(table, table->free_head, elem_size);
    if ((table->free_head = header->free_next) == 0) {
        table->free_tail = 0;
    }

    assert(header->freed);
    header->freed = false;
    return new_id;
}
