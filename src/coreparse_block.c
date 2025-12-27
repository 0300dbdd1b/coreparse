#include "include/coreparse_internal.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void get_blk_path(coreparse_context *ctx, u64 file_num, char *out_buf, u64 size)
{
    snprintf((char*)out_buf, size, "%s/blk%05llu.dat", ctx->blocksdir, (unsigned long long)file_num);
}

static coreparse_file_cache_entry * coreparse_get_file_map(coreparse_context *ctx, u64 file_idx)
{
    int i;
    int best_slot = -1;
    int empty_slot = -1;
    u64 min_usage = (u64)-1;

    for (i = 0; i < COREPARSE_FILE_CACHE_SIZE; i++)
    {
        if (ctx->file_cache[i].map_addr && ctx->file_cache[i].file_idx == file_idx)
        {
            ctx->file_cache[i].last_used = ++ctx->cache_usage_counter;
            return &ctx->file_cache[i];
        }
        if (ctx->file_cache[i].map_addr == NULL)
        {
            empty_slot = i;
        }
        if (ctx->file_cache[i].last_used < min_usage)
        {
            min_usage = ctx->file_cache[i].last_used;
            best_slot = i;
        }
    }

    int target_idx;
    if (empty_slot != -1)
    {
        target_idx = empty_slot;
    }
    else
    {
        target_idx = best_slot;
        munmap(ctx->file_cache[target_idx].map_addr, ctx->file_cache[target_idx].map_size);
        close(ctx->file_cache[target_idx].fd);
        ctx->file_cache[target_idx].map_addr = NULL;
        ctx->file_cache[target_idx].fd = -1;
    }

    char block_path[COREPARSE_MAX_PATH + 32];
    get_blk_path(ctx, file_idx, block_path, sizeof(block_path));

    int fd = open(block_path, O_RDONLY);
    if (fd < 0) return NULL;

    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        return NULL;
    }

    int prot_flags = PROT_READ;
    if (ctx->has_blocks_obfuscation_key) prot_flags |= PROT_WRITE;

    void *map = mmap(NULL, st.st_size, prot_flags, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        return NULL;
    }
    posix_madvise(map, st.st_size, POSIX_MADV_SEQUENTIAL);
    coreparse_file_cache_entry *entry = &ctx->file_cache[target_idx];
    entry->fd = fd;
    entry->map_addr = map;
    entry->map_size = st.st_size;
    entry->file_idx = file_idx;
    entry->last_used = ++ctx->cache_usage_counter;

    return entry;
}

coreparse_block *coreparse_get_block(coreparse_context *ctx, u64 height)
{
    coreparse_block_index_record block_record = ctx->block_index_records[height];
    coreparse_file_cache_entry *map_entry = coreparse_get_file_map(ctx, block_record.block_file);
    if (!map_entry) return NULL;

    u64 raw_offset = (block_record.block_offset >= 8) ? block_record.block_offset - 8 : 0;
    if (raw_offset + 8 > map_entry->map_size) return NULL;

    u8 *base_ptr = (u8*)map_entry->map_addr;
    u32 block_data_size;
    memcpy(&block_data_size, base_ptr + raw_offset + 4, 4);

    if (ctx->has_blocks_obfuscation_key)
    {
        u8 *size_p = (u8*)&block_data_size;
        u64 offset_base = raw_offset + 4;
        for (u8 i = 0; i < 4; i++)
        {
            size_p[i] ^= ctx->blocks_obfuscation_key[(offset_base + i) % 8];
        }
    }

    u64 total_size = 8 + block_data_size;

    coreparse_block *block = malloc(sizeof(coreparse_block)); 
    if (!block) return NULL;

    block->raw_data = base_ptr + raw_offset;
    block->raw_size = total_size;
    block->height   = height;
    block->header   = block_record.header;
    block->tx_count = block_record.tx_count;

    u8 *cursor      = (u8*)block->raw_data + 8 + 80;
    const u8 *end   = (u8*)block->raw_data + total_size;

    if (cursor >= end)
    {
        free(block);
        return NULL;
    }
    compact_size_decode(&cursor, end);

    block->tx_start_ptr = cursor;
    return block;
}

void coreparse_free_block(coreparse_block *block)
{
    if (block)
    {
        free(block);
    }
}
