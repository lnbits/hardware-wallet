# Bowser HWW web installer

## Pinned browser flashers

The installer loads the unmodified
[`esp-web-tools@10.4.0`](https://www.npmjs.com/package/esp-web-tools/v/10.4.0)
release from unpkg for every supported board.

Before changing this pin, physically verify automatic connection and flashing
without using the BOOT button on the T-Display, ESP32-2432S028R,
ESP32-3248S035R, ESP32-3248S035C, and Waveshare ESP32-C6-LCD-1.3.

## Build and package local firmware

Complete the Arduino CLI and Espressif board-index setup in the main
[build instructions](../README.md#build-from-source-with-arduino-cli), then run
these commands from the repository root. The build script installs the pinned
ESP32 Arduino core required by each target, so a separate manual core
installation is not required. It deliberately uses these target definitions:

| Target | ESP32 Arduino core | FQBN |
| --- | --- | --- |
| `lilygo_tdisplay` | `esp32:esp32@2.0.17` | `esp32:esp32:ttgo-lora32` |
| `esp32_2432s028r` | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `esp32_3248s035r` | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `esp32_3248s035c` | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `waveshare_esp32_c6_lcd_1_3` | `esp32:esp32@3.3.11` | `esp32:esp32:esp32c6:CDCOnBoot=cdc` |

Set a version label for the local manifests. It does not need to be a released
version when testing locally:

```bash
firmware_version=0.7.1-local
```

Build and package the classic ESP32 targets first, while core 2.0.17 is
installed:

```bash
./tools/build_firmware.sh \
  lilygo_tdisplay \
  build/lilygo_tdisplay

python3 tools/package_firmware.py \
  lilygo_tdisplay \
  "${firmware_version}" \
  build/lilygo_tdisplay \
  installer/firmware/esp32/current/lilygo_tdisplay \
  --boot-app0 "${HOME}/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin"

./tools/build_firmware.sh \
  esp32_2432s028r \
  build/esp32_2432s028r

python3 tools/package_firmware.py \
  esp32_2432s028r \
  "${firmware_version}" \
  build/esp32_2432s028r \
  installer/firmware/esp32/current/esp32_2432s028r \
  --boot-app0 "${HOME}/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin"

./tools/build_firmware.sh \
  esp32_3248s035r \
  build/esp32_3248s035r

python3 tools/package_firmware.py \
  esp32_3248s035r \
  "${firmware_version}" \
  build/esp32_3248s035r \
  installer/firmware/esp32/current/esp32_3248s035r \
  --boot-app0 "${HOME}/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin"

./tools/build_firmware.sh \
  esp32_3248s035c \
  build/esp32_3248s035c

python3 tools/package_firmware.py \
  esp32_3248s035c \
  "${firmware_version}" \
  build/esp32_3248s035c \
  installer/firmware/esp32/current/esp32_3248s035c \
  --boot-app0 "${HOME}/.arduino15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin"
```

Build and package the C6 target last. Its build switches the installed core to
3.3.11:

```bash
./tools/build_firmware.sh \
  waveshare_esp32_c6_lcd_1_3 \
  build/waveshare_esp32_c6_lcd_1_3

python3 tools/package_firmware.py \
  waveshare_esp32_c6_lcd_1_3 \
  "${firmware_version}" \
  build/waveshare_esp32_c6_lcd_1_3 \
  installer/firmware/esp32/current/waveshare_esp32_c6_lcd_1_3 \
  --boot-app0 "${HOME}/.arduino15/packages/esp32/hardware/esp32/3.3.11/tools/partitions/boot_app0.bin"
```

`package_firmware.py` is silent when it succeeds. Each target directory should
then contain `manifest.json`, `wallet.ino.bin`, the bootloader and partition
images, and `ESP_IMAGE_SHA256.txt`. Validate a package before flashing it
(replace `lilygo_tdisplay` with the target being tested):

```bash
python3 -m json.tool \
  installer/firmware/esp32/current/lilygo_tdisplay/manifest.json \
  > /dev/null

python3 tools/esp_image_hash.py \
  installer/firmware/esp32/current/lilygo_tdisplay/wallet.ino.bin

cat installer/firmware/esp32/current/lilygo_tdisplay/ESP_IMAGE_SHA256.txt
```

The two printed fingerprints must match. Build output and locally packaged
firmware are intentionally ignored by Git.

## Test locally

The installer uses shared styles and images from `lnbits.github.io`. From the
root of this repository, start a local server that serves repository files
first and proxies missing shared assets to the deployed LNbits site:

```bash
npm exec --yes http-server -- . \
  -p 8000 \
  -a 127.0.0.1 \
  -c-1 \
  -P https://lnbits.github.io
```

Open the installer in Chrome, Chromium, or another Web Serial-compatible
browser:

<http://localhost:8000/installer/>

Use `localhost` rather than `0.0.0.0` so the browser treats the page as a
secure context and permits Web Serial. Keep an internet connection while
testing so the server can retrieve the shared LNbits assets.

A plain `python3 -m http.server` serves the local installer files, but cannot
provide the shared `/assets` files used by the GitHub Pages site, so the page
will appear unstyled.

Press `Ctrl+C` in the terminal to stop the server.
