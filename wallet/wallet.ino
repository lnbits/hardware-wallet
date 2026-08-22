/**
   Very cheap little bitcoin HWW for use with lilygo TDisplay
   although with little tinkering any ESP32 will work

   Join us!
   https://t.me/lnbits
   https://t.me/makerbits

*/

#include <FS.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <SD.h>

#include "board_profile.h"
#include "rng_health.h"
#include "ui_scale.h"
#include "ui_controls.h"

#include <Wire.h>
#include "display.h"
#include <ArduinoJson.h>
#include <libwally.h>
#include "qrcoded.h"
#include "bowser_logo.h"
#include "thinking_animation.h"
extern "C" {
#include <secp256k1.h>
#include <secp256k1_ecdh.h>
}

#include <aes.h>

#include "bootloader_random.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define BOWSER_FIRMWARE_VERSION "0.8.2"

fs::SPIFFSFS &FlashFS = SPIFFS;

BowserDisplay tft;


//////////////////////////////// Define and initialize the Global State ////////////////////////////////

struct GlobalState {
  String deviceId;
  bool authenticated;
  bool walletConfigured;
  bool persistSecrets;
  String passwordVerifier;
  String mnemonic;
  String passphrase;
  String passwordSalt;
  String encryptedMnemonic;
  String mnemonicMac;
  uint32_t passwordKdfIterations;
  bool legacyWalletStorage;
  unsigned long startTime;
  const String passwordFileName;
  const String mnemonicFileName;
  const String sharedSecretFileName;
  const String walletFileName;
  const String deviceMetaFileName;
  int button1Pin;
  int button2Pin;
  byte dhe_shared_secret[32];
  // sd card
  bool hasCommandsFile;
  String commands;
  const String commandsInFileName;
  const String commandsOutFileName;
  const String commandsLogFileName;
};

// Note: this is not an endorsment for One World Goverment
GlobalState global = {
  "",
  false,
  false,
  true,
  "",
  "",
  "",
  "",
  "",
  "",
  0,
  false,
  millis(),
  "/hash.txt",
  "/mn.txt",
  "/shared_secret.txt",
  "/wallet.v2",
  "/device_meta.txt",
  BOARD.primaryButtonPin,
  BOARD.secondaryButtonPin,
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  false,
  "",
  "/commands.in.txt",
  "/commands.out.txt",
  "/commands.log.txt",
};

////////////////////////////////           Global State End            ////////////////////////////////

//////////////////////////////// Define and initialize Environament Variables ////////////////////////////////
struct EnvironmentVarialbes {
  String version;
};

EnvironmentVarialbes env = {
  BOWSER_FIRMWARE_VERSION,
};
////////////////////////////////           Env Vars End            ////////////////////////////////

struct EventData {
  String type;
  String data;
};

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
  EventData event; // valid event that the command does not handle
};

struct HwwInitData {
  String passwordVerifier;
  String mnemonic;
  bool success;
};

// do not move/remove, arduino IDE bug
// at least one function definition is require after `struct` declaration
void logInfo(const String msg) {
  logInfoFile(msg);
  logInfoSerial(msg);
}

void logInfoFile(const String msg) {
  if (global.hasCommandsFile == true) {
    Serial.println("/log logInfoFile: " + global.commandsLogFileName + " msg: " + msg);
    appendFile(SD, global.commandsLogFileName.c_str(), msg + "\n");
  }
}

void logInfoSerial(const String msg) {
  Serial.println("/log " + msg);
}

// libwally's PSBT keypath validation and secp256k1 derivation have several
// sizeable nested stack frames. The Arduino C6 core otherwise gives loopTask
// only 8 KiB, leaving too little margin for the compiled signing call chain.
// Keep this after the shared sketch types: the Arduino prototype generator
// treats SET_LOOP_TASK_STACK_SIZE as a function definition.
#if defined(CONFIG_IDF_TARGET_ESP32C6)
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif
