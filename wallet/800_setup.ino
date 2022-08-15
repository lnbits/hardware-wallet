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


}
