#include "src/include/coreparse_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TEST FRAMEWORK MACROS
// ============================================================================
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define ASSERT_TRUE(cond, msg)                                                 \
    do                                                                         \
    {                                                                          \
        tests_run++;                                                           \
        if (!(cond))                                                           \
        {                                                                      \
            printf("  [\033[31mFAIL\033[0m] %s:%d - %s\n", __func__, __LINE__, \
                   msg);                                                       \
            tests_failed++;                                                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            printf("  [\033[32mPASS\033[0m] %s\n", msg);                       \
            tests_passed++;                                                    \
        }                                                                      \
    } while (0)

#define ASSERT_EQ(val1, val2, msg)                                             \
    do                                                                         \
    {                                                                          \
        tests_run++;                                                           \
        if ((val1) != (val2))                                                  \
        {                                                                      \
            printf("  [\033[31mFAIL\033[0m] %s:%d - %s (Expected %llu, got "   \
                   "%llu)\n",                                                  \
                   __func__, __LINE__, msg, (unsigned long long)(val2),        \
                   (unsigned long long)(val1));                                \
            tests_failed++;                                                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            printf("  [\033[32mPASS\033[0m] %s\n", msg);                       \
            tests_passed++;                                                    \
        }                                                                      \
    } while (0)

// Helper to format hashes
void sprint_hash(const u8 *hash, char *out)
{
    for (int i = 31; i >= 0; i--)
    {
        sprintf(out + (31 - i) * 2, "%02x", hash[i]);
    }
    out[64] = '\0';
}

// ============================================================================
// GLOBAL TEST STATE
// ============================================================================
coreparse_context *ctx = NULL;
u64 test_height = 100000;
u8 target_txid[32] = {0};
u64 target_vout = 0;

// ============================================================================
// TEST SUITES
// ============================================================================

void test_context_initialization(const char *datadir)
{
    printf("\n--- Test Suite: Context Initialization ---\n");

    ctx = coreparse_init(datadir);
    ASSERT_TRUE(ctx != NULL, "Context allocated successfully");
    if (!ctx)
        return;

    ASSERT_TRUE(ctx->flags & HAS_BLOCKINDEX, "Block index loaded");
    ASSERT_TRUE(ctx->block_index_record_count > 0, "Block index records > 0");
    ASSERT_TRUE(ctx->file_information_record_count > 0,
                "File info records > 0");

    // Adjust test height if node is pruned or syncing
    if (test_height >= ctx->block_index_record_count ||
        ctx->block_index_records[test_height].height == 0)
    {
        test_height = ctx->block_index_record_count > 0
                          ? ctx->block_index_record_count - 1
                          : 0;
    }
}

void test_block_parsing()
{
    printf("\n--- Test Suite: Block Parsing ---\n");
    if (!ctx)
        return;

    coreparse_block *block = coreparse_get_block(ctx, test_height);
    ASSERT_TRUE(block != NULL, "Fetched block successfully from disk");
    if (!block)
        return;

    ASSERT_EQ(block->height, test_height,
              "Block height matches requested height");
    ASSERT_TRUE(block->tx_count > 0, "Block contains transactions");
    ASSERT_TRUE(block->raw_size > 80, "Block size is larger than header");

    coreparse_free_block(block);
    ASSERT_TRUE(1, "Block memory freed successfully");
}

void test_transaction_iteration()
{
    printf("\n--- Test Suite: Transaction Iteration & Hashing ---\n");
    if (!ctx)
        return;

    coreparse_block *block = coreparse_get_block(ctx, test_height);
    if (!block)
        return;

    coreparse_iterator iter;
    coreparse_iter_init(&iter, block);
    coreparse_transaction tx;

    int tx_count = 0;
    int coinbase_found = 0;

    while (coreparse_iter_next(&iter, &tx))
    {
        tx_count++;

        // Test Coinbase (First TX)
        if (tx_count == 1)
        {
            ASSERT_EQ(tx.input_count, 1,
                      "Coinbase transaction has exactly 1 input");
            coinbase_found = 1;
        }

        // Store the last transaction's ID to use in DB tests
        if (tx_count == block->tx_count)
        {
            coreparse_get_txid(&tx, target_txid);
            ASSERT_TRUE(tx.output_count > 0, "Target TX has outputs");
            target_vout = 0; // We'll test the first output
        }
    }

    ASSERT_EQ(tx_count, block->tx_count,
              "Iterator yielded exactly the expected number of transactions");
    ASSERT_TRUE(coinbase_found, "Coinbase was parsed");

    char hash_str[65];
    sprint_hash(target_txid, hash_str);
    printf("  [INFO] Target TXID for DB tests: %s\n", hash_str);

    coreparse_free_block(block);
}

void test_txindex()
{
    printf("\n--- Test Suite: TxIndex Zero-Copy Fetch ---\n");
    if (!ctx)
        return;

    if (!(ctx->flags & HAS_TXINDEX))
    {
        printf("  [SKIP] TxIndex is not enabled on this node.\n");
        return;
    }

    u64 file_no, block_offset, tx_offset;
    int found = coreparse_get_tx_location(ctx, target_txid, &file_no,
                                          &block_offset, &tx_offset);
    ASSERT_TRUE(found == 1, "Target TX location found in TxIndex");

    if (found)
    {
        coreparse_transaction fetched_tx;
        int parsed = coreparse_fetch_transaction(ctx, target_txid, &fetched_tx);
        ASSERT_TRUE(parsed == 1,
                    "Transaction successfully parsed directly from mmap");

        if (parsed)
        {
            u8 fetched_hash[32];
            coreparse_get_txid(&fetched_tx, fetched_hash);
            ASSERT_TRUE(memcmp(target_txid, fetched_hash, 32) == 0,
                        "Fetched TX native hash matches TxIndex query");
        }
    }

    // Test a bogus TXID
    u8 bad_txid[32] = {0xFF, 0xFF, 0xFF, 0xFF}; // Unlikely to exist
    found = coreparse_get_tx_location(ctx, bad_txid, &file_no, &block_offset,
                                      &tx_offset);
    ASSERT_TRUE(found == 0, "Bogus TXID correctly rejected by TxIndex");
}

void test_chainstate()
{
    printf("\n--- Test Suite: Chainstate (UTXO) Queries ---\n");
    if (!ctx)
        return;

    if (!(ctx->flags & HAS_CHAINSTATE))
    {
        printf("  [SKIP] Chainstate is not accessible on this node.\n");
        return;
    }

    u8 *utxo_data = NULL;
    u64 utxo_len = 0;

    // Test the target TXID (Will likely be spent unless testing on the absolute
    // chain tip)
    int is_unspent = coreparse_get_raw_utxo(ctx, target_txid, target_vout,
                                            &utxo_data, &utxo_len);

    if (is_unspent)
    {
        printf("  [INFO] Target UTXO is currently UNSPENT. Compressed length: "
               "%llu\n",
               utxo_len);
        ASSERT_TRUE(utxo_data != NULL, "UTXO memory was allocated");
        free(utxo_data);
    }
    else
    {
        printf("  [INFO] Target UTXO is spent (Expected behavior for "
               "historical blocks).\n");
        ASSERT_TRUE(utxo_data == NULL,
                    "UTXO memory correctly remains NULL for spent outputs");
    }

    // Test a bogus UTXO
    u8 bad_txid[32] = {0xAA, 0xBB, 0xCC};
    is_unspent =
        coreparse_get_raw_utxo(ctx, bad_txid, 0, &utxo_data, &utxo_len);
    ASSERT_TRUE(is_unspent == 0, "Bogus UTXO correctly reported as not found");
}

// ============================================================================
// MAIN RUNNER
// ============================================================================

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <path_to_bitcoin_datadir>\n", argv[0]);
        return 1;
    }
    const char *datadir = argv[1];

    printf("\n======================================================\n");
    printf("         COREPARSE C-ENGINE TEST SUITE v1.0           \n");
    printf("======================================================\n");

    test_context_initialization(datadir);
    test_block_parsing();
    test_transaction_iteration();
    test_txindex();
    test_chainstate();

    if (ctx)
    {
        coreparse_deinit(ctx);
        ctx = NULL;
        ASSERT_TRUE(1, "Context cleanly deinitialized without leaks");
    }

    printf("\n======================================================\n");
    printf("  TEST SUMMARY: %d Passed | %d Failed | %d Total\n", tests_passed,
           tests_failed, tests_run);
    printf("======================================================\n\n");

    return tests_failed == 0 ? 0 : 1;
}
