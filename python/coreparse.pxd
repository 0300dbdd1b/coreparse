# python/coreparse.pxd

cdef extern from "coreparse_internal.h":

    ctypedef unsigned long long u64
    ctypedef unsigned int u32
    ctypedef unsigned short u16
    ctypedef unsigned char u8

    ctypedef enum coreparse_block_status:
        BLOCK_VALID_UNKNOWN
        BLOCK_VALID_RESERVED
        BLOCK_VALID_TREE
        BLOCK_VALID_TRANSACTIONS
        BLOCK_VALID_CHAIN
        BLOCK_VALID_SCRIPTS
        BLOCK_VALID_MASK
        BLOCK_HAVE_DATA
        BLOCK_HAVE_UNDO
        BLOCK_HAVE_MASK
        BLOCK_FAILED_VALID
        BLOCK_FAILED_CHILD
        BLOCK_FAILED_MASK
        BLOCK_OPT_WITNESS
        BLOCK_STATUS_RESERVED

    cdef struct LDB_Instance:
        pass

    cdef struct coreparse_block_header:
        u32 version
        u8  prev_blockhash[32]
        u8  merkleroot[32]
        u32 time
        u32 bits
        u32 nonce

    cdef struct coreparse_witness_item:
        const u8 * data
        u64        data_len

    cdef struct coreparse_tx_witness:
        coreparse_witness_item *items
        u64                     item_count
        u8                      has_annex

    cdef struct coreparse_transaction:
        const u8 * raw_ptr
        u64         length
        u64         total_size
        u64         base_size
        u64         weight
        u64         vsize
        u32         version
        u32         locktime
        u8          is_segwit
        const u8 * inputs_start
        u64         input_count
        const u8 * outputs_start
        u64         output_count
        const u8 * witness_start

    cdef struct coreparse_tx_input:
        const u8 * txid
        u32         vout
        const u8 * script_sig
        u64         script_len
        u32         sequence

    cdef struct coreparse_tx_output:
        u64         ammount
        const u8 * script_pubkey
        u64         script_len

    cdef struct coreparse_block:
        void * raw_data
        u64         raw_size
        u64         height
        u64         tx_count
        const u8 * tx_start_ptr
        coreparse_block_header header

    cdef struct coreparse_iterator:
        const coreparse_block * block
        const u8 * cursor
        u64        txindex

    cdef struct coreparse_tx_input_iterator:
        const coreparse_block * block
        const u8 * cursor
        u64        remaining

    cdef struct coreparse_tx_output_iterator:
        const coreparse_block * block
        const u8 * cursor
        u64        remaining

    cdef struct coreparse_block_index_record:
        u8                      blockhash[32]
        coreparse_block_header  header
        u64                     version
        u64                     height
        u64                     tx_count
        coreparse_block_status  validation_status
        u64                     block_file
        u64                     block_offset
        u64                     undo_file
        u64                     undo_offset

    cdef struct coreparse_file_information_record:
        u64 file_number
        u64 block_count
        u64 datafile_size
        u64 revfile_size
        u64 lowest_height
        u64 highest_height
        u64 lowest_timestamp
        u64 highest_timestamp

    cdef struct coreparse_file_cache_entry:
        u64     file_idx
        int     fd
        void * map_addr
        u64     map_size
        u64     last_used

    cdef struct coreparse_context:
        char    datadir[1024]
        char    blocksdir[1024]
        char    blockindexdir[1024]
        char    chainstatedir[1024]
        char    txindexdir[1024]

        LDB_Instance    ldb
        LDB_Instance    ldb_chainstate
        LDB_Instance    ldb_txindex

        coreparse_block_index_record * block_index_records
        u64                                 block_index_record_count
        coreparse_file_information_record * file_information_records
        u64                                 file_information_record_count

        u8  chainstate_obfuscation_key[64]
        u16 chainstate_obfuscation_key_len

        u8  txindex_obfuscation_key[64]
        u16 txindex_obfuscation_key_len

        u8  blocks_obfuscation_key[8]
        u8  has_blocks_obfuscation_key

        coreparse_file_cache_entry file_cache[8]
        u64 cache_usage_counter

        u32 flags

    coreparse_context * coreparse_init(const char *datadir)
    void coreparse_deinit(coreparse_context *ctx)

    coreparse_block *coreparse_get_block(coreparse_context *ctx, u64 height)
    void coreparse_free_block(coreparse_block *block)

    void coreparse_iter_init(coreparse_iterator *iterator, const coreparse_block *block)
    int coreparse_iter_next(coreparse_iterator *iterator, coreparse_transaction *view)

    void coreparse_inputs_begin(coreparse_tx_input_iterator *iter, const coreparse_transaction *tx, const coreparse_block *block)
    int coreparse_inputs_next(coreparse_tx_input_iterator *iter, coreparse_tx_input *out)

    void coreparse_outputs_begin(coreparse_tx_output_iterator *iter, const coreparse_transaction *tx, const coreparse_block *block)
    int coreparse_outputs_next(coreparse_tx_output_iterator *iter, coreparse_tx_output *out)

    coreparse_tx_witness coreparse_get_witness_for_input(const coreparse_transaction *tx, u64 input_index, const coreparse_block *block)
    void coreparse_free_witness(coreparse_tx_witness *witness)
    int coreparse_witness_has_annex(const coreparse_tx_witness *witness)

    void coreparse_get_txid(const coreparse_transaction *tx, u8 out_hash[32])
    void coreparse_get_wtxid(const coreparse_transaction *tx, u8 out_hash[32])

    int coreparse_fetch_transaction(coreparse_context *ctx, const u8 *txid, coreparse_transaction *out_tx)
    int coreparse_get_tx_location(coreparse_context *ctx, const u8 *txid, u64 *file_no, u64 *block_offset, u64 *tx_offset)
    int coreparse_get_raw_utxo(coreparse_context *ctx, const u8 *txid, u32 vout, u8 **out_data, u64 *out_len)
