#include "ESPOTADASH.h"
#include <DHT.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

#define DHTPIN 10
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiManager wifiManager;
ESPOTADASH* ota = nullptr;

char hostName[40] = "ESP32_DHT11_01";
char serverAddress[100] = "http://iot.minhhoangcodientu.io.vn:1313";
char firmwareVersion[10] = "1.0.0";

unsigned long lastReadSensor = 0;
const long sensorReadInterval = 5000;
unsigned long lastSensorSend = 0;
const long sensorSendInterval = 30000; // 30 giay (realtime)

float temperature = 0;
float humidity = 0;

void sendSensorData() {
  if (temperature == 0 && humidity == 0) return;
  
  WiFiClient client;
  HTTPClient http;
  
  String url = String(serverAddress) + "/sensorData";
  http.begin(client, url);
  http.addHeader("Content-Type", "text/plain");
  
  String postData = String(hostName) + "\n" + String(temperature, 1) + "\n" + String(humidity, 1);
  
  int httpCode = http.POST(postData);
  if (httpCode > 0) {
    Serial.printf("Sensor data sent. HTTP: %d\n", httpCode);
  } else {
    Serial.printf("Sensor data failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void handleCommand(String cmd) {
  cmd.trim();
  Serial.print("Lenh nhan: ");
  Serial.println(cmd);
  
  if (cmd == "info") {
    String ip = WiFi.localIP().toString();
    Serial.println("=== THONG TIN ===");
    Serial.print("IP: "); Serial.println(ip);
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
    Serial.print("Firmware: "); Serial.println(firmwareVersion);
    Serial.print("Temperature: "); Serial.print(temperature, 1); Serial.println("C");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.println("%");
  }
  else if (cmd == "resetwifi") {
    Serial.println("Resetting WiFi and rebooting...");
    wifiManager.resetSettings();
    delay(1000);
    ESP.restart();
  }
  else if (cmd == "temp") {
    Serial.print("Nhiet do: "); Serial.print(temperature, 1); Serial.println("C");
    Serial.print("Do am: "); Serial.print(humidity, 1); Serial.println("%");
  }
}

void readDHT() {
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  
  if (isnan(newHumidity) || isnan(newTemperature)) {
    Serial.println("Loi doc DHT11!");
    return;
  }
  
  humidity = newHumidity;
  temperature = newTemperature;
  
  Serial.print("DHT11 - Temp: ");
  Serial.print(temperature, 1);
  Serial.print("C, Humidity: ");
  Serial.print(humidity, 1);
  Serial.println("%");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== KHOI DONG ESP32 OTA ===");
  Serial.print("Firmware: ");
  Serial.println(firmwareVersion);
  
  dht.begin();
  
  Serial.println("Dang ket noi WiFi...");
  wifiManager.setConnectTimeout(30);
  wifiManager.setConfigPortalTimeout(180);
  
  if (!wifiManager.autoConnect("ESP32_OTA")) {
    Serial.println("Loi ket noi WiFi!");
    delay(3000);
    ESP.restart();
  }
  
  String ip = WiFi.localIP().toString();
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(ip);
  
  readDHT();
  
  ota = new ESPOTADASH("", "", hostName, serverAddress,
                     10000, 60000, 5000, 3600000, firmwareVersion);
  ota->setCommandCallback(handleCommand);
  ota->begin();
  
  Serial.println("=== HE THONG SAN SANG ===");
}

void loop() {
  if (ota != nullptr) {
    ota->loop();
  }
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastReadSensor >= sensorReadInterval) {
    lastReadSensor = currentMillis;
    readDHT();
  }
  
  if (currentMillis - lastSensorSend >= sensorSendInterval) {
    lastSensorSend = currentMillis;
    sendSensorData();
  }
  
  delay(10);
}
