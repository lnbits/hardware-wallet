CommandResponse executeUnknown(String details) {
  logInfo("Unknown command: " + details);
  return {"Unknown command",  details};
}
