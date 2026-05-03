# 📡 HTTP API - Raspberry Pi Pico W6100

## ApiaryGuard - System Monitoringu i Sterowania Ulami

**Wersja firmware:** 2.5.0  
**Port domyślny:** 8080  
**Protokół:** HTTP/1.1  

---

## 📋 Spis Treści

1. [Wprowadzenie](#wprowadzenie)
2. [Konfiguracja Sieci](#konfiguracja-sieci)
3. [Dostępne Endpointy](#dostępne-endpointy)
4. [Interfejs Graficzny (GUI)](#interfejs-graficzny-gui)
5. [Endpointy Sterowania](#endpointy-sterowania)
6. [Endpointy Statusu](#endpointy-statusu)
7. [Moduł Audio (MEMS Microphone)](#moduł-audio-mems-microphone)
8. [Moduł Radar mmWave (LD2410B)](#moduł-radar-mmwave-ld2410b)
9. [Moduł Wagi (HX711)](#moduł-wagi-hx711)
10. [Moduł Jakości Powietrza (SGP41)](#moduł-jakości-powietrza-sgp41)
11. [Przykłady Użycia](#przykłady-użycia)
12. [Struktura GPIO](#struktura-gpio)

---

## Wprowadzenie

ApiaryGuard to kompleksowy system monitoringu uli pszczelich oparty na mikrokontrolerze **Raspberry Pi Pico** z modułem sieciowym **W6100 Ethernet**. Urządzenie udostępnia bogate HTTP API umożliwiające:

- Odczyt danych z sensorów (temperatura, wilgotność, waga, audio, radar, jakość powietrza)
- Sterowanie efektorami (grzałka, wentylator, pompa perystaltyczna)
- Monitoring w czasie rzeczywistym
- Detekcję zdarzeń i anomalii
- Analizę trendów i prognozowanie

---

## Konfiguracja Sieci

### Domyślne ustawienia sieciowe

```cpp
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
```

### Zmiana adresu IP

Edytuj plik `apiaryguard_pico.ino` w sekcji konfiguracji:

```cpp
IPAddress ip(192, 168, 1, XXX);  // Twój adres IP
IPAddress gateway(192, 168, 1, 1);  // Twój gateway
```

---

## Dostępne Endpointy

### Podsumowanie endpointów

| Kategoria | Endpoint | Metoda | Opis |
|-----------|----------|--------|------|
| **GUI** | `/` | GET | Interfejs graficzny dashboard |
| **Status** | `/status` | GET | Podstawowe dane sensory JSON |
| **Sterowanie** | `/heater/on` | GET | Włącz grzałkę |
| **Sterowanie** | `/heater/off` | GET | Wyłącz grzałkę |
| **Sterowanie** | `/fan/on` | GET | Włącz wentylator |
| **Sterowanie** | `/fan/off` | GET | Wyłącz wentylator |
| **Sterowanie** | `/pump/on` | GET | Włącz pompę |
| **Sterowanie** | `/pump/off` | GET | Wyłącz pompę |
| **Audio** | `/audio/status` | GET | Status modułu audio |
| **Audio** | `/audio/metrics` | GET | Szczegółowe metryki audio (25+ parametrów) |
| **Audio** | `/audio/events` | GET | Zdarzenia akustyczne |
| **Audio** | `/audio/spectrum` | GET | Widmo FFT |
| **Audio** | `/audio/history` | GET | Historia pomiarów audio |
| **Radar** | `/radar/status` | GET | Status radaru mmWave |
| **Radar** | `/radar/params` | GET | Parametry ruchu i energii (27 parametrów) |
| **Radar** | `/radar/anomalies` | GET | Detekcja anomalii i pożytków |
| **Radar** | `/radar/raw` | GET | Surowe dane radaru |
| **Waga** | `/hx711/status` | GET | Status wagi HX711 |
| **Waga** | `/hx711/metrics` | GET | Metryki wagi (30+ parametrów) |
| **Waga** | `/hx711/events` | GET | Zdarzenia wagowe |
| **Waga** | `/hx711/forecast` | GET | Prognoza wagi |
| **Powietrze** | `/airquality/status` | GET | Status jakości powietrza |
| **Powietrze** | `/airquality/metrics` | GET | Metryki jakości (24+ parametry) |
| **Powietrze** | `/airquality/events` | GET | Zdarzenia jakościowe |

---

## Interfejs Graficzny (GUI)

### Dostęp do dashboardu

Otwórz w przeglądarce:
```
http://192.168.1.100:8080/
```

### Struktura Dashboardu

**Nagłówek:**
- Adres IP urządzenia
- Czas pracy (uptime w sekundach)
- Wersja firmware (2.5.0)

**Karty z parametrami:**

1. **🌡️ Środowisko**
   - Temperatura (°C)
   - Wilgotność (%)
   - CO₂ (ppm)
   - VOC (index)
   - Waga (kg)

2. **🎤 Audio**
   - RMS Amplitude (V)
   - Aktywność pszczół (%)
   - Zdrowie ula (%)

3. **⚖️ Waga**
   - Średnia waga (kg)
   - Trend 1h (kg/h)
   - Zapas pokarmu (dni)

4. **📡 Radar**
   - Wykryto ruch (TAK/NIE)
   - Liczba celów
   - Indeks zdrowia (%)

5. **💨 Powietrze**
   - CO₂ equivalent (ppm)
   - VOC index
   - IAQ Index (Indoor Air Quality)

**Sekcja API Links:**
- [Status JSON](/status)
- [Audio Metryki](/audio/metrics)
- [Radar Parametry](/radar/params)
- [Waga Metryki](/hx711/metrics)
- [Powietrze Metryki](/airquality/metrics)
- [Anomalie](/radar/anomalies)
- [Grzałka ON](/heater/on)
- [Wentylator OFF](/fan/off)

---

## Endpointy Sterowania

### Grzałka PWM (GPIO 12)

#### Włącz grzałkę
```http
GET /heater/on
```

**Odpowiedź:**
```
Heater ON
```

#### Wyłącz grzałkę
```http
GET /heater/off
```

**Odpowiedź:**
```
Heater OFF
```

---

### Wentylator PWM (GPIO 13)

#### Włącz wentylator
```http
GET /fan/on
```

**Odpowiedź:**
```
Fan ON
```

#### Wyłącz wentylator
```http
GET /fan/off
```

**Odpowiedź:**
```
Fan OFF
```

---

### Pompa Perystaltyczna PWM (GPIO 14)

#### Włącz pompę
```http
GET /pump/on
```

**Odpowiedź:**
```
Pump ON
```

#### Wyłącz pompę
```http
GET /pump/off
```

**Odpowiedź:**
```
Pump OFF
```

---

## Endpointy Statusu

### GET /status - Podstawowe dane sensory

Zwraca podstawowe informacje o wszystkich sensorach w formacie JSON.

**Request:**
```bash
curl http://192.168.1.100:8080/status
```

**Response:**
```json
{
  "timestamp": "12345678",
  "firmware_version": "2.5.0",
  "uptime_seconds": 3600,
  "ip_address": "192.168.1.100",
  "temperature": 24.56,
  "humidity": 65.23,
  "weight_grams": 45300,
  "weight_kg": 45.300,
  "audio_level": 42,
  "vibration_level": 15,
  "co2_ppm": 450,
  "voc_index": 85,
  "motion_detected": true,
  "relay_state": 0,
  "sensors_ok": true
}
```

**Pola odpowiedzi:**

| Pole | Typ | Opis |
|------|-----|------|
| `timestamp` | String | Czas pomiaru w ms |
| `firmware_version` | String | Wersja oprogramowania |
| `uptime_seconds` | Integer | Czas pracy w sekundach |
| `ip_address` | String | Adres IP urządzenia |
| `temperature` | Float | Temperatura [°C] |
| `humidity` | Float | Wilgotność [%] |
| `weight_grams` | Integer | Waga w gramach |
| `weight_kg` | Float | Waga w kilogramach |
| `audio_level` | Integer | Poziom dźwięku (ADC) |
| `vibration_level` | Integer | Poziom wibracji (ADC) |
| `co2_ppm` | Integer | Stężenie CO2 [ppm] |
| `voc_index` | Integer | Indeks lotnych związków organicznych |
| `motion_detected` | Boolean | Wykryto ruch (radar) |
| `relay_state` | Integer | Stan przekaźników |
| `sensors_ok` | Boolean | Status sensorów |

---

## Moduł Audio (MEMS Microphone)

### GET /audio/status - Status modułu audio

**Request:**
```bash
curl http://192.168.1.100:8080/audio/status
```

**Response:**
```json
{
  "timestamp": "12345678",
  "firmware_version": "2.5.0",
  "uptime_seconds": 3600,
  "microphone_connected": true,
  "buffer_size": 256,
  "samples_collected": 45210,
  "current_metrics": {
    "rms_amplitude": 0.0234,
    "peak_amplitude": 0.0891,
    "zero_crossing_rate": 145.2,
    "dominant_frequency": 287.5,
    "spectral_centroid": 412.3,
    "bee_activity_index": 78.5,
    "swarm_probability": 12.3,
    "stress_indicator": 23.1,
    "hive_health_audio": 85.7
  }
}
```

---

### GET /audio/metrics - Szczegółowe metryki audio

Zwraca ponad 25 parametrów analizy dźwięku.

**Request:**
```bash
curl http://192.168.1.100:8080/audio/metrics
```

**Response:**
```json
{
  "timestamp": "12345678",
  "time_domain": {
    "rms_amplitude": 0.0234,
    "peak_amplitude": 0.0891,
    "peak_to_peak": 0.1523,
    "zero_crossing_rate": 145.2,
    "signal_energy": 1247.5,
    "crest_factor": 3.81,
    "average_amplitude": 0.0187
  },
  "statistics": {
    "mean_value": 0.0012,
    "std_deviation": 0.0231,
    "skewness": -0.15,
    "kurtosis": 2.87,
    "coefficient_of_variation": 19.25,
    "dynamic_range": 42.3
  },
  "frequency_domain": {
    "dominant_frequency": 287.5,
    "spectral_centroid": 412.3,
    "spectral_bandwidth": 234.7,
    "spectral_flatness": 0.23,
    "spectral_rolloff": 678.9,
    "spectral_entropy": 4.56,
    "harmonic_to_noise_ratio": 12.34,
    "autocorrelation_peak": 0.78
  },
  "band_power": {
    "power_low_freq": 23.5,
    "power_bee_band": 567.8,
    "power_swarm_band": 234.5,
    "power_mid_freq": 123.4,
    "power_high_freq": 45.6
  },
  "classification": {
    "bee_activity_index": 78.5,
    "swarm_probability": 12.3,
    "stress_indicator": 23.1,
    "hive_health_audio": 85.7
  },
  "formants_quality": {
    "formant_f1": 245.3,
    "formant_f2": 567.8,
    "formant_f3": 1234.5,
    "brightness": 0.34,
    "roughness": 0.12,
    "sharpness": 0.45,
    "tonality": 0.67,
    "prominence_ratio": 2.34
  },
  "temporal_features": {
    "attack_time": 12.5,
    "decay_time": 45.3,
    "temporal_centroid": 78.9,
    "silence_ratio": 0.05,
    "modulation_index": 0.23
  },
  "psychoacoustics": {
    "loudness": 34.5,
    "roughness_fast": 0.15,
    "spectral_decrease": -0.23,
    "irregularity": 0.34
  }
}
```

**Opis kategorii parametrów:**

#### Time Domain (Domena Czasowa)
- `rms_amplitude` - Efektywna wartość amplitudy [V]
- `peak_amplitude` - Wartość szczytowa [V]
- `peak_to_peak` - Wartość międzyszczytowa [V]
- `zero_crossing_rate` - Częstotliwość przejść przez zero [Hz]
- `signal_energy` - Energia sygnału
- `crest_factor` - Współczynnik szczytu
- `average_amplitude` - Średnia amplituda [V]

#### Statistics (Statystyki)
- `mean_value` - Wartość średnia
- `std_deviation` - Odchylenie standardowe
- `skewness` - Skośność rozkładu
- `kurtosis` - Kurtoza
- `coefficient_of_variation` - Współczynnik zmienności [%]
- `dynamic_range` - Zakres dynamiczny [dB]

#### Frequency Domain (Domena Częstotliwości)
- `dominant_frequency` - Dominująca częstotliwość [Hz]
- `spectral_centroid` - Centrum widmowe [Hz]
- `spectral_bandwidth` - Szerokość pasma [Hz]
- `spectral_flatness` - Płaszczyzna widmowa
- `spectral_rolloff` - Częstotliwość odcięcia [Hz]
- `spectral_entropy` - Entropia widmowa
- `harmonic_to_noise_ratio` - Stosunek harmonicznych do szumu [dB]
- `autocorrelation_peak` - Szczyt autokorelacji

#### Band Power (Moc Pasma)
- `power_low_freq` - Moc niskich częstotliwości
- `power_bee_band` - Moc pasma pszczelego [Hz²]
- `power_swarm_band` - Moc pasma rojowego [Hz²]
- `power_mid_freq` - Moc średnich częstotliwości
- `power_high_freq` - Moc wysokich częstotliwości

#### Classification (Klasyfikacja)
- `bee_activity_index` - Indeks aktywności pszczół [0-100%]
- `swarm_probability` - Prawdopodobieństwo rojenia [%]
- `stress_indicator` - Wskaźnik stresu [%]
- `hive_health_audio` - Zdrowie ula (audio) [%]

---

### GET /audio/events - Zdarzenia akustyczne

**Request:**
```bash
curl http://192.168.1.100:8080/audio/events
```

**Response:**
```json
{
  "timestamp": "12345678",
  "status": "POZYTYWNY",
  "event_type": "NORMAL_ACTIVITY",
  "confidence": 0.92,
  "impact": "POZYTYWNY",
  "description": "Normalna aktywność pszczół",
  "details": {
    "bee_activity_index": 78.5,
    "swarm_probability": 12.3,
    "stress_indicator": 23.1,
    "dominant_frequency": 287.5,
    "power_bee_band": 567.8
  },
  "recent_events": [
    {
      "type": "NORMAL_ACTIVITY",
      "timestamp": "12045678",
      "severity": "LOW",
      "impact": "POZYTYWNY",
      "description": "Normalna aktywność"
    },
    {
      "type": "INCREASED_ACTIVITY",
      "timestamp": "12225678",
      "severity": "LOW",
      "impact": "POZYTYWNY",
      "description": "Zwiększona aktywność"
    }
  ]
}
```

**Typy zdarzeń:**
- `NORMAL_ACTIVITY` - Normalna aktywność
- `INCREASED_ACTIVITY` - Zwiększona aktywność (powrót z pożytku)
- `SWARM_PREPARATION` - Przygotowanie do rojenia
- `STRESS_DETECTED` - Wykryto stres
- `QUEEN_PIPING` - Pipczenie matki

---

### GET /audio/spectrum - Widmo FFT

**Request:**
```bash
curl http://192.168.1.100:8080/audio/spectrum
```

**Response:**
```json
{
  "timestamp": "12345678",
  "fft_size": 256,
  "sample_rate": 8000,
  "frequency_resolution": 31.25,
  "spectrum": [
    {"bin": 0, "frequency": 0.0, "magnitude": 0.001},
    {"bin": 1, "frequency": 31.25, "magnitude": 0.023},
    ...
    {"bin": 127, "frequency": 3968.75, "magnitude": 0.012}
  ],
  "peaks": [
    {"frequency": 287.5, "magnitude": 0.089, "bin": 9},
    {"frequency": 456.25, "magnitude": 0.067, "bin": 14},
    {"frequency": 623.75, "magnitude": 0.054, "bin": 20}
  ]
}
```

**Parametry:**
- `fft_size` - Rozmiar FFT (256 próbek)
- `sample_rate` - Częstotliwość próbkowania (8000 Hz)
- `frequency_resolution` - Rozdzielczość częstotliwościowa (31.25 Hz)
- `spectrum` - Tablica 128 binów widmowych
- `peaks` - Wykryte szczyty widmowe

---

### GET /audio/history - Historia pomiarów

**Request:**
```bash
curl http://192.168.1.100:8080/audio/history
```

**Response:**
```json
{
  "timestamp": "12345678",
  "interval_seconds": 1,
  "data_points": 60,
  "history": [
    {
      "timestamp": "12345618",
      "rms_amplitude": 0.0234,
      "dominant_frequency": 287.5,
      "bee_activity_index": 78.5,
      "swarm_probability": 12.3,
      "event_type": "NORMAL_ACTIVITY"
    },
    ...
  ],
  "trends": {
    "activity_trend": "STABLE",
    "swarm_risk_trend": "STABLE",
    "health_trend": "GOOD"
  }
}
```

**Możliwe trendy:**
- `INCREASING` - Rosnący
- `DECREASING` - Malejący
- `STABLE` - Stabilny

---

## Moduł Radar mmWave (LD2410B)

### GET /radar/status - Status radaru

**Request:**
```bash
curl http://192.168.1.100:8080/radar/status
```

**Response:**
```json
{
  "timestamp": "12345678",
  "firmware_version": "2.5.0",
  "uptime_seconds": 3600,
  "radar_connected": true,
  "buffer_size": 120,
  "samples_collected": 90420
}
```

---

### GET /radar/params - Parametry ruchu i energii

Zwraca 27 parametrów analizy ruchu.

**Request:**
```bash
curl http://192.168.1.100:8080/radar/params
```

**Response:**
```json
{
  "timestamp": "12345678",
  "distance_stats": {
    "mean_cm": 45.3,
    "median_cm": 44.8,
    "std_dev_cm": 12.7,
    "min_cm": 15.2,
    "max_cm": 98.4,
    "percentile_10_cm": 28.5,
    "percentile_90_cm": 67.2
  },
  "energy_analysis": {
    "total_energy": 1247.5,
    "energy_density": 10.4,
    "coefficient_of_variation": 0.34
  },
  "motion_dynamics": {
    "estimated_velocity_cm_s": 23.5,
    "acceleration_cm_s2": 4.2,
    "swarm_liveness_index": 7.8
  },
  "temporal_trends": {
    "trend_slope": 1.23,
    "linear_regression_r2": 0.87,
    "predicted_activity_5min": 8.2
  },
  "anomaly_detection": {
    "zscore_max": 1.8,
    "outliers_count": 2,
    "anomaly_score": 0.15
  },
  "quality_indices": {
    "hive_health_index": 8.5,
    "forage_status": "POZYTYWNY",
    "activity_level": "HIGH"
  }
}
```

**Opis kategorii:**

#### Distance Stats (Statystyki Dystansu)
- `mean_cm` - Średni dystans [cm]
- `median_cm` - Mediana dystansu [cm]
- `std_dev_cm` - Odchylenie standardowe [cm]
- `min_cm` - Minimalny dystans [cm]
- `max_cm` - Maksymalny dystans [cm]
- `percentile_10_cm` - 10 percentyl [cm]
- `percentile_90_cm` - 90 percentyl [cm]

#### Energy Analysis (Analiza Energii)
- `total_energy` - Całkowita energia sygnału
- `energy_density` - Gęstość energii
- `coefficient_of_variation` - Współczynnik zmienności

#### Motion Dynamics (Dynamika Ruchu)
- `estimated_velocity_cm_s` - Szacowana prędkość [cm/s]
- `acceleration_cm_s2` - Przyspieszenie [cm/s²]
- `swarm_liveness_index` - Indeks żywości roju [0-10]

#### Temporal Trends (Trendy Czasowe)
- `trend_slope` - Nachylenie trendu
- `linear_regression_r2` - Współczynnik determinacji R²
- `predicted_activity_5min` - Przewidywana aktywność za 5 min

#### Anomaly Detection (Detekcja Anomalii)
- `zscore_max` - Maksymalna wartość Z-score
- `outliers_count` - Liczba wartości odstających
- `anomaly_score` - Wynik anomalii [0-1]

#### Quality Indices (Indeksy Jakości)
- `hive_health_index` - Indeks zdrowia ula [0-10]
- `forage_status` - Status pożytku (POZYTYWNY/NEGATYWNY)
- `activity_level` - Poziom aktywności (LOW/MEDIUM/HIGH)

---

### GET /radar/anomalies - Detekcja anomalii i pożytków

**Request:**
```bash
curl http://192.168.1.100:8080/radar/anomalies
```

**Response:**
```json
{
  "timestamp": "12345678",
  "status": "POZYTYWNY",
  "event_type": "INTENSYWNY_OBLOT",
  "confidence": 0.92,
  "hive_health_index": 8.5,
  "anomaly_score": 0.1,
  "details": {
    "trend_slope": 1.2,
    "energy_change_percent": 15.4,
    "target_count_avg": 45,
    "zscore_current": 1.8
  },
  "recent_events": [
    {
      "type": "NAGLY_WZROST_RUCHU",
      "timestamp": "12045678",
      "severity": "LOW",
      "description": "Wykryto nagły wzrost aktywności - powrót z pożytku"
    }
  ]
}
```

**Typy zdarzeń radaru:**
- `INTENSYWNY_OBLOT` - Intensywny oblot
- `NAGLY_WZROST_RUCHU` - Nagły wzrost ruchu
- `SPADEK_AKTYWNOSCI` - Spadek aktywności
- `ANOMALIA_WYKRYTA` - Wykryto anomalię
- `NORMALNY_RUCH` - Normalny ruch

---

### GET /radar/raw - Surowe dane radaru

**Request:**
```bash
curl http://192.168.1.100:8080/radar/raw?seconds=5
```

**Response:**
```json
{
  "timestamp": "12345678",
  "samples": [
    {
      "timestamp": "12345673",
      "distance_cm": 45.2,
      "energy": 127,
      "speed_cm_s": 22.5,
      "targets_count": 3
    },
    ...
  ]
}
```

**Parametr zapytania:**
- `seconds` - Liczba sekund historii (domyślnie 5)

---

## Moduł Wagi (HX711)

### GET /hx711/status - Status wagi

**Request:**
```bash
curl http://192.168.1.100:8080/hx711/status
```

**Response:**
```json
{
  "timestamp": "12345678",
  "firmware_version": "2.5.0",
  "uptime_seconds": 3600,
  "hx711_connected": true,
  "buffer_size": 120,
  "samples_collected": 3600
}
```

---

### GET /hx711/metrics - Metryki wagi

Zwraca ponad 30 parametrów analizy wagi.

**Request:**
```bash
curl http://192.168.1.100:8080/hx711/metrics
```

**Response:**
```json
{
  "timestamp": "12345678",
  "weight_raw": {
    "current_grams": 45300,
    "current_kg": 45.300,
    "offset": 12500,
    "scale_factor": 420.5
  },
  "weight_stats": {
    "mean_1h_kg": 45.350,
    "median_1h_kg": 45.330,
    "std_dev_kg": 0.023,
    "min_24h_kg": 44.800,
    "max_24h_kg": 46.100,
    "range_kg": 1.3,
    "cv_percent": 2.1
  },
  "trend_analysis": {
    "slope_1h_g_h": 50,
    "slope_4h_g_h": 30,
    "slope_24h_g_h": -10,
    "linear_regression_r2": 0.89,
    "predicted_weight_1h_kg": 45.350
  },
  "nectar_flow": {
    "inflow_rate_g_h": 80,
    "outflow_rate_g_h": 20,
    "net_flow_g_h": 60,
    "forage_efficiency": 0.78
  },
  "alerts": {
    "low_food_alert": false,
    "rapid_loss_alert": false,
    "swarm_departure": false,
    "supercedure_event": false
  },
  "derived_metrics": {
    "food_reserve_days": 12,
    "colony_strength_index": 8.5,
    "brood_estimate_g": 1500,
    "honey_stores_kg": 13.59
  }
}
```

**Opus kategorii:**

#### Weight Raw (Surowe Dane Wagi)
- `current_grams` - Aktualna waga [g]
- `current_kg` - Aktualna waga [kg]
- `offset` - Offset kalibracyjny
- `scale_factor` - Faktor skalowania

#### Weight Stats (Statystyki Wagi)
- `mean_1h_kg` - Średnia z 1 godziny [kg]
- `median_1h_kg` - Mediana z 1 godziny [kg]
- `std_dev_kg` - Odchylenie standardowe [kg]
- `min_24h_kg` - Minimum z 24 godzin [kg]
- `max_24h_kg` - Maksimum z 24 godzin [kg]
- `range_kg` - Zakres zmian [kg]
- `cv_percent` - Współczynnik zmienności [%]

#### Trend Analysis (Analiza Trendów)
- `slope_1h_g_h` - Nachylenie trendu 1h [g/h]
- `slope_4h_g_h` - Nachylenie trendu 4h [g/h]
- `slope_24h_g_h` - Nachylenie trendu 24h [g/h]
- `linear_regression_r2` - Współczynnik R²
- `predicted_weight_1h_kg` - Przewidywana waga za 1h [kg]

#### Nectar Flow (Przepływ Nektaru)
- `inflow_rate_g_h` - Napływ nektaru [g/h]
- `outflow_rate_g_h` - Odpływ nektaru [g/h]
- `net_flow_g_h` - Przepływ netto [g/h]
- `forage_efficiency` - Efektywność pożytku [0-1]

#### Alerts (Alerty)
- `low_food_alert` - Alert niskiego zapasu pokarmu
- `rapid_loss_alert` - Alert szybkiej utraty wagi
- `swarm_departure` - Wyjście roju
- `supercedure_event` - Wymiana matki

#### Derived Metrics (Metryki Pochodne)
- `food_reserve_days` - Zapas pokarmu w dniach
- `colony_strength_index` - Indeks siły rodziny [0-10]
- `brood_estimate_g` - Szacunkowa masa czerwiu [g]
- `honey_stores_kg` - Zapasy miodu [kg]

---

### GET /hx711/events - Zdarzenia wagowe

**Request:**
```bash
curl http://192.168.1.100:8080/hx711/events
```

**Response:**
```json
{
  "timestamp": "12345678",
  "current_event": "NORMAL_WEIGHT_GAIN",
  "confidence": 0.88,
  "weight_change_g": 50,
  "event_classification": "POZYTKI",
  "recent_events": [
    {
      "type": "WEIGHT_GAIN",
      "timestamp": "11745678",
      "change_g": 120,
      "description": "Powiekszenie wagi - powrot z pozytku"
    },
    {
      "type": "STABLE",
      "timestamp": "10545678",
      "change_g": 5,
      "description": "Stabilna waga"
    }
  ]
}
```

**Typy zdarzeń wagowych:**
- `NORMAL_WEIGHT_GAIN` - Normalny przyrost wagi
- `WEIGHT_GAIN` - Przyrost wagi (pożytek)
- `WEIGHT_LOSS` - Utrata wagi
- `STABLE` - Stabilna waga
- `RAPID_LOSS` - Szybka utrata wagi
- `SWARM_DEPARTURE` - Wyjście roju

---

### GET /hx711/forecast - Prognoza wagi

**Request:**
```bash
curl http://192.168.1.100:8080/hx711/forecast
```

**Response:**
```json
{
  "timestamp": "12345678",
  "current_weight_kg": 45.300,
  "forecast_1h_kg": 45.350,
  "forecast_6h_kg": 45.500,
  "forecast_24h_kg": 45.800,
  "forecast_confidence": 0.82,
  "trend": "INCREASING",
  "seasonal_adjustment": 0.15,
  "weather_impact": "POZYTYWNY"
}
```

**Możliwe trendy:**
- `INCREASING` - Rosnący
- `DECREASING` - Malejący
- `STABLE` - Stabilny

**Weather Impact:**
- `POZYTYWNY` - Dobra pogoda dla pożytku
- `NEGATYWNY` - Niekorzystna pogoda
- `NEUTRALNY` - Neutralny wpływ

---

## Moduł Jakości Powietrza (SGP41)

### GET /airquality/status - Status jakości powietrza

**Request:**
```bash
curl http://192.168.1.100:8080/airquality/status
```

**Response:**
```json
{
  "timestamp": "12345678",
  "firmware_version": "2.5.0",
  "uptime_seconds": 3600,
  "sgp41_connected": true,
  "co2_current_ppm": 450,
  "voc_current_index": 85,
  "iaq_index": 125.5,
  "air_quality_level": 2,
  "alerts_active": false
}
```

**Poziomy jakości powietrza:**
- `1` - Dobra
- `2` - Średnia
- `3` - Zła
- `4` - Bardzo zła

---

### GET /airquality/metrics - Metryki jakości powietrza

Zwraca ponad 24 parametry jakości powietrza.

**Request:**
```bash
curl http://192.168.1.100:8080/airquality/metrics
```

**Response:**
```json
{
  "timestamp": "12345678",
  "basic_params": {
    "co2_current_ppm": 450,
    "voc_current_index": 85,
    "nox_equivalent_ppb": 120
  },
  "co2_stats": {
    "mean_ppm": 465.23,
    "std_dev_ppm": 25.45,
    "min_ppm": 400,
    "max_ppm": 650,
    "range_ppm": 250,
    "cv_percent": 5.47
  },
  "voc_stats": {
    "mean_index": 82.34,
    "std_dev_index": 12.56,
    "min_index": 50,
    "max_index": 150,
    "range_index": 100,
    "cv_percent": 15.25
  },
  "trends": {
    "co2_slope_1h_ppm_h": 2.5,
    "trend_direction": 1,
    "trend_strength": 0.65
  },
  "indices": {
    "iaq_index": 125.5,
    "air_quality_level": 2,
    "ventilation_need_percent": 35.2,
    "stress_from_air_percent": 15.8,
    "hive_comfort_index": 82.5
  },
  "alerts": {
    "poor_ventilation": false,
    "contamination_risk": false,
    "mold_risk": false,
    "high_co2_alert": false,
    "combined_risk_score": 12.5
  },
  "temporal": {
    "variability_index": 25.3,
    "stability_score": 78.5,
    "change_rate": 1.2,
    "volatility_index": 18.7
  },
  "correlations": {
    "comfort_zone_percent": 85.2
  },
  "thresholds": {
    "co2_warning_level": 0,
    "voc_alert_level": 0
  }
}
```

**Opis kategorii:**

#### Basic Params (Podstawowe Parametry)
- `co2_current_ppm` - Aktualne CO2 [ppm]
- `voc_current_index` - Aktualny VOC Index
- `nox_equivalent_ppb` - Ekwivalent NOx [ppb]

#### CO2 Stats (Statystyki CO2)
- `mean_ppm` - Średnie CO2 [ppm]
- `std_dev_ppm` - Odchylenie standardowe CO2 [ppm]
- `min_ppm` - Minimalne CO2 [ppm]
- `max_ppm` - Maksymalne CO2 [ppm]
- `range_ppm` - Zakres CO2 [ppm]
- `cv_percent` - Współczynnik zmienności [%]

#### VOC Stats (Statystyki VOC)
- `mean_index` - Średni VOC Index
- `std_dev_index` - Odchylenie standardowe VOC
- `min_index` - Minimalny VOC Index
- `max_index` - Maksymalny VOC Index
- `range_index` - Zakres VOC
- `cv_percent` - Współczynnik zmienności [%]

#### Trends (Trendy)
- `co2_slope_1h_ppm_h` - Trend CO2 [ppm/h]
- `trend_direction` - Kierunek (-1 spadek, 0 stabilny, 1 wzrost)
- `trend_strength` - Siła trendu [0-1]

#### Indices (Indeksy)
- `iaq_index` - Indoor Air Quality Index [0-500]
- `air_quality_level` - Poziom jakości (1-4)
- `ventilation_need_percent` - Potrzeba wentylacji [%]
- `stress_from_air_percent` - Stres od powietrza [%]
- `hive_comfort_index` - Indeks komfortu ula [%]

#### Alerts (Alerty)
- `poor_ventilation` - Alert słabej wentylacji
- `contamination_risk` - Ryzyko zanieczyszczenia
- `mold_risk` - Ryzyko pleśni
- `high_co2_alert` - Alert wysokiego CO2
- `combined_risk_score` - Łączny wynik ryzyka [%]

#### Temporal (Parametry Temporalne)
- `variability_index` - Indeks zmienności [%]
- `stability_score` - Wynik stabilności [%]
- `change_rate` - Szybkość zmian [index/h]
- `volatility_index` - Indeks niestabilności [%]

#### Thresholds (Progi)
- `co2_warning_level` - Poziom ostrzeżenia CO2 (0-normalny, 1-ostrzeżenie, 2-krytyczny)
- `voc_alert_level` - Poziom alertu VOC (0-normalny, 1-ostrzeżenie, 2-krytyczny)

---

### GET /airquality/events - Zdarzenia jakościowe

**Request:**
```bash
curl http://192.168.1.100:8080/airquality/events
```

**Response:**
```json
{
  "timestamp": "12345678",
  "current_status": "NORMAL",
  "event_type": "GOOD_AIR_QUALITY",
  "confidence": 0.95,
  "details": {
    "co2_current": 450,
    "voc_current": 85,
    "iaq_index": 125.5
  },
  "recent_events": [
    {
      "type": "GOOD_VENTILATION",
      "timestamp": "11745678",
      "severity": "LOW",
      "description": "Dobra wentylacja ula"
    },
    {
      "type": "NORMAL_CO2",
      "timestamp": "11145678",
      "severity": "LOW",
      "description": "Prawidłowy poziom CO2"
    }
  ]
}
```

**Typy zdarzeń jakościowych:**
- `GOOD_AIR_QUALITY` - Dobra jakość powietrza
- `GOOD_VENTILATION` - Dobra wentylacja
- `NORMAL_CO2` - Prawidłowy CO2
- `POOR_VENTILATION` - Słaba wentylacja
- `HIGH_CO2_DETECTED` - Wykryto wysokie CO2
- `CONTAMINATION_ALERT` - Alert zanieczyszczenia
- `MOLD_RISK_DETECTED` - Wykryto ryzyko pleśni

---

## Przykłady Użycia

### 1. Pobranie statusu urządzenia

```bash
curl http://192.168.1.100:8080/status
```

### 2. Włączenie grzałki

```bash
curl http://192.168.1.100:8080/heater/on
```

### 3. Sprawdzenie metryk audio

```bash
curl http://192.168.1.100:8080/audio/metrics | jq .
```

### 4. Monitorowanie anomalii radaru

```bash
watch -n 5 'curl -s http://192.168.1.100:8080/radar/anomalies | jq .event_type'
```

### 5. Pobranie prognozy wagi

```bash
curl http://192.168.1.100:8080/hx711/forecast | jq .forecast_24h_kg
```

### 6. Sprawdzenie jakości powietrza

```bash
curl http://192.168.1.100:8080/airquality/metrics | jq '.indices.iaq_index'
```

### 7. Skrypt bash - monitoring ciągły

```bash
#!/bin/bash
API_URL="http://192.168.1.100:8080"

while true; do
  echo "=== $(date) ==="
  curl -s $API_URL/status | jq '{temp: .temperature, hum: .humidity, weight: .weight_kg}'
  sleep 10
done
```

### 8. Python - pobieranie danych

```python
import requests
import json

API_URL = "http://192.168.1.100:8080"

# Pobierz status
response = requests.get(f"{API_URL}/status")
data = response.json()

print(f"Temperatura: {data['temperature']}°C")
print(f"Wilgotność: {data['humidity']}%")
print(f"Waga: {data['weight_kg']} kg")
print(f"CO2: {data['co2_ppm']} ppm")

# Sprawdź metryki audio
audio_response = requests.get(f"{API_URL}/audio/metrics")
audio_data = audio_response.json()
print(f"Aktywność pszczół: {audio_data['classification']['bee_activity_index']}%")
```

---

## Struktura GPIO

### Pinout Raspberry Pi Pico W6100

| Funkcja | GPIO | Opis |
|---------|------|------|
| **W6100 Ethernet (SPI1)** | | |
| CS | GP5 | Chip Select |
| MOSI | GP7 | SPI1 TX |
| MISO | GP8 | SPI1 RX |
| SCK | GP6 | SPI1 Clock |
| RST | GP4 | Reset |
| INT | GP3 | Przerwania (opcjonalny) |
| **SGP41 (I2C)** | | |
| SDA | GP0 | Data |
| SCL | GP1 | Clock |
| **HX711 (Waga)** | | |
| DT | GP9 | Data |
| SCK | GP10 | Clock |
| **DHT22** | | |
| DATA | GP11 | Temperatura/Wilgotność |
| **PWM Efektory** | | |
| HEATER | GP12 | Grzałka 10W |
| FAN | GP13 | Wentylator 12V |
| PUMP | GP14 | Pompa perystaltyczna |
| **Przekaźniki** | | |
| RELAY_1 | GP15 | Zawór 1 |
| RELAY_2 | GP16 | Zawór 2 |
| RELAY_3 | GP17 | Zawór 3 |
| RELAY_4 | GP18 | Zawór 4 |
| RELAY_5 | GP19 | Dodatkowy |
| RELAY_6 | GP20 | Dodatkowy |
| RELAY_7 | GP21 | Dodatkowy |
| RELAY_8 | GP22 | Dodatkowy |
| **Audio** | | |
| MIC | GP26 | ADC0 - MEMS Microphone |
| PIEZO | GP27 | ADC1 - Vibration Sensor |
| **LD2410B Radar (UART1)** | | |
| RX | GP28 | UART1 RX (do radaru TX) |
| TX | GP29 | UART1 TX (do radaru RX) |

---

## Diagnostyka i Rozwiązywanie Problemów

### Sprawdzanie połączenia sieciowego

```bash
ping 192.168.1.100
```

### Testowanie endpointów

```bash
# Sprawdź czy serwer odpowiada
curl -I http://192.168.1.100:8080/status

# Pełny output z debugowaniem
curl -v http://192.168.1.100:8080/status
```

### Monitorowanie logów

Podłącz się przez USB-C i otwórz monitor portu szeregowego (115200 baud):

```bash
screen /dev/ttyACM0 115200
# lub
minicom -D /dev/ttyACM0 -b 115200
```

### Resetowanie urządzenia

Odłącz i podłącz ponownie zasilanie USB-C lub naciśnij przycisk RESET na płytce Pico.

---

## Bezpieczeństwo

### Zalecenia

1. **Zmień domyślny adres IP** na odpowiedni dla Twojej sieci
2. **Używaj firewalla** aby ograniczyć dostęp do portu 8080
3. **Rozważ dodanie autentykacji** w produkcji
4. **Aktualizuj firmware** regularnie

### Ograniczenia obecnej wersji

- Brak autentykacji HTTP
- Brak szyfrowania HTTPS
- Brak rate limitingu
- Otwarty dostęp do wszystkich endpointów

---

## Wsparcie i Dokumentacja

- **Repozytorium:** `/workspace/src/pico_w6100/`
- **Główny plik:** `apiaryguard_pico.ino`
- **Dokumentacja dodatkowa:** `/workspace/docs/09_api_i_integracje.md`

---

**Wersja dokumentacji:** 1.0  
**Data ostatniej aktualizacji:** 2024  
**Autor:** ApiaryGuard Team
