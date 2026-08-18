# Vendored libwally-core

- Upstream: <https://github.com/ElementsProject/libwally-core>
- Release: `release_1.5.6`
- Commit: `0c41f38fb1c201786e9c3ac9eae4f5f80c051399`
- secp256k1-zkp commit: `45f6f0f158c5ae80a2c8a53398ea4adbf19af6dc`

The `vendor` directory contains the upstream public headers and runtime source.
Tests, language bindings, examples, and repository metadata are omitted. The
runtime source is unmodified. `src/libwally_combined.c` uses libwally's
supported amalgamated build.

Bowser builds with:

- `WALLY_ABI_NO_ELEMENTS=1` (Bitcoin-only ABI);
- `BUILD_MINIMAL=1` (English-only BIP39 wordlist at compile time);
- `ECMULT_WINDOW_SIZE=4`; and
- `COMB_BLOCKS=2`, `COMB_TEETH=5` (the upstream 2 KB signing table).

The smaller secp256k1 tables trade some speed for flash space. Wallet and PSBT
benchmarks must be checked on both Xtensa and RISC-V hardware when changing
these values.
