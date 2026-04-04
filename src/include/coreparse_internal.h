#ifndef COREPARSE_INTERNAL_H
#define COREPARSE_INTERNAL_H

#define CTB_PLATFORM_NOPREFIX
#include "ctb_platform.h"
#include "ctb_types.h"
#define CTB_SHA2_NOPREFIX
#include "ctb_sha2.h"
#include "leveldb.h"
#include "coreparse_encoding.h"

#ifndef COREPARSE_MAX_PATH
#define COREPARSE_MAX_PATH 1024
#endif


#define BLOCK_0_TIMESTAMP 1231006505		// 03 Jan 2009, 18:15:05
#define BLOCK_0_HASH 0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f

#define BLOCK_210000_TIMESTAMP 1354116278 	// 28 Nov 2012, 15:24:38
#define BLOCK_210000_HASH 0x000000000000048b95347e83192f69cf0366076336c639f9b7228e9ba171342e

#define BLOCK_420000_TIMESTAMP 1468082773	// 09 Jul 2016, 16:46:13
#define BLOCK_420000_HASH 0x000000000000000002cce816c0ab2c5c269cb081896b7dcb34b8422d6b74ffa1

#define BLOCK_630000_TIMESTAMP 1589225023	// 11 May 2020, 19:23:43
#define BLOCK_630000_HASH 0x000000000000000000024bead8df69990852c202db0e0097c1a12ea637d7e96d

#define BLOCK_840000_TIMESTAMP 1713571767	// 20 Apr 2024, 00:09:27
#define BLOCK_840000_HASH 0x0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5

#define MAGIC_BYTES_MAINNET		0xf9beb4d9
#define MAGIC_BYTES_TESTNET3	0x0b110907
#define MAGIC_BYTES_TESTNET4	0x1c163f28
#define MAGIC_BYTES_REGTEST		0xfabfb5da

#define TARGET_BLOCK_INTERVAL (2016/2015) * 60 * 10 // take into account the off-by-one bug


// SEE: https://github.com/bitcoin/bitcoin/blob/6d546336e800f7b8990fececab6bc08413f28690/src/node/blockstorage.h#L70
#define MAX_BLOCKFILE_CHUNK_SIZE	0x100000		// 16 MiB
// SEE: https://github.com/bitcoin/bitcoin/blob/6d546336e800f7b8990fececab6bc08413f28690/src/node/blockstorage.h#L72
#define MAX_UNDOFILE_CHUNK_SIZE	0x100000		// 1 MiB
// SEE: https://github.com/bitcoin/bitcoin/blob/6d546336e800f7b8990fececab6bc08413f28690/src/node/blockstorage.h#L74
#define MAX_BLOCKFILE_SIZE 		0x8000000 		// 128 MiB

#define SHA256_HASH_SIZE  32	// 32 bytes
#define BLOCKHEADER_SIZE  80	// 80 bytes

#define MAX_BLOCK_SIZE 1000000
#define MAX_SCRIPT_ELEMENT_SIZE 520		// Maximum number of bytes pushable to the stack
#define MAX_OPS_PER_SCRIPT 201				// Maximum number of non-push operations per script
#define MAX_PUBKEYS_PER_MULTISIG 20		// Maximum number of public keys per multisig
#define MAX_PUBKEYS_PER_MULTI_A 999		// The limit of keys in OP_CHECKSIGADD-based scripts. It is due to the stack limit in BIP342.
#define MAX_SCRIPT_SIZE 10000				// Maximum script length in bytes
#define MAX_STACK_SIZE 1000				// Maximum number of values on script interpreter stack
// Threshold for nLockTime: below this value it is interpreted as block number,
// otherwise as UNIX timestamp.
#define LOCKTIME_THRESHOLD 500000000		// Tue Nov  5 00:53:20 1985 UTC
// Maximum nLockTime. Since a lock time indicates the last invalid timestamp, a
// transaction with this lock time will never be valid unless lock time
// checking is disabled (by setting all input sequence numbers to
// SEQUENCE_FINAL).
#define LOCKTIME_MAX 0xFFFFFFFFU

// Tag for input annex. If there are at least two witness elements for a transaction input,
// and the first byte of the last element is 0x50, this last element is called annex, and
// has meanings independent of the script
#define ANNEX_TAG 0x50
// Validation weight per passing signature (Tapscript only, see BIP 342).
#define VALIDATION_WEIGHT_PER_SIGOP_PASSED 50
// How much weight budget is added to the witness size (Tapscript only, see BIP 342).
#define VALIDATION_WEIGHT_OFFSET 50

#define MAX_COMPACT_SIZE_BYTES 10

#define MAX_VARINT128_BYTES 10


// SEE: https://github.com/bitcoin/bitcoin/blob/60b816439eb4bd837778d424628cd3978e0856d9/src/chain.h#L88
// NOTE: This is litterally a copy-paste from bitcoin /src/chain.h - block_status enum
typedef enum coreparse_block_status
{
	//! Unused.
	BLOCK_VALID_UNKNOWN      =    0,
	//! Reserved (was BLOCK_VALID_HEADER).
	BLOCK_VALID_RESERVED     =    1,
	//! All parent headers found, difficulty matches, timestamp >= median previous, checkpoint. Implies all parents
	//! are also at least TREE.
	BLOCK_VALID_TREE         =    2,
	/**
	 * Only first tx is coinbase, 2 <= coinbase input script length <= 100, transactions valid, no duplicate txids,
	 * sigops, size, merkle root. Implies all parents are at least TREE but not necessarily TRANSACTIONS.
	 *
	 * If a block's validity is at least VALID_TRANSACTIONS, CBlockIndex::nTx will be set. If a block and all previous
	 * blocks back to the genesis block or an assumeutxo snapshot block are at least VALID_TRANSACTIONS,
	 * CBlockIndex::m_chain_tx_count will be set.
	 */
	BLOCK_VALID_TRANSACTIONS =    3,
	//! Outputs do not overspend inputs, no double spends, coinbase output ok, no immature coinbase spends, BIP30.
	//! Implies all previous blocks back to the genesis block or an assumeutxo snapshot block are at least VALID_CHAIN.
	BLOCK_VALID_CHAIN        =    4,
	//! Scripts & signatures ok. Implies all previous blocks back to the genesis block or an assumeutxo snapshot block
	//! are at least VALID_SCRIPTS.
	BLOCK_VALID_SCRIPTS      =    5,
	//! All validity bits.
	BLOCK_VALID_MASK         =   BLOCK_VALID_RESERVED | BLOCK_VALID_TREE | BLOCK_VALID_TRANSACTIONS |
	BLOCK_VALID_CHAIN | BLOCK_VALID_SCRIPTS,
	BLOCK_HAVE_DATA          =    8, //!< full block available in blk*.dat
	BLOCK_HAVE_UNDO          =   16, //!< undo data available in rev*.dat
	BLOCK_HAVE_MASK          =   BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO,
	BLOCK_FAILED_VALID       =   32, //!< stage after last reached validness failed
	BLOCK_FAILED_CHILD       =   64, //!< descends from failed block
	BLOCK_FAILED_MASK        =   BLOCK_FAILED_VALID | BLOCK_FAILED_CHILD,
	BLOCK_OPT_WITNESS        =   128, //!< block data in blk*.dat was received with a witness-enforcing client
	BLOCK_STATUS_RESERVED    =   256, //!< Unused flag that was previously set on assumeutxo snapshot blocks and their
	//!< ancestors before they were validated, and unset when they were validated.
} coreparse_block_status;


typedef struct coreparse_block_header
{
    u32 version;
    u8  prev_blockhash[32];
    u8  merkleroot[32];
    u32 time;
    u32 bits;
    u32 nonce;
}   coreparse_block_header;

typedef struct coreparse_transaction
{
    const u8 *  raw_ptr;
    u64         length;
    u64         total_size;
    u64         base_size;
    u64         weight;
    u64         vsize;
    u32         version;
    u32         locktime;
    u8          is_segwit;
    const u8 *  inputs_start;
    u64         input_count;
    const u8 *  outputs_start;
    u64         output_count;
    const u8 *  witness_start;
}   coreparse_transaction;

typedef struct coreparse_tx_input
{
    const u8 *  txid;
    u32         vout;
    const u8 *  script_sig;
    u64         script_len;
    u32         sequence;
}   coreparse_tx_input;


typedef struct coreparse_tx_output
{
    u64         ammount;
    const u8 *  script_pubkey;
    u64         script_len;
}   coreparse_tx_output;

typedef struct coreparse_witness_item
{
    const u8 *  data;
    u64         data_len;
}   coreparse_witness_item;

typedef struct coreparse_tx_witness
{
    coreparse_witness_item  *items;
    u64                     item_count;
    u8                      has_annex;
} coreparse_tx_witness;

typedef struct coreparse_block
{
    void *      raw_data;
    u64         raw_size;

    u64         height;
    u64         tx_count;
    const u8 *  tx_start_ptr;

    coreparse_block_header  header;
}   coreparse_block;

typedef struct coreparse_iterator
{
    const coreparse_block * block;
    const u8 *  cursor;
    u64         txindex;
}   coreparse_iterator;

typedef struct coreparse_tx_input_iterator
{
    const coreparse_block * block;
    const u8 *  cursor;
    u64         remaining;
}   coreparse_tx_input_iterator;

typedef struct coreparse_tx_output_iterator
{
    const coreparse_block * block;
    const u8 *  cursor;
    u64         remaining;
}   coreparse_tx_output_iterator;

typedef struct coreparse_block_index_record
{
    u8                      blockhash[32];
    coreparse_block_header  header;
    u64                     version;
    u64                     height;
    u64                     tx_count;
    coreparse_block_status  validation_status;
    u64                     block_file;
    u64                     block_offset;
    u64                     undo_file;
    u64                     undo_offset;
}   coreparse_block_index_record;

typedef struct coreparse_file_information_record
{
	u64 file_number;                // 8 bytes - file number
	u64 block_count;                // 8 bytes - The number of blocks stored in the block file with that number
	u64 datafile_size;              // 8 bytes - The size of the blk.dat file with that number
	u64 revfile_size;               // 8 bytes - The size of the rev.dat file with that number
	u64 lowest_height;              // 8 bytes - The lowest height of blocks stored in the block file with that number
	u64 highest_height;             // 8 bytes - The highest height of blocks stored in the block file with that number
	u64 lowest_timestamp;           // 8 bytes - The lowest timestamp of blocks stored in the block file with that number
	u64 highest_timestamp;          // 8 bytes - The highest timestamp of blocks stored in the block file with that number
}   coreparse_file_information_record;


#define COREPARSE_FILE_CACHE_SIZE 8
typedef struct coreparse_file_cache_entry
{
    u64     file_idx;
    int     fd;
    void *  map_addr;
    u64     map_size;
    u64     last_used;
}   coreparse_file_cache_entry;

typedef enum coreparse_ctx_flag
{
    NONE            =   0,
    HAS_XORDAT      =   1 << 0,
    HAS_BLOCKS      =   1 << 1,
    HAS_BLOCKINDEX  =   1 << 2,
    HAS_INDEXES     =   1 << 3,
    HAS_CHAINSTATE  =   1 << 4,
    HAS_TXINDEX     =   1 << 5,
}   coreparse_ctx_flag;

typedef struct coreparse_context
{
    char    datadir[COREPARSE_MAX_PATH];
    char    blocksdir[COREPARSE_MAX_PATH];
    char    blockindexdir[COREPARSE_MAX_PATH];
    char    chainstatedir[COREPARSE_MAX_PATH];
    char    txindexdir[COREPARSE_MAX_PATH];

    LDB_Instance    ldb;
    LDB_Instance    ldb_chainstate;
    LDB_Instance    ldb_txindex;

    coreparse_block_index_record *      block_index_records;
    u64                                 block_index_record_count;
    coreparse_file_information_record * file_information_records;
    u64                                 file_information_record_count;

    u8  chainstate_obfuscation_key[64];
    u16 chainstate_obfuscation_key_len;

    u8  txindex_obfuscation_key[64];
    u16 txindex_obfuscation_key_len;

    u8  blocks_obfuscation_key[8];
    u8  has_blocks_obfuscation_key;

    coreparse_file_cache_entry file_cache[COREPARSE_FILE_CACHE_SIZE];
    u64 cache_usage_counter;

    u32 flags;
}   coreparse_context;


coreparse_context * coreparse_init(const char *datadir);
void coreparse_deinit(coreparse_context *ctx);

coreparse_file_cache_entry * coreparse_get_file_map(coreparse_context *ctx, u64 file_idx);
coreparse_block *coreparse_get_block(coreparse_context *ctx, u64 height);
void coreparse_free_block(coreparse_block *block);

void coreparse_iter_init(coreparse_iterator *iterator, const coreparse_block *block);
int coreparse_iter_next(coreparse_iterator *iterator, coreparse_transaction *view);
void coreparse_inputs_begin(coreparse_tx_input_iterator *iter, const coreparse_transaction *tx, const coreparse_block *block);
int coreparse_inputs_next(coreparse_tx_input_iterator *iter, coreparse_tx_input *out);
void coreparse_outputs_begin(coreparse_tx_output_iterator *iter, const coreparse_transaction *tx, const coreparse_block *block);
int coreparse_outputs_next(coreparse_tx_output_iterator *iter, coreparse_tx_output *out);

coreparse_tx_witness coreparse_get_witness_for_input(const coreparse_transaction *tx, u64 input_index, const coreparse_block *block);
void coreparse_free_witness(coreparse_tx_witness *witness);

int coreparse_witness_has_annex(const coreparse_tx_witness *witness);

void coreparse_get_txid(const coreparse_transaction *tx, u8 out_hash[32]);
void coreparse_get_wtxid(const coreparse_transaction *tx, u8 out_hash[32]);

// Exported from coreparse_iterator.c
int coreparse_parse_tx(const u8 *start_ptr, const u8 *end_boundary, coreparse_transaction *view, const u8 **next_cursor);
int coreparse_fetch_transaction(coreparse_context *ctx, const u8 txid[32], coreparse_transaction *out_tx);
int coreparse_get_tx_location(coreparse_context *ctx, const u8 txid[32], u64 *file_no, u64 *block_offset, u64 *tx_offset);
int coreparse_get_raw_utxo(coreparse_context *ctx, const u8 txid[32], u32 vout, u8 **out_data, u64 *out_len);
void reverse_bytes(u8 *p, u64 len);
// void print_byte_string(const u8 *bytes, u64 size, FILE *output);
// void convert_u32_to_uint8_array(u32 value, u8 *output_array);
// u32 change_endianness_uint32(u32 value);

#define FOREACH_TX(ITER, TX, BLOCK)                         \
    coreparse_iter_init(&(ITER), (BLOCK));                  \
    while (coreparse_iter_next(&(ITER), &(TX)))

#define FOREACH_INPUT(ITER, INPUT, TX_PTR, BLOCK)           \
    coreparse_inputs_begin(&(ITER), (TX_PTR), (BLOCK));     \
    while (coreparse_inputs_next(&(ITER), &(INPUT)))

#define FOREACH_OUTPUT(ITER, OUTPUT, TX_PTR, BLOCK)         \
    coreparse_outputs_begin(&(ITER), (TX_PTR), (BLOCK));    \
    while (coreparse_outputs_next(&(ITER), &(OUTPUT)))
#endif

