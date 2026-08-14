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
  String firmwareHash = getRunningFirmwareSha256();
  showFirmwareHash(firmwareHash);
  armInputForPrompt();

  while (true) {
    char key = pollInputKey();
    if (key == '#') {
      firmwareHash = "";
      return;
    }
    delay(10);
  }
}
