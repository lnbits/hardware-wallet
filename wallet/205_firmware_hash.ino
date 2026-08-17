const char *CONFIRMED_FIRMWARE_BUILD_FILE = "/firmware.verified";

String getRunningFirmwareBuildId() {
  const esp_partition_t *partition = esp_ota_get_running_partition();
  if (partition == NULL) return "";

  esp_app_desc_t description = {};
  if (esp_ota_get_partition_description(partition, &description) != ESP_OK) {
    return "";
  }

  bool hasNonZeroByte = false;
  for (size_t i = 0; i < sizeof(description.app_elf_sha256); i++) {
    if (description.app_elf_sha256[i] != 0) {
      hasNonZeroByte = true;
      break;
    }
  }
  if (!hasNonZeroByte) return "";

  return toHex(description.app_elf_sha256,
               sizeof(description.app_elf_sha256));
}

bool firmwareBuildWasConfirmed(const String &buildId) {
  if (buildId.length() != 64) return false;

  FileData confirmedBuild = readFile(FlashFS,
                                     CONFIRMED_FIRMWARE_BUILD_FILE);
  if (!confirmedBuild.success) return false;
  confirmedBuild.data.trim();
  return confirmedBuild.data == buildId;
}

void rememberConfirmedFirmwareBuild(const String &buildId) {
  if (buildId.length() != 64) return;
  writeFile(FlashFS, CONFIRMED_FIRMWARE_BUILD_FILE, buildId + "\n");
}

String getRunningFirmwareSha256() {
  const esp_partition_t *partition = esp_ota_get_running_partition();
  if (partition == NULL) return "";

  uint8_t digest[32] = {0};
  if (esp_partition_get_sha256(partition, digest) != ESP_OK) {
    clearSensitiveBytes(digest, sizeof(digest));
    return "";
  }

  String firmwareHash = toHex(digest, sizeof(digest));
  clearSensitiveBytes(digest, sizeof(digest));
  return firmwareHash;
}

void waitForFirmwareHashConfirmation() {
  String buildId = getRunningFirmwareBuildId();
  if (firmwareBuildWasConfirmed(buildId)) {
    buildId = "";
    return;
  }

  String firmwareHash = getRunningFirmwareSha256();
  showFirmwareHash(firmwareHash);
  armInputForPrompt();

  while (true) {
    char key = pollInputKey();
    if (key == '#') {
      if (firmwareHash.length() == 64) {
        rememberConfirmedFirmwareBuild(buildId);
      }
      buildId = "";
      firmwareHash = "";
      return;
    }
    delay(10);
  }
}
