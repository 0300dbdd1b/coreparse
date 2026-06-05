# python/coreparse.pyx
# distutils: language = c

cimport coreparse
from libc.stdlib cimport free

# --- Future-Proof: Ultra-Fast Byte-level Script Classifier ---
# Doing this directly on the C-pointers skips Python string allocation entirely.
cdef inline int classify_script_fast(const coreparse.u8* script, coreparse.u64 length):
    # Returns: 0=Other, 1=P2PKH, 2=P2SH, 3=P2WPKH, 4=P2WSH, 5=P2TR
    if length == 25 and script[0] == 0x76 and script[1] == 0xa9 and script[2] == 0x14 and script[23] == 0x88 and script[24] == 0xac:
        return 1 # P2PKH
    elif length == 23 and script[0] == 0xa9 and script[1] == 0x14 and script[22] == 0x87:
        return 2 # P2SH
    elif length == 22 and script[0] == 0x00 and script[1] == 0x14:
        return 3 # P2WPKH
    elif length == 34 and script[0] == 0x00 and script[1] == 0x20:
        return 4 # P2WSH
    elif length == 34 and script[0] == 0x51 and script[1] == 0x20:
        return 5 # P2TR (Taproot)
    return 0 # Other / Multisig / OP_RETURN / P2PK

cdef class Parser:
    """
    A high-performance Python wrapper for the coreparse Bitcoin blockchain parser.
    """
    cdef coreparse.coreparse_context* _ctx

    def __cinit__(self, str datadir):
        cdef bytes py_bytes = datadir.encode('utf-8')
        self._ctx = coreparse.coreparse_init(py_bytes)
        if self._ctx is NULL:
            raise RuntimeError(f"Failed to initialize coreparse context at '{datadir}'.")

    def __dealloc__(self):
        if self._ctx is not NULL:
            coreparse.coreparse_deinit(self._ctx)

    @property
    def max_height(self):
        return self._ctx.block_index_record_count - 1

    @property
    def has_txindex(self):
        return bool(self._ctx.flags & 32)

    @property
    def has_chainstate(self):
        return bool(self._ctx.flags & 16)

    def get_block_time(self, coreparse.u64 height):
        if height >= self._ctx.block_index_record_count:
            return None
        return self._ctx.block_index_records[height].header.time

    def get_tx_location(self, str txid_hex):
        if not self.has_txindex:
            raise RuntimeError("txindex is not enabled or loaded on this node.")

        cdef bytes txid_bytes = bytes.fromhex(txid_hex)[::-1]
        cdef const coreparse.u8* txid_ptr = <const coreparse.u8*>txid_bytes
        cdef coreparse.u64 file_no = 0, block_offset = 0, tx_offset = 0

        cdef int success = coreparse.coreparse_get_tx_location(self._ctx, txid_ptr, &file_no, &block_offset, &tx_offset)
        if not success:
            return None
        return {"file_number": file_no, "block_offset": block_offset, "tx_offset": tx_offset}

    def get_raw_utxo(self, str txid_hex, coreparse.u32 vout):
        if not self.has_chainstate:
            raise RuntimeError("chainstate is not loaded or inaccessible.")

        cdef bytes txid_bytes = bytes.fromhex(txid_hex)[::-1]
        cdef const coreparse.u8* txid_ptr = <const coreparse.u8*>txid_bytes
        cdef coreparse.u8* out_data = NULL
        cdef coreparse.u64 out_len = 0

        cdef int success = coreparse.coreparse_get_raw_utxo(self._ctx, txid_ptr, vout, &out_data, &out_len)
        if not success or out_data is NULL:
            return None

        cdef bytes utxo_bytes = out_data[:out_len]
        free(out_data)
        return utxo_bytes

    def get_block_stats(self, coreparse.u64 height):
        """Processes a single block in pure C and returns aggregated stats."""
        stats = self.get_batch_block_stats(height, height)
        return stats[0] if stats else None

    def get_batch_block_stats(self, coreparse.u64 start_height, coreparse.u64 end_height):
        """
        [FUTURE PROOF API] 
        Iterates over a massive range of blocks entirely in Cython. Bypasses millions
        of Python object allocations and returns a flat array of aggregated metrics.
        This is designed specifically to feed analytics dashboards in under a second.
        """
        cdef coreparse.u64 height
        cdef coreparse.coreparse_block* block
        cdef coreparse.coreparse_iterator tx_iter
        cdef coreparse.coreparse_transaction tx
        cdef coreparse.coreparse_tx_input_iterator in_iter
        cdef coreparse.coreparse_tx_input tx_in
        cdef coreparse.coreparse_tx_output_iterator out_iter
        cdef coreparse.coreparse_tx_output tx_out

        cdef coreparse.u64 total_inputs, total_outputs, total_volume, total_weight, total_vsize
        cdef coreparse.u64 segwit_txs, tx_count, coinbase_subsidy
        cdef coreparse.u64 p2pkh_out, p2sh_out, p2wpkh_out, p2wsh_out, p2tr_out, other_out
        cdef int script_type

        batch_results = []

        if end_height > self.max_height:
            end_height = self.max_height

        for height in range(start_height, end_height + 1):
            block = coreparse.coreparse_get_block(self._ctx, height)
            if block is NULL:
                continue

            total_inputs = 0
            total_outputs = 0
            total_volume = 0
            total_weight = 0
            total_vsize = 0
            segwit_txs = 0
            tx_count = 0
            coinbase_subsidy = 0
            
            p2pkh_out = p2sh_out = p2wpkh_out = p2wsh_out = p2tr_out = other_out = 0

            coreparse.coreparse_iter_init(&tx_iter, block)
            while coreparse.coreparse_iter_next(&tx_iter, &tx):
                tx_count += 1
                total_weight += tx.weight
                total_vsize += tx.vsize
                if tx.is_segwit:
                    segwit_txs += 1
                
                # Count Inputs
                coreparse.coreparse_inputs_begin(&in_iter, &tx, block)
                while coreparse.coreparse_inputs_next(&in_iter, &tx_in):
                    total_inputs += 1

                # Process Outputs
                coreparse.coreparse_outputs_begin(&out_iter, &tx, block)
                while coreparse.coreparse_outputs_next(&out_iter, &tx_out):
                    total_outputs += 1
                    total_volume += tx_out.ammount

                    # Track Coinbase Reward (txindex 0 is always the coinbase)
                    if tx_iter.txindex == 0:
                        coinbase_subsidy += tx_out.ammount

                    # Instant Script Categorization
                    if tx_out.script_len > 0:
                        script_type = classify_script_fast(tx_out.script_pubkey, tx_out.script_len)
                        if script_type == 1: p2pkh_out += 1
                        elif script_type == 2: p2sh_out += 1
                        elif script_type == 3: p2wpkh_out += 1
                        elif script_type == 4: p2wsh_out += 1
                        elif script_type == 5: p2tr_out += 1
                        else: other_out += 1
                    else:
                        other_out += 1

            coreparse.coreparse_free_block(block)

            # Append ONE dictionary per block, fully aggregated.
            batch_results.append({
                "height": height,
                "tx_count": tx_count,
                "inputs": total_inputs,
                "outputs": total_outputs,
                "volume_sats": total_volume,
                "coinbase_sats": coinbase_subsidy,
                "weight": total_weight,
                "vsize": total_vsize,
                "segwit_txs": segwit_txs,
                "script_stats": {
                    "p2pkh": p2pkh_out,
                    "p2sh": p2sh_out,
                    "p2wpkh": p2wpkh_out,
                    "p2wsh": p2wsh_out,
                    "p2tr": p2tr_out,
                    "other": other_out
                }
            })

        return batch_results

    # ... (Keep existing fetch_transaction and get_block_transactions exactly as they were in your source) ...
