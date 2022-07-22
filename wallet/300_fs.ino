//========================================================================//
//=============================SPIFFS STUFF===============================//
//========================================================================//



FileData readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  struct FileData fd = {false, ""};
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return fd;
  }


  while (file.available())
    fd.data += (char)file.read();


  file.close();
  fd.success = true;
  return fd;
}

void writeFile(fs::FS &fs, const char * path, String message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}
