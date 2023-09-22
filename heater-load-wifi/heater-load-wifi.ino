#include <ArduinoJson.h>
#include <Preferences.h>

Preferences prefs;

void initWiFi();
void initMDNS();

void initHttp();
void handleHttpClients();
void readTemperatureFromSensors();

void initPWM();
void initTemperature();

void setup() {
  Serial.begin(115200);
  prefs.begin("heater_load");

  initWiFi();
  initMDNS();
  initHttp();

  initTemperature();
  initPWM();
}

void loop(){
  readTemperatureFromSensors();
  handleHttpClients();
  delay(100);
}
