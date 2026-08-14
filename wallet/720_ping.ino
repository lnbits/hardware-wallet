/**
   @brief Unenrypted `ping` command.
   It will respond with a ping command over `Serial`.
   @param data: String (optional). Some identification token from the client.
   @return CommandResponse
*/
CommandResponse executePing(String data) {
  if (data == "") return {"", ""};

  // A fresh device has no wallet record to unlock. This is deliberately a
  // one-way notice: configured devices remain silent rather than sending a
  // negative `/new` status.
  if (!global.walletConfigured) {
    Serial.println(COMMAND_NEW);
  }
  Serial.println(COMMAND_PING + " 0 " + global.deviceId);
  return {"Contacted by", data};
}
