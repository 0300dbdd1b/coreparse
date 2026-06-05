import sys
import time
import coreparse
import matplotlib.pyplot as plt

def run_ultra_fast_analytics(datadir, start_height, end_height):
    print(f"[*] Booting Fast CoreParse Engine for blocks {start_height} to {end_height}...")
    try:
        p = coreparse.Parser(datadir)
    except Exception as e:
        print(f"Error: {e}")
        return

    # 1. Chunked C-Execution for Progress Logging & Memory Efficiency
    total_blocks = end_height - start_height + 1
    chunk_size = 5000  # Adjust this based on your NVMe speed
    batch_stats = []
    
    print(f"[*] Aggregating {total_blocks:,} blocks natively in C (Chunk size: {chunk_size:,})...")
    
    t0_global = time.time()
    
    try:
        for current_start in range(start_height, end_height + 1, chunk_size):
            current_end = min(current_start + chunk_size - 1, end_height)
            
            chunk_t0 = time.time()
            
            # Call Cython for just this specific chunk
            chunk_stats = p.get_batch_block_stats(current_start, current_end)
            
            chunk_time = time.time() - chunk_t0
            blocks_in_chunk = current_end - current_start + 1
            
            if chunk_stats:
                batch_stats.extend(chunk_stats)
            
            # --- Progress Logging Math ---
            blocks_done = current_end - start_height + 1
            percent = (blocks_done / total_blocks) * 100
            
            # Avoid division by zero
            chunk_time = chunk_time if chunk_time > 0.001 else 0.001
            blocks_per_sec = blocks_in_chunk / chunk_time
            
            elapsed_total = time.time() - t0_global
            avg_speed = blocks_done / elapsed_total
            eta_seconds = (total_blocks - blocks_done) / avg_speed
            
            print(f"  -> Progress: {percent:5.1f}% | Blocks: {current_start}-{current_end} | "
                  f"Speed: {blocks_per_sec:7,.0f} blk/s | ETA: {eta_seconds:4.0f}s")
            
    except KeyboardInterrupt:
        print("\n[!] Process safely interrupted by user. Plotting what we have so far...")
        if not batch_stats:
            sys.exit(0)

    if not batch_stats:
        print("No data returned. Check your block range and datadir.")
        return
        
    print(f"[*] {len(batch_stats):,} blocks successfully parsed in {time.time() - t0_global:.2f} seconds.")
    print("[*] Preparing Matplotlib GUI...")

    # 2. Setup Data Arrays for Matplotlib
    heights, tx_counts, inputs, outputs, volumes = [], [], [], [], []
    utxo_increases, cum_utxos, weights, vsizes = [], [], [], []
    segwit_ratios, avg_tx_vsizes, avg_outs_per_tx = [], [], []

    cumulative_added = 0

    # 3. Unpack the fast C-results
    for stats in batch_stats:
        h = stats["height"]
        num_txs = stats["tx_count"]
        block_ins = stats["inputs"]
        block_outs = stats["outputs"]

        volume_btc = stats["volume_sats"] / 1e8
        net_increase = block_outs - block_ins 
        cumulative_added += net_increase

        heights.append(h)
        tx_counts.append(num_txs)
        inputs.append(block_ins)
        outputs.append(block_outs)
        
        # Volume log scale handles zeros poorly, ensure minimum of 1 satoshi for plot
        volumes.append(max(volume_btc, 0.00000001))
        
        utxo_increases.append(net_increase)
        
        # Scale cumulative UTXOs by millions for clean y-axis
        cum_utxos.append(cumulative_added / 1_000_000)
        
        # Scale Weights and vSizes by millions to remove scientific notation
        weights.append(stats["weight"] / 1_000_000)
        vsizes.append(stats["vsize"] / 1_000_000)
        
        segwit_ratios.append((stats["segwit_txs"] / num_txs * 100) if num_txs else 0)
        avg_tx_vsizes.append((stats["vsize"] / num_txs) if num_txs else 0)
        avg_outs_per_tx.append((block_outs / num_txs) if num_txs else 0)

    # 4. --- High Performance Matplotlib GUI ---
    fig, axes = plt.subplots(4, 3, figsize=(18, 12))
    fig.canvas.manager.set_window_title('Bitcoin Ultra-Fast Chainstate Analytics')
    fig.subplots_adjust(hspace=0.5, wspace=0.3, left=0.05, right=0.95, top=0.95, bottom=0.05)

    # ABSOLUTE SMALLEST POINTS: marker=',' forces exactly 1 pixel per point. 
    # alpha=1.0 keeps them solid blue so they don't look faded.
    plot_kwargs = {'linestyle': 'none', 'marker': ',', 'color': 'blue', 'alpha': 1.0}
    line_kwargs = {'color': 'dodgerblue', 'linewidth': 1.5}

    axes[0, 0].plot(heights, tx_counts, **plot_kwargs)
    axes[0, 0].set_title('Transactions per Block')
    
    axes[0, 1].plot(heights, inputs, **plot_kwargs)
    axes[0, 1].set_title('Inputs per Block')
    
    axes[0, 2].plot(heights, outputs, **plot_kwargs)
    axes[0, 2].set_title('Outputs per Block')

    axes[1, 0].plot(heights, volumes, **plot_kwargs)
    axes[1, 0].set_title('Volume Settled (BTC) [Log Scale]')
    axes[1, 0].set_yscale('log')

    axes[1, 1].plot(heights, utxo_increases, **plot_kwargs)
    axes[1, 1].set_title('UTXO Set Net Increase (Δ)')
    
    axes[1, 2].plot(heights, cum_utxos, **line_kwargs)
    axes[1, 2].set_title('Cumulative UTXOs Added (Millions)')

    axes[2, 0].plot(heights, weights, **plot_kwargs)
    axes[2, 0].set_title('Block Weight (Millions WU)')
    
    axes[2, 1].plot(heights, vsizes, **plot_kwargs)
    axes[2, 1].set_title('Block Virtual Size (Millions vB)')
    
    axes[2, 2].plot(heights, segwit_ratios, **plot_kwargs)
    axes[2, 2].set_title('SegWit Adoption (% of Txs)')

    axes[3, 0].plot(heights, avg_tx_vsizes, **plot_kwargs)
    axes[3, 0].set_title('Average TX vSize (vB) [Log Scale]')
    axes[3, 0].set_yscale('log') 

    axes[3, 1].plot(heights, avg_outs_per_tx, **plot_kwargs)
    axes[3, 1].set_title('Average Outputs per TX [Log Scale]')
    axes[3, 1].set_yscale('log') 
    
    in_out_ratio = [i/o if o > 0 else 0 for i, o in zip(inputs, outputs)]
    axes[3, 2].plot(heights, in_out_ratio, **plot_kwargs)
    axes[3, 2].set_title('Input / Output Ratio')

    # Formatting loop for all axes
    for ax in axes.flat:
        ax.grid(True, linestyle='--', alpha=0.3)
        ax.set_xlabel('Block Height')
        
        # Prevent scientific notation on the X-axis (Block Heights)
        ax.ticklabel_format(style='plain', axis='x')
        
        # Prevent scientific notation on the Y-axis (but ONLY for linear scales)
        if ax.get_yscale() == 'linear':
            ax.ticklabel_format(style='plain', axis='y')

    print("[*] Launching GUI...")
    plt.show()

# ==============================================================================
# MAIN EXECUTION
# ==============================================================================
if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 test.py <datadir> <start_height> <end_height>")
        sys.exit(1)
        
    data_dir = sys.argv[1]
    s_height = int(sys.argv[2])
    e_height = int(sys.argv[3])
    
    run_ultra_fast_analytics(data_dir, s_height, e_height)
