#define FORMAT_ON_FAIL true

void setup() {
  Serial.begin(9600);

  // load screen
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  logo();
  delay(3000);
  h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);

  logSerial("HWW: waiting for commands");
  // in case of forced reboot, tell the client to logout
  logSerial(COMMAND_PASSWORD_CLEAR + " 1");

  if (loadFiles() == false)
    showMessage("Failed to open files",  "Reset or 'help'");

}

bool loadFiles() {
  FileData mnFile = readFile(SPIFFS, "/mn.txt");
  global.encrytptedMnemonic = mnFile.data;

  FileData pwdFile = readFile(SPIFFS, "/hash.txt");
  global.passwordHash = pwdFile.data;

  return mnFile.success && pwdFile.success;
}
