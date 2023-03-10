/**
   @brief Sign a random message.

   @param messageHex: String. The message to be signed in hex format.
   @return CommandResponse
*/
CommandResponse executeSignMessage(String messageHex) {
  showMessage("Please wait", "Signing message..." + messageHex);

  logInfo("Preparing to sign messsge: " + messageHex);

  delay(10000);

  return {"Signed", "some message"};
}
