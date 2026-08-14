#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: $0 <lilygo_tdisplay|esp32_2432s028r|waveshare_esp32_c6_lcd_1_3> [build-directory]" >&2
  exit 2
}

target="${1:-}"
build_directory="${2:-build/${target}}"
repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "${target}" in
  lilygo_tdisplay)
    core_version="2.0.17"
    # Keep the project's established board definition. Although the ESP32 core
    # also exposes a LilyGo T-Display entry, its flash settings are not the
    # settings used by the hardware this firmware has historically targeted.
    fqbn="esp32:esp32:ttgo-lora32"
    profile="lilygo_tdisplay"
    display_backend="display_backends/tft_espi.h"
    tft_setup="${repository_root}/wallet/boards/${profile}/tft_setup.h"
    compiler_c_flags="-MMD -c"
    ;;
  esp32_2432s028r)
    core_version="2.0.17"
    fqbn="esp32:esp32:esp32"
    profile="esp32_2432s028r"
    display_backend="display_backends/tft_espi.h"
    tft_setup="${repository_root}/wallet/boards/${profile}/tft_setup.h"
    compiler_c_flags="-MMD -c"
    ;;
  waveshare_esp32_c6_lcd_1_3)
    core_version="3.3.11"
    fqbn="esp32:esp32:esp32c6:CDCOnBoot=cdc"
    profile="waveshare_esp32_c6_lcd_1_3"
    display_backend="display_backends/tft_espi.h"
    tft_setup="${repository_root}/wallet/boards/${profile}/tft_setup.h"
    # ESP32 core 3.3.x moved the esp_random declaration out of esp_system.h.
    # Keep the vendored uBitcoin snapshot unchanged and provide the core's
    # canonical declaration to C translation units at build time.
    compiler_c_flags="-MMD -c -include esp_random.h"
    ;;
  *)
    usage
    ;;
esac

installed_version="$(arduino-cli core list | awk '$1 == "esp32:esp32" { print $2 }')"
if [[ "${installed_version}" != "${core_version}" ]]; then
  arduino-cli core install "esp32:esp32@${core_version}"
fi

mkdir -p "${build_directory}"

compiler_flags="-MMD -c -DBOWSER_BOARD_PROFILE=boards/${profile}/profile.h -DBOWSER_DISPLAY_BACKEND=${display_backend}"
if [[ -n "${tft_setup}" ]]; then
  compiler_flags+=" -DTFT_ESPI_USER_SETUP_PATH=\"${tft_setup}\""
fi

# Do not inject profiles through build.extra_flags: both pinned ESP32 cores use
# that property for target-defining flags such as -DESP32 and USB mode. The
# compiler.cpp property retains its required compile/dependency flags here.
arduino-cli compile --clean \
  --fqbn "${fqbn}" \
  --libraries "${repository_root}/libraries" \
  --build-path "${build_directory}" \
  --build-property "compiler.c.extra_flags=${compiler_c_flags}" \
  --build-property "compiler.cpp.extra_flags=${compiler_flags}" \
  "${repository_root}/wallet"
