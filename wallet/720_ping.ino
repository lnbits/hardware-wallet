CommandResponse executePing(String data) {
  Serial.println(COMMAND_PING + " 0 " + global.deviceId);
  return {"Contacted by", data};
}