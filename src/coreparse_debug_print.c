#include "include/coreparse_internal.h"
#include <stdio.h>

static void print_hex(const u8 *data, u64 len)
{
    if (!data)
    {
        printf("null");
        return;
    }
    for (u64 i = 0; i < len; i++)
    {
        printf("%02x", data[i]);
    }
}

static void print_hex_rev(const u8 *data, u64 len)
{
    if (!data)
    {
        printf("null");
        return;
    }
    for (long i = len - 1; i >= 0; i--)
    {
        printf("%02x", data[i]);
    }
}

void coreparse_print_input(const coreparse_tx_input *in)
{
    printf("    {\n");
    printf("      \"txid\": \""); print_hex_rev(in->txid, 32); printf("\",\n");
    printf("      \"vout\": %u,\n", in->vout);
    printf("      \"script_len\": %llu,\n", (unsigned long long)in->script_len);
    printf("      \"sequence\": %u\n", in->sequence);
    printf("    }");
}

void coreparse_print_output(const coreparse_tx_output *out)
{
    printf("    {\n");
    printf("      \"amount\": %llu,\n", (unsigned long long)out->ammount);
    printf("      \"script_len\": %llu,\n", (unsigned long long)out->script_len);
    printf("      \"script_pubkey\": \""); print_hex(out->script_pubkey, out->script_len); printf("\"\n");
    printf("    }");
}

void coreparse_print_transaction(const coreparse_transaction *tx, const coreparse_block *block)
{
    printf("  {\n");
    printf("    \"version\": %u,\n", tx->version);
    printf("    \"segwit\": %s,\n", tx->is_segwit ? "true" : "false");
    printf("    \"locktime\": %u,\n", tx->locktime);
    printf("    \"size\": %llu,\n", (unsigned long long)tx->length);
    printf("    \"inputs\": [\n");
    coreparse_tx_input_iterator in_iter;
    coreparse_inputs_begin(&in_iter, tx, block);
    coreparse_tx_input input;
    int first = 1;
    while (coreparse_inputs_next(&in_iter, &input))
    {
        if (!first) printf(",\n");
        coreparse_print_input(&input);
        first = 0;
    }
    printf("\n    ],\n");

    printf("    \"outputs\": [\n");
    coreparse_tx_output_iterator out_iter;
    coreparse_outputs_begin(&out_iter, tx, block);
    coreparse_tx_output output;
    first = 1;
    while (coreparse_outputs_next(&out_iter, &output))
    {
        if (!first) printf(",\n");
        coreparse_print_output(&output);
        first = 0;
    }
    printf("\n    ]\n");
    printf("  }");
}

void coreparse_print_block(const coreparse_block *block)
{
    if (!block)
    {
        printf("Block is NULL\n");
        return;
    }
    printf("{\n");
    printf("  \"height\": %llu,\n", (unsigned long long)block->height);
    printf("  \"prevhash\": \""); print_hex_rev(block->header.prev_blockhash, 32); printf("\",\n");
    printf("  \"timestamp\": %u,\n", block->header.time);
    printf("  \"tx_count\": %llu,\n", (unsigned long long)block->tx_count);
    printf("  \"transactions\": [\n");
    coreparse_iterator tx_iter;
    coreparse_iter_init(&tx_iter, block);
    coreparse_transaction tx;
    int first = 1;
    while (coreparse_iter_next(&tx_iter, &tx))
    {
        if (!first) printf(",\n");
        coreparse_print_transaction(&tx, block);
        first = 0;
    }
    printf("\n  ]\n");
    printf("}\n");
}
