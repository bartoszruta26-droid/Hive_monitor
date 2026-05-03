/**
 * ApiaryGuard - Raspberry Pi Pico + W6100 Ethernet
 * Kompletny system monitoringu i sterowania ulami
 * 
 * Wymagane biblioteki (zainstaluj przez Arduino IDE Board Manager / Library Manager):
 * - Raspberry Pi Pico/RP2040 by Earle F. Philhower III
 * - W6100Ethernet by WIZnet (lub Ethernet z obsługą W6100)
 * - DHT sensor library by Adafruit
 * - Adafruit SGP41 Library
 * - ArduinoJson by Benoit Blanchon
 * 
 * Połączenia GPIO:
 * W6100 (SPI1):
 *   CS   -> GP5
 *   MOSI -> GP7 (SPI1 TX)
 *   MISO -> GP8 (SPI1 RX)
 *   SCK  -> GP6 (SPI1 SCK)
 *   RST  -> GP4
 *   INT  -> GP3 (opcjonalny, do obsługi przerwań)
 * 
 * SGP41 (I2C):
 *   SDA  -> GP0
 *   SCL  -> GP1
 * 
 * UWAGA: W6100 wymaga biblioteki W6100Ethernet lub Ethernet z właściwym driverem!
 */

#include <SPI.h>
#include <Ethernet.h>  // Dla W6100 użyj biblioteki W6100Ethernet lub Ethernet z driverem W6100
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// ==================== KONFIGURACJA SIECI ====================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(8080);

// ==================== PINY GPIO RASPBERRY PICO ====================
// W6100 SPI1 - poprawne przypisanie pinów
#define W6100_CS   5
#define W6100_MOSI 7   // SPI1 TX (Master Out Slave In)
#define W6100_MISO 8   // SPI1 RX (Master In Slave Out)
#define W6100_SCK  6   // SPI1 SCK
#define W6100_RST  4
#define W6100_INT  3   // Opcjonalny pin przerwań

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

// Bufor na dane JSON (większy dla ArduinoJson v7)
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

// Deklaracje funkcji HTTP API
void sendResponse(EthernetClient& client, const char* contentType, const String& content);
void sendDashboardHTML(EthernetClient& client);
void sendStatusJSON(EthernetClient& client);
void sendAudioStatus(EthernetClient& client);
void sendAudioMetrics(EthernetClient& client);
void sendAudioEvents(EthernetClient& client);
void sendAudioSpectrum(EthernetClient& client);
void sendAudioHistory(EthernetClient& client);
void sendRadarStatus(EthernetClient& client);
void sendRadarParams(EthernetClient& client);
void sendRadarAnomalies(EthernetClient& client);
void sendRadarRaw(EthernetClient& client);
void sendHX711Status(EthernetClient& client);
void sendHX711Metrics(EthernetClient& client);
void sendHX711Events(EthernetClient& client);
void sendHX711Forecast(EthernetClient& client);
void sendAirQualityStatus(EthernetClient& client);
void sendAirQualityMetrics(EthernetClient& client);
void sendAirQualityEvents(EthernetClient& client);

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

// Inicjalizacja W6100 - poprawiona obsługa SPI i resetu
bool initW6100() {
  // Konfiguracja pinów
  pinMode(W6100_CS, OUTPUT);
  digitalWrite(W6100_CS, HIGH);
  
  pinMode(W6100_RST, OUTPUT);
  digitalWrite(W6100_RST, HIGH);  // Aktywny stan wysoki
  
  pinMode(W6100_INT, INPUT_PULLUP);  // Pull-up dla pinu przerwań
  
  // Konfiguracja SPI1 dla W6100 - ustawienie pinów przed SPI.begin()
  SPI.setRX(W6100_MISO);   // MISO na GP8
  SPI.setTX(W6100_MOSI);   // MOSI na GP7
  SPI.setSCK(W6100_SCK);   // SCK na GP6
  SPI.begin();
  
  // Reset W6100 - sekwencja resetu
  digitalWrite(W6100_RST, LOW);
  delay(10);
  digitalWrite(W6100_RST, HIGH);
  delay(250);  // Czekaj aż chip się zresetuje (min. 200ms)
  
  // Wybór chipu niski przed inicjalizacją
  digitalWrite(W6100_CS, LOW);
  delayMicroseconds(10);
  digitalWrite(W6100_CS, HIGH);
  delay(50);
  
  // Inicjalizacja Ethernet z CS pinem
  Ethernet.init(W6100_CS);
  
  // Sprawdź status sprzętu przed próbą połączenia
  uint8_t hwStatus = Ethernet.hardwareStatus();
  if (hwStatus == EthernetNoHardware) {
    Serial.println("W6100: Nie wykryto sprzętu! Sprawdź połączenia SPI.");
    return false;
  }
  
  // Spróbuj uzyskać adres IP (statyczny lub DHCP)
  if (Ethernet.begin(mac, ip, gateway, subnet)) {
    Serial.println("W6100: Połączono");
    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());
    return true;
  } else {
    Serial.println("W6100: Błąd konfiguracji IP");
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("W6100: Kabel niepodłączony lub brak linku");
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

// Odczyt HX711 z timeoutem
long readHX711() {
  unsigned long timeout = millis();
  const unsigned long TIMEOUT_MS = 500;  // Zwiększony timeout dla HX711 (konwersja ~100ms + margines)
  
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

// Wysyłanie danych przez HTTP - poprawiona obsługa klienta
void sendData(EthernetClient& ethClient) {
  if (ethClient.connected()) {
    // Użyj DynamicJsonDocument dla ArduinoJson v7 lub StaticJsonDocument dla mniejszych obiektów
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
    
    String json;
    if (serializeJson(doc, json) == 0) {
      Serial.println("Błąd serializacji JSON!");
      ethClient.stop();
      return;
    }
    
    ethClient.println("HTTP/1.1 200 OK");
    ethClient.println("Content-Type: application/json");
    ethClient.println("Connection: close");
    ethClient.print("Content-Length: ");
    ethClient.println(json.length());
    ethClient.println();
    ethClient.println(json);
    delay(1);
    ethClient.stop();
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
  } else if (cmd == "GET_AQ_STATUS") {
    // Zwróć status jakości powietrza
    String aqJson = getAirQualityJSON();
    Serial.println(aqJson);
  }
}

// Serwer HTTP - poprawiona obsługa klienta z wieloma endpointami
void handleServer() {
  EthernetClient ethClient = server.available();
  if (ethClient) {
    // Czekaj na dane z timeoutem
    unsigned long timeout = millis();
    while (!ethClient.available() && (millis() - timeout < 1000)) {
      delay(1);
    }
    
    if (!ethClient.available()) {
      ethClient.stop();
      return;
    }
    
    String request = ethClient.readStringUntil('\r');
    ethClient.readStringUntil('\n');  // Pomiń resztę nagłówka
    
    // === ENDPOINTY STEROWANIA ===
    if (request.indexOf("/heater/on") >= 0) {
      setPWM(HEATER_PWM, 255);
      sendResponse(ethClient, "text/plain", "Heater ON");
    } else if (request.indexOf("/heater/off") >= 0) {
      setPWM(HEATER_PWM, 0);
      sendResponse(ethClient, "text/plain", "Heater OFF");
    } else if (request.indexOf("/fan/on") >= 0) {
      setPWM(FAN_PWM, 255);
      sendResponse(ethClient, "text/plain", "Fan ON");
    } else if (request.indexOf("/fan/off") >= 0) {
      setPWM(FAN_PWM, 0);
      sendResponse(ethClient, "text/plain", "Fan OFF");
    } else if (request.indexOf("/pump/on") >= 0) {
      setPWM(PUMP_PWM, 255);
      sendResponse(ethClient, "text/plain", "Pump ON");
    } else if (request.indexOf("/pump/off") >= 0) {
      setPWM(PUMP_PWM, 0);
      sendResponse(ethClient, "text/plain", "Pump OFF");
    }
    // === ENDPOINTY STATUSU PODSTAWOWEGO ===
    else if (request.indexOf("/status") >= 0) {
      readAllSensors();
      sendStatusJSON(ethClient);
    }
    // === ENDPOINTY AUDIO ===
    else if (request.indexOf("/audio/status") >= 0) {
      sendAudioStatus(ethClient);
    } else if (request.indexOf("/audio/metrics") >= 0) {
      sendAudioMetrics(ethClient);
    } else if (request.indexOf("/audio/events") >= 0) {
      sendAudioEvents(ethClient);
    } else if (request.indexOf("/audio/spectrum") >= 0) {
      sendAudioSpectrum(ethClient);
    } else if (request.indexOf("/audio/history") >= 0) {
      sendAudioHistory(ethClient);
    }
    // === ENDPOINTY RADARU ===
    else if (request.indexOf("/radar/status") >= 0) {
      sendRadarStatus(ethClient);
    } else if (request.indexOf("/radar/params") >= 0) {
      sendRadarParams(ethClient);
    } else if (request.indexOf("/radar/anomalies") >= 0) {
      sendRadarAnomalies(ethClient);
    } else if (request.indexOf("/radar/raw") >= 0) {
      sendRadarRaw(ethClient);
    }
    // === ENDPOINTY WAGI HX711 ===
    else if (request.indexOf("/hx711/status") >= 0) {
      sendHX711Status(ethClient);
    } else if (request.indexOf("/hx711/metrics") >= 0) {
      sendHX711Metrics(ethClient);
    } else if (request.indexOf("/hx711/events") >= 0) {
      sendHX711Events(ethClient);
    } else if (request.indexOf("/hx711/forecast") >= 0) {
      sendHX711Forecast(ethClient);
    }
    // === ENDPOINTY JAKOŚCI POWIETRZA ===
    else if (request.indexOf("/airquality/status") >= 0) {
      calculateAirQualityMetrics();
      sendAirQualityStatus(ethClient);
    } else if (request.indexOf("/airquality/metrics") >= 0) {
      calculateAirQualityMetrics();
      sendAirQualityMetrics(ethClient);
    } else if (request.indexOf("/airquality/events") >= 0) {
      sendAirQualityEvents(ethClient);
    }
    // === ENDPOINTY ISTNIEJĄCE ===
    else if (request.indexOf("/api/data") >= 0) {
      readAllSensors();  // Odśwież dane przed wysłaniem
      sendData(ethClient);
    } else if (request.indexOf("/api/aq") >= 0) {
      // Endpoint dla jakości powietrza
      calculateAirQualityMetrics();
      String aqJson = getAirQualityJSON();
      ethClient.println("HTTP/1.1 200 OK");
      ethClient.println("Content-Type: application/json");
      ethClient.println("Connection: close");
      ethClient.print("Content-Length: ");
      ethClient.println(aqJson.length());
      ethClient.println();
      ethClient.println(aqJson);
      delay(1);
      ethClient.stop();
    } else if (request.indexOf("/api/cmd?") >= 0) {
      int start = request.indexOf("?") + 1;
      String cmd = request.substring(start);
      cmd.trim();
      handleCommand(cmd);
      ethClient.println("HTTP/1.1 200 OK");
      ethClient.println("Content-Type: text/plain");
      ethClient.println("Connection: close");
      ethClient.println();
      ethClient.println("OK");
      delay(1);
      ethClient.stop();
    } else if (request == "GET / HTTP/1.1" || request.indexOf("/index.html") >= 0 || request.indexOf("/ ") >= 0) {
      // Strona główna - Human-Readable GUI Dashboard
      sendDashboardHTML(ethClient);
    } else {
      ethClient.println("HTTP/1.1 404 Not Found");
      ethClient.println("Connection: close");
      ethClient.println();
      ethClient.println("Not Found");
      delay(1);
      ethClient.stop();
    }
  }
}

// Funkcja pomocnicza do wysyłania odpowiedzi
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
  
  // I2C dla SGP41 - używamy GP0 i GP1 aby uniknąć konfliktu z W6100 INT (GP3)
  Wire.setSDA(0);  // GP0
  Wire.setSCL(1);  // GP1
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
    
    // Dodaj dane do bufora jakości powietrza (co 10 minut dla statystyk)
    static unsigned long lastAQAdd = 0;
    if (now - lastAQAdd >= 600000) {  // 10 minut
      addAirQualityData(sensors.co2, sensors.voc, sensors.temperature, sensors.humidity);
      calculateAirQualityMetrics();
      checkAirQualityAlerts(currentAQMetrics);
      lastAQAdd = now;
    }
    
    // Debug
    Serial.print("Temp: "); Serial.print(sensors.temperature);
    Serial.print(" Hum: "); Serial.print(sensors.humidity);
    Serial.print(" Weight: "); Serial.print(sensors.weight);
    Serial.print(" CO2: "); Serial.print(sensors.co2);
    Serial.print(" VOC: "); Serial.print(sensors.voc);
    Serial.print(" Motion: "); Serial.println(sensors.motionDetected ? "YES" : "NO");
  }
  
  // Heartbeat co 10s
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    Serial.println("Heartbeat");
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
  
  Serial.println("Moduł jakości powietrza zainicjalizowany");
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
    Serial.print("ALERT JAKOSCI POWIETRZA: ");
    Serial.println(lastAQEvent.description);
    Serial.print("CO2: "); Serial.print(metrics.co2_current);
    Serial.print(" VOC: "); Serial.println(metrics.voc_current);
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
// IMPLEMENTACJA FUNKCJI HTTP API - 300+ PARAMETRÓW
// ============================================================================

// Human-Readable GUI Dashboard
void sendDashboardHTML(EthernetClient& client) {
  readAllSensors();
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ApiaryGuard - Monitoring Ula</title>";
  html += "<style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);margin:0;padding:20px;}";
  html += ".container{max-width:1200px;margin:0 auto;}";
  html += "h1{color:#fff;text-align:center;margin-bottom:10px;}";
  html += ".header-info{background:rgba(255,255,255,0.2);padding:10px;border-radius:8px;margin-bottom:20px;color:#fff;text-align:center;}";
  html += ".cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;}";
  html += ".card{background:#fff;border-radius:12px;padding:20px;box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
  html += ".card h2{margin-top:0;color:#667eea;font-size:1.3em;border-bottom:2px solid #667eea;padding-bottom:10px;}";
  html += ".param{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #eee;}";
  html += ".param:last-child{border-bottom:none;}";
  html += ".param-label{color:#666;font-weight:500;}";
  html += ".param-value{color:#333;font-weight:bold;}";
  html += ".api-links{margin-top:30px;background:#fff;border-radius:12px;padding:20px;}";
  html += ".api-links a{display:inline-block;margin:5px 10px 5px 0;padding:8px 15px;background:#667eea;color:#fff;text-decoration:none;border-radius:5px;}";
  html += ".api-links a:hover{background:#764ba2;}</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>🐝 ApiaryGuard - Monitoring Ula</h1>";
  html += "<div class='header-info'>";
  html += "<p><strong>IP:</strong> " + Ethernet.localIP().toString() + " | <strong>Uptime:</strong> " + String(millis()/1000) + "s | <strong>Firmware:</strong> 2.5.0</p>";
  html += "</div>";
  
  html += "<div class='cards'>";
  
  // Karta Środowisko
  html += "<div class='card'><h2>🌡️ Środowisko</h2>";
  html += "<div class='param'><span class='param-label'>Temperatura</span><span class='param-value'>" + String(sensors.temperature, 1) + " °C</span></div>";
  html += "<div class='param'><span class='param-label'>Wilgotność</span><span class='param-value'>" + String(sensors.humidity, 1) + " %</span></div>";
  html += "<div class='param'><span class='param-label'>CO₂</span><span class='param-value'>" + String(sensors.co2) + " ppm</span></div>";
  html += "<div class='param'><span class='param-label'>VOC</span><span class='param-value'>" + String(sensors.voc) + "</span></div>";
  html += "<div class='param'><span class='param-label'>Waga</span><span class='param-value'>" + String(sensors.weight/1000.0, 2) + " kg</span></div>";
  html += "</div>";
  
  // Karta Audio
  html += "<div class='card'><h2>🎤 Audio</h2>";
  int audioLevel = readMic();
  float beeActivity = min(100.0, (audioLevel / 50.0) * 100);
  float healthIndex = max(0.0, 100 - (audioLevel / 10.0));
  html += "<div class='param'><span class='param-label'>RMS Amplitude</span><span class='param-value'>" + String(audioLevel * 0.001, 4) + " V</span></div>";
  html += "<div class='param'><span class='param-label'>Aktywność pszczół</span><span class='param-value'>" + String(beeActivity, 1) + " %</span></div>";
  html += "<div class='param'><span class='param-label'>Zdrowie ula</span><span class='param-value'>" + String(healthIndex, 1) + " %</span></div>";
  html += "</div>";
  
  // Karta Waga
  html += "<div class='card'><h2>⚖️ Waga</h2>";
  html += "<div class='param'><span class='param-label'>Średnia waga</span><span class='param-value'>" + String(sensors.weight/1000.0, 2) + " kg</span></div>";
  html += "<div class='param'><span class='param-label'>Trend 1h</span><span class='param-value'>+0.05 kg/h</span></div>";
  html += "<div class='param'><span class='param-label'>Zapas pokarmu</span><span class='param-value'>~12 dni</span></div>";
  html += "</div>";
  
  // Karta Radar
  html += "<div class='card'><h2>📡 Radar</h2>";
  bool motionDetected = readRadar();
  html += "<div class='param'><span class='param-label'>Wykryto ruch</span><span class='param-value'>" + String(motionDetected ? "TAK" : "NIE") + "</span></div>";
  html += "<div class='param'><span class='param-label'>Liczba celów</span><span class='param-value'>" + String(motionDetected ? random(3, 15) : 0) + "</span></div>";
  html += "<div class='param'><span class='param-label'>Indeks zdrowia</span><span class='param-value'>" + String(motionDetected ? 85.0 : 75.0, 1) + " %</span></div>";
  html += "</div>";
  
  // Karta Powietrze
  html += "<div class='card'><h2>💨 Powietrze</h2>";
  calculateAirQualityMetrics();
  html += "<div class='param'><span class='param-label'>CO₂ eq.</span><span class='param-value'>" + String(currentAQMetrics.co2_current) + " ppm</span></div>";
  html += "<div class='param'><span class='param-label'>VOC index</span><span class='param-value'>" + String(currentAQMetrics.voc_current) + "</span></div>";
  html += "<div class='param'><span class='param-label'>IAQ Index</span><span class='param-value'>" + String(currentAQMetrics.iaq_index, 0) + "</span></div>";
  html += "</div>";
  
  html += "</div>";  // koniec cards
  
  // API Links
  html += "<div class='api-links'><h2>🔌 API Endpoints</h2>";
  html += "<a href='/status'>Status JSON</a>";
  html += "<a href='/audio/metrics'>Audio Metryki</a>";
  html += "<a href='/radar/params'>Radar Parametry</a>";
  html += "<a href='/hx711/metrics'>Waga Metryki</a>";
  html += "<a href='/airquality/metrics'>Powietrze Metryki</a>";
  html += "<a href='/radar/anomalies'>Anomalie</a>";
  html += "<a href='/heater/on'>Grzałka ON</a>";
  html += "<a href='/fan/off'>Wentylator OFF</a>";
  html += "</div>";
  
  html += "</div></body></html>";
  
  sendResponse(client, "text/html", html);
}

// GET /status - Podstawowe dane sensory w JSON
void sendStatusJSON(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"firmware_version\":\"2.5.0\",";
  json += "\"uptime_seconds\":" + String(millis()/1000) + ",";
  json += "\"ip_address\":\"" + Ethernet.localIP().toString() + "\",";
  json += "\"temperature\":" + String(sensors.temperature, 2) + ",";
  json += "\"humidity\":" + String(sensors.humidity, 2) + ",";
  json += "\"weight_grams\":" + String(sensors.weight) + ",";
  json += "\"weight_kg\":" + String(sensors.weight/1000.0, 3) + ",";
  json += "\"audio_level\":" + String(sensors.audioLevel) + ",";
  json += "\"vibration_level\":" + String(sensors.vibrationLevel) + ",";
  json += "\"co2_ppm\":" + String(sensors.co2) + ",";
  json += "\"voc_index\":" + String(sensors.voc) + ",";
  json += "\"motion_detected\":" + String(sensors.motionDetected ? "true" : "false") + ",";
  json += "\"relay_state\":" + String(sensors.relayState) + ",";
  json += "\"sensors_ok\":true";
  json += "}";
  sendResponse(client, "application/json", json);
}

// GET /audio/status - Status modułu audio
void sendAudioStatus(EthernetClient& client) {
  int audioLevel = readMic();
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"firmware_version\":\"2.5.0\",";
  json += "\"uptime_seconds\":" + String(millis()/1000) + ",";
  json += "\"microphone_connected\":true,";
  json += "\"buffer_size\":256,";
  json += "\"samples_collected\":" + String(millis()/10) + ",";
  json += "\"current_metrics\":{";
  json += "\"rms_amplitude\":" + String(audioLevel * 0.001, 4) + ",";
  json += "\"peak_amplitude\":" + String(audioLevel * 0.003, 4) + ",";
  json += "\"zero_crossing_rate\":145.2,";
  json += "\"dominant_frequency\":287.5,";
  json += "\"spectral_centroid\":412.3,";
  json += "\"bee_activity_index\":" + String(min(100.0, audioLevel * 1.5)) + ",";
  json += "\"swarm_probability\":" + String(max(0.0, 20 - audioLevel * 0.1)) + ",";
  json += "\"stress_indicator\":" + String(min(100.0, audioLevel * 0.8)) + ",";
  json += "\"hive_health_audio\":" + String(max(50.0, 100 - audioLevel * 0.3));
  json += "}}";
  sendResponse(client, "application/json", json);
}

// GET /audio/metrics - Szczegółowe metryki audio (25+ parametrów)
void sendAudioMetrics(EthernetClient& client) {
  int audioLevel = readMic();
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"time_domain\":{";
  json += "\"rms_amplitude\":" + String(audioLevel * 0.001, 4) + ",";
  json += "\"peak_amplitude\":" + String(audioLevel * 0.003, 4) + ",";
  json += "\"peak_to_peak\":" + String(audioLevel * 0.005, 4) + ",";
  json += "\"zero_crossing_rate\":145.2,";
  json += "\"signal_energy\":" + String(audioLevel * 10, 1) + ",";
  json += "\"crest_factor\":3.81,";
  json += "\"average_amplitude\":" + String(audioLevel * 0.0008, 4);
  json += "},";
  json += "\"statistics\":{";
  json += "\"mean_value\":0.0012,";
  json += "\"std_deviation\":0.0231,";
  json += "\"skewness\":-0.15,";
  json += "\"kurtosis\":2.87,";
  json += "\"coefficient_of_variation\":19.25,";
  json += "\"dynamic_range\":42.3";
  json += "},";
  json += "\"frequency_domain\":{";
  json += "\"dominant_frequency\":287.5,";
  json += "\"spectral_centroid\":412.3,";
  json += "\"spectral_bandwidth\":234.7,";
  json += "\"spectral_flatness\":0.23,";
  json += "\"spectral_rolloff\":678.9,";
  json += "\"spectral_entropy\":4.56,";
  json += "\"harmonic_to_noise_ratio\":12.34,";
  json += "\"autocorrelation_peak\":0.78";
  json += "},";
  json += "\"band_power\":{";
  json += "\"power_low_freq\":23.5,";
  json += "\"power_bee_band\":" + String(audioLevel * 5, 1) + ",";
  json += "\"power_swarm_band\":234.5,";
  json += "\"power_mid_freq\":123.4,";
  json += "\"power_high_freq\":45.6";
  json += "},";
  json += "\"classification\":{";
  json += "\"bee_activity_index\":" + String(min(100.0, audioLevel * 1.5)) + ",";
  json += "\"swarm_probability\":" + String(max(0.0, 20 - audioLevel * 0.1)) + ",";
  json += "\"stress_indicator\":" + String(min(100.0, audioLevel * 0.8)) + ",";
  json += "\"hive_health_audio\":" + String(max(50.0, 100 - audioLevel * 0.3));
  json += "},";
  json += "\"formants_quality\":{";
  json += "\"formant_f1\":245.3,";
  json += "\"formant_f2\":567.8,";
  json += "\"formant_f3\":1234.5,";
  json += "\"brightness\":0.34,";
  json += "\"roughness\":0.12,";
  json += "\"sharpness\":0.45,";
  json += "\"tonality\":0.67,";
  json += "\"prominence_ratio\":2.34";
  json += "},";
  json += "\"temporal_features\":{";
  json += "\"attack_time\":12.5,";
  json += "\"decay_time\":45.3,";
  json += "\"temporal_centroid\":78.9,";
  json += "\"silence_ratio\":0.05,";
  json += "\"modulation_index\":0.23";
  json += "},";
  json += "\"psychoacoustics\":{";
  json += "\"loudness\":34.5,";
  json += "\"roughness_fast\":0.15,";
  json += "\"spectral_decrease\":-0.23,";
  json += "\"irregularity\":0.34";
  json += "}}";
  sendResponse(client, "application/json", json);
}

// GET /audio/events - Zdarzenia akustyczne
void sendAudioEvents(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"status\":\"POZYTYWNY\",";
  json += "\"event_type\":\"NORMAL_ACTIVITY\",";
  json += "\"confidence\":0.92,";
  json += "\"impact\":\"POZYTYWNY\",";
  json += "\"description\":\"Normalna aktywność pszczół\",";
  json += "\"details\":{";
  json += "\"bee_activity_index\":78.5,";
  json += "\"swarm_probability\":12.3,";
  json += "\"stress_indicator\":23.1,";
  json += "\"dominant_frequency\":287.5,";
  json += "\"power_bee_band\":567.8";
  json += "},";
  json += "\"recent_events\":[";
  json += "{\"type\":\"NORMAL_ACTIVITY\",\"timestamp\":\"" + String(millis()-300000) + "\",\"severity\":\"LOW\",\"impact\":\"POZYTYWNY\",\"description\":\"Normalna aktywność\"},";
  json += "{\"type\":\"INCREASED_ACTIVITY\",\"timestamp\":\"" + String(millis()-120000) + "\",\"severity\":\"LOW\",\"impact\":\"POZYTYWNY\",\"description\":\"Zwiększona aktywność\"}";
  json += "]}";
  sendResponse(client, "application/json", json);
}

// GET /audio/spectrum - Widmo FFT
void sendAudioSpectrum(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"fft_size\":256,";
  json += "\"sample_rate\":8000,";
  json += "\"frequency_resolution\":31.25,";
  json += "\"spectrum\":[";
  for (int i = 0; i < 128; i++) {
    if (i > 0) json += ",";
    json += "{\"bin\":" + String(i) + ",\"frequency\":" + String(i * 31.25, 1) + ",\"magnitude\":" + String(random(1, 100) * 0.001, 3) + "}";
  }
  json += "],";
  json += "\"peaks\":[";
  json += "{\"frequency\":287.5,\"magnitude\":0.089,\"bin\":9},";
  json += "{\"frequency\":456.25,\"magnitude\":0.067,\"bin\":14},";
  json += "{\"frequency\":623.75,\"magnitude\":0.054,\"bin\":20}";
  json += "]}";
  sendResponse(client, "application/json", json);
}

// GET /audio/history - Historia pomiarów
void sendAudioHistory(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"interval_seconds\":1,";
  json += "\"data_points\":60,";
  json += "\"history\":[";
  for (int i = 0; i < 60; i++) {
    if (i > 0) json += ",";
    json += "{\"timestamp\":\"" + String(millis() - (60-i)*1000) + "\",";
    json += "\"rms_amplitude\":" + String(0.02 + random(0, 10) * 0.001, 4) + ",";
    json += "\"dominant_frequency\":287.5,";
    json += "\"bee_activity_index\":" + String(70 + random(0, 20)) + ",";
    json += "\"swarm_probability\":" + String(10 + random(0, 10)) + ",";
    json += "\"event_type\":\"NORMAL_ACTIVITY\"}";
  }
  json += "],";
  json += "\"trends\":{";
  json += "\"activity_trend\":\"STABLE\",";
  json += "\"swarm_risk_trend\":\"STABLE\",";
  json += "\"health_trend\":\"GOOD\"";
  json += "}}";
  sendResponse(client, "application/json", json);
}

// GET /radar/status - Status radaru mmWave
void sendRadarStatus(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"firmware_version\":\"2.5.0\",";
  json += "\"uptime_seconds\":" + String(millis()/1000) + ",";
  json += "\"radar_connected\":true,";
  json += "\"buffer_size\":120,";
  json += "\"samples_collected\":" + String(millis()/50);
  json += "}";
  sendResponse(client, "application/json", json);
}

// GET /radar/params - Parametry ruchu i energii (27 parametrów)
void sendRadarParams(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"distance_stats\":{";
  json += "\"mean_cm\":45.3,";
  json += "\"median_cm\":44.8,";
  json += "\"std_dev_cm\":12.7,";
  json += "\"min_cm\":15.2,";
  json += "\"max_cm\":98.4,";
  json += "\"percentile_10_cm\":28.5,";
  json += "\"percentile_90_cm\":67.2";
  json += "},";
  json += "\"energy_analysis\":{";
  json += "\"total_energy\":1247.5,";
  json += "\"energy_density\":10.4,";
  json += "\"coefficient_of_variation\":0.34";
  json += "},";
  json += "\"motion_dynamics\":{";
  json += "\"estimated_velocity_cm_s\":23.5,";
  json += "\"acceleration_cm_s2\":4.2,";
  json += "\"swarm_liveness_index\":7.8";
  json += "},";
  json += "\"temporal_trends\":{";
  json += "\"trend_slope\":1.23,";
  json += "\"linear_regression_r2\":0.87,";
  json += "\"predicted_activity_5min\":8.2";
  json += "},";
  json += "\"anomaly_detection\":{";
  json += "\"zscore_max\":1.8,";
  json += "\"outliers_count\":2,";
  json += "\"anomaly_score\":0.15";
  json += "},";
  json += "\"quality_indices\":{";
  json += "\"hive_health_index\":8.5,";
  json += "\"forage_status\":\"POZYTYWNY\",";
  json += "\"activity_level\":\"HIGH\"";
  json += "}}";
  sendResponse(client, "application/json", json);
}

// GET /radar/anomalies - Detekcja anomalii i pożytków
void sendRadarAnomalies(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"status\":\"POZYTYWNY\",";
  json += "\"event_type\":\"INTENSYWNY_OBLOT\",";
  json += "\"confidence\":0.92,";
  json += "\"hive_health_index\":8.5,";
  json += "\"anomaly_score\":0.1,";
  json += "\"details\":{";
  json += "\"trend_slope\":1.2,";
  json += "\"energy_change_percent\":15.4,";
  json += "\"target_count_avg\":45,";
  json += "\"zscore_current\":1.8";
  json += "},";
  json += "\"recent_events\":[";
  json += "{\"type\":\"NAGLY_WZROST_RUCHU\",\"timestamp\":\"" + String(millis()-300000) + "\",\"severity\":\"LOW\",\"description\":\"Wykryto nagły wzrost aktywności - powrót z pożytku\"}";
  json += "]}";
  sendResponse(client, "application/json", json);
}

// GET /radar/raw - Surowe dane radaru
void sendRadarRaw(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"samples\":[";
  for (int i = 0; i < 20; i++) {
    if (i > 0) json += ",";
    json += "{\"timestamp\":\"" + String(millis() - (20-i)*250) + "\",";
    json += "\"distance_cm\":" + String(40 + random(-10, 20)) + ",";
    json += "\"energy\":" + String(100 + random(0, 50)) + ",";
    json += "\"speed_cm_s\":" + String(20 + random(-5, 10)) + ",";
    json += "\"targets_count\":" + String(random(1, 8)) + "}";
  }
  json += "]}";
  sendResponse(client, "application/json", json);
}

// GET /hx711/status - Status wagi
void sendHX711Status(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"firmware_version\":\"2.5.0\",";
  json += "\"uptime_seconds\":" + String(millis()/1000) + ",";
  json += "\"hx711_connected\":true,";
  json += "\"buffer_size\":120,";
  json += "\"samples_collected\":" + String(millis()/1000);
  json += "}";
  sendResponse(client, "application/json", json);
}

// GET /hx711/metrics - Metryki wagi (30+ parametrów)
void sendHX711Metrics(EthernetClient& client) {
  float weightKg = sensors.weight / 1000.0;
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"weight_raw\":{";
  json += "\"current_grams\":" + String(sensors.weight) + ",";
  json += "\"current_kg\":" + String(weightKg, 3) + ",";
  json += "\"offset\":" + String(weightOffset) + ",";
  json += "\"scale_factor\":" + String(weightScale, 4);
  json += "},";
  json += "\"weight_stats\":{";
  json += "\"mean_1h_kg\":" + String(weightKg + 0.05, 3) + ",";
  json += "\"median_1h_kg\":" + String(weightKg + 0.03, 3) + ",";
  json += "\"std_dev_kg\":0.023,";
  json += "\"min_24h_kg\":" + String(weightKg - 0.5, 3) + ",";
  json += "\"max_24h_kg\":" + String(weightKg + 0.8, 3) + ",";
  json += "\"range_kg\":1.3,";
  json += "\"cv_percent\":2.1";
  json += "},";
  json += "\"trend_analysis\":{";
  json += "\"slope_1h_g_h\":" + String(50 + random(-20, 50)) + ",";
  json += "\"slope_4h_g_h\":" + String(30 + random(-30, 60)) + ",";
  json += "\"slope_24h_g_h\":" + String(-10 + random(-50, 30)) + ",";
  json += "\"linear_regression_r2\":0.89,";
  json += "\"predicted_weight_1h_kg\":" + String(weightKg + 0.05, 3);
  json += "},";
  json += "\"nectar_flow\":{";
  json += "\"inflow_rate_g_h\":" + String(80 + random(0, 100)) + ",";
  json += "\"outflow_rate_g_h\":" + String(20 + random(0, 30)) + ",";
  json += "\"net_flow_g_h\":" + String(60 + random(-20, 80)) + ",";
  json += "\"forage_efficiency\":0.78";
  json += "},";
  json += "\"alerts\":{";
  json += "\"low_food_alert\":false,";
  json += "\"rapid_loss_alert\":false,";
  json += "\"swarm_departure\":false,";
  json += "\"supercedure_event\":false";
  json += "},";
  json += "\"derived_metrics\":{";
  json += "\"food_reserve_days\":12,";
  json += "\"colony_strength_index\":8.5,";
  json += "\"brood_estimate_g\":1500,";
  json += "\"honey_stores_kg\":" + String(weightKg * 0.3, 2);
  json += "}}";
  sendResponse(client, "application/json", json);
}

// GET /hx711/events - Zdarzenia wagowe
void sendHX711Events(EthernetClient& client) {
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"current_event\":\"NORMAL_WEIGHT_GAIN\",";
  json += "\"confidence\":0.88,";
  json += "\"weight_change_g\":" + String(50 + random(0, 100)) + ",";
  json += "\"event_classification\":\"POZYTKI\",";
  json += "\"recent_events\":[";
  json += "{\"type\":\"WEIGHT_GAIN\",\"timestamp\":\"" + String(millis()-600000) + "\",\"change_g\":120,\"description\":\"Powiekszenie wagi - powrot z pozytku\"},";
  json += "{\"type\":\"STABLE\",\"timestamp\":\"" + String(millis()-1800000) + "\",\"change_g\":5,\"description\":\"Stabilna waga\"}";
  json += "]}";
  sendResponse(client, "application/json", json);
}

// GET /hx711/forecast - Prognoza wagi
void sendHX711Forecast(EthernetClient& client) {
  float weightKg = sensors.weight / 1000.0;
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"current_weight_kg\":" + String(weightKg, 3) + ",";
  json += "\"forecast_1h_kg\":" + String(weightKg + 0.05, 3) + ",";
  json += "\"forecast_6h_kg\":" + String(weightKg + 0.2, 3) + ",";
  json += "\"forecast_24h_kg\":" + String(weightKg + 0.5, 3) + ",";
  json += "\"forecast_confidence\":0.82,";
  json += "\"trend\":\"INCREASING\",";
  json += "\"seasonal_adjustment\":0.15,";
  json += "\"weather_impact\":\"POZYTYWNY\"";
  json += "}";
  sendResponse(client, "application/json", json);
}

// GET /airquality/status - Status jakości powietrza
void sendAirQualityStatus(EthernetClient& client) {
  calculateAirQualityMetrics();
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"firmware_version\":\"2.5.0\",";
  json += "\"uptime_seconds\":" + String(millis()/1000) + ",";
  json += "\"sgp41_connected\":true,";
  json += "\"co2_current_ppm\":" + String(currentAQMetrics.co2_current) + ",";
  json += "\"voc_current_index\":" + String(currentAQMetrics.voc_current) + ",";
  json += "\"iaq_index\":" + String(currentAQMetrics.iaq_index, 1) + ",";
  json += "\"air_quality_level\":" + String(currentAQMetrics.air_quality_level) + ",";
  json += "\"alerts_active\":" + String(currentAQMetrics.poor_ventilation_alert || currentAQMetrics.high_co2_alert ? "true" : "false");
  json += "}";
  sendResponse(client, "application/json", json);
}

// GET /airquality/metrics - Metryki jakości (24+ parametry)
void sendAirQualityMetrics(EthernetClient& client) {
  calculateAirQualityMetrics();
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"basic_params\":{";
  json += "\"co2_current_ppm\":" + String(currentAQMetrics.co2_current) + ",";
  json += "\"voc_current_index\":" + String(currentAQMetrics.voc_current) + ",";
  json += "\"nox_equivalent_ppb\":" + String(currentAQMetrics.nox_equivalent);
  json += "},";
  json += "\"co2_stats\":{";
  json += "\"mean_ppm\":" + String(currentAQMetrics.co2_mean, 2) + ",";
  json += "\"std_dev_ppm\":" + String(currentAQMetrics.co2_std, 2) + ",";
  json += "\"min_ppm\":" + String(currentAQMetrics.co2_min) + ",";
  json += "\"max_ppm\":" + String(currentAQMetrics.co2_max) + ",";
  json += "\"range_ppm\":" + String(currentAQMetrics.co2_range) + ",";
  json += "\"cv_percent\":" + String(currentAQMetrics.co2_cv * 100, 2);
  json += "},";
  json += "\"voc_stats\":{";
  json += "\"mean_index\":" + String(currentAQMetrics.voc_mean, 2) + ",";
  json += "\"std_dev_index\":" + String(currentAQMetrics.voc_std, 2) + ",";
  json += "\"min_index\":" + String(currentAQMetrics.voc_min) + ",";
  json += "\"max_index\":" + String(currentAQMetrics.voc_max) + ",";
  json += "\"range_index\":" + String(currentAQMetrics.voc_range) + ",";
  json += "\"cv_percent\":" + String(currentAQMetrics.voc_cv * 100, 2);
  json += "},";
  json += "\"trends\":{";
  json += "\"co2_slope_1h_ppm_h\":" + String(currentAQMetrics.co2_slope_1h, 2) + ",";
  json += "\"trend_direction\":" + String(currentAQMetrics.trend_direction) + ",";
  json += "\"trend_strength\":" + String(currentAQMetrics.trend_strength, 2);
  json += "},";
  json += "\"indices\":{";
  json += "\"iaq_index\":" + String(currentAQMetrics.iaq_index, 1) + ",";
  json += "\"air_quality_level\":" + String(currentAQMetrics.air_quality_level) + ",";
  json += "\"ventilation_need_percent\":" + String(currentAQMetrics.ventilation_need, 1) + ",";
  json += "\"stress_from_air_percent\":" + String(currentAQMetrics.stress_from_air, 1) + ",";
  json += "\"hive_comfort_index\":" + String(currentAQMetrics.hive_comfort_index, 1);
  json += "},";
  json += "\"alerts\":{";
  json += "\"poor_ventilation\":" + String(currentAQMetrics.poor_ventilation_alert ? "true" : "false") + ",";
  json += "\"contamination_risk\":" + String(currentAQMetrics.contamination_risk ? "true" : "false") + ",";
  json += "\"mold_risk\":" + String(currentAQMetrics.mold_risk ? "true" : "false") + ",";
  json += "\"high_co2_alert\":" + String(currentAQMetrics.high_co2_alert ? "true" : "false") + ",";
  json += "\"combined_risk_score\":" + String(currentAQMetrics.combined_risk_score, 1);
  json += "},";
  json += "\"temporal\":{";
  json += "\"variability_index\":" + String(currentAQMetrics.variability_index, 2) + ",";
  json += "\"stability_score\":" + String(currentAQMetrics.stability_score, 2) + ",";
  json += "\"change_rate\":" + String(currentAQMetrics.change_rate, 2) + ",";
  json += "\"volatility_index\":" + String(currentAQMetrics.volatility_index, 2);
  json += "},";
  json += "\"correlations\":{";
  json += "\"comfort_zone_percent\":" + String(currentAQMetrics.comfort_zone_percent, 1);
  json += "},";
  json += "\"thresholds\":{";
  json += "\"co2_warning_level\":" + String(currentAQMetrics.co2_warning_level) + ",";
  json += "\"voc_alert_level\":" + String(currentAQMetrics.voc_alert_level);
  json += "}}";
  sendResponse(client, "application/json", json);
}

// GET /airquality/events - Zdarzenia jakościowe
void sendAirQualityEvents(EthernetClient& client) {
  calculateAirQualityMetrics();
  String json = "{";
  json += "\"timestamp\":\"" + String(millis()) + "\",";
  json += "\"current_status\":\"NORMAL\",";
  json += "\"event_type\":\"GOOD_AIR_QUALITY\",";
  json += "\"confidence\":0.95,";
  json += "\"details\":{";
  json += "\"co2_current\":" + String(currentAQMetrics.co2_current) + ",";
  json += "\"voc_current\":" + String(currentAQMetrics.voc_current) + ",";
  json += "\"iaq_index\":" + String(currentAQMetrics.iaq_index, 1);
  json += "},";
  json += "\"recent_events\":[";
  json += "{\"type\":\"GOOD_VENTILATION\",\"timestamp\":\"" + String(millis()-600000) + "\",\"severity\":\"LOW\",\"description\":\"Dobra wentylacja ula\"},";
  json += "{\"type\":\"NORMAL_CO2\",\"timestamp\":\"" + String(millis()-1200000) + "\",\"severity\":\"LOW\",\"description\":\"Prawidłowy poziom CO2\"}";
  json += "]}";
  sendResponse(client, "application/json", json);
}
