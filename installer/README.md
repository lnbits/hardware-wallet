# Bowser Wallet web installer

## Pinned browser flashers

The installer loads the unmodified
[`esp-web-tools@10.4.0`](https://www.npmjs.com/package/esp-web-tools/v/10.4.0)
release from unpkg for every available board.

Before changing this pin, verify connection and flashing on every available
board. The T-Display, ESP32-2432S028R / E32R28T, and ESP32-3248S035C must
continue to connect automatically without using their BOOT buttons. The
ESP32-3248S035R profile is coming soon.

## Build and package local firmware

Complete the Arduino CLI and Espressif board-index setup in the main
[build instructions](../README.md#build-from-source-with-arduino-cli), then run
these commands from the repository root. The build script installs the pinned
ESP32 Arduino core required by each target, so a separate manual core
installation is not required. It deliberately uses these target definitions:

| Target | Status | ESP32 Arduino core | FQBN |
| --- | --- | --- | --- |
| `lilygo_tdisplay` | Available | `esp32:esp32@2.0.17` | `esp32:esp32:ttgo-lora32` |
| `esp32_2432s028r` | Available | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `esp32_3248s035r` | **Coming soon** | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `esp32_3248s035c` | Available | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `waveshare_esp32_c6_lcd_1_3` | Available | `esp32:esp32@3.3.11` | `esp32:esp32:esp32c6:CDCOnBoot=cdc` |

Use the combined build command when testing through the web installer:

```bash
./tools/build_installer_firmware.sh lilygo_tdisplay
./tools/build_installer_firmware.sh esp32_2432s028r
./tools/build_installer_firmware.sh esp32_3248s035c
./tools/build_installer_firmware.sh waveshare_esp32_c6_lcd_1_3
```

To rebuild all four boards currently offered by the installer, run:

```bash
./tools/build_installer_firmware.sh all
```

The version defaults to the source firmware version with `-local` appended. An
explicit local label can be supplied as the second argument:

```bash
./tools/build_installer_firmware.sh waveshare_esp32_c6_lcd_1_3 0.8.1-local-psbt
```

The command performs a clean source build, stages a complete package, replaces
the old target package, and verifies that the build image, installer image, and
published fingerprint are identical. It prints `Ready` and the fingerprint
only after all checks pass. If compilation or packaging fails, it does not
replace the previous installer package. Build output and locally packaged
firmware are intentionally ignored by Git.

`build_firmware.sh` remains available for direct Arduino CLI uploads, and
`package_firmware.py` remains available to release automation. Do not use the
compile-only command when the next test will flash through the local web
installer.

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
