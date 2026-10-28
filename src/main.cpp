#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <Updater.h>
  ESP8266WebServer server(80);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <Update.h>
  WebServer server(80);
#else
  #error "This project supports ESP8266 and ESP32."
#endif

// Safe local test firmware for ESP Studio.
// Change OTA_KEY before using this on a real device/network.
static const char* TEST_SSID = "ESP-Studio-Test";
static const char* TEST_PASSWORD = "ESPtest8266";
static const char* OTA_KEY = "ESPStudioOTA-ChangeThisKey";

static bool uploadAuthorized = false;
static bool updateFailed = false;

void handleRoot() {
  server.send(200, "text/plain", "ESP Studio test firmware is running.");
}

void handleStatus() {
  String json = "{\"online\":true,\"device\":\"";
#if defined(ESP8266)
  json += "ESP8266 NodeMCU";
#else
  json += "ESP32";
#endif
  json += "\",\"ip\":\"";
  json += WiFi.softAPIP().toString();
  json += "\",\"ota\":true}";
  server.send(200, "application/json", json);
}

void handleUpdateComplete() {
  if (!uploadAuthorized) {
    server.send(401, "application/json", "{\"ok\":false,\"error\":\"Unauthorized OTA request\"}");
    return;
  }
  if (updateFailed || Update.hasError()) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"Firmware update failed\"}");
    return;
  }

  server.send(200, "application/json", "{\"ok\":true,\"message\":\"Firmware installed; restarting\"}");
  delay(700);
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    uploadAuthorized = (server.header("X-ESP-OTA-Key") == OTA_KEY);
    updateFailed = false;

    if (!uploadAuthorized) {
      Serial.println("Rejected unauthorized OTA upload.");
      return;
    }

    Serial.printf("OTA upload started: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      updateFailed = true;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadAuthorized && !updateFailed) {
      size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        Update.printError(Serial);
        updateFailed = true;
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadAuthorized && !updateFailed) {
      if (!Update.end(true)) {
        Update.printError(Serial);
        updateFailed = true;
      } else {
        Serial.printf("OTA upload complete: %u bytes\n", upload.totalSize);
      }
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    updateFailed = true;
    Serial.println("OTA upload aborted.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_AP);
  IPAddress localIP(172, 0, 0, 1);
  IPAddress gateway(172, 0, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(TEST_SSID, TEST_PASSWORD);

  const char* headerKeys[] = {"X-ESP-OTA-Key"};
  server.collectHeaders(headerKeys, 1);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/update", HTTP_POST, handleUpdateComplete, handleUpdateUpload);
  server.begin();

  Serial.println();
  Serial.println("ESP Studio OTA test firmware ready.");
  Serial.print("SSID: ");
  Serial.println(TEST_SSID);
  Serial.print("Status URL: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/api/status");
  Serial.println("OTA endpoint: POST /update (multipart file field; X-ESP-OTA-Key header)");
}

void loop() {
  server.handleClient();
}
