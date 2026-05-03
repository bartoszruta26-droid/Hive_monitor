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

// Serwer HTTP - poprawiona obsługa klienta
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
    
    if (request.indexOf("/api/data") >= 0) {
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
    } else if (request.indexOf("/") >= 0) {
      // Strona główna - podstawowe info
      ethClient.println("HTTP/1.1 200 OK");
      ethClient.println("Content-Type: text/html");
      ethClient.println("Connection: close");
      ethClient.println();
      ethClient.println("<!DOCTYPE html><html><head><title>ApiaryGuard</title></head><body>");
      ethClient.println("<h1>ApiaryGuard - Raspberry Pi Pico</h1>");
      ethClient.print("<p>Status: ");
      ethClient.print(Ethernet.linkStatus() == LinkON ? "Połączony" : "Rozłączony");
      ethClient.println("</p>");
      ethClient.print("<p>IP: ");
      ethClient.print(Ethernet.localIP());
      ethClient.println("</p>");
      ethClient.println("<p>Endpoints:</p>");
      ethClient.println("<ul>");
      ethClient.println("<li><a href='/api/data'>GET /api/data</a> - Dane z sensorów</li>");
      ethClient.println("<li><a href='/api/aq'>GET /api/aq</a> - Jakość powietrza (24+ parametry)</li>");
      ethClient.println("<li>GET /api/cmd?SET_RELAYS:X - Ustaw przekaźniki</li>");
      ethClient.println("<li>GET /api/cmd?GET_AQ_STATUS - Status jakości powietrza</li>");
      ethClient.println("</ul>");
      ethClient.println("</body></html>");
      delay(1);
      ethClient.stop();
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
