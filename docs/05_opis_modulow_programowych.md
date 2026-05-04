## 💻 Opis Modułów Programowych

### Moduł Raspberry Pi Pico (C++)

#### Architektura Firmware

Firmware Raspberry Pi Pico został napisany w C++ z wykorzystaniem Raspberry Pi Pico SDK, zapewniając deterministyczne działanie w czasie rzeczywistym. Kod jest modularny, z wyraźnym rozdziałem odpowiedzialności między warstwę sprzętową (HAL), logikę biznesową i komunikację HTTP.

```cpp
// Przykład: main.cpp - Główna pętla Raspberry Pi Pico
#include "pico/stdlib.h"
#include "config/pin_definitions.h"
#include "sensors/hx711_driver.h"
#include "sensors/microphone_adc.h"
#include "actuators/heater_control.h"
#include "communication/http_server.h"

void setup() {
    stdio_init_all();
    
    // Inicjalizacja WiFi (Pico W) lub Ethernet (W6100)
    init_network();
    
    // Inicjalizacja sensorów
    HX711_init();
    MICROPHONE_init(ADC_SAMPLE_RATE);
    DHT22_begin();
    
    // Kalibracja
    if (EEPROM_read(CALIBRATION_FLAG) == 0) {
        perform_auto_calibration();
    }
    
    // Konfiguracja watchdog
    watchdog_enable(2000, true);
    
    // Start HTTP server
    httpServer.begin();
}

void loop() {
    watchdog_update();
    
    // Akwizycja danych (non-blocking)
    uint32_t weight = HX711_read_average(10);
    float temp = DHT22_readTemperature();
    float humidity = DHT22_readHumidity();
    uint16_t audio_level = MICROPHONE_get_rms();
    
    // Pakietowanie danych
    SensorPacket packet;
    packet.weight = weight;
    packet.temperature = temp;
    packet.humidity = humidity;
    packet.audio_rms = audio_level;
    packet.timestamp = time_us_32();
    
    // Obsługa żądań HTTP z Raspberry Pi 2
    httpServer.handleClient();
    
    // Obsługa komend z RPi przez HTTP API
    process_actuator_commands();
    
    sleep_ms(100); // 10Hz sampling rate
}
```

#### Kluczowe Komponenty Raspberry Pi Pico

1. **HX711 Driver**: 
   - 24-bitowa konwersja ADC
   - Programmable gain amplifier (32/64/128)
   - Auto-zero i auto-calibration
   - Kompensacja dryfu temperaturowego

2. **Audio Processing**:
   - Sampling 8kHz/16-bit
   - RMS calculation w czasie rzeczywistym
   - FFT preprocessing (opcjonalnie)
   - Buffer ringowy dla efektywności

3. **PID Controller**:
   - Implementacja algorytmu PID dla grzałki
   - Auto-tuning parametrów
   - Anti-windup protection
   - Output limiting

4. **HTTP Server Protocol**:
   - RESTful API endpoints
   - JSON payload format
   - Checksum validation
   - Retry mechanism
   - Multi-client support

### Moduł Raspberry Pi 2 (Bash + C++)

#### Architektura Aplikacji

Aplikacja na Raspberry Pi 2 wykorzystuje Bash do operacji systemowych i TUI/GUI, oraz C++ do wysokowydajnego przetwarzania sygnałów. Komunikacja z Raspberry Pi Pico odbywa się przez HTTP API.

##### Warstwa Domain (ApiaryGuard.Core - C++)

```cpp
// Model: Hive.hpp
namespace ApiaryGuard::Core::Models
{
    struct Hive
    {
        int id;
        std::string name;
        std::string location;
        std::chrono::system_clock::time_point installationDate;
        HiveStatus status;
        
        // Navigation properties
        std::vector<SensorReading> sensorReadings;
        std::vector<Alert> alerts;
        std::vector<Treatment> treatments;
        
        // Business logic
        bool IsSwarmingRisk() const
        {
            auto recentWeights = sensorReadings
                | std::views::filter([](const auto& r) { return r.sensorType == SensorType::Weight; })
                | std::views::take(48); // Ostatnie 48 godzin
            
            if (recentWeights.size() < 2) return false;
            
            auto weightDrop = recentWeights.front().value - recentWeights.back().value;
            return weightDrop > 2000; // 2kg spadek = ryzyko rojenia
        }
        
        double CalculateHealthScore() const
        {
            // Algorytm oceny zdrowia rodziny
            auto weightScore = GetWeightScore();
            auto tempScore = GetTemperatureScore();
            auto audioScore = GetAudioActivityScore();
            auto humidityScore = GetHumidityScore();
            
            return (weightScore * 0.3 + tempScore * 0.25 + 
                    audioScore * 0.25 + humidityScore * 0.2);
        }
    };
}
```

##### Warstwa Services (C++)

```cpp
// Service: SensorService.hpp
namespace ApiaryGuard::Core::Services
{
    class SensorService : public ISensorService
    {
    private:
        std::unique_ptr<IPicoCommunication> picoComm;
        std::unique_ptr<IRepository<SensorReading>> readingRepo;
        spdlog::logger& logger;
        
    public:
        SensorService(
            std::unique_ptr<IPicoCommunication> picoComm,
            std::unique_ptr<IRepository<SensorReading>> readingRepo,
            spdlog::logger& logger);
        
        std::future<SensorReading> AcquireLatestReadings(int hiveId) override
        {
            try
            {
                // Pobranie danych z Raspberry Pi Pico przez HTTP API
                auto rawPacket = picoComm->ReadSensorPacket();
                
                // Transformacja i walidacja
                SensorReading reading;
                reading.hiveId = hiveId;
                reading.timestamp = std::chrono::system_clock::now();
                reading.weight = rawPacket.weight / 1000.0; // Convert to kg
                reading.temperature = rawPacket.temperature;
                reading.humidity = rawPacket.humidity;
                reading.audioLevel = rawPacket.audioRms;
                reading.piezoActivity = rawPacket.piezoCount;
                
                // Walidacja zakresów
                if (!ValidateReading(reading))
                {
                    logger.warn("Invalid reading from hive {}", hiveId);
                    throw InvalidSensorDataException("Reading out of expected range");
                }
                
                // Persist do bazy
                readingRepo->AddAsync(reading);
                
                // Trigger eventów
                CheckThresholdsAndTriggerAlerts(reading);
                
                return std::async(std::launch::deferred, [reading]() { return reading; });
            }
            catch (const CommunicationException& ex)
            {
                logger.error("Failed to acquire readings from hive {}: {}", hiveId, ex.what());
                throw;
            }
        }
        
    private:
        void CheckThresholdsAndTriggerAlerts(const SensorReading& reading)
        {
            // Reguły biznesowe dla alertów
            if (reading.weight < 5.0) // Krytycznie niska waga
            {
                TriggerAlert(AlertType::CriticalLowWeight, reading);
            }
            
            if (reading.temperature > 36.0) // Przegrzanie
            {
                TriggerAlert(AlertType::Overheating, reading);
                ActivateCoolingFan(reading.hiveId);
            }
            
            if (reading.humidity > 75.0) // Zbyt wysoka wilgotność
            {
                TriggerAlert(AlertType::HighHumidity, reading);
                ActivateVentilation(reading.hiveId);
            }
        }
    };
}
```

##### Background Workers (C++)

```cpp
// Worker: BeeSoundAnalyzerWorker.hpp
namespace ApiaryGuard::Worker
{
    class BeeSoundAnalyzerWorker : public IBackgroundWorker
    {
    private:
        spdlog::logger& logger;
        std::unique_ptr<IAudioProcessor> audioProcessor;
        std::atomic<bool> running{false};
        
    public:
        BeeSoundAnalyzerWorker(
            spdlog::logger& logger,
            std::unique_ptr<IAudioProcessor> audioProcessor);
        
        void Start() override
        {
            logger.info("Bee Sound Analyzer Worker starting...");
            running = true;
            
            while (running)
            {
                // Pobranie ostatnich nagrań audio
                auto audioSamples = GetUnanalyzedAudioSamples(std::chrono::hours(1));
                
                for (const auto& sample : audioSamples)
                {
                    try
                    {
                        // Analiza FFT i ekstrakcja cech
                        auto features = audioProcessor->ExtractFeatures(sample.filePath);
                        
                        // Klasyfikacja stanu rodziny
                        auto classification = ClassifyBeeState(features);
                        
                        // Predykcja rojenia
                        auto swarmProbability = PredictSwarming(features);
                        
                        // Zapis wyników
                        SaveAnalysisResult(sample.id, classification, swarmProbability);
                        
                        if (swarmProbability > 0.7)
                        {
                            CreateSwarmAlert(sample.hiveId, swarmProbability);
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        logger.error("Error analyzing audio sample {}: {}", sample.id, ex.what());
                    }
                }
                
                // Uruchomienie co 5 minut
                std::this_thread::sleep_for(std::chrono::minutes(5));
            }
            
            logger.info("Bee Sound Analyzer Worker stopped.");
        }
        
        void Stop() override { running = false; }
        
    private:
        double PredictSwarming(const AudioFeatures& features)
        {
            // Implementacja modelu ML
            // - Detekcja piping sounds (queen pipes)
            // - Analiza częstotliwości skrzydeł
            // - Pattern recognition zachowań przedrojowych
            
            auto pipingDetected = std::any_of(
                features.frequencyPeaks.begin(),
                features.frequencyPeaks.end(),
                [](const auto& p) { return p.frequency >= 200 && p.frequency <= 300; });
            
            auto increasedActivity = features.activityLevel > threshold;
            
            auto weightDrop = GetRecentWeightDrop(features.hiveId);
            
            // Ensemble prediction
            auto probability = (
                (pipingDetected ? 0.4 : 0) +
                (increasedActivity ? 0.3 : 0) +
                (weightDrop > 1000 ? 0.3 : 0)
            );
            
            return std::min(probability, 1.0);
        }
    };
}
```

### Moduły C++ (High-Performance Processing)

#### FFT Audio Analyzer (256-point Cooley-Tukey)

```cpp
// fft_analyzer.cpp - Implementacja na Raspberry Pi Pico
#include "fft_analyzer.hpp"
#include <cmath>
#include <vector>

namespace ApiaryGuard {
namespace SignalProcessing {

// Struktura przechowująca 47 parametrów akustycznych
struct AudioMetrics {
    // Parametry czasowe i amplitudowe (7)
    float rms_amplitude;
    float peak_amplitude;
    float peak_to_peak;
    float zero_crossing_rate;
    float signal_energy;
    float crest_factor;
    float average_amplitude;
    
    // Parametry statystyczne (6)
    float mean_value;
    float std_deviation;
    float skewness;
    float kurtosis;
    float coefficient_of_variation;
    float dynamic_range;
    
    // Parametry częstotliwościowe (8)
    float dominant_frequency;
    float spectral_centroid;
    float spectral_bandwidth;
    float spectral_flatness;
    float spectral_rolloff;
    float spectral_entropy;
    float harmonic_to_noise_ratio;
    float autocorrelation_peak;
    
    // Moc w pasmach (5)
    float power_low_freq;
    float power_bee_band;
    float power_swarm_band;
    float power_mid_freq;
    float power_high_freq;
    
    // Wskaźniki klasyfikacji (4)
    float bee_activity_index;
    float swarm_probability;
    float stress_indicator;
    float hive_health_audio;
    
    // Formanty i jakość dźwięku (8)
    float formant_f1, formant_f2, formant_f3;
    float brightness;
    float roughness;
    float sharpness;
    float tonality;
    float prominence_ratio;
    
    // Cechy temporalne (5)
    float attack_time;
    float decay_time;
    float temporal_centroid;
    float silence_ratio;
    float modulation_index;
    
    // Parametry psychoakustyczne (4)
    float loudness;
    float roughness_fast;
    float spectral_decrease;
    float irregularity;
};

class FFTAnalyzer {
private:
    static const int FFT_SIZE = 256;
    float fft_real[FFT_SIZE];
    float fft_imag[FFT_SIZE];
    float spectrum[FFT_SIZE / 2];
    
    // Pełna implementacja Cooley-Tukey radix-2 FFT
    void performFFT(int16_t* input, int size) {
        // Inicjalizacja buforów
        for (int i = 0; i < size; i++) {
            fft_real[i] = (float)input[i] / 2048.0f;
            fft_imag[i] = 0.0f;
        }
        
        // Bit-reversal permutation
        int j = 0;
        for (int i = 0; i < size; i++) {
            if (i < j) {
                float temp_re = fft_real[i];
                float temp_im = fft_imag[i];
                fft_real[i] = fft_real[j];
                fft_imag[i] = fft_imag[j];
                fft_real[j] = temp_re;
                fft_imag[j] = temp_im;
            }
            
            int k = size >> 1;
            while (k <= j) {
                j -= k;
                k >>= 1;
            }
            j += k;
        }
        
        // Butterfly operations z pełną mnożeniem zespolonym
        for (int len = 2; len <= size; len <<= 1) {
            float angle = -2.0f * PI / len;
            float w_re = cosf(angle);
            float w_im = sinf(angle);
            
            for (int i = 0; i < size; i += len) {
                float cur_w_re = 1.0f;
                float cur_w_im = 0.0f;
                
                for (int k = 0; k < len / 2; k++) {
                    int u = i + k;
                    int v = i + k + len / 2;
                    
                    // Pełne mnożenie zespolone
                    float t_re = fft_real[v] * cur_w_re - fft_imag[v] * cur_w_im;
                    float t_im = fft_real[v] * cur_w_im + fft_imag[v] * cur_w_re;
                    
                    fft_real[v] = fft_real[u] - t_re;
                    fft_imag[v] = fft_imag[u] - t_im;
                    fft_real[u] = fft_real[u] + t_re;
                    fft_imag[u] = fft_imag[u] + t_im;
                    
                    // Aktualizacja twiddle factor
                    float temp = cur_w_re * w_re - cur_w_im * w_im;
                    cur_w_im = cur_w_re * w_im + cur_w_im * w_re;
                    cur_w_re = temp;
                }
            }
        }
        
        // Obliczenie modułów widma
        for (int i = 0; i < size / 2; i++) {
            spectrum[i] = sqrtf(fft_real[i] * fft_real[i] + 
                               fft_imag[i] * fft_imag[i]);
        }
    }
    
public:
    FFTAnalyzer() {
        memset(fft_real, 0, sizeof(fft_real));
        memset(fft_imag, 0, sizeof(fft_imag));
        memset(spectrum, 0, sizeof(spectrum));
    }
    
    AudioMetrics analyze(const std::vector<int16_t>& audioSamples) {
        AudioMetrics metrics;
        memset(&metrics, 0, sizeof(AudioMetrics));
        
        // Wykonanie FFT
        performFFT((int16_t*)audioSamples.data(), FFT_SIZE);
        
        // Obliczenie wszystkich 47 parametrów
        calculateTimeDomainMetrics(metrics, audioSamples);
        calculateFrequencyDomainMetrics(metrics);
        calculateStatisticalMetrics(metrics, audioSamples);
        calculateClassificationMetrics(metrics);
        
        return metrics;
    }
};

} // namespace SignalProcessing
} // namespace ApiaryGuard
```
        
        for (int i = 0; i < sampleSize/2; ++i) {
            double magnitude = sqrt(
                fftOutput[i][0] * fftOutput[i][0] + 
                fftOutput[i][1] * fftOutput[i][1]
            );
            
            double frequency = (i * sampleRate) / sampleSize;
            
            // Bee-specific frequency ranges
            if (magnitude > THRESHOLD && 
                (frequency >= 100 && frequency <= 5000)) {
                peaks.push_back({frequency, magnitude});
            }
        }
        
        // Sort by magnitude and return top N peaks
        std::sort(peaks.begin(), peaks.end(), 
            [](const auto& a, const auto& b) {
                return a.magnitude > b.magnitude;
            });
        
        if (peaks.size() > 10) {
            peaks.resize(10);
        }
        
        return peaks;
    }
};

} // namespace SignalProcessing
} // namespace ApiaryGuard
```

#### Swarm Classification Model

```cpp
// swarm_classifier.cpp
#include "swarm_classifier.hpp"
#include <cmath>
#include <array>

namespace ApiaryGuard {
namespace MachineLearning {

class SwarmClassifier {
private:
    // Pre-trained model parameters (simplified example)
    std::array<double, 5> weights;
    double bias;
    
public:
    SwarmClassifier() {
        // Initialize with pre-trained weights
        weights = {0.25, 0.20, 0.30, 0.15, 0.10};
        bias = -0.5;
    }
    
    double predictSwarmProbability(const SwarmFeatures& features) {
        // Feature vector:
        // [0]: piping_sound_intensity
        // [1]: wing_beat_frequency_variance
        // [2]: activity_level_change
        // [3]: weight_drop_rate
        // [4]: temperature_anomaly
        
        double weightedSum = bias;
        
        weightedSum += weights[0] * normalize(features.pipingIntensity, 0, 100);
        weightedSum += weights[1] * normalize(features.wingBeatVariance, 0, 50);
        weightedSum += weights[2] * normalize(features.activityChange, -1, 1);
        weightedSum += weights[3] * normalize(features.weightDrop, 0, 5000);
        weightedSum += weights[4] * normalize(features.tempAnomaly, -5, 5);
        
        // Sigmoid activation
        return 1.0 / (1.0 + exp(-weightedSum));
    }
    
private:
    double normalize(double value, double min, double max) {
        return (value - min) / (max - min);
    }
};

} // namespace MachineLearning
} // namespace ApiaryGuard
```

#### WiFi Setup Script (wifi_setup.sh)

```bash
#!/bin/bash
# wifi_setup.sh - Konfiguracja połączenia WiFi
# Autor: ApiaryGuard Team
# Wersja: 2.1.0

set -euo pipefail

# Konfiguracja
WIFI_SSID="your_ssid"
WIFI_PASSWORD="your_password"
INTERFACE="wlan0"
LOG_FILE="/var/log/apiaryguard/wifi_setup.log"

# Logging function
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# Sprawdzenie uprawnień root
if [[ $EUID -ne 0 ]]; then
    log "ERROR: Ten skrypt musi być uruchomiony jako root"
    exit 1
fi

log "Rozpoczynanie konfiguracji WiFi..."

# Instalacja wymaganych pakietów
log "Instalacja zależności..."
apt-get update
apt-get install -y wireless-tools wpasupplicant network-manager

# Detekcja adaptera WiFi
log "Detekcja adaptera WiFi..."
if ! iwconfig 2>/dev/null | grep -q "wlan0"; then
    log "WARNING: Nie wykryto adaptera WiFi. Sprawdz połączenie."
    exit 1
fi

# Tworzenie konfiguracji wpa_supplicant
log "Tworzenie konfiguracji wpa_supplicant..."
cat > /etc/wpa_supplicant/wpa_supplicant.conf <<EOF
# Konfiguracja WiFi dla ApiaryGuard
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=PL

network={
    ssid="${WIFI_SSID}"
    psk="${WIFI_PASSWORD}"
    key_mgmt=WPA-PSK
}
EOF

# Konfiguracja network interface
log "Konfiguracja interfejsu sieciowego..."
cat >> /etc/network/interfaces <<EOF

# WiFi interface
auto wlan0
iface wlan0 inet dhcp
    wpa-conf /etc/wpa_supplicant/wpa_supplicant.conf
EOF

# Firewall rules
log "Konfiguracja firewall dla WiFi..."
iptables -A FORWARD -o $INTERFACE -j ACCEPT
iptables -A INPUT -i $INTERFACE -m state --state ESTABLISHED,RELATED -j ACCEPT

# Test połączenia
log "Testowanie połączenia..."
if iwconfig wlan0 | grep -q "ESSID"; then
    sleep 5
    
    if ping -c 4 -I wlan0 8.8.8.8 > /dev/null 2>&1; then
        log "SUCCESS: Połączenie WiFi aktywne!"
        ip addr show wlan0 | tee -a "$LOG_FILE"
    else
        log "ERROR: Brak łączności pomimo nawiązania połączenia WiFi"
        exit 1
    fi
else
    log "ERROR: Nie udało się nawiązać połączenia WiFi"
    exit 1
fi

# Konfiguracja auto-start
log "Dodawanie do auto-start..."
systemctl enable Networking.service 2>/dev/null || true

log "Konfiguracja WiFi zakończona pomyślnie!"
echo ""
echo "Aby sprawdzić status: iwconfig wlan0"
echo "Aby rozłączyć: ip link set wlan0 down"
echo "Aby połączyć: ip link set wlan0 up"
echo "Logi: journalctl -u Networking -f"
```
#### Health Check Script (health_check.sh)

```bash
#!/bin/bash
# health_check.sh - Kompleksowa diagnostyka systemu
# Uruchamiany co 5 minut przez cron

set -euo pipefail

HEALTH_REPORT="/var/log/apiaryguard/health_$(date +%Y%m%d_%H%M%S).json"
ALERT_THRESHOLD=3 # Liczba krytycznych błędów przed alertem
ERROR_COUNT=0

# Funkcja pomocnicza do logowania
check_status() {
    local service_name="$1"
    local check_command="$2"
    local status="OK"
    
    if ! eval "$check_command" > /dev/null 2>&1; then
        status="CRITICAL"
        ((ERROR_COUNT++))
        echo "[CRITICAL] $service_name - FAILED"
    else
        echo "[OK] $service_name - Running"
    fi
    
    echo "{\"service\": \"$service_name\", \"status\": \"$status\", \"timestamp\": \"$(date -Iseconds)\"}"
}

# Rozpoczęcie raportu
echo "{" > "$HEALTH_REPORT"
echo "  \"hostname\": \"$(hostname)\"," >> "$HEALTH_REPORT"
echo "  \"check_time\": \"$(date -Iseconds)\"," >> "$HEALTH_REPORT"
echo "  \"services\": [" >> "$HEALTH_REPORT"

# Sprawdzenie usług systemowych
check_status "ApiaryGuard Core (C++)" "systemctl is-active apiaryguard-core" >> "$HEALTH_REPORT"
check_status "ApiaryGuard Worker (C++)" "systemctl is-active apiaryguard-worker" >> "$HEALTH_REPORT"
check_status "HTTP API Service" "curl -sf http://localhost:8080/health > /dev/null" >> "$HEALTH_REPORT"
check_status "WiFi Connection" "iwconfig wlan0 | grep -q 'ESSID'" >> "$HEALTH_REPORT"

# Sprawdzenie zasobów
echo "  ]," >> "$HEALTH_REPORT"
echo "  \"resources\": {" >> "$HEALTH_REPORT"

# CPU Usage
CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1)
echo "    \"cpu_usage_percent\": $CPU_USAGE," >> "$HEALTH_REPORT"

# Memory Usage
MEM_INFO=$(free -m | awk 'NR==2{printf "%.2f", $3*100/$2}')
echo "    \"memory_usage_percent\": $MEM_INFO," >> "$HEALTH_REPORT"

# Disk Usage
DISK_USAGE=$(df -h / | awk 'NR==2{print $5}' | tr -d '%')
echo "    \"disk_usage_percent\": $DISK_USAGE," >> "$HEALTH_REPORT"

# Temperature CPU
if [[ -f /sys/class/thermal/thermal_zone0/temp ]]; then
    TEMP=$(cat /sys/class/thermal/thermal_zone0/temp)
    TEMP_C=$((TEMP / 1000))
    echo "    \"cpu_temperature_celsius\": $TEMP_C," >> "$HEALTH_REPORT"
fi

# Sprawdzenie sensorów
echo "  }," >> "$HEALTH_REPORT"
echo "  \"sensors\": {" >> "$HEALTH_REPORT"

# Test komunikacji z Raspberry Pi Pico przez HTTP API
if curl -sf http://pico.local/api/sensors > /dev/null 2>&1; then
    echo "    \"pico_connection\": \"OK\"," >> "$HEALTH_REPORT"
else
    echo "    \"pico_connection\": \"FAILED\"," >> "$HEALTH_REPORT"
    ((ERROR_COUNT++))
fi

# Ostatni odczyt wagi
LAST_WEIGHT=$(sqlite3 /var/lib/apiaryguard/data.db \
    "SELECT value FROM sensor_readings WHERE sensor_type='weight' ORDER BY timestamp DESC LIMIT 1;" 2>/dev/null || echo "null")
echo "    \"last_weight_kg\": $LAST_WEIGHT," >> "$HEALTH_REPORT"

# Czas ostatniego odczytu
LAST_READING_TIME=$(sqlite3 /var/lib/apiaryguard/data.db \
    "SELECT timestamp FROM sensor_readings ORDER BY timestamp DESC LIMIT 1;" 2>/dev/null || echo "null")
echo "    \"last_reading_timestamp\": \"$LAST_READING_TIME\"" >> "$HEALTH_REPORT"

# Zakończenie raportu
echo "  }," >> "$HEALTH_REPORT"
echo "  \"error_count\": $ERROR_COUNT," >> "$HEALTH_REPORT"

if [[ $ERROR_COUNT -ge $ALERT_THRESHOLD ]]; then
    echo "  \"overall_status\": \"CRITICAL\"" >> "$HEALTH_REPORT"
    # Wysyłanie alertu
    /workspace/software/scripts/bash/system/send_alert.sh "CRITICAL: System health check failed with $ERROR_COUNT errors"
else
    echo "  \"overall_status\": \"OK\"" >> "$HEALTH_REPORT"
fi

echo "}" >> "$HEALTH_REPORT"

# Cleanup starych raportów (>7 dni)
find /var/log/apiaryguard/ -name "health_*.json" -mtime +7 -delete

echo "Health check completed. Errors: $ERROR_COUNT"
exit $ERROR_COUNT
```

---

