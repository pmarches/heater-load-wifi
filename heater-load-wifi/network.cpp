#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>

extern Preferences prefs;
void getTemperatureReadings(float& triac, float& external);
void getTemperatureLimits(float& triacTempLimit, float& externalTempLimit);
void setTemperatureLimits(int sensorIndex, float tempLimit);
void setKnownCompensationTemperatureTriac(float knownCurrentTempTriac);
bool isOverTemperature();

extern "C" unsigned char settings_html[];
extern "C" unsigned int settings_html_len;

WebServer server(80);

void handleIncomingJsonMessage(JsonDocument* jsonDoc);

void setTargetWatts(uint32_t targetWatts, uint32_t loadWatts);
uint32_t getTargetWatts();
void setPhaseSelectionCounts(const uint32_t onPhaseCount, const uint32_t offPhaseCount);
void getPhaseSelectionCounts(unsigned int& highCount, unsigned int& lowCount);

void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Connected to AP successfully!");
}

void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void createConfigurationAP(){
  //Create temporary AP to allow first time configuration
  const String TEMP_AP_NAME="heater load configuration AP";
  Serial.printf("Creating temporary access point named %s\n", TEMP_AP_NAME.c_str());
  WiFi.softAP(TEMP_AP_NAME);
}

void doWifiConnect() {
  String ssid=prefs.getString("wifissid");
  if(ssid.length()==0){
    createConfigurationAP();
  }
  else {
    String password=prefs.getString("wifipassword");
    WiFi.begin(ssid, password);
    Serial.printf("Connecting to WiFi %s\n", ssid);
  }
}

void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");

  doWifiConnect();
}

void initMDNS() {
  if (!MDNS.begin(WiFi.getHostname())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("heater_load", "tcp", 80);
}

void initWiFi() {
  WiFi.disconnect(true);
  delay(1000);
  char hostname[32] = { '\0' };
  uint8_t eth_mac[6];
  WiFi.macAddress(eth_mac);
  snprintf(hostname, 32, "%s%02X%02X%02X", CONFIG_IDF_TARGET "-", eth_mac[3], eth_mac[4], eth_mac[5]);
  WiFi.setHostname(hostname);

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  doWifiConnect();
}

void handleNotFound() {
  String message = F("File Not Found\n\n");

  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, F("text/plain"), message);
}

//TODO Validation of int values. Negatives, MAX, decimals, NaN, ...
void handleJsonCommand(JsonDocument* jsonReq) {
  JsonObject topObject = jsonReq->as<JsonObject>();

  JsonVariant loadWatts = topObject["loadWatts"];
  if(!loadWatts.isNull()){
    prefs.putInt("loadWatts", loadWatts.as<int>());
  }
  JsonVariant targetWattsJson = topObject["targetWatts"];
  if(!targetWattsJson.isNull()){
    setTargetWatts(targetWattsJson, prefs.getInt("loadWatts"));
  }

  JsonVariant onCount = topObject["onPhaseCount"];
  JsonVariant offCount = topObject["offPhaseCount"];
  if (!onCount.isNull() && !offCount.isNull()) {
    //TODO Sanity check here. Negatives, zeroes, ...
    setPhaseSelectionCounts(onCount, offCount);
  }

  JsonVariant wifiSSID = topObject["wifissid"];
  if(!wifiSSID.isNull() && wifiSSID.as<String>().length()>0){
    Serial.println("Updating SSID");
    prefs.putString("wifissid", wifiSSID.as<const char*>());
  }
  JsonVariant wifiPassword = topObject["wifipassword"];
  if(!wifiPassword.isNull() && wifiPassword.as<String>().length()>0){
    Serial.println("Updating password");
    prefs.putString("wifipassword", wifiPassword.as<const char*>());
  }

  JsonVariant triacTemperatureSensor = topObject["triacTemperatureSensor"];
  if(!triacTemperatureSensor.isNull()){
    setKnownCompensationTemperatureTriac(triacTemperatureSensor);
  }

  JsonVariant triacTemperatureLimit = topObject["triacTemperatureLimit"];
  if(!triacTemperatureLimit.isNull()){
    setTemperatureLimits(0, triacTemperatureLimit);
  }

  JsonVariant externalTemperatureLimit = topObject["externalTemperatureLimit"];
  if(!externalTemperatureLimit.isNull()){
    setTemperatureLimits(1, externalTemperatureLimit);
  }
}

void handleStateChange() {
  if(server.args()){
    StaticJsonDocument<200> jsonReq;
    Serial.println("JSON payload is '" + server.arg(0) + "'");
    DeserializationError error = deserializeJson(jsonReq, server.arg(0));
    if (error) {
      server.send(500, F("text/plain"), error.f_str());
      return;
    }

    handleJsonCommand(&jsonReq);
  }

  StaticJsonDocument<200> jsonResponse;
  JsonObject root = jsonResponse.to<JsonObject>();
  unsigned int onPhaseCount;
  unsigned int offPhaseCount;
  getPhaseSelectionCounts(onPhaseCount, offPhaseCount);
  root["onPhaseCount"] = onPhaseCount;
  root["offPhaseCount"] = offPhaseCount;
  root["wifissid"] = prefs.getString("wifissid");
  root["loadWatts"] = prefs.getInt("loadWatts");
  root["targetWatts"] = getTargetWatts();

  float triacTemp=NAN;
  float externalTemp=NAN;
  getTemperatureReadings(triacTemp, externalTemp);
  float triacTempLimit=NAN;
  float externalTempLimit=NAN;
  getTemperatureLimits(triacTempLimit, externalTempLimit);

  root["triacTemperatureSensor"]=triacTemp;
  root["triacTemperatureLimit"]=triacTempLimit;

  root["externalTemperatureSensor"]=externalTemp;
  root["externalTemperatureLimit"]=externalTempLimit;

  root["isOverTemperatureLimit"]=isOverTemperature();

  String jsonBytes;
  serializeJsonPretty(jsonResponse, jsonBytes);
  jsonBytes+="\n";
  server.send(200, F("text/json"), jsonBytes);
}

void handleHttpClients() {
  server.handleClient();
}

void handleOTAUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    /* flashing firmware to ESP*/
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {  //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void initHttp() {
  server.onNotFound(handleNotFound);
  server.on(F("/"), []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", String((const char*)settings_html, (size_t) settings_html_len));
  });
  server.on(F("/state"), handleStateChange);
  server.on(
    "/updateFirmware", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Refresh", "5; /");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    },
    handleOTAUpdate);

  server.begin();
}
