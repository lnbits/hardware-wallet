# Vendored dependencies

Dependencies are committed here to make firmware builds reproducible. Board
configuration and Bowser integration belong outside these directories so an
upstream dependency can be replaced without manually carrying local edits.

## TFT_eSPI

- Upstream: <https://github.com/Bodmer/TFT_eSPI>
- Version: 2.5.44
- Commit: `16e37595040eac69cd628e4bffb56fc30cad6299`
- Runtime source modifications: none

The vendored snapshot deliberately omits upstream examples, documentation,
CI metadata, and non-Arduino build/conversion tooling. Those paths are listed
in the repository `.gitignore`; the retained runtime source remains unchanged
from upstream.

The board-specific setup files live in `wallet/boards` and are selected using
TFT_eSPI's upstream `TFT_ESPI_USER_SETUP_PATH` configuration hook.

All current boards use this single display dependency. TFT_eSPI 2.5.44's
optimized C6 path does not compile against ESP32 Arduino core 3.3.x, so the
Waveshare ESP32-C6 profile selects TFT_eSPI's unmodified portable SPI backend.
That selection is contained in the board setup; the vendored library remains
an exact upstream snapshot.

## libwally-core

- Upstream: <https://github.com/ElementsProject/libwally-core>
- Release: 1.5.6 (`release_1.5.6`)
- Commit: `0c41f38fb1c201786e9c3ac9eae4f5f80c051399`
- Runtime source modifications: none

Bowser uses libwally directly for hashes, HMAC, PBKDF2, BIP39, BIP32,
secp256k1, addresses, and PSBT parsing/signing/serialization. Build details and
the pinned secp256k1-zkp revision are documented in `libwally/UPSTREAM.md`.
The Bitcoin-only, English-only amalgamated build is shared by every board.

## Other dependencies

ArduinoJson 6.19.0 matches its upstream source. QRCode is the modified
LNPoS-derived version. tiny-AES-c remains configured for AES-256 instead of its
upstream AES-128 default so existing encrypted wallet records remain readable.
