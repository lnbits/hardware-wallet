# Board profiles

Bowser keeps one application and selects one board profile at compile time.
Application code consumes the `BOARD` capability object and the common input
API; it does not contain a growing set of board-name `#ifdef` branches.

Release status:

| Target | Status |
| --- | --- |
| `lilygo_tdisplay` | Available |
| `esp32_2432s028r` | **Coming soon** |
| `esp32_3248s035r` | **Coming soon** |
| `esp32_3248s035c` | Available |
| `waveshare_esp32_c6_lcd_1_3` | **Coming soon** |

Coming-soon profiles remain available for local development, but are not
published by the web installer release workflow.

Each directory contains:

- `profile.h`: identity, rotation, backlight, buttons, keypad, microSD, and
  touch capabilities. `uiTextSizeBoost` scales built-in fonts for larger
  displays; each rendered label is still reduced automatically if needed to
  fit its available width.
- A `tft_setup.h` display configuration selected through TFT_eSPI's upstream
  setup hook.

The build target supplies the profile and display backend using compiler
flags. TFT_eSPI's
upstream `TFT_ESPI_USER_SETUP_PATH` hook expects its value to be a string
literal. For example:

```text
-DBOWSER_BOARD_PROFILE=boards/my_board/profile.h
-DBOWSER_DISPLAY_BACKEND=display_backends/tft_espi.h
-DTFT_ESPI_USER_SETUP_PATH="/absolute/path/to/wallet/boards/my_board/tft_setup.h"
```

## Adding a board

1. Copy the closest directory and give it a stable lowercase target ID.
2. Fill out its `BoardProfile` and display configuration from the
   manufacturer's schematic, not from a product listing.
3. Return the correct `SPIClass` from `boardSdSpi()`. Sharing one SPI bus is
   fine when the board is wired that way; separate buses should use separate
   instances.
4. Add the target, pinned ESP32 core, and FQBN to `tools/build_firmware.sh`.
5. Add its chip family and bootloader offset to `tools/package_firmware.py`.
6. Add it to the installer selector and CI target lists.
7. Compile it in CI and test display orientation, backlight polarity, SD
   access, every input action, and the full signing-review path on real
   hardware.

Input is capability-based:

- `hasMatrixKeypad` provides digits, accept, and cancel.
- `hasSdCard` enables air-gapped command discovery on the configured SPI bus.
- `hasTouchscreen` provides contextual touch buttons for any touch backend.
- `xpt2046.enabled` selects resistive XPT2046 input. These boards
  use first-boot calibration; `xpt2046.sharesDisplaySpi` uses TFT_eSPI's SPI
  instance where the display and touch controller share one bus.
- `gt911.enabled` selects capacitive GT911 input on its configured I2C pins.
  Its profile owns the native dimensions and axis transforms, keeping board
  orientation details out of wallet and command logic.
- A one-button profile with `singleButtonLongPressCancels` maps a tap to
  accept/next and a long press to cancel/back.
- Two-button boards map the two buttons directly.

All sources require a stable press and stable release before producing one
logical key event. Sensitive prompts additionally wait for every control to be
released before they arm, preventing a press started on the previous screen
from authorizing the next one.

Screen layouts derive their geometry from `tft.width()`, `tft.height()`, and
the current touch footer height. This allows different resolutions without
per-board coordinates in command or wallet logic.
