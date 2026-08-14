
PSBT parseBase64Psbt(String psbtBase64) {
  PSBT psbt;
  psbt.parseBase64(psbtBase64);
  return psbt;
}

void printOutputDetails(PSBT psbt, HDPrivateKey hd, int index, const Network * network) {
  beginUiScreen(global.hasCommandsFile ? UiControls::ConfirmCancel : UiControls::None);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, 2);
  String outputHeading = "Output " + String(index);
  tft.setTextSize(uiFittedTextSize(outputHeading, 2, tft.width() - 8));
  tft.println(outputHeading);
  tft.setTextSize(uiTextSize(1));
  tft.println("");
  bool isChange = isChangeAddress(hd, psbt.txOutsMeta[index], psbt.tx.txOuts[index]);
  if (isChange == true) tft.print("Change ");
  tft.println("Address:");
  tft.println("");

  // A native SegWit address is too wide at size 2 and can otherwise push the
  // amount underneath the physical review controls on the 240x135 display.
  String address = psbt.tx.txOuts[index].address(network);
  tft.setTextSize(uiFittedTextSize(address, 1, tft.width() - 8));
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(address);
  tft.setTextSize(uiTextSize(1));

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("");
  tft.println("Amount:");
  tft.println("");
  tft.setTextSize(uiTextSize(2));
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  String sats = int64ToString(psbt.tx.txOuts[index].amount);
  printSats(sats, uiTextSize(2));
  tft.println(" sat");
}

void printFeeDetails(uint64_t fee) {
  beginUiScreen(global.hasCommandsFile ? UiControls::ConfirmCancel : UiControls::None);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, max(8, uiContentHeight() / 3));
  String feeLabel = "Fee: " + int64ToString(fee) + " sat";
  tft.setTextSize(uiFittedTextSize(feeLabel, 2, tft.width() - 8));
  tft.print("Fee: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  String sats = int64ToString(fee);
  printSats(sats, uiFittedTextSize(feeLabel, 2, tft.width() - 8));
  tft.println(" sat");
}

void printSdReviewControls() {
  if (BOARD.hasTouchscreen) {
    setUiControls(UiControls::ConfirmCancel);
    return;
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(uiTextSize(1));
  tft.setCursor(0, tft.height() - 11);
  if (BOARD.singleButtonLongPressCancels) {
    tft.print("Tap accept / hold cancel");
  } else {
    tft.print("# / BTN1 accept   * / BTN2 cancel");
  }
}

bool isChangeAddress(HDPrivateKey hd, PSBTOutputMetadata txOutMeta, TxOut txOut) {
  logInfo("### isChangeAddress IN " );
  if (txOutMeta.derivationsLen > 0) { // there is derivation path
    logInfo("### isChangeAddress address1 " + txOut.address());
    logInfo("### isChangeAddress derivationsLen " + String(txOutMeta.derivationsLen));
    // considering only single key for simplicity
    PSBTDerivation der = txOutMeta.derivations[0];
    // logInfo("### isChangeAddress der " + String(der.derivation));
    HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
    logInfo("### isChangeAddress pub " + pub.xpub());
    logInfo("### isChangeAddress address 2" +  pub.address() + " " + txOut.address() );
    return pub.address() == txOut.address();
  }
  logInfo("### isChangeAddress OUT " );
  return false;
}

void printSats(String sats, int textSize) {
  int len = sats.length();
  int offest = len % 3;
  for (int i = 0; i < len; i++) {
    tft.print(sats[i]);
    if ((i + 1 - offest) % 3 == 0) {
      tft.setTextSize(uiTextSize(1));
      tft.print(" ");
      tft.setTextSize(textSize);
    }
  }
}
