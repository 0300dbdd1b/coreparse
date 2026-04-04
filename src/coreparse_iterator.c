#include "include/coreparse_internal.h"
#include <stdlib.h>
#include <string.h>

#include "include/coreparse_internal.h"
#include <stdlib.h>
#include <string.h>

void coreparse_iter_init(coreparse_iterator *iterator,
                         const coreparse_block *block)
{
    iterator->block = block;
    iterator->cursor = block->tx_start_ptr;
    iterator->txindex = 0;
}

int coreparse_parse_tx(const u8 *start_ptr, const u8 *end_boundary,
                       coreparse_transaction *view, const u8 **next_cursor)
{
    const u8 *p = start_ptr;
    const u8 *end = end_boundary;

    if (end - p < 4)
        return 0;

    u64 base_size = 0;

    // 1. Version
    memcpy(&view->version, p, 4);
    p += 4;
    base_size += 4;

    // 2. SegWit Marker & Flag (Not counted in base_size)
    view->is_segwit = 0;
    if (end - p >= 2 && p[0] == 0x00 && p[1] == 0x01)
    {
        view->is_segwit = 1;
        p += 2;
    }

    // 3. Inputs
    const u8 *varint_start = p;
    view->input_count = compact_size_decode((u8 **)&p, end);
    base_size += (p - varint_start);

    view->inputs_start = p;
    for (u64 i = 0; i < view->input_count; i++)
    {
        if (UNLIKELY(end - p < 36))
            return 0;
        p += 36; // TxID(32) + Vout(4)
        base_size += 36;

        varint_start = p;
        u64 len = compact_size_decode((u8 **)&p, end);
        base_size += (p - varint_start);

        p += len; // ScriptSig(len)
        base_size += len;

        if (UNLIKELY(end - p < 4))
            return 0;
        p += 4; // Sequence(4)
        base_size += 4;
    }

    // 4. Outputs
    varint_start = p;
    view->output_count = compact_size_decode((u8 **)&p, end);
    base_size += (p - varint_start);

    view->outputs_start = p;
    for (u64 i = 0; i < view->output_count; i++)
    {
        if (UNLIKELY(end - p < 8))
            return 0;
        p += 8; // Amount(8)
        base_size += 8;

        varint_start = p;
        u64 len = compact_size_decode((u8 **)&p, end);
        base_size += (p - varint_start);

        p += len; // ScriptPubKey(len)
        base_size += len;
    }

    // 5. Witness Data (Not counted in base_size)
    view->witness_start = NULL;
    if (view->is_segwit)
    {
        view->witness_start = p;
        for (u64 i = 0; i < view->input_count; i++)
        {
            u64 items = compact_size_decode((u8 **)&p, end);
            for (u64 j = 0; j < items; j++)
            {
                u64 len = compact_size_decode((u8 **)&p, end);
                p += len;
            }
        }
    }

    // 6. Locktime
    if (UNLIKELY(end - p < 4))
        return 0;
    memcpy(&view->locktime, p, 4);
    p += 4;
    base_size += 4;

    // 7. Calculate final metrics
    view->raw_ptr = start_ptr;
    view->total_size = (p - start_ptr);
    view->length = view->total_size; // NOTE: DEPRECATED: Maintain backwards
                                     // compatibility -- will be removed
    view->base_size = base_size;

    if (view->is_segwit)
    {
        view->weight = (view->base_size * 3) + view->total_size;
    }
    else
    {
        view->weight = view->total_size * 4;
    }

    view->vsize = (view->weight + 3) / 4;

    if (next_cursor)
        *next_cursor = p;
    return 1;
}

int coreparse_iter_next(coreparse_iterator *iterator,
                        coreparse_transaction *view)
{
    if (UNLIKELY(iterator->txindex >= iterator->block->tx_count))
        return 0;

    const u8 *start = iterator->cursor;
    const u8 *end =
        (const u8 *)iterator->block->raw_data + iterator->block->raw_size;

    if (!coreparse_parse_tx(start, end, view, &iterator->cursor))
    {
        return 0; // Malformed tx
    }

    iterator->txindex++;
    return 1;
}

void coreparse_inputs_begin(coreparse_tx_input_iterator *iter,
                            const coreparse_transaction *tx,
                            const coreparse_block *block)
{
    iter->block = block;
    iter->cursor = tx->inputs_start;
    iter->remaining = tx->input_count;
}

int coreparse_inputs_next(coreparse_tx_input_iterator *iter,
                          coreparse_tx_input *out)
{
    if (UNLIKELY(iter->remaining == 0))
        return 0;

    const u8 *p = iter->cursor;
    const u8 *end = (const u8 *)iter->block->raw_data + iter->block->raw_size;

    if (UNLIKELY(end - p < 36))
        return 0;

    out->txid = p;
    memcpy(&out->vout, p + 32, 4);
    p += 36;

    out->script_len = compact_size_decode((u8 **)&p, end);
    out->script_sig = p;
    p += out->script_len;

    if (UNLIKELY(end - p < 4))
        return 0;
    memcpy(&out->sequence, p, 4);
    p += 4;

    iter->cursor = p;
    iter->remaining--;
    return 1;
}

void coreparse_outputs_begin(coreparse_tx_output_iterator *iter,
                             const coreparse_transaction *tx,
                             const coreparse_block *block)
{
    iter->block = block;
    iter->cursor = tx->outputs_start;
    iter->remaining = tx->output_count;
}

int coreparse_outputs_next(coreparse_tx_output_iterator *iter,
                           coreparse_tx_output *out)
{
    if (UNLIKELY(iter->remaining == 0))
        return 0;

    const u8 *p = iter->cursor;
    const u8 *end = (const u8 *)iter->block->raw_data + iter->block->raw_size;

    if (UNLIKELY(end - p < 8))
        return 0;

    memcpy(&out->ammount, p, 8);
    p += 8;

    out->script_len = compact_size_decode((u8 **)&p, end);
    out->script_pubkey = p;
    p += out->script_len;

    iter->cursor = p;
    iter->remaining--;
    return 1;
}

coreparse_tx_witness
coreparse_get_witness_for_input(const coreparse_transaction *tx,
                                u64 input_index, const coreparse_block *block)
{
    coreparse_tx_witness result = {NULL, 0, 0};

    if (!tx->is_segwit || input_index >= tx->input_count || !tx->witness_start)
        return result;

    const u8 *p = tx->witness_start;
    const u8 *end = (const u8 *)block->raw_data + block->raw_size;

    for (u64 i = 0; i < input_index; i++)
    {
        u64 items = compact_size_decode((u8 **)&p, end);
        for (u64 j = 0; j < items; j++)
        {
            u64 len = compact_size_decode((u8 **)&p, end);
            p += len;
        }
    }

    u64 items = compact_size_decode((u8 **)&p, end);
    result.item_count = items;

    if (items > 0)
    {
        result.items = calloc(items, sizeof(coreparse_witness_item));
        if (!result.items)
            return result;

        for (u64 j = 0; j < items; j++)
        {
            u64 len = compact_size_decode((u8 **)&p, end);
            result.items[j].data = p;
            result.items[j].data_len = len;
            p += len;
        }

        if (items >= 2 && result.items[items - 1].data_len > 0)
        {
            if (result.items[items - 1].data[0] == ANNEX_TAG)
            {
                result.has_annex = 1;
            }
        }
    }

    return result;
}

void coreparse_free_witness(coreparse_tx_witness *witness)
{
    if (witness && witness->items)
    {
        free(witness->items);
        witness->items = NULL;
    }
    if (witness)
    {
        witness->item_count = 0;
        witness->has_annex = 0;
    }
}

int coreparse_witness_has_annex(const coreparse_tx_witness *witness)
{
    if (!witness)
        return 0;
    return witness->has_annex;
}

void coreparse_get_txid(const coreparse_transaction *tx, u8 out_hash[32])
{
    u8 hash1[32];
    sha256_ctx ctx;
    sha256_init(&ctx);

    if (tx->is_segwit)
    {

        // 1. Hash the Version (First 4 bytes)
        sha256_update(&ctx, tx->raw_ptr, 4);

        // 2. Hash Inputs + Outputs (Skips bytes 4 & 5 which are Marker & Flag)
        // It ends exactly where the witness data begins.
        u64 in_out_len = tx->witness_start - (tx->raw_ptr + 6);
        sha256_update(&ctx, tx->raw_ptr + 6, in_out_len);

        // 3. Hash Locktime (The last 4 bytes of the transaction)
        sha256_update(&ctx, tx->raw_ptr + tx->total_size - 4, 4);
    }
    else
    {
        // Legacy transactions: Just hash the entire contiguous raw block
        sha256_update(&ctx, tx->raw_ptr, tx->total_size);
    }

    sha256_final(&ctx, hash1);

    // Second SHA256 pass
    sha256(hash1, 32, out_hash);
}

void coreparse_get_wtxid(const coreparse_transaction *tx, u8 out_hash[32])
{
    if (!tx->is_segwit)
    {
        // For legacy transactions, WTXID is identical to TXID
        coreparse_get_txid(tx, out_hash);
        return;
    }

    // For SegWit, WTXID is the double-hash of the ENTIRE transaction including
    // witness
    u8 hash1[32];
    sha256(tx->raw_ptr, tx->total_size, hash1);
    sha256(hash1, 32, out_hash);
}

int coreparse_get_tx_location(coreparse_context *ctx, const u8 txid[32],
                              u64 *file_no, u64 *block_offset, u64 *tx_offset)
{
    if (!(ctx->flags & HAS_TXINDEX))
        return 0;

    // 1. Construct Key: 't' + TXID
    u8 key[33];
    key[0] = 't';
    memcpy(key + 1, txid, 32);

    size_t vlen = 0;
    char *err = NULL;
    char *val = LDB_Get(ctx->ldb_txindex.db, ctx->ldb_txindex.roptions,
                        (const char *)key, 33, &vlen, &err);

    if (err || !val)
    {
        if (err)
            LDB_Free(err);
        return 0; // Transaction not found
    }

    // 2. De-Obfuscate using the TxIndex XOR key
    u8 *data = (u8 *)val;
    if (ctx->txindex_obfuscation_key_len > 0)
    {
        for (size_t i = 0; i < vlen; i++)
        {
            data[i] ^=
                ctx->txindex_obfuscation_key[i %
                                             ctx->txindex_obfuscation_key_len];
        }
    }

    // 3. Decode the VarInts
    u8 *p = data;
    const u8 *end = data + vlen;
    *file_no = varint128_decode(&p, end);
    *block_offset = varint128_decode(&p, end);
    *tx_offset = varint128_decode(&p, end);

    LDB_Free(val);
    return 1;
}

int coreparse_get_raw_utxo(coreparse_context *ctx, const u8 txid[32], u32 vout,
                           u8 **out_data, u64 *out_len)
{
    if (!(ctx->flags & HAS_CHAINSTATE))
        return 0;

    // 1. Construct Key: 'c' + TXID + MSB_varint(vout)
    u8 key[64];
    key[0] = 'C';
    memcpy(key + 1, txid, 32);
    int vout_len = encode_varint_msb(vout, key + 33);

    size_t klen = 33 + vout_len;
    size_t vlen = 0;
    char *err = NULL;

    char *val = LDB_Get(ctx->ldb_chainstate.db, ctx->ldb_chainstate.roptions,
                        (const char *)key, klen, &vlen, &err);

    if (err || !val)
    {
        if (err)
            LDB_Free(err);
        return 0;
    }

    // 2. Copy to a clean buffer so we can safely free LevelDB's string
    u8 *clean_data = malloc(vlen);
    if (!clean_data)
    {
        LDB_Free(val);
        return 0;
    }
    memcpy(clean_data, val, vlen);
    LDB_Free(val);

    if (ctx->chainstate_obfuscation_key_len > 0)
    {
        // 3. De-obfuscate using Chainstate XOR key
        for (size_t i = 0; i < vlen; i++)
        {
            clean_data[i] ^= ctx->chainstate_obfuscation_key
                                 [i % ctx->chainstate_obfuscation_key_len];
        }
    }

    *out_data = clean_data;
    *out_len = vlen;
    return 1;
}

int coreparse_fetch_transaction(coreparse_context *ctx, const u8 txid[32],
                                coreparse_transaction *out_tx)
{
    u64 file_no, block_offset, tx_offset;

    // 1. Ask the TxIndex LevelDB for the physical location
    if (!coreparse_get_tx_location(ctx, txid, &file_no, &block_offset,
                                   &tx_offset))
    {
        return 0;
    }

    // 2. Ensure the specific blkXXXXX.dat file is mapped in our cache
    coreparse_file_cache_entry *map_entry =
        coreparse_get_file_map(ctx, file_no);
    if (!map_entry || !map_entry->map_addr)
    {
        return 0;
    }

    // 3. Calculate absolute file offset
    // Bitcoin Core's index stores block_offset pointing to the 80-byte header
    // (after the 8-byte magic/size). tx_offset is the offset from the start of
    // the block (including magic bytes).
    u64 raw_file_offset =
        (block_offset >= 8 ? block_offset - 8 : 0) + tx_offset;

    if (raw_file_offset >= map_entry->map_size)
    {
        return 0;
    }

    // 4. Jump straight to the transaction bytes and parse it!
    const u8 *tx_start = (const u8 *)map_entry->map_addr + raw_file_offset;
    const u8 *file_end = (const u8 *)map_entry->map_addr + map_entry->map_size;

    return coreparse_parse_tx(tx_start, file_end, out_tx, NULL);
}
