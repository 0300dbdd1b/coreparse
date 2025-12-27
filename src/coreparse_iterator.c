#include "include/coreparse_internal.h"
#include <string.h>

void coreparse_iter_init(coreparse_iterator *iterator, const coreparse_block *block)
{
    iterator->block = block;
    iterator->cursor = block->tx_start_ptr;
    iterator->txindex = 0;
}

int coreparse_iter_next(coreparse_iterator *iterator, coreparse_transaction *view)
{
    if (UNLIKELY(iterator->txindex >= iterator->block->tx_count)) return 0;

    const u8 *p = iterator->cursor;
    const u8 *start = p;
    const u8 *end = (const u8*)iterator->block->raw_data + iterator->block->raw_size;

    if (end - p < 4) return 0;

    memcpy(&view->version, p, 4); // view->version = *(u32*)p;
    p += 4; // Version(4)

    view->is_segwit = 0;
    if (end - p >= 2 && p[0] == 0x00 && p[1] == 0x01)
    {
        view->is_segwit = 1;
        p += 2; // Marker(1) & Flag(1)
    }

    view->input_count = compact_size_decode((u8**)&p, end);
    view->inputs_start = p;
    for (u64 i = 0; i < view->input_count; i++)
    {
        if (UNLIKELY(end - p < 36)) return 0;
        p += 36; // TxID(32) + Vout(4)
        u64 len = compact_size_decode((u8**)&p, end);
        p += len; // Script(len)
        if (UNLIKELY(end - p < 4)) return 0;
        p += 4;   // Sequence(4)
    }

    view->output_count = compact_size_decode((u8**)&p, end);
    view->outputs_start = p;
    for (u64 i = 0; i < view->output_count; i++)
    {
        if (UNLIKELY(end - p < 8)) return 0;
        p += 8; // Amount(8)
        u64 len = compact_size_decode((u8**)&p, end);
        p += len; // Script(len)
    }

    if (view->is_segwit)
    {
        for (u64 i = 0; i < view->input_count; i++)
        {
            u64 items = compact_size_decode((u8**)&p, end);
            for (u64 j = 0; j < items; j++)
            {
                u64 len = compact_size_decode((u8**)&p, end);
                p += len;
            }
        }
    }

    if (UNLIKELY(end - p < 4)) return 0;
    memcpy(&view->locktime, p, 4); // view->locktime = *(u32*)p;
    p += 4; // locktime(4)

    view->raw_ptr = start;
    view->length = (p - start);
    iterator->cursor = p;
    iterator->txindex++;
    return 1;
}

void coreparse_inputs_begin(coreparse_tx_input_iterator *iter, const coreparse_transaction *tx, const coreparse_block *block)
{
    iter->block = block;
    iter->cursor = tx->inputs_start;
    iter->remaining = tx->input_count;
}

int coreparse_inputs_next(coreparse_tx_input_iterator *iter, coreparse_tx_input *out)
{
    if (UNLIKELY(iter->remaining == 0)) return 0;

    const u8 *p = iter->cursor;
    const u8 *end = (const u8*)iter->block->raw_data + iter->block->raw_size;

    if (UNLIKELY(end - p < 36)) return 0;

    out->txid = p;
    memcpy(&out->vout, p + 32, 4); // out->vout = *(u32*)(p + 32);
    p += 36;

    out->script_len = compact_size_decode((u8**)&p, end);
    out->script_sig = p;
    p += out->script_len;

    if (UNLIKELY(end - p < 4)) return 0;
    memcpy(&out->sequence, p, 4); // out->sequence = *(u32*)p;
    p += 4;

    iter->cursor = p;
    iter->remaining--;
    return 1;
}


void coreparse_outputs_begin(coreparse_tx_output_iterator *iter, const coreparse_transaction *tx, const coreparse_block *block)
{
    iter->block = block;
    iter->cursor = tx->outputs_start;
    iter->remaining = tx->output_count;
}

int coreparse_outputs_next(coreparse_tx_output_iterator *iter, coreparse_tx_output *out)
{
    if (UNLIKELY(iter->remaining == 0)) return 0;

    const u8 *p = iter->cursor;
    const u8 *end = (const u8*)iter->block->raw_data + iter->block->raw_size;

    if (UNLIKELY(end - p < 8)) return 0;

    memcpy(&out->ammount, p, 8);
    p += 8; // Ammount(8)

    out->script_len = compact_size_decode((u8**)&p, end);
    out->script_pubkey = p;
    p += out->script_len;

    iter->cursor = p;
    iter->remaining--;
    return 1;
}
