#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${repository_root}/build"
installer_current="${repository_root}/installer/firmware/esp32/current"
arduino_data_root="${ARDUINO_DATA_DIR:-${HOME}/.arduino15}"

classic_targets=(
  lilygo_tdisplay
  esp32_3248s035c
)
c6_target="waveshare_esp32_c6_lcd_1_3"

firmware_version="$({
  sed -n 's/^#define BOWSER_FIRMWARE_VERSION "\([^"]*\)"$/\1/p' \
    "${repository_root}/wallet/wallet.ino"
} | head -n 1)"
if [[ -z "${firmware_version}" ]]; then
  echo "Could not read BOWSER_FIRMWARE_VERSION" >&2
  exit 1
fi

# Identify the exact local source tree, including uncommitted and untracked
# firmware files. The manifest version therefore changes whenever a build
# input changes, even before it is committed.
source_digest="$({
  cd "${repository_root}"
  {
    find wallet libraries -type f -print0
    printf '%s\0' tools/build_firmware.sh tools/package_firmware.py
  } | sort -z | xargs -0 sha256sum
} | sha256sum | cut -c1-12)"
package_version="${firmware_version}-local.${source_digest}"

if [[ "${build_root}" != "${repository_root}/build" ]]; then
  echo "Refusing to clean unexpected build path: ${build_root}" >&2
  exit 1
fi
if [[ "${installer_current}" != "${repository_root}/installer/firmware/esp32/current" ]]; then
  echo "Refusing to clean unexpected installer path: ${installer_current}" >&2
  exit 1
fi

# Both trees contain generated files only. Remove them before doing any work:
# if a later build fails, the installer is missing rather than silently
# continuing to flash firmware from a previous checkout.
rm -rf -- "${build_root}" "${installer_current}"
mkdir -p -- "${build_root}" "${installer_current}"

package_target() {
  local target="$1"
  local core_version="$2"
  local boot_app0="${arduino_data_root}/packages/esp32/hardware/esp32/${core_version}/tools/partitions/boot_app0.bin"
  local target_build="${build_root}/${target}"
  local target_package="${installer_current}/${target}"

  "${repository_root}/tools/build_firmware.sh" "${target}" "${target_build}"
  python3 "${repository_root}/tools/package_firmware.py" \
    "${target}" \
    "${package_version}" \
    "${target_build}" \
    "${target_package}" \
    --boot-app0 "${boot_app0}"

  cmp --silent \
    "${target_build}/wallet.ino.bin" \
    "${target_package}/wallet.ino.bin"
  python3 -m json.tool "${target_package}/manifest.json" >/dev/null

  local calculated_hash
  local packaged_hash
  calculated_hash="$(
    python3 "${repository_root}/tools/esp_image_hash.py" \
      "${target_package}/wallet.ino.bin"
  )"
  packaged_hash="$(tr -d '\r\n' < "${target_package}/ESP_IMAGE_SHA256.txt")"
  if [[ "${calculated_hash}" != "${packaged_hash}" ]]; then
    echo "Packaged hash mismatch for ${target}" >&2
    exit 1
  fi

  printf 'Packaged %s %s %s\n' "${target}" "${package_version}" "${packaged_hash}"
}

for target in "${classic_targets[@]}"; do
  package_target "${target}" "2.0.17"
done
package_target "${c6_target}" "3.3.11"

printf 'Installer firmware is current: %s\n' "${installer_current}"
