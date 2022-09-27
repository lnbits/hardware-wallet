void loop() {
  if (global.hasCommandsFile == true) {
    writeFile(SD, global.commandsLogFileName.c_str(),  "#### commands file logs ####\n");
  }

  listenForCommands();
}
