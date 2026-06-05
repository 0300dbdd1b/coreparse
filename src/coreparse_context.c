#include "include/coreparse_internal.h"
#include "include/leveldb.h"
#include <string.h>
#include <signal.h>

#define CTB_FS_NOPREFIX
#include "include/ctb_fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

static coreparse_context * g_emergency_ctx = NULL;

static void coreparse_signal_handler(int signum)
{
    if (g_emergency_ctx != NULL)
    {
        coreparse_deinit(g_emergency_ctx);
        g_emergency_ctx = NULL;
    }
}

coreparse_block_index_record    coreparse_get_block_index_record(const u8 *key, u64 klen, const u8 *val, u64 vlen)
{
    coreparse_block_index_record record = {0};
    u8 *p = (u8 *)val;
    const u8 *end = val + vlen;

    if (klen >= 33)
    {
        memcpy(record.blockhash, key + 1, 32);
        reverse_bytes(record.blockhash, 32);
    }

    record.version              = varint128_decode(&p, end);
    record.height               = varint128_decode(&p, end);
    record.validation_status    = varint128_decode(&p, end);
    record.tx_count             = varint128_decode(&p, end);
    record.block_file           = varint128_decode(&p, end);
    record.block_offset         = varint128_decode(&p, end);

    if ((end - p) > BLOCKHEADER_SIZE)
    {
        record.undo_file        = varint128_decode(&p, end);
        record.undo_offset      = varint128_decode(&p, end);
    }
    if ((vlen) >= BLOCKHEADER_SIZE)
    {
        const u8 *h = end - BLOCKHEADER_SIZE;
        memcpy(&record.header.version,          h + 0,  4);
        memcpy(record.header.prev_blockhash,    h + 4,  32);
        memcpy(record.header.merkleroot,        h + 36, 32);
        memcpy(&record.header.time,             h + 68, 4);
        memcpy(&record.header.bits,             h + 72, 4);
        memcpy(&record.header.nonce,            h + 76, 4);
        reverse_bytes(record.header.prev_blockhash, 32);
        reverse_bytes(record.header.merkleroot, 32);
    }
    return record;
}


coreparse_file_information_record coreparse_get_file_info_record(const u8 *key, u64 klen, const u8 *val, u64 vlen)
{
    coreparse_file_information_record rec = {0};

    // The key is 'f' followed by a 4-byte Little Endian integer
    if (klen == 5 && key[0] == 'f')
    {
        rec.file_number = key[1] | (key[2] << 8) | (key[3] << 16) | (key[4] << 24);
    }

    u8 *p = (u8 *)val;
    const u8 *end = val + vlen;


    rec.block_count         = varint128_decode(&p, end);
    rec.datafile_size       = varint128_decode(&p, end);
    rec.revfile_size        = varint128_decode(&p, end);
    rec.lowest_height       = varint128_decode(&p, end);
    rec.highest_height      = varint128_decode(&p, end);
    rec.lowest_timestamp    = varint128_decode(&p, end);
    rec.highest_timestamp   = varint128_decode(&p, end);

    return rec;
}

static void load_blocks_obfuscation_key(coreparse_context *ctx)
{
    char path[COREPARSE_MAX_PATH + 32]; // INFO: Silencing warning -Wformat-truncation
    snprintf(path, sizeof(path), "%s/xor.dat", ctx->blocksdir);

    FILE *f = fopen(path, "rb");
    if (f)
    {
        if (fread(ctx->blocks_obfuscation_key, 1, 8, f) == 8)
        {
            for (int i = 0; i < 8; i++)
            {
                if (ctx->blocks_obfuscation_key[i] != 0)
                {
                    ctx->has_blocks_obfuscation_key = 1;
                    break;
                }
            }
        }
        fclose(f);
    }
}

static void load_db_obfuscation_key(LDB_Instance *ldb, u8 *out_key, u16 *out_len, const char *db_name)
{
    const u8 obf_key_str[] = {0x0e, 'o', 'b', 'f', 'u', 's', 'c', 'a', 't', 'e', '_', 'k', 'e', 'y'};

    char *err = NULL;
    size_t vlen = 0;
    char *val = LDB_Get(ldb->db, ldb->roptions, (const char*)obf_key_str, sizeof(obf_key_str), &vlen, &err);

    if (err != NULL)
    {
        LDB_Free(err);
        return;
    }

    if (val != NULL)
    {
        u8 *p = (u8*)val;
        const u8 *end = (u8*)val + vlen;


        u64 stored_len = *p++; // Usually a 1-byte compact size for short keys

        if (stored_len > 0 && stored_len <= 64 && (u64)(end - p) >= stored_len)
        {
            memcpy(out_key, p, stored_len);
            *out_len = (u16)stored_len;
            printf("[*] %s LevelDB Obfuscation Detected (Key Len: %d)\n", db_name, *out_len);
        }
        LDB_Free(val);
    }
}


coreparse_context * coreparse_init(const char *datadir)
{

    struct sigaction sa;
    sa.sa_handler = coreparse_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT,   &sa, NULL) == -1)   return NULL;
    if (sigaction(SIGTERM,  &sa, NULL) == -1)   return NULL;

    coreparse_context * ctx = calloc(1, sizeof(coreparse_context));
    if (!ctx) return NULL;

    snprintf(ctx->datadir,              COREPARSE_MAX_PATH, "%s",                               datadir);
    snprintf(ctx->blocksdir,            COREPARSE_MAX_PATH, "%s/blocks",                        datadir);
    snprintf(ctx->blockindexdir,        COREPARSE_MAX_PATH, "%s/blocks/index",                  datadir);
    snprintf(ctx->txindexdir,           COREPARSE_MAX_PATH, "%s/indexes/txindex",               datadir);
    snprintf(ctx->chainstatedir,        COREPARSE_MAX_PATH, "%s/chainstate",                    datadir);
    snprintf(ctx->coinstatindexdir,     COREPARSE_MAX_PATH, "%s/indexes/coinstatsindex/db",     datadir);
    snprintf(ctx->txospenderindexdir,   COREPARSE_MAX_PATH, "%s/indexes/txospenderindex",       datadir);

    if (!fs_path_is_dir(ctx->datadir))          goto error;
    if (!fs_path_is_dir(ctx->blocksdir))        goto error;
    if (!fs_path_is_dir(ctx->blockindexdir))    goto error;
    load_blocks_obfuscation_key(ctx);

    if (fs_path_is_dir(ctx->txindexdir))
    {
        ctx->ldb_txindex = LDB_InitOpen(ctx->txindexdir);
        if (ctx->ldb_txindex.errors == NULL)
        {
            ctx->flags |= (HAS_INDEXES | HAS_TXINDEX);
            load_db_obfuscation_key(&ctx->ldb_txindex, ctx->txindex_obfuscation_key, &ctx->txindex_obfuscation_key_len, "txindex");
        }
    }
    if (fs_path_is_dir(ctx->chainstatedir))
    {
        ctx->ldb_chainstate = LDB_InitOpen(ctx->chainstatedir);
        if (ctx->ldb_chainstate.errors == NULL)
        {
            ctx->flags |= (HAS_INDEXES | HAS_CHAINSTATE);
            load_db_obfuscation_key(&ctx->ldb_chainstate, ctx->chainstate_obfuscation_key, &ctx->chainstate_obfuscation_key_len, "chainstate");
        }
    }
    ctx->ldb = LDB_InitOpen(ctx->blockindexdir);
    if (ctx->ldb.errors != NULL)                goto error;
    ctx->flags |= HAS_BLOCKINDEX;

    u64 counts[2] = {0,0};
    LDB_CountEntriesForPrefixes(ctx->ldb, "bf", 2, counts);
    ctx->block_index_records = calloc(counts[0], sizeof(coreparse_block_index_record));
    ctx->block_index_record_count = counts[0];
    ctx->file_information_records = calloc(counts[1], sizeof(coreparse_file_information_record));
    ctx->file_information_record_count = counts[1];
    LDB_Iterator *iterator = LDB_CreateIterator(ctx->ldb.db, ctx->ldb.roptions);
    if (!iterator)                              goto error;
    const   u8 *key;
    const   u8 *val;
    u64     klen;
    u64     vlen;
    for (LDB_IterSeekToFirst(iterator); LDB_IterValid(iterator); LDB_IterNext(iterator))
    {
        key = (const u8 *)LDB_IterKey(iterator, &klen);
        val = (const u8 *)LDB_IterValue(iterator, &vlen);
        if (klen > 0 && key[0] == 'b')
        {
            coreparse_block_index_record record = coreparse_get_block_index_record(key, klen, val, vlen);
            int is_valid_chain = (record.validation_status & BLOCK_VALID_MASK) >= BLOCK_VALID_CHAIN;
            if (is_valid_chain || record.height == 0)
            {
                ctx->block_index_records[record.height] = record;
            }
        }
        else if (klen > 0 && key[0] == 'f')
        {
            coreparse_file_information_record record =  coreparse_get_file_info_record(key, klen, val, vlen);
            ctx->file_information_records[record.file_number] = record;
        }
        else if (klen > 0 && key[0] == 'l')
        {
            continue;
        }
        else if (klen > 0 && key[0] == 'R')
        {
            continue;
        }
    }

    ctx->cache_usage_counter = 0;
    for (u64 i = 0; i < COREPARSE_FILE_CACHE_SIZE; i++)
    {
        ctx->file_cache[i].file_idx = -1;
        ctx->file_cache[i].fd = -1;
        ctx->file_cache[i].map_addr = NULL;
        ctx->file_cache[i].map_size = 0;
        ctx->file_cache[i].last_used = 0;
    }
    LDB_IterDestroy(iterator);
    g_emergency_ctx = ctx;
    return (ctx);

error:
    if (ctx && ctx->block_index_records)        free(ctx->block_index_records);
    if (ctx && ctx->file_information_records)   free(ctx->file_information_records);
    if (ctx)                                    free(ctx);
    return (NULL);
}


void coreparse_deinit(coreparse_context *ctx)
{
    if (!ctx) return;

    for (int i = 0; i < COREPARSE_FILE_CACHE_SIZE; i++)
    {
        if (ctx->file_cache[i].map_addr)
        {
            munmap(ctx->file_cache[i].map_addr, ctx->file_cache[i].map_size);
        }
        if (ctx->file_cache[i].fd != -1)
        {
            close(ctx->file_cache[i].fd);
        }
    }
    if (ctx->block_index_records)
    {
        free(ctx->block_index_records);
        ctx->block_index_records = NULL;
    }
    if (ctx->file_information_records)
    {
        free(ctx->file_information_records);
        ctx->file_information_records = NULL;
    }
    if (ctx->ldb.db)
    {
        LDB_Deinit(&ctx->ldb);
    }
    if (ctx->ldb_txindex.db)
    {
        LDB_Deinit(&ctx->ldb_txindex);
    }
    if (ctx->ldb_chainstate.db)
    {
        LDB_Deinit(&ctx->ldb_chainstate);
    }

    free(ctx);
}
