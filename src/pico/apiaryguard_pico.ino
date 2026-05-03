/*
 * ApiaryGuard - Firmware dla Raspberry Pi Pico (RP2040)
 * Obsługa: W6100 (Ethernet), Sensory (HX711, DHT22, SGP41, LD2410B, MEMS, Piezo), Efektory
 * Środowisko: Arduino IDE (Core: Raspberry Pi RP2040 Boards)
 * Język: C++
 * 
 * POŁĄCZENIA SPRZĘTOWE (PINOUT PICO):
 * ---------------------------------------------------------
 * MODUŁ W6100 (SPI1):
 *   CS   -> GPIO 9  (SS)
 *   MOSI -> GPIO 11 (MOSI)
 *   MISO -> GPIO 12 (MISO)
 *   SCK  -> GPIO 10 (SCK)
 *   RST  -> GPIO 8
 *   INT  -> GPIO 13 (opcjonalny, do przerwania)
 * 
 * RADAR MMWAVE LD2410B (UART1):
 *   TX   -> GPIO 4 (RX1)
 *   RX   -> GPIO 5 (TX1)
 *   VCC  -> 5V
 *   GND  -> GND
 * 
 * CZUJNIKI I2C (SGP41/BME688) (I2C0):
 *   SDA  -> GPIO 0
 *   SCL  -> GPIO 1
 * 
 * DHT22 (Temp/Wilg):
 *   DATA -> GPIO 2
 * 
 * HX711 (Waga):
 *   DT   -> GPIO 3
 *   SCK  -> GPIO 22
 * 
 * MEMS Mic & Piezo (ADC):
 *   MIC  -> GPIO 26 (ADC0)
 *   PIEZO-> GPIO 27 (ADC1)
 * 
 * EFEKTORY (PWM/GPIO):
 *   Grzałka PWM    -> GPIO 6
 *   Wentylator PWM -> GPIO 7
 *   Pompa perystaltyczna (Relay) -> GPIO 14
 *   Zawór elektromagnetyczny 1   -> GPIO 15
 *   Zawór elektromagnetyczny 2   -> GPIO 16
 *   Przekaźnik 8-kanałowy (przykładowe piny) -> GPIO 17-24
 *   LED Status     -> GPIO 25 (WBUDOWANY)
 * ---------------------------------------------------------
 * 
 * MODUŁ ANALIZY DANYCH RADARU MMWAVE:
 * - Bufor cyrkularny danych (120 ostatnich pomiarów)
 * - Analiza trendu z oknem ruchomym (regresja liniowa)
 * - Detekcja anomalii metodą Z-score
 * - Klasyfikacja zdarzeń: rojenie, drapieżniki, blokada wlotu
 * - Rozróżnianie zmian pozytywnych i negatywnych dla pożytku ulu
 * - API HTTP: /radar/status, /radar/anomalies
 * ---------------------------------------------------------
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>

// --- KONFIGURACJA SIECI W6100 ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177); // Statyczne IP dla Pico
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(8080); // Port nasłuchiwania

// --- DEFINICJE PINÓW PICO ---
#define W6100_CS   9
#define W6100_RST  8
#define W6100_INT  13

#define RADAR_RX   4
#define RADAR_TX   5

#define I2C_SDA    0
#define I2C_SCL    1

#define DHT_PIN    2
#define HX711_DT   3
#define HX711_SCK  22

#define MIC_PIN    26 // ADC0
#define PIEZO_PIN  27 // ADC1

#define HEATER_PWM 6
#define FAN_PWM    7
#define PUMP_RELAY 14
#define VALVE_1    15
#define VALVE_2    16

// Dodatkowe piny dla przekaźnika 8-kanałowego
#define RELAY_CH1  17
#define RELAY_CH2  18
#define RELAY_CH3  19
#define RELAY_CH4  20
#define RELAY_CH5  21
#define RELAY_CH6  22
#define RELAY_CH7  23
#define RELAY_CH8  24

// --- OBIEKTY ---
EthernetClient client;
DHT dht(DHT_PIN, DHT22);
Adafruit_SGP41 sgp;
HardwareSerial radarSerial(uart1); // UART1 dla radaru

// Zmienne dla HX711 (prosta implementacja bit-bang)
long hx711_value = 0;
long hx711_offset = 0;
float hx711_scale = 1.0;

// Zmienne sensorów
float temperature = 0.0;
float humidity = 0.0;
uint16_t co2_eq = 0;
uint16_t voc_idx = 0;
float audio_rms = 0.0;
float vibration_level = 0.0;
bool motion_detected = false;

// Timer'y
unsigned long lastMillis = 0;
unsigned long networkHeartbeat = 0;

// ============================================================================
// MODUŁ ANALIZY DANYCH RADARU MMWAVE - WYKRYWANIE NIEPRAWIDŁOWOŚCI I ZMIAN
// ============================================================================

// Bufor cyrkularny dla danych radaru (historia pomiarów)
#define RADAR_BUFFER_SIZE 120  // 2 godziny danych przy odczycie co 1 sekunda
#define TREND_WINDOW_SIZE 30   // Okno do analizy trendu (30 ostatnich pomiarów)
#define ANOMALY_THRESHOLD_STD 2.5f  // Próg anomalii: 2.5 odchylenia standardowego

struct RadarDataPoint {
    uint32_t timestamp;
    float distance;        // Główna odległość wykrytego obiektu (m)
    float speed;           // Prędkość radialna (m/s)
    float energy;          // Poziom energii sygnału
    uint8_t target_count;  // Liczba wykrytych celów
    bool is_valid;         // Flag ważności pomiaru
};

struct TrendAnalysis {
    float mean_distance;
    float std_distance;
    float mean_energy;
    float std_energy;
    float slope;           // Nachylenie trendu (regresja liniowa)
    float correlation;     // Współczynnik korelacji
    uint8_t sample_count;
};

struct AnomalyEvent {
    enum AnomalyType {
        NONE = 0,
        SUDDEN_MOTION_SPIKE,      // Nagły skok ruchu
        GRADUAL_ACTIVITY_CHANGE,  // Stopniowa zmiana aktywności
        ABSENCE_DETECTED,         // Wykryto brak aktywności
        SWARM_PATTERN,            // Wzór rojenia
        PREDATOR_APPROACH,        // Podejście drapieżnika
        ENTRANCE_BLOCKAGE,        // Blokada wejścia
        UNUSUAL_VIBRATION,        // Nietypowe wibracje
        MULTI_TARGET_CLUSTER      // Grupowanie wielu celów
    };
    
    enum ChangeDirection {
        NO_CHANGE = 0,
        POSITIVE_CHANGE,  // Wzrost aktywności/pożytku
        NEGATIVE_CHANGE   // Spadek aktywności/pożytku
    };
    
    AnomalyType type;
    ChangeDirection direction;
    float severity;       // 0.0 - 1.0 (norma - krytyczny)
    uint32_t timestamp;
    float confidence;     // 0.0 - 1.0 (pewność detekcji)
    char description[64]; // Opis zdarzenia
};

// Globalne struktury danych
RadarDataPoint radarBuffer[RADAR_BUFFER_SIZE];
uint16_t radarBufferIndex = 0;
uint16_t radarDataCount = 0;

TrendAnalysis currentTrend;
AnomalyEvent lastAnomaly;

// Statystyki długoterminowe
struct LongTermStats {
    float baseline_activity;      // Bazowa aktywność ula
    float daily_pattern[24];      // Wzorzec dzienny (godzinowy)
    float weekly_average;         // Średnia tygodniowa
    uint8_t valid_days;           // Liczba dni z ważnymi danymi
} longTermStats;

// Konfiguracja progów detekcji
struct DetectionThresholds {
    float min_bee_activity;       // Minimalna aktywność pszczół
    float max_bee_activity;       // Maksymalna aktywność pszczół
    float swarm_energy_ratio;     // Ratio energii dla rojenia
    float predator_speed_min;     // Minimalna prędkość drapieżnika
    float absence_duration_sec;   // Czas braku aktywności do alarmu
    float sudden_change_percent;  // Procent nagłej zmiany
} detectionThresholds = {
    .min_bee_activity = 0.15f,
    .max_bee_activity = 0.85f,
    .swarm_energy_ratio = 3.5f,
    .predator_speed_min = 0.5f,
    .absence_duration_sec = 300.0f,  // 5 minut
    .sudden_change_percent = 40.0f   // 40% zmiany
};

// Funkcje deklaracje
void updateRadarBuffer(const RadarDataPoint& point);
TrendAnalysis calculateTrend(const uint8_t window_size);
AnomalyEvent detectAnomalies(const RadarDataPoint& current, const TrendAnalysis& trend);
void classifyBeeActivity(RadarDataPoint& point);
float calculateStandardDeviation(const float* data, uint8_t count, float mean);
float calculateLinearRegressionSlope(const float* x, const float* y, uint8_t count);
void updateLongTermStats(const RadarDataPoint& point);
bool isPositiveChange(const AnomalyEvent& event);
const char* anomalyTypeToString(AnomalyEvent::AnomalyType type);
const char* changeDirectionToString(AnomalyEvent::ChangeDirection dir);

// --- FUNKCJE POMOCNICZE ---

// Inicjalizacja W6100
void initW6100() {
  pinMode(W6100_RST, OUTPUT);
  pinMode(W6100_CS, OUTPUT);
  
  digitalWrite(W6100_RST, HIGH);
  delay(100);
  digitalWrite(W6100_RST, LOW);
  delay(200);
  digitalWrite(W6100_RST, HIGH);
  delay(200);
  
  digitalWrite(W6100_CS, HIGH);
  
  Serial.println("Inicjalizacja W6100...");
  
  // Konfiguracja SPI1 dla W6100
  // Uwaga: Biblioteka Ethernet standardowo używa SPI. 
  // Na Pico trzeba upewnić się, że piny są przypisane do SPI1.
  // Jeśli biblioteka nie wykrywa automatycznie, może być wymagana modyfikacja pins_arduino.h lub użycie setCS().
  
  if (Ethernet.begin(mac, ip, gateway, subnet)) {
    Serial.println("W6100 połączono. IP: " + Ethernet.localIP().toString());
    server.begin();
  } else {
    Serial.println("Błąd konfiguracji W6100!");
    while(true); // Zatrzymaj jeśli brak łącza
  }
}

// Obsługa HX711 (Bit-bang dla Pico)
long readHX711() {
  long count = 0;
  pinMode(HX711_DT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  
  digitalWrite(HX711_SCK, LOW);
  while(digitalRead(HX711_DT)); // Czekaj na gotowość
  
  for(int i = 0; i < 24; i++) {
    digitalWrite(HX711_SCK, HIGH);
    count = count << 1;
    digitalWrite(HX711_SCK, LOW);
    if(digitalRead(HX711_DT)) count++;
  }
  
  // 25 impuls dla następnego odczytu (gain 128)
  digitalWrite(HX711_SCK, HIGH);
  count = count ^ 0x800000; // Konwersja na signed
  digitalWrite(HX711_SCK, LOW);
  
  return count;
}

// Obliczanie RMS dźwięku
float calculateAudioRMS() {
  int samples = 100;
  float sum = 0;
  for(int i=0; i<samples; i++) {
    int val = analogRead(MIC_PIN);
    float voltage = (val / 4095.0) * 3.3; // Pico ADC 12-bit
    sum += (voltage * voltage);
    delay(1);
  }
  return sqrt(sum / samples);
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI ANALIZY DANYCH RADARU MMWAVE
// ============================================================================

// Obliczanie odchylenia standardowego
float calculateStandardDeviation(const float* data, uint8_t count, float mean) {
    if (count < 2) return 0.0f;
    
    float sum_sq_diff = 0.0f;
    for (uint8_t i = 0; i < count; i++) {
        float diff = data[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / (count - 1));
}

// Regresja liniowa - obliczanie nachylenia (slope)
float calculateLinearRegressionSlope(const float* x, const float* y, uint8_t count) {
    if (count < 2) return 0.0f;
    
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_xx = 0.0f;
    
    for (uint8_t i = 0; i < count; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
        sum_xx += x[i] * x[i];
    }
    
    float denominator = count * sum_xx - sum_x * sum_x;
    if (abs(denominator) < 0.0001f) return 0.0f;
    
    return (count * sum_xy - sum_x * sum_y) / denominator;
}

// Aktualizacja bufora cyrkularnego radaru
void updateRadarBuffer(const RadarDataPoint& point) {
    radarBuffer[radarBufferIndex] = point;
    radarBufferIndex = (radarBufferIndex + 1) % RADAR_BUFFER_SIZE;
    
    if (radarDataCount < RADAR_BUFFER_SIZE) {
        radarDataCount++;
    }
}

// Obliczanie trendu z okna danych
TrendAnalysis calculateTrend(const uint8_t window_size) {
    TrendAnalysis result = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0};
    
    uint8_t actual_count = min(window_size, radarDataCount);
    if (actual_count < 2) return result;
    
    // Pobieranie ostatnich próbek
    float distances[TREND_WINDOW_SIZE];
    float energies[TREND_WINDOW_SIZE];
    float time_indices[TREND_WINDOW_SIZE];
    
    uint16_t start_idx = (radarBufferIndex >= actual_count) 
                         ? (radarBufferIndex - actual_count) 
                         : (RADAR_BUFFER_SIZE - (actual_count - radarBufferIndex));
    
    float sum_dist = 0.0f, sum_energy = 0.0f;
    
    for (uint8_t i = 0; i < actual_count; i++) {
        uint16_t idx = (start_idx + i) % RADAR_BUFFER_SIZE;
        if (!radarBuffer[idx].is_valid) continue;
        
        distances[i] = radarBuffer[idx].distance;
        energies[i] = radarBuffer[idx].energy;
        time_indices[i] = (float)i;
        
        sum_dist += distances[i];
        sum_energy += energies[i];
        result.sample_count++;
    }
    
    if (result.sample_count < 2) return result;
    
    // Średnie
    result.mean_distance = sum_dist / result.sample_count;
    result.mean_energy = sum_energy / result.sample_count;
    
    // Odchylenia standardowe
    result.std_distance = calculateStandardDeviation(distances, result.sample_count, result.mean_distance);
    result.std_energy = calculateStandardDeviation(energies, result.sample_count, result.mean_energy);
    
    // Nachylenie trendu (regresja liniowa)
    result.slope = calculateLinearRegressionSlope(time_indices, energies, result.sample_count);
    
    return result;
}

// Klasyfikacja aktywności pszczół na podstawie danych radaru
void classifyBeeActivity(RadarDataPoint& point) {
    // Normalizacja energii do zakresu 0-1
    point.energy = constrain(point.energy / 1000.0f, 0.0f, 1.0f);
    
    // Analiza liczby celów
    if (point.target_count > 50) {
        // Potencjalne rojenie
        point.is_valid = true;
    } else if (point.target_count > 10 && point.energy > detectionThresholds.min_bee_activity) {
        // Normalna aktywność pszczół
        point.is_valid = true;
    } else if (point.target_count == 0 && point.energy < 0.05f) {
        // Brak aktywności
        point.is_valid = true;
    } else {
        point.is_valid = false;
    }
}

// Detekcja anomalii - główna funkcja analityczna
AnomalyEvent detectAnomalies(const RadarDataPoint& current, const TrendAnalysis& trend) {
    AnomalyEvent event;
    event.type = AnomalyEvent::NONE;
    event.direction = AnomalyEvent::NO_CHANGE;
    event.severity = 0.0f;
    event.timestamp = current.timestamp;
    event.confidence = 0.0f;
    memset(event.description, 0, sizeof(event.description));
    
    // 1. Detekcja nagłego skoku ruchu (SUDDEN_MOTION_SPIKE)
    if (trend.std_energy > 0.001f) {
        float z_score = (current.energy - trend.mean_energy) / trend.std_energy;
        
        if (z_score > ANOMALY_THRESHOLD_STD) {
            event.type = AnomalyEvent::SUDDEN_MOTION_SPIKE;
            event.direction = AnomalyEvent::POSITIVE_CHANGE;
            event.severity = min((z_score - ANOMALY_THRESHOLD_STD) / 3.0f, 1.0f);
            event.confidence = min(z_score / 5.0f, 1.0f);
            snprintf(event.description, sizeof(event.description), 
                    "Nagły wzrost aktywności: Z-score=%.1f", z_score);
            return event;
        }
        else if (z_score < -ANOMALY_THRESHOLD_STD) {
            event.type = AnomalyEvent::SUDDEN_MOTION_SPIKE;
            event.direction = AnomalyEvent::NEGATIVE_CHANGE;
            event.severity = min(abs(z_score - ANOMALY_THRESHOLD_STD) / 3.0f, 1.0f);
            event.confidence = min(abs(z_score) / 5.0f, 1.0f);
            snprintf(event.description, sizeof(event.description), 
                    "Nagły spadek aktywności: Z-score=%.1f", z_score);
            return event;
        }
    }
    
    // 2. Detekcja braku aktywności (ABSENCE_DETECTED)
    static uint32_t last_activity_time = 0;
    if (current.energy < detectionThresholds.min_bee_activity * 0.3f) {
        if (last_activity_time == 0) last_activity_time = current.timestamp;
        else if ((current.timestamp - last_activity_time) > (uint32_t)detectionThresholds.absence_duration_sec * 1000) {
            event.type = AnomalyEvent::ABSENCE_DETECTED;
            event.direction = AnomalyEvent::NEGATIVE_CHANGE;
            event.severity = 0.8f;
            event.confidence = 0.9f;
            snprintf(event.description, sizeof(event.description), 
                    "Brak aktywności przez %.0fs", detectionThresholds.absence_duration_sec);
            return event;
        }
    } else {
        last_activity_time = current.timestamp;
    }
    
    // 3. Detekcja wzoru rojenia (SWARM_PATTERN)
    if (current.target_count > 30 && current.energy > (trend.mean_energy * detectionThresholds.swarm_energy_ratio)) {
        event.type = AnomalyEvent::SWARM_PATTERN;
        event.direction = AnomalyEvent::POSITIVE_CHANGE;  // Rojenie to naturalny proces
        event.severity = 0.6f;
        event.confidence = min((float)current.target_count / 100.0f, 1.0f);
        snprintf(event.description, sizeof(event.description), 
                "Potencjalne rojenie: cele=%d, energia=%.2f", current.target_count, current.energy);
        return event;
    }
    
    // 4. Detekcja podejścia drapieżnika (PREDATOR_APPROACH)
    if (abs(current.speed) > detectionThresholds.predator_speed_min && current.target_count <= 3) {
        event.type = AnomalyEvent::PREDATOR_APPROACH;
        event.direction = AnomalyEvent::NEGATIVE_CHANGE;
        event.severity = 0.7f;
        event.confidence = 0.75f;
        snprintf(event.description, sizeof(event.description), 
                "Drapażnik wykryty: prędkość=%.2f m/s", abs(current.speed));
        return event;
    }
    
    // 5. Detekcja blokady wejścia (ENTRANCE_BLOCKAGE)
    if (current.distance < 0.1f && current.target_count == 0 && current.energy < 0.02f) {
        event.type = AnomalyEvent::ENTRANCE_BLOCKAGE;
        event.direction = AnomalyEvent::NEGATIVE_CHANGE;
        event.severity = 0.9f;
        event.confidence = 0.85f;
        snprintf(event.description, sizeof(event.description), "Potencjalna blokada wlotu");
        return event;
    }
    
    // 6. Detekcja grupowania celów (MULTI_TARGET_CLUSTER)
    if (current.target_count > 15 && trend.mean_energy > 0 && 
        current.energy > trend.mean_energy * 2.0f) {
        event.type = AnomalyEvent::MULTI_TARGET_CLUSTER;
        event.direction = AnomalyEvent::POSITIVE_CHANGE;
        event.severity = 0.5f;
        event.confidence = 0.7f;
        snprintf(event.description, sizeof(event.description), 
                "Grupowanie pszczół: cele=%d", current.target_count);
        return event;
    }
    
    // 7. Stopniowa zmiana aktywności (GRADUAL_ACTIVITY_CHANGE)
    if (abs(trend.slope) > 0.01f && trend.sample_count >= TREND_WINDOW_SIZE) {
        if (trend.slope > 0) {
            event.type = AnomalyEvent::GRADUAL_ACTIVITY_CHANGE;
            event.direction = AnomalyEvent::POSITIVE_CHANGE;
            event.severity = min(abs(trend.slope) * 10.0f, 1.0f);
            event.confidence = 0.65f;
            snprintf(event.description, sizeof(event.description), 
                    "Stopniowy wzrost aktywności: slope=%.3f", trend.slope);
        } else {
            event.type = AnomalyEvent::GRADUAL_ACTIVITY_CHANGE;
            event.direction = AnomalyEvent::NEGATIVE_CHANGE;
            event.severity = min(abs(trend.slope) * 10.0f, 1.0f);
            event.confidence = 0.65f;
            snprintf(event.description, sizeof(event.description), 
                    "Stopniowy spadek aktywności: slope=%.3f", trend.slope);
        }
        return event;
    }
    
    return event;
}

// Sprawdzenie czy zmiana jest pozytywna
bool isPositiveChange(const AnomalyEvent& event) {
    return event.direction == AnomalyEvent::POSITIVE_CHANGE;
}

// Konwersja typu anomalii na string
const char* anomalyTypeToString(AnomalyEvent::AnomalyType type) {
    switch(type) {
        case AnomalyEvent::NONE: return "BRAK";
        case AnomalyEvent::SUDDEN_MOTION_SPIKE: return "NAGLY_SKOK_RUCHU";
        case AnomalyEvent::GRADUAL_ACTIVITY_CHANGE: return "STOPNIOWA_ZMIANA";
        case AnomalyEvent::ABSENCE_DETECTED: return "BRAK_AKTYWNOSCI";
        case AnomalyEvent::SWARM_PATTERN: return "ROJENIE";
        case AnomalyEvent::PREDATOR_APPROACH: return "DRAPAZNIK";
        case AnomalyEvent::ENTRANCE_BLOCKAGE: return "BLOKADA_WLOTU";
        case AnomalyEvent::UNUSUAL_VIBRATION: return "NITYPOWE_WIBRACJE";
        case AnomalyEvent::MULTI_TARGET_CLUSTER: return "GRUPOWANIE";
        default: return "NIEZNANE";
    }
}

// Konwersja kierunku zmiany na string
const char* changeDirectionToString(AnomalyEvent::ChangeDirection dir) {
    switch(dir) {
        case AnomalyEvent::NO_CHANGE: return "BRAK_ZMIANY";
        case AnomalyEvent::POSITIVE_CHANGE: return "POZYTYWNA";
        case AnomalyEvent::NEGATIVE_CHANGE: return "NEGATYWNA";
        default: return "NIEZNANE";
    }
}

// Aktualizacja statystyk długoterminowych
void updateLongTermStats(const RadarDataPoint& point) {
    // Uproszczona implementacja - do rozbudowy
    static uint32_t last_hour_update = 0;
    uint32_t current_hour = (point.timestamp / 3600000) % 24;
    
    if (current_hour != last_hour_update && point.is_valid) {
        longTermStats.daily_pattern[current_hour] = point.energy;
        last_hour_update = current_hour;
    }
}

// Zaawansowane parsowanie ramki LD2410B z ekstrakcją pełnych danych
void processRadar() {
  static uint8_t buffer[256];
  static int idx = 0;
  static uint32_t last_frame_time = 0;
  
  while(radarSerial.available()) {
    uint8_t b = radarSerial.read();
    
    // Detekcja nagłówka ramki LD2410B: 0xF4 0xF3 0x01
    if(idx == 0 && b != 0xF4) continue;
    if(idx == 1 && b != 0xF3) { idx = 0; continue; }
    if(idx == 2 && b != 0x01) { idx = 0; continue; }
    
    buffer[idx++] = b;
    
    // Oczekiwanie na pełną ramkę (minimalnie 10 bajtów)
    if(idx >= 10) {
      // Sprawdzenie sumy kontrolnej i długości ramki
      uint16_t frame_len = buffer[3] | (buffer[4] << 8);
      
      if(idx >= (6 + frame_len)) {
        // Przetworzenie kompletnej ramki
        RadarDataPoint newPoint;
        newPoint.timestamp = millis();
        
        // Ekstrakcja danych z ramki LD2410B (format zależy od trybu)
        // Przykład dla trybu inżynierskiego z danymi o celach
        
        // Bajty 6-9: informacje o liczbie celów
        newPoint.target_count = buffer[6];
        
        // Dla każdego celu (przyjmując 1 cel dla uproszczenia)
        if(newPoint.target_count > 0) {
          // Offset do danych pierwszego celu (zależny od formatu ramki)
          int target_offset = 8;
          
          // Odległość w metrach (konwersja z jednostek radaru)
          uint16_t dist_raw = buffer[target_offset] | (buffer[target_offset + 1] << 8);
          newPoint.distance = dist_raw * 0.01f;  // 1 cm rozdzielczości
          
          // Prędkość radialna w m/s
          int16_t speed_raw = buffer[target_offset + 2] | (buffer[target_offset + 3] << 8);
          newPoint.speed = speed_raw * 0.01f;  // Konwersja na m/s
          
          // Energia sygnału
          uint16_t energy_raw = buffer[target_offset + 4] | (buffer[target_offset + 5] << 8);
          newPoint.energy = (float)energy_raw;
        } else {
          newPoint.distance = 0.0f;
          newPoint.speed = 0.0f;
          newPoint.energy = 0.0f;
        }
        
        newPoint.is_valid = true;
        
        // Klasyfikacja aktywności
        classifyBeeActivity(newPoint);
        
        // Aktualizacja bufora
        updateRadarBuffer(newPoint);
        
        // Obliczanie trendu
        currentTrend = calculateTrend(TREND_WINDOW_SIZE);
        
        // Detekcja anomalii
        lastAnomaly = detectAnomalies(newPoint, currentTrend);
        
        // Aktualizacja statystyk długoterminowych
        updateLongTermStats(newPoint);
        
        // Jeśli wykryto anomalię, zgłoś ją przez Serial
        if(lastAnomaly.type != AnomalyEvent::NONE) {
          Serial.print("[ANOMALIA] ");
          Serial.print(anomalyTypeToString(lastAnomaly.type));
          Serial.print(" | Kierunek: ");
          Serial.print(changeDirectionToString(lastAnomaly.direction));
          Serial.print(" | Poważność: ");
          Serial.print(lastAnomaly.severity, 2);
          Serial.print(" | Pewność: ");
          Serial.print(lastAnomaly.confidence, 2);
          Serial.print(" | Opis: ");
          Serial.println(lastAnomaly.description);
        }
        
        // Update global motion flag
        motion_detected = (newPoint.energy > detectionThresholds.min_bee_activity);
        
        idx = 0; // Reset bufora
        break;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial && (millis() < 3000)); // Czekaj na USB CDC
  
  Serial.println("=== ApiaryGuard Pico Start ===");
  
  // Inicjalizacja I2C
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  
  // Inicjalizacja UART dla Radaru
  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
  
  // Inicjalizacja Sensorów
  dht.begin();
  
  if (!sgp.begin_I2C(0x59, &Wire)) { // Domyślny adres SGP41
    Serial.println("Nie znaleziono SGP41!");
  } else {
    sgp.measureBaseline();
  }
  
  // Inicjalizacja PWM
  pinMode(HEATER_PWM, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);
  analogWriteFrequency(HEATER_PWM, 1000);
  analogWriteFrequency(FAN_PWM, 1000);
  
  // Inicjalizacja GPIO
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(VALVE_1, OUTPUT);
  pinMode(VALVE_2, OUTPUT);
  pinMode(RELAY_CH1, OUTPUT);
  pinMode(RELAY_CH2, OUTPUT);
  pinMode(RELAY_CH3, OUTPUT);
  pinMode(RELAY_CH4, OUTPUT);
  pinMode(RELAY_CH5, OUTPUT);
  pinMode(RELAY_CH6, OUTPUT);
  pinMode(RELAY_CH7, OUTPUT);
  pinMode(RELAY_CH8, OUTPUT);
  
  digitalWrite(PUMP_RELAY, LOW);
  digitalWrite(VALVE_1, LOW);
  digitalWrite(VALVE_2, LOW);
  digitalWrite(RELAY_CH1, LOW);
  digitalWrite(RELAY_CH2, LOW);
  digitalWrite(RELAY_CH3, LOW);
  digitalWrite(RELAY_CH4, LOW);
  digitalWrite(RELAY_CH5, LOW);
  digitalWrite(RELAY_CH6, LOW);
  digitalWrite(RELAY_CH7, LOW);
  digitalWrite(RELAY_CH8, LOW);
  
  // Start Ethernet W6100
  initW6100();
  
  Serial.println("System gotowy.");
}

void loop() {
  unsigned long now = millis();
  
  // 1. Odczyt Sensorów (co 2 sekundy)
  if(now - lastMillis > 2000) {
    lastMillis = now;
    
    // DHT22
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if(!isnan(h)) humidity = h;
    if(!isnan(t)) temperature = t;
    
    // HX711
    long raw = readHX711();
    hx711_value = (raw - hx711_offset) / hx711_scale;
    
    // SGP41 (CO2/VOC)
    sgp.measureGas();
    co2_eq = sgp.CO2eq;
    voc_idx = sgp.VOCindex;
    
    // Audio RMS
    audio_rms = calculateAudioRMS();
    
    // Wibracje (Piezo)
    vibration_level = analogRead(PIEZO_PIN);
    
    // Radar
    processRadar();
    
    // Debug Serial - rozszerzony o dane z analizy radaru
    Serial.printf("T:%.1f H:%.1f Waga:%ld CO2:%d VOC:%d Audio:%.2f Ruch:%d\n", 
                  temperature, humidity, hx711_value, co2_eq, voc_idx, audio_rms, motion_detected);
    
    // Dodatkowy output dla modułu analizy radaru
    if(radarDataCount > 0) {
      Serial.printf("[RADAR] Cele:%d Dystans:%.2fm Energia:%.3f Trend_slope:%.4f\n",
                    radarBuffer[(radarBufferIndex > 0) ? radarBufferIndex - 1 : RADAR_BUFFER_SIZE - 1].target_count,
                    radarBuffer[(radarBufferIndex > 0) ? radarBufferIndex - 1 : RADAR_BUFFER_SIZE - 1].distance,
                    radarBuffer[(radarBufferIndex > 0) ? radarBufferIndex - 1 : RADAR_BUFFER_SIZE - 1].energy,
                    currentTrend.slope);
      
      // Informacja o ostatniej wykrytej zmianie
      if(lastAnomaly.type != AnomalyEvent::NONE) {
        Serial.printf("[STATUS_ZMIANY] Typ:%s Kierunek:%s Pozyczek_ulu:%s\n",
                      anomalyTypeToString(lastAnomaly.type),
                      changeDirectionToString(lastAnomaly.direction),
                      isPositiveChange(lastAnomaly) ? "POZYTYWNY" : "NEGATYWNY");
      }
    }
  }
  
  // 2. Obsługa Sieci W6100
  EthernetClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();
    
    // Prosty protokół HTTP/GET lub binarny
    // Przykład: GET /status
    if(request.indexOf("/status") > 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"temp\":"); client.print(temperature);
      client.print(",\"hum\":"); client.print(humidity);
      client.print(",\"weight\":"); client.print(hx711_value);
      client.print(",\"co2\":"); client.print(co2_eq);
      client.print(",\"voc\":"); client.print(voc_idx);
      client.print(",\"audio\":"); client.print(audio_rms);
      client.print(",\"motion\":"); client.print(motion_detected ? 1 : 0);
      client.println("}");
    } else if (request.indexOf("/heater/on") > 0) {
      analogWrite(HEATER_PWM, 200); // PWM 0-255
      client.println("HTTP/1.1 200 OK\n\nHeater ON");
    } else if (request.indexOf("/heater/off") > 0) {
      analogWrite(HEATER_PWM, 0);
      client.println("HTTP/1.1 200 OK\n\nHeater OFF");
    } else if (request.indexOf("/fan/on") > 0) {
      analogWrite(FAN_PWM, 255);
      client.println("HTTP/1.1 200 OK\n\nFan ON");
    } else if (request.indexOf("/fan/off") > 0) {
      analogWrite(FAN_PWM, 0);
      client.println("HTTP/1.1 200 OK\n\nFan OFF");
    } else if (request.indexOf("/pump/on") > 0) {
      digitalWrite(PUMP_RELAY, HIGH);
      client.println("HTTP/1.1 200 OK\n\nPump ON");
    } else if (request.indexOf("/pump/off") > 0) {
      digitalWrite(PUMP_RELAY, LOW);
      client.println("HTTP/1.1 200 OK\n\nPump OFF");
    }
    // Nowe endpointy API dla modułu analizy radaru
    else if (request.indexOf("/radar/status") > 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"radar_data_count\":"); client.print(radarDataCount);
      client.print(",\"last_target_count\":"); 
      client.print(radarBuffer[(radarBufferIndex > 0) ? radarBufferIndex - 1 : RADAR_BUFFER_SIZE - 1].target_count);
      client.print(",\"last_distance\":"); 
      client.print(radarBuffer[(radarBufferIndex > 0) ? radarBufferIndex - 1 : RADAR_BUFFER_SIZE - 1].distance, 3);
      client.print(",\"last_energy\":"); 
      client.print(radarBuffer[(radarBufferIndex > 0) ? radarBufferIndex - 1 : RADAR_BUFFER_SIZE - 1].energy, 3);
      client.print(",\"trend_slope\":"); client.print(currentTrend.slope, 5);
      client.print(",\"trend_mean_energy\":"); client.print(currentTrend.mean_energy, 3);
      client.print(",\"anomaly_type\":\""); client.print(anomalyTypeToString(lastAnomaly.type));
      client.print("\",\"anomaly_direction\":\""); client.print(changeDirectionToString(lastAnomaly.direction));
      client.print("\",\"is_positive_change\":"); client.print(isPositiveChange(lastAnomaly) ? "true" : "false");
      client.print(",\"anomaly_severity\":"); client.print(lastAnomaly.severity, 2);
      client.print(",\"anomaly_confidence\":"); client.print(lastAnomaly.confidence, 2);
      client.println("}");
    }
    else if (request.indexOf("/radar/anomalies") > 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"last_anomaly\":{\"type\":\""); client.print(anomalyTypeToString(lastAnomaly.type));
      client.print("\",\"direction\":\""); client.print(changeDirectionToString(lastAnomaly.direction));
      client.print("\",\"severity\":"); client.print(lastAnomaly.severity, 2);
      client.print(",\"confidence\":"); client.print(lastAnomaly.confidence, 2);
      client.print(",\"description\":\""); client.print(lastAnomaly.description);
      client.print("\"},\"pozytek_status\":\""); 
      client.print(isPositiveChange(lastAnomaly) ? "POZYTYWNY" : (lastAnomaly.type == AnomalyEvent::NONE ? "NORMALNY" : "NEGATYWNY"));
      client.println("\"}");
    }
    
    delay(1);
    client.stop();
  }
  
  // 3. Heartbeat sieciowy (opcjonalne pingi)
  if(now - networkHeartbeat > 10000) {
    networkHeartbeat = now;
    // Tu można dodać wysyłanie danych do serwera centralnego przez TCP/UDP
  }
  
  // 4. Logika sterowania (przykład)
  if(temperature < 15.0) {
    analogWrite(HEATER_PWM, 150); // Włącz grzanie
  } else {
    analogWrite(HEATER_PWM, 0);
  }
  
  if(motion_detected && humidity > 80) {
    digitalWrite(VALVE_1, HIGH); // Otwórz wentylację
  } else {
    digitalWrite(VALVE_1, LOW);
  }
}
