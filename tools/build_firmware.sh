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
    ;;
  esp32_2432s028r)
    core_version="2.0.17"
    fqbn="esp32:esp32:esp32"
    profile="esp32_2432s028r"
    ;;
  waveshare_esp32_c6_lcd_1_3)
    core_version="3.0.1"
    fqbn="esp32:esp32:esp32c6:CDCOnBoot=cdc"
    profile="waveshare_esp32_c6_lcd_1_3"
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

arduino-cli compile --clean \
  --fqbn "${fqbn}" \
  --libraries "${repository_root}/libraries" \
  --build-path "${build_directory}" \
  --build-property "build.extra_flags=-DBOWSER_BOARD_PROFILE=boards/${profile}/profile.h -DTFT_ESPI_USER_SETUP_PATH=../../wallet/boards/${profile}/tft_setup.h" \
  "${repository_root}/wallet"
