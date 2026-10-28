#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  ESP8266WebServer server(80);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  WebServer server(80);
#else
  #error "This project supports ESP8266 and ESP32."
#endif

// Benign local test firmware: creates its own test network and exposes
// a simple status page. It does not collect credentials or impersonate
// another network.
static const char* TEST_SSID = "ESP-Studio-Test";
static const char* TEST_PASSWORD = "ESPtest8266";

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
  json += "\"}";
  server.send(200, "application/json", json);
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

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.begin();

  Serial.println();
  Serial.println("ESP Studio test firmware ready.");
  Serial.print("SSID: ");
  Serial.println(TEST_SSID);
  Serial.print("Status URL: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/api/status");
}

void loop() {
  server.handleClient();
}
