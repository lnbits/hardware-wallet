const char *FIRMWARE_VERIFICATION_FILE = "/firmware.fp";
const uint8_t FIRMWARE_FINGERPRINT_ENDING_LENGTH = 8;

String getRunningFirmwareBuildId() {
  const esp_partition_t *partition = esp_ota_get_running_partition();
  if (partition == NULL) return "";

  esp_app_desc_t description = {};
  if (esp_ota_get_partition_description(partition, &description) != ESP_OK) {
    return "";
  }
  return toHex(description.app_elf_sha256, sizeof(description.app_elf_sha256));
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

bool loadStoredFirmwareFingerprint(String currentBuildId) {
  if (currentBuildId.length() != 64 || !isStrictHex(currentBuildId)) {
    return false;
  }

  FileData record = readFile(FlashFS, FIRMWARE_VERIFICATION_FILE);
  if (!record.success) return false;

  String storedBuildId = getLineAtPosition(record.data, 0);
  String storedEnding = getLineAtPosition(record.data, 1);
  storedBuildId.trim();
  storedEnding.trim();
  if (
    storedBuildId != currentBuildId ||
    storedEnding.length() != FIRMWARE_FINGERPRINT_ENDING_LENGTH ||
    !isStrictHex(storedEnding)
  ) return false;

  global.firmwareFingerprintEnding = storedEnding;
  return true;
}

void storeFirmwareFingerprint(String currentBuildId, String firmwareHash) {
  if (
    currentBuildId.length() != 64 ||
    !isStrictHex(currentBuildId) ||
    firmwareHash.length() != 64 ||
    !isStrictHex(firmwareHash)
  ) return;

  String ending = firmwareHash.substring(
    firmwareHash.length() - FIRMWARE_FINGERPRINT_ENDING_LENGTH
  );
  writeFile(
    FlashFS,
    FIRMWARE_VERIFICATION_FILE,
    currentBuildId + "\n" + ending + "\n"
  );
  if (!loadStoredFirmwareFingerprint(currentBuildId)) {
    logInfo("Firmware fingerprint record could not be verified");
  }
}

void verifyFirmwareIfNeeded() {
  String currentBuildId = getRunningFirmwareBuildId();
  if (loadStoredFirmwareFingerprint(currentBuildId)) return;

  String firmwareHash = getRunningFirmwareSha256();
  showFirmwareHash(firmwareHash);
  armInputForPrompt();

  while (true) {
    char key = pollInputKey();
    if (key == '#') {
      if (firmwareHash.length() == 64 && isStrictHex(firmwareHash)) {
        global.firmwareFingerprintEnding = firmwareHash.substring(
          firmwareHash.length() - FIRMWARE_FINGERPRINT_ENDING_LENGTH
        );
        storeFirmwareFingerprint(currentBuildId, firmwareHash);
      }
      firmwareHash = "";
      currentBuildId = "";
      return;
    }
    delay(10);
  }
}
