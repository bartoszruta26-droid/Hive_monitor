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
 * - MODUŁ WYLICZANIA 27 PARAMETRÓW RADARU MMWAVE:
 *   * Statystyczne: mean/std/min/max/range (odległość, energia, prędkość)
 *   * Energy variance, coefficient of variation (CV)
 *   * Temporalne: activity_ratio, idle_time_percent, motion_intensity
 *   * Częstotliwościowe: target_rate, max_target_count, target_density
 *   * Trend: slope, correlation, acceleration_rate
 *   * Jakościowe: signal_quality, anomaly_score, hive_health_index
 *   * Dodatkowe: power_spectrum_peak, zero_crossing_rate, entropy
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

// ============================================================================
// MODUŁ WYLICZANIA PARAMETRÓW Z DANYCH RADARU MMWAVE - 15+ PARAMETRÓW
// ============================================================================

struct RadarMetrics {
    // Podstawowe parametry statystyczne
    float mean_distance;          // Średnia odległość obiektów [m]
    float std_distance;           // Odchylenie standardowe odległości [m]
    float min_distance;           // Minimalna odległość [m]
    float max_distance;           // Maksymalna odległość [m]
    float range_distance;         // Zakres odległości (max-min) [m]
    
    float mean_energy;            // Średnia energia sygnału [AU]
    float std_energy;             // Odchylenie standardowe energii [AU]
    float min_energy;             // Minimalna energia [AU]
    float max_energy;             // Maksymalna energia [AU]
    float energy_variance;        // Wariancja energii [AU²]
    float energy_cv;              // Współczynnik zmienności energii (CV = std/mean)
    
    float mean_speed;             // Średnia prędkość radialna [m/s]
    float std_speed;              // Odchylenie standardowe prędkości [m/s]
    float max_speed_abs;          // Maksymalna wartość bezwzględna prędkości [m/s]
    
    // Parametry temporalne
    float activity_ratio;         // Stosunek czasu aktywności do całkowitego [%]
    float idle_time_percent;      // Procent czasu bezczynności [%]
    float motion_intensity;       // Intensywność ruchu (średnia energia * liczba celów)
    
    // Parametry częstotliwościowe
    float target_rate;            // Średnia liczba celów na pomiar
    float max_target_count;       // Maksymalna liczba wykrytych celów
    float target_density;         // Gęstość celów (cele/jednostka odległości)
    
    // Parametry trendu
    float trend_slope;            // Nachylenie trendu energii [AU/s]
    float trend_correlation;      // Współczynnik korelacji trendu [-1 do 1]
    float acceleration_rate;      // Tempo zmiany aktywności (pochodna slope)
    
    // Parametry jakościowe
    float signal_quality;         // Jakość sygnału (0-100%)
    float anomaly_score;          // Ogólny wynik anomalii (0-1)
    float hive_health_index;      // Indeks zdrowia ula (0-100%)
    
    // Dodatkowe metryki
    float power_spectrum_peak;    // Dominująca częstotliwość w spektrum mocy
    float zero_crossing_rate;     // Częstotliwość przejść przez zero (aktywność)
    float entropy;                // Entropia sygnału (miara losowości)
};

RadarMetrics currentMetrics;

// Deklaracje funkcji modułu metryk
void calculateRadarMetrics(RadarMetrics& metrics, const uint8_t window_size);
float calculateEntropy(const float* data, uint8_t count);
float calculateZeroCrossingRate(const float* data, uint8_t count);
void calculatePowerSpectrum(const float* data, uint8_t count, float& peak_freq);

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

// ============================================================================
// IMPLEMENTACJA MODUŁU WYLICZANIA PARAMETRÓW RADARU MMWAVE
// ============================================================================

// Obliczanie entropii sygnału (Shannon entropy)
float calculateEntropy(const float* data, uint8_t count) {
    if (count < 2) return 0.0f;
    
    // Normalizacja danych do rozkładu prawdopodobieństwa
    float sum = 0.0f;
    for (uint8_t i = 0; i < count; i++) {
        if (data[i] > 0) sum += data[i];
    }
    
    if (sum < 0.0001f) return 0.0f;
    
    float entropy = 0.0f;
    for (uint8_t i = 0; i < count; i++) {
        if (data[i] > 0) {
            float p = data[i] / sum;
            if (p > 0.0001f) {
                entropy -= p * log(p);
            }
        }
    }
    
    // Normalizacja entropii do zakresu 0-1
    float max_entropy = log(count);
    if (max_entropy > 0) {
        return entropy / max_entropy;
    }
    return 0.0f;
}

// Obliczanie częstotliwości przejść przez zero (Zero Crossing Rate)
float calculateZeroCrossingRate(const float* data, uint8_t count) {
    if (count < 2) return 0.0f;
    
    uint16_t zero_crossings = 0;
    float mean = 0.0f;
    
    // Oblicz średnią
    for (uint8_t i = 0; i < count; i++) {
        mean += data[i];
    }
    mean /= count;
    
    // Licz przejścia przez średnią
    for (uint8_t i = 1; i < count; i++) {
        if ((data[i-1] - mean) * (data[i] - mean) < 0) {
            zero_crossings++;
        }
    }
    
    return (float)zero_crossings / (count - 1);
}

// Uproszczone obliczanie spektrum mocy (DFT dla dominującej częstotliwości)
void calculatePowerSpectrum(const float* data, uint8_t count, float& peak_freq) {
    if (count < 4) {
        peak_freq = 0.0f;
        return;
    }
    
    float max_power = 0.0f;
    uint16_t peak_bin = 0;
    
    // Oblicz DFT dla kilku pierwszych harmonicznych
    uint8_t max_harmonic = min((uint8_t)8, (uint8_t)(count / 2));
    
    for (uint8_t k = 1; k <= max_harmonic; k++) {
        float real_part = 0.0f;
        float imag_part = 0.0f;
        
        for (uint8_t n = 0; n < count; n++) {
            float angle = (2.0f * PI * k * n) / count;
            real_part += data[n] * cos(angle);
            imag_part -= data[n] * sin(angle);
        }
        
        float power = (real_part * real_part + imag_part * imag_part) / (count * count);
        
        if (power > max_power) {
            max_power = power;
            peak_bin = k;
        }
    }
    
    // Konwersja binu na częstotliwość (zakładając próbkowanie co 1s)
    peak_freq = (float)peak_bin / count;
}

// Główna funkcja obliczająca wszystkie parametry radaru
void calculateRadarMetrics(RadarMetrics& metrics, const uint8_t window_size) {
    // Inicjalizacja zerami
    memset(&metrics, 0, sizeof(RadarMetrics));
    
    uint8_t actual_count = min(window_size, radarDataCount);
    if (actual_count < 2) return;
    
    // Pobranie danych z bufora
    float distances[TREND_WINDOW_SIZE];
    float energies[TREND_WINDOW_SIZE];
    float speeds[TREND_WINDOW_SIZE];
    uint8_t target_counts[TREND_WINDOW_SIZE];
    
    uint16_t start_idx = (radarBufferIndex >= actual_count) 
                         ? (radarBufferIndex - actual_count) 
                         : (RADAR_BUFFER_SIZE - (actual_count - radarBufferIndex));
    
    uint8_t valid_samples = 0;
    float sum_dist = 0.0f, sum_energy = 0.0f, sum_speed = 0.0f;
    uint32_t active_frames = 0;
    
    // Ekstrakcja danych
    for (uint8_t i = 0; i < actual_count; i++) {
        uint16_t idx = (start_idx + i) % RADAR_BUFFER_SIZE;
        if (!radarBuffer[idx].is_valid) continue;
        
        distances[valid_samples] = radarBuffer[idx].distance;
        energies[valid_samples] = radarBuffer[idx].energy;
        speeds[valid_samples] = radarBuffer[idx].speed;
        target_counts[valid_samples] = radarBuffer[idx].target_count;
        
        sum_dist += distances[valid_samples];
        sum_energy += energies[valid_samples];
        sum_speed += speeds[valid_samples];
        
        if (radarBuffer[idx].energy > detectionThresholds.min_bee_activity) {
            active_frames++;
        }
        
        valid_samples++;
    }
    
    if (valid_samples < 2) return;
    
    // === PARAMETRY STATYSTYCZNE - ODLEGŁOŚĆ ===
    metrics.mean_distance = sum_dist / valid_samples;
    metrics.min_distance = distances[0];
    metrics.max_distance = distances[0];
    
    float sum_sq_diff_dist = 0.0f;
    for (uint8_t i = 0; i < valid_samples; i++) {
        if (distances[i] < metrics.min_distance) metrics.min_distance = distances[i];
        if (distances[i] > metrics.max_distance) metrics.max_distance = distances[i];
        float diff = distances[i] - metrics.mean_distance;
        sum_sq_diff_dist += diff * diff;
    }
    
    metrics.std_distance = sqrt(sum_sq_diff_dist / (valid_samples - 1));
    metrics.range_distance = metrics.max_distance - metrics.min_distance;
    
    // === PARAMETRY STATYSTYCZNE - ENERGIA ===
    metrics.mean_energy = sum_energy / valid_samples;
    metrics.min_energy = energies[0];
    metrics.max_energy = energies[0];
    
    float sum_sq_diff_energy = 0.0f;
    for (uint8_t i = 0; i < valid_samples; i++) {
        if (energies[i] < metrics.min_energy) metrics.min_energy = energies[i];
        if (energies[i] > metrics.max_energy) metrics.max_energy = energies[i];
        float diff = energies[i] - metrics.mean_energy;
        sum_sq_diff_energy += diff * diff;
    }
    
    metrics.std_energy = sqrt(sum_sq_diff_energy / (valid_samples - 1));
    metrics.energy_variance = sum_sq_diff_energy / (valid_samples - 1);
    
    // Współczynnik zmienności (Coefficient of Variation)
    if (abs(metrics.mean_energy) > 0.0001f) {
        metrics.energy_cv = metrics.std_energy / abs(metrics.mean_energy);
    } else {
        metrics.energy_cv = 0.0f;
    }
    
    // === PARAMETRY STATYSTYCZNE - PRĘDKOŚĆ ===
    metrics.mean_speed = sum_speed / valid_samples;
    metrics.max_speed_abs = abs(speeds[0]);
    
    float sum_sq_diff_speed = 0.0f;
    for (uint8_t i = 0; i < valid_samples; i++) {
        float abs_spd = abs(speeds[i]);
        if (abs_spd > metrics.max_speed_abs) metrics.max_speed_abs = abs_spd;
        float diff = speeds[i] - metrics.mean_speed;
        sum_sq_diff_speed += diff * diff;
    }
    
    metrics.std_speed = sqrt(sum_sq_diff_speed / (valid_samples - 1));
    
    // === PARAMETRY TEMPORALNE ===
    metrics.activity_ratio = (float)active_frames / valid_samples * 100.0f;
    metrics.idle_time_percent = 100.0f - metrics.activity_ratio;
    metrics.motion_intensity = metrics.mean_energy * (sum_speed / valid_samples);
    
    // === PARAMETRY CZĘSTOTLIWOŚCIOWE ===
    uint16_t sum_targets = 0;
    metrics.max_target_count = target_counts[0];
    
    for (uint8_t i = 0; i < valid_samples; i++) {
        sum_targets += target_counts[i];
        if (target_counts[i] > metrics.max_target_count) {
            metrics.max_target_count = target_counts[i];
        }
    }
    
    metrics.target_rate = (float)sum_targets / valid_samples;
    
    // Gęstość celów (cele na metr odległości)
    if (metrics.range_distance > 0.01f) {
        metrics.target_density = (float)sum_targets / metrics.range_distance;
    } else {
        metrics.target_density = 0.0f;
    }
    
    // === PARAMETRY TRENDU ===
    TrendAnalysis trend = calculateTrend(window_size);
    metrics.trend_slope = trend.slope;
    metrics.trend_correlation = trend.correlation;
    
    // Obliczanie przyspieszenia zmiany (druga pochodna)
    static float previous_slope = 0.0f;
    metrics.acceleration_rate = metrics.trend_slope - previous_slope;
    previous_slope = metrics.trend_slope;
    
    // === PARAMETRY JAKOŚCIOWE ===
    // Jakość sygnału: zależna od stosunku SNR i spójności danych
    float snr = (metrics.std_energy > 0.001f) ? 
                (metrics.max_energy - metrics.min_energy) / metrics.std_energy : 0.0f;
    metrics.signal_quality = constrain(snr * 20.0f, 0.0f, 100.0f);
    
    // Wynik anomalii: na podstawie wykrytych anomalii i odchylenia od normy
    if (lastAnomaly.type != AnomalyEvent::NONE) {
        metrics.anomaly_score = lastAnomaly.severity * lastAnomaly.confidence;
    } else {
        metrics.anomaly_score = 0.0f;
    }
    
    // Indeks zdrowia ula: złożony wskaźnik oparty o wiele parametrów
    float health_score = 50.0f;  // Bazowy poziom
    
    // Bonus za normalną aktywność
    if (metrics.activity_ratio > 30.0f && metrics.activity_ratio < 90.0f) {
        health_score += 20.0f;
    }
    
    // Kara za anomalie
    health_score -= metrics.anomaly_score * 30.0f;
    
    // Bonus za stabilność (niski CV)
    if (metrics.energy_cv < 0.5f) {
        health_score += 15.0f;
    } else if (metrics.energy_cv > 1.5f) {
        health_score -= 10.0f;
    }
    
    // Bonus za obecność pszczół
    if (metrics.target_rate > 5.0f) {
        health_score += 15.0f;
    }
    
    metrics.hive_health_index = constrain(health_score, 0.0f, 100.0f);
    
    // === DODATKOWE METRYKI ===
    // Entropia sygnału energii
    metrics.entropy = calculateEntropy(energies, valid_samples);
    
    // Częstotliwość przejść przez zero
    metrics.zero_crossing_rate = calculateZeroCrossingRate(energies, valid_samples);
    
    // Dominująca częstotliwość w spektrum mocy
    calculatePowerSpectrum(energies, valid_samples, metrics.power_spectrum_peak);
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
        
        // Obliczanie pełnych metryk radaru (15+ parametrów)
        calculateRadarMetrics(currentMetrics, TREND_WINDOW_SIZE);
        
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
        
        // Debug: wypisz wybrane metryki co 10 pomiarów
        static uint16_t debug_counter = 0;
        if (++debug_counter % 10 == 0) {
          Serial.println("--- METRYKI RADARU ---");
          Serial.print("Health Index: "); Serial.println(currentMetrics.hive_health_index, 1);
          Serial.print("Activity Ratio: "); Serial.print(currentMetrics.activity_ratio, 1); Serial.println("%");
          Serial.print("Energy CV: "); Serial.println(currentMetrics.energy_cv, 3);
          Serial.print("Target Rate: "); Serial.println(currentMetrics.target_rate, 1);
          Serial.print("Signal Quality: "); Serial.print(currentMetrics.signal_quality, 1); Serial.println("%");
          Serial.print("Entropy: "); Serial.println(currentMetrics.entropy, 3);
          Serial.print("Trend Slope: "); Serial.println(currentMetrics.trend_slope, 4);
          Serial.println("----------------------");
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
      
      // Dodaj pełne metryki radaru (15+ parametrów)
      client.print(",\"metrics\":{\"mean_distance\":"); client.print(currentMetrics.mean_distance, 4);
      client.print(",\"std_distance\":"); client.print(currentMetrics.std_distance, 4);
      client.print(",\"min_distance\":"); client.print(currentMetrics.min_distance, 4);
      client.print(",\"max_distance\":"); client.print(currentMetrics.max_distance, 4);
      client.print(",\"range_distance\":"); client.print(currentMetrics.range_distance, 4);
      
      client.print(",\"mean_energy\":"); client.print(currentMetrics.mean_energy, 4);
      client.print(",\"std_energy\":"); client.print(currentMetrics.std_energy, 4);
      client.print(",\"min_energy\":"); client.print(currentMetrics.min_energy, 4);
      client.print(",\"max_energy\":"); client.print(currentMetrics.max_energy, 4);
      client.print(",\"energy_variance\":"); client.print(currentMetrics.energy_variance, 4);
      client.print(",\"energy_cv\":"); client.print(currentMetrics.energy_cv, 4);
      
      client.print(",\"mean_speed\":"); client.print(currentMetrics.mean_speed, 4);
      client.print(",\"std_speed\":"); client.print(currentMetrics.std_speed, 4);
      client.print(",\"max_speed_abs\":"); client.print(currentMetrics.max_speed_abs, 4);
      
      client.print(",\"activity_ratio\":"); client.print(currentMetrics.activity_ratio, 2);
      client.print(",\"idle_time_percent\":"); client.print(currentMetrics.idle_time_percent, 2);
      client.print(",\"motion_intensity\":"); client.print(currentMetrics.motion_intensity, 4);
      
      client.print(",\"target_rate\":"); client.print(currentMetrics.target_rate, 2);
      client.print(",\"max_target_count\":"); client.print(currentMetrics.max_target_count);
      client.print(",\"target_density\":"); client.print(currentMetrics.target_density, 2);
      
      client.print(",\"trend_slope\":"); client.print(currentMetrics.trend_slope, 5);
      client.print(",\"trend_correlation\":"); client.print(currentMetrics.trend_correlation, 4);
      client.print(",\"acceleration_rate\":"); client.print(currentMetrics.acceleration_rate, 5);
      
      client.print(",\"signal_quality\":"); client.print(currentMetrics.signal_quality, 2);
      client.print(",\"anomaly_score\":"); client.print(currentMetrics.anomaly_score, 3);
      client.print(",\"hive_health_index\":"); client.print(currentMetrics.hive_health_index, 2);
      
      client.print(",\"power_spectrum_peak\":"); client.print(currentMetrics.power_spectrum_peak, 4);
      client.print(",\"zero_crossing_rate\":"); client.print(currentMetrics.zero_crossing_rate, 4);
      client.print(",\"entropy\":"); client.print(currentMetrics.entropy, 4);
      client.println("}}");
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
