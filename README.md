# 🐝 ApiaryGuard Pro

<div align="center">

**Enterprise Multi-Apiary Monitoring & Management System**

[![Version](https://img.shields.io/badge/version-2.5.0-blue.svg?style=for-the-badge)](../../releases)
[![License](https://img.shields.io/badge/license-Apache%202.0-green.svg?style=for-the-badge)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%202%20%7C%20Pico-orange.svg?style=for-the-badge)](docs/03_specyfikacja_sprzetowa.md)
[![Connectivity](https://img.shields.io/badge/connectivity-LTE%20%7C%20Ethernet%20PoE-red.svg?style=for-the-badge)](docs/02_architektura_systemu.md)
[![Code](https://img.shields.io/badge/code-C%2B%2B%20%7C%20Bash-yellow.svg?style=for-the-badge)](src/)
[![AI](https://img.shields.io/badge/AI-Qwen%20Agent-purple.svg?style=for-the-badge)](docs/12_rozszerzenia_przyszlosciowe.md)

![ApiaryGuard Banner](https://img.shields.io/badge/🐝-Professional%20Beekeeping%20Solution-black?style=for-the-badge&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCIgZmlsbD0ibm9uZSIgc3Ryb2tlPSJjdXJyZW50Q29sb3IiIHN0cm9rZS13aWR0aD0iMiI+PHBhdGggZD0iTTEyIDJMMTQgNkwxOCA2TDIwIDEwTDIyIDEwTDIwIDE0TDIyIDE4TDE4IDIyTDE0IDIyTDEyIDI2Ii8+PC9zdmc+)

</div>

---

## 📖 O Projekcie

**ApiaryGuard Pro** to zaawansowany system monitoringu i zarządzania pasieką klasy enterprise, zaprojektowany do pracy w trudnych warunkach terenowych. System umożliwia centralną obsługę **wielu uli** jednocześnie dzięki architekturze master-slave z Raspberry Pi 2 jako serwerem i Raspberry Pi Pico w każdym ulu.

### ✨ Kluczowe Cechy

| Cecha | Opis |
|-------|------|
| 🏗️ **Multi-Tenancy** | Jeden serwer obsługuje dziesiątki uli z unikalnymi ID |
| 📡 **Hybrydowa Łączność** | LTE Aero2 (SIM free) + Ethernet PoE |
| ⚡ **Zero Python** | Czyste C++ i Bash dla maksymalnej wydajności |
| 🤖 **AI-Ready** | Gotowy pod integrację z Qwen Agent |
| 📷 **Vision System** | Kamera PoE z analizą obrazu co 60s |
| 🌡️ **Full Diagnostics** | Waga, dźwięk, wibracje, temp., wilgotność, CO₂, VOC |

---

## 🏗️ Architektura Systemu

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         POZIOM PASIEKI (CLOUD/EDGE)                         │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐          │
│  │   Dashboard     │    │   Analityka     │    │   Powiadom.     │          │
│  │   Web/Mobile    │    │   Danych        │    │   SMS/Email     │          │
│  └────────┬────────┘    └────────┬────────┘    └────────┬────────┘          │
│           └───────────────────────┼──────────────────────┘                   │
│                                   │ API REST                                  │
└───────────────────────────────────┼───────────────────────────────────────────┘
                                    │
                          ┌─────────▼─────────┐
                          │   Apache Server   │
                          │  (Raspberry Pi 2) │
                          │  - HTTP/HTTPS     │
                          │  - Local Database │
                          └─────────┬─────────┘
                                    │
            ┌───────────────────────┼───────────────────────┐
            │                       │                       │
    ┌───────▼───────┐       ┌───────▼───────┐       ┌───────▼───────┐
    │   Moduł LTE   │       │   Ethernet    │       │   Zasilanie   │
    │   (Aero2)     │       │   PoE Splitter│       │     PoE       │
    └───────────────┘       └───────────────┘       └───────────────┘
                                    │
                          ┌─────────▼─────────┐
                          │ Raspberry Pi Pico │
                          │   (Slave Device)  │
                          │   - Sensor Hub    │
                          │   - Actuator Ctrl │
                          └─────────┬─────────┘
                                    │
            ┌───────────────────────┼───────────────────────┐
            │                       │                       │
    ┌───────▼───────┐       ┌───────▼───────┐       ┌───────▼───────┐
    │   Sensory     │       │   Efektory    │       │   Komunikacja │
    │ • HX711 Waga  │       │ • Heater 10W  │       │ • I2C         │
    │ • Mic Audio   │       │ • Fan PWM     │       │ • SPI         │
    │ • DHT22 T/H   │       │ • Dispenser   │       │ • UART        │
    │ • Piezo Vib   │       │ • Valves      │       │ • GPIO        │
    │ • SGP41 Air   │       │ • Relays      │       │               │
    │ • Radar MMWave│       │               │       │               │
    └───────────────┘       └───────────────┘       └───────────────┘
```

---

## 📦 Zawartość Repozytorium

### Aktualna Struktura Plików (Stan: 2025)

```
/workspace/
├── 📘 README.md                        # Główna dokumentacja projektu
├── 📄 LICENSE                          # Apache License 2.0
├── 🔧 hive_monitor_installer.sh        # Automatyczny instalator z TUI
├── 📁 docs/                            # Szczegółowa dokumentacja techniczna
│   ├── 00_README_INTRO.md              # Wprowadzenie do dokumentacji
│   ├── 01_wstep_i_opis_projektu.md     # Opis projektu i cele
│   ├── 02_architektura_systemu.md      # Architektura hardware/software
│   ├── 03_specyfikacja_sprzetowa.md    # Specyfikacja komponentów
│   ├── 04_struktura_katalogow_i_plikow.md  # Struktura repozytorium
│   ├── 05_opis_modulow_programowych.md # Dokumentacja modułów C++
│   ├── 06_funkcjonalnosci_sensorow_i_efektorow.md  # Funkcje sensorów
│   ├── 07_zaawansowane_funkcje_oprogramowania.md   # Zaawansowane funkcje
│   ├── 08_instalacja_i_konfiguracja.md # Instrukcje instalacji
│   ├── 09_api_i_integracje.md          # API REST i integracje
│   ├── 10_bezpieczenstwo_i_niezawodnosc.md  # Bezpieczeństwo
│   ├── 11_konserwacja_i_rozwiazywanie_problemow.md  # Konserwacja
│   ├── 12_rozszerzenia_przyszlosciowe.md  # Roadmap i AI
│   ├── 13_licencja_i_wspolpraca.md     # Licencja i kontrybucje
│   ├── 14_nowe_parametry_sensorow.md   # Nowe metryki sensorów
│   ├── 15_dynamiczna_detekcja_sensorow.md  # Hot-plug sensorów
│   └── http_api_raspberry_pico.md      # API HTTP dla Pico
├── 📁 src/                             # Kod źródłowy projektu
│   ├── pico_refactored/                # Firmware Raspberry Pi Pico (C++)
│   │   ├── apiaryguard_pico.ino        # Główny plik Arduino IDE
│   │   ├── include/                    # Nagłówki C++
│   │   │   ├── config.h                # Konfiguracja pinów i parametrów
│   │   │   ├── sensors.h               # Interfejsy sensorów
│   │   │   ├── hx711.h                 # Driver wagi HX711
│   │   │   ├── audio_analysis.h        # Analiza dźwięku FFT
│   │   │   ├── radar_analysis.h        # Analiza radaru MMWave
│   │   │   ├── air_quality.h           # Sensor jakości powietrza
│   │   │   ├── weight_analysis.h       # Metryki wagowe
│   │   │   ├── network.h               # Komunikacja sieciowa
│   │   │   └── effectors.h             # Sterowanie efektorami
│   │   └── src/                        # Implementacje C++
│   │       ├── sensors.cpp             # Obsługa wszystkich sensorów
│   │       ├── hx711.cpp               # Implementacja wagi
│   │       ├── audio_analysis.cpp      # FFT i detekcja dźwięków
│   │       ├── radar_analysis.cpp      # Przetwarzanie sygnału radaru
│   │       ├── air_quality.cpp         # SGP41/BME688 driver
│   │       ├── weight_analysis.cpp     # Obliczanie metryk wagi
│   │       ├── network.cpp             # HTTP client/server, UDP
│   │       ├── effectors.cpp           # PWM, przekaźniki, dozowniki
│   │       └── weight_analysis.cpp     # Trendy i prognozy wagi
│   │
│   ├── pico_w6100/                     # Firmware Pico + Ethernet W6100
│   │   ├── apiaryguard_pico.ino        # Wersja z kontrolerem Ethernet
│   │   └── README.md                   # Instrukcje specyficzne dla W6100
│   │
│   ├── pico_dual_mode/                 # Tryb dual-core RP2040
│   │   └── pico_dual_mode.ino          # Wykorzystanie obu rdzeni M0+
│   │
│   ├── arduino_nano_dual_mode/         # [ARCHIWALNE] Arduino Nano
│   │   └── apiaryguard_nano_dual.ino   # Legacy code - tylko referencyjnie
│   │
│   ├── rpi_tui/                        # Backend Raspberry Pi 2 (C++/Bash)
│   │   ├── apiary_collector.cpp        # Kolektor danych (UDP Server port 5005)
│   │   ├── apiary_collector_types.h    # Struktury danych kolektora
│   │   ├── apiary_collector_parser.h   # Parser wiadomości UDP
│   │   ├── apiary_collector_computation.h  # Obliczenia statystyczne
│   │   ├── apiary_collector_network.h  # Warstwa sieciowa
│   │   ├── apiary_logger.cpp           # System logowania zdarzeń
│   │   ├── apiary_logger.h             # Interfejs loggera
│   │   ├── apiary_logger_debug.cpp     # Debug logging helpers
│   │   ├── apiary_logger_debug.h       # Nagłówek debug
│   │   ├── apiary_database.cpp         # Operacje na SQLite
│   │   ├── apiary_database.h           # Interfejs bazy danych
│   │   ├── test_debug_main.cpp         # Testy jednostkowe debug
│   │   ├── apiary_tui.sh               # Terminal User Interface (Bash)
│   │   ├── install_apiary.sh           # Skrypt instalacyjny TUI
│   │   ├── Makefile                    # Build system dla C++
│   │   └── README.md                   # Dokumentacja modułu RPi2
│   │
│   ├── webui/                          # Frontend Web (HTML/JS/PHP)
│   │   ├── index.html                  # Główny interfejs użytkownika
│   │   ├── app.js                      # Logika frontendu JavaScript
│   │   ├── api.php                     # Backend PHP dla API REST
│   │   └── README.md                   # Instrukcje deploymentu WebUI
│   │
│   └── android_app/                    # Aplikacja mobilna Android (Kotlin/Java)
│       ├── app/
│       │   ├── src/main/
│       │   │   ├── java/com/apiguard/apiary/
│       │   │   │   ├── ui/             # Activity, Fragments, Views
│       │   │   │   ├── network/        # Retrofit, API clients
│       │   │   │   ├── repository/     # Data repositories
│       │   │   │   ├── data/
│       │   │   │   │   ├── remote/     # API services
│       │   │   │   │   ├── local/      # Room database, DAO
│       │   │   │   │   └── model/      # Data models, DTOs
│       │   │   │   └── model/          # Domain models
│       │   │   ├── res/
│       │   │   │   ├── layout/         # XML layouts
│       │   │   │   ├── values/         # Strings, colors, styles
│       │   │   │   ├── drawable/       # Assets, icons
│       │   │   │   └── mipmap-*/       # App icons
│       │   │   └── AndroidManifest.xml
│       │   ├── build.gradle            # Konfiguracja buildu Android
│       │   └── proguard-rules.pro      # Reguły ProGuard
│       ├── gradle/
│       │   └── wrapper/
│       ├── build.gradle                # Project-level build config
│       ├── settings.gradle             # Ustawienia projektu
│       └── README.md                   # Dokumentacja aplikacji mobilnej
│
├── 📁 doc/                             # Dodatkowa dokumentacja specjalistyczna
│   └── habibi/                         # Materiały badawcze / publikacje
│       ├── spis_tresci.md              # Spis treści opracowania
│       ├── 01_wstep.md                 # Wstęp teoretyczny
│       ├── 02_rozdzial_1_podstawy_prawne.md  # Aspekty prawne
│       ├── 03_rozdzial_2_charakterystyka_osiagniecia.md
│       ├── 03_01_architektura_systemu.md
│       ├── 03_02_innowacyjnosc.md
│       ├── 03_03_wklad_w_dyscypline.md
│       └── 04_rozdzial_3_definicja_osiagniecia.md
│
└── 📁 tools/                           # Narzędzia pomocnicze (planowane)
    ├── flash_pico.sh                   # Flashowanie firmware Pico
    ├── backup_config.sh                # Backup konfiguracji
    └── diagnostic_tools.sh             # Diagnostyka systemu
```

---

## 🔌 Specyfikacja Sprzętowa

### Jednostka Centralna: Raspberry Pi 2 Model B

| Parametr | Wartość |
|----------|---------|
| Procesor | Broadcom BCM2836 Quad-Core 900 MHz |
| RAM | 1 GB LPDDR2 |
| Storage | microSDHC 16GB+ Class 10 |
| Network | 10/100 Mbps Ethernet |
| OS | Raspberry Pi OS Lite (64-bit) |

### Mikrokontroler: Raspberry Pi Pico (RP2040)

| Parametr | Wartość |
|----------|---------|
| MCU | RP2040 Dual-Core ARM Cortex-M0+ @ 133 MHz |
| Flash | 2MB QSPI |
| SRAM | 264 KB |
| ADC | 3x 12-bit |
| Communication | UART, I2C, SPI |

### Sensory

| Sensor | Parametry | Zastosowanie |
|--------|-----------|--------------|
| **HX711 + Strain Gauge** | 24-bit ADC, ±5g precyzji | Waga ula (200kg) |
| **Mikrofon MEMS** | 20Hz-20kHz, SNR >58dB | Analiza brzmienia rodziny |
| **DHT22/AM2302** | -40°C do +80°C, 0-100% RH | Temperatura i wilgotność |
| **Piezo Transducer** | 1Hz-10kHz | Wibracje i akustyka |
| **SGP41/BME688** | CO₂, VOC, NOx, Etanol | Jakość powietrza |
| **MMWave Radar** | 24GHz/60GHz, 0.2-8m | Detekcja ruchu pszczół |
| **Kamera PoE** | 2MP HD | Monitoring wizyjny |

### Efektory

| Efektor | Specyfikacja | Funkcja |
|---------|--------------|---------|
| **Grzałka** | 10W @ 12V DC | Termoterapia, suszenie |
| **Wentylator** | 20 CFM @ 12V PWM | Chłodzenie, wentylacja |
| **Dozownik** | ±0.1ml precyzji | Leki, olejki, syrop |
| **Zawory** | 12V solenoid NC | Kontrola wylotka |
| **Przekaźniki** | 8-kanałowy 10A | Sterowanie urządzeniami |

---

## 🚀 Szybki Start

### 1. Instalacja Automatyczna

```bash
cd /workspace
chmod +x hive_monitor_installer.sh
./hive_monitor_installer.sh
```

### 2. Kompilacja Firmware Pico

```bash
cd /workspace/src/pico_refactored
# Otwórz w Arduino IDE lub PlatformIO
# Skonfiguruj piny w include/config.h
# Wgraj na Raspberry Pi Pico
```

### 3. Uruchomienie Backend na RPi2

```bash
cd /workspace/src/rpi_tui
make all
./apiary_collector          # Tryb sieciowy (UDP port 5005)
# lub
./apiary_collector --sim    # Tryb symulacji (test)
```

### 4. Uruchomienie TUI

```bash
./apiary_tui.sh
```

### 5. Dostęp do WebUI

```bash
# Wymagany Apache2 + PHP
sudo apt-get install -y apache2 libapache2-mod-php php-curl
# Pliki WebUI są w /workspace/src/webui/
# Skopiuj do /var/www/html/
```

---

## 📊 Funkcjonalności

### Monitorowanie w Czasie Rzeczywistym

- **Waga**: 80 metryk HX711 (trendy, prognozy, anomalie)
- **Audio**: FFT analysis, detekcja rojenia, queenless sound
- **Środowisko**: Temp, wilgotność, CO₂, VOC, ciśnienie
- **Wizja**: Liczenie pszczół, detekcja intruzów, time-lapse

### Automatyzacja i Efektori

- **Termoregulacja**: PID control grzałki i wentylatora
- **Terapia**: Automatyczne podawanie leków (warroza, grzybice)
- **Bezpieczeństwo**: Zamykanie wylotka nocą/zimą
- **Alerty**: Powiadomienia SMS/Email przy anomaliach

### Zaawansowana Analityka

| Kategoria | Metryki |
|-----------|---------|
| **Statystyczne** | mean, std, min, max, variance, skewness, kurtosis, Gini |
| **Temporalne** | current_rate, acceleration, jerk, entropy |
| **Trendy** | slopes (1h/4h/24h), correlation, direction, strength |
| **Pożytki** | nectar_inflow_rate, foraging_efficiency, honey_production_idx |
| **Konsumpcja** | consumption_rate, food_reserve_days, starvation_risk |
| **Cykliczność** | daily_amplitude, circadian_strength, seasonal_trend |
| **Zdrowie** | colony_growth_rate, stress_indicator, vitality_index |
| **Prognozy** | predicted_weight_24h, forecast_confidence, expected_yield |

---

## 🔌 API i Integracje

### REST API Endpoints

| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/api/hives` | GET | Lista wszystkich uli |
| `/api/hives/{id}` | GET | Dane konkretnego ula |
| `/api/sensors/{id}` | GET | Ostatnie odczyty sensorów |
| `/api/actuators/{id}` | POST | Sterowanie efektorami |
| `/api/alerts` | GET | Historia alertów |
| `/api/reports/daily` | GET | Raport dobowy |

### Przykład Zapytania

```bash
curl -X GET http://localhost:8080/api/hives/UL-1 \
  -H "Accept: application/json"
```

### Przykład Odpowiedzi

```json
{
  "id": "UL-1",
  "timestamp": "2025-01-15T10:30:00Z",
  "sensors": {
    "weight": 45.67,
    "temperature": 24.3,
    "humidity": 65.5,
    "co2": 850,
    "voc": 120
  },
  "status": "healthy",
  "battery": 98
}
```

---

## 🛡️ Bezpieczeństwo

- **EMF Shielding**: Mu-metal compartments dla radarów MMWave
- **IP68 Enclosure**: Pełna ochrona przed wodą i pyłem
- **Watchdog Timer**: 8s timeout z auto-reset
- **Thermal Protection**: Thermal fuse 45°C dla grzałek
- **Encryption**: HTTPS/TLS dla komunikacji zdalnej
- **Authentication**: JWT tokens dla API

---

## 🔧 Konserwacja

### Harmonogram

| Częstotliwość | Zadanie |
|---------------|---------|
| **Codziennie** | Check statusu online, alerty |
| **Co tydzień** | Przegląd logów, backup danych |
| **Co miesiąc** | Kalibracja wag, czyszczenie sensorów |
| **Co kwartał** | Test baterii, aktualizacja firmware |
| **Rocznie** | Pełny przegląd mechaniczny, wymiana uszczelek |

### Troubleshooting

| Problem | Rozwiązanie |
|---------|-------------|
| Brak danych z ula | Sprawdź połączenie Ethernet, restart Pico |
| Niepoprawna waga | Przeprowadź kalibrację HX711 (tare + scale) |
| Wysoka temperatura | Sprawdź wentylator, zwiększ PWM |
| Alert CO₂ | Zwiększ wentylację, sprawdź szczelność |
| Offline LTE | Sprawdź kartę SIM Aero2, restart dongle |

---

## 📈 Roadmap Rozwoju

### Q1 2025
- [ ] Integracja Qwen AI Agent
- [ ] Predykcja rójki ML
- [ ] Mobile App (iOS/Android)

### Q2 2025
- [ ] LoRaWAN mesh networking
- [ ] Solar power management
- [ ] Multi-apiary dashboard

### Q3 2025
- [ ] Computer vision disease detection
- [ ] Autonomous treatment protocols
- [ ] Cloud sync & backup


<div align="center">

**Made with ❤️ for Beekeepers Worldwide**

[⬆ Wróć do góry](#-apiaryguard-pro)

</div>

---

## 📊 Kompleksowa Lista Parametrów Systemowych

ApiaryGuard Pro przetwarza **ponad 338+ parametrów** w czasie rzeczywistym, podzielonych na 8 głównych modułów sensorycznych:

| Moduł | Liczba Parametrów | Kategorie Metryk | Przykładowe Wskaźniki |
|-------|-------------------|------------------|----------------------|
| **HX711 (Waga)** | 105+ | Statystyki, Trendy, Pożytek, Konsumpcja, Cykle, Zdrowie, Prognozy | `nectar_inflow_rate`, `colony_growth_rate`, `winter_readiness`, `predicted_weight_24h` |
| **Audio (MEMS Mic)** | 97+ | Czasowe, Częstotliwościowe, FFT, Psychoakustyczne, Bioakustyczne | `swarm_probability`, `bee_activity_index`, `spectral_centroid`, `harmonic_to_noise_ratio` |
| **Radar MMWave** | 27 | Odległość, Energia, Prędkość, Anomalie, Jakość | `hive_health_index`, `target_density`, `motion_intensity`, `anomaly_score` |
| **TempHumidity** | 28 | Komfort Termiczny, Stabilność, Trendy, Alarmy | `heat_index`, `dew_point`, `brood_stress_index`, `mold_risk` |
| **AirQuality** | 24 | Gazy (CO₂/VOC/NOx), IAQ, Wentylacja, Alerty | `iaq_index`, `ventilation_need`, `contamination_risk`, `stress_from_air` |
| **PiezoVibration** | 22 | Amplituda, Częstotliwość, Klasyfikacja Źródeł | `bee_traffic_score`, `intrusion_probability`, `predator_vib_score` |
| **Barometric** | 18 | Ciśnienie, Prognoza Pogody, Warunki Wylotów | `weather_trend`, `storm_probability`, `foraging_conditions` |
| **Light** | 17 | Natężenie, Cykl Dobowy, Synchronizacja Cyrkadiana | `circadian_sync`, `daylight_duration`, `foraging_light_index` |

### Przykładowe Zastosowania Zaawansowanych Metryk

#### 🍯 Detekcja Pożytku i Produkcji Miodu
```json
{
  "nectar_inflow_rate": 0.45,        // kg/h - aktualny napływ nektaru
  "foraging_efficiency": 87.3,       // % - efektywność zbierania
  "honey_production_idx": 92.1,      // % - indeks produkcji miodu
  "bloom_intensity": 78.5,           // % - intensywność kwitnienia
  "expected_honey_yield": 15.2       // kg - oczekiwany zbiór
}
```

#### 🚨 Wczesne Wykrywanie Rójki
```json
{
  "swarm_probability": 87.3,         // % - prawdopodobieństwo rojenia
  "weight_drop_24h": -2.3,           // kg - utrata wagi
  "audio_piping_detected": true,     // wykrycie dźwięków piping
  "radar_activity_spike": 156.7,     // % - wzrost aktywności
  "temperature_elevation": 3.2       // °C - wzrost temp. w ulu
}
```

#### 🦠 Detekcja Chorób i Pasożytów
```json
{
  "varroa_detection_confidence": 0.82,  // pewność wykrycia warrozy
  "nosema_risk_index": 34.5,            // ryzyko nosemy
  "colony_growth_rate": -2.3,           // %/dzień - spadek populacji
  "stress_indicator": 0.67,             // 0-1 - poziom stresu
  "vitality_index": 45.2                // % - witalność kolonii
}
```

---

## 🔧 Szczegółowa Specyfikacja Techniczna

### Architektura Oprogramowania

#### Warstwa Firmware (Raspberry Pi Pico - C++)
- **Dynamiczna Detekcja Sensorów**: Automatyczne wykrywanie podłączonych sensorów przy starcie
- **Real-time Processing**: Przetwarzanie sygnałów z częstotliwością do 8kHz (audio)
- **Watchdog Timer**: 8s timeout z auto-reset dla niezawodności
- **Low-power Modes**: Zarządzanie energią dla trybu bateryjnego
- **HTTP API Server**: Wbudowany serwer HTTP dla komunikacji z RPi2

#### Warstwa Backend (Raspberry Pi 2 - C++ + Bash)
- **UDP Collector**: Wysokowydajny kolektor danych (port 5005)
- **SQLite Database**: Lokalna baza z historią pomiarów
- **Gentle Code Pattern**: Odporność na błędy bez przerywania działania
- **Multi-hive Support**: Obsługa wielu uli jednocześnie
- **TUI Interface**: Terminal User Interface w Bash

#### Warstwa Frontend (WebUI - HTML/JS/PHP)
- **Real-time Dashboard**: Aktualizacja co 5 sekund
- **Responsive Design**: Działa na mobile i desktop
- **Chart Visualization**: Wykresy trendów i historii
- **Alert Management**: Panel alertów i powiadomień

### Protokoły Komunikacyjne

| Interfejs | Protokół | Częstotliwość | Zastosowanie |
|-----------|----------|---------------|--------------|
| **Pico ↔ RPi2** | HTTP/JSON | 1-10 Hz | Telemetria sensorów |
| **RPi2 ↔ Database** | SQLite API | Event-driven | Zapis odczytów |
| **RPi2 ↔ WebUI** | REST/JSON | On-demand | Dashboard dane |
| **RPi2 ↔ Cloud** | HTTPS/MQTT | 15 min | Sync zdalny |
| **LTE Modem** | PPP/AT Commands | N/A | Łączność zewnętrzna |

### Bezpieczeństwo i Niezawodność

- **EMF Shielding**: Osłony mu-metal dla radarów MMWave
- **IP68 Enclosure**: Pełna ochrona przed wodą i pyłem
- **Thermal Protection**: Thermal fuse 45°C dla grzałek
- **Data Encryption**: TLS 1.3 dla komunikacji zdalnej
- **Authentication**: JWT tokens dla API
- **Redundancy**: Podwójne zasilanie (PoE + bateria)
- **Auto-recovery**: Watchdog + health check scripts

---

## 🤖 Integracja z AI - Qwen Agent

ApiaryGuard Pro integruje zaawansowanego asystenta AI opartego na modelu **Qwen** (Alibaba Cloud), który zapewnia:

### Funkcjonalności AI

1. **Autonomiczny Monitoring**: Analiza strumieni danych w czasie rzeczywistym z rekomendacjami działań
2. **Conversational Dashboard**: Naturalna interakcja głosowa/tekstowa z systemem
3. **Predykcyjne Scenariusze**: Wielowariantowe symulacje przyszłych stanów ula
4. **Generowanie Kodu**: Automatyczne tworzenie skryptów Bash/C++ na żądanie
5. **Detekcja Anomalii**: Wykrywanie nietypowych wzorców z wyjaśnieniami przyczyn

### Przykład Dialogu z AI

```
👤 Użytkownik: "Pokaż mi ule z problemami zdrowotnymi"

🤖 Qwen Agent:
🔍 Przeanalizowano 12 uli...

Znaleziono 3 ule wymagające uwagi:

📍 Ul #7 - WARROZA (prawdopodobieństwo 87%)
   - Osyp: 47 roztoczy/24h (norma: <10)
   - Audio: drapanie w paśmie 200-500Hz
   - Waga: spadek kondycji -1.2kg
   💡 Zalecenie: Kwas mrówkowy + termoterapia

📍 Ul #12 - NOSEMA (ryzyko 64%)
   - Konsumpcja: wzrost o 34% przy spadku masy
   - Audio: obniżona aktywność (bee_activity_index: 34%)
   💡 Zalecenie: Badanie mikroskopowe, suplementacja

📍 Ul #3 - STRES TERMICZNY
   - Temperatura: 38.5°C (przekroczenie o 3.5°C)
   - Wentylacja: maksymalna (wentylator 100%)
   💡 Zalecenie: Dodaj izolację, sprawdź cień

Czy chcesz wygenerować plan leczenia dla wszystkich 3 uli 
```
## 👥 Współpraca (Contribute)

Wkład w rozwój projektu jest mile widziany! Zobacz nasze [wytyczne dla kontrybutorów](docs/13_licencja_i_wspolpraca.md).

### Kontakt

- 📧 Email: contact@apiaryguard.pro
- 🌐 Strona: https://apiaryguard.pro
- 💬 Discord: https://discord.gg/apiaryguard
- 🐦 Twitter: @ApiaryGuardPro
- 📚 Dokumentacja: [/docs](docs/README.md)

### Jak Kontrybuować?

1. Fork repozytorium
2. Stwórz branch feature (`git checkout -b feature/AmazingFeature`)
3. Commit zmian (`git commit -m 'Add AmazingFeature'`)
4. Push (`git push origin feature/AmazingFeature`)
5. Otwórz Pull Request

---

<div align="center">

**ApiaryGuard Pro v2.5.0** | **Enterprise Multi-Apiary Monitoring System**

Made with ❤️ for Beekeepers Worldwide

[⬆ Wróć do góry](#-apiaryguard-pro)

</div>
