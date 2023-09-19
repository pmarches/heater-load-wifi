#include <ArduinoJson.h>
#include <Preferences.h>

Preferences prefs;

void initWiFi();
void initMDNS();

void initHttp();
void handleHttpClients();
void updateTemperatureReadings();

void initPWM();

void setup() {
  Serial.begin(115200);
  prefs.begin("heater_load");

  initWiFi();
  initMDNS();
  initHttp();

  initPWM();
}

void loop(){
  updateTemperatureReadings();
  handleHttpClients();
  delay(100);
}
