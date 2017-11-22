# SPV: A Bitcoin SPV Client

This is my attempt at writing a from-scratch Bitcoin SPV client. The
implementation language is C++. This is purely for me to learn about the Bitcoin
protocol: you shouldn't actually use this, and certainly not on mainnet. If you
are interested in learning more about how the Bitcoin protocol works, or are
interested in writing you own SPV client, you may find the source code here
useful as a reference.

## Compiling

First fetch the third party build dependencies:

```bash
$ git submodule init
$ git submodule update
```

To build `spv`, you'll need autoconf, automake, and a recent (C++14) compiler:

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
 * libuv (version 1.x)

Other dependencies/third party libs:

 * The `picosha2.h` header file from
   [okdshin/PicoSHA2](https://github.com/okdshin/PicoSHA2) is included in this
   project.
 * This project includes [skypjack/uvw](https://github.com/skypjack/uvw) via a
   git submodule.

## Licensing

This code is free software, licensed under the GPL v3+.
