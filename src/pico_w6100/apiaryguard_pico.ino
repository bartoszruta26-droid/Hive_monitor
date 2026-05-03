/**
 * ApiaryGuard - Raspberry Pi Pico + W6100 Ethernet
 * Kompletny system monitoringu i sterowania ulami
 * 
 * Wymagane biblioteki (zainstaluj przez Arduino IDE Board Manager):
 * - Raspberry Pi Pico/RP2040 by Earle F. Philhower III
 * - Ethernet by Arduino (dla W6100)
 * - DHT sensor library by Adafruit
 * - Adafruit SGP41 Library
 * 
 * Połączenia GPIO:
 * W6100 (SPI1):
 *   CS   -> GP5
 *   MOSI -> GP7 (SPI1 TX)
 *   MISO -> GP8 (SPI1 RX)
 *   SCK  -> GP6 (SPI1 SCK)
 *   RST  -> GP4
 *   INT  -> GP3
 */

#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <EEPROM.h>

// ==================== KONFIGURACJA SIECI ====================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(8080);

// ==================== PINY GPIO RASPBERRY PICO ====================
// W6100 SPI1
#define W6100_CS   5
#define W6100_MOSI 7
#define W6100_MISO 8
#define W6100_SCK  6
#define W6100_RST  4
#define W6100_INT  3

// HX711 (Waga)
#define HX711_DT   9
#define HX711_SCK  10

// DHT22 (Temperatura i Wilgotność)
#define DHT_PIN    11
#define DHT_TYPE   DHT22

// PWM Efektory
#define HEATER_PWM     12  // Grzałka 10W
#define FAN_PWM        13  // Wentylator 12V
#define PUMP_PWM       14  // Pompa perystaltyczna

// Przekaźniki (GPIO)
#define RELAY_1        15  // Zawór 1
#define RELAY_2        16  // Zawór 2
#define RELAY_3        17  // Zawór 3
#define RELAY_4        18  // Zawór 4
#define RELAY_5        19  // Dodatkowy
#define RELAY_6        20  // Dodatkowy
#define RELAY_7        21  // Dodatkowy
#define RELAY_8        22  // Dodatkowy

// Audio MEMS Microphone
#define MIC_PIN        26  // ADC0

// Piezo Vibration Sensor
#define PIEZO_PIN      27  // ADC1

// LD2410B Radar MMWave (UART1)
#define RADAR_RX       28  // UART1 RX (do radaru TX)
#define RADAR_TX       29  // UART1 TX (do radaru RX)

// ==================== OBIEKTY ====================
EthernetClient client;
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SGP41 sgp;
HardwareSerial& radarSerial = Serial1;

// ==================== ZMIENNE GLOBALNE ====================
unsigned long lastHeartbeat = 0;
unsigned long lastSensorRead = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000;  // 10 sekund
const unsigned long SENSOR_INTERVAL = 1000;       // 1 sekunda

// Dane z sensorów
struct SensorData {
  float temperature;
  float humidity;
  long weight;
  int audioLevel;
  int vibrationLevel;
  int co2;
  int voc;
  bool motionDetected;
  uint8_t relayState;
} sensors;

// Kalibracja wagi
long weightOffset = 0;
float weightScale = 1.0;

// ==================== FUNKCJE POMOCNICZE ====================

// Inicjalizacja W6100
bool initW6100() {
  pinMode(W6100_CS, OUTPUT);
  digitalWrite(W6100_CS, HIGH);
  
  pinMode(W6100_RST, OUTPUT);
  digitalWrite(W6100_RST, LOW);
  delay(100);
  digitalWrite(W6100_RST, HIGH);
  delay(200);
  
  pinMode(W6100_INT, INPUT);
  
  // Konfiguracja SPI1 dla W6100
  SPI.setRX(W6100_MISO);
  SPI.setTX(W6100_MOSI);
  SPI.setSCK(W6100_SCK);
  SPI.begin();
  
  // Reset W6100
  digitalWrite(W6100_CS, LOW);
  delay(10);
  digitalWrite(W6100_CS, HIGH);
  delay(100);
  
  // Inicjalizacja Ethernet
  Ethernet.init(W6100_CS);
  
  if (Ethernet.begin(mac, ip, gateway, subnet)) {
    Serial.println("W6100: Połączono");
    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());
    return true;
  } else {
    Serial.println("W6100: Błąd konfiguracji DHCP/statycznego IP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("W6100: Nie wykryto sprzętu!");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("W6100: Kabel niepodłączony");
    }
    return false;
  }
}

// Inicjalizacja HX711
void initHX711() {
  pinMode(HX711_SCK, OUTPUT);
  pinMode(HX711_DT, INPUT);
  digitalWrite(HX711_SCK, LOW);
  
  // Kalibracja zerowa
  weightOffset = readHX711();
  Serial.print("HX711 Offset: ");
  Serial.println(weightOffset);
}

// Odczyt HX711
long readHX711() {
  long value = 0;
  while (digitalRead(HX711_DT));
  
  for (int i = 0; i < 24; i++) {
    digitalWrite(HX711_SCK, HIGH);
    value = value << 1;
    if (digitalRead(HX711_DT)) value++;
    digitalWrite(HX711_SCK, LOW);
  }
  
  // 25 impuls dla następnego odczytu (gain 128)
  digitalWrite(HX711_SCK, HIGH);
  digitalWrite(HX711_SCK, LOW);
  
  value -= weightOffset;
  return (long)(value * weightScale);
}

// Odczyt mikrofonu MEMS (RMS)
int readMic() {
  int sum = 0;
  int samples = 100;
  
  for (int i = 0; i < samples; i++) {
    int val = analogRead(MIC_PIN);
    sum += (val - 2048) * (val - 2048);  // 12-bit ADC, środek ~2048
    delayMicroseconds(100);
  }
  
  return sqrt(sum / samples);
}

// Odczyt piezo
int readPiezo() {
  return analogRead(PIEZO_PIN);
}

// Odczyt radaru LD2410B
bool readRadar() {
  static uint8_t buffer[20];
  static int idx = 0;
  
  while (radarSerial.available()) {
    uint8_t b = radarSerial.read();
    
    // Szukaj nagłówka pakietu
    if (idx == 0 && b != 0xF4) continue;
    if (idx == 1 && b != 0xF3) { idx = 0; continue; }
    
    buffer[idx++] = b;
    
    if (idx >= 8) {  // Minimalny pakiet
      // Sprawdź czy wykryto ruch (bajt 6-7 to odległość)
      if (buffer[6] > 0 || buffer[7] > 0) {
        return true;
      }
      idx = 0;
    }
  }
  return false;
}

// Ustawienie PWM (0-255)
void setPWM(int pin, int value) {
  analogWrite(pin, constrain(value, 0, 255));
}

// Ustawienie przekaźników
void setRelays(uint8_t state) {
  sensors.relayState = state;
  digitalWrite(RELAY_1, bitRead(state, 0));
  digitalWrite(RELAY_2, bitRead(state, 1));
  digitalWrite(RELAY_3, bitRead(state, 2));
  digitalWrite(RELAY_4, bitRead(state, 3));
  digitalWrite(RELAY_5, bitRead(state, 4));
  digitalWrite(RELAY_6, bitRead(state, 5));
  digitalWrite(RELAY_7, bitRead(state, 6));
  digitalWrite(RELAY_8, bitRead(state, 7));
}

// Odczyt wszystkich sensorów
void readAllSensors() {
  // Temperatura i wilgotność
  sensors.temperature = dht.readTemperature();
  sensors.humidity = dht.readHumidity();
  
  // Waga
  sensors.weight = readHX711();
  
  // Audio
  sensors.audioLevel = readMic();
  
  // Wibracje
  sensors.vibrationLevel = readPiezo();
  
  // CO2 i VOC (SGP41)
  sensors.co2 = sgp.measureCO2();
  sensors.voc = sgp.measureVocIndex();
  
  // Radar
  sensors.motionDetected = readRadar();
}

// Wysyłanie danych przez HTTP
void sendData() {
  if (client.connected()) {
    StaticJsonDocument<512> doc;
    doc["temp"] = sensors.temperature;
    doc["hum"] = sensors.humidity;
    doc["weight"] = sensors.weight;
    doc["audio"] = sensors.audioLevel;
    doc["vibration"] = sensors.vibrationLevel;
    doc["co2"] = sensors.co2;
    doc["voc"] = sensors.voc;
    doc["motion"] = sensors.motionDetected;
    doc["relays"] = sensors.relayState;
    doc["timestamp"] = millis();
    
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
}

// Obsługa komend HTTP
void handleCommand(String cmd) {
  if (cmd.startsWith("SET_RELAYS:")) {
    uint8_t state = cmd.substring(11).toInt();
    setRelays(state);
  } else if (cmd.startsWith("SET_HEATER:")) {
    int value = cmd.substring(11).toInt();
    setPWM(HEATER_PWM, value);
  } else if (cmd.startsWith("SET_FAN:")) {
    int value = cmd.substring(8).toInt();
    setPWM(FAN_PWM, value);
  } else if (cmd.startsWith("SET_PUMP:")) {
    int value = cmd.substring(9).toInt();
    setPWM(PUMP_PWM, value);
  } else if (cmd.startsWith("CALIB_WEIGHT")) {
    weightOffset = readHX711();
    Serial.println("Waga skalibrowana");
  }
}

// Serwer HTTP
void handleServer() {
  EthernetClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.readStringUntil('\n');  // Pomiń resztę nagłówka
    
    if (request.indexOf("/api/data") >= 0) {
      sendData();
    } else if (request.indexOf("/api/cmd?") >= 0) {
      int start = request.indexOf("?") + 1;
      String cmd = request.substring(start);
      cmd.trim();
      handleCommand(cmd);
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("OK");
    } else {
      client.println("HTTP/1.1 404 Not Found");
      client.println();
    }
    delay(1);
    client.stop();
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);  // Czekaj na USB
  
  Serial.println("\n=== ApiaryGuard Pico W6100 ===");
  
  // Inicjalizacja EEPROM
  EEPROM.begin(512);
  
  // Inicjalizacja W6100
  if (!initW6100()) {
    Serial.println("BŁĄD: W6100 nie działa!");
    while (1);  // Zatrzymaj jeśli brak sieci
  }
  
  // Start serwera
  server.begin();
  Serial.println("Serwer HTTP na porcie 8080");
  
  // Inicjalizacja sensorów
  dht.begin();
  
  // I2C dla SGP41
  Wire.setSDA(2);  // GP2
  Wire.setSCL(3);  // GP3
  Wire.begin();
  
  if (!sgp.begin(&Wire)) {
    Serial.println("SGP41 nie wykryty!");
  } else {
    sgp.measureCO2();
    sgp.measureVocIndex();
  }
  
  // UART1 dla radaru
  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
  
  // HX711
  initHX711();
  
  // Piny wyjściowe
  pinMode(HEATER_PWM, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);
  pinMode(PUMP_PWM, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);
  pinMode(RELAY_5, OUTPUT);
  pinMode(RELAY_6, OUTPUT);
  pinMode(RELAY_7, OUTPUT);
  pinMode(RELAY_8, OUTPUT);
  
  // Domyślne wyłączenie
  setRelays(0);
  setPWM(HEATER_PWM, 0);
  setPWM(FAN_PWM, 0);
  setPWM(PUMP_PWM, 0);
  
  Serial.println("Inicjalizacja zakończona");
}

// ==================== LOOP ====================
void loop() {
  unsigned long now = millis();
  
  // Obsługa klienta Ethernet
  handleServer();
  
  // Odczyt sensorów co 1s
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    readAllSensors();
    lastSensorRead = now;
    
    // Debug
    Serial.print("Temp: "); Serial.print(sensors.temperature);
    Serial.print(" Hum: "); Serial.print(sensors.humidity);
    Serial.print(" Weight: "); Serial.print(sensors.weight);
    Serial.print(" Motion: "); Serial.println(sensors.motionDetected ? "YES" : "NO");
  }
  
  // Heartbeat co 10s
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    Serial.println("Heartbeat");
    lastHeartbeat = now;
  }
  
  delay(10);
}
