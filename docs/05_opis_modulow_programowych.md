## 💻 Opis Modułów Programowych

### Moduł Arduino Nano (C++)

#### Architektura Firmware

Firmware Arduino Nano został napisany w C++ z wykorzystaniem frameworka Arduino, zapewniając deterministyczne działanie w czasie rzeczywistym. Kod jest modularny, z wyraźnym rozdziałem odpowiedzialności między warstwę sprzętową (HAL), logikę biznesową i komunikację.

```cpp
// Przykład: main.cpp - Główna pętla Arduino
#include <Wire.h>
#include "config/pin_definitions.h"
#include "sensors/hx711_driver.h"
#include "sensors/microphone_adc.h"
#include "actuators/heater_control.h"
#include "communication/i2c_slave.h"

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SLAVE_ADDRESS);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
    
    // Inicjalizacja sensorów
    HX711_init();
    MICROPHONE_init(ADC_SAMPLE_RATE);
    DHT22_begin();
    
    // Kalibracja
    if (EEPROM_read(CALIBRATION_FLAG) == 0) {
        perform_auto_calibration();
    }
    
    // Konfiguracja watchdog
    watchdog_enable(WDTO_2S);
}

void loop() {
    watchdog_reset();
    
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
    packet.timestamp = millis();
    
    // Wysyłka do RPi przez I2C
    i2c_send_packet(&packet);
    
    // Obsługa komend z RPi
    process_actuator_commands();
    
    delay(100); // 10Hz sampling rate
}
```

#### Kluczowe Komponenty Arduino

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

4. **I2C Protocol**:
   - Custom binary protocol
   - Checksum validation
   - Retry mechanism
   - Multi-message support

### Moduł Raspberry Pi (C# .NET Core)

#### Architektura Aplikacji

Aplikacja C# wykorzystuje .NET Core 6.0+ z architekturą Clean Architecture, zapewniając separację concernów, testowalność i łatwość utrzymania.

##### Warstwa Domain (ApiaryGuard.Core)

```csharp
// Model: Hive.cs
namespace ApiaryGuard.Core.Models
{
    public class Hive
    {
        public int Id { get; set; }
        public string Name { get; set; }
        public string Location { get; set; }
        public DateTime InstallationDate { get; set; }
        public HiveStatus Status { get; set; }
        
        // Navigation properties
        public ICollection<SensorReading> SensorReadings { get; set; }
        public ICollection<Alert> Alerts { get; set; }
        public ICollection<Treatment> Treatments { get; set; }
        
        // Business logic
        public bool IsSwarmingRisk()
        {
            var recentWeights = SensorReadings
                .Where(r => r.SensorType == SensorType.Weight)
                .OrderByDescending(r => r.Timestamp)
                .Take(48) // Ostatnie 48 godzin
                .ToList();
            
            if (recentWeights.Count < 2) return false;
            
            var weightDrop = recentWeights.First().Value - recentWeights.Last().Value;
            return weightDrop > 2000; // 2kg spadek = ryzyko rojenia
        }
        
        public double CalculateHealthScore()
        {
            // Algorytm oceny zdrowia rodziny
            var weightScore = GetWeightScore();
            var tempScore = GetTemperatureScore();
            var audioScore = GetAudioActivityScore();
            var humidityScore = GetHumidityScore();
            
            return (weightScore * 0.3 + tempScore * 0.25 + 
                    audioScore * 0.25 + humidityScore * 0.2);
        }
    }
}
```

##### Warstwa Services

```csharp
// Service: SensorService.cs
namespace ApiaryGuard.Core.Services
{
    public class SensorService : ISensorService
    {
        private readonly IArduinoCommunication _arduinoComm;
        private readonly IRepository<SensorReading> _readingRepo;
        private readonly ILogger<SensorService> _logger;
        
        public SensorService(
            IArduinoCommunication arduinoComm,
            IRepository<SensorReading> readingRepo,
            ILogger<SensorService> logger)
        {
            _arduinoComm = arduinoComm;
            _readingRepo = readingRepo;
            _logger = logger;
        }
        
        public async Task<SensorReading> AcquireLatestReadings(int hiveId)
        {
            try
            {
                // Pobranie danych z Arduino
                var rawPacket = await _arduinoComm.ReadSensorPacket();
                
                // Transformacja i walidacja
                var reading = new SensorReading
                {
                    HiveId = hiveId,
                    Timestamp = DateTime.UtcNow,
                    Weight = rawPacket.Weight / 1000.0, // Convert to kg
                    Temperature = rawPacket.Temperature,
                    Humidity = rawPacket.Humidity,
                    AudioLevel = rawPacket.AudioRms,
                    PiezoActivity = rawPacket.PiezoCount
                };
                
                // Walidacja zakresów
                if (!ValidateReading(reading))
                {
                    _logger.LogWarning($"Invalid reading from hive {hiveId}");
                    throw new InvalidSensorDataException("Reading out of expected range");
                }
                
                // Persist do bazy
                await _readingRepo.AddAsync(reading);
                
                // Trigger eventów
                await CheckThresholdsAndTriggerAlerts(reading);
                
                return reading;
            }
            catch (CommunicationException ex)
            {
                _logger.LogError(ex, $"Failed to acquire readings from hive {hiveId}");
                throw;
            }
        }
        
        private async Task CheckThresholdsAndTriggerAlerts(SensorReading reading)
        {
            // Reguły biznesowe dla alertów
            if (reading.Weight < 5.0) // Krytycznie niska waga
            {
                await TriggerAlert(AlertType.CriticalLowWeight, reading);
            }
            
            if (reading.Temperature > 36.0) // Przegrzanie
            {
                await TriggerAlert(AlertType.Overheating, reading);
                await ActivateCoolingFan(reading.HiveId);
            }
            
            if (reading.Humidity > 75.0) // Zbyt wysoka wilgotność
            {
                await TriggerAlert(AlertType.HighHumidity, reading);
                await ActivateVentilation(reading.HiveId);
            }
        }
    }
}
```

##### Background Workers

```csharp
// Worker: BeeSoundAnalyzerWorker.cs
namespace ApiaryGuard.Worker.Workers
{
    public class BeeSoundAnalyzerWorker : BackgroundService
    {
        private readonly ILogger<BeeSoundAnalyzerWorker> _logger;
        private readonly IServiceScopeFactory _scopeFactory;
        private readonly IAudioProcessor _audioProcessor;
        
        public BeeSoundAnalyzerWorker(
            ILogger<BeeSoundAnalyzerWorker> logger,
            IServiceScopeFactory scopeFactory,
            IAudioProcessor audioProcessor)
        {
            _logger = logger;
            _scopeFactory = scopeFactory;
            _audioProcessor = audioProcessor;
        }
        
        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            _logger.LogInformation("Bee Sound Analyzer Worker starting...");
            
            while (!stoppingToken.IsCancellationRequested)
            {
                using var scope = _scopeFactory.CreateScope();
                var context = scope.ServiceProvider.GetRequiredService<AppDbContext>();
                
                // Pobranie ostatnich nagrań audio
                var audioSamples = await context.AudioRecordings
                    .Where(a => !a.IsAnalyzed && a.CreatedAt > DateTime.UtcNow.AddHours(-1))
                    .ToListAsync(stoppingToken);
                
                foreach (var sample in audioSamples)
                {
                    try
                    {
                        // Analiza FFT i ekstrakcja cech
                        var features = await _audioProcessor.ExtractFeatures(sample.FilePath);
                        
                        // Klasyfikacja stanu rodziny
                        var classification = await ClassifyBeeState(features);
                        
                        // Predykcja rojenia
                        var swarmProbability = await PredictSwarming(features);
                        
                        // Zapis wyników
                        sample.AnalysisResult = classification;
                        sample.SwarmProbability = swarmProbability;
                        sample.IsAnalyzed = true;
                        
                        if (swarmProbability > 0.7)
                        {
                            await CreateSwarmAlert(sample.HiveId, swarmProbability);
                        }
                        
                        await context.SaveChangesAsync(stoppingToken);
                    }
                    catch (Exception ex)
                    {
                        _logger.LogError(ex, $"Error analyzing audio sample {sample.Id}");
                    }
                }
                
                // Uruchomienie co 5 minut
                await Task.Delay(TimeSpan.FromMinutes(5), stoppingToken);
            }
            
            _logger.LogInformation("Bee Sound Analyzer Worker stopped.");
        }
        
        private async Task<double> PredictSwarming(AudioFeatures features)
        {
            // Implementacja modelu ML
            // - Detekcja piping sounds (queen pipes)
            // - Analiza częstotliwości skrzydeł
            // - Pattern recognition zachowań przedrojowych
            
            var pipingDetected = features.FrequencyPeaks
                .Any(p => p.Frequency >= 200 && p.Frequency <= 300);
            
            var increasedActivity = features.ActivityLevel > _threshold;
            
            var weightDrop = await GetRecentWeightDrop(features.HiveId);
            
            // Ensemble prediction
            var probability = (
                (pipingDetected ? 0.4 : 0) +
                (increasedActivity ? 0.3 : 0) +
                (weightDrop > 1000 ? 0.3 : 0)
            );
            
            return Math.Min(probability, 1.0);
        }
    }
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

### Skrypty Bash (System Operations)

#### LTE Setup Script (lte_setup.sh)

```bash
#!/bin/bash
# lte_setup.sh - Konfiguracja połączenia LTE Aero2
# Autor: ApiaryGuard Team
# Wersja: 2.1.0

set -euo pipefail

# Konfiguracja
APN="darmowy"
PIN="" # Opcjonalny PIN karty SIM
INTERFACE="usb0"
LOG_FILE="/var/log/apiaryguard/lte_setup.log"

# Logging function
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# Sprawdzenie uprawnień root
if [[ $EUID -ne 0 ]]; then
    log "ERROR: Ten skrypt musi być uruchomiony jako root"
    exit 1
fi

log "Rozpoczynanie konfiguracji LTE Aero2..."

# Instalacja wymaganych pakietów
log "Instalacja zależności..."
apt-get update
apt-get install -y ppp wvdial usb-modescreen modemmanager

# Detekcja modemu
log "Detekcja modemu USB..."
if ! lsusb | grep -q "Huawei\|ZTE\|Option"; then
    log "WARNING: Nie wykryto modemu USB. Sprawdz połączenie."
    exit 1
fi

# Konfiguracja usb-modeswitch (jeśli potrzebne)
if [[ -f /etc/usb_modeswitch.d/* ]]; then
    log "Konfiguracja usb-modeswitch..."
    usb_modeswitch -v 0x12d1 -p 0x1506 -V 0x12d1 -P 0x1001 -M "55534243123456780000000000000011062000000100000000000000000000" || true
fi

# Tworzenie konfiguracji PPP
log "Tworzenie konfiguracji PPP..."
cat > /etc/ppp/peers/aero2 <<EOF
# Konfiguracja PPP dla Aero2
/dev/ttyUSB0
115200
noauth
defaultroute
replacedefaultroute
usepeerdns
noipdefault
persist
maxfail 0
holdoff 5
debug
logfile /var/log/ppp/aero2.log

# APN
connect "/usr/sbin/chat -v -f /etc/chatscripts/aero2"
EOF

# Chat script
cat > /etc/chatscripts/aero2 <<EOF
ABORT BUSY
ABORT 'NO CARRIER'
ABORT VOICE
ABORT 'NO DIALTONE'
'' ATZ
OK AT+CGDCONT=1,"IP","${APN}"
OK ATDT*99***1#
CONNECT ''
EOF

# Konfiguracja network interface
log "Konfiguracja interfejsu sieciowego..."
cat >> /etc/network/interfaces <<EOF

# LTE Aero2 interface
auto aero2
iface aero2 inet ppp
    provider aero2
EOF

# Firewall rules
log "Konfiguracja firewall dla LTE..."
iptables -A FORWARD -o $INTERFACE -j ACCEPT
iptables -A INPUT -i $INTERFACE -m state --state ESTABLISHED,RELATED -j ACCEPT

# Test połączenia
log "Testowanie połączenia..."
if pon aero2 stderr; then
    sleep 10
    
    if ping -c 4 -I ppp0 8.8.8.8 > /dev/null 2>&1; then
        log "SUCCESS: Połączenie LTE aktywne!"
        ip addr show ppp0 | tee -a "$LOG_FILE"
    else
        log "ERROR: Brak łączności pomimo nawiązania połączenia PPP"
        poff aero2
        exit 1
    fi
else
    log "ERROR: Nie udało się nawiązać połączenia PPP"
    exit 1
fi

# Konfiguracja auto-start
log "Dodawanie do auto-start..."
systemctl enable ppp@aero2.service 2>/dev/null || true

log "Konfiguracja LTE zakończona pomyślnie!"
echo ""
echo "Aby rozłączyć: poff aero2"
echo "Aby połączyć: pon aero2"
echo "Logi: tail -f /var/log/ppp/aero2.log"
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
check_status "Apache2" "systemctl is-active apache2" >> "$HEALTH_REPORT"
check_status "ApiaryGuard.Core" "systemctl is-active apiaryguard-core" >> "$HEALTH_REPORT"
check_status "ApiaryGuard.Worker" "systemctl is-active apiaryguard-worker" >> "$HEALTH_REPORT"
check_status "Mosquitto MQTT" "systemctl is-active mosquitto" >> "$HEALTH_REPORT"
check_status "LTE Connection" "ip link show ppp0 | grep -q UNKNOWN" >> "$HEALTH_REPORT"

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

# Test komunikacji z Arduino
if i2cdetect -y -r 1 0x48 0x48 > /dev/null 2>&1; then
    echo "    \"arduino_connection\": \"OK\"," >> "$HEALTH_REPORT"
else
    echo "    \"arduino_connection\": \"FAILED\"," >> "$HEALTH_REPORT"
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

