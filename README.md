This is my attempt at writing a from-scratch Bitcoin SPV client. The
implementation language is C++. This is purely for me to learn about the Bitcoin
protocol: you shouldn't actually use this, and certainly not on mainnet.

To compile you'll need autoconf and automake:

```bash
$ ./autogen.sh
$ ./configure
$ make
```

The `make` command will produce an executable at `src/spv`.

License is GPL v3+.
