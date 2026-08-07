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

  setupKeypad();
  while (true) {
    char key = scanKeypad();
    bool buttonPressed = digitalRead(global.button1Pin) == LOW;
    if (global.button2Pin != global.button1Pin) {
      buttonPressed = buttonPressed || digitalRead(global.button2Pin) == LOW;
    }

    if (key == '#' || buttonPressed) {
      while (
        scanKeypad() != 0 ||
        digitalRead(global.button1Pin) == LOW ||
        (
          global.button2Pin != global.button1Pin &&
          digitalRead(global.button2Pin) == LOW
        )
      ) {
        delay(10);
      }
      firmwareHash = "";
      return;
    }
    delay(10);
  }
}
