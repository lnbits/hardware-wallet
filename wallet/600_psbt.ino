
PSBT parseBase64Psbt(String psbtBase64) {
  PSBT psbt;
  psbt.parseBase64(psbtBase64);
  return psbt;
}

// todo: remove
void printPsbtDetails(PSBT psbt, HDPrivateKey hd) {

  // check parsing is ok
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 30);
  tft.println("Transactions details:");
  // going through all outputs
  tft.println("Outputs:");
  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    // print addresses
    tft.print(psbt.tx.txOuts[i].address(&Mainnet)); // todo: support testnet
    if (psbt.txOutsMeta[i].derivationsLen > 0) { // there is derivation path
      // considering only single key for simplicity
      PSBTDerivation der = psbt.txOutsMeta[i].derivations[0];
      HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
      if (pub.address() == psbt.tx.txOuts[i].address()) {
        tft.print(" (change) ");
      }
    }
    tft.print(" -> ");
    tft.print(psbt.tx.txOuts[i].btcAmount() * 1e3);
    tft.println(" mBTC");
  }
  tft.print("Fee: ");
  tft.print(float(psbt.fee()) / 100); // Arduino can't print 64-bit ints
  tft.println(" bits");
}

void printOutputDetails(PSBT psbt, HDPrivateKey hd, int index, const Network * network) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.println("Output " + String(index));
  tft.setTextSize(1);
  tft.println("");
  bool isChange = isChangeAddress(hd, psbt.txOutsMeta[index], psbt.tx.txOuts[index]);
  if (isChange == true) tft.print("Change ");
  tft.println("Address:");
  tft.println("");

  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println(psbt.tx.txOuts[index].address(network));
  tft.setTextSize(1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("");
  tft.println("Amount:");
  tft.println("");
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  String sats = int64ToString(psbt.tx.txOuts[index].amount);
  printSats(sats, 2);
  tft.println(" sat");
}

void printFeeDetails(uint64_t fee) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 30);
  tft.setTextSize(2);
  tft.print("Fee: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  String sats = int64ToString(fee);
  printSats(sats, 2);
  tft.println(" sat");
}

bool isChangeAddress(HDPrivateKey hd, PSBTOutputMetadata txOutMeta, TxOut txOut) {
  Serial.println("### isChangeAddress IN " );
  if (txOutMeta.derivationsLen > 0) { // there is derivation path
    Serial.println("### isChangeAddress address1 " + txOut.address());
    Serial.println("### isChangeAddress derivationsLen " + String(txOutMeta.derivationsLen));
    // considering only single key for simplicity
    PSBTDerivation der = txOutMeta.derivations[0];
    // Serial.println("### isChangeAddress der " + String(der.derivation));
    HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
    Serial.println("### isChangeAddress pub " + pub.xpub());
    Serial.println("### isChangeAddress address 2" +  pub.address() + " " + txOut.address() );
    return pub.address() == txOut.address();
  }
  Serial.println("### isChangeAddress OUT " );
  return false;
}

void printSats(String sats, int textSize) {
  int len = sats.length();
  int offest = len % 3;
  for (int i = 0; i < len; i++) {
    tft.print(sats[i]);
    if ((i + 1 - offest) % 3 == 0) {
      tft.setTextSize(1);
      tft.print(" ");
      tft.setTextSize(textSize);
    }
  }
}
