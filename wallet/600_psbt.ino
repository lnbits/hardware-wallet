
PSBT parseBase64Psbt(String psbtBase64) {
  PSBT psbt;
  psbt.parseBase64(psbtBase64);
  return psbt;
}

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

