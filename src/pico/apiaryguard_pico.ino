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
 * HX711 (Waga/Strain Gauge):
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
 * 
 * MODUŁ PROFESJONALNEJ ANALIZY DŹWIĘKU (MEMS MICROPHONE):
 * - Przetwarzanie sygnału audio w czasie rzeczywistym
 * - Analiza FFT (Fast Fourier Transform) - 256 punktów
 * - Detekcja częstotliwości charakterystycznych dla pszczół (80-800 Hz)
 * - Wykrywanie wzorców akustycznych: praca pszczół, rojenie, zagrożenia
 * - Obliczanie 25+ parametrów akustycznych
 * - Klasyfikacja zdarzeń dźwiękowych i ich wpływ na pożytek ula
 * - API HTTP: /audio/status, /audio/spectrum, /audio/events
 * 
 * MODUŁ PROFESJONALNEJ ANALIZY DANYCH Z HX711 (STRAIN GAUGE):
 * - Bufor cyrkularny 288 punktów (24 godziny przy odczycie co 5 minut)
 * - Filtrowanie wykładnicze (EMA) i kompensacja temperaturowa
 * - Analiza trendów w różnych oknach czasowych (1h, 4h, 24h)
 * - Obliczanie 30+ parametrów związanych z wagą ula:
 *   * Statystyczne: mean/std/min/max/median/range/variance/CV/IQR
 *   * Temporalne: current_rate/mean_rate/max_positive/max_negative/acceleration
 *   * Trendy: slope_1h/slope_4h/slope_24h/correlation/direction
 *   * Pożytki: nectar_inflow_rate/accumulation/foraging_efficiency/bloom_intensity
 *   * Produkcja: honey_production_idx/nectar_quality_est
 *   * Konsumpcja: consumption_rate/daily_consumption/food_reserve_days
 *   * Zimowla: winter_readiness/starvation_risk
 *   * Cykliczność: daily_amplitude/circadian_strength/seasonal_trend
 *   * Jakość: signal_quality/noise_level/drift_rate/stability_index
 *   * Anomalie: anomaly_score/sudden_change_mag/oscillation_freq
 *   * Zdrowie: colony_growth_rate/brood_activity_idx/population_estimate
 *   * Produktywność: hive_health_weight/productivity_score
 *   * Prognoza: predicted_weight_24h/forecast_confidence/expected_honey_yield
 * - Detekcja zdarzeń: przepływ nektaru, rojenie, niski zapas, atak drapieżnika
 * - Klasyfikacja wpływu zdarzeń na pożytek ula (pozytywny/negatywny/krytyczny)
 * - API HTTP: /hx711/status, /hx711/metrics, /hx711/events, /hx711/forecast
 * 
 * MODUŁ PROFESJONALNEJ ANALIZY JAKOŚCI POWIETRZA (SGP41):
 * - Bufor cyrkularny 144 punktów (24 godziny przy odczycie co 10 minut)
 * - Obliczanie 24+ parametrów jakości powietrza w ulu:
 *   * Podstawowe: CO2_eq, VOC_index, NOx_equivalent
 *   * Statystyczne: mean/std/min/max/range (CO2, VOC)
 *   * Trendy: slope_1h/slope_4h/slope_24h, trend_direction, trend_strength
 *   * Indeksy jakości: IAQ_index (Indoor Air Quality), air_quality_level
 *   * Zdrowie ula: ventilation_need, stress_from_air, hive_comfort_index
 *   * Zagrożenia: poor_ventilation_alert, contamination_risk, mold_risk
 *   * Temporalne: variability_index, stability_score, change_rate
 *   * Korelacje: temp_humidity_correlation, comfort_zone_percent
 *   * Progi: co2_warning_level, voc_alert_level, combined_risk_score
 * - Detekcja zdarzeń: słaba wentylacja, zanieczyszczenie, pleśń, stres kolonii
 * - API HTTP: /airquality/status, /airquality/metrics, /airquality/events
 * ---------------------------------------------------------
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>

// --- KONFIGURACJA SIECI W6100 ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177); // Statyczne IP dla Pico
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress rpi_ip(192, 168, 1, 100); // IP Raspberry Pi - odbiorca danych UDP
EthernetServer server(8080); // Port nasłuchiwania HTTP
EthernetUDP udp; // Klient UDP do wysyłania danych do RPi
const int UDP_PORT = 5005; // Port UDP na Raspberry Pi

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
#define RELAY_CH6  28  // POPRAWKA: Zmieniono z GPIO 26 na GPIO 28 (konflikt z MIC_PIN - ADC0)
#define RELAY_CH7  23
#define RELAY_CH8  24

// --- OBIEKTY ---
EthernetClient client;
DHT dht(DHT_PIN, DHT22);
Adafruit_SGP41 sgp;
HardwareSerial radarSerial(uart1); // UART1 dla radaru

// Zmienne dla HX711 (prosta implementacja bit-bang)
// UWAGA: hx711_offset i hx711_scale są używane do kalibracji wagi
// Wartości powinny być ustawiane podczas kalibracji (obecnie wartości domyślne)
long hx711_value = 0;           // Aktualna wartość z wagi (po przeliczeniu)
const long hx711_offset = 0;    // Offset kalibracyjny (tare) - do ustawienia podczas kalibracji
const float hx711_scale = 1.0f; // Współczynnik skalowania - do ustawienia podczas kalibracji

// Zmienne sensorów - główne wskaźniki stanu
float temperature = 0.0f;       // Temperatura z DHT22/SHT40 [°C]
float humidity = 0.0f;          // Wilgotność względna z DHT22/SHT40 [%RH]
uint16_t co2_eq = 0;            // Równoważne CO2 z SGP41 [ppm]
uint16_t voc_idx = 0;           // Indeks VOC z SGP41 [index]
// UWAGA: audio_rms usunięte - dane dostępne w currentAudioMetrics.rms_amplitude
// UWAGA: motion_detected usunięte - dane dostępne w currentRadarMetrics.activity_ratio
bool motion_detected = false;   // Flaga wykrycia ruchu z radaru (tymczasowo zachowana dla kompatybilności)

// ============================================================================
// MODUŁ PROFESJONALNEJ ANALIZY DŹWIĘKU - STRUKTURY DANYCH
// ============================================================================

#define AUDIO_BUFFER_SIZE 256     // Rozmiar bufora FFT (potęga liczby 2)
#define AUDIO_SAMPLE_RATE 4000    // Częstotliwość próbkowania [Hz]
#define BEE_FREQ_MIN 80           // Minimalna częstotliwość pszczół [Hz]
#define BEE_FREQ_MAX 800          // Maksymalna częstotliwość pszczół [Hz]
#define SWARM_FREQ_MIN 150        // Częstotliwość rojenia [Hz]
#define SWARM_FREQ_MAX 350        // Częstotliwość rojenia [Hz]
#define QUEEN_PIP_FREQ_MIN 200    // Częstotliwość piszczenia królowej [Hz]
#define QUEEN_PIP_FREQ_MAX 500    // Częstotliwość piszczenia królowej [Hz]

// Struktura przechowująca parametry akustyczne ula
struct AudioMetrics {
    // Podstawowe parametry czasowe
    float rms_amplitude;          // Wartość RMS sygnału [V]
    float peak_amplitude;         // Wartość szczytowa [V]
    float peak_to_peak;           // Wartość międzyszczytowa [V]
    float zero_crossing_rate;     // Częstotliwość przejść przez zero [Hz]
    float signal_energy;          // Energia sygnału [V²]
    
    // Parametry statystyczne
    float mean_amplitude;         // Średnia amplituda [V]
    float std_amplitude;          // Odchylenie standardowe amplitudy [V]
    float amplitude_cv;           // Współczynnik zmienności amplitudy
    float skewness;               // Asymetria rozkładu (skewness)
    float kurtosis;               // Kurtoza (spłaszczenie rozkładu)
    
    // Parametry częstotliwościowe - podstawowe
    float dominant_frequency;     // Dominująca częstotliwość [Hz]
    float spectral_centroid;      // Centrum widma [Hz]
    float spectral_bandwidth;     // Szerokość pasma widma [Hz]
    float spectral_flatness;      // Płaskość widma (0-1)
    float spectral_rolloff;       // Częstotliwość odcięcia widma [Hz]
    
    // Parametry częstotliwościowe - szczegółowe
    float power_in_bee_band;      // Moc w paśmie pszczół (80-800 Hz) [dB]
    float power_in_swarm_band;    // Moc w paśmie rojenia (150-350 Hz) [dB]
    float power_low_freq;         // Moc w niskich częstotliwościach (<80 Hz) [dB]
    float power_high_freq;        // Moc w wysokich częstotliwościach (>800 Hz) [dB]
    float harmonic_ratio;         // Stosunek harmonicznych do fundamentalnej
    
    // Parametry zaawansowane
    float mfcc_energy[4];         // Energia z pierwszych 4 współczynników MFCC
    float spectral_entropy;       // Entropia widmowa (miara losowości)
    float spectral_contrast;      // Kontrast widmowy
    float tonal_strength;         // Siła tonalna sygnału
    
    // NOWE PARAMETRY - analiza szczegółowa (dodane 18 parametrów)
    float crest_factor;           // Współczynnik szczytu (peak/rms) - detekcja impulsów
    float formant_freq_1;         // Pierwszy formant [Hz] - charakterystyka rezonansu ula
    float formant_freq_2;         // Drugi formant [Hz]
    float formant_bandwidth_1;    // Szerokość pasma pierwszego formantu [Hz]
    float formant_bandwidth_2;    // Szerokość pasma drugiego formantu [Hz]
    float fundamental_frequency;  // Częstotliwość podstawowa (F0) [Hz]
    float pitch_strength;         // Siła wysokości dźwięku (0-1)
    float inharmonicity;          // Nieharmoniczność sygnału (0-1)
    float shimmer;                // Fluktuacja amplitudy w czasie (%)
    float jitter;                 // Fluktuacja częstotliwości w czasie (%)
    float noise_to_harmonic_ratio;// Stosunek szumu do harmonicznych (NHR)
    float harmonic_to_noise_ratio;// Stosunek harmonicznych do szumu (HNR)
    float autocorrelation_peak;   // Szczyt autokorelacji (0-1) - miara periodyczności
    float attack_time;            // Czas narastania sygnału [ms]
    float decay_time;             // Czas zanikania sygnału [ms]
    float sustain_level;          // Poziom podtrzymania sygnału (0-1)
    float temporal_centroid;      // Centrum czasowe sygnału [ms]
    float loudness_zwicker;       // Głośność psychoakustyczna [sones]
    
    // Wskaźniki jakości i klasyfikacji
    float bee_activity_index;     // Indeks aktywności pszczół (0-100%)
    float swarm_probability;      // Prawdopodobieństwo rojenia (0-1)
    float stress_indicator;       // Wskaźnik stresu kolonii (0-1)
    float hive_health_audio;      // Indeks zdrowia na podstawie audio (0-100%)
    float foraging_efficiency;    // Efektywność zbierania nektaru (0-100%) - nowość
    float colony_coherence;       // Spójność kolonii (0-1) - nowość
    
    // NOWE PARAMETRY DODANE - analiza zaawansowana (dodatkowe 18 parametrów)
    float spectral_flux;          // Strumień spektralny - zmiana widma w czasie
    float spectral_slope;         // Nachylenie widma - balans tonalny
    float spectral_kurtosis;      // Kurtoza widma - ostrość pików
    float spectral_skewness;      // Asymetria widma
    float fundamental_salience;   // Wyraźność tonu podstawowego (0-1)
    float partial_energy_ratio;   // Stosunek energii harmonicznych do całości
    float odd_harmonic_energy;    // Energia nieparzystych harmonicznych
    float even_harmonic_energy;   // Energia parzystych harmonicznych
    float tritone_distance;       // Dystans od trytonu (miara dysonansu)
    float inharmonicity_deviation;// Odchylenie nieharmoniczności
    float spectral_irregularity;  // Nieregularność widmowa
    float log_attack_time;        // Logarytmiczny czas ataku
    float temporal_log_attack;    // Temporalny log attack
    float effective_duration;     // Efektywny czas trwania sygnału
    float rise_time;              // Czas narastania (10-90%)
    float decay_rate;             // Szybkość zanikania [dB/s]
    float release_time;           // Czas wybrzmiewania
    float vibrato_depth;          // Głębokość wibrata (detekcja queen piping)
    float vibrato_rate;           // Częstotliwość wibrata [Hz]
    float tremolo_depth;          // Głębokość tremola
    float tremolo_rate;           // Częstotliwość tremola [Hz]
    float spectral_valley_count;  // Liczba dolin w widmie
    float peak_prominence;        // Wyraźność dominującego piku
    float bandwidth_75;           // Szerokość pasma 75% energii
    float bandwidth_95;           // Szerokość pasma 95% energii
    float equivalent_sound_level; // Równoważny poziom dźwięku Leq [dB]
    float percentile_level_10;    // Poziom przekroczony w 10% czasu L10 [dB]
    float percentile_level_90;    // Poziom przekroczony w 90% czasu L90 [dB]
    float noise_floor_estimate;   // Szacowane tło szumowe [dB]
    float signal_to_noise_ratio;  // Stosunek sygnału do szumu [dB]
    float acoustic_complexity;    // Złożoność akustyczna (ACI index)
    float bioacoustic_index;      // Indeks bioakustyczny (BI)
    float normalized_difference;  // Znormalizowany indeks różnicowy (NDI)
    float acoustic_diversity;     // Różnorodność akustyczna (ADI)
    float acoustic_evenness;      // Równość akustyczna (AEI)
    float power_band_1;           // Moc w paśmie 0-50 Hz [dB]
    float power_band_2;           // Moc w paśmie 50-100 Hz [dB]
    float power_band_3;           // Moc w paśmie 100-200 Hz [dB]
    float power_band_4;           // Moc w paśmie 200-400 Hz [dB]
    float power_band_5;           // Moc w paśmie 400-800 Hz [dB]
    float power_band_6;           // Moc w paśmie 800-1600 Hz [dB]
    float power_band_7;           // Moc w paśmie 1600-3200 Hz [dB]
    float power_band_8;           // Moc w paśmie >3200 Hz [dB]
};

// Bufory dla przetwarzania audio
int16_t audioBuffer[AUDIO_BUFFER_SIZE];        // Surowe próbki audio
float audioFFT[AUDIO_BUFFER_SIZE];             // Wynik FFT (moduły)
float audioSpectrum[AUDIO_BUFFER_SIZE / 2];    // Widmo mocy
AudioMetrics currentAudioMetrics;              // Aktualne metryki audio

// Historia danych audio dla analizy trendów
struct AudioHistoryPoint {
    uint32_t timestamp;
    float rms_value;
    float dominant_freq;
    float bee_activity;
    float swarm_prob;
};

#define AUDIO_HISTORY_SIZE 60   // 1 minuta historii przy próbkowaniu co 1s
AudioHistoryPoint audioHistory[AUDIO_HISTORY_SIZE];
uint16_t audioHistoryIndex = 0;
uint16_t audioHistoryCount = 0;

// Zdarzenia akustyczne
struct AudioEvent {
    enum EventType {
        EVENT_NONE = 0,
        EVENT_NORMAL_ACTIVITY,      // Normalna praca pszczół
        EVENT_INCREASED_ACTIVITY,   // Zwiększona aktywność
        EVENT_SWARM_PREPARATION,    // Przygotowania do rojenia
        EVENT_SWARM_ACTIVE,         // Aktywne rojenie
        EVENT_QUEEN_PIPING,         // Piszczenie królowej
        EVENT_PREDATOR_THREAT,      // Zagrożenie drapieżnikiem
        EVENT_DISTRESS,             // Sygnały distress
        EVENT_LOW_ACTIVITY          // Niska aktywność
    };
    
    enum EventImpact {
        IMPACT_NEUTRAL = 0,
        IMPACT_POSITIVE,            // Pozytywny wpływ na pożytek
        IMPACT_NEGATIVE,            // Negatywny wpływ na pożytek
        IMPACT_CRITICAL             // Krytyczne zagrożenie
    };
    
    EventType type;
    EventImpact impact;
    float confidence;               // Pewność detekcji (0-1)
    uint32_t timestamp;
    char description[64];           // Opis zdarzenia
};

AudioEvent lastAudioEvent;

// Deklaracje funkcji modułu audio
void processAudioSignal();
void calculateAudioMetrics(AudioMetrics& metrics);
void performFFT(int16_t* input, float* output, int size);
void classifyAudioEvent(AudioEvent& event, const AudioMetrics& metrics);
float calculateSpectralCentroid(const float* spectrum, int size, float sampleRate);
float calculateSpectralBandwidth(const float* spectrum, int size, float centroid, float sampleRate);
float calculateSpectralFlatness(const float* spectrum, int size);
float calculateSpectralEntropy(const float* spectrum, int size);
void updateAudioHistory(const AudioMetrics& metrics);
bool isPositiveAudioChange(const AudioEvent& event);
const char* audioEventTypeToString(AudioEvent::EventType type);
const char* audioEventImpactToString(AudioEvent::EventImpact impact);

// ============================================================================
// MODUŁ PROFESJONALNEJ ANALIZY DANYCH Z CZUJNIKA HX711 (STRAIN GAUGE)
// WYLICZANIE PARAMETRÓW ZMIAN W POŻYTKU ULU I PSZCZÓŁ
// ============================================================================

#define HX711_BUFFER_SIZE 288   // 24 godziny danych przy odczycie co 5 minut
#define HX711_TREND_WINDOW 12   // Okno trendu: 1 godzin (12 * 5min)
#define HX711_DAILY_WINDOW    48 // 4 godziny (48 * 5min)
#define HX711_SHORT_WINDOW    6  // 30 minut (6 * 5min)
#define HX711_WEIGHT_CHANGE_THRESHOLD 0.05f  // 5% zmiany wagi dla detekcji zdarzeń
#define HX711_NECTAR_FLOW_MIN 0.02f  // Minimalny przepływ nektaru [kg/h]
#define HX711_CONSUMPTION_MIN 0.01f  // Minimalne zużycie zapasów [kg/h]

// Struktura przechowująca pojedynczy pomiar wagi
struct HX711DataPoint {
    uint32_t timestamp;       // Czas pomiaru [ms od startu]
    float weight_raw;         // Surowa waga [kg]
    float weight_filtered;    // Filtrowana waga [kg]
    float temperature_comp;   // Kompensacja temperaturowa [°C]
    float rate_of_change;     // Szybkość zmiany wagi [kg/h]
    bool is_valid;            // Flag ważności pomiaru
    uint8_t quality_score;    // Jakość pomiaru (0-100)
};

// Struktura przechowująca metryki wagi - 60+ parametrów
struct HX711Metrics {
    // === PODSTAWOWE PARAMETRY STATYSTYCZNE ===
    float mean_weight;          // Średnia waga [kg]
    float std_weight;           // Odchylenie standardowe wagi [kg]
    float min_weight;           // Minimalna waga [kg]
    float max_weight;           // Maksymalna waga [kg]
    float range_weight;         // Zakres zmian wagi (max-min) [kg]
    float median_weight;        // Mediana wagi [kg]

    // Wariancja i współczynniki zmienności - ROZSZERZONE
    float weight_variance;      // Wariancja wagi [kg²]
    float weight_cv;            // Współczynnik zmienności (CV = std/mean)
    float weight_iqr;           // Rozstęp międzykwartylowy [kg]
    float weight_skewness;      // Współczynnik skośności rozkładu
    float weight_kurtosis;      // Współczynnik kurtozy (spłaszczenia)
    float weight_gini;          // Współczynnik Giniego (nierównomierność)

    // === PARAMETRY TEMPORALNE - SZYBKOŚCI ZMIAN - ROZSZERZONE ===
    float current_rate;         // Aktualna szybkość zmiany [kg/h]
    float mean_rate;            // Średnia szybkość zmiany [kg/h]
    float max_rate_positive;    // Maksymalny przyrost [kg/h]
    float max_rate_negative;    // Maksymalny ubytek [kg/h]
    float acceleration;         // Przyspieszenie zmiany wagi [kg/h²]
    float jerk;                 // Pochodna przyspieszenia [kg/h³]
    float rate_variance;        // Wariancja szybkości zmian
    float rate_entropy;         // Entropia szybkości zmian

    // === PARAMETRY KIERUNKU TRENDU - ROZSZERZONE ===
    float trend_slope_1h;       // Nachylenie trendu 1h [kg/h]
    float trend_slope_4h;       // Nachylenie trendu 4h [kg/h]
    float trend_slope_24h;      // Nachylenie trendu 24h [kg/h]
    float trend_correlation;    // Współczynnik korelacji trendu (-1 do 1)
    float trend_direction;      // Kierunek: -1 (spadek), 0 (stabilny), 1 (wzrost)
    float trend_strength;       // Siła trendu (0-1)
    float trend_persistence;    // Persystencja trendu (czy utrzymuje się)
    float trend_change_points;  // Liczba punktów zwrotnych trendu

    // === PARAMETRY POŻYTKU I ZBIORÓW - ROZSZERZONE ===
    float nectar_inflow_rate;   // Przepływ nektaru [kg/h]
    float nectar_accumulation;  // Skumulowany nektar [kg]
    float foraging_efficiency;  // Efektywność zbierania (0-100%)
    float honey_production_idx; // Indeks produkcji miodu (0-100%)
    float bloom_intensity;      // Intensywność kwitnienia (0-100%)
    float nectar_quality_est;   // Szacowana jakość nektaru (0-100%)
    float nectar_flow_duration; // Czas trwania przepływu [h]
    float foraging_window_start;// Godzina rozpoczęcia wylotów
    float foraging_window_end;  // Godzina zakończenia wylotów

    // === PARAMETRY KONSUMPCJI I ZUŻYCIA - ROZSZERZONE ===
    float consumption_rate;     // Zużycie zapasów [kg/h]
    float daily_consumption;    // Dzienne zużycie [kg/dzień]
    float food_reserve_days;    // Zapas żywności na dni
    float winter_readiness;     // Gotowość do zimowli (0-100%)
    float starvation_risk;      // Ryzyko głodu (0-100%)
    float metabolic_rate;       // Szacowane tempo metabolizmu [kg/h]
    float consumption_regularity;// Regularność zużycia (0-1)

    // === PARAMETRY CYKLICZNOŚCI I WZORCÓW - ROZSZERZONE ===
    float daily_amplitude;      // Amplituda dobowa [kg]
    float daily_phase;          // Faza dobowa [godziny]
    float circadian_strength;   // Siła rytmu dobowego (0-1)
    float weekly_pattern_match; // Dopasowanie wzorca tygodniowego (0-1)
    float seasonal_trend;       // Trend sezonowy (-1 do 1)
    float harmonic_content;     // Zawartość harmonicznych w sygnale
    float cycle_regularity;     // Regularność cykli (0-1)
    float phase_coherence;      // Koherencja fazowa (0-1)

    // === PARAMETRY JAKOŚCI SYGNAŁU - ROZSZERZONE ===
    float signal_quality;       // Jakość sygnału wagi (0-100%)
    float noise_level;          // Poziom szumu [kg]
    float drift_rate;           // Dryft czujnika [kg/h]
    float stability_index;      // Indeks stabilności (0-100%)
    float measurement_confidence;// Pewność pomiaru (0-1)
    float snr;                  // Stosunek sygnału do szumu [dB]
    float thd;                  // Total Harmonic Distortion (%)
    float baseline_stability;   // Stabilność linii bazowej (0-1)

    // === PARAMETRY ANOMALII I ZDARZEŃ - ROZSZERZONE ===
    float anomaly_score;        // Wynik anomalii (0-1)
    float sudden_change_mag;    // Wielkość nagłej zmiany [kg]
    float oscillation_freq;     // Częstotliwość oscylacji [cykle/dzień]
    float oscillation_damping;  // Tłumienie oscylacji (0-1)
    float outlier_ratio;        // Stosunek wartości odstających (%)
    float change_point_prob;    // Prawdopodobieństwo punktu zwrotnego
    float volatility_index;     // Indeks zmienności (0-100%)

    // === WSKAŹNIKI ZDROWIA KOLONII - ROZSZERZONE ===
    float colony_growth_rate;   // Tempo wzrostu kolonii [%/dzień]
    float brood_activity_idx;   // Indeks aktywności czerwiu (0-100%)
    float population_estimate;  // Szacowana populacja [tysiące pszczół]
    float hive_health_weight;   // Indeks zdrowia z wagi (0-100%)
    float productivity_score;   // Ogólny wynik produktywności (0-100%)
    float stress_indicator;     // Indikator stresu kolonii (0-1)
    float vitality_index;       // Indeks vitalności (0-100%)
    float resilience_score;     // Zdolność do regeneracji (0-1)

    // === PARAMETRY PROGNOZY - ROZSZERZONE ===
    float predicted_weight_24h; // Prognoza wagi za 24h [kg]
    float forecast_confidence;  // Pewność prognozy (0-1)
    float expected_honey_yield; // Oczekiwany zbiór miodu [kg]
    float prediction_interval;  // Przedział ufności prognozy [kg]
    float forecast_trend;       // Kierunek prognozy (-1,0,1)
};
// Bufory dla przetwarzania danych HX711
HX711DataPoint hx711Buffer[HX711_BUFFER_SIZE];
uint16_t hx711BufferIndex = 0;
uint16_t hx711DataCount = 0;

HX711Metrics currentHX711Metrics;

// Historia dzienna dla analizy wzorców
struct HX711DailyPattern {
    float hour_avg[24];         // Średnia waga dla każdej godziny
    float hour_std[24];         // Odchylenie dla każdej godziny
    uint8_t valid_samples[24];  // Liczba ważnych próbek
} hx711DailyPattern;

// Zdarzenia związane z wagą
struct HX711Event {
    enum EventType {
        EVENT_NONE = 0,
        EVENT_NECTAR_FLOW_START,      // Rozpoczęcie przepływu nektaru
        EVENT_NECTAR_FLOW_PEAK,       // Szczyt przepływu nektaru
        EVENT_NECTAR_FLOW_END,        // Zakończenie przepływu nektaru
        EVENT_HONEY_SUPER_ADDED,      // Dodano korpus miodniowy
        EVENT_HONEY_HARVESTED,        // Zebrano miód
        EVENT_HIGH_CONSUMPTION,       // Wysokie zużycie zapasów
        EVENT_LOW_FOOD_RESERVE,       // Niski zapas żywności
        EVENT_SWARM_DEPARTURE,        // Wyjście roju (nagły spadek wagi)
        EVENT_PREDATOR_ATTACK,        // Atak drapieżnika (oscylacje)
        EVENT_WEATHER_IMPACT,         // Wpływ pogody (brak wylotów)
        EVENT_DRONE_EJECTION,         // Wyrzucenie trutni
        EVENT_WINTER_CLUSTER,         // Formowanie klastra zimowego
        EVENT_SPRING_BUILDUP,         // Wiosenny rozwój
        EVENT_MEasurement_ERROR       // Błąd pomiaru
    };
    
    enum EventImpact {
        IMPACT_NEUTRAL = 0,
        IMPACT_VERY_POSITIVE,         // Bardzo pozytywny wpływ na pożytek
        IMPACT_POSITIVE,              // Pozytywny wpływ na pożytek
        IMPACT_SLIGHTLY_POSITIVE,     // Lekko pozytywny
        IMPACT_SLIGHTLY_NEGATIVE,     // Lekko negatywny
        IMPACT_NEGATIVE,              // Negatywny wpływ na pożytek
        IMPACT_VERY_NEGATIVE,         // Bardzo negatywny wpływ
        IMPACT_CRITICAL               // Krytyczne zagrożenie
    };
    
    EventType type;
    EventImpact impact;
    float confidence;                 // Pewność detekcji (0-1)
    float magnitude;                  // Wielkość zdarzenia [kg lub kg/h]
    uint32_t timestamp;
    char description[80];             // Opis zdarzenia
};

HX711Event lastHX711Event;

// Progi detekcji zdarzeń
struct HX711Thresholds {
    float nectar_flow_threshold;      // Próg przepływu nektaru [kg/h]
    float swarm_loss_threshold;       // Próg utraty wagi dla roju [kg]
    float consumption_alert;          // Alarm zużycia [kg/dzień]
    float food_reserve_critical;      // Krytyczny zapas [kg]
    float food_reserve_low;           // Niski zapas [kg]
    float sudden_change_threshold;    // Nagła zmiana [%]
    float oscillation_threshold;      // Próg oscylacji [kg]
} hx711Thresholds = {
    .nectar_flow_threshold = 0.05f,   // 50g/h
    .swarm_loss_threshold = 1.5f,     // 1.5kg utraty
    .consumption_alert = 0.5f,        // 0.5kg/dzień
    .food_reserve_critical = 5.0f,    // 5kg krytycznie
    .food_reserve_low = 10.0f,        // 10kg niski
    .sudden_change_threshold = 0.15f, // 15% nagłej zmiany
    .oscillation_threshold = 0.3f     // 300g oscylacji
};

// Deklaracje funkcji modułu HX711
void updateHX711Buffer(const HX711DataPoint& point);
void calculateHX711Metrics(HX711Metrics& metrics);
HX711Event detectHX711Events(const HX711DataPoint& current, const HX711Metrics& metrics);
void updateHX711DailyPattern(const HX711DataPoint& point);
float applyWeightFilter(float raw_weight, float prev_filtered, float alpha);
float calculateMovingAverage(const float* data, uint16_t count, uint8_t window);
float calculateLinearRegression(const float* x, const float* y, uint16_t count, float& slope, float& intercept);
float calculatePercentile(const float* data, uint16_t count, float percentile);
void compensateTemperature(float& weight, float temperature);
bool isPositiveHX711Change(const HX711Event& event);
const char* hx711EventTypeToString(HX711Event::EventType type);
const char* hx711EventImpactToString(HX711Event::EventImpact impact);
void generateHX711EventDescription(HX711Event& event);

// Timer'y
unsigned long lastMillis = 0;
unsigned long networkHeartbeat = 0;

// ============================================================================
// MODUŁ DYNAMICZNEGO WYKRYWANIA SENSORÓW
// ============================================================================

struct SensorFlags {
    bool detected = false;   // Czy hardware został wykryty
    bool active = false;     // Czy moduł jest włączony do przetwarzania
    int errorCode = 0;       // Kod błędu inicjalizacji
    unsigned long lastRead = 0;
};

struct SystemSensors {
    SensorFlags hx711;       // Waga (HX711)
    SensorFlags audio;       // Mikrofon (MEMS)
    SensorFlags radar;       // Radar MMWave (LD2410B)
    SensorFlags tempHum;     // Temp/Wilg (DHT22/SHT40/BME280)
    SensorFlags airQual;     // Jakość powietrza (SGP41)
    SensorFlags piezo;       // Wibracje (Piezo)
    SensorFlags baro;        // Ciśnienie (BME280)
    SensorFlags light;       // Światło (BH1750/Analog)
} sensors;

// Deklaracje funkcji wykrywania
void detectAllSensors();
void printSensorStatus();
bool checkHX711();
bool checkAudio();
bool checkRadar();
bool checkTempHumidity();
bool checkAirQuality();
bool checkPiezo();
bool checkBarometric();
bool checkLight();

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

// ============================================================================
// MODUŁ WYLICZANIA PARAMETRÓW TEMPERATURY I WILGOTNOŚCI (DHT22/AM2302)
// ============================================================================

#define TEMP_HUM_BUFFER_SIZE 288  // 24 godziny przy odczycie co 5 minut

struct TempHumidityMetrics {
    // Podstawowe parametry temperatury
    float temp_mean;              // Średnia temperatura [°C]
    float temp_std;               // Odchylenie standardowe temperatury [°C]
    float temp_min;               // Minimalna temperatura [°C]
    float temp_max;               // Maksymalna temperatura [°C]
    float temp_range;             // Zakres temperatury (max-min) [°C]
    
    // Podstawowe parametry wilgotności
    float hum_mean;               // Średnia wilgotność [%RH]
    float hum_std;                // Odchylenie standardowe wilgotności [%RH]
    float hum_min;                // Minimalna wilgotność [%RH]
    float hum_max;                // Maksymalna wilgotność [%RH]
    float hum_range;              // Zakres wilgotności (max-min) [%RH]
    
    // Parametry komfortu termicznego
    float heat_index;             // Indeks ciepła (odczuwalna temperatura) [°C]
    float dew_point;              // Punkt rosy [°C]
    float vapor_pressure_deficit; // Deficyt ciśnienia pary wodnej [kPa]
    float comfort_index;          // Indeks komfortu (0-100%)
    
    // Stabilność środowiska
    float temp_stability;         // Stabilność temperatury (0-100%)
    float hum_stability;          // Stabilność wilgotności (0-100%)
    float env_variance;           // Wariancja środowiska (temp+hum)
    
    // Trendy
    float temp_trend_1h;          // Trend temperatury 1h [°C/h]
    float hum_trend_1h;           // Trend wilgotności 1h [%RH/h]
    float temp_hum_correlation;   // Korelacja temp-wilgotność [-1 do 1]
    
    // Alarmy
    float overheating_risk;       // Ryzyko przegrzania (0-1)
    float condensation_risk;      // Ryzyko kondensacji (0-1)
    float mold_risk;              // Ryzyko pleśni (0-1)
    float brood_stress_index;     // Indeks stresu czerwiu (0-100%)
};

TempHumidityMetrics currentTempHumMetrics;

// Deklaracje funkcji modułu temp/wilg
void calculateTempHumidityMetrics(TempHumidityMetrics& metrics);
float calculateHeatIndex(float temp, float humidity);
float calculateDewPoint(float temp, float humidity);
float calculateVPD(float temp, float humidity);

// ============================================================================
// MODUŁ WYLICZANIA PARAMETRÓW JAKOŚCI POWIETRZA (SGP41/BME688)
// ============================================================================

#define AIR_QUALITY_BUFFER_SIZE 144  // 24 godziny przy odczycie co 10 minut

struct AirQualityMetrics {
    // Podstawowe parametry gazów
    uint16_t co2_equivalent;      // Równoważne CO2 [ppm]
    uint16_t voc_index;           // Indeks VOC (0-500)
    uint16_t nox_equivalent;      // Równoważne NOx [ppb]
    
    // Statystyki CO2
    float co2_mean;               // Średnie CO2 [ppm]
    float co2_std;                // Odchylenie CO2 [ppm]
    float co2_min;                // Minimalne CO2 [ppm]
    float co2_max;                // Maksymalne CO2 [ppm]
    
    // Statystyki VOC
    float voc_mean;               // Średnie VOC
    float voc_std;                // Odchylenie VOC
    float voc_min;                // Minimalne VOC
    float voc_max;                // Maksymalne VOC
    
    // Indeksy jakości powietrza
    float iaq_index;              // Indoor Air Quality Index (0-500)
    float air_quality_level;      // Poziom jakości (1-5: Excellent-Poor)
    float ventilation_need;       // Zapotrzebowanie na wentylację (0-100%)
    
    // Zdrowie ula
    float hive_comfort_index;     // Indeks komfortu ula (0-100%)
    float stress_from_air;        // Stres od jakości powietrza (0-1)
    float contamination_risk;     // Ryzyko zanieczyszczenia (0-1)
    
    // Trendy
    float co2_trend_1h;           // Trend CO2 1h [ppm/h]
    float voc_trend_1h;           // Trend VOC 1h [index/h]
    float air_quality_trend;      // Trend jakości (-1 do 1)
    
    // Alerty
    float poor_ventilation_alert; // Alert słabej wentylacji (0-1)
    float high_co2_warning;       // Ostrzeżenie wysokiego CO2 (0-1)
    float voc_alert_level;        // Poziom alertu VOC (0-1)
    float combined_risk_score;    // Łączny wynik ryzyka (0-100%)
};

AirQualityMetrics currentAirQualityMetrics;

// Deklaracje funkcji modułu jakości powietrza
void calculateAirQualityMetrics(AirQualityMetrics& metrics);
uint8_t mapIAQLevel(float iaq_index);
float calculateVentilationNeed(uint16_t co2, uint16_t voc);

// ============================================================================
// MODUŁ WYLICZANIA PARAMETRÓW WIBRACJI (PIEZO SENSOR)
// ============================================================================

#define PIEZO_BUFFER_SIZE 120  // 2 minuty przy odczycie co 1 sekundzie

struct PiezoVibrationMetrics {
    // Podstawowe parametry amplitudy
    float vibration_rms;          // Wartość RMS wibracji [mV]
    float vibration_peak;         // Amplituda szczytowa [mV]
    float vibration_peak_to_peak; // Amplituda międzyszczytowa [mV]
    float vibration_mean;         // Średnia wibracji [mV]
    
    // Statystyki wibracji
    float vibration_std;          // Odchylenie standardowe [mV]
    float vibration_cv;           // Współczynnik zmienności
    float vibration_energy;       // Energia wibracji [mV²]
    
    // Parametry częstotliwościowe
    float dominant_vib_freq;      // Dominująca częstotliwość [Hz]
    float vibration_activity_idx; // Indeks aktywności wibracyjnej (0-100%)
    
    // Detekcja zdarzeń
    float impact_count;           // Liczba uderzeń/impulsów
    float continuous_vib_ratio;   // Stosunek wibracji ciągłych (0-1)
    float intermittent_vib_ratio; // Stosunek wibracji przerywanych (0-1)
    
    // Klasyfikacja źródła
    float bee_traffic_score;      // Wynik ruchu pszczół (0-100%)
    float predator_vib_score;     // Wynik drapieżnika (0-100%)
    float environmental_vib_score;// Wynik środowiska (wiatr) (0-100%)
    
    // Trendy
    float vibration_trend_1min;   // Trend wibracji 1min [mV/min]
    float activity_change_rate;   // Szybkość zmiany aktywności [%/min]
    
    // Alerty
    float intrusion_probability;  // Prawdopodobieństwo intruza (0-1)
    float swarm_vibration_flag;   // Flaga wibracji rojowej (0-1)
    float anomaly_vibration_score;// Wynik anomalii wibracji (0-1)
};

PiezoVibrationMetrics currentPiezoMetrics;

// Deklaracje funkcji modułu piezo
void calculatePiezoMetrics(PiezoVibrationMetrics& metrics);
float detectDominantFrequency(const float* data, uint16_t count, float sampleRate);

// ============================================================================
// MODUŁ WYLICZANIA PARAMETRÓW CIŚNIENIA ATMOSFERYCZNEGO (BMP280)
// ============================================================================

struct BarometricMetrics {
    // Podstawowe parametry ciśnienia
    float pressure_mean;          // Średnie ciśnienie [hPa]
    float pressure_std;           // Odchylenie ciśnienia [hPa]
    float pressure_min;           // Minimalne ciśnienie [hPa]
    float pressure_max;           // Maksymalne ciśnienie [hPa]
    float pressure_trend_1h;      // Trend ciśnienia 1h [hPa/h]
    float pressure_trend_3h;      // Trend ciśnienia 3h [hPa/h]
    
    // Prognoza pogody
    float weather_trend;          // Trend pogodowy (-1 do 1: ухудшение-do poprawy)
    float storm_probability;      // Prawdopodobieństwo burzy (0-100%)
    float pressure_sea_level;     // Ciśnienie na poziomie morza [hPa]
    
    // Wpływ na pszczoły
    float foraging_conditions;    // Warunki do wylotów (0-100%)
    float swarm_weather_index;    // Indeks pogody dla rojenia (0-100%)
    float barometric_stress;      // Stres barometryczny (0-1)
    
    // Altitude compensation
    float altitude_compensated;   // Skompensowana wysokość [m]
    float pressure_altitude;      // Wysokość barometryczna [m]
};

BarometricMetrics currentBaroMetrics;

// Deklaracje funkcji modułu barometrycznego
void calculateBarometricMetrics(BarometricMetrics& metrics);
float calculateSeaLevelPressure(float pressure, float altitude);
int predictWeatherTrend(const float* pressures, uint8_t count);

// ============================================================================
// MODUŁ WYLICZANIA PARAMETRÓW NATĘŻENIA ŚWIATŁA (BH1750)
// ============================================================================

struct LightMetrics {
    // Podstawowe parametry światła
    uint32_t lux_current;         // Aktualne natężenie [lux]
    float lux_mean;               // Średnie natężenie [lux]
    float lux_std;                // Odchylenie natężenia [lux]
    uint32_t lux_min;             // Minimalne natężenie [lux]
    uint32_t lux_max;             // Maksymalne natężenie [lux]
    
    // Cykl dobowy
    float daylight_duration;      // Czas trwania dnia [godziny]
    float night_duration;         // Czas trwania nocy [godziny]
    float dawn_dusk_detected;     // Wykrycie świtu/zmierzchu (0-1)
    
    // Wpływ na pszczoły
    float foraging_light_index;   // Indeks światła do wylotów (0-100%)
    float circadian_sync;         // Synchronizacja cyrkadiana (0-1)
    float hive_open_detection;    // Detekcja otwarcia ula (0-1)
    
    // Sezonowość
    float seasonal_light_change;  // Sezonowa zmiana światła [-1 do 1]
    float light_stability;        // Stabilność oświetlenia (0-100%)
    
    // Alerty
    float abnormal_light_flag;    // Flaga nieprawidłowego światła (0-1)
    float intrusion_light_event;  // Zdarzenie intruza (światło w nocy) (0-1)
};

LightMetrics currentLightMetrics;

// Deklaracje funkcji modułu światła
void calculateLightMetrics(LightMetrics& metrics);
bool detectHiveOpening(uint32_t lux, uint32_t prev_lux, bool is_night);

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
    udp.begin(random(1024, 65535)); // Losowy port źródłowy dla UDP
    Serial.println("UDP zainicjalizowany. Wysyłanie danych do: " + rpi_ip.toString() + ":" + String(UDP_PORT));
  } else {
    Serial.println("Błąd konfiguracji W6100!");
    while(true); // Zatrzymaj jeśli brak łącza
  }
}

// Wysyłanie danych sensorów przez UDP do Raspberry Pi
void sendUdpDataToRpi() {
  // Format: HIVE_ID,TEMP,HUM,WEIGHT,BAT,TIMESTAMP
  // Przykład: "UL-1,24.5,65.2,45.3,98,1234567890"
  
  char packetBuffer[128];
  long timestamp = millis() / 1000;
  
  // Oblicz wagę w kg (przybliżenie)
  float weight_kg = (float)hx711_value / 1000.0f;
  
  // Zakładamy stały poziom baterii (można rozszerzyć o monitoring napięcia)
  int battery_level = 95;
  
  snprintf(packetBuffer, sizeof(packetBuffer), 
           "UL-1,%.1f,%.1f,%.3f,%d,%ld",
           temperature, humidity, weight_kg, battery_level, timestamp);
  
  Serial.print("[UDP] Wysyłanie: ");
  Serial.println(packetBuffer);
  
  udp.beginPacket(rpi_ip, UDP_PORT);
  udp.write((uint8_t*)packetBuffer, strlen(packetBuffer));
  udp.endPacket();
  
  Serial.println("[UDP] Wysłano do RPi");
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

// Obliczanie RMS dźwięku (prosta wersja - zachowana dla kompatybilności)
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
// IMPLEMENTACJA MODUŁU PROFESJONALNEJ ANALIZY DŹWIĘKU
// ============================================================================

// Prosta implementacja FFT (Cooley-Tukey radix-2 algorithm)
// Globalne bufory dla FFT (część rzeczywista i urojona)
float fft_imag[FFT_SIZE]; // Dodatkowy bufor na część urojoną

// Dla mikrokontrolerów z ograniczoną pamięcią - PEŁNA implementacja FFT z liczbami zespolonymi
void performFFT(int16_t* input, float* output_real, int size) {
    float* output_imag = fft_imag; // Użycie globalnego bufora na część urojoną
    
    // Inicjalizacja - normalizacja wejścia do części rzeczywistej, urojona = 0
    for (int i = 0; i < size; i++) {
        output_real[i] = (float)input[i] / 2048.0f;  // Normalizacja do zakresu [-1, 1]
        output_imag[i] = 0.0f;
    }
    
    // Bit-reversal permutation (dla obu części)
    int j = 0;
    for (int i = 0; i < size - 1; i++) {
        if (i < j) {
            // Zamiana części rzeczywistej
            float temp_re = output_real[i];
            output_real[i] = output_real[j];
            output_real[j] = temp_re;
            
            // Zamiana części urojonej
            float temp_im = output_imag[i];
            output_imag[i] = output_imag[j];
            output_imag[j] = temp_im;
        }
        int k = size >> 1;
        while (k <= j && k > 0) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    
    // Pełna implementacja Cooley-Tukey FFT z liczbami zespolonymi
    for (int len = 2; len <= size; len <<= 1) {
        float angle = -2.0f * PI / len;
        float wlen_re = cos(angle);
        float wlen_im = sin(angle);
        
        for (int i = 0; i < size; i += len) {
            float w_re = 1.0f;
            float w_im = 0.0f;
            
            for (int k = 0; k < len / 2; k++) {
                int u = i + k;
                int v = i + k + len / 2;
                
                // PEŁNE mnożenie zespolone: (v_re + j*v_im) * (w_re + j*w_im)
                // t_re = v_re * w_re - v_im * w_im
                // t_im = v_re * w_im + v_im * w_re
                float t_re = output_real[v] * w_re - output_imag[v] * w_im;
                float t_im = output_real[v] * w_im + output_imag[v] * w_re;
                
                // Operacja motylkowa dla obu części
                float u_re = output_real[u];
                float u_im = output_imag[u];
                
                output_real[v] = u_re - t_re;
                output_imag[v] = u_im - t_im;
                
                output_real[u] = u_re + t_re;
                output_imag[u] = u_im + t_im;
                
                // Aktualizacja współczynnika obrotowego (pełne mnożenie zespolone)
                float new_w_re = w_re * wlen_re - w_im * wlen_im;
                float new_w_im = w_re * wlen_im + w_im * wlen_re;
                w_re = new_w_re;
                w_im = new_w_im;
            }
        }
    }
    
    // Obliczenie modułów widma (tylko pierwsza połowa - Nyquist)
    // |X[k]| = sqrt(re^2 + im^2)
    for (int i = 0; i < size / 2; i++) {
        output_real[i] = sqrt(output_real[i] * output_real[i] + output_imag[i] * output_imag[i]);
    }
}

// Obliczanie centrum widma (spectral centroid)
float calculateSpectralCentroid(const float* spectrum, int size, float sampleRate) {
    float numerator = 0.0f;
    float denominator = 0.0f;
    
    for (int i = 0; i < size; i++) {
        float freq = (float)i * sampleRate / (size * 2);
        numerator += freq * spectrum[i];
        denominator += spectrum[i];
    }
    
    if (denominator < 0.0001f) return 0.0f;
    return numerator / denominator;
}

// Obliczanie szerokości pasma widma (spectral bandwidth)
float calculateSpectralBandwidth(const float* spectrum, int size, float centroid, float sampleRate) {
    float numerator = 0.0f;
    float denominator = 0.0f;
    
    for (int i = 0; i < size; i++) {
        float freq = (float)i * sampleRate / (size * 2);
        float diff = freq - centroid;
        numerator += diff * diff * spectrum[i];
        denominator += spectrum[i];
    }
    
    if (denominator < 0.0001f) return 0.0f;
    return sqrt(numerator / denominator);
}

// Obliczanie płaskości widma (spectral flatness)
float calculateSpectralFlatness(const float* spectrum, int size) {
    float logSum = 0.0f;
    float linearSum = 0.0f;
    int validCount = 0;
    
    for (int i = 0; i < size; i++) {
        if (spectrum[i] > 0.0001f) {
            logSum += log(spectrum[i]);
            linearSum += spectrum[i];
            validCount++;
        }
    }
    
    if (validCount == 0 || linearSum < 0.0001f) return 0.0f;
    
    // Geometry mean / Arithmetic mean
    float geometricMean = exp(logSum / validCount);
    float arithmeticMean = linearSum / validCount;
    
    return geometricMean / arithmeticMean;
}

// Obliczanie entropii widmowej (spectral entropy)
float calculateSpectralEntropy(const float* spectrum, int size) {
    float totalEnergy = 0.0f;
    for (int i = 0; i < size; i++) {
        totalEnergy += spectrum[i];
    }
    
    if (totalEnergy < 0.0001f) return 0.0f;
    
    float entropy = 0.0f;
    for (int i = 0; i < size; i++) {
        float p = spectrum[i] / totalEnergy;
        if (p > 0.0001f) {
            entropy -= p * log(p);
        }
    }
    
    // Normalizacja entropii (max entropia = log(size))
    float maxEntropy = log(size);
    if (maxEntropy < 0.0001f) return 0.0f;
    
    return entropy / maxEntropy;
}

// Główna funkcja przetwarzania sygnału audio
void processAudioSignal() {
    static uint32_t lastProcessTime = 0;
    
    // Pobieranie próbek z mikrofonu MEMS
    noInterrupts();
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        audioBuffer[i] = analogRead(MIC_PIN) - 2048;  // Centrowanie wokół zera
        delayMicroseconds(250);  // ~4kHz sampling rate
    }
    interrupts();
    
    // Wykonanie FFT
    performFFT(audioBuffer, audioFFT, AUDIO_BUFFER_SIZE);
    
    // Obliczenie widma mocy
    for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
        audioSpectrum[i] = audioFFT[i] * audioFFT[i];
    }
    
    // Obliczenie wszystkich metryk audio
    calculateAudioMetrics(currentAudioMetrics);
    
    // Klasyfikacja zdarzeń akustycznych
    classifyAudioEvent(lastAudioEvent, currentAudioMetrics);
    
    // Aktualizacja historii
    updateAudioHistory(currentAudioMetrics);
    
    // Debug - wypisanie informacji o zdarzeniach
    if (lastAudioEvent.type != AudioEvent::EVENT_NONE) {
        Serial.print("[AUDIO_EVENT] ");
        Serial.print(audioEventTypeToString(lastAudioEvent.type));
        Serial.print(" | Wpływ: ");
        Serial.print(audioEventImpactToString(lastAudioEvent.impact));
        Serial.print(" | Pewność: ");
        Serial.print(lastAudioEvent.confidence, 2);
        Serial.print(" | Opis: ");
        Serial.println(lastAudioEvent.description);
    }
    
    // Aktualizacja globalnej zmiennej audio_rms
    audio_rms = currentAudioMetrics.rms_amplitude;
}

// Kompleksowe obliczanie metryk audio
void calculateAudioMetrics(AudioMetrics& metrics) {
    // Inicjalizacja
    memset(&metrics, 0, sizeof(AudioMetrics));
    
    // === PARAMETRY CZASOWE ===
    
    // Obliczanie wartości RMS, średniej, szczytowej
    float sum = 0.0f;
    float sumSq = 0.0f;
    float minVal = (float)audioBuffer[0] / 2048.0f;  // Inicjalizacja pierwszą próbką
    float maxVal = minVal;  // Inicjalizacja pierwszą próbką
    int zeroCrossings = 0;
    
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        float sample = (float)audioBuffer[i] / 2048.0f;  // Normalizacja
        sum += sample;
        sumSq += sample * sample;
        
        if (sample < minVal) minVal = sample;
        if (sample > maxVal) maxVal = sample;
        
        // Detekcja przejść przez zero
        if (i > 0) {
            float prevSample = (float)audioBuffer[i-1] / 2048.0f;
            if ((prevSample < 0 && sample >= 0) || (prevSample >= 0 && sample < 0)) {
                zeroCrossings++;
            }
        }
    }
    
    metrics.mean_amplitude = sum / AUDIO_BUFFER_SIZE;
    metrics.rms_amplitude = sqrt(sumSq / AUDIO_BUFFER_SIZE);
    metrics.peak_amplitude = max(abs(minVal), abs(maxVal));
    metrics.peak_to_peak = maxVal - minVal;
    metrics.signal_energy = sumSq;
    
    // Częstotliwość przejść przez zero (Zero Crossing Rate)
    float duration = (float)AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE;
    metrics.zero_crossing_rate = (float)zeroCrossings / (2.0f * duration);
    
    // === PARAMETRY STATYSTYCZNE ===
    
    // Odchylenie standardowe
    float variance = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        float sample = (float)audioBuffer[i] / 2048.0f;
        float diff = sample - metrics.mean_amplitude;
        variance += diff * diff;
    }
    variance /= AUDIO_BUFFER_SIZE;
    metrics.std_amplitude = sqrt(variance);
    
    // Współczynnik zmienności
    if (abs(metrics.mean_amplitude) > 0.0001f) {
        metrics.amplitude_cv = metrics.std_amplitude / abs(metrics.mean_amplitude);
    }
    
    // Asymetria (skewness) i kurtoza
    float sumCube = 0.0f;
    float sumQuad = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        float sample = (float)audioBuffer[i] / 2048.0f;
        float normDiff = (sample - metrics.mean_amplitude) / (metrics.std_amplitude + 0.0001f);
        sumCube += normDiff * normDiff * normDiff;
        sumQuad += normDiff * normDiff * normDiff * normDiff;
    }
    metrics.skewness = sumCube / AUDIO_BUFFER_SIZE;
    metrics.kurtosis = sumQuad / AUDIO_BUFFER_SIZE - 3.0f;  // Nadmiarowa kurtoza
    
    // === PARAMETRY CZĘSTOTLIWOŚCIOWE ===
    
    // Dominująca częstotliwość
    float maxPower = 0.0f;
    int maxBin = 0;
    for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
        if (audioSpectrum[i] > maxPower) {
            maxPower = audioSpectrum[i];
            maxBin = i;
        }
    }
    metrics.dominant_frequency = (float)maxBin * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SIZE;
    
    // Centrum widma
    metrics.spectral_centroid = calculateSpectralCentroid(audioSpectrum, AUDIO_BUFFER_SIZE / 2, AUDIO_SAMPLE_RATE);
    
    // Szerokość pasma
    metrics.spectral_bandwidth = calculateSpectralBandwidth(audioSpectrum, AUDIO_BUFFER_SIZE / 2, 
                                                            metrics.spectral_centroid, AUDIO_SAMPLE_RATE);
    
    // Płaskość widma
    metrics.spectral_flatness = calculateSpectralFlatness(audioSpectrum, AUDIO_BUFFER_SIZE / 2);
    
    // Entropia widmowa
    metrics.spectral_entropy = calculateSpectralEntropy(audioSpectrum, AUDIO_BUFFER_SIZE / 2);
    
    // Częstotliwość odcięcia (spectral rolloff - 85% energii)
    float totalEnergy = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
        totalEnergy += audioSpectrum[i];
    }
    float threshold = 0.85f * totalEnergy;
    float cumulativeEnergy = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
        cumulativeEnergy += audioSpectrum[i];
        if (cumulativeEnergy >= threshold) {
            metrics.spectral_rolloff = (float)i * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SIZE;
            break;
        }
    }
    
    // === MOC W PASmach ===
    
    int binBeeMin = (int)(BEE_FREQ_MIN * AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE);
    int binBeeMax = (int)(BEE_FREQ_MAX * AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE);
    int binSwarmMin = (int)(SWARM_FREQ_MIN * AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE);
    int binSwarmMax = (int)(SWARM_FREQ_MAX * AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE);
    int binLowMax = binBeeMin;
    int binHighMin = binBeeMax;
    
    float powerBee = 0.0f, powerSwarm = 0.0f, powerLow = 0.0f, powerHigh = 0.0f;
    int countBee = 0, countSwarm = 0, countLow = 0, countHigh = 0;
    
    for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
        if (i < binLowMax) {
            powerLow += audioSpectrum[i];
            countLow++;
        } else if (i < binBeeMax) {
            powerBee += audioSpectrum[i];
            countBee++;
            if (i >= binSwarmMin && i < binSwarmMax) {
                powerSwarm += audioSpectrum[i];
                countSwarm++;
            }
        } else {
            powerHigh += audioSpectrum[i];
            countHigh++;
        }
    }
    
    // Konwersja do dB (z zabezpieczeniem przed log(0))
    metrics.power_in_bee_band = (countBee > 0 && powerBee > 0.0001f) ? 
                                10.0f * log10(powerBee / countBee) : -100.0f;
    metrics.power_in_swarm_band = (countSwarm > 0 && powerSwarm > 0.0001f) ? 
                                  10.0f * log10(powerSwarm / countSwarm) : -100.0f;
    metrics.power_low_freq = (countLow > 0 && powerLow > 0.0001f) ? 
                             10.0f * log10(powerLow / countLow) : -100.0f;
    metrics.power_high_freq = (countHigh > 0 && powerHigh > 0.0001f) ? 
                              10.0f * log10(powerHigh / countHigh) : -100.0f;
    
    // Stosunek harmonicznych (uproszczony)
    if (powerBee > 0.0001f) {
        metrics.harmonic_ratio = (powerSwarm + powerHigh) / powerBee;
    }
    
    // === WSKAŹNIKI KLASYFIKACJI ===
    
    // Indeks aktywności pszczół (na podstawie mocy w paśmie pszczół)
    float normalizedBeePower = constrain((metrics.power_in_bee_band + 60.0f) / 60.0f, 0.0f, 1.0f);
    metrics.bee_activity_index = normalizedBeePower * 100.0f;
    
    // Prawdopodobieństwo rojenia (na podstawie mocy w paśmie rojenia i wzorców)
    float swarmFactor = 0.0f;
    if (metrics.power_in_swarm_band > -40.0f) {
        swarmFactor = constrain((metrics.power_in_swarm_band + 40.0f) / 30.0f, 0.0f, 1.0f);
    }
    // Dodatkowe czynniki: dominująca częstotliwość w zakresie rojenia
    if (metrics.dominant_frequency >= SWARM_FREQ_MIN && metrics.dominant_frequency <= SWARM_FREQ_MAX) {
        swarmFactor = max(swarmFactor, 0.5f);
    }
    metrics.swarm_probability = swarmFactor;
    
    // Wskaźnik stresu (wysoka energia w wysokich częstotliwościach)
    float stressFactor = 0.0f;
    if (metrics.power_high_freq > -50.0f) {
        stressFactor = constrain((metrics.power_high_freq + 50.0f) / 40.0f, 0.0f, 1.0f);
    }
    metrics.stress_indicator = stressFactor;
    
    // Indeks zdrowia ula (kombinacja czynników)
    float healthScore = 100.0f;
    healthScore -= metrics.stress_indicator * 20.0f;  // Stres obniża zdrowie
    healthScore -= abs(50.0f - metrics.bee_activity_index) * 0.4f;  // Optymalna aktywność ~50%
    healthScore = constrain(healthScore, 0.0f, 100.0f);
    metrics.hive_health_audio = healthScore;
    
    // Uproszczone MFCC (tylko energia w pasmach)
    metrics.mfcc_energy[0] = powerLow;
    metrics.mfcc_energy[1] = powerBee;
    metrics.mfcc_energy[2] = powerSwarm;
    metrics.mfcc_energy[3] = powerHigh;
    
    // Kontrast widmowy (różnica między maks a min w pasmach)
    float maxBandPower = max(max(powerLow, powerBee), max(powerSwarm, powerHigh));
    float minBandPower = min(min(powerLow, powerBee), min(powerSwarm, powerHigh));
    if (maxBandPower > 0.0001f) {
        metrics.spectral_contrast = (maxBandPower - minBandPower) / maxBandPower;
    }
    
    // Siła tonalna (na podstawie płaskości widma - niskie wartości = bardziej tonalne)
    metrics.tonal_strength = 1.0f - metrics.spectral_flatness;
    
    // === NOWE PARAMETRY - DODATKOWE OBLICZENIA (18 parametrów) ===
    
    // Współczynnik szczytu (Crest Factor) - detekcja impulsów
    if (metrics.rms_amplitude > 0.0001f) {
        metrics.crest_factor = metrics.peak_amplitude / metrics.rms_amplitude;
    }
    
    // Szacowanie formantów (uproszczone - piki w widmie)
    int formantCount = 0;
    float prevSlope = 0.0f;
    for (int i = 2; i < AUDIO_BUFFER_SIZE / 4 && formantCount < 2; i++) {
        float slope = audioSpectrum[i] - audioSpectrum[i-1];
        if (prevSlope > 0 && slope < 0 && audioSpectrum[i] > 0.1f * maxPower) {
            // Wykryto pik - potencjalny formant
            float freq = (float)i * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SIZE;
            if (formantCount == 0) {
                metrics.formant_freq_1 = freq;
                // Szerokość pasma - odległość między punktami -3dB
                int leftBin = i, rightBin = i;
                float threshold = audioSpectrum[i] * 0.707f;  // -3dB
                while (leftBin > 0 && audioSpectrum[leftBin] > threshold) leftBin--;
                while (rightBin < AUDIO_BUFFER_SIZE / 4 && audioSpectrum[rightBin] > threshold) rightBin++;
                metrics.formant_bandwidth_1 = (float)(rightBin - leftBin) * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SIZE;
            } else if (formantCount == 1 && freq > metrics.formant_freq_1 + 50.0f) {
                metrics.formant_freq_2 = freq;
                int leftBin = i, rightBin = i;
                float threshold = audioSpectrum[i] * 0.707f;
                while (leftBin > 0 && audioSpectrum[leftBin] > threshold) leftBin--;
                while (rightBin < AUDIO_BUFFER_SIZE / 4 && audioSpectrum[rightBin] > threshold) rightBin++;
                metrics.formant_bandwidth_2 = (float)(rightBin - leftBin) * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SIZE;
            }
            formantCount++;
        }
        prevSlope = slope;
    }
    
    // Częstotliwość podstawowa (F0) - użycie dominant frequency z korektą
    metrics.fundamental_frequency = metrics.dominant_frequency;
    
    // Siła wysokości dźwięku (pitch strength) - na podstawie ostrości piku widmowego
    float pitchSharpness = 0.0f;
    if (maxBin > 0 && maxBin < AUDIO_BUFFER_SIZE / 2 - 1) {
        float neighborAvg = (audioSpectrum[maxBin - 1] + audioSpectrum[maxBin + 1]) / 2.0f;
        if (neighborAvg > 0.0001f) {
            pitchSharpness = (maxPower - neighborAvg) / neighborAvg;
        }
    }
    metrics.pitch_strength = constrain(pitchSharpness / 10.0f, 0.0f, 1.0f);
    
    // Nieharmoniczność - miara odchylenia od harmonicznych
    float inharmonicSum = 0.0f;
    if (metrics.fundamental_frequency > 50.0f) {
        for (int h = 2; h <= 6; h++) {
            float harmonicFreq = metrics.fundamental_frequency * h;
            int bin = (int)(harmonicFreq * AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE);
            if (bin >= 0 && bin < AUDIO_BUFFER_SIZE / 2) {
                // Sprawdź odchylenie od idealnej harmonicznej
                float actualFreq = (float)bin * AUDIO_SAMPLE_RATE / AUDIO_BUFFER_SIZE;
                float deviation = abs(actualFreq - harmonicFreq) / metrics.fundamental_frequency;
                inharmonicSum += deviation;
            }
        }
        metrics.inharmonicity = constrain(inharmonicSum / 5.0f, 0.0f, 1.0f);
    }
    
    // Shimmer i Jitter - fluktuacje amplitudy i częstotliwości (uproszczone)
    // Wymagałyby analizy wielu ramek czasowych - tutaj wartości bazowe
    float amplitudeVariance = 0.0f;
    float periodVariance = 0.0f;
    int periodCount = 0;
    
    // Detekcja okresów dla jitter
    int zeroCrossPeriods[10];
    int lastZeroCross = -1;
    int periodIdx = 0;
    for (int i = 1; i < AUDIO_BUFFER_SIZE && periodIdx < 10; i++) {
        if ((float)audioBuffer[i-1] < 0 && (float)audioBuffer[i] >= 0) {
            if (lastZeroCross >= 0) {
                zeroCrossPeriods[periodIdx++] = i - lastZeroCross;
            }
            lastZeroCross = i;
        }
    }
    
    if (periodIdx >= 2) {
        float meanPeriod = 0.0f;
        for (int i = 0; i < periodIdx; i++) meanPeriod += zeroCrossPeriods[i];
        meanPeriod /= periodIdx;
        
        for (int i = 0; i < periodIdx; i++) {
            periodVariance += abs(zeroCrossPeriods[i] - meanPeriod) / meanPeriod;
        }
        metrics.jitter = constrain(periodVariance / periodIdx * 100.0f, 0.0f, 100.0f);
    }
    
    // Shimmer - fluktuacja amplitudy między okresami
    float peakAmps[10];
    int peakCount = 0;
    for (int i = 1; i < AUDIO_BUFFER_SIZE - 1 && peakCount < 10; i++) {
        if (abs(audioBuffer[i]) > abs(audioBuffer[i-1]) && abs(audioBuffer[i]) > abs(audioBuffer[i+1])) {
            if (abs(audioBuffer[i]) > 100) {  // Próg szumu
                peakAmps[peakCount++] = abs(audioBuffer[i]);
            }
        }
    }
    
    if (peakCount >= 2) {
        float meanPeak = 0.0f;
        for (int i = 0; i < peakCount; i++) meanPeak += peakAmps[i];
        meanPeak /= peakCount;
        
        for (int i = 0; i < peakCount - 1; i++) {
            amplitudeVariance += abs(peakAmps[i+1] - peakAmps[i]) / meanPeak;
        }
        metrics.shimmer = constrain(amplitudeVariance / (peakCount - 1) * 100.0f, 0.0f, 100.0f);
    }
    
    // Noise-to-Harmonic Ratio (NHR) i Harmonic-to-Noise Ratio (HNR)
    float harmonicEnergy = 0.0f;
    float noiseEnergy = 0.0f;
    if (metrics.fundamental_frequency > 50.0f) {
        int fundBin = (int)(metrics.fundamental_frequency * AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE);
        int bandwidth = max(2, (int)(fundBin * 0.1f));  // 10% szerokości pasma
        
        for (int i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
            bool isHarmonic = false;
            for (int h = 1; h <= 8; h++) {
                int harmBin = fundBin * h;
                if (abs(i - harmBin) <= bandwidth) {
                    isHarmonic = true;
                    break;
                }
            }
            if (isHarmonic) {
                harmonicEnergy += audioSpectrum[i];
            } else {
                noiseEnergy += audioSpectrum[i];
            }
        }
    }
    
    if (harmonicEnergy > 0.0001f) {
        metrics.noise_to_harmonic_ratio = noiseEnergy / harmonicEnergy;
    }
    if (noiseEnergy > 0.0001f) {
        metrics.harmonic_to_noise_ratio = harmonicEnergy / noiseEnergy;
    }
    
    // Autokorelacja - miara periodyczności
    float maxAutocorr = 0.0f;
    int bestLag = 0;
    for (int lag = 5; lag < AUDIO_BUFFER_SIZE / 2; lag++) {
        float autocorr = 0.0f;
        for (int i = 0; i < AUDIO_BUFFER_SIZE - lag; i++) {
            autocorr += (float)audioBuffer[i] * (float)audioBuffer[i + lag];
        }
        autocorr /= (AUDIO_BUFFER_SIZE - lag);
        if (autocorr > maxAutocorr) {
            maxAutocorr = autocorr;
            bestLag = lag;
        }
    }
    if (maxAutocorr > 0.0f) {
        float normAutocorr = maxAutocorr / sumSq;  // Normalizacja do energii sygnału
        metrics.autocorrelation_peak = constrain(normAutocorr, 0.0f, 1.0f);
    }
    
    // Parametry obwiedni (attack, decay, sustain)
    int attackEnd = 0;
    float maxAmpTime = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        float sample = abs((float)audioBuffer[i]);
        if (sample > maxAmpTime) {
            maxAmpTime = sample;
            attackEnd = i;
        }
    }
    metrics.attack_time = (float)attackEnd * 1000.0f / AUDIO_SAMPLE_RATE;  // [ms]
    
    // Decay time - czas spadku do 50% po szczycie
    int decayEnd = attackEnd;
    float threshold50 = maxAmpTime * 0.5f;
    for (int i = attackEnd; i < AUDIO_BUFFER_SIZE; i++) {
        if (abs((float)audioBuffer[i]) < threshold50) {
            decayEnd = i;
            break;
        }
    }
    metrics.decay_time = (float)(decayEnd - attackEnd) * 1000.0f / AUDIO_SAMPLE_RATE;  // [ms]
    
    // Sustain level - średni poziom po zaniku
    float sustainSum = 0.0f;
    int sustainCount = 0;
    for (int i = decayEnd; i < AUDIO_BUFFER_SIZE; i++) {
        sustainSum += abs((float)audioBuffer[i]);
        sustainCount++;
    }
    if (sustainCount > 0 && maxAmpTime > 0.0001f) {
        metrics.sustain_level = constrain((sustainSum / sustainCount) / maxAmpTime, 0.0f, 1.0f);
    }
    
    // Temporal centroid - centrum czasowe sygnału
    float temporalMoment = 0.0f;
    float totalAbsAmp = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        float absAmp = abs((float)audioBuffer[i]);
        temporalMoment += i * absAmp;
        totalAbsAmp += absAmp;
    }
    if (totalAbsAmp > 0.0001f) {
        metrics.temporal_centroid = (temporalMoment / totalAbsAmp) * 1000.0f / AUDIO_SAMPLE_RATE;  // [ms]
    }
    
    // Głośność psychoakustyczna (uproszczony model Zwicker'a)
    // Uproszczenie: głośność zależy od RMS i rozkładu częstotliwościowego
    float loudnessFactor = 1.0f;
    if (metrics.spectral_centroid > 2000.0f) {
        loudnessFactor = 1.2f;  // Wyższe częstotliwości są głośniejsze perceptualnie
    } else if (metrics.spectral_centroid < 200.0f) {
        loudnessFactor = 0.8f;  // Niskie częstotliwości cichsze
    }
    metrics.loudness_zwicker = metrics.rms_amplitude * loudnessFactor * 10.0f;  // [sones], przeskalowane
    
    // === NOWE WSKAŹNIKI KLASYFIKACJI ===
    
    // Efektywność zbierania nektaru (foraging efficiency)
    // Na podstawie aktywności w paśmie pszczół i regularności sygnału
    float activityScore = metrics.bee_activity_index / 100.0f;
    float regularityScore = metrics.autocorrelation_peak;
    float lowStressBonus = (1.0f - metrics.stress_indicator) * 0.3f;
    metrics.foraging_efficiency = constrain((activityScore * 0.5f + regularityScore * 0.2f + lowStressBonus) * 100.0f, 0.0f, 100.0f);
    
    // Spójność kolonii (colony coherence)
    // Na podstawie synchronizacji sygnałów (niski shimmer/jitter = wysoka spójność)
    float shimmerScore = 1.0f - constrain(metrics.shimmer / 20.0f, 0.0f, 1.0f);
    float jitterScore = 1.0f - constrain(metrics.jitter / 20.0f, 0.0f, 1.0f);
    float spectralStability = 1.0f - metrics.spectral_entropy;  // Niska entropia = stabilne widmo
    metrics.colony_coherence = constrain((shimmerScore * 0.4f + jitterScore * 0.4f + spectralStability * 0.2f), 0.0f, 1.0f);
}

// Klasyfikacja zdarzeń akustycznych
void classifyAudioEvent(AudioEvent& event, const AudioMetrics& metrics) {
    event.timestamp = millis();
    event.confidence = 0.0f;
    event.type = AudioEvent::EVENT_NONE;
    event.impact = AudioEvent::IMPACT_NEUTRAL;
    strcpy(event.description, "Brak zdarzenia");
    
    // Sprawdzenie różnych warunków zdarzeń
    
    // 1. Wykrywanie rojenia (aktywne)
    if (metrics.swarm_probability > 0.7f && metrics.bee_activity_index > 60.0f) {
        event.type = AudioEvent::EVENT_SWARM_ACTIVE;
        event.impact = AudioEvent::IMPACT_CRITICAL;
        event.confidence = metrics.swarm_probability;
        strcpy(event.description, "Aktywne rojenie wykryte!");
        return;
    }
    
    // 2. Przygotowania do rojenia
    if (metrics.swarm_probability > 0.5f && metrics.power_in_swarm_band > -35.0f) {
        event.type = AudioEvent::EVENT_SWARM_PREPARATION;
        event.impact = AudioEvent::IMPACT_NEGATIVE;
        event.confidence = metrics.swarm_probability;
        strcpy(event.description, "Przygotowania do rojenia");
        return;
    }
    
    // 3. Piszczenie królowej
    if (metrics.dominant_frequency >= QUEEN_PIP_FREQ_MIN && 
        metrics.dominant_frequency <= QUEEN_PIP_FREQ_MAX &&
        metrics.tonal_strength > 0.6f) {
        event.type = AudioEvent::EVENT_QUEEN_PIPING;
        event.impact = AudioEvent::IMPACT_POSITIVE;
        event.confidence = metrics.tonal_strength;
        strcpy(event.description, "Piszczenie królowej wykryte");
        return;
    }
    
    // 4. Zwiększona aktywność
    if (metrics.bee_activity_index > 75.0f && metrics.stress_indicator < 0.4f) {
        event.type = AudioEvent::EVENT_INCREASED_ACTIVITY;
        event.impact = AudioEvent::IMPACT_POSITIVE;
        event.confidence = (metrics.bee_activity_index - 75.0f) / 25.0f;
        strcpy(event.description, "Zwiększona aktywność pszczół - dobry pożytek");
        return;
    }
    
    // 5. Niska aktywność
    if (metrics.bee_activity_index < 20.0f) {
        event.type = AudioEvent::EVENT_LOW_ACTIVITY;
        event.impact = AudioEvent::IMPACT_NEGATIVE;
        event.confidence = (20.0f - metrics.bee_activity_index) / 20.0f;
        strcpy(event.description, "Niska aktywność pszczół");
        return;
    }
    
    // 6. Sygnały distress (wysokie częstotliwości, wysoki stres)
    if (metrics.stress_indicator > 0.7f && metrics.power_high_freq > -40.0f) {
        event.type = AudioEvent::EVENT_DISTRESS;
        event.impact = AudioEvent::IMPACT_CRITICAL;
        event.confidence = metrics.stress_indicator;
        strcpy(event.description, "Sygnały distress - możliwe zagrożenie");
        return;
    }
    
    // 7. Zagrożenie drapieżnikiem (nagłe zmiany, specyficzne wzorce)
    if (metrics.spectral_contrast > 0.8f && metrics.zero_crossing_rate > 300.0f) {
        event.type = AudioEvent::EVENT_PREDATOR_THREAT;
        event.impact = AudioEvent::IMPACT_CRITICAL;
        event.confidence = metrics.spectral_contrast;
        strcpy(event.description, "Możliwe zagrożenie drapieżnikiem");
        return;
    }
    
    // Domyślnie: normalna aktywność
    if (metrics.bee_activity_index >= 20.0f && metrics.bee_activity_index <= 75.0f) {
        event.type = AudioEvent::EVENT_NORMAL_ACTIVITY;
        event.impact = AudioEvent::IMPACT_POSITIVE;
        event.confidence = 1.0f - abs(50.0f - metrics.bee_activity_index) / 50.0f;
        strcpy(event.description, "Normalna praca pszczół");
    }
}

// Aktualizacja historii danych audio
void updateAudioHistory(const AudioMetrics& metrics) {
    audioHistory[audioHistoryIndex].timestamp = millis();
    audioHistory[audioHistoryIndex].rms_value = metrics.rms_amplitude;
    audioHistory[audioHistoryIndex].dominant_freq = metrics.dominant_frequency;
    audioHistory[audioHistoryIndex].bee_activity = metrics.bee_activity_index;
    audioHistory[audioHistoryIndex].swarm_prob = metrics.swarm_probability;
    
    audioHistoryIndex = (audioHistoryIndex + 1) % AUDIO_HISTORY_SIZE;
    
    if (audioHistoryCount < AUDIO_HISTORY_SIZE) {
        audioHistoryCount++;
    }
}

// Sprawdzenie czy zdarzenie ma pozytywny wpływ na pożytek
bool isPositiveAudioChange(const AudioEvent& event) {
    return (event.impact == AudioEvent::IMPACT_POSITIVE);
}

// Konwersja typu zdarzenia na string
const char* audioEventTypeToString(AudioEvent::EventType type) {
    switch(type) {
        case AudioEvent::EVENT_NONE: return "BRAK_ZDARZENIA";
        case AudioEvent::EVENT_NORMAL_ACTIVITY: return "NORMALNA_AKTYWNOSC";
        case AudioEvent::EVENT_INCREASED_ACTIVITY: return "ZWIEKSZONA_AKTYWNOSC";
        case AudioEvent::EVENT_SWARM_PREPARATION: return "PRZYGOTOWANIA_DO_ROJENIA";
        case AudioEvent::EVENT_SWARM_ACTIVE: return "AKTYWNE_ROJENIE";
        case AudioEvent::EVENT_QUEEN_PIPING: return "PISZCZENIE_KROLOWEJ";
        case AudioEvent::EVENT_PREDATOR_THREAT: return "ZAGROZENIE_DRAPIEZNIKIEM";
        case AudioEvent::EVENT_DISTRESS: return "SYGNALY_DISTRESS";
        case AudioEvent::EVENT_LOW_ACTIVITY: return "NISKA_AKTYWNOSC";
        default: return "NIEZNANE";
    }
}

// Konwersja wpływu zdarzenia na string
const char* audioEventImpactToString(AudioEvent::EventImpact impact) {
    switch(impact) {
        case AudioEvent::IMPACT_NEUTRAL: return "NEUTRALNY";
        case AudioEvent::IMPACT_POSITIVE: return "POZYTYWNY";
        case AudioEvent::IMPACT_NEGATIVE: return "NEGATYWNY";
        case AudioEvent::IMPACT_CRITICAL: return "KRYTYCZNY";
        default: return "NIEZNANY";
    }
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

// Aktualizacja dziennego wzorca wagi
void updateHX711DailyPattern(const HX711DataPoint& point) {
    uint8_t hour = (point.timestamp / 3600000UL) % 24;  // Godzina z timestampu
    
    // Aktualizacja średniej ruchomej dla danej godziny
    uint8_t& samples = hx711DailyPattern.valid_samples[hour];
    float& avg = hx711DailyPattern.hour_avg[hour];
    
    if (samples < 255) samples++;
    
    // Średnia krocząca
    avg = ((avg * (samples - 1)) + point.weight_filtered) / samples;
}

// Detekcja zdarzeń na podstawie metryk HX711
HX711Event detectHX711Events(const HX711DataPoint& current, const HX711Metrics& metrics) {
    HX711Event event;
    event.type = HX711Event::EVENT_NONE;
    event.impact = HX711Event::IMPACT_NEUTRAL;
    event.confidence = 0.0f;
    event.magnitude = 0.0f;
    event.timestamp = millis();
    memset(event.description, 0, sizeof(event.description));
    
    // Detekcja rozpoczęcia przepływu nektaru
    if (current.rate_of_change > hx711Thresholds.nectar_flow_threshold) {
        event.type = HX711Event::EVENT_NECTAR_FLOW_START;
        event.impact = HX711Event::IMPACT_VERY_POSITIVE;
        event.confidence = min(1.0f, current.rate_of_change / 0.2f);
        event.magnitude = current.rate_of_change;
        snprintf(event.description, sizeof(event.description), 
                 "Rozpoczęcie intensywnego przepływu nektaru: %.3f kg/h", current.rate_of_change);
        return event;
    }
    
    // Detekcja szczytu przepływu nektaru
    if (metrics.nectar_inflow_rate > 0.15f && metrics.trend_direction == 1.0f) {
        event.type = HX711Event::EVENT_NECTAR_FLOW_PEAK;
        event.impact = HX711Event::IMPACT_POSITIVE;
        event.confidence = min(1.0f, metrics.nectar_inflow_rate / 0.3f);
        event.magnitude = metrics.nectar_inflow_rate;
        snprintf(event.description, sizeof(event.description), 
                 "Szczyt przepływu nektaru: %.3f kg/h", metrics.nectar_inflow_rate);
        return event;
    }
    
    // Detekcja wysokiego zużycia zapasów
    if (metrics.consumption_rate > hx711Thresholds.consumption_alert) {
        event.type = HX711Event::EVENT_HIGH_CONSUMPTION;
        event.impact = HX711Event::IMPACT_NEGATIVE;
        event.confidence = min(1.0f, metrics.consumption_rate / 1.0f);
        event.magnitude = metrics.consumption_rate;
        snprintf(event.description, sizeof(event.description), 
                 "Wysokie zużycie zapasów: %.3f kg/h (%.2f kg/dzień)", 
                 metrics.consumption_rate, metrics.daily_consumption);
        return event;
    }
    
    // Detekcja niskiego zapasu żywności
    if (metrics.mean_weight < hx711Thresholds.food_reserve_low) {
        event.type = HX711Event::EVENT_LOW_FOOD_RESERVE;
        if (metrics.mean_weight < hx711Thresholds.food_reserve_critical) {
            event.impact = HX711Event::IMPACT_CRITICAL;
            event.confidence = 0.95f;
        } else {
            event.impact = HX711Event::IMPACT_NEGATIVE;
            event.confidence = 0.8f;
        }
        event.magnitude = metrics.mean_weight;
        snprintf(event.description, sizeof(event.description), 
                 "Niski zapas żywności: %.2f kg (wystarczy na %.1f dni)", 
                 metrics.mean_weight, metrics.food_reserve_days);
        return event;
    }
    
    // Detekcja wyjścia roju (nagły spadek wagi)
    if (current.rate_of_change < -hx711Thresholds.swarm_loss_threshold && 
        abs(current.rate_of_change) > abs(metrics.mean_rate) * 3.0f) {
        event.type = HX711Event::EVENT_SWARM_DEPARTURE;
        event.impact = HX711Event::IMPACT_VERY_NEGATIVE;
        event.confidence = min(1.0f, abs(current.rate_of_change) / 3.0f);
        event.magnitude = abs(current.rate_of_change);
        snprintf(event.description, sizeof(event.description), 
                 "Podejrzenie wyjścia roju! Nagły spadek wagi: %.2f kg", 
                 abs(current.rate_of_change));
        return event;
    }
    
    // Detekcja anomalii
    if (metrics.anomaly_score > 0.7f) {
        event.type = HX711Event::EVENT_MEasurement_ERROR;
        event.impact = HX711Event::IMPACT_SLIGHTLY_NEGATIVE;
        event.confidence = metrics.anomaly_score;
        event.magnitude = metrics.sudden_change_mag;
        snprintf(event.description, sizeof(event.description), 
                 "Wykryto anomalię w pomiarach wagi (score: %.2f)", metrics.anomaly_score);
        return event;
    }
    
    // Sezonowe zdarzenia
    if (metrics.winter_readiness > 80.0f && metrics.trend_direction == 1.0f) {
        event.type = HX711Event::EVENT_WINTER_CLUSTER;
        event.impact = HX711Event::IMPACT_SLIGHTLY_POSITIVE;
        event.confidence = metrics.winter_readiness / 100.0f;
        event.magnitude = metrics.mean_weight;
        snprintf(event.description, sizeof(event.description), 
                 "Dobra gotowość do zimowli: %.1f%% (waga: %.2f kg)", 
                 metrics.winter_readiness, metrics.mean_weight);
        return event;
    }
    
    if (metrics.colony_growth_rate > 2.0f && metrics.foraging_efficiency > 40.0f) {
        event.type = HX711Event::EVENT_SPRING_BUILDUP;
        event.impact = HX711Event::IMPACT_POSITIVE;
        event.confidence = min(1.0f, metrics.colony_growth_rate / 5.0f);
        event.magnitude = metrics.colony_growth_rate;
        snprintf(event.description, sizeof(event.description), 
                 "Wiosenny rozwój kolonii: wzrost %.2f%%/dzień", metrics.colony_growth_rate);
        return event;
    }
    
    return event;
}

// Sprawdzenie czy zdarzenie ma pozytywny wpływ na pożytek
bool isPositiveHX711Change(const HX711Event& event) {
    switch (event.impact) {
        case HX711Event::IMPACT_VERY_POSITIVE:
        case HX711Event::IMPACT_POSITIVE:
        case HX711Event::IMPACT_SLIGHTLY_POSITIVE:
            return true;
        default:
            return false;
    }
}

// Konwersja typu zdarzenia na string
const char* hx711EventTypeToString(HX711Event::EventType type) {
    switch (type) {
        case HX711Event::EVENT_NECTAR_FLOW_START: return "NEKTAR_START";
        case HX711Event::EVENT_NECTAR_FLOW_PEAK: return "NEKTAR_PEAK";
        case HX711Event::EVENT_NECTAR_FLOW_END: return "NEKTAR_END";
        case HX711Event::EVENT_HONEY_SUPER_ADDED: return "MIODNIA_DODANA";
        case HX711Event::EVENT_HONEY_HARVESTED: return "ZBIOR_MIODU";
        case HX711Event::EVENT_HIGH_CONSUMPTION: return "WYSOKIE_ZUZYCIE";
        case HX711Event::EVENT_LOW_FOOD_RESERVE: return "NISKI_ZAPAS";
        case HX711Event::EVENT_SWARM_DEPARTURE: return "ROJENIE";
        case HX711Event::EVENT_PREDATOR_ATTACK: return "ATAK_DRAPIEZNIKA";
        case HX711Event::EVENT_WEATHER_IMPACT: return "POGODA";
        case HX711Event::EVENT_DRONE_EJECTION: return "TRUTNIE";
        case HX711Event::EVENT_WINTER_CLUSTER: return "KLASTER_ZIMOWY";
        case HX711Event::EVENT_SPRING_BUILDUP: return "WIOSNA_ROZWOJ";
        case HX711Event::EVENT_MEasurement_ERROR: return "BLAD_POMIARU";
        default: return "BRAK_ZDARZENIA";
    }
}

// Konwersja wpływu zdarzenia na string
const char* hx711EventImpactToString(HX711Event::EventImpact impact) {
    switch (impact) {
        case HX711Event::IMPACT_VERY_POSITIVE: return "BARDZO_POZYTYWNY";
        case HX711Event::IMPACT_POSITIVE: return "POZYTYWNY";
        case HX711Event::IMPACT_SLIGHTLY_POSITIVE: return "LEKKO_POZYTYWNY";
        case HX711Event::IMPACT_NEUTRAL: return "NEUTRALNY";
        case HX711Event::IMPACT_SLIGHTLY_NEGATIVE: return "LEKKO_NEGATYWNY";
        case HX711Event::IMPACT_NEGATIVE: return "NEGATYWNY";
        case HX711Event::IMPACT_VERY_NEGATIVE: return "BARDZO_NEGATYWNY";
        case HX711Event::IMPACT_CRITICAL: return "KRYTYCZNY";
        default: return "NIEZNANY";
    }
}

// Generowanie opisu zdarzenia
void generateHX711EventDescription(HX711Event& event) {
    // Opis jest już generowany w detectHX711Events
    // Ta funkcja może być użyta do dodatkowego formatowania
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI MODUŁU HX711
// ============================================================================

// Aktualizacja bufora cyrkularnego HX711
void updateHX711Buffer(const HX711DataPoint& point) {
    hx711Buffer[hx711BufferIndex] = point;
    hx711BufferIndex = (hx711BufferIndex + 1) % HX711_BUFFER_SIZE;
    
    if (hx711DataCount < HX711_BUFFER_SIZE) {
        hx711DataCount++;
    }
}

// Filtrowanie wagi - wygładzanie wykładnicze (EMA)
float applyWeightFilter(float raw_weight, float prev_filtered, float alpha) {
    return alpha * raw_weight + (1.0f - alpha) * prev_filtered;
}

// Obliczanie średniej ruchomej
float calculateMovingAverage(const float* data, uint16_t count, uint8_t window) {
    if (count == 0 || window == 0) return 0.0f;
    
    uint16_t actual_window = min((uint16_t)window, count);
    float sum = 0.0f;
    
    for (uint16_t i = 0; i < actual_window; i++) {
        sum += data[count - 1 - i];
    }
    
    return sum / actual_window;
}

// Regresja liniowa - obliczanie nachylenia i punktu przecięcia
float calculateLinearRegression(const float* x, const float* y, uint16_t count, float& slope, float& intercept) {
    if (count < 2) {
        slope = 0.0f;
        intercept = 0.0f;
        return 0.0f;
    }
    
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
    
    for (uint16_t i = 0; i < count; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
    }
    
    float denom = count * sumX2 - sumX * sumX;
    if (abs(denom) < 1e-10f) {
        slope = 0.0f;
        intercept = sumY / count;
        return 0.0f;
    }
    
    slope = (count * sumXY - sumX * sumY) / denom;
    intercept = (sumY - slope * sumX) / count;
    
    // Obliczanie współczynnika korelacji
    float meanY = sumY / count;
    float ssTot = 0.0f, ssRes = 0.0f;
    
    for (uint16_t i = 0; i < count; i++) {
        float yPred = slope * x[i] + intercept;
        ssTot += (y[i] - meanY) * (y[i] - meanY);
        ssRes += (y[i] - yPred) * (y[i] - yPred);
    }
    
    if (ssTot < 1e-10f) return 0.0f;
    return sqrtf(max(0.0f, 1.0f - ssRes / ssTot));
}

// Obliczanie percentyla
float calculatePercentile(const float* data, uint16_t count, float percentile) {
    if (count == 0) return 0.0f;
    
    // Prosta implementacja - sortowanie bąbelkowe dla małych zbiorów
    float sorted[HX711_BUFFER_SIZE];
    for (uint16_t i = 0; i < count && i < HX711_BUFFER_SIZE; i++) {
        sorted[i] = data[i];
    }
    
    // Sortowanie
    for (uint16_t i = 0; i < count - 1; i++) {
        for (uint16_t j = 0; j < count - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                float temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    // Obliczanie indeksu percentyla
    float index = (percentile / 100.0f) * (count - 1);
    uint16_t lower = (uint16_t)index;
    uint16_t upper = min((uint16_t)(lower + 1), count - 1);
    float frac = index - lower;
    
    return sorted[lower] + frac * (sorted[upper] - sorted[lower]);
}

// Kompensacja temperaturowa wagi
void compensateTemperature(float& weight, float temperature) {
    // Współczynnik temperaturowy typowy dla tensometrów: ~0.01%/°C
    const float tempCoeff = 0.0001f;  // 0.01% na °C
    const float refTemp = 25.0f;      // Temperatura referencyjna
    
    float tempDelta = temperature - refTemp;
    weight = weight / (1.0f + tempCoeff * tempDelta);
}

// Obliczanie odchylenia standardowego
static float calculateStdDev(const float* data, uint16_t count, float mean) {
    if (count < 2) return 0.0f;
    
    float sumSq = 0.0f;
    for (uint16_t i = 0; i < count; i++) {
        float diff = data[i] - mean;
        sumSq += diff * diff;
    }
    
    return sqrtf(sumSq / (count - 1));
}

// Główna funkcja obliczania metryk HX711
void calculateHX711Metrics(HX711Metrics& metrics) {
    if (hx711DataCount == 0) {
        // Inicjalizacja zerowa
        memset(&metrics, 0, sizeof(HX711Metrics));
        return;
    }
    
    // Pobranie danych do tablic
    float weights[HX711_BUFFER_SIZE];
    float rates[HX711_BUFFER_SIZE];
    float times[HX711_BUFFER_SIZE];
    
    uint16_t validCount = 0;
    for (uint16_t i = 0; i < hx711DataCount && i < HX711_BUFFER_SIZE; i++) {
        uint16_t idx = (hx711BufferIndex > i) ? (hx711BufferIndex - 1 - i) : (HX711_BUFFER_SIZE - 1 - i + hx711BufferIndex);
        if (hx711Buffer[idx].is_valid) {
            weights[validCount] = hx711Buffer[idx].weight_filtered;
            rates[validCount] = hx711Buffer[idx].rate_of_change;
            times[validCount] = (float)i * 5.0f / 60.0f;  // Godziny od najnowszego (5min interval)
            validCount++;
        }
    }
    
    if (validCount == 0) return;
    
    // === PODSTAWOWE STATYSTYKI ===
    float sumWeight = 0.0f;
    metrics.min_weight = weights[0];
    metrics.max_weight = weights[0];
    
    for (uint16_t i = 0; i < validCount; i++) {
        sumWeight += weights[i];
        if (weights[i] < metrics.min_weight) metrics.min_weight = weights[i];
        if (weights[i] > metrics.max_weight) metrics.max_weight = weights[i];
    }
    
    metrics.mean_weight = sumWeight / validCount;
    metrics.std_weight = calculateStdDev(weights, validCount, metrics.mean_weight);
    metrics.range_weight = metrics.max_weight - metrics.min_weight;
    metrics.median_weight = calculatePercentile(weights, validCount, 50.0f);
    metrics.weight_variance = metrics.std_weight * metrics.std_weight;
    metrics.weight_cv = (abs(metrics.mean_weight) > 1e-6f) ? metrics.std_weight / abs(metrics.mean_weight) : 0.0f;
    metrics.weight_iqr = calculatePercentile(weights, validCount, 75.0f) - calculatePercentile(weights, validCount, 25.0f);
    
    // === PARAMETRY TEMPORALNE ===
    metrics.current_rate = (validCount > 0) ? rates[0] : 0.0f;
    
    float sumRate = 0.0f;
    metrics.max_rate_positive = 0.0f;
    metrics.max_rate_negative = 0.0f;
    
    for (uint16_t i = 0; i < validCount; i++) {
        sumRate += rates[i];
        if (rates[i] > metrics.max_rate_positive) metrics.max_rate_positive = rates[i];
        if (rates[i] < metrics.max_rate_negative) metrics.max_rate_negative = rates[i];
    }
    
    metrics.mean_rate = sumRate / validCount;
    
    // Przyspieszenie (zmiana szybkości)
    if (validCount >= 2) {
        metrics.acceleration = (rates[0] - rates[1]) * 12.0f;  // kg/h² (12 godzin^-1)
    } else {
        metrics.acceleration = 0.0f;
    }
    
    // === TRENDY ===
    float slope1h, intercept1h;
    uint16_t window1h = min(validCount, (uint16_t)HX711_TREND_WINDOW);
    metrics.trend_correlation = calculateLinearRegression(times, weights, window1h, slope1h, intercept1h);
    metrics.trend_slope_1h = -slope1h;  // Negatywne bo czas idzie wstecz
    
    float slope4h, intercept4h;
    uint16_t window4h = min(validCount, (uint16_t)HX711_DAILY_WINDOW);
    calculateLinearRegression(times, weights, window4h, slope4h, intercept4h);
    metrics.trend_slope_4h = -slope4h;
    
    float slope24h, intercept24h;
    calculateLinearRegression(times, weights, validCount, slope24h, intercept24h);
    metrics.trend_slope_24h = -slope24h;
    
    // Kierunek trendu
    if (metrics.trend_slope_1h > 0.01f) metrics.trend_direction = 1.0f;
    else if (metrics.trend_slope_1h < -0.01f) metrics.trend_direction = -1.0f;
    else metrics.trend_direction = 0.0f;
    
    // === PARAMETRY POŻYTKU ===
    if (metrics.current_rate > HX711_NECTAR_FLOW_MIN) {
        metrics.nectar_inflow_rate = metrics.current_rate;
    } else {
        metrics.nectar_inflow_rate = 0.0f;
    }
    
    // Akumulacja nektaru (suma dodatnich zmian)
    metrics.nectar_accumulation = 0.0f;
    for (uint16_t i = 0; i < validCount; i++) {
        if (rates[i] > 0) {
            metrics.nectar_accumulation += rates[i] * (5.0f / 60.0f);  // 5 min = 5/60 h
        }
    }
    
    // Efektywność zbierania
    float activityHours = 0.0f;
    for (uint16_t i = 0; i < validCount; i++) {
        if (rates[i] > HX711_NECTAR_FLOW_MIN) activityHours += 5.0f / 60.0f;
    }
    metrics.foraging_efficiency = min(100.0f, (activityHours / 12.0f) * 100.0f);  // Zakładając 12h dnia
    
    // Intensywność kwitnienia
    metrics.bloom_intensity = min(100.0f, metrics.nectar_inflow_rate * 200.0f);  // Skalowanie
    
    // Indeks produkcji miodu
    metrics.honey_production_idx = 0.6f * metrics.foraging_efficiency + 0.4f * metrics.bloom_intensity;
    
    // Jakość nektaru (szacowana na podstawie stabilności dopływu)
    float rateStd = calculateStdDev(rates, validCount, metrics.mean_rate);
    float stabilityFactor = 1.0f - min(1.0f, rateStd / max(0.1f, abs(metrics.mean_rate)));
    metrics.nectar_quality_est = stabilityFactor * 100.0f;
    
    // === KONSUMPCJA ===
    if (metrics.current_rate < -HX711_CONSUMPTION_MIN) {
        metrics.consumption_rate = -metrics.current_rate;
    } else {
        metrics.consumption_rate = 0.0f;
    }
    
    metrics.daily_consumption = metrics.consumption_rate * 24.0f;
    
    // Zapas żywności na dni
    if (metrics.daily_consumption > 0.01f) {
        metrics.food_reserve_days = metrics.mean_weight / metrics.daily_consumption;
    } else {
        metrics.food_reserve_days = 999.0f;  // Bardzo duży zapas
    }
    
    // Gotowość do zimowli
    float winterTarget = 15.0f;  // 15kg jako cel zimowy
    metrics.winter_readiness = min(100.0f, (metrics.mean_weight / winterTarget) * 100.0f);
    
    // Ryzyko głodu
    if (metrics.food_reserve_days < 3.0f) {
        metrics.starvation_risk = 100.0f;
    } else if (metrics.food_reserve_days < 7.0f) {
        metrics.starvation_risk = 50.0f + (7.0f - metrics.food_reserve_days) * 12.5f;
    } else if (metrics.food_reserve_days < 14.0f) {
        metrics.starvation_risk = 20.0f + (14.0f - metrics.food_reserve_days) * 4.28f;
    } else {
        metrics.starvation_risk = 0.0f;
    }
    
    // === CYKLICZNOŚĆ ===
    metrics.daily_amplitude = metrics.max_weight - metrics.min_weight;
    metrics.circadian_strength = min(1.0f, metrics.daily_amplitude / 2.0f);  // Normalizacja
    
    // === JAKOŚĆ SYGNAŁU ===
    metrics.noise_level = metrics.std_weight;
    metrics.stability_index = max(0.0f, 100.0f - metrics.weight_cv * 100.0f);
    metrics.measurement_confidence = min(1.0f, metrics.stability_index / 100.0f);
    
    // Dryft czujnika
    if (validCount >= 10) {
        float recentMean = calculateMovingAverage(weights, validCount, 5);
        float olderMean = calculateMovingAverage(weights, validCount - 5, 5);
        metrics.drift_rate = (recentMean - olderMean) * 2.4f;  // kg/dzień
    } else {
        metrics.drift_rate = 0.0f;
    }
    
    // Jakość sygnału
    metrics.signal_quality = max(0.0f, 100.0f - metrics.noise_level * 10.0f);
    
    // === ANOMALIE ===
    float zScore = (abs(metrics.current_rate - metrics.mean_rate) > 1e-6f) ? 
                   abs(metrics.current_rate - metrics.mean_rate) / max(0.001f, metrics.std_weight) : 0.0f;
    metrics.anomaly_score = min(1.0f, zScore / 3.0f);  // Normalizacja do 1
    metrics.sudden_change_mag = abs(metrics.current_rate);
    
    // === ZDROWIE KOLONII ===
    // Tempo wzrostu
    metrics.colony_growth_rate = metrics.trend_slope_24h * 24.0f / max(1.0f, metrics.mean_weight) * 100.0f;
    
    // Indeks czerwiu (szacowany z aktywności)
    float broodFactor = min(1.0f, metrics.foraging_efficiency / 50.0f);
    metrics.brood_activity_idx = broodFactor * 100.0f;
    
    // Szacowana populacja (zakładając 0.1kg na 1000 pszczół)
    metrics.population_estimate = metrics.mean_weight * 10.0f;  // Tysiące pszczół
    
    // Indeks zdrowia
    float healthFactors = (100.0f - metrics.starvation_risk) * 0.4f + 
                          metrics.stability_index * 0.3f +
                          metrics.foraging_efficiency * 0.3f;
    metrics.hive_health_weight = healthFactors;
    
    // Produktywność
    metrics.productivity_score = 0.5f * metrics.foraging_efficiency + 
                                  0.3f * metrics.honey_production_idx +
                                  0.2f * metrics.colony_growth_rate;
    metrics.productivity_score = max(0.0f, min(100.0f, metrics.productivity_score));
    
    // === PROGNOZA ===
    metrics.predicted_weight_24h = metrics.mean_weight + metrics.trend_slope_24h * 24.0f;
    metrics.forecast_confidence = max(0.0f, 1.0f - metrics.anomaly_score) * metrics.measurement_confidence;
    
    // Oczekiwany zbiór miodu
    float surplusWeight = max(0.0f, metrics.mean_weight - 10.0f);  // Nadmiar ponad 10kg
    metrics.expected_honey_yield = surplusWeight * 0.8f;  // 80% nadmiaru do zbioru

    // === NOWE PARAMETRY - ROZSZERZENIA ===
    
    // Obliczanie skośności (skewness)
    if (validCount >= 3) {
        float sumCubed = 0.0f;
        for (uint16_t i = 0; i < validCount; i++) {
            float diff = (weights[i] - metrics.mean_weight) / max(0.001f, metrics.std_weight);
            sumCubed += diff * diff * diff;
        }
        metrics.weight_skewness = sumCubed / validCount;
    } else {
        metrics.weight_skewness = 0.0f;
    }
    
    // Obliczanie kurtozy (kurtosis)
    if (validCount >= 4) {
        float sumQuartic = 0.0f;
        for (uint16_t i = 0; i < validCount; i++) {
            float diff = (weights[i] - metrics.mean_weight) / max(0.001f, metrics.std_weight);
            sumQuartic += diff * diff * diff * diff;
        }
        metrics.weight_kurtosis = sumQuartic / validCount - 3.0f;  // Excess kurtosis
    } else {
        metrics.weight_kurtosis = 0.0f;
    }
    
    // Obliczanie współczynnika Giniego (uproszczone)
    if (validCount >= 2) {
        float sorted[HX711_BUFFER_SIZE];
        for (uint16_t i = 0; i < validCount; i++) sorted[i] = weights[i];
        // Proste sortowanie
        for (uint16_t i = 0; i < validCount - 1; i++) {
            for (uint16_t j = 0; j < validCount - i - 1; j++) {
                if (sorted[j] > sorted[j + 1]) {
                    float temp = sorted[j];
                    sorted[j] = sorted[j + 1];
                    sorted[j + 1] = temp;
                }
            }
        }
        float sumAbsDiff = 0.0f;
        for (uint16_t i = 0; i < validCount; i++) {
            for (uint16_t j = 0; j < validCount; j++) {
                sumAbsDiff += abs(sorted[i] - sorted[j]);
            }
        }
        metrics.weight_gini = sumAbsDiff / (2.0f * validCount * validCount * max(0.001f, metrics.mean_weight));
    } else {
        metrics.weight_gini = 0.0f;
    }
    
    // Jerk (pochodna przyspieszenia)
    if (validCount >= 3) {
        float prevAccel = (rates[1] - rates[2]) * 12.0f;
        metrics.jerk = (metrics.acceleration - prevAccel) * 12.0f;  // kg/h³
    } else {
        metrics.jerk = 0.0f;
    }
    
    // Wariancja szybkości zmian
    metrics.rate_variance = calculateStdDev(rates, validCount, metrics.mean_rate);
    metrics.rate_variance = metrics.rate_variance * metrics.rate_variance;
    
    // Entropia szybkości zmian (uproszczona)
    float rateHist[10] = {0};
    float rateMin = metrics.max_rate_negative;
    float rateRange = metrics.max_rate_positive - metrics.max_rate_negative;
    if (rateRange > 0.001f) {
        for (uint16_t i = 0; i < validCount; i++) {
            int bin = (int)((rates[i] - rateMin) / rateRange * 9.0f);
            bin = constrain(bin, 0, 9);
            rateHist[bin]++;
        }
        float entropy = 0.0f;
        for (int b = 0; b < 10; b++) {
            if (rateHist[b] > 0) {
                float p = rateHist[b] / validCount;
                entropy -= p * logf(p + 1e-10f);
            }
        }
        metrics.rate_entropy = entropy / logf(10.0f);  // Normalizacja do 0-1
    } else {
        metrics.rate_entropy = 0.0f;
    }
    
    // Siła trendu
    metrics.trend_strength = abs(metrics.trend_correlation);
    
    // Persystencja trendu (czy kierunek się utrzymuje)
    if (validCount >= 6) {
        float slopeOld, interceptOld;
        float timesOld[HX711_BUFFER_SIZE];
        for (uint16_t i = 0; i < validCount/2; i++) timesOld[i] = times[i + validCount/2];
        calculateLinearRegression(timesOld, weights + validCount/2, validCount/2, slopeOld, interceptOld);
        float oldDirection = (slopeOld > 0.01f) ? 1.0f : (slopeOld < -0.01f) ? -1.0f : 0.0f;
        metrics.trend_persistence = (oldDirection == metrics.trend_direction) ? 1.0f : 0.0f;
    } else {
        metrics.trend_persistence = 0.5f;
    }
    
    // Liczba punktów zwrotnych trendu
    metrics.trend_change_points = 0;
    for (uint16_t i = 1; i < validCount - 1; i++) {
        if ((rates[i] > 0 && rates[i-1] < 0 && rates[i+1] < 0) ||
            (rates[i] < 0 && rates[i-1] > 0 && rates[i+1] > 0)) {
            metrics.trend_change_points++;
        }
    }
    
    // Czas trwania przepływu nektaru
    float flowHours = 0.0f;
    for (uint16_t i = 0; i < validCount; i++) {
        if (rates[i] > HX711_NECTAR_FLOW_MIN) flowHours += 5.0f / 60.0f;
    }
    metrics.nectar_flow_duration = flowHours;
    
    // Okno wylotów (szacowane)
    float firstFlight = 24.0f, lastFlight = 0.0f;
    for (uint16_t i = 0; i < validCount; i++) {
        if (rates[i] > HX711_NECTAR_FLOW_MIN) {
            float hour = fmodf((float)(millis() - i * 300000) / 3600000.0f, 24.0f);
            if (hour < firstFlight) firstFlight = hour;
            if (hour > lastFlight) lastFlight = hour;
        }
    }
    metrics.foraging_window_start = (firstFlight < 24.0f) ? firstFlight : 6.0f;
    metrics.foraging_window_end = (lastFlight > 0.0f) ? lastFlight : 18.0f;
    
    // Metabolic rate (szacowany z konsumpcji)
    metrics.metabolic_rate = metrics.consumption_rate * 1.2f;  // Współczynnik konwersji
    
    // Regularność zużycia
    if (abs(metrics.mean_rate) > 0.001f) {
        metrics.consumption_regularity = 1.0f - min(1.0f, metrics.rate_variance / abs(metrics.mean_rate));
    } else {
        metrics.consumption_regularity = 0.5f;
    }
    
    // Zawartość harmonicznych (FFT uproszczone)
    metrics.harmonic_content = min(1.0f, metrics.std_weight / max(0.001f, metrics.mean_weight));
    
    // Regularność cykli
    metrics.cycle_regularity = metrics.circadian_strength * (1.0f - metrics.anomaly_score);
    
    // Koherencja fazowa
    metrics.phase_coherence = metrics.cycle_regularity * metrics.measurement_confidence;
    
    // SNR (stosunek sygnału do szumu)
    float signalPower = metrics.mean_weight * metrics.mean_weight;
    float noisePower = metrics.noise_level * metrics.noise_level;
    metrics.snr = 10.0f * log10f(max(1e-10f, signalPower / noisePower));
    
    // THD (Total Harmonic Distortion - uproszczone)
    metrics.thd = min(100.0f, metrics.harmonic_content * 50.0f);
    
    // Stabilność linii bazowej
    if (validCount >= 10) {
        float baselineStd = calculateStdDev(weights + validCount - 10, 10, 
                                            calculateMovingAverage(weights, validCount, 10));
        metrics.baseline_stability = 1.0f - min(1.0f, baselineStd / max(0.001f, metrics.mean_weight));
    } else {
        metrics.baseline_stability = metrics.measurement_confidence;
    }
    
    // Outlier ratio
    uint16_t outlierCount = 0;
    float threshold = 2.5f * metrics.std_weight;
    for (uint16_t i = 0; i < validCount; i++) {
        if (abs(weights[i] - metrics.mean_weight) > threshold) outlierCount++;
    }
    metrics.outlier_ratio = (validCount > 0) ? (float)outlierCount / validCount * 100.0f : 0.0f;
    
    // Prawdopodobieństwo punktu zwrotnego
    metrics.change_point_prob = min(1.0f, metrics.trend_change_points / 5.0f + metrics.anomaly_score * 0.5f);
    
    // Indeks zmienności (volatility)
    metrics.volatility_index = min(100.0f, metrics.weight_cv * 200.0f + metrics.anomaly_score * 50.0f);
    
    // Indikator stresu
    float stressFactors = metrics.anomaly_score * 0.3f + 
                          metrics.starvation_risk / 100.0f * 0.3f +
                          (1.0f - metrics.stability_index / 100.0f) * 0.4f;
    metrics.stress_indicator = min(1.0f, stressFactors);
    
    // Indeks vitalności
    float vitalityFactors = metrics.foraging_efficiency * 0.3f +
                           (100.0f - metrics.starvation_risk) / 100.0f * 0.3f +
                           metrics.stability_index / 100.0f * 0.2f +
                           metrics.circadian_strength * 0.2f;
    metrics.vitality_index = vitalityFactors * 100.0f;
    
    // Zdolność do regeneracji (resilience)
    float resilienceFactors = (1.0f - metrics.anomaly_score) * 0.4f +
                             metrics.stability_index / 100.0f * 0.3f +
                             metrics.vitality_index / 100.0f * 0.3f;
    metrics.resilience_score = min(1.0f, resilienceFactors);
    
    // Przedział ufności prognozy
    metrics.prediction_interval = metrics.std_weight * 2.0f * (1.0f - metrics.forecast_confidence);
    
    // Kierunek prognozy
    if (metrics.predicted_weight_24h > metrics.mean_weight + 0.1f) {
        metrics.forecast_trend = 1.0f;
    } else if (metrics.predicted_weight_24h < metrics.mean_weight - 0.1f) {
        metrics.forecast_trend = -1.0f;
    } else {
        metrics.forecast_trend = 0.0f;
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
    uint8_t valid_count = 0;  // Licznik ważnych próbek
    
    for (uint8_t i = 0; i < actual_count; i++) {
        uint16_t idx = (start_idx + i) % RADAR_BUFFER_SIZE;
        if (!radarBuffer[idx].is_valid) continue;
        
        // Używamy valid_count jako indeksu dla pakowania ważnych danych
        distances[valid_count] = radarBuffer[idx].distance;
        energies[valid_count] = radarBuffer[idx].energy;
        time_indices[valid_count] = (float)valid_count;
        
        sum_dist += distances[valid_count];
        sum_energy += energies[valid_count];
        valid_count++;
    }
    
    result.sample_count = valid_count;
    
    if (result.sample_count < 2) return result;
    
    // Średnie
    result.mean_distance = sum_dist / result.sample_count;
    result.mean_energy = sum_energy / result.sample_count;
    
    // Odchylenia standardowe
    result.std_distance = calculateStandardDeviation(distances, result.sample_count, result.mean_distance);
    result.std_energy = calculateStandardDeviation(energies, result.sample_count, result.mean_energy);
    
    // Nachylenie trendu (regresja liniowa)
    result.slope = calculateLinearRegressionSlope(time_indices, energies, result.sample_count);
    
    // Obliczanie współczynnika korelacji (Pearson correlation coefficient)
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_xx = 0.0f, sum_yy = 0.0f;
    for (uint8_t i = 0; i < result.sample_count; i++) {
        sum_x += time_indices[i];
        sum_y += energies[i];
        sum_xy += time_indices[i] * energies[i];
        sum_xx += time_indices[i] * time_indices[i];
        sum_yy += energies[i] * energies[i];
    }
    
    float numerator = result.sample_count * sum_xy - sum_x * sum_y;
    float denominator_x = result.sample_count * sum_xx - sum_x * sum_x;
    float denominator_y = result.sample_count * sum_yy - sum_y * sum_y;
    
    if (denominator_x > 0.0f && denominator_y > 0.0f) {
        result.correlation = numerator / sqrt(denominator_x * denominator_y);
        // Clamp correlation to [-1, 1] range to handle floating point errors
        if (result.correlation > 1.0f) result.correlation = 1.0f;
        if (result.correlation < -1.0f) result.correlation = -1.0f;
    } else {
        result.correlation = 0.0f;
    }
    
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
    
    // Use absolute motion for motion intensity to avoid cancellation from opposite directions
    float sum_abs_speed = 0.0f;
    for (uint8_t i = 0; i < valid_samples; i++) {
        sum_abs_speed += abs(speeds[i]);
    }
    metrics.motion_intensity = metrics.mean_energy * (sum_abs_speed / valid_samples);
    
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

// ============================================================================
// IMPLEMENTACJA FUNKCJI MODUŁU TEMPERATURY I WILGOTNOŚCI (28 PARAMETRÓW)
// ============================================================================

float calculateHeatIndex(float temp, float humidity) {
    // Uproszczony wzór na heat index (Steadman)
    if (temp < 27.0f) return temp;
    float hi = -8.784695 + 1.61139411f * temp + 2.338549f * humidity
               - 0.14611605f * temp * humidity - 0.012308094f * temp * temp
               - 0.016424828f * humidity * humidity + 0.002211732f * temp * temp * humidity
               + 0.00072546f * temp * humidity * humidity - 0.000003582f * temp * temp * humidity * humidity;
    return hi;
}

float calculateDewPoint(float temp, float humidity) {
    // Wzór Magnus-Tetena
    float a = 17.27f;
    float b = 237.7f;
    float alpha = ((a * temp) / (b + temp)) + log(humidity / 100.0f);
    return (b * alpha) / (a - alpha);
}

float calculateVPD(float temp, float humidity) {
    // Vapor Pressure Deficit [kPa]
    float es = 0.6108f * exp((17.27f * temp) / (temp + 237.3f));  // Ciśnienie nasycenia
    float ea = es * (humidity / 100.0f);  // Rzeczywiste ciśnienie pary
    return es - ea;
}

void calculateTempHumidityMetrics(TempHumidityMetrics& metrics) {
    // Inicjalizacja zerami
    memset(&metrics, 0, sizeof(TempHumidityMetrics));
    
    // Pobranie aktualnych wartości
    float current_temp = temperature;
    float current_hum = humidity;
    
    // === PODSTAWOWE PARAMETRY STATYSTYCZNE ===
    // W pełnej implementacji użyj bufora z historią pomiarów
    // Tutaj uproszczona wersja z pojedynczego pomiaru
    metrics.temp_mean = current_temp;
    metrics.temp_std = 0.5f;  // Szacowane odchylenie
    metrics.temp_min = current_temp - 1.0f;
    metrics.temp_max = current_temp + 1.0f;
    metrics.temp_range = metrics.temp_max - metrics.temp_min;
    
    metrics.hum_mean = current_hum;
    metrics.hum_std = 2.0f;  // Szacowane odchylenie
    metrics.hum_min = max(0.0f, current_hum - 5.0f);
    metrics.hum_max = min(100.0f, current_hum + 5.0f);
    metrics.hum_range = metrics.hum_max - metrics.hum_min;
    
    // === PARAMETRY KOMFORTU TERMICZNEGO ===
    metrics.heat_index = calculateHeatIndex(current_temp, current_hum);
    metrics.dew_point = calculateDewPoint(current_temp, current_hum);
    metrics.vapor_pressure_deficit = calculateVPD(current_temp, current_hum);
    
    // Indeks komfortu (0-100%)
    float comfort_temp = (current_temp >= 32.0f && current_temp <= 36.0f) ? 100.0f : 
                         (current_temp >= 28.0f && current_temp <= 40.0f) ? 70.0f : 30.0f;
    float comfort_hum = (current_hum >= 50.0f && current_hum <= 70.0f) ? 100.0f :
                        (current_hum >= 40.0f && current_hum <= 80.0f) ? 70.0f : 30.0f;
    metrics.comfort_index = (comfort_temp + comfort_hum) / 2.0f;
    
    // === STABILNOŚĆ ŚRODOWISKA ===
    metrics.temp_stability = max(0.0f, 100.0f - metrics.temp_range * 10.0f);
    metrics.hum_stability = max(0.0f, 100.0f - metrics.hum_range * 5.0f);
    metrics.env_variance = (metrics.temp_std * metrics.temp_std) + (metrics.hum_std * metrics.hum_std);
    
    // === TRENDY (uproszczone) ===
    static float prev_temp = 0.0f, prev_hum = 0.0f;
    static unsigned long last_trend_time = 0;
    unsigned long now = millis();
    
    if (prev_temp > 0.0f && (now - last_trend_time) > 3600000) {
        metrics.temp_trend_1h = current_temp - prev_temp;
        metrics.hum_trend_1h = current_hum - prev_hum;
    }
    prev_temp = current_temp;
    prev_hum = current_hum;
    last_trend_time = now;
    
    // Korelacja temp-wilgotność (szacowana)
    metrics.temp_hum_correlation = -0.3f;  // Typowa ujemna korelacja
    
    // === ALARMY I RYZYKA ===
    // Ryzyko przegrzania
    metrics.overheating_risk = (current_temp > 38.0f) ? min(1.0f, (current_temp - 38.0f) / 5.0f) : 0.0f;
    
    // Ryzyko kondensacji
    float dew_diff = current_temp - metrics.dew_point;
    metrics.condensation_risk = (dew_diff < 3.0f) ? min(1.0f, (3.0f - dew_diff) / 3.0f) : 0.0f;
    
    // Ryzyko pleśni
    metrics.mold_risk = (current_hum > 75.0f && current_temp > 20.0f) ? 
                        min(1.0f, (current_hum - 75.0f) / 25.0f) : 0.0f;
    
    // Indeks stresu czerwiu
    float brood_optimal_temp = 34.5f;
    float temp_deviation = abs(current_temp - brood_optimal_temp);
    metrics.brood_stress_index = min(100.0f, temp_deviation * 10.0f + metrics.overheating_risk * 50.0f);
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI MODUŁU JAKOŚCI POWIETRZA (24 PARAMETRY)
// ============================================================================

uint8_t mapIAQLevel(float iaq_index) {
    if (iaq_index <= 50) return 1;      // Excellent
    if (iaq_index <= 100) return 2;     // Good
    if (iaq_index <= 150) return 3;     // Moderate
    if (iaq_index <= 200) return 4;     // Poor
    return 5;                            // Very Poor
}

float calculateVentilationNeed(uint16_t co2, uint16_t voc) {
    float need = 0.0f;
    if (co2 > 1000) need += min(50.0f, (co2 - 1000) / 20.0f);
    if (voc > 100) need += min(50.0f, (voc - 100) / 5.0f);
    return min(100.0f, need);
}

void calculateAirQualityMetrics(AirQualityMetrics& metrics) {
    // Inicjalizacja zerami
    memset(&metrics, 0, sizeof(AirQualityMetrics));
    
    // Pobranie aktualnych wartości z SGP41
    metrics.co2_equivalent = co2_eq;
    metrics.voc_index = voc_idx;
    metrics.nox_equivalent = 0;  // SGP41 nie mierzy NOx bezpośrednio
    
    // === STATYSTYKI CO2 i VOC ===
    metrics.co2_mean = (float)co2_eq;
    metrics.co2_std = 50.0f;  // Szacowane
    metrics.co2_min = max(400, co2_eq - 100);
    metrics.co2_max = co2_eq + 100;
    
    metrics.voc_mean = (float)voc_idx;
    metrics.voc_std = 10.0f;
    metrics.voc_min = max(0, voc_idx - 20);
    metrics.voc_max = voc_idx + 20;
    
    // === INDEKSY JAKOŚCI POWIETRZA ===
    // IAQ Index (0-500)
    metrics.iaq_index = min(500.0f, (float)co2_eq * 0.3f + (float)voc_idx * 2.0f);
    metrics.air_quality_level = (float)mapIAQLevel(metrics.iaq_index);
    metrics.ventilation_need = calculateVentilationNeed(co2_eq, voc_idx);
    
    // === ZDROWIE ULA ===
    // Indeks komfortu ula
    float co2_comfort = (co2_eq < 1500) ? 100.0f : max(0.0f, 100.0f - (co2_eq - 1500) / 10.0f);
    float voc_comfort = (voc_idx < 50) ? 100.0f : max(0.0f, 100.0f - (voc_idx - 50) * 2.0f);
    metrics.hive_comfort_index = (co2_comfort + voc_comfort) / 2.0f;
    
    // Stres od jakości powietrza
    metrics.stress_from_air = max(0.0f, min(1.0f, (metrics.iaq_index - 100) / 400.0f));
    
    // Ryzyko zanieczyszczenia
    metrics.contamination_risk = (voc_idx > 150) ? min(1.0f, (voc_idx - 150) / 350.0f) : 0.0f;
    
    // === TRENDY ===
    static int16_t prev_co2 = 0, prev_voc = 0;
    if (prev_co2 > 0) {
        metrics.co2_trend_1h = (float)(co2_eq - prev_co2);
        metrics.voc_trend_1h = (float)(voc_idx - prev_voc);
    }
    prev_co2 = co2_eq;
    prev_voc = voc_idx;
    
    metrics.air_quality_trend = -(metrics.co2_trend_1h / 100.0f + metrics.voc_trend_1h / 20.0f) / 2.0f;
    
    // === ALERTY ===
    metrics.poor_ventilation_alert = (metrics.ventilation_need > 50.0f) ? 
                                      min(1.0f, metrics.ventilation_need / 100.0f) : 0.0f;
    
    metrics.high_co2_warning = (co2_eq > 2000) ? min(1.0f, (co2_eq - 2000) / 3000.0f) : 0.0f;
    
    metrics.voc_alert_level = (voc_idx > 200) ? min(1.0f, (voc_idx - 200) / 300.0f) : 0.0f;
    
    // Łączny wynik ryzyka
    metrics.combined_risk_score = min(100.0f, 
        metrics.stress_from_air * 40.0f + 
        metrics.contamination_risk * 30.0f + 
        metrics.poor_ventilation_alert * 30.0f);
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI MODUŁU WIBRACJI PIEZO (22 PARAMETRY)
// ============================================================================

float detectDominantFrequency(const float* data, uint16_t count, float sampleRate) {
    // Prosta detekcja dominantnej częstotliwości metodą DFT
    if (count < 4) return 0.0f;
    
    uint16_t max_bin = 0;
    float max_power = 0.0f;
    
    // Sprawdź kilka pierwszych harmonicznych
    for (uint8_t k = 1; k <= 8 && k < count / 2; k++) {
        float real_part = 0.0f;
        float imag_part = 0.0f;
        
        for (uint16_t n = 0; n < count; n++) {
            float angle = (2.0f * PI * k * n) / count;
            real_part += data[n] * cos(angle);
            imag_part -= data[n] * sin(angle);
        }
        
        float power = (real_part * real_part + imag_part * imag_part) / (count * count);
        if (power > max_power) {
            max_power = power;
            max_bin = k;
        }
    }
    
    return (float)max_bin * sampleRate / count;
}

void calculatePiezoMetrics(PiezoVibrationMetrics& metrics) {
    // Inicjalizacja zerami
    memset(&metrics, 0, sizeof(PiezoVibrationMetrics));
    
    // Pobranie aktualnego odczytu z Piezo
    int raw_value = analogRead(PIEZO_PIN);
    float voltage = raw_value * (3.3f / 4095.0f) * 1000.0f;  // Konwersja na mV
    
    // === PODSTAWOWE PARAMETRY AMPLITUDY ===
    metrics.vibration_rms = voltage * 0.707f;  // Przybliżenie dla sygnału sinusoidalnego
    metrics.vibration_peak = voltage;
    metrics.vibration_peak_to_peak = voltage * 2.0f;
    metrics.vibration_mean = voltage * 0.5f;
    
    // === STATYSTYKI WIBRACJI ===
    metrics.vibration_std = voltage * 0.2f;  // Szacowane
    metrics.vibration_cv = (metrics.vibration_mean > 0.01f) ? 
                           metrics.vibration_std / metrics.vibration_mean : 0.0f;
    metrics.vibration_energy = voltage * voltage;
    
    // === PARAMETRY CZĘSTOTLIWOŚCIOWE ===
    // W pełnej wersji użyj bufora z próbkami
    metrics.dominant_vib_freq = 250.0f;  // Typowa częstotliwość pszczół [Hz]
    metrics.vibration_activity_idx = min(100.0f, voltage * 2.0f);
    
    // === DETEKCJA ZDARZEŃ ===
    metrics.impact_count = (voltage > 50.0f) ? 1.0f : 0.0f;  // Próg uderzenia
    metrics.continuous_vib_ratio = (voltage > 20.0f) ? 0.7f : 0.2f;
    metrics.intermittent_vib_ratio = 1.0f - metrics.continuous_vib_ratio;
    
    // === KLASYFIKACJA ŹRÓDŁA ===
    // Na podstawie charakterystyki sygnału
    if (voltage > 100.0f && metrics.dominant_vib_freq < 100.0f) {
        // Potencjalny drapieżnik (niska częstotliwość, wysoka amplituda)
        metrics.predator_vib_score = min(100.0f, voltage / 2.0f);
        metrics.bee_traffic_score = 20.0f;
    } else if (voltage > 10.0f && metrics.dominant_vib_freq >= 80.0f && 
               metrics.dominant_vib_freq <= 800.0f) {
        // Aktywność pszczół
        metrics.bee_traffic_score = min(100.0f, voltage * 1.5f);
        metrics.predator_vib_score = 5.0f;
    } else {
        // Szum środowiskowy
        metrics.environmental_vib_score = min(100.0f, 50.0f + voltage * 0.5f);
    }
    
    // === TRENDY ===
    static float prev_vib = 0.0f;
    static unsigned long last_vib_time = 0;
    unsigned long now = millis();
    
    if (prev_vib > 0.0f && (now - last_vib_time) > 60000) {
        metrics.vibration_trend_1min = voltage - prev_vib;
        metrics.activity_change_rate = (voltage - prev_vib) / prev_vib * 100.0f;
    }
    prev_vib = voltage;
    last_vib_time = now;
    
    // === ALERTY ===
    // Prawdopodobieństwo intruza
    metrics.intrusion_probability = metrics.predator_vib_score / 100.0f * 
                                    (voltage > 80.0f ? 1.5f : 1.0f);
    metrics.intrusion_probability = min(1.0f, metrics.intrusion_probability);
    
    // Flaga wibracji rojowej
    metrics.swarm_vibration_flag = (metrics.dominant_vib_freq >= 150.0f && 
                                     metrics.dominant_vib_freq <= 350.0f &&
                                     voltage > 30.0f) ? 1.0f : 0.0f;
    
    // Wynik anomalii
    metrics.anomaly_vibration_score = (voltage > 200.0f || metrics.intrusion_probability > 0.7f) ? 
                                       min(1.0f, voltage / 300.0f) : 0.0f;
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI MODUŁU CIŚNIENIA ATMOSFERYCZNEGO (18 PARAMETRÓW)
// ============================================================================

float calculateSeaLevelPressure(float pressure, float altitude) {
    // Korekcja ciśnienia do poziomu morza
    return pressure * pow(1.0f - (altitude / 44330.0f), -5.255f);
}

int predictWeatherTrend(const float* pressures, uint8_t count) {
    if (count < 2) return 0;
    float trend = pressures[count-1] - pressures[0];
    if (trend > 2.0f) return 1;   // Poprawa
    if (trend < -2.0f) return -1; // Pogorszenie
    return 0;                      // Stabilnie
}

void calculateBarometricMetrics(BarometricMetrics& metrics) {
    // Inicjalizacja zerami
    memset(&metrics, 0, sizeof(BarometricMetrics));
    
    // Symulacja odczytu ciśnienia (w pełnej wersji z BMP280)
    static float simulated_pressure = 1013.25f;
    static float prev_pressure = 1013.25f;
    
    // Lekka losowa zmienność dla demonstracji
    simulated_pressure += (random(-10, 11) / 100.0f);
    simulated_pressure = constrain(simulated_pressure, 980.0f, 1040.0f);
    
    float current_pressure = simulated_pressure;
    
    // === PODSTAWOWE PARAMETRY CIŚNIENIA ===
    metrics.pressure_mean = current_pressure;
    metrics.pressure_std = 1.0f;  // Szacowane
    metrics.pressure_min = current_pressure - 2.0f;
    metrics.pressure_max = current_pressure + 2.0f;
    
    // Trendy
    metrics.pressure_trend_1h = current_pressure - prev_pressure;
    metrics.pressure_trend_3h = (current_pressure - prev_pressure) * 3.0f;
    
    prev_pressure = current_pressure;
    
    // === PROGNOZA POGODY ===
    // Trend pogodowy (-1 do 1)
    metrics.weather_trend = constrain(metrics.pressure_trend_3h / 5.0f, -1.0f, 1.0f);
    
    // Prawdopodobieństwo burzy
    if (metrics.pressure_trend_3h < -4.0f) {
        metrics.storm_probability = min(100.0f, abs(metrics.pressure_trend_3h) * 15.0f);
    } else {
        metrics.storm_probability = 0.0f;
    }
    
    // Ciśnienie na poziomie morza (zakładając wysokość 100m n.p.m.)
    metrics.pressure_sea_level = calculateSeaLevelPressure(current_pressure, 100.0f);
    
    // === WPŁYW NA PSZCZOŁY ===
    // Warunki do wylotów
    float pressure_comfort = (current_pressure >= 1010.0f && current_pressure <= 1025.0f) ? 100.0f :
                             (current_pressure >= 1000.0f && current_pressure <= 1035.0f) ? 70.0f : 30.0f;
    float trend_penalty = abs(metrics.pressure_trend_1h) * 5.0f;
    metrics.foraging_conditions = max(0.0f, pressure_comfort - trend_penalty);
    
    // Indeks pogody dla rojenia
    metrics.swarm_weather_index = (current_pressure > 1015.0f && metrics.weather_trend > 0.0f) ? 
                                   70.0f + metrics.weather_trend * 30.0f : 30.0f;
    
    // Stres barometryczny
    metrics.barometric_stress = (abs(metrics.pressure_trend_1h) > 3.0f) ? 
                                 min(1.0f, abs(metrics.pressure_trend_1h) / 10.0f) : 0.0f;
    
    // === ALTITUDE COMPENSATION ===
    metrics.altitude_compensated = 100.0f;  // Zakładana wysokość
    metrics.pressure_altitude = 44330.0f * (1.0f - pow(current_pressure / 1013.25f, 1.0f / 5.255f));
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI MODUŁU NATĘŻENIA ŚWIATŁA (17 PARAMETRÓW)
// ============================================================================

bool detectHiveOpening(uint32_t lux, uint32_t prev_lux, bool is_night) {
    // Detekcja nagłej zmiany światła sugerującej otwarcie ula
    if (is_night && lux > 100 && prev_lux < 10) return true;
    if (!is_night && abs((int)lux - (int)prev_lux) > 5000) return true;
    return false;
}

void calculateLightMetrics(LightMetrics& metrics) {
    // Inicjalizacja zerami
    memset(&metrics, 0, sizeof(LightMetrics));
    
    // Symulacja odczytu światła (w pełnej wersji z BH1750/TSL2561)
    static uint32_t simulated_lux = 1000;
    static unsigned long sim_start_time = 0;
    
    if (sim_start_time == 0) sim_start_time = millis();
    unsigned long hours_elapsed = (millis() - sim_start_time) / 3600000;
    
    // Symulacja cyklu dobowego
    uint8_t hour_of_day = hours_elapsed % 24;
    if (hour_of_day >= 6 && hour_of_day <= 18) {
        // Dzień - symulacja zmian światła
        simulated_lux = 5000 + random(0, 20000);
    } else {
        // Noc
        simulated_lux = random(0, 50);
    }
    
    uint32_t current_lux = simulated_lux;
    bool is_night = (current_lux < 100);
    
    // === PODSTAWOWE PARAMETRY ŚWIATŁA ===
    metrics.lux_current = current_lux;
    metrics.lux_mean = (float)current_lux;
    metrics.lux_std = (float)current_lux * 0.2f;
    metrics.lux_min = max((uint32_t)0, current_lux - 1000);
    metrics.lux_max = current_lux + 1000;
    
    // === CYKL DOBOWY ===
    // Czas trwania dnia/nocy (szacowany)
    metrics.daylight_duration = 12.0f;  // Godzin
    metrics.night_duration = 24.0f - metrics.daylight_duration;
    
    // Wykrycie świtu/zmierzchu
    static uint32_t prev_lux = 0;
    if ((prev_lux < 100 && current_lux >= 100) || (prev_lux >= 100 && current_lux < 100)) {
        metrics.dawn_dusk_detected = 1.0f;
    } else {
        metrics.dawn_dusk_detected = 0.0f;
    }
    prev_lux = current_lux;
    
    // === WPŁYW NA PSZCZOŁY ===
    // Indeks światła do wylotów
    if (current_lux >= 1000 && current_lux <= 50000) {
        metrics.foraging_light_index = 100.0f;
    } else if (current_lux >= 500) {
        metrics.foraging_light_index = 70.0f;
    } else {
        metrics.foraging_light_index = 20.0f;
    }
    
    // Synchronizacja cyrkadiana
    bool proper_cycle = (is_night && hour_of_day >= 20) || (!is_night && hour_of_day >= 6 && hour_of_day <= 18);
    metrics.circadian_sync = proper_cycle ? 0.95f : 0.5f;
    
    // Detekcja otwarcia ula
    static bool hive_was_open = false;
    if (detectHiveOpening(current_lux, prev_lux, is_night)) {
        metrics.hive_open_detection = 1.0f;
        hive_was_open = true;
    } else {
        metrics.hive_open_detection = hive_was_open ? 0.5f : 0.0f;
        if (hive_was_open) hive_was_open = false;
    }
    
    // === SEZONOWOŚĆ ===
    metrics.seasonal_light_change = 0.0f;  // Wymaga długoterminowych danych
    metrics.light_stability = 100.0f - min(100.0f, metrics.lux_std / 100.0f);
    
    // === ALERTY ===
    // Nieprawidłowe światło
    metrics.abnormal_light_flag = (is_night && current_lux > 500) ? 1.0f : 0.0f;
    
    // Zdarzenie intruza (światło w nocy)
    metrics.intrusion_light_event = (is_night && current_lux > 1000) ? 1.0f : 0.0f;
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
    
    // Detekcja nagłówka ramki LD2410B: 0xF4 0xF3 0xF2 0xF1 (4 bajty)
    if(idx == 0 && b != 0xF4) continue;
    if(idx == 1 && b != 0xF3) { idx = 0; continue; }
    if(idx == 2 && b != 0xF2) { idx = 0; continue; }
    if(idx == 3 && b != 0xF1) { idx = 0; continue; }
    
    // Bounds check przed zapisem do bufora
    if(idx >= 255) {
      idx = 0;
      continue;
    }
    
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
  
  Serial.println("=== ApiaryGuard Pico v2.0 Start ===");
  
  // Inicjalizacja I2C
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  
  // Inicjalizacja UART dla Radaru
  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
  
  // ================================================================
  // DYNAMICZNE WYKRYWANIE SENSORÓW
  // ================================================================
  Serial.println(">> Skanowanie i wykrywanie sensorów...");
  detectAllSensors();
  printSensorStatus();
  
  // Inicjalizacja Sensorów (tylko jeśli wykryte)
  if (sensors.tempHum.detected) {
    dht.begin();
    Serial.println(">> DHT22/SHT40 zainicjalizowany");
  }
  
  if (sensors.airQual.detected) {
    if (sgp.begin_I2C(0x59, &Wire)) {
      sgp.measureBaseline();
      Serial.println(">> SGP41 zainicjalizowany");
    }
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
  
  Serial.println(">> System gotowy.");
}

void loop() {
  unsigned long now = millis();
  
  // ================================================================
  // WARUNKOWY ODCZYT SENSORÓW - tylko dla wykrytych modułów
  // ================================================================
  
  if(now - lastMillis > 2000) {
    lastMillis = now;
    
    // DHT22/SHT40 - tylko jeśli wykryty
    if (sensors.tempHum.active) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if(!isnan(h)) humidity = h;
      if(!isnan(t)) temperature = t;
    }
    
    // HX711 - tylko jeśli wykryty
    if (sensors.hx711.active) {
      long raw = readHX711();
      hx711_value = (raw - hx711_offset) / hx711_scale;
    }
    
    // SGP41 (CO2/VOC) - tylko jeśli wykryty
    if (sensors.airQual.active) {
      sgp.measureGas();
      co2_eq = sgp.CO2eq;
      voc_idx = sgp.VOCindex;
    }
    
    // Audio - profesjonalna analiza z FFT (co 5 sekund) - tylko jeśli wykryty
    static unsigned long lastAudioProcess = 0;
    if (sensors.audio.active && (now - lastAudioProcess > 5000)) {
        lastAudioProcess = now;
        processAudioSignal();
        
        // Debug output dla modułu audio
        Serial.printf("[AUDIO] RMS:%.3f DomFreq:%.1fHz BeeAct:%.1f%% SwarmProb:%.2f Health:%.1f%%\n",
                      currentAudioMetrics.rms_amplitude,
                      currentAudioMetrics.dominant_frequency,
                      currentAudioMetrics.bee_activity_index,
                      currentAudioMetrics.swarm_probability,
                      currentAudioMetrics.hive_health_audio);
        
        // Dodatkowe nowe parametry audio
        Serial.printf("[AUDIO_NEW] CrestFactor:%.2f Formant1:%.1fHz F0:%.1fHz PitchStr:%.2f Inharm:%.2f\n",
                      currentAudioMetrics.crest_factor,
                      currentAudioMetrics.formant_freq_1,
                      currentAudioMetrics.fundamental_frequency,
                      currentAudioMetrics.pitch_strength,
                      currentAudioMetrics.inharmonicity);
        
        Serial.printf("[AUDIO_NEW2] Shimmer:%.1f%% Jitter:%.1f%% NHR:%.2f HNR:%.2f AutoCorr:%.2f\n",
                      currentAudioMetrics.shimmer,
                      currentAudioMetrics.jitter,
                      currentAudioMetrics.noise_to_harmonic_ratio,
                      currentAudioMetrics.harmonic_to_noise_ratio,
                      currentAudioMetrics.autocorrelation_peak);
        
        Serial.printf("[AUDIO_ENV] Attack:%.1fms Decay:%.1fms Sustain:%.2f TempCent:%.1fms Loudness:%.2f\n",
                      currentAudioMetrics.attack_time,
                      currentAudioMetrics.decay_time,
                      currentAudioMetrics.sustain_level,
                      currentAudioMetrics.temporal_centroid,
                      currentAudioMetrics.loudness_zwicker);
        
        Serial.printf("[AUDIO_CLASS] ForagingEff:%.1f%% ColonyCoh:%.2f SpectEnt:%.3f Tonality:%.2f\n",
                      currentAudioMetrics.foraging_efficiency,
                      currentAudioMetrics.colony_coherence,
                      currentAudioMetrics.spectral_entropy,
                      currentAudioMetrics.tonal_strength);
        
        // Informacja o zdarzeniach audio
        if(lastAudioEvent.type != AudioEvent::EVENT_NONE) {
            Serial.printf("[AUDIO_STATUS_ZMIANY] Typ:%s Wplyw:%s Pozyczek_ulu:%s\n",
                          audioEventTypeToString(lastAudioEvent.type),
                          audioEventImpactToString(lastAudioEvent.impact),
                          isPositiveAudioChange(lastAudioEvent) ? "POZYTYWNY" : "NEGATYWNY/NEUTRALNY");
        }
    }
    
    // Audio RMS (prosta wersja) - tylko jeśli wykryty
    if (sensors.audio.active) {
      audio_rms = calculateAudioRMS();
    }
    
    // Wibracje (Piezo) - tylko jeśli wykryty
    // Aktualizacja metryk piezo zamiast pojedynczej zmiennej
    if (sensors.piezo.active) {
      calculatePiezoMetrics(currentPiezoMetrics);
    }
    
    // Radar - tylko jeśli wykryty
    if (sensors.radar.active) {
      processRadar();
    }
    
    // HX711 - profesjonalna analiza wagi (co 5 minut)
    static unsigned long lastHX711Process = 0;
    if (now - lastHX711Process > 300000) {  // 5 minut = 300000ms
        lastHX711Process = now;
        
        // Konwersja surowej wartości na kg (przykładowa kalibracja)
        float weightKg = (float)hx711_value / 1000.0f;  // Zakładając skalowanie 1000:1
        
        // Kompensacja temperaturowa
        compensateTemperature(weightKg, temperature);
        
        // Filtrowanie wagii (wygładzanie)
        static float prevFilteredWeight = 0.0f;
        if (prevFilteredWeight == 0.0f) prevFilteredWeight = weightKg;
        float filteredWeight = applyWeightFilter(weightKg, prevFilteredWeight, 0.3f);
        prevFilteredWeight = filteredWeight;
        
        // Tworzenie punktu danych
        HX711DataPoint newPoint;
        newPoint.timestamp = now;
        newPoint.weight_raw = weightKg;
        newPoint.weight_filtered = filteredWeight;
        newPoint.temperature_comp = temperature;
        newPoint.is_valid = true;
        
        // Obliczanie szybkości zmiany wagi
        if (hx711DataCount > 0) {
            HX711DataPoint& prevPoint = hx711Buffer[(hx711BufferIndex > 0) ? hx711BufferIndex - 1 : HX711_BUFFER_SIZE - 1];
            float timeDiffHours = (float)(now - prevPoint.timestamp) / 3600000.0f;  // ms -> godziny
            if (timeDiffHours > 0.001f) {  // Unikanie dzielenia przez zero
                newPoint.rate_of_change = (filteredWeight - prevPoint.weight_filtered) / timeDiffHours;
            } else {
                newPoint.rate_of_change = 0.0f;
            }
        } else {
            newPoint.rate_of_change = 0.0f;
        }
        
        // Jakość pomiaru (na podstawie stabilności)
        newPoint.quality_score = 100;
        if (abs(newPoint.rate_of_change) > 1.0f) newPoint.quality_score -= 20;  // Duże zmiany obniżają jakość
        if (temperature < 10.0f || temperature > 40.0f) newPoint.quality_score -= 10;  // Ekstremalne temperatury
        
        // Aktualizacja bufora i wzorców
        updateHX711Buffer(newPoint);
        updateHX711DailyPattern(newPoint);
        
        // Obliczanie metryk
        calculateHX711Metrics(currentHX711Metrics);
        
        // Detekcja zdarzeń
        lastHX711Event = detectHX711Events(newPoint, currentHX711Metrics);
        
        // Debug output dla modułu HX711
        Serial.printf("[HX711] Waga:%.3fkg Filtrowana:%.3fkg Zmiana:%.4fkg/h Temp:%.1fC Jakosc:%d%%\n",
                      weightKg, filteredWeight, newPoint.rate_of_change, temperature, newPoint.quality_score);
        
        // Podstawowe metryki
        Serial.printf("[HX711_METRICS] Srednia:%.3fkg Std:%.4fkg Trend1h:%.4fkg/h Trend4h:%.4fkg/h\n",
                      currentHX711Metrics.mean_weight, currentHX711Metrics.std_weight,
                      currentHX711Metrics.trend_slope_1h, currentHX711Metrics.trend_slope_4h);
        
        // Parametry pożytku
        Serial.printf("[HX711_POZYTKI] NectarInflow:%.4fkg/h ForagingEff:%.1f%% BloomIntensity:%.1f%% HoneyProdIdx:%.1f%%\n",
                      currentHX711Metrics.nectar_inflow_rate, currentHX711Metrics.foraging_efficiency,
                      currentHX711Metrics.bloom_intensity, currentHX711Metrics.honey_production_idx);
        
        // Parametry konsumpcji
        Serial.printf("[HX711_KONSUMPCJA] Rate:%.4fkg/h Daily:%.3fkg ZapasDni:%.1f StarvationRisk:%.1f%%\n",
                      currentHX711Metrics.consumption_rate, currentHX711Metrics.daily_consumption,
                      currentHX711Metrics.food_reserve_days, currentHX711Metrics.starvation_risk);
        
        // Wskaźniki zdrowia
        Serial.printf("[HX711_HEALTH] ColonyGrowth:%.2f%%/dz BroodIdx:%.1f%% PopEst:%.1fk Health:%.1f%% Productivity:%.1f%%\n",
                      currentHX711Metrics.colony_growth_rate, currentHX711Metrics.brood_activity_idx,
                      currentHX711Metrics.population_estimate, currentHX711Metrics.hive_health_weight,
                      currentHX711Metrics.productivity_score);
        
        // Informacja o zdarzeniach
        if(lastHX711Event.type != HX711Event::EVENT_NONE) {
            Serial.printf("[HX711_EVENT] Typ:%s Wplyw:%s Pozyczek_ulu:%s Magnitude:%.3fkg Confidence:%.1f%%\n",
                          hx711EventTypeToString(lastHX711Event.type),
                          hx711EventImpactToString(lastHX711Event.impact),
                          isPositiveHX711Change(lastHX711Event) ? "POZYTYWNY" : "NEGATYWNY/NEUTRALNY",
                          lastHX711Event.magnitude, lastHX711Event.confidence * 100.0f);
            Serial.printf("[HX711_EVENT_DESC] %s\n", lastHX711Event.description);
        }
        
        // Prognoza
        Serial.printf("[HX711_FORECAST] Pred24h:%.3fkg ForecastConf:%.1f%% ExpectedYield:%.2fkg\n",
                      currentHX711Metrics.predicted_weight_24h, currentHX711Metrics.forecast_confidence * 100.0f,
                      currentHX711Metrics.expected_honey_yield);
    }
    
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
  
  // 2. Obsługa Sieci W6100 - HTTP Server z GUI i API
  EthernetClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();
    
    Serial.print("[HTTP] Ządanie: ");
    Serial.println(request);
    
    auto sendHeader = [&](const char* contentType) {
      client.println("HTTP/1.1 200 OK");
      client.printf("Content-Type: %s\r\n", contentType);
      client.println("Connection: close");
      client.println();
    };
    
    // STRONA GŁÓWNA - HUMAN READABLE GUI Z ROZSZERZONYM DEBUGOWANIEM
    if(request.indexOf("GET / ") >= 0 && request.indexOf(".json") < 0 && 
       request.indexOf("/status") < 0 && request.indexOf("/metrics") < 0 &&
       request.indexOf("/events") < 0 && request.indexOf("/anomalies") < 0 &&
       request.indexOf("/heater") < 0 && request.indexOf("/fan") < 0 && 
       request.indexOf("/pump") < 0) {
      
      sendHeader("text/html; charset=utf-8");
      client.println("<!DOCTYPE html><html lang='pl'><head><meta charset='UTF-8'><title>ApiaryGuard Pico - Dashboard</title>");
      client.println("<style>");
      client.println("body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);padding:20px;color:#333}");
      client.println("h1{color:white;text-align:center;text-shadow:2px 2px 4px rgba(0,0,0,0.3)}");
      client.println("h2{color:#667eea;border-bottom:3px solid #667eea;padding-bottom:10px;margin-top:0}");
      client.println("h3{color:#764ba2;margin-top:30px}");
      client.println(".card{background:white;border-radius:15px;padding:20px;margin:15px 0;box-shadow:0 4px 15px rgba(0,0,0,0.2)}");
      client.println(".metric{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid #eee;font-size:1.1em}");
      client.println(".metric:last-child{border-bottom:none}");
      client.println(".metric span:first-child{font-weight:600;color:#555}");
      client.println(".metric span:last-child{font-weight:700;color:#667eea}");
      client.println(".status-ok{color:#22c55e}");
      client.println(".status-warning{color:#f59e0b}");
      client.println(".status-error{color:#ef4444}");
      client.println(".info-bar{background:rgba(255,255,255,0.95);border-radius:10px;padding:15px;margin:15px 0;display:flex;justify-content:space-around;flex-wrap:wrap}");
      client.println(".info-item{text-align:center;margin:10px}");
      client.println(".info-label{font-size:0.9em;color:#666}");
      client.println(".info-value{font-size:1.3em;font-weight:700;color:#667eea}");
      client.println(".debug-section{background:#f8f9fa;border-radius:10px;padding:15px;margin:15px 0;border-left:4px solid #667eea}");
      client.println(".debug-title{font-weight:700;color:#667eea;margin-bottom:10px}");
      client.println(".debug-row{display:flex;justify-content:space-between;padding:5px 0;border-bottom:1px dashed #ddd;font-size:0.95em}");
      client.println(".debug-row:last-child{border-bottom:none}");
      client.println(".api-links{background:white;border-radius:10px;padding:15px;margin:15px 0}");
      client.println(".api-links a{display:inline-block;background:#667eea;color:white;padding:10px 20px;margin:5px;border-radius:5px;text-decoration:none;font-weight:600}");
      client.println(".api-links a:hover{background:#764ba2}");
      client.println("</style></head><body>");
      
      client.println("<h1>🐝 ApiaryGuard Pico - Monitoring Ula 🐝</h1>");
      
      // Pasek informacyjny
      client.println("<div class='info-bar'>");
      client.printf("<div class='info-item'><div class='info-label'>Adres IP</div><div class='info-value'>%s</div></div>", Ethernet.localIP().toString().c_str());
      client.printf("<div class='info-item'><div class='info-label'>Czas pracy</div><div class='info-value'>%lu s</div></div>", millis()/1000);
      client.printf("<div class='info-item'><div class='info-label'>RAM Free</div><div class='info-value'>~%lu KB</div></div>", (unsigned long)(rp2040.getFreeHeap() / 1024));
      client.printf("<div class='info-item'><div class='info-label'>CPU Temp</div><div class='info-value'>%.1f C</div></div>", analogReadTempSensor());
      client.println("</div>");
      
      // Środowisko
      client.println("<div class='card'><h2>🌡️ Środowisko w Ulu</h2>");
      client.printf("<div class='metric'><span>Temperatura</span><span>%.1f °C</span></div>", temperature);
      client.printf("<div class='metric'><span>Wilgotność</span><span>%.1f %%</span></div>", humidity);
      float heatIndex = calculateHeatIndex(temperature, humidity);
      client.printf("<div class='metric'><span>Indeks ciepła</span><span>%.1f °C</span></div>", heatIndex);
      float dewPoint = calculateDewPoint(temperature, humidity);
      client.printf("<div class='metric'><span>Punkt rosy</span><span>%.1f °C</span></div>", dewPoint);
      float vpd = calculateVPD(temperature, humidity);
      client.printf("<div class='metric'><span>VPD</span><span>%.2f kPa</span></div>", vpd);
      client.printf("<div class='metric'><span>CO₂ (eq)</span><span>%d ppm</span></div>", co2_eq);
      client.printf("<div class='metric'><span>VOC Index</span><span>%d</span></div>", voc_idx);
      client.println("</div>");
      
      // Waga ulu
      client.println("<div class='card'><h2>⚖️ Waga Ula - Analiza</h2>");
      client.printf("<div class='metric'><span>Aktualna waga</span><span>%.3f kg</span></div>", currentHX711Metrics.mean_weight);
      client.printf("<div class='metric'><span>Waga filtrowana</span><span>%.3f kg</span></div>", currentHX711Metrics.filtered_weight);
      client.printf("<div class='metric'><span>Trend 1h</span><span>%.4f kg/h</span></div>", currentHX711Metrics.trend_slope_1h);
      client.printf("<div class='metric'><span>Trend 4h</span><span>%.4f kg/h</span></div>", currentHX711Metrics.trend_slope_4h);
      client.printf("<div class='metric'><span>Trend 24h</span><span>%.4f kg/h</span></div>", currentHX711Metrics.trend_slope_24h);
      client.printf("<div class='metric'><span>Zmiana 24h</span><span>%.3f kg</span></div>", currentHX711Metrics.weight_change_24h);
      client.printf("<div class='metric'><span>Zapas pokarmu</span><span>%.1f dni</span></div>", currentHX711Metrics.food_reserve_days);
      client.printf("<div class='metric'><span>Ryzyko głodu</span><span>%.1f %%</span></div>", currentHX711Metrics.starvation_risk);
      client.printf("<div class='metric'><span>Jakość sygnału</span><span>%d %%</span></div>", currentHX711Metrics.signal_quality);
      client.println("</div>");
      
      // Pożytki i produkcja miodu
      client.println("<div class='card'><h2>🍯 Pożytki i Produkcja Miodu</h2>");
      client.printf("<div class='metric'><span>Napływ nektaru</span><span>%.4f kg/h</span></div>", currentHX711Metrics.nectar_inflow_rate);
      client.printf("<div class='metric'><span>Efektywność zbierania</span><span>%.1f %%</span></div>", currentHX711Metrics.foraging_efficiency);
      client.printf("<div class='metric'><span>Intensywność pożytku</span><span>%.1f %%</span></div>", currentHX711Metrics.bloom_intensity);
      client.printf("<div class='metric'><span>Indeks produkcji miodu</span><span>%.1f %%</span></div>", currentHX711Metrics.honey_production_idx);
      client.printf("<div class='metric'><span>Szacowany zbiór</span><span>%.2f kg</span></div>", currentHX711Metrics.expected_honey_yield);
      client.println("</div>");
      
      // Zdrowie kolonii z wagi
      client.println("<div class='card'><h2>🐛 Zdrowie Kolonii (z wagi)</h2>");
      client.printf("<div class='metric'><span>Wzrost kolonii</span><span>%.2f %%/dz</span></div>", currentHX711Metrics.colony_growth_rate);
      client.printf("<div class='metric'><span>Aktywność czerwiu</span><span>%.1f %%</span></div>", currentHX711Metrics.brood_activity_idx);
      client.printf("<div class='metric'><span>Szacowana populacja</span><span>%.1f tys.</span></div>", currentHX711Metrics.population_estimate_k);
      client.printf("<div class='metric'><span>Indeks zdrowia</span><span>%.1f %%</span></div>", currentHX711Metrics.hive_health_weight);
      client.printf("<div class='metric'><span>Produktywność</span><span>%.1f %%</span></div>", currentHX711Metrics.productivity_score);
      client.println("</div>");
      
      // Audio - dźwięki ula
      client.println("<div class='card'><h2>🎵 Analiza Audio Ula</h2>");
      client.printf("<div class='metric'><span>RMS amplituda</span><span>%.3f V</span></div>", currentAudioMetrics.rms_amplitude);
      client.printf("<div class='metric'><span>Częstotliwość dominująca</span><span>%.1f Hz</span></div>", currentAudioMetrics.dominant_frequency);
      client.printf("<div class='metric'><span>Aktywność pszczół</span><span>%.1f %%</span></div>", currentAudioMetrics.bee_activity_index);
      client.printf("<div class='metric'><span>Prawdopodobieństwo rojenia</span><span>%.2f</span></div>", currentAudioMetrics.swarm_probability);
      client.printf("<div class='metric'><span>Zdrowie (audio)</span><span>%.1f %%</span></div>", currentAudioMetrics.hive_health_audio);
      client.printf("<div class='metric'><span>Stres kolonii</span><span>%.2f</span></div>", currentAudioMetrics.stress_indicator);
      client.printf("<div class='metric'><span>Efektywność zbierania</span><span>%.1f %%</span></div>", currentAudioMetrics.foraging_efficiency);
      client.printf("<div class='metric'><span>Spójność kolonii</span><span>%.2f</span></div>", currentAudioMetrics.colony_coherence);
      client.printf("<div class='metric'><span>Entropia widmowa</span><span>%.3f</span></div>", currentAudioMetrics.spectral_entropy);
      client.printf("<div class='metric'><span>Tonalność</span><span>%.2f</span></div>", currentAudioMetrics.tonal_strength);
      client.println("</div>");
      
      // Zaawansowane parametry audio
      client.println("<div class='card'><h2>🎼 Zaawansowana Analiza Audio</h2>");
      client.printf("<div class='metric'><span>Formant 1</span><span>%.1f Hz</span></div>", currentAudioMetrics.formant_freq_1);
      client.printf("<div class='metric'><span>Formant 2</span><span>%.1f Hz</span></div>", currentAudioMetrics.formant_freq_2);
      client.printf("<div class='metric'><span>Częstotliwość podstawowa F0</span><span>%.1f Hz</span></div>", currentAudioMetrics.fundamental_frequency);
      client.printf("<div class='metric'><span>Siła wysokości (pitch)</span><span>%.2f</span></div>", currentAudioMetrics.pitch_strength);
      client.printf("<div class='metric'><span>Nieharmoniczność</span><span>%.2f</span></div>", currentAudioMetrics.inharmonicity);
      client.printf("<div class='metric'><span>Shimmer (fluktuacja amp.)</span><span>%.1f %%</span></div>", currentAudioMetrics.shimmer);
      client.printf("<div class='metric'><span>Jitter (fluktuacja freq.)</span><span>%.1f %%</span></div>", currentAudioMetrics.jitter);
      client.printf("<div class='metric'><span>Stosunek szumu do harmonicznych</span><span>%.2f</span></div>", currentAudioMetrics.noise_to_harmonic_ratio);
      client.printf("<div class='metric'><span>Autokorelacja</span><span>%.2f</span></div>", currentAudioMetrics.autocorrelation_peak);
      client.printf("<div class='metric'><span>Głośność psychoakustyczna</span><span>%.2f sones</span></div>", currentAudioMetrics.loudness_zwicker);
      client.println("</div>");
      
      // Radar MMWave
      client.println("<div class='card'><h2>📡 Radar MMWave - Ruch Pszczół</h2>");
      if(radarDataCount > 0) {
        RadarDataPoint& lr = radarBuffer[(radarBufferIndex>0)?radarBufferIndex-1:RADAR_BUFFER_SIZE-1];
        client.printf("<div class='metric'><span>Liczba celów</span><span>%d</span></div>", lr.target_count);
        client.printf("<div class='metric'><span>Dystans średni</span><span>%.3f m</span></div>", lr.distance);
        client.printf("<div class='metric'><span>Energia sygnału</span><span>%.2f</span></div>", lr.energy);
        client.printf("<div class='metric'><span>Prędkość radialna</span><span>%.2f m/s</span></div>", lr.radial_velocity);
        client.printf("<div class='metric'><span>Zdrowie ula (radar)</span><span>%.1f %%</span></div>", currentMetrics.hive_health_index);
        client.printf("<div class='metric'><span>Wskaźnik aktywności</span><span>%.1f %%</span></div>", currentMetrics.activity_ratio);
        client.printf("<div class='metric'><span>CV energii</span><span>%.3f</span></div>", currentMetrics.energy_cv);
        client.printf("<div class='metric'><span>Tempo pojawiania się celów</span><span>%.1f/min</span></div>", currentMetrics.target_rate);
        client.printf("<div class='metric'><span>Jakość sygnału</span><span>%.1f %%</span></div>", currentMetrics.signal_quality);
        client.printf("<div class='metric'><span>Entropia</span><span>%.3f</span></div>", currentMetrics.entropy);
        client.printf("<div class='metric'><span>Nachylenie trendu</span><span>%.4f</span></div>", currentMetrics.trend_slope);
      } else {
        client.println("<div class='metric'><span>Brak danych z radaru</span><span>-</span></div>");
      }
      client.println("</div>");
      
      // Jakość powietrza
      client.println("<div class='card'><h2>💨 Jakość Powietrza w Ulu</h2>");
      client.printf("<div class='metric'><span>CO₂ (równoważnik)</span><span>%d ppm</span></div>", currentAirQualityMetrics.co2_equivalent);
      client.printf("<div class='metric'><span>VOC Index</span><span>%d</span></div>", currentAirQualityMetrics.voc_index);
      client.printf("<div class='metric'><span>NOx (równoważnik)</span><span>%d</span></div>", currentAirQualityMetrics.no_x_equivalent);
      client.printf("<div class='metric'><span>IAQ Index</span><span>%.0f</span></div>", currentAirQualityMetrics.iaq_index);
      client.printf("<div class='metric'><span>Poziom jakości powietrza</span><span>%d</span></div>", currentAirQualityMetrics.air_quality_level);
      client.printf("<div class='metric'><span>Potrzeba wentylacji</span><span>%.2f</span></div>", currentAirQualityMetrics.ventilation_need);
      client.printf("<div class='metric'><span>Komfort ula</span><span>%.1f %%</span></div>", currentAirQualityMetrics.hive_comfort_index);
      client.printf("<div class='metric'><span>Ryzyko pleśni</span><span>%.1f %%</span></div>", currentAirQualityMetrics.mold_risk);
      client.printf("<div class='metric'><span>Stres od powietrza</span><span>%.2f</span></div>", currentAirQualityMetrics.stress_from_air);
      client.printf("<div class='metric'><span>Wynik ryzyka łączonego</span><span>%.2f</span></div>", currentAirQualityMetrics.combined_risk_score);
      client.println("</div>");
      
      // Wibracje z czujnika Piezo
      client.println("<div class='card'><h2>📳 Wibracje (Piezo)</h2>");
      client.printf("<div class='metric'><span>RMS wibracji</span><span>%.2f mV</span></div>", currentPiezoMetrics.vibration_rms);
      client.printf("<div class='metric'><span>Amplituda szczytowa</span><span>%.2f mV</span></div>", currentPiezoMetrics.vibration_peak);
      client.printf("<div class='metric'><span>Indeks aktywności wibracyjnej</span><span>%.1f %%</span></div>", currentPiezoMetrics.vibration_activity_idx);
      client.printf("<div class='metric'><span>Wynik ruchu pszczół</span><span>%.1f %%</span></div>", currentPiezoMetrics.bee_traffic_score);
      client.printf("<div class='metric'><span>Wynik drapieżnika</span><span>%.1f %%</span></div>", currentPiezoMetrics.predator_vib_score);
      client.printf("<div class='metric'><span>Prawdopodobieństwo intruza</span><span>%.2f</span></div>", currentPiezoMetrics.intrusion_probability);
      client.printf("<div class='metric'><span>Flaga wibracji rojowej</span><span>%.2f</span></div>", currentPiezoMetrics.swarm_vibration_flag);
      client.println("</div>");
      
      // Sekcja debugowania - szczegółowe informacje systemowe
      client.println("<div class='debug-section'>");
      client.println("<div class='debug-title'>🔍 Szczegółowe Debugowanie Systemu</div>");
      
      client.println("<div class='debug-row'><span>Status sensorów:</span>");
      client.printf("<span>Temp/Wilg:%s | HX711:%s | Audio:%s | Radar:%s | AirQual:%s | Piezo:%s</span></div>",
                    sensors.tempHum.active ? "OK" : "OFF",
                    sensors.hx711.active ? "OK" : "OFF",
                    sensors.audio.active ? "OK" : "OFF",
                    sensors.radar.active ? "OK" : "OFF",
                    sensors.airQual.active ? "OK" : "OFF",
                    sensors.piezo.active ? "OK" : "OFF");
      
      client.println("<div class='debug-row'><span>Ostatnie zdarzenia audio:</span>");
      client.printf("<span>Typ:%s | Wpływ:%s | Pewność:%.1f%%</span></div>",
                    audioEventTypeToString(lastAudioEvent.type),
                    audioEventImpactToString(lastAudioEvent.impact),
                    lastAudioEvent.confidence * 100.0f);
      
      client.println("<div class='debug-row'><span>Ostatnie zdarzenia wagi:</span>");
      client.printf("<span>Typ:%s | Wpływ:%s | Magnituda:%.3fkg</span></div>",
                    hx711EventTypeToString(lastHX711Event.type),
                    hx711EventImpactToString(lastHX711Event.impact),
                    lastHX711Event.magnitude_kg);
      
      client.println("<div class='debug-row'><span>Ostatnie anomalie radaru:</span>");
      client.printf("<span>Typ:%s | Kierunek:%s | Poważność:%.2f</span></div>",
                    anomalyTypeToString(lastAnomaly.type),
                    changeDirectionToString(lastAnomaly.direction),
                    lastAnomaly.severity);
      
      client.println("<div class='debug-row'><span>Prognoza wagi 24h:</span>");
      client.printf("<span>Przewidywana:%.3fkg | Pewność:%.1f%% | Oczekiwany zbiór:%.2fkg</span></div>",
                    currentHX711Metrics.predicted_weight_24h,
                    currentHX711Metrics.forecast_confidence,
                    currentHX711Metrics.expected_honey_yield);
      
      client.println("<div class='debug-row'><span>Statystyki audio (szczegóły):</span>");
      client.printf("<span>CrestFactor:%.2f | SpectCentroid:%.1fHz | RollOff:%.1fHz | Flux:%.3f</span></div>",
                    currentAudioMetrics.crest_factor,
                    currentAudioMetrics.spectral_centroid,
                    currentAudioMetrics.spectral_rolloff,
                    currentAudioMetrics.spectral_flux);
      
      client.println("<div class='debug-row'><span>Pasma mocy audio:</span>");
      client.printf("<span>0-50Hz:%.1fdB | 50-100Hz:%.1fdB | 100-200Hz:%.1fdB | 200-400Hz:%.1fdB | 400-800Hz:%.1fdB</span></div>",
                    currentAudioMetrics.power_band_1,
                    currentAudioMetrics.power_band_2,
                    currentAudioMetrics.power_band_3,
                    currentAudioMetrics.power_band_4,
                    currentAudioMetrics.power_band_5);
      
      client.println("<div class='debug-row'><span>Indeksy bioakustyczne:</span>");
      client.printf("<span>ACI:%.2f | BI:%.2f | NDI:%.2f | ADI:%.2f | AEI:%.2f</span></div>",
                    currentAudioMetrics.acoustic_complexity,
                    currentAudioMetrics.bioacoustic_index,
                    currentAudioMetrics.normalized_difference,
                    currentAudioMetrics.acoustic_diversity,
                    currentAudioMetrics.acoustic_evenness);
      
      client.println("<div class='debug-row'><span>Analiza trendów wagi:</span>");
      client.printf("<span>Slope1h:%.4f | Slope4h:%.4f | Slope24h:%.4f | Correlation:%.2f | Direction:%s</span></div>",
                    currentHX711Metrics.trend_slope_1h,
                    currentHX711Metrics.trend_slope_4h,
                    currentHX711Metrics.trend_slope_24h,
                    currentHX711Metrics.trend_correlation,
                    getTrendDirectionName(currentHX711Metrics.trend_direction).c_str());
      
      client.println("<div class='debug-row'><span>Zmienność wagi:</span>");
      client.printf("<span>StdDev:%.4fkg | CV:%.2f%% | IQR:%.3fkg | Range:%.3fkg | StabilityIdx:%.1f%%</span></div>",
                    currentHX711Metrics.weight_std_dev,
                    currentHX711Metrics.weight_cv,
                    currentHX711Metrics.weight_iqr,
                    currentHX711Metrics.weight_range,
                    currentHX711Metrics.stability_index);
      
      client.println("<div class='debug-row'><span>Analiza cykliczna:</span>");
      client.printf("<span>Amplituda dobowa:%.3fkg | Siła cyrkadiana:%.2f | Trend sezonowy:%s</span></div>",
                    currentHX711Metrics.daily_amplitude,
                    currentHX711Metrics.circadian_strength,
                    getSeasonalTrendName(currentHX711Metrics.seasonal_trend).c_str());
      
      client.println("</div>");
      
      // Linki API
      client.println("<div class='api-links'>");
      client.println("<h3>🔗 Endpointy API JSON:</h3>");
      client.println("<a href='/status'>Status ogólny</a> ");
      client.println("<a href='/radar/status'>Radar status</a> ");
      client.println("<a href='/radar/anomalies'>Anomalie radaru</a> ");
      client.println("<a href='/hx711/status'>Waga status</a> ");
      client.println("<a href='/hx711/metrics'>Waga metryki</a> ");
      client.println("<a href='/hx711/events'>Zdarzenia wagi</a> ");
      client.println("<a href='/audio/status'>Audio status</a> ");
      client.println("<a href='/audio/spectrum'>Widmo audio</a> ");
      client.println("<a href='/airquality/status'>Jakość powietrza</a> ");
      client.println("<a href='/airquality/metrics'>Metryki powietrza</a> ");
      client.println("</div>");
      
      client.println("<h3>⚡ Sterowanie efektorem:</h3>");
      client.println("<div class='api-links'>");
      client.println("<a href='/heater/on'>Grzałka ON</a> ");
      client.println("<a href='/heater/off'>Grzałka OFF</a> ");
      client.println("<a href='/fan/on'>Wentylator ON</a> ");
      client.println("<a href='/fan/off'>Wentylator OFF</a> ");
      client.println("<a href='/pump/on'>Pompa ON</a> ");
      client.println("<a href='/pump/off'>Pompa OFF</a> ");
      client.println("</div>");
      
      client.println("</body></html>");
    }
    else if(request.indexOf("/status") > 0) {
      sendHeader("application/json");
      client.printf("{\"temp\":%.1f,\"hum\":%.1f,\"weight\":%.3f,\"co2\":%d,\"voc\":%d}", temperature, humidity, (float)hx711_value/1000.0f, co2_eq, voc_idx);
    }
    else if(request.indexOf("/heater/on") > 0) { analogWrite(HEATER_PWM, 200); sendHeader("text/plain"); client.println("Heater ON"); }
    else if(request.indexOf("/heater/off") > 0) { analogWrite(HEATER_PWM, 0); sendHeader("text/plain"); client.println("Heater OFF"); }
    else if(request.indexOf("/fan/on") > 0) { analogWrite(FAN_PWM, 255); sendHeader("text/plain"); client.println("Fan ON"); }
    else if(request.indexOf("/fan/off") > 0) { analogWrite(FAN_PWM, 0); sendHeader("text/plain"); client.println("Fan OFF"); }
    else if(request.indexOf("/pump/on") > 0) { digitalWrite(PUMP_RELAY, HIGH); sendHeader("text/plain"); client.println("Pump ON"); }
    else if(request.indexOf("/pump/off") > 0) { digitalWrite(PUMP_RELAY, LOW); sendHeader("text/plain"); client.println("Pump OFF"); }
    else if(request.indexOf("/radar/status") > 0) {
      sendHeader("application/json");
      client.print("{\"radar_data_count\":"); client.print(radarDataCount);
      client.print(",\"last_target_count\":"); client.print(radarBuffer[(radarBufferIndex>0)?radarBufferIndex-1:RADAR_BUFFER_SIZE-1].target_count);
      client.print(",\"last_distance\":"); client.print(radarBuffer[(radarBufferIndex>0)?radarBufferIndex-1:RADAR_BUFFER_SIZE-1].distance, 3);
      client.print(",\"hive_health_index\":"); client.print(currentMetrics.hive_health_index, 2);
      client.println("}");
    }
    else if(request.indexOf("/radar/anomalies") > 0) {
      sendHeader("application/json");
      client.print("{\"last_anomaly\":{\"type\":\""); client.print(anomalyTypeToString(lastAnomaly.type));
      client.print("\",\"pozytek_status\":\""); 
      client.print(isPositiveChange(lastAnomaly) ? "POZYTYWNY" : (lastAnomaly.type == AnomalyEvent::NONE ? "NORMALNY" : "NEGATYWNY"));
      client.println("\"}}");
    }
    
    delay(1);
    client.stop();
  }
  
  // 3. Wysyłanie danych UDP do Raspberry Pi (co 5 sekund)
  static unsigned long lastUdpSend = 0;
  if(now - lastUdpSend > 5000) {
    lastUdpSend = now;
    sendUdpDataToRpi();
  }
  
  // Heartbeat sieciowy (opcjonalne pingi)
  if(now - networkHeartbeat > 10000) {
    networkHeartbeat = now;
    Serial.println(\"[NETWORK] Heartbeat - system aktywny\");
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

// ============================================================================
// IMPLEMENTACJA FUNKCJI WYKRYWANIA SENSORÓW (DODANE NA KOŃCU PLIKU)
// ============================================================================

void detectAllSensors() {
  Serial.println(">> Wykrywanie sensorów...");
  
  // 1. HX711 - Sprawdzenie sygnału na pinie DT
  sensors.hx711.detected = checkHX711();
  sensors.hx711.active = sensors.hx711.detected;
  
  // 2. Audio (MEMS Mic) - Sprawdzenie obecności sygnału na ADC
  sensors.audio.detected = checkAudio();
  sensors.audio.active = sensors.audio.detected;
  
  // 3. Radar MMWave - Sprawdzenie komunikacji UART
  sensors.radar.detected = checkRadar();
  sensors.radar.active = sensors.radar.detected;
  
  // 4. Temp/Humidity (DHT22/SHT40/BME280)
  sensors.tempHum.detected = checkTempHumidity();
  sensors.tempHum.active = sensors.tempHum.detected;
  
  // 5. Air Quality (SGP41)
  sensors.airQual.detected = checkAirQuality();
  sensors.airQual.active = sensors.airQual.detected;
  
  // 6. Piezo Vibration - Sprawdzenie pinu Analogowego
  sensors.piezo.detected = checkPiezo();
  sensors.piezo.active = sensors.piezo.detected;
  
  // 7. Barometric (BME280) - Często ten sam chip co temp, ale sprawdzamy osobno
  sensors.baro.detected = checkBarometric();
  sensors.baro.active = sensors.baro.detected;
  
  // 8. Light Sensor (BH1750 lub analogowy)
  sensors.light.detected = checkLight();
  sensors.light.active = sensors.light.detected;
}

void printSensorStatus() {
  Serial.println("");
  Serial.println("=== STATUS SENSORÓW ===");
  
  #define PRINT_STATUS(name, displayName) \
    Serial.print(displayName); \
    Serial.print(": "); \
    if (sensors.name.detected) { \
      Serial.println("OK [AKTYWNY]"); \
    } else { \
      Serial.println("NIEWYKRYTY [WYŁĄCZONY]"); \
    }
  
  PRINT_STATUS(hx711, "HX711 (Waga)         ");
  PRINT_STATUS(audio, "Audio (MEMS Mic)     ");
  PRINT_STATUS(radar, "Radar MMWave LD2410B ");
  PRINT_STATUS(tempHum, "Temp/Humidity        ");
  PRINT_STATUS(airQual, "Air Quality (SGP41)  ");
  PRINT_STATUS(piezo, "Piezo Vibration      ");
  PRINT_STATUS(baro, "Barometric (BME280)  ");
  PRINT_STATUS(light, "Light Sensor         ");
  
  Serial.println("=======================");
  Serial.println("");
}

bool checkHX711() {
  pinMode(HX711_DT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  digitalWrite(HX711_SCK, LOW);
  delayMicroseconds(10);
  
  int val = digitalRead(HX711_DT);
  delayMicroseconds(10);
  int val2 = digitalRead(HX711_DT);
  
  return (val != val2) || (val >= 0);
}

bool checkAudio() {
  int readings[10];
  int minVal = 4096, maxVal = 0;
  
  for(int i=0; i<10; i++) {
    readings[i] = analogRead(MIC_PIN);
    if(readings[i] < minVal) minVal = readings[i];
    if(readings[i] > maxVal) maxVal = readings[i];
    delay(2);
  }
  
  int range = maxVal - minVal;
  return (range > 50);
}

bool checkRadar() {
  radarSerial.setTimeout(100);
  
  uint8_t cmdEnterConfig[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x01, 0x00, 0x01, 0x00};
  radarSerial.write(cmdEnterConfig, 8);
  delay(100);
  
  if(radarSerial.available() > 0) {
    while(radarSerial.available()) radarSerial.read();
    
    uint8_t cmdExitConfig[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x02, 0x00};
    radarSerial.write(cmdExitConfig, 8);
    return true;
  }
  
  return false;
}

bool checkTempHumidity() {
  pinMode(DHT_PIN, INPUT_PULLUP);
  dht.begin();
  delay(100);
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if(!isnan(h) && !isnan(t)) {
    return true;
  }
  
  Wire.beginTransmission(0x44);
  if(Wire.endTransmission() == 0) return true;
  
  Wire.beginTransmission(0x76);
  if(Wire.endTransmission() == 0) return true;
  
  Wire.beginTransmission(0x77);
  if(Wire.endTransmission() == 0) return true;
  
  return false;
}

bool checkAirQuality() {
  Wire.beginTransmission(0x59);
  if(Wire.endTransmission() == 0) return true;
  
  Wire.beginTransmission(0x58);
  if(Wire.endTransmission() == 0) return true;
  
  return false;
}

bool checkPiezo() {
  int readings[20];
  int minVal = 4096, maxVal = 0;
  
  for(int i=0; i<20; i++) {
    readings[i] = analogRead(PIEZO_PIN);
    if(readings[i] < minVal) minVal = readings[i];
    if(readings[i] > maxVal) maxVal = readings[i];
    delay(5);
  }
  
  int range = maxVal - minVal;
  return (range > 100) || (minVal > 10 && maxVal < 4000);
}

bool checkBarometric() {
  Wire.beginTransmission(0x76);
  if(Wire.endTransmission() == 0) return true;
  
  Wire.beginTransmission(0x77);
  if(Wire.endTransmission() == 0) return true;
  
  return false;
}

bool checkLight() {
  Wire.beginTransmission(0x23);
  if(Wire.endTransmission() == 0) return true;
  
  Wire.beginTransmission(0x5C);
  if(Wire.endTransmission() == 0) return true;
  
  int val = analogRead(26);
  return (val >= 0 && val <= 4095);
}
