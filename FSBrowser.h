// Holds the current upload
File fsUploadFile;

// Format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if(bytes < (1024 * 1024)) {
    return String(bytes/1024.0) + "KB";
  } else if(bytes < (1024 * 1024 * 1024)) {
    return String(bytes/1024.0/1024.0) + "MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if(webServer.hasArg("download")) {
    return "application/octet-stream";
  }
  else if(filename.endsWith(".htm")) {
    return "text/html";
  }
  else if(filename.endsWith(".html")) {
    return "text/html";
  }
  else if(filename.endsWith(".css")) {
    return "text/css";
  }
  else if(filename.endsWith(".js")) {
    return "application/javascript";
  }
  else if(filename.endsWith(".png")) {
    return "image/png";
  }
  else if(filename.endsWith(".gif")) {
    return "image/gif";
  }
  else if(filename.endsWith(".jpg")) {
    return "image/jpeg";
  }
  else if(filename.endsWith(".ico")) {
    return "image/x-icon";
  }
  else if(filename.endsWith(".xml")) {
    return "text/xml";
  }
  else if(filename.endsWith(".pdf")) {
    return "application/x-pdf";
  }
  else if(filename.endsWith(".zip")) {
    return "application/x-zip";
  }
  else if(filename.endsWith(".gz")) {
    return "application/x-gzip";
  }

  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);

  if(path.endsWith("/")) {
    path += "index.htm";
  }

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";

  if(LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
    if(LittleFS.exists(pathWithGz)) {
      path += ".gz";
    }

    File file = LittleFS.open(path, "r");
    size_t sent = webServer.streamFile(file, contentType);
    file.close();

    return true;
  }
  return false;
}

void handleFileUpload() {
  if(webServer.uri() != "/edit") {
    return;
  }

  HTTPUpload& upload = webServer.upload();

  if(upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;

    if(!filename.startsWith("/")) {
      filename = "/"+filename;
    }

    Serial.print("handleFileUpload Name: "); 
    Serial.println(filename);
    fsUploadFile = LittleFS.open(filename, "w");
    filename = String();
  } 
  else if(upload.status == UPLOAD_FILE_WRITE) {
    if(fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } 
  else if(upload.status == UPLOAD_FILE_END) {
    if(fsUploadFile) {
      fsUploadFile.close();
    }

    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if(webServer.args() == 0) {
    return webServer.send(500, "text/plain", "BAD ARGS");
  }

  String path = webServer.arg(0);
  Serial.println("handleFileDelete: " + path);

  if(path == "/") {
    return webServer.send(500, "text/plain", "BAD PATH");
  }

  if(!LittleFS.exists(path)) {
    return webServer.send(404, "text/plain", "FileNotFound");
  }

  LittleFS.remove(path);
  webServer.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if(webServer.args() == 0) {
    return webServer.send(500, "text/plain", "BAD ARGS");
  }

  String path = webServer.arg(0);
  Serial.println("handleFileCreate: " + path);

  if(path == "/") {
    return webServer.send(500, "text/plain", "BAD PATH");
  }

  if(LittleFS.exists(path)) {
    return webServer.send(500, "text/plain", "FILE EXISTS");
  }

  File file = LittleFS.open(path, "w");

  if(file) {
    file.close();
  }
  else {
    return webServer.send(500, "text/plain", "CREATE FAILED");
  }

  webServer.send(200, "text/plain", "");
  path = String();
}

// Call 'callback' for every file below 'dirPath', including files in subdirectories.
// LittleFS has real directories (SPIFFS was flat), so walk them to keep the same file list.
void forEachFile(const String& dirPath, std::function<void(const String& path, size_t size)> callback) {
  String base = dirPath.endsWith("/") ? dirPath : dirPath + "/";
  Dir dir = LittleFS.openDir(base);

  while (dir.next()) {
    String path = base + dir.fileName();

    if (dir.isDirectory()) {
      forEachFile(path, callback);
    }
    else {
      callback(path, dir.fileSize());
    }
  }
}

void handleFileList() {
  if(!webServer.hasArg("dir")) {
    webServer.send(500, "text/plain", "BAD ARGS"); 
    return;
  }
  
  String path = webServer.arg("dir");
  Serial.println("handleFileList: " + path);

  if (!path.endsWith("/")) {
    path += "/";
  }

  String output = "[";

  // Names are relative to the requested directory, e.g. "css/style.css", like SPIFFS listed them
  forEachFile(path, [&](const String& filePath, size_t size) {
    if (output != "[") {
      output += ',';
    }

    output += "{\"type\":\"file\",\"name\":\"";
    output += filePath.substring(path.length());
    output += "\",\"size\":";
    output += String(size);
    output += "}";
  });
  
  output += "]";
  webServer.send(200, "application/json", output);
}
