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

```
/workspace/
├── 📘 README.md                    # Ta dokumentacja
├── 📄 LICENSE                      # Apache License 2.0
├── 🔧 hive_monitor_installer.sh    # Instalator z TUI
├── 📁 docs/                        # Szczegółowa dokumentacja
│   ├── 00_README_INTRO.md
│   ├── 01_wstep_i_opis_projektu.md
│   ├── 02_architektura_systemu.md
│   ├── 03_specyfikacja_sprzetowa.md
│   ├── 04_struktura_katalogow_i_plikow.md
│   ├── 05_opis_modulow_programowych.md
│   ├── 06_funkcjonalnosci_sensorow_i_efektorow.md
│   ├── 07_zaawansowane_funkcje_oprogramowania.md
│   ├── 08_instalacja_i_konfiguracja.md
│   ├── 09_api_i_integracje.md
│   ├── 10_bezpieczenstwo_i_niezawodnosc.md
│   ├── 11_konserwacja_i_rozwiazywanie_problemow.md
│   ├── 12_rozszerzenia_przyszlosciowe.md
│   ├── 13_licencja_i_wspolpraca.md
│   ├── 14_nowe_parametry_sensorow.md
│   └── 15_dynamiczna_detekcja_sensorow.md
├── 📁 src/                         # Kod źródłowy
│   ├── pico_refactored/            # Firmware Raspberry Pi Pico
│   │   ├── include/                # Nagłówki C++
│   │   └── src/                    # Implementacje C++
│   ├── rpi_tui/                    # Backend Raspberry Pi 2
│   │   ├── apiary_collector.cpp    # Kolektor danych (UDP Server)
│   │   ├── apiary_logger.cpp       # System logowania
│   │   └── apiary_tui.sh           # Terminal UI
│   └── webui/                      # Frontend Web
│       ├── index.html              # Interfejs użytkownika
│       ├── app.js                  # Logika frontendu
│       └── api.php                 # Backend PHP
└── 📁 doc/                         # Dodatkowa dokumentacja
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

---

## 📄 Licencja

Ten projekt jest licencjonowany na warunkach **Apache License 2.0**.  
Szczegóły w pliku [LICENSE](LICENSE).

---

## 👥 Współpraca

Wkład w rozwój projektu jest mile widziany! Zobacz nasze [wytyczne dla kontrybutorów](docs/13_licencja_i_wspolpraca.md).

### Kontakt

- 📧 Email: contact@apiaryguard.pro
- 🌐 Strona: https://apiaryguard.pro
- 💬 Discord: https://discord.gg/apiaryguard

---

<div align="center">

**Made with ❤️ for Beekeepers Worldwide**

[⬆ Wróć do góry](#-apiaryguard-pro)

</div>
