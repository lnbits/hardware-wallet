#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'EOF'
Usage: tools/build_installer_firmware.sh <target|all> [version]

Builds fresh firmware and replaces the matching local web-installer package.
"all" builds the three boards currently offered by the installer.
EOF
  exit 2
}

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
requested_target="${1:-}"
firmware_version="${2:-}"

case "${requested_target}" in
  lilygo_tdisplay|esp32_2432s028r|esp32_3248s035r|esp32_3248s035c|waveshare_esp32_c6_lcd_1_3)
    targets=("${requested_target}")
    ;;
  all)
    targets=(lilygo_tdisplay esp32_3248s035c waveshare_esp32_c6_lcd_1_3)
    ;;
  *)
    usage
    ;;
esac

if [[ -z "${firmware_version}" ]]; then
  source_version="$(sed -n 's/^#define BOWSER_FIRMWARE_VERSION "\([^"]*\)"$/\1/p' "${repository_root}/wallet/wallet.ino")"
  if [[ -z "${source_version}" ]]; then
    echo "Could not read BOWSER_FIRMWARE_VERSION from wallet/wallet.ino" >&2
    exit 1
  fi
  firmware_version="${source_version}-local"
fi

arduino_data_directory="$(arduino-cli config get directories.data)"
if [[ -z "${arduino_data_directory}" ]]; then
  echo "Could not determine the Arduino data directory" >&2
  exit 1
fi

for target in "${targets[@]}"; do
  case "${target}" in
    waveshare_esp32_c6_lcd_1_3)
      # ESP32 Arduino core 3.x emits boot_app0.bin into the build directory.
      # Unlike core 2.x, it does not ship the file under tools/partitions.
      boot_app0="${repository_root}/build/${target}/boot_app0.bin"
      ;;
    *)
      boot_app0="${arduino_data_directory}/packages/esp32/hardware/esp32/2.0.17/tools/partitions/boot_app0.bin"
      ;;
  esac

  build_directory="${repository_root}/build/${target}"
  package_directory="${repository_root}/installer/firmware/esp32/current/${target}"

  echo "Building ${target}..."
  "${repository_root}/tools/build_firmware.sh" "${target}" "${build_directory}"

  echo "Replacing local installer package for ${target}..."
  python3 "${repository_root}/tools/package_firmware.py" \
    "${target}" \
    "${firmware_version}" \
    "${build_directory}" \
    "${package_directory}" \
    --boot-app0 "${boot_app0}"

  build_hash="$(python3 "${repository_root}/tools/esp_image_hash.py" "${build_directory}/wallet.ino.bin")"
  package_hash="$(python3 "${repository_root}/tools/esp_image_hash.py" "${package_directory}/wallet.ino.bin")"
  published_hash="$(tr -d '[:space:]' < "${package_directory}/ESP_IMAGE_SHA256.txt")"

  if [[ "${build_hash}" != "${package_hash}" || "${build_hash}" != "${published_hash}" ]]; then
    echo "Firmware fingerprint verification failed for ${target}" >&2
    exit 1
  fi

  echo "Ready: ${target} ${firmware_version} ${build_hash}"
done
