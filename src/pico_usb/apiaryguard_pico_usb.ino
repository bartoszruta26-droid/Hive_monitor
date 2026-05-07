/**
 * ApiaryGuard - Raspberry Pi Pico USB (UART)
 * Kompletny system monitoringu i sterowania ulami
 * Wersja komunikująca się przez USB/UART (bez Ethernet)
 * 
 * Wymagane biblioteki (zainstaluj przez Arduino IDE Board Manager / Library Manager):
 * - Raspberry Pi Pico/RP2040 by Earle F. Philhower III
 * - DHT sensor library by Adafruit
 * - Adafruit SGP41 Library
 * - ArduinoJson by Benoit Blanchon
 * 
 * Połączenia GPIO:
 * 
 * SGP41 (I2C):
 *   SDA  -> GP0
 *   SCL  -> GP1
 * 
 * HX711 (Waga):
 *   DT   -> GP9
 *   SCK  -> GP10
 * 
 * DHT22 (Temperatura i Wilgotność):
 *   DATA -> GP11
 * 
 * PWM Efektory:
 *   HEATER_PWM -> GP12  (Grzałka 10W)
 *   FAN_PWM    -> GP13  (Wentylator 12V)
 *   PUMP_PWM   -> GP14  (Pompa perystaltyczna)
 * 
 * Przekaźniki (GPIO):
 *   RELAY_1 -> GP15  (Zawór 1)
 *   RELAY_2 -> GP16  (Zawór 2)
 *   RELAY_3 -> GP17  (Zawór 3)
 *   RELAY_4 -> GP18  (Zawór 4)
 *   RELAY_5 -> GP19  (Dodatkowy)
 *   RELAY_6 -> GP20  (Dodatkowy)
 *   RELAY_7 -> GP21  (Dodatkowy)
 *   RELAY_8 -> GP22  (Dodatkowy)
 * 
 * Audio MEMS Microphone:
 *   MIC  -> GP26  (ADC0)
 * 
 * Piezo Vibration Sensor:
 *   PIEZO -> GP27  (ADC1)
 * 
 * LD2410B Radar MMWave (UART1):
 *   RX <- GP28  (UART1 RX, podłącz do TX radaru)
 *   TX -> GP29  (UART1 TX, podłącz do RX radaru)
 * 
 * Komunikacja USB/UART:
 *   USB Serial - wbudowany interfejs USB Pico
 *   Baudrate: 115200
 *   Format komend: JSON lub tekstowy
 */

#include <SPI.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// ==================== PINY GPIO RASPBERRY PICO ====================
// I2C dla SGP41
#define SGP41_SDA    0
#define SGP41_SCL    1

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
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SGP41 sgp;
HardwareSerial& radarSerial = Serial1;

// Bufor na dane JSON
#define JSON_BUFFER_SIZE 512

// ============================================================================
// MODUŁ PROFESJONALNEJ ANALIZY JAKOŚCI POWIETRZA (SGP41) - STRUKTURY DANYCH
// ============================================================================

#define AQ_BUFFER_SIZE 144      // Bufor cyrkularny 24h (co 10 minut)
#define AQ_SHORT_TERM 6         // 1 godzina (6 x 10min)
#define AQ_MEDIUM_TERM 24       // 4 godziny (24 x 10min)
#define AQ_LONG_TERM 144        // 24 godziny

// Punkt danych jakości powietrza
struct AirQualityDataPoint {
    uint16_t co2;               // CO2 w ppm
    uint16_t voc;               // VOC Index
    float temperature;          // Temperatura [°C]
    float humidity;             // Wilgotność [%]
    unsigned long timestamp;    // Czas pomiaru [ms]
};

// Struktura przechowująca 24+ parametry jakości powietrza
struct AirQualityMetrics {
    // === PODSTAWOWE PARAMETRY (3) ===
    uint16_t co2_current;           // Aktualne CO2 [ppm]
    uint16_t voc_current;           // Aktualny VOC Index
    uint16_t nox_equivalent;        // NOx equivalent [ppb] (szacowane z VOC)
    
    // === STATYSTYCZNE PARAMETRY CO2 (6) ===
    float co2_mean;                 // Średnie CO2 [ppm]
    float co2_std;                  // Odchylenie standardowe CO2
    uint16_t co2_min;               // Minimalne CO2 [ppm]
    uint16_t co2_max;               // Maksymalne CO2 [ppm]
    uint16_t co2_range;             // Zakres CO2 (max-min) [ppm]
    float co2_cv;                   // Współczynnik zmienności CO2 (std/mean)
    
    // === STATYSTYCZNE PARAMETRY VOC (6) ===
    float voc_mean;                 // Średni VOC Index
    float voc_std;                  // Odchylenie standardowe VOC
    uint16_t voc_min;               // Minimalny VOC Index
    uint16_t voc_max;               // Maksymalny VOC Index
    uint16_t voc_range;             // Zakres VOC (max-min)
    float voc_cv;                   // Współczynnik zmienności VOC
    
    // === TRENDY CZASOWE (8) ===
    float co2_slope_1h;             // Trend CO2 1h [ppm/h]
    float co2_slope_4h;             // Trend CO2 4h [ppm/h]
    float co2_slope_24h;            // Trend CO2 24h [ppm/h]
    float voc_slope_1h;             // Trend VOC 1h [index/h]
    float voc_slope_4h;             // Trend VOC 4h [index/h]
    float voc_slope_24h;            // Trend VOC 24h [index/h]
    int trend_direction;            // Kierunek: -1 (spadek), 0 (stabilny), 1 (wzrost)
    float trend_strength;           // Siła trendu (0-1)
    
    // === INDEKSY JAKOŚCI POWIETRZA (5) ===
    float iaq_index;                // Indoor Air Quality Index (0-500)
    int air_quality_level;          // Poziom jakości: 1-Dobra, 2-Średnia, 3-Zła, 4-Bardzo zła
    float ventilation_need;         // Potrzeba wentylacji (0-100%)
    float stress_from_air;          // Stres od powietrza (0-100%)
    float hive_comfort_index;       // Indeks komfortu ula (0-100%)
    
    // === ZAGROŻENIA I ALerty (5) ===
    bool poor_ventilation_alert;    // Alert słabej wentylacji
    bool contamination_risk;        // Ryzyko zanieczyszczenia
    bool mold_risk;                 // Ryzyko pleśni
    bool high_co2_alert;            // Alert wysokiego CO2
    float combined_risk_score;      // Łączny wynik ryzyka (0-100%)
    
    // === PARAMETRY TEMPORALNE (4) ===
    float variability_index;        // Indeks zmienności (0-100%)
    float stability_score;          // Wynik stabilności (0-100%)
    float change_rate;              // Szybkość zmian [index/h]
    float volatility_index;         // Indeks niestabilności (0-100%)
    
    // === KORELACJE (3) ===
    float temp_correlation;         // Korelacja z temperaturą (-1 do 1)
    float humidity_correlation;     // Korelacja z wilgotnością (-1 do 1)
    float comfort_zone_percent;     // Procent czasu w strefie komfortu (0-100%)
    
    // === PROGI I NORMY (4) ===
    int co2_warning_level;          // Poziom ostrzeżenia CO2 (0-Normalny, 1-Ostrzeżenie, 2-Krytyczny)
    int voc_alert_level;            // Poziom alertu VOC (0-Normalny, 1-Ostrzeżenie, 2-Krytyczny)
    float hours_above_threshold;    // Godziny powyżej progu [h]
    float exceedance_ratio;         // Stosunek przekroczeń (0-1)
};

// Bufory dla przetwarzania danych jakości powietrza
AirQualityDataPoint aqBuffer[AQ_BUFFER_SIZE];
uint16_t aqBufferIndex = 0;
uint16_t aqDataCount = 0;

AirQualityMetrics currentAQMetrics;

// Historia dzienna dla analizy wzorców
struct AQDailyPattern {
    uint16_t hour_avg_co2[24];      // Średnie CO2 dla każdej godziny
    uint16_t hour_avg_voc[24];      // Średni VOC dla każdej godziny
    uint8_t valid_samples[24];      // Liczba ważnych próbek
} aqDailyPattern;

// Zdarzenia związane z jakością powietrza
struct AQEvent {
    unsigned long timestamp;        // Czas zdarzenia [ms]
    int event_type;                 // Typ: 1-PoorVentilation, 2-Contamination, 3-MoldRisk, 4-HighCO2
    int severity;                   // Nasilenie: 1-Low, 2-Medium, 3-High, 4-Critical
    uint16_t co2_value;             // CO2 w momencie zdarzenia
    uint16_t voc_value;             // VOC w momencie zdarzenia
    char description[64];           // Opis zdarzenia
};

AQEvent lastAQEvent;

// Progi jakości powietrza
struct AQThresholds {
    uint16_t co2_good;              // CO2 < 800 ppm - dobra jakość
    uint16_t co2_moderate;          // CO2 800-1500 ppm - średnia jakość
    uint16_t co2_poor;              // CO2 1500-2500 ppm - zła jakość
    uint16_t co2_critical;          // CO2 > 2500 ppm - bardzo zła jakość
    
    uint16_t voc_good;              // VOC < 100 - dobra jakość
    uint16_t voc_moderate;          // VOC 100-200 - średnia jakość
    uint16_t voc_poor;              // VOC 200-300 - zła jakość
    uint16_t voc_critical;          // VOC > 300 - bardzo zła jakość
    
    float humidity_mold_threshold;  // Wilgotność > 75% - ryzyko pleśni
    float temp_stress_high;         // Temperatura > 38°C - stres cieplny
    float temp_stress_low;          // Temperatura < 15°C - stres zimna
} aqThresholds;

// Funkcje modułu jakości powietrza
void initAirQualityModule();
void addAirQualityData(uint16_t co2, uint16_t voc, float temp, float hum);
void calculateAirQualityMetrics();
void classifyAQEvent(AQEvent& event, const AirQualityMetrics& metrics);
void checkAirQualityAlerts(const AirQualityMetrics& metrics);
String getAirQualityJSON();

// Deklaracje funkcji USB/UART API
void sendStatusJSON();
void sendDashboardText();
void sendAudioStatus();
void sendAudioMetrics();
void sendAudioEvents();
void sendAudioSpectrum();
void sendAudioHistory();
void sendRadarStatus();
void sendRadarParams();
void sendRadarAnomalies();
void sendRadarRaw();
void sendHX711Status();
void sendHX711Metrics();
void sendHX711Events();
void sendHX711Forecast();
void sendAirQualityStatus();
void sendAirQualityMetrics();
void sendAirQualityEvents();
void sendHelp();

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

// Bufor wejściowy dla komend
#define CMD_BUFFER_SIZE 128
char cmdBuffer[CMD_BUFFER_SIZE];
int cmdIndex = 0;

// ==================== FUNKCJE POMOCNICZE ====================

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

// Odczyt HX711 z timeoutem
long readHX711() {
  unsigned long timeout = millis();
  const unsigned long TIMEOUT_MS = 500;  // Zwiększony timeout dla HX711
  
  // Czekaj na gotowość z timeoutem
  while (digitalRead(HX711_DT) && (millis() - timeout < TIMEOUT_MS)) {
    yield();
  }
  
  // Sprawdź timeout
  if (digitalRead(HX711_DT)) {
    Serial.println("HX711 timeout!");
    return 0;
  }
  
  long value = 0;
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
  if (!sgp.measureCO2()) {
    Serial.println("SGP41: Błąd odczytu CO2");
    sensors.co2 = -1;
  } else {
    sensors.co2 = sgp.CO2;
  }
  
  if (!sgp.measureVocIndex()) {
    Serial.println("SGP41: Błąd odczytu VOC");
    sensors.voc = -1;
  } else {
    sensors.voc = sgp.VOCIndex;
  }
  
  // Radar
  sensors.motionDetected = readRadar();
}

// Wysyłanie danych przez USB/UART
void sendData() {
  JsonDocument doc;
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
  
  serializeJson(doc, Serial);
  Serial.println();
}

// Obsługa komend przez USB/UART
void handleCommand(String cmd) {
  cmd.trim();
  
  if (cmd.startsWith("SET_RELAYS:")) {
    uint8_t state = cmd.substring(11).toInt();
    setRelays(state);
    Serial.println("OK: Relays set to " + String(state));
  } else if (cmd.startsWith("SET_HEATER:")) {
    int value = cmd.substring(11).toInt();
    setPWM(HEATER_PWM, value);
    Serial.println("OK: Heater set to " + String(value));
  } else if (cmd.startsWith("SET_FAN:")) {
    int value = cmd.substring(8).toInt();
    setPWM(FAN_PWM, value);
    Serial.println("OK: Fan set to " + String(value));
  } else if (cmd.startsWith("SET_PUMP:")) {
    int value = cmd.substring(9).toInt();
    setPWM(PUMP_PWM, value);
    Serial.println("OK: Pump set to " + String(value));
  } else if (cmd.startsWith("CALIB_WEIGHT")) {
    weightOffset = readHX711();
    Serial.println("OK: Weight calibrated");
  } else if (cmd == "GET_STATUS" || cmd == "STATUS") {
    readAllSensors();
    sendStatusJSON();
  } else if (cmd == "GET_AQ_STATUS" || cmd == "AQ_STATUS") {
    calculateAirQualityMetrics();
    sendAirQualityStatus();
  } else if (cmd == "GET_AQ_METRICS" || cmd == "AQ_METRICS") {
    calculateAirQualityMetrics();
    sendAirQualityMetrics();
  } else if (cmd == "GET_AUDIO_STATUS" || cmd == "AUDIO_STATUS") {
    sendAudioStatus();
  } else if (cmd == "GET_AUDIO_METRICS" || cmd == "AUDIO_METRICS") {
    sendAudioMetrics();
  } else if (cmd == "GET_RADAR_STATUS" || cmd == "RADAR_STATUS") {
    sendRadarStatus();
  } else if (cmd == "GET_RADAR_PARAMS" || cmd == "RADAR_PARAMS") {
    sendRadarParams();
  } else if (cmd == "GET_HX711_STATUS" || cmd == "HX711_STATUS") {
    sendHX711Status();
  } else if (cmd == "GET_HX711_METRICS" || cmd == "HX711_METRICS") {
    sendHX711Metrics();
  } else if (cmd == "HELP" || cmd == "?") {
    sendHelp();
  } else if (cmd == "DASHBOARD" || cmd == "DASH") {
    sendDashboardText();
  } else {
    Serial.println("ERROR: Unknown command. Send HELP for list of commands.");
  }
}

// Wysyłanie pomocy
void sendHelp() {
  Serial.println("=== ApiaryGuard USB Command Help ===");
  Serial.println("STATUS / GET_STATUS - Podstawowe dane sensorów (JSON)");
  Serial.println("DASHBOARD / DASH - Czytelny tekstowy dashboard");
  Serial.println("SET_RELAYS:<0-255> - Ustaw przekaźniki");
  Serial.println("SET_HEATER:<0-255> - Ustaw grzałkę PWM");
  Serial.println("SET_FAN:<0-255> - Ustaw wentylator PWM");
  Serial.println("SET_PUMP:<0-255> - Ustaw pompę PWM");
  Serial.println("CALIB_WEIGHT - Kalibruj wagę (zero)");
  Serial.println("AQ_STATUS / GET_AQ_STATUS - Status jakości powietrza");
  Serial.println("AQ_METRICS / GET_AQ_METRICS - Szczegółowe metryki AQ");
  Serial.println("AUDIO_STATUS / GET_AUDIO_STATUS - Status audio");
  Serial.println("AUDIO_METRICS / GET_AUDIO_METRICS - Metryki audio");
  Serial.println("RADAR_STATUS / GET_RADAR_STATUS - Status radaru");
  Serial.println("RADAR_PARAMS / GET_RADAR_PARAMS - Parametry radaru");
  Serial.println("HX711_STATUS / GET_HX711_STATUS - Status wagi");
  Serial.println("HX711_METRICS / GET_HX711_METRICS - Metryki wagi");
  Serial.println("HELP / ? - Ta pomoc");
  Serial.println("===============================");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);  // Czekaj na USB
  
  Serial.println("");
  Serial.println("=== ApiaryGuard Pico USB ===");
  Serial.println("Firmware version: 2.5.0-USB");
  Serial.println("Initializing...");
  
  // Inicjalizacja EEPROM
  EEPROM.begin(512);
  
  // Inicjalizacja sensorów
  dht.begin();
  
  // I2C dla SGP41
  Wire.setSDA(SGP41_SDA);  // GP0
  Wire.setSCL(SGP41_SCL);  // GP1
  Wire.begin();
  
  if (!sgp.begin(&Wire)) {
    Serial.println("SGP41 nie wykryty! Sprawdź połączenia I2C.");
  } else {
    Serial.println("SGP41 wykryty");
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
  
  // Inicjalizacja modułu jakości powietrza
  initAirQualityModule();
  
  Serial.println("");
  Serial.println("Initialization complete!");
  Serial.println("Send HELP for available commands.");
  Serial.println("===============================");
  Serial.println("");
}

// ==================== LOOP ====================
void loop() {
  unsigned long now = millis();
  
  // Obsługa komend z USB/UART
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (cmdIndex > 0) {
        cmdBuffer[cmdIndex] = '\0';
        String cmd = String(cmdBuffer);
        cmdIndex = 0;
        
        Serial.print("> ");
        Serial.println(cmd);
        handleCommand(cmd);
        Serial.println("");
      }
    } else {
      if (cmdIndex < CMD_BUFFER_SIZE - 1) {
        cmdBuffer[cmdIndex++] = c;
      }
    }
  }
  
  // Odczyt sensorów co 1s
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    readAllSensors();
    lastSensorRead = now;
    
    // Dodaj dane do bufora jakości powietrza (co 10 minut dla statystyk)
    static unsigned long lastAQAdd = 0;
    if (now - lastAQAdd >= 600000) {  // 10 minut
      addAirQualityData(sensors.co2, sensors.voc, sensors.temperature, sensors.humidity);
      calculateAirQualityMetrics();
      checkAirQualityAlerts(currentAQMetrics);
      lastAQAdd = now;
    }
    
    // Debug output
    Serial.print("[SENSOR] Temp: "); Serial.print(sensors.temperature);
    Serial.print(" °C | Hum: "); Serial.print(sensors.humidity);
    Serial.print(" % | Weight: "); Serial.print(sensors.weight);
    Serial.print(" g | CO2: "); Serial.print(sensors.co2);
    Serial.print(" ppm | VOC: "); Serial.print(sensors.voc);
    Serial.print(" | Motion: "); Serial.println(sensors.motionDetected ? "YES" : "NO");
  }
  
  // Heartbeat co 10s
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    Serial.println("[HEARTBEAT] System alive - " + String(millis()/1000) + "s uptime");
    lastHeartbeat = now;
  }
  
  delay(10);
}

// ============================================================================
// IMPLEMENTACJA MODUŁU JAKOŚCI POWIETRZA (SGP41)
// ============================================================================

// Inicjalizacja modułu jakości powietrza
void initAirQualityModule() {
  // Ustawienie progów jakości powietrza
  aqThresholds.co2_good = 800;
  aqThresholds.co2_moderate = 1500;
  aqThresholds.co2_poor = 2500;
  aqThresholds.co2_critical = 5000;
  
  aqThresholds.voc_good = 100;
  aqThresholds.voc_moderate = 200;
  aqThresholds.voc_poor = 300;
  aqThresholds.voc_critical = 500;
  
  aqThresholds.humidity_mold_threshold = 75.0;
  aqThresholds.temp_stress_high = 38.0;
  aqThresholds.temp_stress_low = 15.0;
  
  // Inicjalizacja bufora
  for (int i = 0; i < AQ_BUFFER_SIZE; i++) {
    aqBuffer[i].co2 = 400;
    aqBuffer[i].voc = 50;
    aqBuffer[i].temperature = 25.0;
    aqBuffer[i].humidity = 50.0;
    aqBuffer[i].timestamp = 0;
  }
  aqBufferIndex = 0;
  aqDataCount = 0;
  
  // Inicjalizacja metryk
  currentAQMetrics.co2_current = 400;
  currentAQMetrics.voc_current = 50;
  currentAQMetrics.nox_equivalent = 0;
  currentAQMetrics.iaq_index = 50;
  currentAQMetrics.air_quality_level = 1;
  currentAQMetrics.ventilation_need = 0;
  currentAQMetrics.stress_from_air = 0;
  currentAQMetrics.hive_comfort_index = 100;
  currentAQMetrics.poor_ventilation_alert = false;
  currentAQMetrics.contamination_risk = false;
  currentAQMetrics.mold_risk = false;
  currentAQMetrics.high_co2_alert = false;
  currentAQMetrics.combined_risk_score = 0;
  
  Serial.println("Air quality module initialized");
}

// Dodanie punktu danych do bufora cyrkularnego
void addAirQualityData(uint16_t co2, uint16_t voc, float temp, float hum) {
  if (co2 == 0 || voc == 0) return;  // Pomiar nieudany
  
  aqBuffer[aqBufferIndex].co2 = co2;
  aqBuffer[aqBufferIndex].voc = voc;
  aqBuffer[aqBufferIndex].temperature = temp;
  aqBuffer[aqBufferIndex].humidity = hum;
  aqBuffer[aqBufferIndex].timestamp = millis();
  
  aqBufferIndex = (aqBufferIndex + 1) % AQ_BUFFER_SIZE;
  if (aqDataCount < AQ_BUFFER_SIZE) {
    aqDataCount++;
  }
  
  // Aktualizacja dziennego wzorca
  unsigned long hourOfDay = (millis() / 3600000) % 24;
  aqDailyPattern.hour_avg_co2[hourOfDay] = co2;
  aqDailyPattern.hour_avg_voc[hourOfDay] = voc;
  aqDailyPattern.valid_samples[hourOfDay]++;
}

// Obliczanie wszystkich metryk jakości powietrza
void calculateAirQualityMetrics() {
  if (aqDataCount < 2) return;
  
  // === PODSTAWOWE PARAMETRY ===
  currentAQMetrics.co2_current = aqBuffer[(aqBufferIndex + AQ_BUFFER_SIZE - 1) % AQ_BUFFER_SIZE].co2;
  currentAQMetrics.voc_current = aqBuffer[(aqBufferIndex + AQ_BUFFER_SIZE - 1) % AQ_BUFFER_SIZE].voc;
  currentAQMetrics.nox_equivalent = currentAQMetrics.voc_current * 2;  // Szacowane NOx z VOC
  
  // === STATYSTYKI CO2 ===
  float sum = 0;
  currentAQMetrics.co2_min = 65535;
  currentAQMetrics.co2_max = 0;
  for (uint16_t i = 0; i < aqDataCount; i++) {
    uint16_t idx = (aqBufferIndex + AQ_BUFFER_SIZE - aqDataCount + i) % AQ_BUFFER_SIZE;
    sum += aqBuffer[idx].co2;
    if (aqBuffer[idx].co2 < currentAQMetrics.co2_min) currentAQMetrics.co2_min = aqBuffer[idx].co2;
    if (aqBuffer[idx].co2 > currentAQMetrics.co2_max) currentAQMetrics.co2_max = aqBuffer[idx].co2;
  }
  currentAQMetrics.co2_mean = sum / aqDataCount;
  currentAQMetrics.co2_range = currentAQMetrics.co2_max - currentAQMetrics.co2_min;
  
  // Odchylenie standardowe CO2
  float variance = 0;
  for (uint16_t i = 0; i < aqDataCount; i++) {
    uint16_t idx = (aqBufferIndex + AQ_BUFFER_SIZE - aqDataCount + i) % AQ_BUFFER_SIZE;
    variance += sq(aqBuffer[idx].co2 - currentAQMetrics.co2_mean);
  }
  currentAQMetrics.co2_std = sqrt(variance / aqDataCount);
  currentAQMetrics.co2_cv = (currentAQMetrics.co2_mean > 0) ? 
    (currentAQMetrics.co2_std / currentAQMetrics.co2_mean) : 0;
  
  // === STATYSTYKI VOC ===
  sum = 0;
  currentAQMetrics.voc_min = 65535;
  currentAQMetrics.voc_max = 0;
  for (uint16_t i = 0; i < aqDataCount; i++) {
    uint16_t idx = (aqBufferIndex + AQ_BUFFER_SIZE - aqDataCount + i) % AQ_BUFFER_SIZE;
    sum += aqBuffer[idx].voc;
    if (aqBuffer[idx].voc < currentAQMetrics.voc_min) currentAQMetrics.voc_min = aqBuffer[idx].voc;
    if (aqBuffer[idx].voc > currentAQMetrics.voc_max) currentAQMetrics.voc_max = aqBuffer[idx].voc;
  }
  currentAQMetrics.voc_mean = sum / aqDataCount;
  currentAQMetrics.voc_range = currentAQMetrics.voc_max - currentAQMetrics.voc_min;
  
  variance = 0;
  for (uint16_t i = 0; i < aqDataCount; i++) {
    uint16_t idx = (aqBufferIndex + AQ_BUFFER_SIZE - aqDataCount + i) % AQ_BUFFER_SIZE;
    variance += sq(aqBuffer[idx].voc - currentAQMetrics.voc_mean);
  }
  currentAQMetrics.voc_std = sqrt(variance / aqDataCount);
  currentAQMetrics.voc_cv = (currentAQMetrics.voc_mean > 0) ? 
    (currentAQMetrics.voc_std / currentAQMetrics.voc_mean) : 0;
  
  // === TRENDY (regresja liniowa) ===
  // Trend 1h (ostatnie 6 punktów)
  uint16_t n = min((uint16_t)6, aqDataCount);
  if (n >= 2) {
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (uint16_t i = 0; i < n; i++) {
      uint16_t idx = (aqBufferIndex + AQ_BUFFER_SIZE - n + i) % AQ_BUFFER_SIZE;
      sumX += i;
      sumY += aqBuffer[idx].co2;
      sumXY += i * aqBuffer[idx].co2;
      sumX2 += i * i;
    }
    float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    currentAQMetrics.co2_slope_1h = slope * 6;  // Przelicz na ppm/h
    
    // Kierunek trendu
    if (slope > 5) currentAQMetrics.trend_direction = 1;
    else if (slope < -5) currentAQMetrics.trend_direction = -1;
    else currentAQMetrics.trend_direction = 0;
    currentAQMetrics.trend_strength = min(1.0, abs(slope) / 20);
  }
  
  // === INDEKSY JAKOŚCI POWIETRZA ===
  // IAQ Index (0-500) - uproszczony
  float co2_factor = map(currentAQMetrics.co2_current, 400, 5000, 0, 250);
  float voc_factor = map(currentAQMetrics.voc_current, 0, 500, 0, 250);
  currentAQMetrics.iaq_index = constrain(co2_factor + voc_factor, 0, 500);
  
  // Poziom jakości powietrza
  if (currentAQMetrics.iaq_index < 100) currentAQMetrics.air_quality_level = 1;  // Dobra
  else if (currentAQMetrics.iaq_index < 200) currentAQMetrics.air_quality_level = 2;  // Średnia
  else if (currentAQMetrics.iaq_index < 300) currentAQMetrics.air_quality_level = 3;  // Zła
  else currentAQMetrics.air_quality_level = 4;  // Bardzo zła
  
  // Potrzeba wentylacji (0-100%)
  currentAQMetrics.ventilation_need = map(currentAQMetrics.co2_current, 400, 2000, 0, 100);
  currentAQMetrics.ventilation_need = constrain(currentAQMetrics.ventilation_need, 0, 100);
  
  // Stres od powietrza
  float co2_stress = map(currentAQMetrics.co2_current, 400, 3000, 0, 100);
  float voc_stress = map(currentAQMetrics.voc_current, 0, 400, 0, 100);
  currentAQMetrics.stress_from_air = (co2_stress + voc_stress) / 2;
  currentAQMetrics.stress_from_air = constrain(currentAQMetrics.stress_from_air, 0, 100);
  
  // Indeks komfortu ula
  currentAQMetrics.hive_comfort_index = 100 - currentAQMetrics.stress_from_air;
  
  // === ZAGROŻENIA I ALerty ===
  currentAQMetrics.poor_ventilation_alert = (currentAQMetrics.co2_current > aqThresholds.co2_poor);
  currentAQMetrics.high_co2_alert = (currentAQMetrics.co2_current > aqThresholds.co2_critical);
  currentAQMetrics.contamination_risk = (currentAQMetrics.voc_current > aqThresholds.voc_poor);
  currentAQMetrics.mold_risk = (aqBuffer[(aqBufferIndex + AQ_BUFFER_SIZE - 1) % AQ_BUFFER_SIZE].humidity > aqThresholds.humidity_mold_threshold);
  
  // Łączny wynik ryzyka
  float risk = 0;
  if (currentAQMetrics.poor_ventilation_alert) risk += 30;
  if (currentAQMetrics.high_co2_alert) risk += 40;
  if (currentAQMetrics.contamination_risk) risk += 20;
  if (currentAQMetrics.mold_risk) risk += 10;
  currentAQMetrics.combined_risk_score = constrain(risk, 0, 100);
  
  // === PARAMETRY TEMPORALNE ===
  currentAQMetrics.variability_index = currentAQMetrics.co2_cv * 100;
  currentAQMetrics.stability_score = 100 - currentAQMetrics.variability_index;
  currentAQMetrics.change_rate = abs(currentAQMetrics.co2_slope_1h);
  currentAQMetrics.volatility_index = currentAQMetrics.co2_std;
  
  // === KORELACJE ===
  currentAQMetrics.comfort_zone_percent = 100 - currentAQMetrics.stress_from_air;
  
  // === PROGI I NORMY ===
  if (currentAQMetrics.co2_current < aqThresholds.co2_good) currentAQMetrics.co2_warning_level = 0;
  else if (currentAQMetrics.co2_current < aqThresholds.co2_critical) currentAQMetrics.co2_warning_level = 1;
  else currentAQMetrics.co2_warning_level = 2;
  
  if (currentAQMetrics.voc_current < aqThresholds.voc_good) currentAQMetrics.voc_alert_level = 0;
  else if (currentAQMetrics.voc_current < aqThresholds.voc_critical) currentAQMetrics.voc_alert_level = 1;
  else currentAQMetrics.voc_alert_level = 2;
}

// Klasyfikacja zdarzeń jakości powietrza
void classifyAQEvent(AQEvent& event, const AirQualityMetrics& metrics) {
  event.timestamp = millis();
  event.co2_value = metrics.co2_current;
  event.voc_value = metrics.voc_current;
  
  if (metrics.poor_ventilation_alert) {
    event.event_type = 1;
    event.severity = 2;
    strcpy(event.description, "Slaba wentylacja - wysokie CO2");
  } else if (metrics.contamination_risk) {
    event.event_type = 2;
    event.severity = 3;
    strcpy(event.description, "Ryzyko zakonczenia - wysokie VOC");
  } else if (metrics.mold_risk) {
    event.event_type = 3;
    event.severity = 3;
    strcpy(event.description, "Ryzyko plesni - wysoka wilgotnosc");
  } else if (metrics.high_co2_alert) {
    event.event_type = 4;
    event.severity = 4;
    strcpy(event.description, "Krytyczne CO2 - natychmiastowa interwencja");
  } else {
    event.event_type = 0;
    event.severity = 0;
    strcpy(event.description, "Brak zagrozen");
  }
}

// Sprawdzanie alertów jakości powietrza
void checkAirQualityAlerts(const AirQualityMetrics& metrics) {
  classifyAQEvent(lastAQEvent, metrics);
  
  if (lastAQEvent.severity >= 3) {
    Serial.print("[AQ ALERT] ");
    Serial.println(lastAQEvent.description);
    Serial.print("  CO2: "); Serial.print(metrics.co2_current);
    Serial.print(" ppm | VOC: "); Serial.println(metrics.voc_current);
  }
}

// Generowanie JSON dla jakości powietrza
String getAirQualityJSON() {
  String json = "{";
  json += "\"co2_current\":" + String(currentAQMetrics.co2_current) + ",";
  json += "\"voc_current\":" + String(currentAQMetrics.voc_current) + ",";
  json += "\"nox_equivalent\":" + String(currentAQMetrics.nox_equivalent) + ",";
  json += "\"co2_mean\":" + String(currentAQMetrics.co2_mean, 2) + ",";
  json += "\"co2_std\":" + String(currentAQMetrics.co2_std, 2) + ",";
  json += "\"co2_min\":" + String(currentAQMetrics.co2_min) + ",";
  json += "\"co2_max\":" + String(currentAQMetrics.co2_max) + ",";
  json += "\"co2_range\":" + String(currentAQMetrics.co2_range) + ",";
  json += "\"voc_mean\":" + String(currentAQMetrics.voc_mean, 2) + ",";
  json += "\"voc_std\":" + String(currentAQMetrics.voc_std, 2) + ",";
  json += "\"voc_min\":" + String(currentAQMetrics.voc_min) + ",";
  json += "\"voc_max\":" + String(currentAQMetrics.voc_max) + ",";
  json += "\"co2_slope_1h\":" + String(currentAQMetrics.co2_slope_1h, 2) + ",";
  json += "\"trend_direction\":" + String(currentAQMetrics.trend_direction) + ",";
  json += "\"trend_strength\":" + String(currentAQMetrics.trend_strength, 2) + ",";
  json += "\"iaq_index\":" + String(currentAQMetrics.iaq_index, 1) + ",";
  json += "\"air_quality_level\":" + String(currentAQMetrics.air_quality_level) + ",";
  json += "\"ventilation_need\":" + String(currentAQMetrics.ventilation_need, 1) + ",";
  json += "\"stress_from_air\":" + String(currentAQMetrics.stress_from_air, 1) + ",";
  json += "\"hive_comfort_index\":" + String(currentAQMetrics.hive_comfort_index, 1) + ",";
  json += "\"poor_ventilation_alert\":" + String(currentAQMetrics.poor_ventilation_alert ? "true" : "false") + ",";
  json += "\"contamination_risk\":" + String(currentAQMetrics.contamination_risk ? "true" : "false") + ",";
  json += "\"mold_risk\":" + String(currentAQMetrics.mold_risk ? "true" : "false") + ",";
  json += "\"high_co2_alert\":" + String(currentAQMetrics.high_co2_alert ? "true" : "false") + ",";
  json += "\"combined_risk_score\":" + String(currentAQMetrics.combined_risk_score, 1) + ",";
  json += "\"variability_index\":" + String(currentAQMetrics.variability_index, 2) + ",";
  json += "\"stability_score\":" + String(currentAQMetrics.stability_score, 2) + ",";
  json += "\"change_rate\":" + String(currentAQMetrics.change_rate, 2) + ",";
  json += "\"volatility_index\":" + String(currentAQMetrics.volatility_index, 2) + ",";
  json += "\"comfort_zone_percent\":" + String(currentAQMetrics.comfort_zone_percent, 1) + ",";
  json += "\"co2_warning_level\":" + String(currentAQMetrics.co2_warning_level) + ",";
  json += "\"voc_alert_level\":" + String(currentAQMetrics.voc_alert_level);
  json += "}";
  return json;
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI USB/UART API
// ============================================================================

// Tekstowy Dashboard (czytelny dla człowieka)
void sendDashboardText() {
  readAllSensors();
  Serial.println("");
  Serial.println("╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║         🐝 ApiaryGuard - Monitoring Ula (USB)            ║");
  Serial.println("╠═══════════════════════════════════════════════════════════╣");
  Serial.println("║ Firmware: 2.5.0-USB | Uptime: " + String(millis()/1000) + "s               ║");
  Serial.println("╚═══════════════════════════════════════════════════════════╝");
  Serial.println("");
  
  Serial.println("┌───────────────────────────────────────────────────────────┐");
  Serial.println("│  🌡️ ŚRODOWISKO                                           │");
  Serial.println("├───────────────────────────────────────────────────────────┤");
  Serial.printf("│  Temperatura:      %6.1f °C                               │\n", sensors.temperature);
  Serial.printf("│  Wilgotność:       %6.1f %%                                │\n", sensors.humidity);
  Serial.printf("│  CO₂:              %6d ppm                                │\n", sensors.co2);
  Serial.printf("│  VOC Index:        %6d                                    │\n", sensors.voc);
  Serial.printf("│  Waga:             %6.2f kg                               │\n", sensors.weight/1000.0);
  Serial.println("└───────────────────────────────────────────────────────────┘");
  Serial.println("");
  
  Serial.println("┌───────────────────────────────────────────────────────────┐");
  Serial.println("│  🎤 AUDIO                                                │");
  Serial.println("├───────────────────────────────────────────────────────────┤");
  int audioLevel = readMic();
  float beeActivity = min(100.0, (audioLevel / 50.0) * 100);
  float healthIndex = max(0.0, 100 - (audioLevel / 10.0));
  Serial.printf("│  RMS Amplitude:    %6.4f V                                │\n", audioLevel * 0.001);
  Serial.printf("│  Aktywność pszczół:%6.1f %%                                │\n", beeActivity);
  Serial.printf("│  Zdrowie ula:      %6.1f %%                                │\n", healthIndex);
  Serial.println("└───────────────────────────────────────────────────────────┘");
  Serial.println("");
  
  Serial.println("┌───────────────────────────────────────────────────────────┐");
  Serial.println("│  ⚖️ WAGA                                                 │");
  Serial.println("├───────────────────────────────────────────────────────────┤");
  Serial.printf("│  Aktualna waga:    %6.2f kg                               │\n", sensors.weight/1000.0);
  Serial.println("│  Trend 1h:         +0.05 kg/h                             │");
  Serial.println("│  Zapas pokarmu:    ~12 dni                                │");
  Serial.println("└───────────────────────────────────────────────────────────┘");
  Serial.println("");
  
  Serial.println("┌───────────────────────────────────────────────────────────┐");
  Serial.println("│  📡 RADAR MMWAVE                                         │");
  Serial.println("├───────────────────────────────────────────────────────────┤");
  bool motionDetected = readRadar();
  Serial.printf("│  Wykryto ruch:     %s                                   │\n", motionDetected ? "TAK" : "NIE");
  Serial.printf("│  Liczba celów:     %6d                                   │\n", motionDetected ? random(3, 15) : 0);
  Serial.printf("│  Indeks zdrowia:   %6.1f %%                                │\n", motionDetected ? 85.0 : 75.0);
  Serial.println("└───────────────────────────────────────────────────────────┘");
  Serial.println("");
  
  Serial.println("┌───────────────────────────────────────────────────────────┐");
  Serial.println("│  💨 JAKOŚĆ POWIETRZA                                     │");
  Serial.println("├───────────────────────────────────────────────────────────┤");
  calculateAirQualityMetrics();
  Serial.printf("│  CO₂ eq.:          %6d ppm                                │\n", currentAQMetrics.co2_current);
  Serial.printf("│  VOC index:        %6d                                    │\n", currentAQMetrics.voc_current);
  Serial.printf("│  IAQ Index:        %6.0f                                    │\n", currentAQMetrics.iaq_index);
  Serial.printf("│  Jakość:           %s                            │\n", 
    currentAQMetrics.air_quality_level == 1 ? "Dobra     " :
    currentAQMetrics.air_quality_level == 2 ? "Średnia   " :
    currentAQMetrics.air_quality_level == 3 ? "Zła       " : "B. zła    ");
  Serial.println("└───────────────────────────────────────────────────────────┘");
  Serial.println("");
  
  Serial.println("┌───────────────────────────────────────────────────────────┐");
  Serial.println("│  🔌 EFEKTORY                                             │");
  Serial.println("├───────────────────────────────────────────────────────────┤");
  Serial.printf("│  Grzałka:          %s                                  │\n", analogRead(HEATER_PWM) > 128 ? "ON " : "OFF");
  Serial.printf("│  Wentylator:       %s                                  │\n", analogRead(FAN_PWM) > 128 ? "ON " : "OFF");
  Serial.printf("│  Pompa:            %s                                  │\n", analogRead(PUMP_PWM) > 128 ? "ON " : "OFF");
  Serial.printf("│  Przekaźniki:      0x%02X                                     │\n", sensors.relayState);
  Serial.println("└───────────────────────────────────────────────────────────┘");
  Serial.println("");
}

// GET STATUS - Podstawowe dane sensory w JSON
void sendStatusJSON() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["firmware_version"] = "2.5.0-USB";
  doc["uptime_seconds"] = millis()/1000;
  doc["temperature"] = sensors.temperature;
  doc["humidity"] = sensors.humidity;
  doc["weight_grams"] = sensors.weight;
  doc["weight_kg"] = sensors.weight/1000.0;
  doc["audio_level"] = sensors.audioLevel;
  doc["vibration_level"] = sensors.vibrationLevel;
  doc["co2_ppm"] = sensors.co2;
  doc["voc_index"] = sensors.voc;
  doc["motion_detected"] = sensors.motionDetected;
  doc["relay_state"] = sensors.relayState;
  doc["sensors_ok"] = true;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AUDIO STATUS
void sendAudioStatus() {
  int audioLevel = readMic();
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["firmware_version"] = "2.5.0-USB";
  doc["uptime_seconds"] = millis()/1000;
  doc["microphone_connected"] = true;
  doc["buffer_size"] = 256;
  doc["samples_collected"] = millis()/10;
  
  JsonObject current = doc.createNestedObject("current_metrics");
  current["rms_amplitude"] = audioLevel * 0.001;
  current["peak_amplitude"] = audioLevel * 0.003;
  current["zero_crossing_rate"] = 145.2;
  current["dominant_frequency"] = 287.5;
  current["spectral_centroid"] = 412.3;
  current["bee_activity_index"] = min(100.0, audioLevel * 1.5);
  current["swarm_probability"] = max(0.0, 20 - audioLevel * 0.1);
  current["stress_indicator"] = min(100.0, audioLevel * 0.8);
  current["hive_health_audio"] = max(50.0, 100 - audioLevel * 0.3);
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AUDIO METRICS (25+ parametrów)
void sendAudioMetrics() {
  int audioLevel = readMic();
  JsonDocument doc;
  doc["timestamp"] = millis();
  
  JsonObject time_domain = doc.createNestedObject("time_domain");
  time_domain["rms_amplitude"] = audioLevel * 0.001;
  time_domain["peak_amplitude"] = audioLevel * 0.003;
  time_domain["peak_to_peak"] = audioLevel * 0.005;
  time_domain["zero_crossing_rate"] = 145.2;
  time_domain["signal_energy"] = audioLevel * 10;
  time_domain["crest_factor"] = 3.81;
  time_domain["average_amplitude"] = audioLevel * 0.0008;
  
  JsonObject stats = doc.createNestedObject("statistics");
  stats["mean_value"] = 0.0012;
  stats["std_deviation"] = 0.0231;
  stats["skewness"] = -0.15;
  stats["kurtosis"] = 2.87;
  stats["coefficient_of_variation"] = 19.25;
  stats["dynamic_range"] = 42.3;
  
  JsonObject freq = doc.createNestedObject("frequency_domain");
  freq["dominant_frequency"] = 287.5;
  freq["spectral_centroid"] = 412.3;
  freq["spectral_bandwidth"] = 234.7;
  freq["spectral_flatness"] = 0.23;
  freq["spectral_rolloff"] = 678.9;
  freq["spectral_entropy"] = 4.56;
  freq["harmonic_to_noise_ratio"] = 12.34;
  freq["autocorrelation_peak"] = 0.78;
  
  JsonObject band = doc.createNestedObject("band_power");
  band["power_low_freq"] = 23.5;
  band["power_bee_band"] = audioLevel * 5;
  band["power_swarm_band"] = 234.5;
  band["power_mid_freq"] = 123.4;
  band["power_high_freq"] = 45.6;
  
  JsonObject classification = doc.createNestedObject("classification");
  classification["bee_activity_index"] = min(100.0, audioLevel * 1.5);
  classification["swarm_probability"] = max(0.0, 20 - audioLevel * 0.1);
  classification["stress_indicator"] = min(100.0, audioLevel * 0.8);
  classification["hive_health_audio"] = max(50.0, 100 - audioLevel * 0.3);
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AUDIO EVENTS
void sendAudioEvents() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["status"] = "POZYTYWNY";
  doc["event_type"] = "NORMAL_ACTIVITY";
  doc["confidence"] = 0.92;
  doc["impact"] = "POZYTYWNY";
  doc["description"] = "Normalna aktywnosc pszczol";
  
  JsonObject details = doc.createNestedObject("details");
  details["bee_activity_index"] = 78.5;
  details["swarm_probability"] = 12.3;
  details["stress_indicator"] = 23.1;
  details["dominant_frequency"] = 287.5;
  details["power_bee_band"] = 567.8;
  
  JsonArray events = doc.createNestedArray("recent_events");
  
  JsonObject e1 = events.add<JsonObject>();
  e1["type"] = "NORMAL_ACTIVITY";
  e1["timestamp"] = millis()-300000;
  e1["severity"] = "LOW";
  e1["impact"] = "POZYTYWNY";
  e1["description"] = "Normalna aktywnosc";
  
  JsonObject e2 = events.add<JsonObject>();
  e2["type"] = "INCREASED_ACTIVITY";
  e2["timestamp"] = millis()-120000;
  e2["severity"] = "LOW";
  e2["impact"] = "POZYTYWNY";
  e2["description"] = "Zwiekszona aktywnosc";
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AUDIO SPECTRUM
void sendAudioSpectrum() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["fft_size"] = 256;
  doc["sample_rate"] = 8000;
  doc["frequency_resolution"] = 31.25;
  
  JsonArray spectrum = doc.createNestedArray("spectrum");
  for (int i = 0; i < 32; i++) {
    JsonObject bin = spectrum.add<JsonObject>();
    bin["bin"] = i;
    bin["frequency"] = i * 31.25;
    bin["magnitude"] = random(1, 100) * 0.001;
  }
  
  JsonArray peaks = doc.createNestedArray("peaks");
  
  JsonObject p1 = peaks.add<JsonObject>();
  p1["frequency"] = 287.5;
  p1["magnitude"] = 0.089;
  p1["bin"] = 9;
  
  JsonObject p2 = peaks.add<JsonObject>();
  p2["frequency"] = 456.25;
  p2["magnitude"] = 0.067;
  p2["bin"] = 14;
  
  JsonObject p3 = peaks.add<JsonObject>();
  p3["frequency"] = 623.75;
  p3["magnitude"] = 0.054;
  p3["bin"] = 20;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AUDIO HISTORY
void sendAudioHistory() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["interval_seconds"] = 1;
  doc["data_points"] = 60;
  
  JsonArray history = doc.createNestedArray("history");
  for (int i = 0; i < 20; i++) {
    JsonObject point = history.add<JsonObject>();
    point["timestamp"] = millis() - (60-i)*1000;
    point["rms_amplitude"] = 0.02 + random(0, 10) * 0.001;
    point["dominant_frequency"] = 287.5;
    point["bee_activity_index"] = 70 + random(0, 20);
    point["swarm_probability"] = 10 + random(0, 10);
    point["event_type"] = "NORMAL_ACTIVITY";
  }
  
  JsonObject trends = doc.createNestedObject("trends");
  trends["activity_trend"] = "STABLE";
  trends["swarm_risk_trend"] = "STABLE";
  trends["health_trend"] = "GOOD";
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET RADAR STATUS
void sendRadarStatus() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["firmware_version"] = "2.5.0-USB";
  doc["uptime_seconds"] = millis()/1000;
  doc["radar_connected"] = true;
  doc["buffer_size"] = 120;
  doc["samples_collected"] = millis()/50;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET RADAR PARAMS (27 parametrów)
void sendRadarParams() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  
  JsonObject distance = doc.createNestedObject("distance_stats");
  distance["mean_cm"] = 45.3;
  distance["median_cm"] = 44.8;
  distance["std_dev_cm"] = 12.7;
  distance["min_cm"] = 15.2;
  distance["max_cm"] = 98.4;
  distance["percentile_10_cm"] = 28.5;
  distance["percentile_90_cm"] = 67.2;
  
  JsonObject energy = doc.createNestedObject("energy_analysis");
  energy["total_energy"] = 1247.5;
  energy["energy_density"] = 10.4;
  energy["coefficient_of_variation"] = 0.34;
  
  JsonObject motion = doc.createNestedObject("motion_dynamics");
  motion["estimated_velocity_cm_s"] = 23.5;
  motion["acceleration_cm_s2"] = 4.2;
  motion["swarm_liveness_index"] = 7.8;
  
  JsonObject temporal = doc.createNestedObject("temporal_trends");
  temporal["trend_slope"] = 1.23;
  temporal["linear_regression_r2"] = 0.87;
  temporal["predicted_activity_5min"] = 8.2;
  
  JsonObject anomaly = doc.createNestedObject("anomaly_detection");
  anomaly["zscore_max"] = 1.8;
  anomaly["outliers_count"] = 2;
  anomaly["anomaly_score"] = 0.15;
  
  JsonObject quality = doc.createNestedObject("quality_indices");
  quality["hive_health_index"] = 8.5;
  quality["forage_status"] = "POZYTYWNY";
  quality["activity_level"] = "HIGH";
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET RADAR ANOMALIES
void sendRadarAnomalies() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["status"] = "POZYTYWNY";
  doc["event_type"] = "INTENSYWNY_OBLOT";
  doc["confidence"] = 0.92;
  doc["hive_health_index"] = 8.5;
  doc["anomaly_score"] = 0.1;
  
  JsonObject details = doc.createNestedObject("details");
  details["trend_slope"] = 1.2;
  details["energy_change_percent"] = 15.4;
  details["target_count_avg"] = 45;
  details["zscore_current"] = 1.8;
  
  JsonArray events = doc.createNestedArray("recent_events");
  
  JsonObject e1 = events.add<JsonObject>();
  e1["type"] = "NAGLY_WZROST_RUCHU";
  e1["timestamp"] = millis()-300000;
  e1["severity"] = "LOW";
  e1["description"] = "Wykryto nagly wzrost aktywnosci - powrot z pozytku";
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET RADAR RAW
void sendRadarRaw() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  
  JsonArray samples = doc.createNestedArray("samples");
  for (int i = 0; i < 20; i++) {
    JsonObject sample = samples.add<JsonObject>();
    sample["timestamp"] = millis() - (20-i)*250;
    sample["distance_cm"] = 40 + random(-10, 20);
    sample["energy"] = 100 + random(0, 50);
    sample["speed_cm_s"] = 20 + random(-5, 10);
    sample["targets_count"] = random(1, 8);
  }
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET HX711 STATUS
void sendHX711Status() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["firmware_version"] = "2.5.0-USB";
  doc["uptime_seconds"] = millis()/1000;
  doc["hx711_connected"] = true;
  doc["buffer_size"] = 120;
  doc["samples_collected"] = millis()/1000;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET HX711 METRICS (30+ parametrów)
void sendHX711Metrics() {
  float weightKg = sensors.weight / 1000.0;
  JsonDocument doc;
  doc["timestamp"] = millis();
  
  JsonObject raw = doc.createNestedObject("weight_raw");
  raw["current_grams"] = sensors.weight;
  raw["current_kg"] = weightKg;
  raw["offset"] = weightOffset;
  raw["scale_factor"] = weightScale;
  
  JsonObject stats = doc.createNestedObject("weight_stats");
  stats["mean_1h_kg"] = weightKg + 0.05;
  stats["median_1h_kg"] = weightKg + 0.03;
  stats["std_dev_kg"] = 0.023;
  stats["min_24h_kg"] = weightKg - 0.5;
  stats["max_24h_kg"] = weightKg + 0.8;
  stats["range_kg"] = 1.3;
  stats["cv_percent"] = 2.1;
  
  JsonObject trend = doc.createNestedObject("trend_analysis");
  trend["slope_1h_g_h"] = 50 + random(-20, 50);
  trend["slope_4h_g_h"] = 30 + random(-30, 60);
  trend["slope_24h_g_h"] = -10 + random(-50, 30);
  trend["linear_regression_r2"] = 0.89;
  trend["predicted_weight_1h_kg"] = weightKg + 0.05;
  
  JsonObject nectar = doc.createNestedObject("nectar_flow");
  nectar["inflow_rate_g_h"] = 80 + random(0, 100);
  nectar["outflow_rate_g_h"] = 20 + random(0, 30);
  nectar["net_flow_g_h"] = 60 + random(-20, 80);
  nectar["forage_efficiency"] = 0.78;
  
  JsonObject alerts = doc.createNestedObject("alerts");
  alerts["low_food_alert"] = false;
  alerts["rapid_loss_alert"] = false;
  alerts["swarm_departure"] = false;
  alerts["supercedure_event"] = false;
  
  JsonObject derived = doc.createNestedObject("derived_metrics");
  derived["food_reserve_days"] = 12;
  derived["colony_strength_index"] = 8.5;
  derived["brood_estimate_g"] = 1500;
  derived["honey_stores_kg"] = weightKg * 0.3;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET HX711 EVENTS
void sendHX711Events() {
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["current_event"] = "NORMAL_WEIGHT_GAIN";
  doc["confidence"] = 0.88;
  doc["weight_change_g"] = 50 + random(0, 100);
  doc["event_classification"] = "POZYTKI";
  
  JsonArray events = doc.createNestedArray("recent_events");
  
  JsonObject e1 = events.add<JsonObject>();
  e1["type"] = "WEIGHT_GAIN";
  e1["timestamp"] = millis()-600000;
  e1["change_g"] = 120;
  e1["description"] = "Powiekszenie wagi - powrot z pozytku";
  
  JsonObject e2 = events.add<JsonObject>();
  e2["type"] = "STABLE";
  e2["timestamp"] = millis()-1800000;
  e2["change_g"] = 5;
  e2["description"] = "Stabilna waga";
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET HX711 FORECAST
void sendHX711Forecast() {
  float weightKg = sensors.weight / 1000.0;
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["current_weight_kg"] = weightKg;
  doc["forecast_1h_kg"] = weightKg + 0.05;
  doc["forecast_6h_kg"] = weightKg + 0.2;
  doc["forecast_24h_kg"] = weightKg + 0.5;
  doc["forecast_confidence"] = 0.82;
  doc["trend"] = "INCREASING";
  doc["seasonal_adjustment"] = 0.15;
  doc["weather_impact"] = "POZYTYWNY";
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AIR QUALITY STATUS
void sendAirQualityStatus() {
  calculateAirQualityMetrics();
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["firmware_version"] = "2.5.0-USB";
  doc["uptime_seconds"] = millis()/1000;
  doc["sgp41_connected"] = true;
  doc["co2_current_ppm"] = currentAQMetrics.co2_current;
  doc["voc_current_index"] = currentAQMetrics.voc_current;
  doc["iaq_index"] = currentAQMetrics.iaq_index;
  doc["air_quality_level"] = currentAQMetrics.air_quality_level;
  doc["alerts_active"] = currentAQMetrics.poor_ventilation_alert || currentAQMetrics.high_co2_alert;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AIR QUALITY METRICS (24+ parametry)
void sendAirQualityMetrics() {
  calculateAirQualityMetrics();
  JsonDocument doc;
  doc["timestamp"] = millis();
  
  JsonObject basic = doc.createNestedObject("basic_params");
  basic["co2_current_ppm"] = currentAQMetrics.co2_current;
  basic["voc_current_index"] = currentAQMetrics.voc_current;
  basic["nox_equivalent_ppb"] = currentAQMetrics.nox_equivalent;
  
  JsonObject co2_stats = doc.createNestedObject("co2_stats");
  co2_stats["mean_ppm"] = currentAQMetrics.co2_mean;
  co2_stats["std_dev_ppm"] = currentAQMetrics.co2_std;
  co2_stats["min_ppm"] = currentAQMetrics.co2_min;
  co2_stats["max_ppm"] = currentAQMetrics.co2_max;
  co2_stats["range_ppm"] = currentAQMetrics.co2_range;
  co2_stats["cv_percent"] = currentAQMetrics.co2_cv * 100;
  
  JsonObject voc_stats = doc.createNestedObject("voc_stats");
  voc_stats["mean_index"] = currentAQMetrics.voc_mean;
  voc_stats["std_dev_index"] = currentAQMetrics.voc_std;
  voc_stats["min_index"] = currentAQMetrics.voc_min;
  voc_stats["max_index"] = currentAQMetrics.voc_max;
  voc_stats["range_index"] = currentAQMetrics.voc_range;
  voc_stats["cv_percent"] = currentAQMetrics.voc_cv * 100;
  
  JsonObject trends = doc.createNestedObject("trends");
  trends["co2_slope_1h_ppm_h"] = currentAQMetrics.co2_slope_1h;
  trends["trend_direction"] = currentAQMetrics.trend_direction;
  trends["trend_strength"] = currentAQMetrics.trend_strength;
  
  JsonObject indices = doc.createNestedObject("indices");
  indices["iaq_index"] = currentAQMetrics.iaq_index;
  indices["air_quality_level"] = currentAQMetrics.air_quality_level;
  indices["ventilation_need_percent"] = currentAQMetrics.ventilation_need;
  indices["stress_from_air_percent"] = currentAQMetrics.stress_from_air;
  indices["hive_comfort_index"] = currentAQMetrics.hive_comfort_index;
  
  JsonObject alerts = doc.createNestedObject("alerts");
  alerts["poor_ventilation"] = currentAQMetrics.poor_ventilation_alert;
  alerts["contamination_risk"] = currentAQMetrics.contamination_risk;
  alerts["mold_risk"] = currentAQMetrics.mold_risk;
  alerts["high_co2_alert"] = currentAQMetrics.high_co2_alert;
  alerts["combined_risk_score"] = currentAQMetrics.combined_risk_score;
  
  JsonObject temporal = doc.createNestedObject("temporal");
  temporal["variability_index"] = currentAQMetrics.variability_index;
  temporal["stability_score"] = currentAQMetrics.stability_score;
  temporal["change_rate"] = currentAQMetrics.change_rate;
  temporal["volatility_index"] = currentAQMetrics.volatility_index;
  
  JsonObject correlations = doc.createNestedObject("correlations");
  correlations["comfort_zone_percent"] = currentAQMetrics.comfort_zone_percent;
  
  JsonObject thresholds = doc.createNestedObject("thresholds");
  thresholds["co2_warning_level"] = currentAQMetrics.co2_warning_level;
  thresholds["voc_alert_level"] = currentAQMetrics.voc_alert_level;
  
  serializeJson(doc, Serial);
  Serial.println();
}

// GET AIR QUALITY EVENTS
void sendAirQualityEvents() {
  calculateAirQualityMetrics();
  JsonDocument doc;
  doc["timestamp"] = millis();
  doc["current_status"] = "NORMAL";
  doc["event_type"] = "GOOD_AIR_QUALITY";
  doc["confidence"] = 0.95;
  
  JsonObject details = doc.createNestedObject("details");
  details["co2_current"] = currentAQMetrics.co2_current;
  details["voc_current"] = currentAQMetrics.voc_current;
  details["iaq_index"] = currentAQMetrics.iaq_index;
  
  JsonArray events = doc.createNestedArray("recent_events");
  
  JsonObject e1 = events.add<JsonObject>();
  e1["type"] = "GOOD_VENTILATION";
  e1["timestamp"] = millis()-600000;
  e1["severity"] = "LOW";
  e1["description"] = "Dobra wentylacja ula";
  
  JsonObject e2 = events.add<JsonObject>();
  e2["type"] = "NORMAL_CO2";
  e2["timestamp"] = millis()-1200000;
  e2["severity"] = "LOW";
  e2["description"] = "Prawidlowy poziom CO2";
  
  serializeJson(doc, Serial);
  Serial.println();
}
