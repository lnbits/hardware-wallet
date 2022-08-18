/**
   Very cheap little bitcoin HWW for use with lilygo TDisplay
   although with little tinkering any ESP32 will work

   Join us!
   https://t.me/lnbits
   https://t.me/makerbits

*/

#include <FS.h>
#include <SPIFFS.h>

#include <Wire.h>
#include <TFT_eSPI.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include "Bitcoin.h"
#include "PSBT.h"

#include <aes.h>

#include <FS.h>
#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;


SHA256 h;
TFT_eSPI tft = TFT_eSPI();
const byte EMPTY_32[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

//////////////////////////////// Define and initialize the Global State ////////////////////////////////
struct GlobalState {
  String deviceUUID;
  bool authenticated;
  String passwordHash;
  String encrytptedMnemonic;
  unsigned long startTime;
  const String passwordFileName;
  const String mnemonicFileName;
  const String sharedSecretFileName;
  const String deviceMetaFileName;
  byte dhe_shared_secret[32];
};

// Note: this is not an endorsment for One World Goverment
GlobalState global = {
  "",
  false,
  "",
  "",
  millis(),
  "/hash.txt",
  "/mn.txt",
  "/shared_secret.txt",
  "/device_meta.txt",
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

////////////////////////////////           Global State End            ////////////////////////////////

struct FileData {
  bool success;
  String data;
};

struct Command {
  String cmd;
  String data;
};

struct CommandResponse {
  String message;
  String subMessage;
  int statusCode;
};

struct HwwInitData {
  String passwordHash;
  String mnemonic;
  bool success;
};

// do not move/remove, arduino IDE bug
// at least one function definition is require after `struct` declaration
void logSerial(String msg) {
  Serial.println("/log " + msg);
}
