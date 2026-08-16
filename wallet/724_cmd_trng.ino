/**
   @brief Display a live histogram of the ESP32 hardware RNG.

   The command first runs the same conditioned-output health test used before
   seed generation. It then maps fresh hardware RNG words without modulo bias
   into 100 bins and draws each observation on the device. It collects a fixed
   5,000 observations, for an expected count of 50 per bin, then calculates a
   chi-squared goodness-of-fit statistic with 99 degrees of freedom. This is a
   distribution diagnostic, not proof that future RNG output is unpredictable.

   No raw RNG output or histogram data leaves the device. The response contains
   only the sample count, chi-squared statistic, observed range, and verdict.

   @return CommandResponse
      - `/trng 1 5000 <chi-squared> <minimum> <maximum> <verdict>` as soon as
        sampling and analysis complete.
      - `/trng 0 <reason>` if validation cannot run.
*/

const uint16_t TRNG_VISUAL_BIN_COUNT = 100;
const uint16_t TRNG_VISUAL_SAMPLE_COUNT = 5000;
const uint8_t TRNG_VISUAL_GRAPH_MAX_COUNT = 80;
const float TRNG_CHI_SQUARED_LOWER = 61.137f;
const float TRNG_CHI_SQUARED_UPPER = 148.230f;

uint8_t nextTrngVisualBin() {
  // UINT32_MAX + 1 is not a multiple of 100. Reject its short upper tail so
  // every displayed number has exactly the same probability.
  const uint32_t limit = UINT32_MAX - (UINT32_MAX % TRNG_VISUAL_BIN_COUNT);
  uint32_t sample = 0;
  do {
    sample = esp_random();
  } while (sample >= limit);
  return uint8_t(sample % TRNG_VISUAL_BIN_COUNT);
}

int16_t trngVisualBinX(uint16_t bin) {
  return int32_t(bin) * tft.width() / TRNG_VISUAL_BIN_COUNT;
}

void drawTrngVisualAxis(int16_t labelY, uint8_t labelTextSize) {
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(labelTextSize);

  const uint8_t labels[] = {1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  for (uint8_t index = 0; index < sizeof(labels); index++) {
    String label = String(labels[index]);
    int16_t centerX = int32_t(labels[index] - 1) * tft.width() /
      TRNG_VISUAL_BIN_COUNT;
    int16_t labelX = centerX - uiTextPixelWidth(label, labelTextSize) / 2;
    int16_t maximumLabelX = tft.width() -
      uiTextPixelWidth(label, labelTextSize);
    if (labelX < 0) labelX = 0;
    if (labelX > maximumLabelX) labelX = maximumLabelX;
    tft.setCursor(labelX, labelY);
    tft.print(label);
  }
}

void drawTrngVisualObservation(
  uint8_t bin,
  uint16_t count,
  int16_t graphBottom,
  int16_t observationHeight
) {
  int16_t x = trngVisualBinX(bin);
  int16_t nextX = trngVisualBinX(bin + 1);
  int16_t width = nextX - x - 1;
  if (width < 1) width = 1;
  uint16_t visibleCount = count > TRNG_VISUAL_GRAPH_MAX_COUNT
    ? TRNG_VISUAL_GRAPH_MAX_COUNT
    : count;
  int16_t y = graphBottom - int16_t(visibleCount) * observationHeight;
  tft.fillRect(x, y, width, observationHeight, TFT_GREEN);
}

float trngChiSquared(const uint16_t *counts) {
  const float expected = float(TRNG_VISUAL_SAMPLE_COUNT) /
    TRNG_VISUAL_BIN_COUNT;
  float statistic = 0.0f;
  for (uint16_t bin = 0; bin < TRNG_VISUAL_BIN_COUNT; bin++) {
    float difference = float(counts[bin]) - expected;
    statistic += difference * difference / expected;
  }
  return statistic;
}

CommandResponse executeTrng(String commandData) {
  commandData.trim();
  if (commandData != "") {
    sendCommandOutput(COMMAND_TRNG, "0 invalid_args");
    return {"TRNG histogram", "No arguments expected"};
  }

  uint8_t healthProbe[32] = {0};
  bool healthy = deriveHealthyHardwareEntropy(healthProbe, sizeof(healthProbe));
  clearSensitiveBytes(healthProbe, sizeof(healthProbe));
  if (!healthy) {
    showMessage("TRNG check failed", "Do not create a wallet");
    sendCommandOutput(COMMAND_TRNG, "0 health_failed");
    return {"TRNG check failed", "Do not create a wallet"};
  }

  uint16_t counts[TRNG_VISUAL_BIN_COUNT] = {0};
  beginUiScreen(UiControls::ContinueOnly);
  int16_t contentHeight = uiContentHeight();
  String title = "TRNG distribution 1-100";
  uint8_t titleSize = uiFittedTextSize(title, 2, tft.width() - 4);
  int16_t titleHeight = uiTextPixelHeight(titleSize);
  const uint8_t labelTextSize = 1;
  int16_t labelY = contentHeight - uiTextPixelHeight(labelTextSize);
  int16_t graphTop = titleHeight + 5;
  int16_t availableGraphHeight = labelY - graphTop - 3;
  int16_t observationHeight = availableGraphHeight /
    TRNG_VISUAL_GRAPH_MAX_COUNT;
  if (observationHeight < 1) observationHeight = 1;
  if (observationHeight > 3) observationHeight = 3;
  int16_t graphHeight = observationHeight * TRNG_VISUAL_GRAPH_MAX_COUNT;
  int16_t graphBottom = graphTop + graphHeight;

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(titleSize);
  tft.setCursor(2, 0);
  tft.print(title);
  tft.drawFastHLine(0, graphBottom, tft.width(), TFT_DARKGREEN);
  drawTrngVisualAxis(labelY, labelTextSize);

  uint32_t sampleCount = 0;
  bootloader_random_enable();
  while (sampleCount < TRNG_VISUAL_SAMPLE_COUNT) {
    uint8_t bin = nextTrngVisualBin();
    counts[bin]++;
    sampleCount++;
    drawTrngVisualObservation(
      bin,
      counts[bin],
      graphBottom,
      observationHeight
    );
    delay(1);
  }
  bootloader_random_disable();

  uint16_t minimumCount = counts[0];
  uint16_t maximumCount = counts[0];
  for (uint16_t bin = 1; bin < TRNG_VISUAL_BIN_COUNT; bin++) {
    if (counts[bin] < minimumCount) minimumCount = counts[bin];
    if (counts[bin] > maximumCount) maximumCount = counts[bin];
  }
  float chiSquared = trngChiSquared(counts);
  bool looksHealthy =
    chiSquared >= TRNG_CHI_SQUARED_LOWER &&
    chiSquared <= TRNG_CHI_SQUARED_UPPER;

  tft.fillRect(0, 0, tft.width(), titleHeight + 2, TFT_BLACK);
  String verdict = looksHealthy ? "healthy" : "unexpected";
  String complete = "Chi2 " + String(chiSquared, 1) + " " + verdict;
  tft.setTextColor(looksHealthy ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.setTextSize(uiFittedTextSize(complete, 2, tft.width() - 4));
  tft.setCursor(2, 0);
  tft.print(complete);

  String result = "1 " + String(sampleCount) + " " +
    String(chiSquared, 2) + " " + String(minimumCount) + " " +
    String(maximumCount) + " " + verdict;
  sendCommandOutput(COMMAND_TRNG, result);

  // ContinueOnly has no cancel action: ignore keypad/back-button input and
  // leave the histogram visible until the affirmative control is used. The
  // aggregate result has already been returned to WebSerial or the SD file.
  armInputForPrompt();
  while (waitForKeypadKey() != '#') {
    delay(10);
  }

  // Give every input type visible feedback that Continue was accepted without
  // replacing the histogram with a redundant command-complete screen.
  logo(0);
  return {"", ""};
}
