#include <WiFiUdp.h>
#include <Servo.h>
#include <WebOTA.h>

const char* host     = "ESP-OTA"; // Used for MDNS resolution
const char* ssid     = "OpenRc";
const char* password = "10241024";

void setup() {
    Serial.begin(115200);
    init_wifi(ssid, password, host);
    // To use a specific port and path uncomment this line
    // Defaults are 8080 and "/webota"
    // webota.init(8888, "/update");
    Serial.write("------3-----");
}
void loop() {
    // Other loop code here

    webota.handle();
}