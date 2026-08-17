# Bowser HWW web installer

## Pinned browser flashers

The installer loads the unmodified
[`esp-web-tools@10.4.0`](https://www.npmjs.com/package/esp-web-tools/v/10.4.0)
release from unpkg for every available board.

Before changing this pin, verify connection and flashing on every available
board. The T-Display and ESP32-3248S035C must continue to connect automatically
without using their BOOT buttons. The other profiles are coming soon.

## Build and package local firmware

Complete the Arduino CLI and Espressif board-index setup in the main
[build instructions](../README.md#build-from-source-with-arduino-cli), then run
one command from the repository root:

```bash
./tools/rebuild_installer_firmware.sh
```

This is intentionally a full, clean rebuild. It removes the generated
`build/` tree and `installer/firmware/esp32/current/` package before compiling
anything. If compilation fails, the local installer is left incomplete and
cannot silently flash firmware from an earlier checkout. It then builds and
packages every selectable board, validates each manifest and firmware hash,
and confirms that the packaged application is byte-for-byte identical to the
fresh build.

The local manifest version includes a fingerprint of the exact firmware input
tree, including uncommitted and untracked files. For example:

```text
0.8.1-local.12ab34cd56ef
```

Changing a firmware input or checking out different source changes that
fingerprint. The script installs the pinned ESP32 Arduino core required by
each target, so a separate manual core installation is not required. It uses
these target definitions:

| Target | Status | ESP32 Arduino core | FQBN |
| --- | --- | --- | --- |
| `lilygo_tdisplay` | Available | `esp32:esp32@2.0.17` | `esp32:esp32:ttgo-lora32` |
| `esp32_2432s028r` | **Coming soon** | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `esp32_3248s035r` | **Coming soon** | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `esp32_3248s035c` | Available | `esp32:esp32@2.0.17` | `esp32:esp32:esp32` |
| `waveshare_esp32_c6_lcd_1_3` | Available | `esp32:esp32@3.3.11` | `esp32:esp32:esp32c6:CDCOnBoot=cdc` |

Coming-soon profiles remain available through `tools/build_firmware.sh`, but
are deliberately excluded from the local installer until they become
selectable. Generated build output and the complete `current` package are
ignored by Git; historical version directories are release archives and are
never referenced by the current installer.

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
