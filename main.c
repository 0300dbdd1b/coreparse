#include "src/include/coreparse_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// --- MACROS ---
#ifndef FOREACH_TX
#define FOREACH_TX(ITER, TX, BLOCK) \
    coreparse_iter_init(&(ITER), (BLOCK)); \
    while (coreparse_iter_next(&(ITER), &(TX)))

#define FOREACH_INPUT(ITER, INPUT, TX_PTR, BLOCK) \
    coreparse_inputs_begin(&(ITER), (TX_PTR), (BLOCK)); \
    while (coreparse_inputs_next(&(ITER), &(INPUT)))

#define FOREACH_OUTPUT(ITER, OUTPUT, TX_PTR, BLOCK) \
    coreparse_outputs_begin(&(ITER), (TX_PTR), (BLOCK)); \
    while (coreparse_outputs_next(&(ITER), &(OUTPUT)))
#endif

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <datadir> <count>\n", argv[0]);
        return 1;
    }

    const char *datadir = argv[1];
    u64 count = strtoull(argv[2], NULL, 10);

    coreparse_context *ctx = coreparse_init(datadir);
    if (!ctx) return 1;

    printf("Benchmarking %llu blocks...\n", (unsigned long long)count);

    u64 total_tx = 0;
    u64 total_inputs = 0;
    u64 total_outputs = 0;
    
    // Start Clock
    clock_t start = clock();

    for (u64 h = 0; h < count && h < ctx->block_index_record_count; h++) {
        coreparse_block *block = coreparse_get_block(ctx, h);
        if (!block) continue; // Skip missing blocks (e.g. pruned)

        coreparse_iterator tx_iter;
        coreparse_transaction tx;
        
        FOREACH_TX(tx_iter, tx, block) {
            total_tx++;

            // Force access to inputs
            coreparse_tx_input_iterator in_iter;
            coreparse_tx_input input;
            FOREACH_INPUT(in_iter, input, &tx, block) {
                total_inputs++;
                // Volatile read to prevent compiler optimization
                volatile u32 dummy = input.sequence; 
                (void)dummy; 
            }

            // Force access to outputs
            coreparse_tx_output_iterator out_iter;
            coreparse_tx_output output;
            FOREACH_OUTPUT(out_iter, output, &tx, block) {
                total_outputs++;
                volatile u64 dummy = output.ammount; 
                (void)dummy;
            }
        }
        coreparse_free_block(block);
    }

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Done.\n");
    printf("Time:    %.4f seconds\n", time_spent);
    printf("Speed:   %.0f blocks/sec\n", count / time_spent);
    printf("Txs:     %llu\n", (unsigned long long)total_tx);
    printf("Inputs:  %llu\n", (unsigned long long)total_inputs);
    printf("Outputs: %llu\n", (unsigned long long)total_outputs);

    coreparse_deinit(ctx);
    return 0;
}
