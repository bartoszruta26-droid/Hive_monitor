# 🆕 Nowe Parametry Sensorów Środowiskowych

## Przegląd Dodanych Metryk

W ramach rozbudowy systemu ApiaryGuard Pro dodano **ponad 380+ parametrów** we wszystkich modułach sensorowych, które umożliwiają kompleksową analizę środowiska ulu:

| Moduł Sensora | Liczba Parametrów | Opis |
|---------------|-------------------|------|
| **HX711 (Waga)** | 105+ | Statystyki, trendy, pożytek, konsumpcja, cykle, zdrowie kolonii, prognozy |
| **Audio (Mikrofon)** | 97+ | Analiza FFT, parametry czasowe, częstotliwościowe, psychoakustyczne, bioakustyczne |
| **Radar MMWave** | 27 | Detekcja ruchu, odległość, energia, prędkość, anomalie |
| **TempHumidityMetrics** | 28 | Statystyki, komfort termiczny, stabilność, trendy, alarmy |
| **AirQualityMetrics** | 24 | Gazy (CO₂, VOC, NOx), IAQ, zdrowie ula, trendy, alerty |
| **PiezoVibrationMetrics** | 22 | Amplituda, częstotliwość, zdarzenia, klasyfikacja źródła |
| **BarometricMetrics** | 18 | Ciśnienie, prognoza pogody, wpływ na pszczoły |
| **LightMetrics** | 17 | Natężenie światła, cykl dobowy, synchronizacja cyrkadiana |
| **RAZEM** | **338+** | **8 modułów sensorycznych** |

---

## 📊 1. Moduł Temperatury i Wilgotności (TempHumidityMetrics) - 28 Parametrów

### Podstawowe Parametry Statystyczne

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `temp_mean` | float | °C | Średnia temperatura |
| `temp_std` | float | °C | Odchylenie standardowe temperatury |
| `temp_min` | float | °C | Minimalna temperatura |
| `temp_max` | float | °C | Maksymalna temperatura |
| `temp_range` | float | °C | Zakres temperatury (max-min) |
| `hum_mean` | float | %RH | Średnia wilgotność |
| `hum_std` | float | %RH | Odchylenie standardowe wilgotności |
| `hum_min` | float | %RH | Minimalna wilgotność |
| `hum_max` | float | %RH | Maksymalna wilgotność |
| `hum_range` | float | %RH | Zakres wilgotności (max-min) |

### Parametry Komfortu Termicznego

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `heat_index` | float | °C | Indeks ciepła (odczuwalna temperatura) |
| `dew_point` | float | °C | Punkt rosy |
| `vapor_pressure_deficit` | float | kPa | Deficyt ciśnienia pary wodnej (VPD) |
| `comfort_index` | float | % | Indeks komfortu termicznego |

### Stabilność Środowiska

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `temp_stability` | float | % | Stabilność temperatury |
| `hum_stability` | float | % | Stabilność wilgotności |
| `env_variance` | float | - | Wariancja środowiska (temp+hum) |

### Trendy i Korelacje

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `temp_trend_1h` | float | °C/h | Trend temperatury 1h |
| `hum_trend_1h` | float | %RH/h | Trend wilgotności 1h |
| `temp_hum_correlation` | float | - | Korelacja temp-wilgotność |

### Alarmy i Ryzyka

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `overheating_risk` | float | - | Ryzyko przegrzania |
| `condensation_risk` | float | - | Ryzyko kondensacji |
| `mold_risk` | float | - | Ryzyko pleśni |
| `brood_stress_index` | float | % | Indeks stresu czerwiu |

---

## 🌬️ 2. Moduł Jakości Powietrza (AirQualityMetrics) - 24 Parametry

### Podstawowe Parametry Gazów

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `co2_equivalent` | uint16_t | ppm | Równoważne CO₂ |
| `voc_index` | uint16_t | index | Indeks VOC |
| `nox_equivalent` | uint16_t | ppb | Równoważne NOx |

### Statystyki CO₂ i VOC

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `co2_mean`, `co2_std`, `co2_min`, `co2_max` | float | Statystyki CO₂ |
| `voc_mean`, `voc_std`, `voc_min`, `voc_max` | float | Statystyki VOC |

### Indeksy Jakości Powietrza

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `iaq_index` | float | Indoor Air Quality Index (0-500) |
| `air_quality_level` | float | Poziom jakości (1-5) |
| `ventilation_need` | float | Zapotrzebowanie na wentylację (0-100%) |

### Zdrowie Ula i Alerty

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `hive_comfort_index` | float | Indeks komfortu ula |
| `stress_from_air` | float | Stres od jakości powietrza |
| `contamination_risk` | float | Ryzyko zanieczyszczenia |
| `poor_ventilation_alert` | float | Alert słabej wentylacji |
| `high_co2_warning` | float | Ostrzeżenie wysokiego CO₂ |
| `combined_risk_score` | float | Łączny wynik ryzyka |

---

## 📳 3. Moduł Wibrometrii (PiezoVibrationMetrics) - 22 Parametry

### Podstawowe Parametry Amplitudy

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `vibration_rms` | float | mV | Wartość RMS wibracji |
| `vibration_peak` | float | mV | Amplituda szczytowa |
| `vibration_peak_to_peak` | float | mV | Amplituda międzyszczytowa |
| `vibration_mean` | float | mV | Średnia wibracji |

### Parametry Częstotliwościowe i Zdarzenia

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `dominant_vib_freq` | float | Dominująca częstotliwość [Hz] |
| `vibration_activity_idx` | float | Indeks aktywności wibracyjnej |
| `impact_count` | float | Liczba uderzeń/impulsów |
| `continuous_vib_ratio` | float | Stosunek wibracji ciągłych |

### Klasyfikacja Źródła

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `bee_traffic_score` | float | Wynik ruchu pszczół |
| `predator_vib_score` | float | Wynik drapieżnika |
| `environmental_vib_score` | float | Wynik środowiska (wiatr) |
| `intrusion_probability` | float | Prawdopodobieństwo intruza |

---

## 🌡️ 4. Moduł Ciśnienia Atmosferycznego (BarometricMetrics) - 18 Parametrów

### Podstawowe Parametry Ciśnienia

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `pressure_mean` | float | hPa | Średnie ciśnienie |
| `pressure_std` | float | hPa | Odchylenie ciśnienia |
| `pressure_min`, `pressure_max` | float | hPa | Ekstrema ciśnienia |
| `pressure_trend_1h`, `pressure_trend_3h` | float | hPa/h | Trendy ciśnienia |

### Prognoza Pogody

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `weather_trend` | float | Trend pogodowy (-1 do 1) |
| `storm_probability` | float | Prawdopodobieństwo burzy |
| `foraging_conditions` | float | Warunki do wylotów |
| `swarm_weather_index` | float | Indeks pogody dla rojenia |

---

## 💡 5. Moduł Natężenia Światła (LightMetrics) - 17 Parametrów

### Podstawowe Parametry Światła

| Nazwa Parametru | Typ | Jednostka | Opis |
|-----------------|-----|-----------|------|
| `lux_current` | uint32_t | lux | Aktualne natężenie |
| `lux_mean` | float | lux | Średnie natężenie |
| `lux_std` | float | lux | Odchylenie natężenia |
| `lux_min`, `lux_max` | uint32_t | lux | Ekstrema natężenia |

### Cykl Dobowy i Wpływ na Pszczoły

| Nazwa Parametru | Typ | Opis |
|-----------------|-----|------|
| `daylight_duration` | float | Czas trwania dnia [godziny] |
| `night_duration` | float | Czas trwania nocy [godziny] |
| `dawn_dusk_detected` | float | Wykrycie świtu/zmierzchu |
| `foraging_light_index` | float | Indeks światła do wylotów |
| `circadian_sync` | float | Synchronizacja cyrkadiana |
| `hive_open_detection` | float | Detekcja otwarcia ula |

---

## 📈 Podsumowanie Wszystkich Parametrów

| Moduł | Liczba Parametrów | Główne Kategorie |
|-------|-------------------|------------------|
| **HX711Metrics (Waga)** | 105+ | Statystyki, Trendy, Pożytek, Konsumpcja, Cykle, Jakość Sygnału, Anomalie, Zdrowie Kolonii, Prognozy |
| **AudioMetrics (Akustyka)** | 97+ | Czasowe, Amplitudowe, Częstotliwościowe, Psychoakustyczne, Bioakustyczne, Pasma Mocy |
| **RadarMetrics (MMWave)** | 27 | Odległość, Energia, Prędkość, Aktywność, Trendy, Jakość, Anomalie |
| TempHumidityMetrics | 28 | Statystyki, Komfort Termiczny, Stabilność, Trendy, Alarmy |
| AirQualityMetrics | 24 | Gazy (CO₂/VOC/NOx), IAQ, Zdrowie Ula, Trendy, Alerty |
| PiezoVibrationMetrics | 22 | Amplituda, Częstotliwość, Zdarzenia, Klasyfikacja Źródła |
| BarometricMetrics | 18 | Ciśnienie, Prognoza Pogody, Wpływ na Pszczoły |
| LightMetrics | 17 | Światło, Cykl Dobowy, Synchronizacja Cyrkadiana, Alerty |
| **RAZEM** | **338+** | **8 kompletnych modułów sensorycznych** |

---

## 🔗 Integracja z API HTTP

Wszystkie nowe parametry są dostępne przez endpointy HTTP:

```
GET /apiary/environment/status
GET /apiary/temperature/metrics
GET /apiary/airquality/metrics
GET /apiary/vibration/metrics
GET /apiary/barometric/metrics
GET /apiary/light/metrics
```

### Przykładowy Response JSON

```json
{
  "timestamp": "2025-01-15T14:30:00Z",
  "temperature_humidity": {
    "temp_mean": 34.5,
    "heat_index": 36.2,
    "comfort_index": 92.5,
    "brood_stress_index": 5.0
  },
  "air_quality": {
    "co2_equivalent": 850,
    "voc_index": 45,
    "iaq_index": 65,
    "ventilation_need": 25.0
  },
  "vibration": {
    "vibration_rms": 12.5,
    "dominant_vib_freq": 250.0,
    "bee_traffic_score": 78.0,
    "intrusion_probability": 0.02
  },
  "barometric": {
    "pressure_mean": 1013.25,
    "weather_trend": 0.65,
    "foraging_conditions": 85.0
  },
  "light": {
    "lux_current": 15000,
    "foraging_light_index": 95.0,
    "circadian_sync": 0.98
  }
}
```

---

## 🎯 Zastosowanie Praktyczne

### Wczesne Wykrywanie Problemów
- **Condensation risk > 0.7**: Alert o ryzyku kondensacji
- **CO₂ > 2000 ppm**: Konieczność poprawy wentylacji
- **Vibration anomaly > 0.8**: Potencjalny intruz lub drapieżnik

### Optymalizacja Pożytku
- **Foraging conditions > 80%**: Idealne warunki na zbiór
- **Heat index 34-35°C**: Optymalna temperatura czerwiu
- **Circadian sync > 0.9**: Prawidłowy rytm dobowy

### Predykcja Zdarzeń
- **Pressure trend < -2 hPa/3h**: Nadchodząca burza
- **Swarm weather index > 70%**: Ryzyko rojenia
- **Brood stress index > 50%**: Problemy z czerwiem

---

## 🔄 Częstotliwość Aktualizacji

| Moduł | Sampling Rate | Update Rate | Buffer Size | Liczba Parametrów |
|-------|--------------|-------------|-------------|-------------------|
| HX711 (Waga) | 10 Hz | Co 5min (agregacja) | 288 punktów (24h) | 105+ |
| Audio (Mikrofon) | 8 kHz | Co 1s (FFT) | 60 punktów | 97+ |
| Radar MMWave | 1 Hz | Co 1s | 120 punktów | 27 |
| Temp/Humidity | 0.5 Hz | Co 2s | 288 punktów (24h) | 28 |
| Air Quality | 0.1 Hz | Co 10s | 144 punkty (24h) | 24 |
| Vibration (Piezo) | 100 Hz | Co 1s | 120 punktów | 22 |
| Barometric | 1 Hz | Co 1min | 72 punkty | 18 |
| Light | 1 Hz | Co 1min | 1440 punktów (24h) | 17 |

---

**ApiaryGuard Pro** - Kompleksowy monitoring środowiska ulu z ponad **338+ parametrami analitycznymi** we wszystkich modułach sensorycznych.

### Pełna Lista Endpointów API

```
GET /apiary/status              - Status całego systemu ze wszystkimi parametrami
GET /hx711/status               - Waga: 105+ parametrów (statystyki, trendy, pożytek, zdrowie)
GET /hx711/metrics              - Historia metryk wagi
GET /hx711/events               - Wykryte zdarzenia wagi
GET /hx711/forecast             - Prognoza wagi i zbiorów
GET /audio/status               - Audio: 97+ parametrów (FFT, psychoakustyka, bioakustyka)
GET /audio/metrics              - Historia metryk audio
GET /audio/events               - Zdarzenia akustyczne (rojenie, stress, aktywność)
GET /radar/status               - Radar MMWave: 27 parametrów (ruch, odległość, energia)
GET /radar/metrics              - Metryki radaru
GET /radar/anomalies            - Wykryte anomalie ruchu
GET /temperature/status         - Temp/Wilg: 28 parametrów (komfort, stabilność, alarmy)
GET /airquality/status          - Jakość powietrza: 24 parametry (CO₂, VOC, IAQ)
GET /vibration/status           - Wibracje Piezo: 22 parametry (aktywność, intruzi)
GET /barometric/status          - Ciśnienie: 18 parametrów (prognoza pogody)
GET /light/status               - Światło: 17 parametrów (cykl dobowy, circadian sync)
```
