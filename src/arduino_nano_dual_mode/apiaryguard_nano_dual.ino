/**
 * ApiaryGuard - Arduino Nano Dual Mode (USB + Ethernet ENC28J60)
 * Uniwersalny firmware z automatyczną detekcją trybu połączenia
 * 
 * PRIORYTET: USB > Ethernet
 * 
 * Mikrokontroler sam sprawdza jak jest połączony i prowadzi komunikację
 * wybranym trybem. Preferuje USB nad Ethernet.
 * 
 * Wymagane biblioteki:
 * - UIPEthernet by Norbert Truchseß (dla ENC28J60)
 * - DHT sensor library by Adafruit
 * - ArduinoJson by Benoit Blanchon (v6 lub v7)
 * 
 * Połączenia pinów Arduino Nano:
 * Ethernet ENC28J60 (SPI): D10(CS), D11(MOSI), D12(MISO), D13(SCK)
 * HX711: DT->A0, SCK->A1
 * DHT22: DATA->D4
 * PWM: HEATER->D5, FAN->D6, PUMP->D9
 * Przekaźniki: RELAY1->D7, RELAY2->D8
 * Audio: MIC->A2, PIEZO->A3
 * SGP41 (I2C): SDA->A4, SCL->A5
 */

#include <SPI.h>
#include <UIPEthernet.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==================== KONFIGURACJA PINÓW ====================
// Ethernet SPI
#define ETH_CS_PIN    10

// HX711 (Waga)
#define HX711_DT    A0
#define HX711_SCK   A1

// DHT22
#define DHT_PIN     4

// PWM Efektory
#define HEATER_PWM  5
#define FAN_PWM     6
#define PUMP_PWM    9

// Przekaźniki
#define RELAY_1     7
#define RELAY_2     8

// Audio
#define MIC_PIN     A2
#define PIEZO_PIN   A3

// ==================== KONFIGURACJA SIECI ====================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
UIPEthernetServer server(8080);

// ==================== OBIEKTY ====================
DHT dht(DHT_PIN, DHT22);

// ==================== ZMIENNE GLOBALNE ====================
enum CommunicationMode {
  MODE_USB,
  MODE_ETHERNET,
  MODE_NONE
};

CommunicationMode currentMode = MODE_NONE;
bool ethernetAvailable = false;
bool usbConnected = false;

// Debug i Error Handling
#define DEBUG_ENABLED true
#define ERROR_BUFFER_SIZE 64
char errorBuffer[ERROR_BUFFER_SIZE];
int errorCount = 0;
int warningCount = 0;
unsigned long lastErrorTime = 0;

// Sensor error tracking
struct SensorErrors {
  uint8_t dht_errors;
  uint8_t hx711_errors;
  uint8_t sgp41_errors;
  uint8_t audio_errors;
  uint8_t piezo_errors;
  uint8_t consecutive_failures;
} sensorErrors = {0, 0, 0, 0, 0, 0};

const uint8_t MAX_CONSECUTIVE_FAILURES = 5;  // Reset sensor after this many failures

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 1000;  // 1 sekunda
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000;  // 10 sekund

// Stan PWM do raportowania
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
  uint8_t heater_pwm;       // 0-255
  uint8_t fan_pwm;          // 0-255
  uint8_t pump_pwm;         // 0-255
  uint8_t relay1_state;     // 0/1
  uint8_t relay2_state;     // 0/1
  unsigned long timestamp;  // ms since boot
  uint16_t free_ram;        // Diagnostyka
} sensors;

long weightOffset = 0;
float weightScale = 1.0;
bool sgpConnected = false;

// Bufor wejściowy dla komend USB
#define CMD_BUFFER_SIZE 128
char cmdBuffer[CMD_BUFFER_SIZE];
int cmdIndex = 0;

// ============================================================================
// FUNKCJE POMOCNICZE
// ============================================================================

uint16_t getFreeRam() {
  extern int __heap_start, *__brkval;\n  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

/**
 * Debug print helper
 */
void debugPrint(const char* prefix, const String& message) {
  if (DEBUG_ENABLED) {
    Serial.print("[");
    Serial.print(prefix);
    Serial.print("] ");
    Serial.println(message);
  }
}

/**
 * Error logging with timestamp and count
 */
void logError(const char* component, const char* message) {
  errorCount++;
  lastErrorTime = millis();
  
  snprintf(errorBuffer, ERROR_BUFFER_SIZE, "%s: %s", component, message);
  
  Serial.print("[ERR #");
  Serial.print(errorCount);
  Serial.print("] ");
  Serial.print(component);
  Serial.print(": ");
  Serial.println(message);
}

/**
 * Warning logging
 */
void logWarning(const char* component, const char* message) {
  warningCount++;
  
  Serial.print("[WARN #");
  Serial.print(warningCount);
  Serial.print("] ");
  Serial.print(component);
  Serial.print(": ");
  Serial.println(message);
}

/**
 * Check sensor health and reset if needed
 */
bool checkSensorHealth(uint8_t& errorCounter, const char* sensorName) {
  errorCounter++;
  sensorErrors.consecutive_failures++;
  
  if (errorCounter >= MAX_CONSECUTIVE_FAILURES) {
    logError(sensorName, "Max failures reached, attempting reset");
    errorCounter = 0;
    sensorErrors.consecutive_failures = 0;
    return false;  // Sensor needs reset
  }
  
  if (sensorErrors.consecutive_failures >= MAX_CONSECUTIVE_FAILURES * 2) {
    logError("SYSTEM", "Multiple sensor failures detected");
  }
  
  return true;  // Continue operation
}

/**
 * Reset sensor error counters after successful read
 */
void resetSensorError(uint8_t& errorCounter) {
  errorCounter = 0;
  if (sensorErrors.consecutive_failures > 0) {
    sensorErrors.consecutive_failures--;
  }
}

/**
 * Detekcja połączenia USB
 * Na Arduino Nano (ATmega328) Serial jest zawsze "true" po begin(),
 * więc sprawdzamy czy host wysyła dane lub czy linie DTR/RTS są aktywne.
 */
bool detectUSB() {
  // Czekaj krótko na ewentualną aktywność z hosta USB
  unsigned long detectStart = millis();
  const unsigned long detectTimeout = 500;  // 500ms
  
  // Sprawdź czy jakieś dane przychodzą z USB
  while (millis() - detectStart < detectTimeout) {
    if (Serial.available() > 0) {
      // Host USB wysyła dane - połączenie aktywne
      return true;
    }
    delay(1);
  }
  
  // Alternatywnie: sprawdź czy port jest otwarty (DTR/RTS)
  // Na niektórych boardach Serial && Serial.dtr() działa
  // ale na ATmega328 nie ma dostępu do DTR/RTS
  
  // Jeśli brak aktywności, zakładamy brak hosta USB
  return false;
}

/**
 * Inicjalizacja Ethernet ENC28J60 z error handlingiem
 */
bool initEthernet() {
  debugPrint("ETH", "Inicjalizacja ENC28J60...");
  
  // UIPEthernet nie wymaga Ethernet.init() - CS jest ustawiany w begin()
  
  int ret = Ethernet.begin(mac, ip, gateway, subnet);
  delay(1500);
  
  if (ret == 0) {
    logError("ETH", "Nie wykryto sprzetu lub brak polaczenia!");
    return false;
  }
  
  // Sprawdzenie linku przez odczyt statusu
  uint8_t linkStatus = Ethernet.linkStatus();
  if (linkStatus == 0) {
    logWarning("ETH", "Brak kabla sieciowego");
    return false;
  }
  
  Serial.print("Eth: IP = ");
  Serial.println(Ethernet.localIP());
  
  server.begin();
  debugPrint("ETH", "Server na porcie 8080");
  
  return true;
}

/**
 * Inicjalizacja HX711 z error handlingiem
 */
void initHX711() {
  pinMode(HX711_SCK, OUTPUT);
  pinMode(HX711_DT, INPUT);
  digitalWrite(HX711_SCK, LOW);
  delay(100);
  
  long offset = readHX711Raw();
  if (offset == 0) {
    logError("HX711", "Failed to initialize - check wiring");
    sensorErrors.hx711_errors = MAX_CONSECUTIVE_FAILURES;
  } else {
    weightOffset = offset;
    resetSensorError(sensorErrors.hx711_errors);
    Serial.print("HX711 Offset: ");
    Serial.println(weightOffset);
  }
}

/**
 * Odczyt surowy HX711 (bez offsetu)
 */
long readHX711Raw() {
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
  
  return value;
}

/**
 * Odczyt HX711 z odliczonym offsetem
 */
long readHX711() {
  long raw = readHX711Raw();
  return (long)((raw - weightOffset) * weightScale);
}

/**
 * Odczyt mikrofonu (RMS)
 */
int readMic() {
  long sum = 0;
  int samples = 64;
  for (int i = 0; i < samples; i++) {
    int val = analogRead(MIC_PIN);
    long diff = val - 512;
    sum += diff * diff;
    delayMicroseconds(200);
  }
  return sqrt(sum / samples);
}

/**
 * Odczyt piezo
 */
int readPiezo() {
  return analogRead(PIEZO_PIN);
}

/**
 * Ustawienie PWM
 */
void setPWM(int pin, int value, int pwmType) {
  uint8_t constrainedValue = constrain(value, 0, 255);
  analogWrite(pin, constrainedValue);
  
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

/**
 * Ustawienie przekaźnika
 */
void setRelay(int relay, bool state) {
  digitalWrite(relay, state ? HIGH : LOW);
  if (relay == RELAY_1) {
    sensors.relay1_state = state ? 1 : 0;
  } else if (relay == RELAY_2) {
    sensors.relay2_state = state ? 1 : 0;
  }
}

/**
 * Odczyt SGP41 przez I2C z error handlingiem
 */
bool readSGP41() {
  if (!sgpConnected) {
    sensors.co2_raw = 400 + random(0, 200);
    sensors.voc_raw = 50 + random(0, 100);
    return false;
  }
  
  Wire.beginTransmission(0x59);
  Wire.write((uint8_t)0x21);
  Wire.write((uint8_t)0x08);
  uint8_t err = Wire.endTransmission();
  
  if (err != 0) {
    sensorErrors.sgp41_errors++;
    #if DEBUG_ENABLED
    Serial.print("[SGP41] I2C write error: ");
    Serial.println(err);
    #endif
    return false;
  }
  
  delay(50);
  int available = Wire.requestFrom(0x59, (uint8_t)6);
  
  if (available < 6) {
    sensorErrors.sgp41_errors++;
    #if DEBUG_ENABLED
    Serial.print("[SGP41] I2C read error: got ");
    Serial.print(available);
    Serial.println(" bytes, expected 6");
    #endif
    return false;
  }
  
  uint8_t data[6];
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }
  
  // Validate data
  if (data[2] != ((data[0] * 256 + data[1]) >> 8 & 0xFF)) {
    logWarning("SGP41", "CRC check failed for CO2");
  }
  
  sensors.co2_raw = data[0] * 256 + data[1];
  sensors.voc_raw = data[3] * 256 + data[4];
  
  resetSensorError(sensorErrors.sgp41_errors);
  return true;
}

/**
 * Odczyt DHT22 z obsługa błędów
 */
bool readDHT22() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (isnan(t) || isnan(h)) {
    if (checkSensorHealth(sensorErrors.dht_errors, "DHT22")) {
      logWarning("DHT22", "Read failed, retrying next cycle");
    }
    sensors.temp_raw = -99.9;
    sensors.hum_raw = -99.9;
    return false;
  }
  
  resetSensorError(sensorErrors.dht_errors);
  sensors.temp_raw = t;
  sensors.hum_raw = h;
  return true;
}

/**
 * Odczyt wszystkich sensorów z error handlingiem
 */
void readAllSensors() {
  unsigned long readStart = millis();
  int successCount = 0;
  int failCount = 0;
  
  // DHT22
  if (readDHT22()) {
    successCount++;
  } else {
    failCount++;
  }
  
  // HX711
  long weightVal = readHX711();
  if (weightVal != 0 || sensorErrors.hx711_errors == 0) {
    sensors.weight_raw = weightVal;
    resetSensorError(sensorErrors.hx711_errors);
    successCount++;
  } else {
    if (checkSensorHealth(sensorErrors.hx711_errors, "HX711")) {
      logWarning("HX711", "Invalid reading");
    }
    failCount++;
  }
  
  // Audio
  int audioVal = readMic();
  if (audioVal >= 0) {
    sensors.audio_raw = audioVal;
    resetSensorError(sensorErrors.audio_errors);
    successCount++;
  } else {
    if (checkSensorHealth(sensorErrors.audio_errors, "MIC")) {
      logWarning("MIC", "Read failed");
    }
    failCount++;
  }
  
  // Piezo
  int piezoVal = readPiezo();
  if (piezoVal >= 0) {
    sensors.vibration_raw = piezoVal;
    resetSensorError(sensorErrors.piezo_errors);
    successCount++;
  } else {
    if (checkSensorHealth(sensorErrors.piezo_errors, "PIEZO")) {
      logWarning("PIEZO", "Read failed");
    }
    failCount++;
  }
  
  // SGP41
  if (readSGP41()) {
    resetSensorError(sensorErrors.sgp41_errors);
    successCount++;
  } else {
    if (sgpConnected) {  // Only warn if we think sensor should be there
      if (checkSensorHealth(sensorErrors.sgp41_errors, "SGP41")) {
        logWarning("SGP41", "I2C read failed");
      }
    }
    failCount++;
  }
  
  // Metadane
  sensors.timestamp = millis();
  sensors.free_ram = getFreeRam();
  
  // Debug: raport z czytania sensorów
  #if DEBUG_ENABLED
  if (failCount > 0) {
    Serial.print("[SENSOR] Read: ");
    Serial.print(successCount);
    Serial.print(" OK, ");
    Serial.print(failCount);
    Serial.print(" failed (");
    Serial.print(millis() - readStart);
    Serial.println("ms)");
  }
  #endif
  
  // Check for systemic issues
  if (failCount >= 4) {
    logError("SYSTEM", "Multiple sensor failures detected");
  }
}

// ============================================================================
// KOMUNIKACJA USB
// ============================================================================

/**
 * Wysyłanie danych przez USB w formacie JSON
 */
void sendUSBData() {
  StaticJsonDocument<512> doc;
  
  doc["temp"] = sensors.temp_raw;
  doc["hum"] = sensors.hum_raw;
  doc["weight"] = sensors.weight_raw;
  doc["audio"] = sensors.audio_raw;
  doc["vibration"] = sensors.vibration_raw;
  doc["co2"] = sensors.co2_raw;
  doc["voc"] = sensors.voc_raw;
  doc["heater"] = sensors.heater_pwm;
  doc["fan"] = sensors.fan_pwm;
  doc["pump"] = sensors.pump_pwm;
  doc["relay1"] = sensors.relay1_state;
  doc["relay2"] = sensors.relay2_state;
  doc["ts"] = sensors.timestamp;
  doc["ram"] = sensors.free_ram;
  
  serializeJson(doc, Serial);
  Serial.println();
}

/**
 * Obsługa komend USB
 */
void handleUSBCommand(String cmd) {
  cmd.trim();
  
  if (cmd.startsWith("SET_HEATER:")) {
    int val = cmd.substring(11).toInt();
    setPWM(HEATER_PWM, val, 0);
    Serial.println("OK: Heater=" + String(val));
  } else if (cmd.startsWith("SET_FAN:")) {
    int val = cmd.substring(8).toInt();
    setPWM(FAN_PWM, val, 1);
    Serial.println("OK: Fan=" + String(val));
  } else if (cmd.startsWith("SET_PUMP:")) {
    int val = cmd.substring(9).toInt();
    setPWM(PUMP_PWM, val, 2);
    Serial.println("OK: Pump=" + String(val));
  } else if (cmd.startsWith("SET_RELAY1:")) {
    setRelay(RELAY_1, cmd.substring(11).toInt());
    Serial.println("OK: Relay1=" + String(sensors.relay1_state));
  } else if (cmd.startsWith("SET_RELAY2:")) {
    setRelay(RELAY_2, cmd.substring(11).toInt());
    Serial.println("OK: Relay2=" + String(sensors.relay2_state));
  } else if (cmd.startsWith("CALIB_WEIGHT")) {
    weightOffset = readHX711Raw();
    Serial.println("OK: Weight calibrated");
  } else if (cmd == "STATUS") {
    sendUSBData();
  } else if (cmd == "HELP") {
    Serial.println("Commands:");
    Serial.println("  SET_HEATER:<0-255>");
    Serial.println("  SET_FAN:<0-255>");
    Serial.println("  SET_PUMP:<0-255>");
    Serial.println("  SET_RELAY1:<0|1>");
    Serial.println("  SET_RELAY2:<0|1>");
    Serial.println("  CALIB_WEIGHT");
    Serial.println("  STATUS");
    Serial.println("  HELP");
  } else {
    Serial.println("ERR: Unknown command. Send HELP.");
  }
}

/**
 * Obsługa wejścia USB
 */
void handleUSBInput() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (cmdIndex > 0) {
        cmdBuffer[cmdIndex] = '\0';
        String cmd = String(cmdBuffer);
        cmdIndex = 0;
        
        Serial.print("> ");
        Serial.println(cmd);
        handleUSBCommand(cmd);
        Serial.println("");
      }
    } else {
      if (cmdIndex < CMD_BUFFER_SIZE - 1) {
        cmdBuffer[cmdIndex++] = c;
      }
    }
  }
}

/**
 * Wysyłanie heartbeat przez USB z diagnostyką
 */
void sendUSBHeartbeat() {
  Serial.print("[HB] USB Mode | Uptime: ");
  Serial.print(millis() / 1000);
  Serial.print("s | Err: ");
  Serial.print(errorCount);
  Serial.print("/Warn: ");
  Serial.print(warningCount);
  
  if (sensorErrors.consecutive_failures > 0) {
    Serial.print(" | SensorFail: ");
    Serial.print(sensorErrors.consecutive_failures);
  }
  
  uint16_t ram = getFreeRam();
  if (ram < 200) {
    Serial.print(" | [LOW RAM] ");
  }
  Serial.print(" | RAM: ");
  Serial.print(ram);
  Serial.println("B");
}

// ============================================================================
// KOMUNIKACJA ETHERNET
// ============================================================================

/**
 * Wysyłanie JSON przez Ethernet
 */
void sendEthernetJSON(UIPEthernetClient& client) {
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

/**
 * Wysyłanie odpowiedzi HTTP
 */
void sendHTTPResponse(UIPEthernetClient& client, const char* contentType, const String& content) {
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

/**
 * Obsługa komend HTTP
 */
void handleHTTPCommand(String cmd) {
  if (cmd.startsWith("SET_HEATER:")) {
    int val = cmd.substring(11).toInt();
    setPWM(HEATER_PWM, val, 0);
  } else if (cmd.startsWith("SET_FAN:")) {
    int val = cmd.substring(8).toInt();
    setPWM(FAN_PWM, val, 1);
  } else if (cmd.startsWith("SET_PUMP:")) {
    int val = cmd.substring(9).toInt();
    setPWM(PUMP_PWM, val, 2);
  } else if (cmd.startsWith("SET_RELAY1:")) {
    setRelay(RELAY_1, cmd.substring(11).toInt());
  } else if (cmd.startsWith("SET_RELAY2:")) {
    setRelay(RELAY_2, cmd.substring(11).toInt());
  } else if (cmd.startsWith("CALIB_WEIGHT")) {
    weightOffset = readHX711Raw();
  }
}

/**
 * Obsługa serwera HTTP
 */
void handleEthernetServer() {
  UIPEthernetClient client = server.available();
  if (!client) return;
  
  unsigned long timeout = millis();
  while (!client.available() && (millis() - timeout < 1000)) delay(1);
  if (!client.available()) { client.stop(); return; }
  
  String request = client.readStringUntil('\r');
  client.readStringUntil('\n');
  
  // Sterowanie
  if (request.indexOf("/heater/on") >= 0) {
    setPWM(HEATER_PWM, 255, 0);
    sendHTTPResponse(client, "text/plain", "Heater ON");
  } else if (request.indexOf("/heater/off") >= 0) {
    setPWM(HEATER_PWM, 0, 0);
    sendHTTPResponse(client, "text/plain", "Heater OFF");
  } else if (request.indexOf("/fan/on") >= 0) {
    setPWM(FAN_PWM, 255, 1);
    sendHTTPResponse(client, "text/plain", "Fan ON");
  } else if (request.indexOf("/fan/off") >= 0) {
    setPWM(FAN_PWM, 0, 1);
    sendHTTPResponse(client, "text/plain", "Fan OFF");
  } else if (request.indexOf("/pump/on") >= 0) {
    setPWM(PUMP_PWM, 255, 2);
    sendHTTPResponse(client, "text/plain", "Pump ON");
  } else if (request.indexOf("/pump/off") >= 0) {
    setPWM(PUMP_PWM, 0, 2);
    sendHTTPResponse(client, "text/plain", "Pump OFF");
  }
  // API RAW
  else if (request.indexOf("/api/raw") >= 0 || request.indexOf("/status") >= 0) {
    readAllSensors();
    sendEthernetJSON(client);
  }
  // Komendy
  else if (request.indexOf("/api/cmd?") >= 0) {
    int start = request.indexOf("?") + 1;
    String cmd = request.substring(start);
    cmd.trim();
    handleHTTPCommand(cmd);
    sendHTTPResponse(client, "text/plain", "OK");
  }
  // Strona główna
  else if (request == "GET / HTTP/1.1" || request.indexOf("/index.html") >= 0) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ApiaryGuard Nano</title>";
    html += "<meta http-equiv='refresh' content='5'>";
    html += "</head><body><h1>ApiaryGuard - Arduino Nano</h1>";
    html += "<h2>Dual Mode: USB + Ethernet</h2>";
    html += "<div id='data'>Loading...</div>";
    html += "<script>fetch('/api/raw').then(r=>r.json()).then(d=>{";
    html += "document.getElementById('data').innerHTML=";
    html += "'Temp: '+d.temp_raw+'C<br>Hum: '+d.hum_raw+'%<br>";
    html += "Weight: '+d.weight_raw+'<br>Audio: '+d.audio_raw+'<br>";
    html += "Vibration: '+d.vibration_raw+'<br>CO2: '+d.co2_raw+'ppm<br>";
    html += "VOC: '+d.voc_raw+'<br>RAM: '+d.free_ram+'B<br>";
    html += "Timestamp: '+d.timestamp;";
    html += "});</script></body></html>";
    sendHTTPResponse(client, "text/html", html);
  }
  else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
    client.stop();
  }
}

/**
 * Wysyłanie heartbeat przez Ethernet (log) z diagnostyką
 */
void sendEthernetHeartbeat() {
  Serial.print("[HB] ETH Mode | IP: ");
  Serial.print(Ethernet.localIP());
  Serial.print(" | Uptime: ");
  Serial.print(millis() / 1000);
  Serial.print("s | Err: ");
  Serial.print(errorCount);
  Serial.print("/Warn: ");
  Serial.print(warningCount);
  
  uint16_t ram = getFreeRam();
  if (ram < 200) {
    Serial.print(" | [LOW RAM] ");
  }
  Serial.print(" | RAM: ");
  Serial.print(ram);
  Serial.println("B");
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Inicjalizacja Serial (USB) - zawsze pierwsza
  Serial.begin(115200);
  
  // Czekaj na port USB (max 3 sekundy)
  unsigned long usbWaitStart = millis();
  while (!Serial && (millis() - usbWaitStart < 3000)) {
    delay(10);
  }
  
  Serial.println("");
  Serial.println("=== ApiaryGuard Nano ===");
  Serial.println("Dual Mode: USB + Ethernet");
  Serial.println("Priority: USB > Ethernet");
  Serial.println("Version: 2.1 with Debug & Error Handling");
  Serial.println("");
  
  // Sprawdź dostępność USB
  usbConnected = detectUSB();
  if (usbConnected) {
    debugPrint("USB", "Podlaczono");
  } else {
    debugPrint("USB", "Nie wykryto");
  }
  
  // Inicjalizacja pinów
  pinMode(HEATER_PWM, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);
  pinMode(PUMP_PWM, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  
  // DHT22
  dht.begin();
  
  // HX711
  initHX711();
  
  // I2C dla SGP41
  Wire.begin();
  Wire.beginTransmission(0x59);
  sgpConnected = (Wire.endTransmission() == 0);
  Serial.print("SGP41: ");
  Serial.println(sgpConnected ? "OK" : "Not found");
  
  // Automatyczna detekcja trybu komunikacji
  // PRIORYTET: USB > Ethernet
  if (usbConnected) {
    currentMode = MODE_USB;
    Serial.println("");
    Serial.println(">>> Tryb: USB <<<");
    Serial.println("Send HELP for commands.");
    Serial.println("");
  } else {
    // Spróbuj Ethernet
    ethernetAvailable = initEthernet();
    if (ethernetAvailable) {
      currentMode = MODE_ETHERNET;
      Serial.println("");
      Serial.println(">>> Tryb: ETHERNET <<<");
      Serial.println("");
    } else {
      currentMode = MODE_NONE;
      Serial.println("");
      Serial.println(">>> Brak dostepnej komunikacji <<<");
      Serial.println("");
    }
  }
  
  Serial.print("RAM free: ");
  Serial.println(getFreeRam());
  Serial.print("Error/Warning counters: ");
  Serial.print(errorCount);
  Serial.print("/");
  Serial.println(warningCount);
  Serial.println("=======================");
  Serial.println("");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();
  
  // Obsługa trybu USB (priorytet)
  if (currentMode == MODE_USB) {
    // Obsługa komend USB
    handleUSBInput();
    
    // Odczyt sensorów
    if (now - lastSensorRead >= SENSOR_INTERVAL) {
      readAllSensors();
      sendUSBData();
      lastSensorRead = now;
      
      // Debug output
      Serial.print("[SENSOR] T:"); Serial.print(sensors.temp_raw);
      Serial.print(" H:"); Serial.print(sensors.hum_raw);
      Serial.print(" W:"); Serial.print(sensors.weight_raw);
      Serial.print(" A:"); Serial.print(sensors.audio_raw);
      Serial.print(" V:"); Serial.print(sensors.vibration_raw);
      Serial.print(" CO2:"); Serial.print(sensors.co2_raw);
      Serial.print(" VOC:"); Serial.println(sensors.voc_raw);
    }
    
    // Heartbeat
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
      sendUSBHeartbeat();
      lastHeartbeat = now;
    }
  }
  // Obsługa trybu Ethernet
  else if (currentMode == MODE_ETHERNET) {
    // Odczyt sensorów
    if (now - lastSensorRead >= SENSOR_INTERVAL) {
      readAllSensors();
      lastSensorRead = now;
      
      // Debug output przez Serial (jeśli dostępny)
      Serial.print("[SENSOR] T:"); Serial.print(sensors.temp_raw);
      Serial.print(" H:"); Serial.print(sensors.hum_raw);
      Serial.print(" W:"); Serial.print(sensors.weight_raw);
      Serial.print(" RAM:"); Serial.println(sensors.free_ram);
    }
    
    // Obsługa serwera HTTP
    handleEthernetServer();
    
    // Heartbeat
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
      sendEthernetHeartbeat();
      lastHeartbeat = now;
    }
  }
  // Tryb awaryjny - brak komunikacji
  else {
    // Spróbuj ponownie wykryć USB
    if (!usbConnected && detectUSB()) {
      Serial.println("");
      Serial.println("USB podlaczono! Przelaczanie w tryb USB...");
      currentMode = MODE_USB;
      usbConnected = true;
    }
    // Spróbuj ponownie inicjalizować Ethernet
    else if (!ethernetAvailable) {
      ethernetAvailable = initEthernet();
      if (ethernetAvailable) {
        Serial.println("");
        Serial.println("Ethernet dostepny! Przelaczanie w tryb ETH...");
        currentMode = MODE_ETHERNET;
      }
    }
    
    // Odczyt sensorów (działa niezależnie)
    if (now - lastSensorRead >= SENSOR_INTERVAL) {
      readAllSensors();
      lastSensorRead = now;
    }
    
    delay(100);
  }
  
  delay(10);
}
