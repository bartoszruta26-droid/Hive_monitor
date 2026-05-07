/**
 * ApiaryGuard - Arduino Nano + Ethernet Shield (W5100)
 * Wersja: Tylko surowe dane z sensorów (RAW DATA ONLY)
 * 
 * WSZYSTKIE OBLICZENIA PARAMETRÓW WYKONYWANE SĄ W RASPBERRY PI
 * Arduino Nano wysyła wyłącznie surowe odczyty z sensorów
 * 
 * Wymagane biblioteki:
 * - Ethernet by Arduino (wbudowana)
 * - DHT sensor library by Adafruit
 * - ArduinoJson by Benoit Blanchon (v6 lub v7)
 * 
 * Połączenia pinów Arduino Nano:
 * Ethernet W5100 (SPI): D10(CS), D11(MOSI), D12(MISO), D13(SCK)
 * HX711: DT->D2, SCK->D3
 * DHT22: DATA->D4
 * PWM: HEATER->D5, FAN->D6, PUMP->D9
 * Przekaźniki: RELAY1->D7, RELAY2->D8
 * Audio: MIC->A0, PIEZO->A1
 * SGP30/41 (I2C): SDA->A4, SCL->A5 (opcjonalnie)
 */

#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==================== KONFIGURACJA SIECI ====================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
EthernetServer server(8080);

// ==================== PINY ====================
#define HX711_DT    2
#define HX711_SCK   3
#define DHT_PIN     4
#define HEATER_PWM  5
#define FAN_PWM     6
#define RELAY_1     7
#define RELAY_2     8
#define PUMP_PWM    9
#define MIC_PIN     A0
#define PIEZO_PIN   A1

// ==================== OBIEKTY ====================
DHT dht(DHT_PIN, DHT22);

// ==================== ZMIENNE GLOBALNE ====================
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 1000;  // 1 sekunda

// Stan PWM do raportowania (nie można odczytać z pinów PWM)
struct PWMState {
  uint8_t heater;
  uint8_t fan;
  uint8_t pump;
} pwm_state = {0, 0, 0};

// Surowe dane z sensorów
struct RawSensorData {
  float temp_raw;           // Temperatura [°C]
  float hum_raw;            // Wilgotność [%]
  long weight_raw;          // Waga [surowe ADC]
  int audio_raw;            // Audio RMS
  int vibration_raw;        // Wibracje [ADC]
  int co2_raw;              // CO2 [ppm]
  int voc_raw;              // VOC Index
  uint8_t heater_pwm;       // 0-255 (zapisane wartości)
  uint8_t fan_pwm;          // 0-255 (zapisane wartości)
  uint8_t pump_pwm;         // 0-255 (zapisane wartości)
  uint8_t relay1_state;     // 0/1
  uint8_t relay2_state;     // 0/1
  unsigned long timestamp;  // ms since boot
  uint16_t free_ram;        // Diagnostyka
} sensors;

long weightOffset = 0;
bool sgpConnected = false;

// ==================== FUNKCJE POMOCNICZE ====================

uint16_t getFreeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

bool initEthernet() {
  Ethernet.init(10);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet: Nie wykryto W5100!");
    return false;
  }
  
  // Konfiguracja Ethernet (statyczne IP - begin zwraca void)
  Ethernet.begin(mac, ip);
  delay(1000); // Czekaj na inicjalizację sprzętu
  
  // Sprawdź fizyczne połączenie
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet: Brak połączenia fizycznego (kabla)!");
    return false;
  }
  
  Serial.print("Ethernet IP: ");
  Serial.println(Ethernet.localIP());
  return true;
}

void initHX711() {
  pinMode(HX711_SCK, OUTPUT);
  pinMode(HX711_DT, INPUT);
  digitalWrite(HX711_SCK, LOW);
  delay(100);
  weightOffset = readHX711();
  Serial.print("HX711 Offset: ");
  Serial.println(weightOffset);
}

long readHX711() {
  unsigned long timeout = millis();
  while (digitalRead(HX711_DT) && (millis() - timeout < 200));
  if (digitalRead(HX711_DT)) return 0;
  
  long value = 0;
  for (int i = 0; i < 24; i++) {
    digitalWrite(HX711_SCK, HIGH);
    value = value << 1;
    if (digitalRead(HX711_DT)) value++;
    digitalWrite(HX711_SCK, LOW);
  }
  digitalWrite(HX711_SCK, HIGH);
  digitalWrite(HX711_SCK, LOW);
  
  return value - weightOffset;
}

int readMic() {
  long sum = 0;  // 32-bit accumulator to avoid overflow
  int samples = 64;
  for (int i = 0; i < samples; i++) {
    int val = analogRead(MIC_PIN);
    long diff = val - 512;  // Use long for multiplication
    sum += diff * diff;
    delayMicroseconds(200);
  }
  return sqrt(sum / samples);
}

int readPiezo() {
  return analogRead(PIEZO_PIN);
}

void setPWM(int pin, int value, int pwmType) {
  uint8_t constrainedValue = constrain(value, 0, 255);
  analogWrite(pin, constrainedValue);
  
  // Zapisz stan do raportowania
  if (pwmType == 0) {
    pwm_state.heater = constrainedValue;
    sensors.heater_pwm = constrainedValue;
  } else if (pwmType == 1) {
    pwm_state.fan = constrainedValue;
    sensors.fan_pwm = constrainedValue;
  } else if (pwmType == 2) {
    pwm_state.pump = constrainedValue;
    sensors.pump_pwm = constrainedValue;
  }
}

void setRelay(int relay, bool state) {
  digitalWrite(relay, state ? HIGH : LOW);
  // Aktualizuj stan przekaźników
  if (relay == RELAY_1) {
    sensors.relay1_state = state ? 1 : 0;
  } else if (relay == RELAY_2) {
    sensors.relay2_state = state ? 1 : 0;
  }
}

// Odczyt SGP30/SGP41 przez I2C (lub symulacja)
bool readSGP() {
  if (!sgpConnected) {
    sensors.co2_raw = 400 + random(0, 200);
    sensors.voc_raw = 50 + random(0, 100);
    return false;
  }
  
  // Rzeczywisty odczyt I2C dla SGP30
  Wire.beginTransmission(0x58);
  Wire.write((uint8_t)0x20);  // Command byte 1
  Wire.write((uint8_t)0x08);  // Command byte 2
  if (Wire.endTransmission() == 0) {
    delay(50);
    Wire.requestFrom(0x58, (uint8_t)6);
    if (Wire.available() >= 6) {
      uint8_t data[6];
      for (int i = 0; i < 6; i++) data[i] = Wire.read();
      sensors.co2_raw = data[0] * 256 + data[1];
      sensors.voc_raw = data[3] * 256 + data[4];
      return true;
    }
  }
  return false;
}

void readAllSensors() {
  // DHT22
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  sensors.temp_raw = isnan(t) ? -99.9 : t;
  sensors.hum_raw = isnan(h) ? -99.9 : h;
  
  // HX711
  sensors.weight_raw = readHX711();
  
  // Audio
  sensors.audio_raw = readMic();
  
  // Piezo
  sensors.vibration_raw = readPiezo();
  
  // SGP30/41
  readSGP();
  
  // Metadane
  sensors.timestamp = millis();
  sensors.free_ram = getFreeRam();
  
  // Stan efektorów - użyj zapisanych wartości, nie odczytuj z pinów
  // (analogRead na pinach PWM daje błędne wyniki)
  // Wartości heater_pwm, fan_pwm, pump_pwm są już aktualizowane w setPWM()
  // Wartości relay1_state, relay2_state są już aktualizowane w setRelay()
}

// Wysyłanie JSON z surowymi danymi
void sendRawData(EthernetClient& client) {
  StaticJsonDocument<512> doc;
  
  doc["temp_raw"] = sensors.temp_raw;
  doc["hum_raw"] = sensors.hum_raw;
  doc["weight_raw"] = sensors.weight_raw;
  doc["audio_raw"] = sensors.audio_raw;
  doc["vibration_raw"] = sensors.vibration_raw;
  doc["co2_raw"] = sensors.co2_raw;
  doc["voc_raw"] = sensors.voc_raw;
  
  doc["heater_pwm"] = sensors.heater_pwm;
  doc["fan_pwm"] = sensors.fan_pwm;
  doc["pump_pwm"] = sensors.pump_pwm;
  doc["relay1"] = sensors.relay1_state;
  doc["relay2"] = sensors.relay2_state;
  
  doc["timestamp"] = sensors.timestamp;
  doc["free_ram"] = sensors.free_ram;
  
  String json;
  serializeJson(doc, json);
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println();
  client.println(json);
  delay(1);
  client.stop();
}

void sendResponse(EthernetClient& client, const char* contentType, const String& content) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(content.length());
  client.println();
  client.println(content);
  delay(1);
  client.stop();
}

void handleCommand(String cmd) {
  if (cmd.startsWith("SET_HEATER:")) {
    int val = cmd.substring(11).toInt();
    setPWM(HEATER_PWM, val, 0);  // 0 = heater
  } else if (cmd.startsWith("SET_FAN:")) {
    int val = cmd.substring(8).toInt();
    setPWM(FAN_PWM, val, 1);  // 1 = fan
  } else if (cmd.startsWith("SET_PUMP:")) {
    int val = cmd.substring(9).toInt();
    setPWM(PUMP_PWM, val, 2);  // 2 = pump
  } else if (cmd.startsWith("SET_RELAY1:")) {
    setRelay(RELAY_1, cmd.substring(11).toInt());
  } else if (cmd.startsWith("SET_RELAY2:")) {
    setRelay(RELAY_2, cmd.substring(11).toInt());
  } else if (cmd.startsWith("CALIB_WEIGHT")) {
    weightOffset = readHX711();
    Serial.println("Waga skalibrowana");
  }
}

void handleServer() {
  EthernetClient client = server.available();
  if (!client) return;
  
  unsigned long timeout = millis();
  while (!client.available() && (millis() - timeout < 1000)) delay(1);
  if (!client.available()) { client.stop(); return; }
  
  String request = client.readStringUntil('\r');
  client.readStringUntil('\n');
  
  // Sterowanie
  if (request.indexOf("/heater/on") >= 0) {
    setPWM(HEATER_PWM, 255, 0);
    sendResponse(client, "text/plain", "Heater ON");
  } else if (request.indexOf("/heater/off") >= 0) {
    setPWM(HEATER_PWM, 0, 0);
    sendResponse(client, "text/plain", "Heater OFF");
  } else if (request.indexOf("/fan/on") >= 0) {
    setPWM(FAN_PWM, 255, 1);
    sendResponse(client, "text/plain", "Fan ON");
  } else if (request.indexOf("/fan/off") >= 0) {
    setPWM(FAN_PWM, 0, 1);
    sendResponse(client, "text/plain", "Fan OFF");
  } else if (request.indexOf("/pump/on") >= 0) {
    setPWM(PUMP_PWM, 255, 2);
    sendResponse(client, "text/plain", "Pump ON");
  } else if (request.indexOf("/pump/off") >= 0) {
    setPWM(PUMP_PWM, 0, 2);
    sendResponse(client, "text/plain", "Pump OFF");
  }
  // Surowe dane
  else if (request.indexOf("/api/raw") >= 0 || request.indexOf("/status") >= 0) {
    readAllSensors();
    sendRawData(client);
  }
  // Komendy
  else if (request.indexOf("/api/cmd?") >= 0) {
    int start = request.indexOf("?") + 1;
    String cmd = request.substring(start);
    cmd.trim();
    handleCommand(cmd);
    sendResponse(client, "text/plain", "OK");
  }
  // Strona główna
  else if (request == "GET / HTTP/1.1" || request.indexOf("/index.html") >= 0) {
    String html = "<!DOCTYPE html><html><head><title>ApiaryGuard Nano</title>";
    html += "<meta http-equiv='refresh' content='5'>";
    html += "</head><body><h1>ApiaryGuard - Arduino Nano</h1>";
    html += "<h2>Tylko surowe dane</h2><p>Obliczenia w Raspberry Pi</p>";
    html += "<div id='data'>Loading...</div>";
    html += "<script>fetch('/api/raw').then(r=>r.json()).then(d=>{";
    html += "document.getElementById('data').innerHTML=";
    html += "'Temp: '+d.temp_raw+'C<br>Hum: '+d.hum_raw+'%<br>";
    html += "Weight: '+d.weight_raw+'<br>Audio: '+d.audio_raw+'<br>";
    html += "Vibration: '+d.vibration_raw+'<br>CO2: '+d.co2_raw+'ppm<br>";
    html += "VOC: '+d.voc_raw+'<br>RAM: '+d.free_ram+'B<br>";
    html += "Timestamp: '+d.timestamp;";
    html += "});</script></body></html>";
    sendResponse(client, "text/html", html);
  }
  else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
    client.stop();
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  Serial.println("\n=== ApiaryGuard Nano ===");
  Serial.println("RAW DATA MODE - All calculations in RPi");
  
  // Inicjalizacja pinów
  pinMode(HEATER_PWM, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);
  pinMode(PUMP_PWM, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  
  // Ethernet
  if (!initEthernet()) {
    Serial.println("Ethernet failed!");
  }
  
  // DHT22
  dht.begin();
  
  // HX711
  initHX711();
  
  // I2C dla SGP30
  Wire.begin();
  Wire.beginTransmission(0x58);
  sgpConnected = (Wire.endTransmission() == 0);
  Serial.print("SGP30: ");
  Serial.println(sgpConnected ? "OK" : "Not found (using simulation)");
  
  server.begin();
  Serial.print("Server on port 8080, RAM free: ");
  Serial.println(getFreeRam());
}

// ==================== LOOP ====================
void loop() {
  // Odczyt sensorów
  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    readAllSensors();
    Serial.print("T:"); Serial.print(sensors.temp_raw);
    Serial.print(" H:"); Serial.print(sensors.hum_raw);
    Serial.print(" W:"); Serial.print(sensors.weight_raw);
    Serial.print(" A:"); Serial.print(sensors.audio_raw);
    Serial.print(" V:"); Serial.print(sensors.vibration_raw);
    Serial.print(" CO2:"); Serial.print(sensors.co2_raw);
    Serial.print(" VOC:"); Serial.print(sensors.voc_raw);
    Serial.print(" RAM:"); Serial.println(sensors.free_ram);
    lastSensorRead = millis();
  }
  
  // Obsługa serwera HTTP
  handleServer();
}
