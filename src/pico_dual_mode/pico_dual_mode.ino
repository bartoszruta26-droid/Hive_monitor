/*
 * Raspberry Pi Pico - Dual Mode Communication (USB Priority + ENC28J60)
 * 
 * Opis:
 * Mikrokontroler automatycznie wykrywa aktywne połączenie.
 * 1. Priorytet: USB (Serial). Jeśli wykryto komunikację hosta, nasłuchuje komend tekstowych.
 * 2. Fallback: Ethernet (ENC28J60). Jeśli brak aktywności na USB, uruchamia serwer HTTP.
 * 
 * Wersja poprawiona - pełna obsługa wszystkich sensorów jak w pico_refactored:
 * - DHT22 (temperatura/wilgotność)
 * - HX711 (waga) z buforowaniem i analizą trendów
 * - SGP41 (NOx/VOC) z metrykami jakości powietrza
 * - Mikrofon (ADC) z analizą FFT i detekcją aktywności pszczół
 * - LD2410B Radar (UART) z analizą ruchu
 * 
 * Wymagane biblioteki (Arduino IDE):
 * - UIPEthernet (dla modułu ENC28J60)
 * - DHT sensor library (Adafruit)
 * - HX711-Arduino (by Bogdan Necula)
 * - SparkFun SGP41 Arduino Library
 * 
 * Pinout (Konfiguracja):
 * --- Sensory ---
 * DHT22 (Temp/Hum) : GPIO 2 (Pin 4)
 * HX711 (Waga)     : DT -> GPIO 3 (Pin 5), SCK -> GPIO 4 (Pin 6)
 * SGP41 (NOx/VOC)  : I2C (SDA: GPIO 0/Pin 1, SCL: GPIO 1/Pin 2)
 * Mikrofon/Analog  : GPIO 26 (ADC0)
 * Radar LD2410B    : RX -> GPIO 4, TX -> GPIO 5 (UART1)
 * 
 * --- Aktuary ---
 * Grzałka (PWM)    : GPIO 5 (Pin 7)
 * Wentylator (PWM) : GPIO 6 (Pin 9)
 * Pompa (PWM)      : GPIO 7 (Pin 10)
 * Przekaźnik 1     : GPIO 8 (Pin 11)
 * Przekaźnik 2     : GPIO 9 (Pin 12)
 * Piezo/Buzzer     : GPIO 10 (Pin 14)
 * 
 * --- Ethernet (ENC28J60) ---
 * CS   : GPIO 17 (Pin 22)
 * MOSI : GPIO 19 (Pin 25)
 * MISO : GPIO 16 (Pin 21)
 * SCK  : GPIO 18 (Pin 24)
 */

#include <SPI.h>
#include <UIPEthernet.h>
#include <DHT.h>
#include <HX711.h>
#include <Wire.h>
#include <SparkFun_SGP41_Arduino_Library.h>
#include <HardwareSerial.h>
#include <math.h>

// ================= KONFIGURACJA PINÓW =================
#define PIN_DHT 2
#define PIN_HX711_DT 3
#define PIN_HX711_SCK 4
#define PIN_MIC 26

#define PIN_HEATER 5
#define PIN_FAN 6
#define PIN_PUMP 7
#define PIN_RELAY1 8
#define PIN_RELAY2 9
#define PIN_BUZZER 10

#define PIN_ETH_CS 17

// Radar LD2410B (UART1)
#define RADAR_RX 4
#define RADAR_TX 5

// ================= KONFIGURACJA SIECIOWA =================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ================= STAŁE SYSTEMOWE =================
#define AUDIO_BUFFER_SIZE 256
#define AUDIO_SAMPLE_RATE 4000
#define BEE_FREQ_MIN 80
#define BEE_FREQ_MAX 800
#define SWARM_FREQ_MIN 150
#define SWARM_FREQ_MAX 350
#define HX711_BUFFER_SIZE 288
#define HX711_SHORT_WINDOW 6
#define AIRQUAL_BUFFER_SIZE 144
#define RADAR_BUFFER_SIZE 120
#define RADAR_TREND_WINDOW 20
#define CO2_WARNING_LEVEL 1000
#define CO2_ALERT_LEVEL 1500
#define VOC_ALERT_LEVEL 250
#define HX711_WEIGHT_CHANGE_THRESH 0.05f
#define HX711_NECTAR_FLOW_MIN 0.02f
#define HX711_CONSUMPTION_MIN 0.01f

// ================= STRUKTURY DANYCH =================
struct SensorFlags {
    bool detected = false;
    bool active = false;
    uint8_t error_count = 0;
};

struct SensorState {
    SensorFlags tempHum;
    SensorFlags airQual;
    SensorFlags hx711;
    SensorFlags radar;
    SensorFlags audio;
};

struct AudioMetrics {
    float zero_crossing_rate = 0.0f;
    float rms_amplitude = 0.0f;
    float peak_amplitude = 0.0f;
    float dominant_frequency = 0.0f;
    float spectral_centroid = 0.0f;
    float power_in_bee_band = 0.0f;
    float power_in_swarm_band = 0.0f;
    float bee_activity_index = 0.0f;
    float swarm_probability = 0.0f;
    float hive_health_audio = 0.0f;
};

struct HX711DataPoint {
    unsigned long timestamp;
    float weight_raw;
    float weight_filtered;
    bool is_valid = false;
};

struct HX711Metrics {
    float mean_weight = 0.0f;
    float std_weight = 0.0f;
    float min_weight = 0.0f;
    float max_weight = 0.0f;
    float current_rate = 0.0f;
    float trend_slope_1h = 0.0f;
    float trend_direction = 0.0f;
    float nectar_inflow_rate = 0.0f;
    float consumption_rate = 0.0f;
    float hive_health_weight = 50.0f;
    float anomaly_score = 0.0f;
};

struct AirQualityMetrics {
    uint16_t co2_eq = 0;
    uint16_t voc_idx = 0;
    float mean_co2 = 0.0f;
    float mean_voc = 0.0f;
    float trend_slope_1h = 0.0f;
    float trend_direction = 0.0f;
    float iaq_index = 100.0f;
    float ventilation_need = 0.0f;
    float stress_level = 0.0f;
};

struct RadarMetrics {
    float distance = 0.0f;
    float energy = 0.0f;
    float speed = 0.0f;
    float mean_distance = 0.0f;
    float activity_ratio = 0.0f;
    float motion_intensity = 0.0f;
    float trend_slope = 0.0f;
    float signal_quality = 100.0f;
    float anomaly_score = 0.0f;
    float hive_health_index = 50.0f;
};

// ================= OBIEKTY GLOBALNE =================
DHT dht(PIN_DHT, DHT22);
HX711 scale;
SGP41 sgp41;
HardwareSerial radarSerial(uart1);

EthernetServer server(80);
EthernetClient client;

// Stan sensorów
SensorState sensors;

// Metryki
AudioMetrics currentAudioMetrics;
HX711Metrics currentHX711Metrics;
AirQualityMetrics currentAirMetrics;
RadarMetrics currentRadarMetrics;

// Bufory
int16_t audioBuffer[AUDIO_BUFFER_SIZE];
float audioFFT[AUDIO_BUFFER_SIZE];
HX711DataPoint hx711Buffer[HX711_BUFFER_SIZE];
uint16_t co2History[AIRQUAL_BUFFER_SIZE];
uint16_t vocHistory[AIRQUAL_BUFFER_SIZE];
float distanceHistory[RADAR_BUFFER_SIZE];
float energyHistory[RADAR_BUFFER_SIZE];

// Zmienne stanu
bool usbActive = false;
bool ethActive = false;
unsigned long lastActivityTime = 0;
unsigned long lastSensorRead = 0;

// Dane z sensorów
float temperature = 0.0f;
float humidity = 0.0f;
float weight = 0.0f;
uint16_t nox = 0;
uint16_t voc = 0;
int micLevel = 0;

// Ustawienia aktuatorów
int heaterDuty = 0;
int fanDuty = 0;
int pumpDuty = 0;
bool relay1State = false;
bool relay2State = false;

// Kalibracja wagi
#define SCALE_CALIBRATION_FACTOR 11000.0f
#define SCALE_TARE_OFFSET 0
long hx711_offset = 0;
float hx711_scale = SCALE_CALIBRATION_FACTOR;
long hx711_value = 0;

// Indeksy historii
int hx711HistoryIdx = 0;
int airHistoryIdx = 0;
int radarHistoryIdx = 0;
bool historyInitialized = false;

// Debug i Error Handling
#define DEBUG_ENABLED true
#define ERROR_BUFFER_SIZE 128
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
  uint8_t radar_errors;
  uint8_t consecutive_failures;
} sensorErrors = {0, 0, 0, 0, 0, 0};

const uint8_t MAX_CONSECUTIVE_FAILURES = 5;  // Reset sensor after this many failures

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

void setup() {
  // Inicjalizacja Serial (USB) - zawsze startuje
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  Serial.println("\n=== ApiaryGuard Pico Dual Mode v2.1 ===");
  Serial.println(">> Pełna obsługa sensorów jak w pico_refactored");
  Serial.println(">> Debug & Error Handling enabled");

  // Konfiguracja Pinów
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  // PWM Setup
  analogWriteFreq(1000);
  analogWriteRange(255);

  // Inicjalizacja I2C dla SGP41
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  
  // Inicjalizacja UART dla radaru
  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
  debugPrint("UART1", "Zainicjalizowany dla radaru");

  // Detekcja sensorów
  Serial.println(">> Skanowanie sensorów...");
  detectAllSensors();
  printSensorStatus();
  
  // Inicjalizacja wykrytych sensorów
  initDetectedSensors();

  // Sprawdzenie trybu połączenia
  detectConnectionMode();
  
  // Podsumowanie inicjalizacji
  Serial.println("=======================");
  Serial.print("Free heap: ");
  Serial.println(rp2040.getFreeHeap());
  Serial.print("Error/Warning counters: ");
  Serial.print(errorCount);
  Serial.print("/");
  Serial.println(warningCount);
  Serial.println("");
}

void loop() {
  unsigned long now = millis();
  
  // 1. Obsługa czujników (co 1 sekundę)
  if (now - lastSensorRead > 1000) {
    readAllSensors(now);
    lastSensorRead = now;
  }

  // 2. Przetwarzanie audio okresowo (co 5 sekund)
  processAudioPeriodically(now);
  
  // 3. Przetwarzanie wagi okresowo (co 6 sekund)
  processWeightPeriodically(now);
  
  // 4. Przetwarzanie jakości powietrza (co 60 sekund)
  processAirQualityPeriodically(now);
  
  // 5. Przetwarzanie radaru (co 2 sekundy)
  processRadarPeriodically(now);

  // 6. Detekcja aktywności USB
  if (Serial.available()) {
    usbActive = true;
    lastActivityTime = now;
    handleUSBCommand();
  }

  // 7. Logika przełączania trybów
  if (usbActive && (now - lastActivityTime < 5000)) {
    // Tryb USB - priorytet na komunikację szeregową
  } else {
    // Brak aktywności USB -> Tryb Ethernet
    if (!ethActive) {
      Serial.println("Brak aktywności USB. Uruchamianie Ethernet...");
      initEthernet();
    }
    
    if (ethActive) {
      EthernetClient client = server.available();
      if (client) {
        handleEthernetClient(client);
      }
    }
  }

  // Aktualizacja wyjść PWM
  updateOutputs();
}

// ================= FUNKCJE DETEKCJI SENSORÓW =================

bool detectHX711() {
  pinMode(PIN_HX711_DT, INPUT);
  pinMode(PIN_HX711_SCK, OUTPUT);
  digitalWrite(PIN_HX711_SCK, LOW);
  
  unsigned long start = millis();
  while(digitalRead(PIN_HX711_DT)) {
    if (millis() - start > 50) {
      logWarning("HX711", "Nie wykryto (timeout)");
      sensorErrors.hx711_errors = MAX_CONSECUTIVE_FAILURES;
      return false;
    }
    delayMicroseconds(10);
  }
  
  debugPrint("HX711", "Wykryto");
  resetSensorError(sensorErrors.hx711_errors);
  return true;
}

bool detectDHT() {
  pinMode(PIN_DHT, INPUT_PULLUP);
  dht.begin();
  
  for (int i = 0; i < 3; i++) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (!isnan(h) && !isnan(t)) {
      Serial.printf("  [✓] DHT: Wykryto (T=%.1f°C, H=%.1f%%)\n", t, h);
      resetSensorError(sensorErrors.dht_errors);
      return true;
    }
    delay(100);
  }
  
  logWarning("DHT", "Nie wykryto");
  sensorErrors.dht_errors = MAX_CONSECUTIVE_FAILURES;
  return false;
}

bool detectSGP41() {
  Wire.beginTransmission(0x59);
  uint8_t error = Wire.endTransmission();
  
  if (error == 0) {
    debugPrint("SGP41", "Wykryto (0x59)");
    resetSensorError(sensorErrors.sgp41_errors);
    return true;
  }
  
  logWarning("SGP41", "Nie wykryto na I2C");
  sensorErrors.sgp41_errors = MAX_CONSECUTIVE_FAILURES;
  return false;
}

bool detectRadar() {
  radarSerial.flush();
  delay(100);
  
  if (radarSerial.available()) {
    debugPrint("Radar", "Dane na UART");
    resetSensorError(sensorErrors.radar_errors);
    return true;
  }
  
  debugPrint("Radar", "Brak odpowiedzi (może być w trybie idle)");
  return true; // Nie blokuj - radar może być w trybie idle
}

void detectAllSensors() {
  sensors.tempHum.detected = detectDHT();
  sensors.airQual.detected = detectSGP41();
  sensors.hx711.detected = detectHX711();
  sensors.radar.detected = detectRadar();
  sensors.audio.detected = true; // ADC zawsze dostępne
  
  sensors.tempHum.active = sensors.tempHum.detected;
  sensors.airQual.active = sensors.airQual.detected;
  sensors.hx711.active = sensors.hx711.detected;
  sensors.radar.active = sensors.radar.detected;
  sensors.audio.active = true;
}

void printSensorStatus() {
  Serial.println("\n>> Status sensorów:");
  Serial.printf("  Temp/Wilg:     %s\n", sensors.tempHum.detected ? "OK" : "NIE ZNALEZIONO");
  Serial.printf("  Jakość pow.:   %s\n", sensors.airQual.detected ? "OK" : "NIE ZNALEZIONO");
  Serial.printf("  Waga (HX711):  %s\n", sensors.hx711.detected ? "OK" : "NIE ZNALEZIONO");
  Serial.printf("  Radar:         %s\n", sensors.radar.detected ? "OK" : "NIE ZNALEZIONO");
  Serial.printf("  Audio:         %s\n", sensors.audio.detected ? "OK" : "NIE ZNALEZIONO");
}

void initDetectedSensors() {
  if (sensors.tempHum.active) {
    dht.begin();
    Serial.println(">> DHT zainicjalizowany");
  }
  
  if (sensors.airQual.active) {
    if (sgp41.begin(Wire)) {
      sgp41.measureRawSignal();
      Serial.println(">> SGP41 zainicjalizowany");
    } else {
      sensors.airQual.active = false;
      Serial.println(">> SGP41 inicjalizacja nieudana");
    }
  }
  
  if (sensors.hx711.active) {
    scale.begin(PIN_HX711_DT, PIN_HX711_SCK);
    scale.set_scale(SCALE_CALIBRATION_FACTOR);
    scale.tare(SCALE_TARE_OFFSET);
    hx711_offset = scale.read_average(10);
    Serial.println(">> HX711 zainicjalizowany");
  }
}

void detectConnectionMode() {
  // Prosta detekcja: czekamy chwilę na sygnał z hosta USB
  delay(1000);
  if (Serial) {
    Serial.println("Wykryto połączenie USB. Tryb główny: SERIAL.");
    Serial.println("Wpisz 'HELP' aby zobaczyć listę komend.");
    usbActive = true;
    lastActivityTime = millis();
  } else {
    Serial.println("Nie wykryto hosta USB. Przejście w tryb Ethernet.");
    usbActive = false;
    initEthernet();
  }
}

void initEthernet() {
  Ethernet.init(PIN_ETH_CS);
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Nie udało się uzyskać adresu IP (DHCP). Próba statycznego...");
    Ethernet.begin(mac, ip, gateway, subnet);
  }
  
  server.begin();
  ethActive = true;
  
  IPAddress myIP = Ethernet.localIP();
  Serial.print("Serwer HTTP uruchomiony na: http://");
  Serial.println(myIP);
}

// ================= CZYTANIE SENSORÓW =================

void readAllSensors(unsigned long now) {
  unsigned long readStart = millis();
  int successCount = 0;
  int failCount = 0;
  
  // DHT - temperatura i wilgotność
  if (sensors.tempHum.active) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperature = t;
      humidity = h;
      resetSensorError(sensorErrors.dht_errors);
      successCount++;
    } else {
      if (checkSensorHealth(sensorErrors.dht_errors, "DHT")) {
        logWarning("DHT", "Read failed");
      }
      failCount++;
    }
  }

  // HX711 - waga
  if (sensors.hx711.active && scale.is_ready()) {
    long raw = scale.get_units(5);
    if (raw != 0) {
      hx711_value = (raw - hx711_offset) / hx711_scale;
      weight = (float)hx711_value;
      resetSensorError(sensorErrors.hx711_errors);
      successCount++;
    } else {
      if (checkSensorHealth(sensorErrors.hx711_errors, "HX711")) {
        logWarning("HX711", "Zero reading");
      }
      failCount++;
    }
  }

  // SGP41 - jakość powietrza
  if (sensors.airQual.active && sgp41.isConnected()) {
    sgp41.measureRawSignal();
    nox = sgp41.rawNOx;
    voc = sgp41.rawVoc;
    if (nox > 0 && voc > 0) {
      resetSensorError(sensorErrors.sgp41_errors);
      successCount++;
    } else {
      if (checkSensorHealth(sensorErrors.sgp41_errors, "SGP41")) {
        logWarning("SGP41", "Invalid readings");
      }
      failCount++;
    }
  }

  // Mikrofon (ADC) - zawsze dostępny
  micLevel = analogRead(PIN_MIC);
  if (micLevel >= 0) {
    resetSensorError(sensorErrors.audio_errors);
    successCount++;
  } else {
    if (checkSensorHealth(sensorErrors.audio_errors, "MIC")) {
      logWarning("MIC", "Read failed");
    }
    failCount++;
  }
  
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
  if (failCount >= 3) {
    logError("SYSTEM", "Multiple sensor failures detected");
  }
}

// ================= ANALIZA AUDIO =================

void performFFT() {
  for (int k = 0; k < AUDIO_BUFFER_SIZE / 2; k++) {
    float real = 0.0f;
    float imag = 0.0f;
    
    for (int n = 0; n < AUDIO_BUFFER_SIZE; n++) {
      float angle = (2.0f * PI * k * n) / AUDIO_BUFFER_SIZE;
      real += audioBuffer[n] * cos(angle);
      imag -= audioBuffer[n] * sin(angle);
    }
    
    audioFFT[k] = sqrt(real * real + imag * imag) / AUDIO_BUFFER_SIZE;
  }
}

void readAudioSignal() {
  for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
    int raw = analogRead(PIN_MIC);
    audioBuffer[i] = raw - 2048;
    delayMicroseconds(250);
  }
}

void calculateAudioMetrics() {
  int zeroCrossings = 0;
  for (int i = 1; i < AUDIO_BUFFER_SIZE; i++) {
    if ((audioBuffer[i-1] >= 0 && audioBuffer[i] < 0) ||
        (audioBuffer[i-1] < 0 && audioBuffer[i] >= 0)) {
      zeroCrossings++;
    }
  }
  currentAudioMetrics.zero_crossing_rate = (float)zeroCrossings / AUDIO_BUFFER_SIZE;
  
  float sum = 0.0f;
  currentAudioMetrics.peak_amplitude = 0.0f;
  for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
    float val = abs(audioBuffer[i]);
    sum += val * val;
    if (val > currentAudioMetrics.peak_amplitude) {
      currentAudioMetrics.peak_amplitude = val;
    }
  }
  currentAudioMetrics.rms_amplitude = sqrt(sum / AUDIO_BUFFER_SIZE);
  
  int beeBinMin = (BEE_FREQ_MIN * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
  int beeBinMax = (BEE_FREQ_MAX * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
  int swarmBinMin = (SWARM_FREQ_MIN * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
  int swarmBinMax = (SWARM_FREQ_MAX * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
  
  float beePower = 0.0f;
  float swarmPower = 0.0f;
  float totalPower = 0.0f;
  int dominantBin = 0;
  float maxMagnitude = 0.0f;
  
  for (int i = 1; i < AUDIO_BUFFER_SIZE / 2; i++) {
    float mag = audioFFT[i];
    totalPower += mag * mag;
    
    if (mag > maxMagnitude) {
      maxMagnitude = mag;
      dominantBin = i;
    }
    
    if (i >= beeBinMin && i < beeBinMax) {
      beePower += mag * mag;
    }
    if (i >= swarmBinMin && i < swarmBinMax) {
      swarmPower += mag * mag;
    }
  }
  
  currentAudioMetrics.dominant_frequency = (dominantBin * AUDIO_SAMPLE_RATE) / AUDIO_BUFFER_SIZE;
  currentAudioMetrics.power_in_bee_band = (totalPower > 0) ? beePower / totalPower : 0.0f;
  currentAudioMetrics.power_in_swarm_band = (totalPower > 0) ? swarmPower / totalPower : 0.0f;
  currentAudioMetrics.bee_activity_index = currentAudioMetrics.power_in_bee_band * currentAudioMetrics.rms_amplitude / 1000.0f;
  currentAudioMetrics.swarm_probability = currentAudioMetrics.power_in_swarm_band * 2.0f;
  currentAudioMetrics.hive_health_audio = constrain(currentAudioMetrics.bee_activity_index * 100.0f, 0.0f, 100.0f);
}

void processAudioPeriodically(unsigned long now) {
  static unsigned long lastProcess = 0;
  if (now - lastProcess < 5000) return;
  lastProcess = now;
  
  if (!sensors.audio.active) return;
  
  readAudioSignal();
  performFFT();
  calculateAudioMetrics();
}

// ================= ANALIZA WAGI =================

void updateHX711Buffer() {
  HX711DataPoint point;
  point.timestamp = millis();
  point.weight_raw = (float)hx711_value;
  point.weight_filtered = point.weight_raw;
  point.is_valid = (hx711_value != 0);
  
  hx711Buffer[hx711HistoryIdx] = point;
  hx711HistoryIdx = (hx711HistoryIdx + 1) % HX711_BUFFER_SIZE;
  if (hx711HistoryIdx == 0) historyInitialized = true;
}

void calculateHX711Metrics() {
  float samples[HX711_SHORT_WINDOW];
  int validCount = 0;
  
  for (int i = HX711_BUFFER_SIZE - HX711_SHORT_WINDOW; i < HX711_BUFFER_SIZE; i++) {
    int idx = i % HX711_BUFFER_SIZE;
    if (hx711Buffer[idx].is_valid && validCount < HX711_SHORT_WINDOW) {
      samples[validCount++] = hx711Buffer[idx].weight_filtered;
    }
  }
  
  if (validCount == 0) {
    currentHX711Metrics.mean_weight = 0.0f;
    currentHX711Metrics.current_rate = 0.0f;
    currentHX711Metrics.hive_health_weight = 50.0f;
    return;
  }
  
  float sum = 0.0f;
  currentHX711Metrics.min_weight = samples[0];
  currentHX711Metrics.max_weight = samples[0];
  
  for (int i = 0; i < validCount; i++) {
    sum += samples[i];
    if (samples[i] < currentHX711Metrics.min_weight) currentHX711Metrics.min_weight = samples[i];
    if (samples[i] > currentHX711Metrics.max_weight) currentHX711Metrics.max_weight = samples[i];
  }
  currentHX711Metrics.mean_weight = sum / validCount;
  
  if (validCount >= 2) {
    float timeSpanMinutes = HX711_SHORT_WINDOW / 10.0f;
    currentHX711Metrics.current_rate = (samples[validCount-1] - samples[0]) / timeSpanMinutes;
  }
  
  currentHX711Metrics.trend_direction = constrain(currentHX711Metrics.current_rate / 100.0f, -1.0f, 1.0f);
  
  if (currentHX711Metrics.current_rate > HX711_WEIGHT_CHANGE_THRESH) {
    currentHX711Metrics.nectar_inflow_rate = currentHX711Metrics.current_rate;
    currentHX711Metrics.consumption_rate = 0.0f;
  } else if (currentHX711Metrics.current_rate < -HX711_WEIGHT_CHANGE_THRESH) {
    currentHX711Metrics.consumption_rate = abs(currentHX711Metrics.current_rate);
    currentHX711Metrics.nectar_inflow_rate = 0.0f;
  } else {
    currentHX711Metrics.nectar_inflow_rate = 0.0f;
    currentHX711Metrics.consumption_rate = 0.0f;
  }
  
  float stabilityScore = 100.0f - constrain(currentHX711Metrics.std_weight * 10.0f, 0.0f, 100.0f);
  float activityScore = constrain(abs(currentHX711Metrics.nectar_inflow_rate - currentHX711Metrics.consumption_rate) * 10.0f, 0.0f, 100.0f);
  currentHX711Metrics.hive_health_weight = (stabilityScore + activityScore) / 2.0f;
}

void processWeightPeriodically(unsigned long now) {
  static unsigned long lastProcess = 0;
  if (now - lastProcess < 6000) return;
  lastProcess = now;
  
  if (!sensors.hx711.active) return;
  
  updateHX711Buffer();
  calculateHX711Metrics();
}

// ================= JAKOŚĆ POWIETRZA =================

void updateAirHistory() {
  if (nox > 5000 || voc > 500) return;
  
  co2History[airHistoryIdx] = nox;
  vocHistory[airHistoryIdx] = voc;
  airHistoryIdx = (airHistoryIdx + 1) % AIRQUAL_BUFFER_SIZE;
  if (airHistoryIdx == 0) historyInitialized = true;
}

float calculateMeanCO2(int window) {
  uint32_t sum = 0;
  int count = 0;
  int start = (window > airHistoryIdx) ? 0 : airHistoryIdx - window;
  
  for (int i = start; i < airHistoryIdx && count < window; i++) {
    if (co2History[i] > 0 && co2History[i] < 10000) {
      sum += co2History[i];
      count++;
    }
  }
  
  return (count > 0) ? (float)sum / count : 0.0f;
}

void calculateAirMetrics() {
  currentAirMetrics.co2_eq = nox;
  currentAirMetrics.voc_idx = voc;
  
  updateAirHistory();
  currentAirMetrics.mean_co2 = calculateMeanCO2(60);
  
  float co2Score = 100.0f - constrain((float)currentAirMetrics.co2_eq / CO2_WARNING_LEVEL * 100.0f, 0.0f, 100.0f);
  float vocScore = 100.0f - constrain((float)currentAirMetrics.voc_idx / VOC_ALERT_LEVEL * 100.0f, 0.0f, 100.0f);
  currentAirMetrics.iaq_index = (co2Score + vocScore) / 2.0f;
  
  if (currentAirMetrics.co2_eq > CO2_WARNING_LEVEL || currentAirMetrics.voc_idx > VOC_ALERT_LEVEL) {
    currentAirMetrics.ventilation_need = constrain(
      ((float)currentAirMetrics.co2_eq / CO2_WARNING_LEVEL + 
       (float)currentAirMetrics.voc_idx / VOC_ALERT_LEVEL) * 50.0f,
      0.0f, 100.0f);
  } else {
    currentAirMetrics.ventilation_need = 0.0f;
  }
}

void processAirQualityPeriodically(unsigned long now) {
  static unsigned long lastProcess = 0;
  if (now - lastProcess < 60000) return;
  lastProcess = now;
  
  if (!sensors.airQual.active) return;
  
  calculateAirMetrics();
}

// ================= RADAR =================

bool readRadarData() {
  static uint8_t buffer[24];
  static int bufIdx = 0;
  
  while (radarSerial.available()) {
    uint8_t b = radarSerial.read();
    
    if (bufIdx == 0 && b != 0xF4) continue;
    
    buffer[bufIdx++] = b;
    
    if (bufIdx >= 24) {
      if (buffer[0] == 0xF4 && buffer[1] == 0xF3 && 
          buffer[2] == 0xF2 && buffer[3] == 0xF1) {
        
        uint16_t dist = buffer[8] | (buffer[9] << 8);
        uint16_t energy = buffer[12] | (buffer[13] << 8);
        
        if (dist > 5000) {
          bufIdx = 0;
          return false;
        }
        
        currentRadarMetrics.distance = (float)dist / 100.0f;
        currentRadarMetrics.energy = (float)energy / 100.0f;
        currentRadarMetrics.speed = (energy > 50) ? 0.5f : 0.0f;
        
        bufIdx = 0;
        return true;
      }
      
      bufIdx = 0;
    }
  }
  
  return false;
}

void updateRadarHistory() {
  distanceHistory[radarHistoryIdx] = currentRadarMetrics.distance;
  energyHistory[radarHistoryIdx] = currentRadarMetrics.energy;
  radarHistoryIdx = (radarHistoryIdx + 1) % RADAR_BUFFER_SIZE;
  if (radarHistoryIdx == 0) historyInitialized = true;
}

void calculateRadarMetrics() {
  updateRadarHistory();
  
  float sum = 0.0f;
  int count = 0;
  int start = (RADAR_TREND_WINDOW > radarHistoryIdx) ? 0 : radarHistoryIdx - RADAR_TREND_WINDOW;
  
  for (int i = start; i < radarHistoryIdx && count < RADAR_TREND_WINDOW; i++) {
    if (energyHistory[i] > 30.0f) count++;
  }
  
  currentRadarMetrics.activity_ratio = (count > 0) ? (float)count / RADAR_TREND_WINDOW : 0.0f;
  currentRadarMetrics.motion_intensity = constrain(currentRadarMetrics.energy / 100.0f, 0.0f, 1.0f);
  
  float activityScore = currentRadarMetrics.activity_ratio * 100.0f;
  currentRadarMetrics.hive_health_index = (activityScore + currentRadarMetrics.signal_quality) / 2.0f;
}

void processRadarPeriodically(unsigned long now) {
  static unsigned long lastProcess = 0;
  if (now - lastProcess < 2000) return;
  lastProcess = now;
  
  if (!sensors.radar.active) return;
  
  if (readRadarData()) {
    calculateRadarMetrics();
  }
}

void handleUSBCommand() {
  static String buffer = "";
  
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCommand(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }
}

void handleEthernetClient(EthernetClient client) {
  if (client.connected()) {
    if (client.available()) {
      String request = client.readStringUntil('\r');
      // Bardzo uproszczona obsługa HTTP GET
      if (request.indexOf("/api/status") >= 0) {
        sendJsonResponse(client);
      } else if (request.indexOf("/cmd?") >= 0) {
        // Parsowanie komendy z URL (np. /cmd?SET_HEATER=50)
        int eqIndex = request.indexOf('=');
        if (eqIndex != -1) {
          String cmdPart = request.substring(request.indexOf('?') + 1, eqIndex);
          String valPart = request.substring(eqIndex + 1);
          // Usunięcie ewentualnych znaków kończących HTTP
          valPart.trim(); 
          processCommand(cmdPart + " " + valPart);
        }
        sendHtmlDashboard(client);
      } else {
        sendHtmlDashboard(client);
      }
      delay(1);
      client.stop();
    }
  }
}

void processCommand(String cmd) {
  cmd.toUpperCase();
  cmd.trim();
  
  // Przykładowe komendy:
  // SET_HEATER 50
  // SET_FAN 100
  // SET_PUMP 0
  // SET_RELAY1 ON
  // CALIB_WEIGHT
  // STATUS
  
  if (cmd.startsWith("SET_HEATER")) {
    int val = extractValue(cmd);
    heaterDuty = constrain(val, 0, 255);
    Serial.println("OK: Grzałka ustawiona na " + String(heaterDuty));
  }
  else if (cmd.startsWith("SET_FAN")) {
    int val = extractValue(cmd);
    fanDuty = constrain(val, 0, 255);
    Serial.println("OK: Wentylator ustawiony na " + String(fanDuty));
  }
  else if (cmd.startsWith("SET_PUMP")) {
    int val = extractValue(cmd);
    pumpDuty = constrain(val, 0, 255);
    Serial.println("OK: Pompa ustawiona na " + String(pumpDuty));
  }
  else if (cmd.startsWith("SET_RELAY1")) {
    relay1State = (cmd.indexOf("ON") > 0 || cmd.indexOf("1") > 0);
    Serial.println("OK: Przekaźnik 1: " + String(relay1State ? "ON" : "OFF"));
  }
  else if (cmd.startsWith("SET_RELAY2")) {
    relay2State = (cmd.indexOf("ON") > 0 || cmd.indexOf("1") > 0);
    Serial.println("OK: Przekaźnik 2: " + String(relay2State ? "ON" : "OFF"));
  }
  else if (cmd.startsWith("CALIB_WEIGHT")) {
    scale.tare();
    Serial.println("OK: Waga wyzerowana (Tara).");
  }
  else if (cmd.startsWith("STATUS")) {
    printStatus(Serial);
  }
  else if (cmd.startsWith("HELP")) {
    Serial.println("Dostępne komendy:");
    Serial.println("  SET_HEATER [0-255]");
    Serial.println("  SET_FAN [0-255]");
    Serial.println("  SET_PUMP [0-255]");
    Serial.println("  SET_RELAY1 [ON/OFF]");
    Serial.println("  SET_RELAY2 [ON/OFF]");
    Serial.println("  CALIB_WEIGHT");
    Serial.println("  STATUS");
  }
  else {
    Serial.println("Nieznana komenda. Wpisz HELP.");
  }
}

int extractValue(String cmd) {
  int spaceIndex = cmd.indexOf(' ');
  if (spaceIndex != -1) {
    return cmd.substring(spaceIndex + 1).toInt();
  }
  return 0;
}

void updateOutputs() {
  analogWrite(PIN_HEATER, heaterDuty);
  analogWrite(PIN_FAN, fanDuty);
  analogWrite(PIN_PUMP, pumpDuty);
  digitalWrite(PIN_RELAY1, relay1State ? HIGH : LOW);
  digitalWrite(PIN_RELAY2, relay2State ? HIGH : LOW);
}

void printStatus(Print& out) {
  out.print("{\"temp\":"); out.print(temperature);
  out.print(", \"hum\":"); out.print(humidity);
  out.print(", \"weight\":"); out.print(weight);
  out.print(", \"nox\":"); out.print(nox);
  out.print(", \"voc\":"); out.print(voc);
  out.print(", \"mic\":"); out.print(micLevel);
  out.print(", \"heater\":"); out.print(heaterDuty);
  out.print(", \"fan\":"); out.print(fanDuty);
  out.print(", \"relay1\":"); out.print(relay1State ? 1 : 0);
  // Dodatkowe metryki z analiz
  out.print(", \"audio_bee_idx\":"); out.print(currentAudioMetrics.bee_activity_index, 2);
  out.print(", \"audio_swarm_prob\":"); out.print(currentAudioMetrics.swarm_probability, 2);
  out.print(", \"weight_rate\":"); out.print(currentHX711Metrics.current_rate, 3);
  out.print(", \"weight_health\":"); out.print(currentHX711Metrics.hive_health_weight, 1);
  out.print(", \"air_iaq\":"); out.print(currentAirMetrics.iaq_index, 1);
  out.print(", \"radar_activity\":"); out.print(currentRadarMetrics.activity_ratio, 2);
  out.println("}");
}

void sendJsonResponse(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  printStatus(client);
}

void sendHtmlDashboard(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>Pico Controller</title>");
  client.println("<meta http-equiv='refresh' content='5'>");
  client.println("</head><body>");
  client.println("<h1>Panel Sterowania Pico</h1>");
  client.print("<p>Temperatura: "); client.print(temperature); client.println(" C</p>");
  client.print("<p>Wilgotność: "); client.print(humidity); client.println(" %</p>");
  client.print("<p>Waga: "); client.print(weight); client.println(" kg</p>");
  client.print("<p>NOx: "); client.print(nox); client.println("</p>");
  
  client.println("<h3>Sterowanie</h3>");
  client.println("<a href='/cmd?SET_HEATER=0'><button>Grzałka OFF</button></a>");
  client.println("<a href='/cmd?SET_HEATER=128'><button>Grzałka 50%</button></a>");
  client.println("<a href='/cmd?SET_HEATER=255'><button>Grzałka 100%</button></a><br>");
  
  client.println("<a href='/cmd?SET_RELAY1=ON'><button>Relay1 ON</button></a>");
  client.println("<a href='/cmd?SET_RELAY1=OFF'><button>Relay1 OFF</button></a><br>");
  
  client.println("</body></html>");
}
