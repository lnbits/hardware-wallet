CommandResponse executePing(String data) {
  Serial.println(COMMAND_PING + " 0 " + global.deviceUUID);
  return {"Contacted by", data};
}