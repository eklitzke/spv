# SPV: A Bitcoin SPV Client

This is my attempt at writing a from-scratch Bitcoin SPV client. The
implementation language is C++. This is purely for me to learn about the Bitcoin
protocol: you probably shouldn't use this.

If you are interested in learning more about how the Bitcoin protocol works, or
are interested in writing you own SPV client, you may find the source code here
useful as a reference. I would also recommend taking a look at
[libbtc](https://github.com/libbtc/libbtc), which is more fully featured.

## Status

This code is considered **alpha** and incomplete. To avoid being a nuisance to
the network, the client is currently hard-coded to connect to testnet.

Currently the client can connect to the peers via the DNS seed list, and
initiate header sync, downloading the full set of block headers starting from
the genesis block. The block headers themselves are stored in a RocksDB
database, in a directory named `.spv`.

## Compiling

To build `spv`, you'll need autoconf, automake, and a bleeding-edge C++17
compiler:

```bash
$ ./autogen.sh
$ ./configure
$ make
```

The `make` command will produce an executable at `src/spv`.

### Dependencies

Build dependencies:

 * autoconf
 * automake
 * pkg-config
 * [libuv](https://github.com/libuv/libuv) (version 1.x)
 * [librocksdb](http://rocksdb.org/)

Other dependencies/third party libs; these are all included as git subtrees:

 * [gabime/spdlog](https://github.com/gabime/spdlog)
 * [jarro2783/cxxopts](https://github.com/jarro2783/cxxopts)
 * [okdshin/PicoSHA2](https://github.com/okdshin/PicoSHA2)
 * [skypjack/uvw](https://github.com/skypjack/uvw)

## Licensing

This code is free software, licensed under the GPL v3+.
