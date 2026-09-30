#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: $0 <lilygo_tdisplay|esp32_2432s028r|esp32_3248s035r|esp32_3248s035c|waveshare_esp32_c6_lcd_1_3|lilygo_tdisplay_s3_amoled> [build-directory]" >&2
  exit 2
}

target="${1:-}"
build_directory="${2:-build/${target}}"
repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
wally_vendor="${repository_root}/libraries/libwally/vendor"

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
  esp32_3248s035r)
    core_version="2.0.17"
    fqbn="esp32:esp32:esp32"
    profile="esp32_3248s035r"
    display_backend="display_backends/tft_espi.h"
    tft_setup="${repository_root}/wallet/boards/${profile}/tft_setup.h"
    compiler_c_flags="-MMD -c"
    ;;
  esp32_3248s035c)
    core_version="2.0.17"
    fqbn="esp32:esp32:esp32"
    profile="esp32_3248s035c"
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
    compiler_c_flags="-MMD -c"
    ;;
  lilygo_tdisplay_s3_amoled)
    core_version="2.0.17"
    fqbn="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc"
    profile="lilygo_tdisplay_s3_amoled"
    display_backend="display_backends/arduino_gfx_rm67162.h"
    tft_setup="${repository_root}/wallet/boards/${profile}/tft_setup.h"
    compiler_c_flags="-MMD -c"
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

wally_flags="-DWALLY_CORE_BUILD=1 -DBUILD_MINIMAL=1 -DWALLY_ABI_NO_ELEMENTS=1"
wally_flags+=" -DECMULT_WINDOW_SIZE=4 -DCOMB_BLOCKS=2 -DCOMB_TEETH=5"
wally_flags+=" -I${wally_vendor} -I${wally_vendor}/src -I${wally_vendor}/include"
wally_flags+=" -I${wally_vendor}/src/ccan -I${wally_vendor}/src/secp256k1/include"
wally_flags+=" -fno-strict-aliasing"

compiler_c_flags+=" ${wally_flags}"
compiler_flags="-MMD -c ${wally_flags} -DBOWSER_BOARD_PROFILE=boards/${profile}/profile.h -DBOWSER_DISPLAY_BACKEND=${display_backend}"
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
