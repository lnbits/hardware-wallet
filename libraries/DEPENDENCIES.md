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

## Existing cryptographic and QR dependencies

The display-library upgrade does not modify uBitcoin, QRCode, tiny-AES-c, or
ArduinoJson. ArduinoJson 6.19.0 matches its upstream source. uBitcoin contains
historical project changes and upgrades, QRCode is the modified LNPoS-derived
version, and tiny-AES-c is configured for AES-256 instead of its upstream
AES-128 default. Preserve and audit those differences before replacing these
dependencies with newer upstream snapshots.

ESP32 core 3.3.x declares `esp_random()` in `esp_random.h`. The C6 build forces
that official core header into C translation units so the existing uBitcoin
snapshot continues to call the hardware RNG without modifying the dependency.
