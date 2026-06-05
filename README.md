# coreparse

`coreparse` is a small side project of mine that aims to provide a C API for reading and parsing a Bitcoin Core datadir directly from disk.

The project started as a personal tool for experimenting with Bitcoin Core's on-disk data without having to go through JSON-RPC for every block, transaction, or index lookup.

> Status: experimental. The API is not stable yet.

## Goal

The goal of `coreparse` is to make it easier to inspect and parse data from a local Bitcoin Core datadir, including block files and selected LevelDB indexes.

It currently focuses on:

-   reading the Bitcoin Core block index;
    
-   loading blocks from `blk*.dat` files;
    
-   parsing blocks and transactions;
    
-   iterating transactions, inputs, outputs, and witness data;
    
-   computing transaction hashes;
    
-   using `txindex` when available;
    
-   reading raw chainstate UTXO records when available.
    

The library is designed around lightweight views into mapped block files, so the parser avoids unnecessary copies where possible.

## Non-goals

`coreparse` is not a Bitcoin node, wallet, consensus implementation, or full validator.

It does not replace Bitcoin Core. It only reads and parses data already stored by a local Bitcoin Core node.

## Build

```sh
make

```

This currently builds the C library from source.

The build system and public API are still expected to change.

## Notes

For reliable results, use `coreparse` on a stopped Bitcoin Core node or on a copy/snapshot of the datadir. Reading Bitcoin Core's LevelDB databases while the node is running may fail or produce inconsistent results.

This project is currently experimental and mainly intended for my own research and tooling.

## License

This project uses the [UNO Reverse Public License](https://github.com/Zuhaitz-dev/URPL).
