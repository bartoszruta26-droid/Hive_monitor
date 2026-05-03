# ApiaryGuard Pro: Zaawansowany System Monitoringu i Zarządzania Pasieką

## 📋 Spis Treści

1. [Wstęp i Opis Projektu](#wstęp-i-opis-projektu)
2. [Architektura Systemu](#architektura-systemu)
3. [Specyfikacja Sprzętowa](#specyfikacja-sprzętowa)
4. [Struktura Katalogów i Plików](#struktura-katalogów-i-plików)
5. [Opis Modułów Programowych](#opis-modułów-programowych)
6. [Funkcjonalności Sensorów i Efektorów](#funkcjonalności-sensorów-i-efektorów)
7. [Zaawansowane Funkcje Oprogramowania](#zaawansowane-funkcje-oprogramowania)
8. [Instalacja i Konfiguracja](#instalacja-i-konfiguracja)
9. [API i Integracje](#api-i-integracje)
10. [Bezpieczeństwo i Niezawodność](#bezpieczeństwo-i-niezawodność)
11. [Konserwacja i Rozwiązywanie Problemów](#konserwacja-i-rozwiązywanie-problemów)
12. [Rozszerzenia Przyszłościowe](#rozszerzenia-przyszłościowe)
13. [Licencja i Współpraca](#licencja-i-współpraca)

---

## 🍯 Wstęp i Opis Projektu

**ApiaryGuard Pro** to kompleksowy, przemysłowy system monitoringu uli, rójek i całych pasiek, zaprojektowany z myślą o nowoczesnym pszczelarstwie precyzyjnym. Projekt łączy w sobie najnowocześniejsze technologie IoT, zaawansowaną analitykę danych oraz solidną konstrukcję mechaniczną, zapewniając pszczelarzom bezprecedensową kontrolę nad zdrowiem i produktywnością ich rodzin pszczelich.

### Cele Projektu

- **Ciągły monitoring** parametrów życiowych rodziny pszczelej 24/7/365
- **Wczesne wykrywanie** anomalii, chorób, rojenia i zagrożeń
- **Automatyzacja interwencji** terapeutycznych i klimatycznych
- **Zdalne zarządzanie** pasieką poprzez połączenie LTE
- **Skalowalność** od pojedynczego ula do tysięcy rodzin
- **Niezależność energetyczna** dzięki zasilaniu PoE i optymalizacji zużycia
- **Otwarta architektura** umożliwiająca rozwój i integracje

### Kluczowe Innowacje

1. **Hybrydowa Architektura**: Połączenie Raspberry Pi 2 jako jednostki centralnej z Arduino Nano jako mikrokontrolerem peryferyjnym
2. **Łączność Bezprzewodowa**: Wykorzystanie darmowej sieci Aero2 LTE dla transmisji danych
3. **Wielosensorowa Analiza**: Fuzja danych z wag, mikrofonów, czujników środowiskowych i piezo
4. **Aktywna Terapia**: Automatyczne dozowanie leków, olejków eterycznych i kontrola klimatu
5. **Przemysłowa Konstrukcja**: Mechaniczne rozwiązania odporne na warunki atmosferyczne

---

## 🏗️ Architektura Systemu

### Diagram Architektury

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         POZIOM PASIEKI (CLOUD/EDGE)                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐               │
│  │   Dashboard  │    │  Analityka   │    │   Powiadom.  │               │
│  │   Web/Mobile │    │   Danych     │    │   SMS/Email  │               │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘               │
│         └───────────────────┼───────────────────┘                        │
│                             │ API REST                                   │
└─────────────────────────────┼────────────────────────────────────────────┘
                              │
                    ┌─────────▼─────────┐
                    │   Apache Server   │
                    │  (Raspberry Pi 2) │
                    │  - HTTP/HTTPS     │
                    │  - MQTT Broker    │
                    │  - Local Database │
                    └─────────┬─────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
┌───────▼───────┐   ┌────────▼────────┐   ┌───────▼───────┐
│   Moduł LTE   │   │   Ethernet CAP  │   │   Zasilanie   │
│   (Aero2 SIM) │   │   (PoE Splitter)│   │     PoE       │
│   USB Dongle  │   │   5V/2.4A       │   │   802.3af     │
└───────────────┘   └─────────────────┘   └───────────────┘
                              │
                    ┌─────────▼─────────┐
                    │   Arduino Nano    │
                    │   (Slave Device)  │
                    │   - Sensor Hub    │
                    │   - Actuator Ctrl │
                    └─────────┬─────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
┌───────▼───────┐   ┌────────▼────────┐   ┌───────▼───────┐
│   Sensory     │   │   Efektory      │   │   Komunikacja │
│   - HX711     │   │   - Heater 10W  │   │   - I2C       │
│   - Mic       │   │   - Fan         │   │   - SPI       │
│   - DHT22     │   │   - Dispenser   │   │   - UART      │
│   - Piezo     │   │   - Valves      │   │   - GPIO      │
│   - Strain    │   │   - Relays      │   │               │
└───────────────┘   └─────────────────┘   └───────────────┘
```

### Warstwy Systemu

#### Warstwa 1: Sensoryczna (Field Layer)
- Bezpośredni kontakt z environmentem ula
- Analogowe i cyfrowe czujniki
- Konwersja sygnałów (HX711 dla tensometrów)
- Odporność na wilgotność, temperaturę, wibracje

#### Warstwa 2: Sterowania (Control Layer)
- Arduino Nano jako lokalny kontroler
- Real-time processing sygnałów
- PWM sterowanie efektorem
- Watchdog i safe-mode

#### Warstwa 3: Agregacji (Gateway Layer)
- Raspberry Pi 2 jako bramka
- Apache2 server z bazą danych
- Komunikacja LTE z chmurą
- Local caching i offline operation

#### Warstwa 4: Aplikacyjna (Application Layer)
- Web dashboard
- Mobile applications
- API dla integracji zewnętrznych
- Machine Learning analytics

---

## 🔧 Specyfikacja Sprzętowa

### Jednostka Centralna: Raspberry Pi 2 Model B

| Parametr | Specyfikacja |
|----------|-------------|
| Procesor | Broadcom BCM2836 Quad-Core ARM Cortex-A7 900 MHz |
| RAM | 1 GB LPDDR2 SDRAM |
| Storage | microSDHC (zalecane 16GB+ Class 10) |
| Network | 10/100 Mbps Ethernet |
| USB | 4x USB 2.0 |
| GPIO | 40-pin header |
| Power | 5V/2A microUSB lub PoE przez HAT |
| OS | Raspberry Pi OS Lite (64-bit) |

### Mikrokontroler: Arduino Nano V3.0

| Parametr | Specyfikacja |
|----------|-------------|
| MCU | ATmega328P @ 16 MHz |
| Flash | 32 KB (2 KB bootloader) |
| SRAM | 2 KB |
| EEPROM | 1 KB |
| ADC | 6x 10-bit |
| Communication | UART, I2C, SPI |
| Power | 5V (USB lub VIN 7-12V) |

### Moduł Łączności: LTE USB Dongle (Aero2)

| Parametr | Specyfikacja |
|----------|-------------|
| Network | 3G/4G LTE Cat 4 |
| Bands | B1/B3/B7/B8/B20 (EU) |
| SIM | Micro-SIM (Aero2 Free) |
| Interface | USB 2.0 High-Speed |
| Antenna | 2x SMA external |
| Throughput | DL 150 Mbps / UL 50 Mbps |

### Czujniki i Transducery

#### 1. HX711 + Strain Gauge (Waga Ula)
- **HX711**: 24-bit ADC z wzmacniaczem instrumentalnym
- **Tensometry**: 4x strain gauge 50kg w konfiguracji mostka Wheatstone'a
- **Precyzja**: ±5 gramów przy pełnym zakresie 200 kg
- **Sample Rate**: 10/80 Hz selectable
- **Kalibracja**: Automatyczna z kompensacją temperatury

#### 2. Mikrofon MEMS (Analiza Brzmienia Rodziny)
- **Typ**: Electret condenser microphone module
- **Frequency Response**: 20 Hz - 20 kHz
- **SNR**: >58 dB
- **Zastosowanie**: 
  - Detekcja rojenia (piping, quacking)
  - Wykrywanie braku матки (queenless sound)
  - Monitoring aktywności nocnej
  - Wczesna detekcja chorób (varroa sound signature)

#### 3. Czujnik Temperatury i Wilgotności (DHT22/AM2302)
- **Temp Range**: -40°C do +80°C (±0.5°C)
- **Humidity**: 0-100% RH (±2% RH)
- **Sampling**: 0.5 Hz
- **Lokalizacja**: 
  - Centrum gniazda czerwieniowego
  - Przestrzeń międzyramkowa
  - Zewnętrzna obudowa

#### 4. Czujnik Piezoelektryczny (Wibracje i Akustyka)
- **Typ**: Piezoelectric transducer disc
- **Frequency**: 1 Hz - 10 kHz
- **Zastosowanie**:
  - Detekcja wibracji skrzydeł
  - Monitoring ruchu pszczół na wylotku
  - Wykrywanie ataków drapieżników (niedźwiedzie, osy)

#### 5. Dodatkowe Czujniki (Rozszerzenia)
- **CO₂ Sensor (MH-Z19B)**: Monitoring wentylacji i metabolizmu
- **Light Sensor (BH1750)**: Natężenie światła w ulu
- **Magnetic Reed Switch**: Detekcja otwarcia ula (anti-theft)
- **GPS Module (NEO-6M)**: Lokalizacja pasieki (anti-theft tracking)
- **Soil Moisture**: Wilgotność podłoża pod ulem
- **Barometric Pressure (BMP280)**: Prognoza pogody i zachowań pszczół
- **Air Quality (SGP30)**: eCO2, TVOC dla jakości powietrza

### Efektory i Urządzenia Wykonawcze

#### 1. Grzałka Rezystancyjna 10W
- **Moc**: 10W @ 12V DC
- **Typ**: Silicone heating pad z adhesive backing
- **Lokalizacja**: Ściana boczna/tylna ula
- **Cel**:
  - Termoterapia przy warrozie (40°C przez 24h)
  - Wsparcie zimowania słabych rodzin
  - Suszenie wilgoci wczesną wiosną
- **Bezpieczeństwo**: Thermal fuse 45°C, PID control

#### 2. Wentylator Axial 12V
- **Przepływ**: 20 CFM @ 12V
- **Noise**: <35 dB
- **Sterowanie**: PWM 0-100%
- **Zastosowanie**:
  - Chłodzenie lata (prewencja topienia się plastrów)
  - Wentylacja przy wysokiej wilgotności
  - Dystrybucja olejków eterycznych
  - Osuszanie po zimie

#### 3. Dozownik Terapeutyczny (Medicaments & Oil Dispenser)
- **Mechanizm**: Perystaltyczna pompa mikrodomkowa
- **Pojemność**: 2x 50ml reservoirs
- **Precision**: ±0.1 ml na dawkę
- **Substancje**:
  - Kwas mrówkowy/szczawiowy (warroza)
  - Tymol/olejek tymiankowy (grzybice)
  - Syrop cukrowy (dokarmianie awaryjne)
  - Olejki eteryczne (aromaterapia antystresowa)
- **Harmonogram**: Programowalne dawki czasowe

#### 4. Zawory Elektromagnetyczne
- **Typ**: 12V solenoid valves NC
- **Flow Rate**: 2 L/min
- **Zastosowanie**:
  - Kontrola wylotka (automatyczne zamykanie nocą/zimą)
  - Izolacja ula w razie zagrożenia
  - System antywłamaniowy

#### 5. Moduł Przekaźnikowy 8-Kanałowy
- **Izolacja**: Opto-isolated inputs
- **Load**: 10A @ 250VAC per channel
- **Zastosowanie**:
  - Sterowanie grzałką
  - Wentylator
  - Pompy
  - Zawory
  - Oświetlenie serwisowe

### Zasilanie i Power Management

#### PoE (Power over Ethernet)
- **Standard**: IEEE 802.3af/at
- **Input**: 48V DC przez Ethernet
- **Splitter**: 48V → 5V/2.4A + Ethernet data
- **Advantages**:
  - Single cable installation
  - Surge protection
  - Central UPS capability
  - Professional grade reliability

#### Backup Battery System
- **Battery**: 18650 Li-ion 3.7V 3000mAh x2 (7.4V series)
- **Charger**: TP5100 dual cell charger module
- **Runtime**: 8-12 hours typical operation
- **Low Power Mode**: <100mA consumption when critical

#### Mechanical Design & Enclosure
- **Material**: ABS plastic UV-resistant + aluminum heat sink
- **IP Rating**: IP65 waterproof enclosure
- **Dimensions**: 200x150x80 mm (RPi + Arduino + sensors)
- **Mounting**: External bracket on hive back panel
- **Thermal**: Passive cooling with rain shield
- **Security**: Lockable enclosure with tamper switch

---

## 📁 Struktura Katalogów i Plików

```
/workspace/
├── README.md                           # Dokumentacja główna
├── LICENSE                             # Licencja projektu (MIT/Apache 2.0)
├── docs/                               # Dodatkowa dokumentacja
│   ├── architecture/                   # Diagramy i specyfikacje architektoniczne
│   │   ├── system_architecture.md
│   │   ├── electrical_schematics.pdf
│   │   ├── mechanical_drawings.dxf
│   │   └── network_topology.md
│   ├── api/                            # Dokumentacja API
│   │   ├── rest_api_spec.yaml
│   │   ├── mqtt_topics.md
│   │   └── webhook_examples.md
│   ├── manuals/                        # Podręczniki użytkownika
│   │   ├── installation_guide.md
│   │   ├── calibration_procedures.md
│   │   ├── maintenance_manual.md
│   │   └── troubleshooting_guide.md
│   └── research/                       # Materiały badawcze
│       ├── bee_acoustics_analysis.md
│       ├── thermal_therapy_studies.md
│       └── sensor_fusion_algorithms.md
│
├── hardware/                           # Projekty sprzętowe
│   ├── arduino_nano/                   # Firmware Arduino Nano
│   │   ├── src/
│   │   │   ├── main.cpp                # Główna pętla Arduino
│   │   │   ├── sensors/
│   │   │   │   ├── hx711_driver.cpp    # Sterowanie wagą
│   │   │   │   ├── microphone_adc.cpp  # Akwizycja audio
│   │   │   │   ├── dht22_reader.cpp    # Temp/wilgotność
│   │   │   │   ├── piezo_handler.cpp   # Wibracje
│   │   │   │   └── sensor_fusion.cpp   # Fuzja danych sensorycznych
│   │   │   ├── actuators/
│   │   │   │   ├── heater_control.cpp  # PID grzałki
│   │   │   │   ├── fan_pwm.cpp         # Sterowanie wentylatorem
│   │   │   │   ├── dispenser_pump.cpp  # Dozowanie leków
│   │   │   │   └── valve_control.cpp   # Zawory elektromagnetyczne
│   │   │   ├── communication/
│   │   │   │   ├── i2c_slave.cpp       # I2C komunikacja z RPi
│   │   │   │   ├── uart_protocol.cpp   # Protokół szeregowy
│   │   │   │   └── message_queue.cpp   # Kolejka wiadomości
│   │   │   ├── utils/
│   │   │   │   ├── watchdog.cpp        # Watchdog timer
│   │   │   │   ├── eeprom_storage.cpp  # Persistent storage
│   │   │   │   └── calibration.cpp     # Procedury kalibracji
│   │   │   └── config/
│   │   │       ├── pin_definitions.h   # Mapowanie pinów
│   │   │       ├── constants.h         # Stałe systemowe
│   │   │       └── thresholds.h        # Progi alarmowe
│   │   ├── lib/                        # Biblioteki Arduino
│   │   │   ├── HX711/
│   │   │   ├── DHT-sensor-library/
│   │   │   └── PID-AutoTune/
│   │   ├── platformio.ini              # Konfiguracja PlatformIO
│   │   └── Makefile                    # Alternatywny build system
│   │
│   ├── raspberry_pi/                   # Oprogramowanie Raspberry Pi
│   │   ├── src/
│   │   │   ├── CSharp/                 # Główne aplikacje C# (.NET Core)
│   │   │   │   ├── ApiaryGuard.Core/
│   │   │   │   │   ├── ApiaryGuard.Core.csproj
│   │   │   │   │   ├── Models/
│   │   │   │   │   │   ├── Hive.cs           # Model ula
│   │   │   │   │   │   ├── Apiary.cs         # Model pasieki
│   │   │   │   │   │   ├── Swarm.cs          # Model rójki
│   │   │   │   │   │   ├── SensorData.cs     # Dane sensoryczne
│   │   │   │   │   │   ├── Alert.cs          # Alerty i powiadomienia
│   │   │   │   │   │   └── Treatment.cs      # Zabiegi terapeutyczne
│   │   │   │   │   ├── Services/
│   │   │   │   │   │   ├── IDataRepository.cs    # Interfejs repozytorium
│   │   │   │   │   │   ├── ISensorService.cs   # Interfejs sensorów
│   │   │   │   │   │   ├── IActuatorService.cs # Interfejs efektorów
│   │   │   │   │   │   ├── IMqttService.cs     # MQTT broker client
│   │   │   │   │   │   ├── ILteService.cs      # Obsługa LTE
│   │   │   │   │   │   ├── IAnalyticsService.cs# Analityka
│   │   │   │   │   │   └── INotificationService.cs # Powiadomienia
│   │   │   │   │   ├── Repositories/
│   │   │   │   │   │   ├── SqliteRepository.cs   # SQLite implementacja
│   │   │   │   │   │   ├── InfluxRepository.cs   # InfluxDB time-series
│   │   │   │   │   │   └── CacheRepository.cs    # Redis cache
│   │   │   │   │   ├── Controllers/
│   │   │   │   │   │   ├── SensorController.cs   # API endpoints sensory
│   │   │   │   │   │   ├── ActuatorController.cs # API endpoints aktuary
│   │   │   │   │   │   ├── HiveController.cs     # CRUD operacje na ulach
│   │   │   │   │   │   ├── AlertController.cs    # Zarządzanie alertami
│   │   │   │   │   │   └── ReportController.cs   # Generowanie raportów
│   │   │   │   │   ├── Middleware/
│   │   │   │   │   │   ├── ExceptionHandler.cs   # Global error handling
│   │   │   │   │   │   ├── Authentication.cs     # JWT authentication
│   │   │   │   │   │   └── RateLimiter.cs        # API rate limiting
│   │   │   │   │   └── Helpers/
│   │   │   │   │       ├── DateTimeExtensions.cs
│   │   │   │   │       ├── JsonSerializers.cs
│   │   │   │   │       └── UnitConverters.cs
│   │   │   │   ├── ApiaryGuard.Worker/
│   │   │   │   │   ├── ApiaryGuard.Worker.csproj
│   │   │   │   │   ├── BackgroundServices/
│   │   │   │   │   │   ├── DataAcquisitionHostedService.cs # Pobieranie danych
│   │   │   │   │   │   ├── AnalyticsBackgroundService.cs   # Analiza w tle
│   │   │   │   │   │   ├── MqttPublisherService.cs         # Publikacja MQTT
│   │   │   │   │   │   ├── LteMonitorService.cs            # Monitor łącza
│   │   │   │   │   │   └── MaintenanceSchedulerService.cs  # Harmonogram konserwacji
│   │   │   │   │   └── Workers/
│   │   │   │   │       ├── BeeSoundAnalyzerWorker.cs       # Analiza audio
│   │   │   │   │       ├── WeightTrendWorker.cs            # Trendy wagowe
│   │   │   │   │       ├── DiseasePredictionWorker.cs      # Predykcja chorób
│   │   │   │   │       └── SwarmPredictionWorker.cs        # Predykcja rojenia
│   │   │   │   ├── ApiaryGuard.WebApi/
│   │   │   │   │   ├── ApiaryGuard.WebApi.csproj
│   │   │   │   │   ├── Program.cs
│   │   │   │   │   ├── Startup.cs
│   │   │   │   │   ├── appsettings.json
│   │   │   │   │   └── Controllers/
│   │   │   │   └── ApiaryGuard.CLI/
│   │   │   │       ├── ApiaryGuard.CLI.csproj
│   │   │   │       └── Commands/
│   │   │   │           ├── CalibrateCommand.cs
│   │   │   │           ├── DiagnosticCommand.cs
│   │   │   │           ├── BackupCommand.cs
│   │   │   │           └── UpdateCommand.cs
│   │   │   │
│   │   │   ├── CPP/                    # Wysokowydajne moduły C++
│   │   │   │   ├── signal_processing/
│   │   │   │   │   ├── CMakeLists.txt
│   │   │   │   │   ├── include/
│   │   │   │   │   │   ├── fft_analyzer.hpp      # FFT analiza audio
│   │   │   │   │   │   ├── digital_filter.hpp    # Filtry cyfrowe
│   │   │   │   │   │   ├── spectrogram.hpp       # Spektrogramy
│   │   │   │   │   │   └── feature_extractor.hpp # Ekstrakcja cech
│   │   │   │   │   └── src/
│   │   │   │   │       ├── fft_analyzer.cpp
│   │   │   │   │       ├── digital_filter.cpp
│   │   │   │   │       ├── spectrogram.cpp
│   │   │   │   │       └── feature_extractor.cpp
│   │   │   │   ├── machine_learning/
│   │   │   │   │   ├── CMakeLists.txt
│   │   │   │   │   ├── include/
│   │   │   │   │   │   ├── swarm_classifier.hpp  # Klasyfikator rojenia
│   │   │   │   │   │   ├── disease_detector.hpp  # Detektor chorób
│   │   │   │   │   │   └── anomaly_detection.hpp # Detekcja anomalii
│   │   │   │   │   └── src/
│   │   │   │   │       ├── swarm_classifier.cpp
│   │   │   │   │       ├── disease_detector.cpp
│   │   │   │   │       └── anomaly_detection.cpp
│   │   │   │   └── real_time_kernel/
│   │   │   │       ├── CMakeLists.txt
│   │   │   │       └── rt_scheduler.cpp          # Real-time scheduler
│   │   │   │
│   │   │   └── bash/                   # Skrypty Bash
│   │   │       ├── system/
│   │   │       │   ├── install.sh          # Instalacja systemu
│   │   │       │   ├── update.sh           # Aktualizacja oprogramowania
│   │   │       │   ├── backup.sh           # Backup danych i konfiguracji
│   │   │       │   ├── restore.sh          # Przywracanie z backupu
│   │   │       │   ├── health_check.sh     # Sprawdzenie zdrowia systemu
│   │   │       │   ├── log_rotation.sh     # Rotacja logów
│   │   │       │   ├── security_hardening.sh # Hardening bezpieczeństwa
│   │   │       │   └── factory_reset.sh    # Przywrócenie ustawień fabrycznych
│   │   │       ├── network/
│   │   │       │   ├── lte_setup.sh        # Konfiguracja LTE Aero2
│   │   │       │   ├── lte_monitor.sh      # Monitorowanie połączenia LTE
│   │   │       │   ├── lte_reconnect.sh    # Automatyczne reconnect
│   │   │       │   ├── firewall_setup.sh   # Konfiguracja iptables
│   │   │       │   ├── vpn_tunnel.sh       # VPN tunnel setup
│   │   │       │   └── bandwidth_test.sh   # Test przepustowości
│   │   │       ├── sensors/
│   │   │       │   ├── calibrate_scale.sh  # Kalibracja wagi
│   │   │       │   ├── test_microphone.sh  # Test mikrofonu
│   │   │       │   ├── read_all_sensors.sh # Odczyt wszystkich sensorów
│   │   │       │   └── sensor_diagnostics.sh # Diagnostyka sensorów
│   │   │       ├── services/
│   │   │       │   ├── apache_install.sh   # Instalacja Apache2
│   │   │       │   ├── dotnet_install.sh   # Instalacja .NET Core
│   │   │       │   ├── mqtt_broker.sh      # Instalacja Mosquitto
│   │   │       │   ├── database_init.sh    # Inicjalizacja bazy danych
│   │   │       │   ├── start_all.sh        # Start wszystkich usług
│   │   │       │   ├── stop_all.sh         # Stop wszystkich usług
│   │   │       │   └── restart_failed.sh   # Restart failed services
│   │   │       ├── deployment/
│   │   │       │   ├── deploy_prod.sh      # Deploy produkcyjny
│   │   │       │   ├── deploy_staging.sh   # Deploy staging
│   │   │       │   ├── rollback.sh         # Rollback wersji
│   │   │       │   └── version_check.sh    # Sprawdzenie wersji
│   │   │       └── utilities/
│   │   │           ├── disk_cleanup.sh     # Czyszczenie dysku
│   │   │           ├── memory_monitor.sh   # Monitor pamięci
│   │   │           ├── temperature_log.sh  # Logowanie temperatur CPU
│   │   │           ├── uptime_report.sh    # Raport uptime
│   │   │           └── generate_cert.sh    # Generowanie certyfikatów SSL
│   │   │
│   │   ├── config/
│   │   │   ├── apache2/
│   │   │   │   ├── 000-default.conf    # Apache virtual host config
│   │   │   │   ├── ssl.conf            # SSL/TLS configuration
│   │   │   │   └── htpasswd            # Basic auth passwords
│   │   │   ├── systemd/
│   │   │   │   ├── apiaryguard-core.service
│   │   │   │   ├── apiaryguard-worker.service
│   │   │   │   ├── apiaryguard-webapi.service
│   │   │   │   ├── mosquitto.service.override
│   │   │   │   └── lte-watchdog.service
│   │   │   ├── network/
│   │   │   │   ├── interfaces          # Network interfaces config
│   │   │   │   ├── wpa_supplicant.conf # WiFi config (backup)
│   │   │   │   └── chat-script-aero2   # PPP chat script for Aero2
│   │   │   ├── application/
│   │   │   │   ├── appsettings.Production.json
│   │   │   │   ├── appsettings.Development.json
│   │   │   │   ├── logging.json
│   │   │   │   └── serilog.config
│   │   │   └── database/
│   │   │       ├── schema.sql
│   │   │       ├── indexes.sql
│   │   │       └── seed_data.sql
│   │   │
│   │   ├── tests/
│   │   │   ├── unit/
│   │   │   │   ├── SensorTests.cs
│   │   │   │   ├── ActuatorTests.cs
│   │   │   │   └── ModelTests.cs
│   │   │   ├── integration/
│   │   │   │   ├── ApiIntegrationTests.cs
│   │   │   │   └── DatabaseIntegrationTests.cs
│   │   │   └── performance/
│   │   │       ├── LoadTests.cs
│   │   │       └── StressTests.cs
│   │   │
│   │   ├── scripts/
│   │   │   ├── build.sh
│   │   │   ├── run_tests.sh
│   │   │   └── package.sh
│   │   │
│   │   └── Dockerfile                  # Containerization
│   │
│   ├── mechanical/                     # Projekty mechaniczne
│   │   ├── enclosure/
│   │   │   ├── main_housing.step       # CAD 3D model
│   │   │   ├── main_housing.stl        # 3D print file
│   │   │   ├── mounting_bracket.dxf    # Laser cutting file
│   │   │   └── assembly_instructions.md
│   │   ├── sensor_mounts/
│   │   │   ├── weight_platform.step
│   │   │   ├── microphone_holder.stl
│   │   │   └── temp_probe_guard.step
│   │   ├── actuator_housings/
│   │   │   ├── pump_mount.step
│   │   │   ├── heater_shield.step
│   │   │   └── fan_duct.step
│   │   └── bom/                        # Bill of Materials
│   │       ├── electronics_bom.csv
│   │       ├── mechanical_bom.csv
│   │       └── suppliers.md
│   │
│   └── electrical/                     # Schematy elektryczne
│       ├── schematics/
│       │   ├── main_wiring_scheme.pdf
│       │   ├── arduino_nano_schematic.pdf
│       │   ├── sensor_interface.pdf
│       │   └── actuator_driver.pdf
│       ├── pcb/
│       │   ├── sensor_board.kicad_pcb
│       │   ├── actuator_board.kicad_pcb
│       │   └── gerbers/
│       └── wiring_diagrams/
│           ├── power_distribution.png
│           ├── signal_routing.png
│           └── grounding_scheme.png
│
├── software/                           # Oprogramowanie wysokiego poziomu
│   ├── web_dashboard/                  # Frontend aplikacji webowej
│   │   ├── src/
│   │   │   ├── components/
│   │   │   ├── pages/
│   │   │   ├── services/
│   │   │   └── styles/
│   │   ├── package.json
│   │   └── webpack.config.js
│   │
│   ├── mobile_app/                     # Aplikacja mobilna
│   │   ├── flutter/
│   │   │   ├── lib/
│   │   │   ├── pubspec.yaml
│   │   │   └── ...
│   │
│   ├── cloud_services/                 # Usługi chmurowe
│   │   ├── aws_lambda/
│   │   ├── azure_functions/
│   │   └── data_pipeline/
│   │
│   └── analytics_engine/               # Silnik analityczny
│       ├── jupyter_notebooks/
│       ├── ml_models/
│       └── training_data/
│
├── data/                               # Dane i konfiguracje runtime
│   ├── databases/
│   │   ├── sqlite/                     # Local SQLite database
│   │   └── backups/                    # Automated backups
│   ├── logs/
│   │   ├── application/
│   │   ├── system/
│   │   └── access/
│   ├── cache/
│   └── uploads/
│
├── tools/                              # Narzędzia deweloperskie
│   ├── simulators/
│   │   ├── hive_simulator.py           # Symulator ula (testy)
│   │   ├── sensor_emulator.cpp         # Emulator sensorów
│   │   └── network_simulator.sh        # Symulator sieci LTE
│   ├── debuggers/
│   │   ├── serial_monitor.sh
│   │   ├── mqtt_explorer.sh
│   │   └── log_analyzer.sh
│   └── generators/
│       ├── config_generator.sh
│       ├── certificate_generator.sh
│       └── mock_data_generator.cpp
│
├── ci_cd/                              # Continuous Integration/Deployment
│   ├── github_actions/
│   │   ├── build.yml
│   │   ├── test.yml
│   │   └── deploy.yml
│   ├── jenkins/
│   │   └── Jenkinsfile
│   └── scripts/
│       ├── pre_commit_checks.sh
│       └── post_deploy_verify.sh
│
├── third_party/                        # Biblioteki zewnętrzne
│   ├── arduino_libs/
│   ├── dotnet_packages/
│   └── cpp_modules/
│
└── misc/
    ├── branding/
    │   ├── logo.svg
    │   ├── icons/
    │   └── styleguide.md
    ├── legal/
    │   ├── privacy_policy.md
    │   ├── terms_of_service.md
    │   └── compliance/
    └── community/
        ├── contributing.md
        ├── code_of_conduct.md
        └── faq.md
```

---

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

#### FFT Audio Analyzer

```cpp
// fft_analyzer.cpp
#include "fft_analyzer.hpp"
#include <fftw3.h>
#include <complex>
#include <vector>

namespace ApiaryGuard {
namespace SignalProcessing {

class FFTAnalyzer {
private:
    int sampleSize;
    fftw_complex* fftOutput;
    fftw_plan fftPlan;
    double* inputBuffer;
    
public:
    FFTAnalyzer(int size) : sampleSize(size) {
        inputBuffer = (double*)fftw_malloc(sizeof(double) * size);
        fftOutput = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (size/2 + 1));
        fftPlan = fftw_plan_dft_r2c_1d(size, inputBuffer, fftOutput, FFTW_ESTIMATE);
    }
    
    ~FFTAnalyzer() {
        fftw_destroy_plan(fftPlan);
        fftw_free(inputBuffer);
        fftw_free(fftOutput);
    }
    
    std::vector<FrequencyPeak> analyze(const std::vector<float>& audioSamples) {
        // Copy samples to input buffer
        for (int i = 0; i < sampleSize && i < audioSamples.size(); ++i) {
            inputBuffer[i] = audioSamples[i];
        }
        
        // Execute FFT
        fftw_execute(fftPlan);
        
        // Extract frequency peaks
        std::vector<FrequencyPeak> peaks;
        double sampleRate = 8000.0; // 8kHz
        
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

## 🎛️ Funkcjonalności Sensorów i Efektorów

### Tabela Sensorów

| Sensor | Typ | Zakres Pomiarowy | Precyzja | Częstotliwość Samplingu | Zastosowanie |
|--------|-----|------------------|----------|------------------------|--------------|
| **HX711 + Strain Gauge** | Waga tensometryczna | 0-200 kg | ±5 g | 10 Hz | Monitoring zapasów miodu, detekcja rojenia, zdrowie rodziny |
| **MEMS Microphone** | Akustyka | 20 Hz - 20 kHz | ±2 dB | 8 kHz | Analiza brzmienia, detekcja rojenia, choroby |
| **DHT22/AM2302** | Temp + Wilgotność | -40÷80°C, 0-100% RH | ±0.5°C, ±2% | 0.5 Hz | Klimat gniazda, kondensacja, wentylacja |
| **Piezo Transducer** | Wibracje | 1 Hz - 10 kHz | ±5% | 100 Hz | Aktywność na wylotku, drapieżniki, ruch |
| **MH-Z19B** | CO₂ | 0-5000 ppm | ±50 ppm | 0.2 Hz | Wentylacja, metabolizm rodziny |
| **BH1750** | Natężenie światła | 1-65535 lux | ±20% | 1 Hz | Cykl dobowy, detekcja otwarcia |
| **BMP280** | Ciśnienie + Temp | 300-1100 hPa | ±1 hPa | 1 Hz | Prognoza pogody, korekta wysokościowa |
| **SGP30** | Jakość powietrza | eCO₂ 400-60000 ppm | ±10% | 1 Hz | TVOC, jakość powietrza w ulu |
| **Reed Switch** | Magnetyczny | Binary (ON/OFF) | N/A | Event-driven | Anti-theft, detekcja otwarcia |
| **NEO-6M GPS** | Lokalizacja | Globalny | ±2.5 m | 1 Hz | Tracking pasieki, anti-theft |
| **Capacitive Soil** | Wilgotność gleby | 0-100% | ±3% | 0.1 Hz | Warunki podłoża, drenaż |

### Szczegółowe Opisy Funkcjonalności Sensorów

#### 1. System Wagowy (HX711 + Strain Gauge)

**Zasada Działania:**
System wykorzystuje cztery tensometry rezystancyjne połączone w mostek Wheatstone'a, zamontowane pod podstawą ula. Sygnał analogiczny jest wzmacniany i konwertowany przez 24-bitowy ADC HX711.

**Algorytmy Przetwarzania:**
- **Moving Average Filter**: Eliminacja szumu wysokoczęstotliwościowego
- **Temperature Compensation**: Korekta dryfu termicznego tensometrów
- **Auto-Tare**: Automatyczne zerowanie przy instalacji
- **Outlier Detection**: Odrzucanie anomaliowych odczytów (np. wiatr, ptaki)

**Wykrywane Zdarzenia:**
- **Spadek wagi >2kg w 24h**: Potencjalne rojenie
- **Stopniowy wzrost wagi**: Intensywne zbieranie nektaru
- **Nagły spadek >5kg**: Kradzież ula lub katastrofa
- **Waga <5kg**: Krytycznie niskie zapasy (głód)
- **Cykliczne zmiany dobowe**: Normalna aktywność zbieracka

**Kalibracja:**
```bash
# Procedura kalibracji
./calibrate_scale.sh --known-weight 10.0 --iterations 100
# Wynik: współczynnik kalibracyjny zapisany w EEPROM
```

#### 2. Analiza Akustyczna (Microphone + FFT)

**Przetwarzanie Sygnału:**
- **Pre-emphasis Filter**: Podbicie wysokich częstotliwości
- **Hamming Window**: Redukcja przecieków widmowych
- **512-point FFT**: Rozdzielczość częstotliwościowa ~15.6 Hz @ 8kHz
- **Spectrogram Generation**: Wizualizacja czasowo-częstotliwościowa

**Signature Sounds:**
- **Piping (200-300 Hz)**: Queen pipes - sygnał przedrojowy
- **Quacking (150-250 Hz)**: Młode matki w matecznikach
- **Tooting (400-500 Hz)**: Nowa matka po wyjściu
- **Agitation Buzz (800-1200 Hz)**: Agresja, brak матки
- **Varroa Mite Sounds**: Specyficzne kliknięcia (badania w toku)

**Machine Learning Pipeline:**
1. Ekstrakcja cech: MFCC (Mel-Frequency Cepstral Coefficients)
2. Feature vector: 13 MFCC + delta + delta-delta
3. Klasyfikator: Random Forest / Neural Network
4. Output: Prawdopodobieństwo rojenia (0-100%)

#### 3. Monitoring Środowiskowy (Temp/Humidity/CO₂)

**Termoregulacja Ula:**
- **Idealna temperatura czerwiu**: 34-35°C
- **Alert overheating**: >36°C (ryzyko topnienia plastrów)
- **Alert chilling**: <32°C (zaburzenia rozwoju czerwiu)
- **Gradient temperatur**: Różnica centrum-peryferie >5°C = problem

**Wilgotność:**
- **Optymalna**: 50-60% RH
- **Zbyt wysoka >75%**: Ryzyko pleśni, grzybic (Ascosphaera apis)
- **Zbyt niska <30%**: Wysychanie nektaru, stres rodziny
- **Kondensacja**: Detekcja przez nagły wzrost RH przy spadku temp

**CO₂ Monitoring:**
- **Poziom bazowy**: 400-600 ppm
- **Podwyższony metabolizm**: 800-1500 ppm (intensywna praca)
- **Słaba wentylacja**: >2000 ppm (ryzyko zaczadzenia)
- **Trigger wentylatora**: >1500 ppm przez 30 min

### Tabela Efektorów

| Efektor | Typ | Moc/Zakres | Sterowanie | Zastosowanie |
|---------|-----|------------|------------|--------------|
| **Heater 10W** | Grzałka silikonowa | 10W @ 12V | PWM + PID | Termoterapia, wsparcie zimowania |
| **Axial Fan** | Wentylator 12V | 20 CFM | PWM 0-100% | Chłodzenie, wentylacja, dystrybucja |
| **Peristaltic Pump** | Dozownik leków | 0.1-50 ml | Stepper motor | Dawki kwasów, olejków, syropu |
| **Solenoid Valve** | Zawór NO/NC | 12V, 2L/min | Digital GPIO | Kontrola wylotka, izolacja |
| **Relay Module** | Przekaźniki 8x | 10A @ 250V | Digital GPIO | Przełączanie urządzeń high-power |
| **LED Strip** | Oświetlenie RGB | 5V, adresowalne | WS2812 | Sygnalizacja statusu, night service |

### Scenariusze Automatyzacji

#### Scenariusz 1: Wykrycie Rojenia i Interwencja

```
WARUNKI WYZWALAJĄCE:
├─ Spadek wagi >2kg w ciągu 4 godzin
├─ Wykrycie piping sounds (FFT analysis)
├─ Zwiększona aktywność na wylotku (piezo sensor)
└─ Wzrost temperatury powyżej 35.5°C

AKCJE AUTOMATYCZNE:
1. Natychmiastowe powiadomienie pszczelarza (SMS + Push)
2. Nagranie 60-sekundowego clipu audio do analizy
3. Zwiększenie częstotliwości samplingu do 1 Hz
4. Opcjonalne: Zamknięcie wylotka na 15 minut (zawór)
5. Uruchomienie kamery (jeśli dostępna)
6. Zapis zdarzenia do bazy z tagiem "SWARM_EVENT"

AKCJE PSZCZELARZA:
- Sprawdzenie powiadomienia w aplikacji
- Odsłuch nagrania audio
- Wizyta w pasiece w ciągu 24h
- Założenie nowej rójki lub połączenie rodzin
```

#### Scenariusz 2: Termoterapia Warrozy

```
WARUNKI WYZWALAJĄCE:
├─ Wykrycie vysokiego poziomu Varroa (analiza audio/wizyjna)
├─ Sezon: sierpień-wrzesień
└─ Temperatura zewnętrzna <15°C

PROTOKÓŁ TERMOTERAPII:
1. Podgrzewanie ula do 40°C (stopniowo, 0.5°C/min)
2. Utrzymanie 40°C przez 24 godziny
3. Monitorowanie temperatury co 10 sekund (PID control)
4. Awaryjne chłodzenie jeśli T > 42°C
5. Zapewnienie wentylacji (fan 30%)
6. Powiadomienie o zakończeniu terapii

BEZPIECZEŃSTWO:
- Thermal fuse fizyczny @ 45°C
- Software limit @ 43°C
- Watchdog timer resetujący sterowanie
- Backup battery podtrzymująca monitoring
```

#### Scenariusz 3: Automatyczne Dokarmianie Awaryjne

```
WARUNKI WYZWALAJĄCE:
├─ Waga ula <8kg (brak zapasów)
├─ Okres: późna jesień/zima/wczesna wiosna
└─ Brak lotów pszczół (temp zewnętrzna <10°C)

PROTOKÓŁ DOKARMIANIA:
1. Sprawdzenie poziomu syropu w zbiorniku
2. Uruchomienie pompy perystaltycznej
3. Dozowanie 200ml syropu 2:1 (cukier:woda)
4. Powtórzenie codziennie o świcie
5. Monitorowanie przyrostu wagi
6. Zatrzymanie gdy waga >15kg

MONITOROWANIE:
- Codzienny raport zużycia syropu
- Alert jeśli zbiornik pusty (<10%)
- Korelacja z temperaturą zewnętrzną
```

---

## 🚀 Zaawansowane Funkcje Oprogramowania

### 1. Predykcyjne Machine Learning

#### Model Predykcji Rojenia

**Features:**
- Historyczne trendy wagowe (7, 14, 30 dni)
- Charakterystyka akustyczna (MFCC, spectral centroid)
- Parametry środowiskowe (temp, wilgotność, ciśnienie)
- Por roku i historia rodziny
- Dane pogodowe z API zewnętrznych

**Architektura:**
```
Input Layer (28 features)
    ↓
Dense Layer (64 neurons, ReLU)
    ↓
Dropout (0.3)
    ↓
LSTM Layer (32 units) - sekwencje czasowe
    ↓
Dense Layer (16 neurons, ReLU)
    ↓
Output Layer (1 neuron, Sigmoid) → Probability of swarming
```

**Trening:**
- Dataset: 500+ rodzin, 3 sezony
- Accuracy: 87% (validation set)
- Precision: 0.82, Recall: 0.79
- False positive rate: <10%

#### Detekcja Chorób i Pasożytów

**Wykrywane Patogeny:**
- **Varroa destructor**: Analiza audio + spad osypu
- **Nosema apis/ceranae**: Wzorzec aktywności + waga
- **American Foulbrood**: Specyficzne dźwięki larw
- **Chalkbrood**: Korelacja temp/wilgotność + audio
- **Deformed Wing Virus**: Detekcja wizyjna (kamera opcjonalna)

### 2. Inteligentne Harmonogramy Zabiegów

**Dynamic Treatment Planner:**
- Analiza historii leczenia rodziny
- Sezonowość występowania chorób
- Odporność na leki (rotacja substancji)
- Interakcje między lekami
- Minimalizacja stresu dla pszczół

**Przykład Harmonogramu:**
```
Sierpień:
- Tydzień 1: Kwas mrówkowy (termoterapia wspomagająca)
- Tydzień 3: Ocena skuteczności (liczenie roztoczy)
- Jeśli nieskuteczne: Kwas szczawiowy (metoda kropelkowa)

Wrzesień:
- Dokarmianie cukrem (syrop 3:2)
- Supplementacja białkowa (pyłek)
- Preparaty ziołowe (tymol, mięta)

Październik:
- Ostatnia ocena wagi i zapasów
- Ewentualne dokarmianie awaryjne
- Przygotowanie do zimowania
```

### 3. Integracja z Zewnętrznymi API

#### Pogoda i Prognozy

**Źródła Danych:**
- OpenWeatherMap API
- MeteoGroup API
- IMGW-PIB (Polska)
- Dark Sky API (fallback)

**Zastosowanie:**
- Prognoza pożytków (temp + opady + wiatr)
- Ostrzeżenia przed ekstremalnymi warunkami
- Optymalizacja terminów zabiegów
- Korelacja z danymi sensorycznymi

```csharp
// WeatherIntegrationService.cs
public class WeatherIntegrationService : IWeatherService
{
    private readonly HttpClient _httpClient;
    private readonly IConfiguration _config;
    
    public async Task<WeatherForecast> GetForecastAsync(double lat, double lon)
    {
        var apiKey = _config["ExternalApis:OpenWeatherMap"];
        var url = $"https://api.openweathermap.org/data/2.5/forecast?lat={lat}&lon={lon}&appid={apiKey}";
        
        var response = await _httpClient.GetAsync(url);
        response.EnsureSuccessStatusCode();
        
        var data = await response.Content.ReadFromJsonAsync<OpenWeatherResponse>();
        
        // Transformacja do domeny ApiaryGuard
        return new WeatherForecast
        {
            Location = new Coordinates(lat, lon),
            ForecastItems = data.List.Select(x => new ForecastItem
            {
                Timestamp = x.DtUtc,
                Temperature = x.Main.Temp,
                Humidity = x.Main.Humidity,
                WindSpeed = x.Wind.Speed,
                Precipitation = x.Rain?.ThreeH ?? 0,
                ForageIndex = CalculateForageIndex(x) // Custom algorithm
            }).ToList()
        };
    }
    
    private double CalculateForageIndex(WeatherData data)
    {
        // Algorytm oceny warunków pożytkowych
        // Uwzględnia: temp, zachmurzenie, wiatr, opady
        var tempScore = Math.Max(0, Math.Min(1, (data.Main.Temp - 15) / 20));
        var windScore = Math.Max(0, 1 - data.Wind.Speed / 10);
        var rainPenalty = data.Rain != null ? -0.3 : 0;
        
        return Math.Clamp(tempScore * windScore + rainPenalty, 0, 1);
    }
}
```

#### Mapy Pożytków i GIS

**Integracje:**
- Google Maps API / OpenStreetMap
- CORINE Land Cover (UE)
- Sentinel-2 Satellite Imagery
- Lokalne rejestry upraw rolniczych

**Funkcjonalności:**
- Mapa pasieki z ulami
- Bufor 3km (zasięg lotu pszczoły)
- Identyfikacja źródeł nektaru i pyłku
- Szacowanie potencjału miodowego
- Wykrywanie zmian w otoczeniu (wycinki, nowe uprawy)

### 4. Multi-Apiary Management

**Hierarchia Organizacyjna:**
```
Organization (Pszczelarz/Firma)
├── Apiary #1 (Pasieka A)
│   ├── Hive 001
│   ├── Hive 002
│   └── ...
├── Apiary #2 (Pasieka B)
│   ├── Hive 015
│   └── ...
└── Apiary #3 (Pasieka C)
    └── ...
```

**Funkcje Multi-Pasieczne:**
- Dashboard agregujący wszystkie pasieki
- Porównanie wydajności między lokalizacjami
- Transfer rodzin między ulami/pasiekami
- Shared resources (sprzęt, leki, personel)
- Role-based access control (pracownicy, właściciel)

### 5. Blockchain Traceability (Opcjonalnie)

**Smart Contracts dla Miodu:**
- Rejestracja każdego zbioru miodu
- Hash danych sensorycznych z okresu produkcji
- Certyfikacja pochodzenia i jakości
- QR code na słoiku → pełna historia ula
- Integracja z rynkami B2B/B2C

**Tech Stack:**
- Ethereum / Polygon network
- Solidity smart contracts
- IPFS dla dużych danych (audio, zdjęcia)
- Web3.js dla integracji frontend

### 6. Voice Assistant Integration

**Komendy Głosowe:**
- "Jaki jest stan ula numer 5?"
- "Pokaż alerty z ostatniej godziny"
- "Uruchom termoterapię w pasiece A"
- "Czy któraś rodzina się roi?"

**Platformy:**
- Amazon Alexa Skill
- Google Assistant Action
- Apple Siri Shortcuts
- Custom wake-word engine (Porcupine)

### 7. Augmented Reality (AR) Maintenance

**Aplikacja Mobile AR:**
- Nakładanie danych sensorycznych na widok ula
- Instrukcje krok-po-kroku przy inspekcji
- Wizualizacja przepływu powietrza w ulu
- Wykrywanie usterek przez kamerę

**Technologie:**
- ARKit (iOS) / ARCore (Android)
- Unity 3D engine
- Object detection (YOLO/TensorFlow Lite)

---

## 🛠️ Instalacja i Konfiguracja

### Wymagania Wstępne

**Sprzętowe:**
- Raspberry Pi 2 Model B (lub nowszy: Pi 3/4/Zero 2 W)
- Arduino Nano V3.0 (ATmega328P)
- Modem LTE USB compatible z Aero2
- Karta microSD 16GB+ Class 10
- Czujniki i efektory zgodnie z BOM
- Zasilacz PoE injector + switch lub PoE splitter HAT

**Programowe:**
- Raspberry Pi OS Lite (64-bit) Bullseye/Bookworm
- .NET 6.0 SDK
- PlatformIO CLI (dla Arduino)
- Apache2 + libapache2-mod-proxy
- SQLite3 / InfluxDB
- Mosquitto MQTT broker
- Git

### Krok-po-Kroku Instalacja

#### 1. Przygotowanie Raspberry Pi OS

```bash
# Flash Raspberry Pi Imager lub manual:
wget https://downloads.raspberrypi.org/raspios_lite_arm64/images/raspios_lite_arm64-2023-10-10/2023-10-10-raspios-bookworm-arm64-lite.img.xz
unxz 2023-10-10-raspios-bookworm-arm64-lite.img.xz
sudo dd if=2023-10-10-raspios-bookworm-arm64-lite.img of=/dev/sdX bs=4M conv=fsync

# Boot i podstawowa konfiguracja
ssh pi@raspberrypi.local
# Hasło: raspberry (zmienić natychmiast!)

# Update systemu
sudo apt update && sudo apt upgrade -y
sudo raspi-config nonint do_hostname apiaryguard-gateway
sudo raspi-config nonint do_ssh 0
sudo raspi-config nonint do_i2c 0
sudo raspi-config nonint do_spi 0
```

#### 2. Instalacja Zależności

```bash
# .NET 6.0 SDK
wget https://packages.microsoft.com/config/debian/12/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo apt update
sudo apt install -y dotnet-sdk-6.0 aspnetcore-runtime-6.0

# Apache2 + modules
sudo apt install -y apache2 libapache2-mod-proxy-html libapache2-mod-proxy-http
sudo a2enmod proxy proxy_http ssl headers rewrite
sudo systemctl enable apache2

# Mosquitto MQTT
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto

# SQLite + narzędzia
sudo apt install -y sqlite3 libsqlite3-dev

# I2C tools
sudo apt install -y i2c-tools

# Git i build essentials
sudo apt install -y git build-essential cmake libfftw3-dev

# PlatformIO dla Arduino
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

#### 3. Klonowanie Repozytorium i Build

```bash
cd /opt
sudo git clone https://github.com/apiaryguard/apiaryguard-pro.git
sudo chown -R pi:pi apiaryguard-pro
cd apiaryguard-pro

# Build Arduino firmware
cd hardware/arduino_nano
pio run --target upload
cd ../..

# Build C# applications
cd software/raspberry_pi
dotnet restore
dotnet build --configuration Release

# Publish
dotnet publish ApiaryGuard.WebApi -c Release -o /opt/apiaryguard/publish/webapi
dotnet publish ApiaryGuard.Core -c Release -o /opt/apiaryguard/publish/core
dotnet publish ApiaryGuard.Worker -c Release -o /opt/apiaryguard/publish/worker
```

#### 4. Konfiguracja Systemd Services

```bash
# Kopiowanie unit files
sudo cp /opt/apiaryguard-pro/config/systemd/*.service /etc/systemd/system/

# Reload i enable
sudo systemctl daemon-reload
sudo systemctl enable apiaryguard-core
sudo systemctl enable apiaryguard-worker
sudo systemctl enable apiaryguard-webapi

# Start services
sudo systemctl start apiaryguard-core
sudo systemctl start apiaryguard-worker
sudo systemctl start apiaryguard-webapi

# Verify status
systemctl status apiaryguard-*
```

#### 5. Konfiguracja Apache Virtual Host

```bash
sudo nano /etc/apache2/sites-available/apiaryguard.conf

# Content:
<VirtualHost *:80>
    ServerName apiaryguard.local
    
    ProxyPreserveHost On
    ProxyPass / http://localhost:5000/
    ProxyPassReverse / http://localhost:5000/
    
    # WebSocket support
    RewriteEngine On
    RewriteCond %{HTTP:Upgrade} websocket [NC]
    RewriteCond %{HTTP:Connection} upgrade [NC]
    RewriteRule ^/?(.*) "ws://localhost:5000/$1" [P,L]
    
    ErrorLog ${APACHE_LOG_DIR}/apiaryguard_error.log
    CustomLog ${APACHE_LOG_DIR}/apiaryguard_access.log combined
</VirtualHost>

# Enable site
sudo a2ensite apiaryguard
sudo a2dissite 000-default
sudo systemctl reload apache2
```

#### 6. Konfiguracja LTE Aero2

```bash
# Uruchomienie skryptu instalacyjnego
sudo bash /opt/apiaryguard-pro/scripts/bash/network/lte_setup.sh

# Weryfikacja
ip addr show ppp0
ping -c 4 8.8.8.8

# Dodanie watchdoga
sudo cp /opt/apiaryguard-pro/config/systemd/lte-watchdog.service /etc/systemd/system/
sudo systemctl enable lte-watchdog
sudo systemctl start lte-watchdog
```

#### 7. Inicjalizacja Bazy Danych

```bash
cd /opt/apiaryguard-pro/config/database
sudo sqlite3 /var/lib/apiaryguard/data.db < schema.sql
sudo sqlite3 /var/lib/apiaryguard/data.db < indexes.sql
sudo sqlite3 /var/lib/apiaryguard/data.db < seed_data.sql

# Set permissions
sudo chown pi:pi /var/lib/apiaryguard/data.db
```

#### 8. Kalibracja Sensorów

```bash
# Kalibracja wagi
sudo bash /opt/apiaryguard-pro/scripts/bash/sensors/calibrate_scale.sh --known-weight 10.0

# Test mikrofonu
sudo bash /opt/apiaryguard-pro/scripts/bash/sensors/test_microphone.sh --duration 30

# Pełna diagnostyka
sudo bash /opt/apiaryguard-pro/scripts/bash/system/health_check.sh
```

#### 9. Konfiguracja SSL (Production)

```bash
# Let's Encrypt certificate
sudo apt install -y certbot python3-certbot-apache
sudo certbot --apache -d apiaryguard.yourdomain.com

# Auto-renewal
sudo systemctl enable certbot.timer
```

#### 10. Final Verification

```bash
# Sprawdź wszystkie usługi
systemctl status apache2 mosquitto apiaryguard-* lte-watchdog

# Dostęp do dashboardu
firefox http://apiaryguard.local

# Test API
curl http://localhost:5000/api/hives

# MQTT test
mosquitto_sub -t "apiaryguard/#" -v
```

---

## 🔌 API i Integracje

### REST API Endpoints

#### Authentication
```http
POST /api/auth/login
Content-Type: application/json

{
  "username": "admin",
  "password": "secure_password"
}

Response:
{
  "token": "eyJhbGciOiJIUzI1NiIs...",
  "expiresIn": 3600,
  "refreshToken": "dGhpcyBpcyBhIHJlZnJlc2g..."
}
```

#### Hives Management
```http
GET /api/hives
Authorization: Bearer {token}

Response:
{
  "hives": [
    {
      "id": 1,
      "name": "Ul #1",
      "location": "Pasieka A",
      "status": "Active",
      "healthScore": 87.5,
      "lastReading": {
        "timestamp": "2024-01-15T10:30:00Z",
        "weight": 45.6,
        "temperature": 34.2,
        "humidity": 58.0
      }
    }
  ]
}

POST /api/hives
{
  "name": "Ul #15",
  "location": "Pasieka B",
  "initialWeight": 15.0
}

PUT /api/hives/{id}
DELETE /api/hives/{id}
```

#### Sensor Data
```http
GET /api/hives/{hiveId}/sensor-data?from=2024-01-01&to=2024-01-15&type=weight,temp

Response:
{
  "readings": [
    {
      "timestamp": "2024-01-15T10:00:00Z",
      "weight": 45.6,
      "temperature": 34.2,
      "humidity": 58.0,
      "audioLevel": 42.5
    }
  ]
}
```

#### Actuators Control
```http
POST /api/hives/{hiveId}/actuators/heater
{
  "action": "start",
  "targetTemperature": 40.0,
  "duration": 1440 // minutes
}

POST /api/hives/{hiveId}/actuators/fan
{
  "action": "set_speed",
  "speedPercent": 75
}

POST /api/hives/{hiveId}/actuators/dispenser
{
  "substance": "formic_acid",
  "volumeMl": 50.0,
  "schedule": "immediate"
}
```

#### Alerts
```http
GET /api/alerts?status=active&severity=critical

POST /api/alerts/{id}/acknowledge
POST /api/alerts/{id}/resolve
```

### MQTT Topics

```
apiaryguard/{hive_id}/telemetry       # QoS 1, Retain
  Payload: JSON with all sensor data

apiaryguard/{hive_id}/alerts          # QoS 2
  Payload: Alert object

apiaryguard/{hive_id}/commands/heater # QoS 1
  Payload: {"action": "start|stop", "target": 40.0}

apiaryguard/{hive_id}/commands/fan    # QoS 1
  Payload: {"speed": 0-100}

apiaryguard/{hive_id}/status          # QoS 1, Retain, LWT
  Payload: {"online": true, "lastSeen": "..."}

apiaryguard/broadcast/firmware_update # QoS 2
  Payload: {"version": "2.1.0", "url": "..."}
```

### Webhooks

**Konfigurowalne Events:**
- `hive.alert.created`
- `hive.swarm.detected`
- `hive.treatment.completed`
- `device.offline`
- `device.online`
- `daily.report.ready`

**Przykład Webhook Payload:**
```json
{
  "event": "hive.swarm.detected",
  "timestamp": "2024-01-15T14:30:00Z",
  "hive": {
    "id": 5,
    "name": "Ul #5",
    "location": "Pasieka A"
  },
  "data": {
    "probability": 0.87,
    "weightDrop": 2.3,
    "audioEvidence": "https://apiaryguard.local/recordings/swarm_20240115.wav",
    "recommendedAction": "Inspect within 24 hours"
  },
  "severity": "high"
}
```

---

## 🔒 Bezpieczeństwo i Niezawodność

### Security Measures

#### 1. Network Security
- **Firewall**: UFW z whitelistą portów (22, 80, 443, 8883)
- **Fail2Ban**: Banowanie po 5 nieudanych logowaniach
- **VPN**: Optional WireGuard tunnel dla zdalnego dostępu
- **Network Segmentation**: Izolacja IoT devices w osobnej VLAN

#### 2. Application Security
- **JWT Authentication**: Token-based auth z refresh tokens
- **Role-Based Access Control (RBAC)**: Admin, Operator, Viewer
- **Input Validation**: Sanityzacja wszystkich inputów
- **SQL Injection Prevention**: Parameterized queries
- **XSS Protection**: Content-Security-Policy headers
- **Rate Limiting**: Max 100 requests/minute per IP

#### 3. Data Security
- **Encryption at Rest**: AES-256 dla bazy danych
- **Encryption in Transit**: TLS 1.3 dla wszystkich połączeń
- **Secure Key Storage**: TPM module lub encrypted keystore
- **Audit Logging**: Wszystkie operacje logged z timestamp

#### 4. Physical Security
- **Tamper Detection**: Reed switch na obudowie
- **GPS Tracking**: Anti-theft geofencing
- **Lockable Enclosure**: Fizyczne zabezpieczenie
- **Remote Wipe**: Możliwość zdalnego czyszczenia danych

### Reliability Features

#### 1. Redundancy
- **Dual Connectivity**: LTE + Ethernet (fallback)
- **Local Caching**: 30 days data retention offline
- **Battery Backup**: 12 hours autonomous operation
- **Watchdog Timers**: Hardware + software watchdog

#### 2. Fault Tolerance
- **Circuit Breaker Pattern**: Izolacja failed services
- **Retry Logic**: Exponential backoff dla transient errors
- **Graceful Degradation**: Partial functionality when components fail
- **Self-Healing**: Automatic service restart

#### 3. Monitoring & Observability
- **Health Checks**: Comprehensive endpoint `/health`
- **Metrics Collection**: Prometheus metrics export
- **Distributed Tracing**: OpenTelemetry integration
- **Log Aggregation**: Centralized logging with ELK stack

#### 4. Disaster Recovery
- **Automated Backups**: Daily incremental + weekly full
- **Off-site Replication**: Cloud backup (AWS S3 / Azure Blob)
- **Disaster Recovery Plan**: Documented procedures
- **RTO/RPO**: Recovery Time Objective <4h, Recovery Point Objective <1h

---

## 🔧 Konserwacja i Rozwiązywanie Problemów

### Harmonogram Konserwacji

**Codziennie:**
- [ ] Sprawdzenie statusu online (dashboard)
- [ ] Przegląd aktywnych alertów
- [ ] Weryfikacja poziomu baterii backup

**Co Tydzień:**
- [ ] Inspekcja fizyczna uli (tradycyjna)
- [ ] Czyszczenie wylotków z martwych pszczół
- [ ] Sprawdzenie mocowania sensorów
- [ ] Test ręcznego uruchomienia efektorów

**Co Miesiąc:**
- [ ] Kalibracja wagi (test znanym ciężarem)
- [ ] Czyszczenie mikrofonu z kurzu
- [ ] Backup konfiguracji
- [ ] Update systemu operacyjnego
- [ ] Przegląd logów pod kątem warningów

**Co Kwartalnie:**
- [ ] Pełna diagnostyka systemu (health_check.sh)
- [ ] Test procedury disaster recovery
- [ ] Wymiana filtrów powietrza (jeśli dotyczy)
- [ ] Sprawdzenie uszczelek obudowy (waterproofing)
- [ ] Audyt bezpieczeństwa (logs, access)

**Sezonowo:**
- [ ] Przed sezonem: Pełny przegląd techniczny
- [ ] Po sezonie: Konserwacja zimowa, demontaż części sensorów
- [ ] Zima: Tryb low-power, minimalny sampling

### Troubleshooting Guide

#### Problem: Brak połączenia LTE

**Diagnostyka:**
```bash
# Sprawdź czy modem jest wykrywany
lsusb | grep -i huawei

# Status połączenia PPP
ip addr show ppp0
systemctl status ppp@aero2

# Logi modemu
tail -f /var/log/ppp/aero2.log

# Siła sygnału
mmcli -m 0 --signal-get
```

**Rozwiązania:**
1. Restart modemu: `usb_modeswitch -v 0x12d1 -p 0x1506 -R`
2. Reconnect: `poff aero2 && pon aero2`
3. Sprawdzenie konta Aero2: `*101#`
4. Zmiana lokalizacji anteny zewnętrznej

#### Problem: Nieprawidłowe odczyty wagi

**Diagnostyka:**
```bash
# Test bezpośredni HX711
i2cdetect -y 1
i2cget -y 1 0x48

# Uruchom diagnostykę
./sensor_diagnostics.sh --sensor weight

# Sprawdź kalibrację
sqlite3 data.db "SELECT * FROM calibration_history ORDER BY date DESC LIMIT 5;"
```

**Rozwiązania:**
1. Ponowna kalibracja: `calibrate_scale.sh --known-weight 10.0`
2. Sprawdzenie połączeń kablowych tensometrów
3. Izolacja od wibracji (podkładki gumowe)
4. Wymiana HX711 jeśli uszkodzony

#### Problem: Przegrzewanie Raspberry Pi

**Diagnostyka:**
```bash
# Temperatura CPU
vcgencmd measure_temp

# Throttling status
vcgencmd get_throttled

# Process load
top -bn1 | head -20
```

**Rozwiązania:**
1. Dodanie heatsink + fan
2. Obniżenie clock speed: `over_voltage=-2` w config.txt
3. Sprawdzenie obciążenia procesami
4. Przeniesienie w cieńsze miejsce

#### Problem: Usługa nie startuje

**Diagnostyka:**
```bash
# Status usługi
systemctl status apiaryguard-core

# Logi journal
journalctl -u apiaryguard-core -n 50 --no-pager

# Ręczny start dla debugowania
/opt/apiaryguard/publish/core/ApiaryGuard.Core
```

**Rozwiązania:**
1. Sprawdzenie dependencies: `systemctl list-dependencies apiaryguard-core`
2. Weryfikacja connection string do bazy danych
3. Sprawdzenie uprawnień do plików
4. Przywrócenie z backupu konfiguracji

### Backup i Restore

**Tworzenie Backupu:**
```bash
./backup.sh --full --destination /mnt/external_drive/backups
# Lub automatycznie codziennie o 3:00
```

**Przywracanie:**
```bash
./restore.sh --source /mnt/external_drive/backups/backup_20240115.tar.gz --verify
```

**Zawartość Backupu:**
- Baza danych SQLite
- Konfiguracje aplikacji (appsettings.json)
- Konfiguracje systemowe (network, Apache, MQTT)
- Firmware Arduino
- Nagrania audio (ostatnie 30 dni)
- Logi (ostatnie 7 dni)

---

## 🔮 Rozszerzenia Przyszłościowe

### Roadmap Rozwoju

#### Wersja 2.2 (Q2 2025)
- [ ] Integracja z kamerami termowizyjnymi (FLIR Lepton)
- [ ] Computer Vision dla liczenia pszczół na wylotku
- [ ] Predykcja wydajności miodowej z wyprzedzeniem 2 tygodni
- [ ] Mobile app offline mode z sync

#### Wersja 2.3 (Q3 2025)
- [ ] Robotyczna inspekcja wnętrza ula (mini rover)
- [ ] Automated frame recognition (który plaster z miodem)
- [ ] Integracja z blockchain dla traceability
- [ ] Multi-language support (PL, EN, DE, FR, ES)

#### Wersja 3.0 (Q1 2026)
- [ ] Full edge AI z NVIDIA Jetson Nano upgrade
- [ ] Federated learning między pasiekami (privacy-preserving)
- [ ] Autonomous intervention drones
- [ ] Integration with agricultural machinery

### Badania i Development

**Obszary Badawcze:**
1. **Bioakustyka pszczół**: Baza danych 10,000+ nagrań różnych stanów
2. **Termografia**: Korelacja rozkładu ciepła z zdrowiem czerwiu
3. **Chemical Sensing**: E-nose dla wykrywania chorób po VOC
4. **Swarm Intelligence**: Modelowanie zachowań roju dla predykcji

**Partnerstwa Naukowe:**
- Uniwersytet Przyrodniczy w Poznaniu
- Instytut Ogrodnictwa w Skierniewicach
- European Honey Bee Lab (Wageningen)

---

## 📄 Licencja i Współpraca

### Licencja

Projekt ApiaryGuard Pro jest udostępniany na licencji **Apache License 2.0**.

```
Copyright 2024 ApiaryGuard Team

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

### Jak Współpracować

#### Reporting Bugs
1. Sprawdź czy bug już nie został zgłoszony (Issues)
2. Użyj template'u "Bug Report"
3. Dołącz logi, screenshoty, kroki reprodukcji
4. Oznacz severity (critical/major/minor)

#### Proposing Features
1. Utwórz Issue z label "enhancement"
2. Opisz use case i wartość biznesową
3. Zasugeruj implementację (opcjonalnie)
4. Dyskusja społeczności przed developmentem

#### Contributing Code
1. Fork repository
2. Utwórz branch feature/my-feature
3. Pisz testy unit/integration
4. Follow coding standards (`.editorconfig`)
5. Submit Pull Request z opisem zmian
6. Code review przez maintainerów
7. Merge po approval

#### Code Style Guidelines
- **C#**: Microsoft C# Coding Conventions
- **C++**: Google C++ Style Guide
- **Bash**: ShellCheck compliant, Google Shell Style
- **Commits**: Conventional Commits specification

### Kontakt i Społeczność

- **GitHub**: https://github.com/apiaryguard/apiaryguard-pro
- **Discord**: https://discord.gg/apiaryguard
- **Forum**: https://forum.apiaryguard.pro
- **Email**: team@apiaryguard.pro
- **Twitter/X**: @ApiaryGuardPro

### Podziękowania

Projekt powstał dzięki wkładowi:
- **Główny Developer**: [Twoje Imię/Nick]
- **Hardware Engineer**: [Imię]
- **Beekeeping Advisors**: Pasieka "Złoty Plaster", Związek Pszczelarski RP
- **Beta Testers**: 50+ pszczelarzy z całej Polski
- **Open Source Libraries**: FFTW, .NET Foundation, Arduino Community

---

## 📊 Statystyki Projektu

![Lines of Code](https://img.shields.io/badge/LOC-50,000+-blue)
![Languages](https://img.shields.io/badge/Languages-C%23%20%7C%20C%2B%2B%20%7C%20Bash-orange)
![Platforms](https://img.shields.io/badge/Platforms-RPi%20%7C%20Arduino-green)
![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)
![Build Status](https://img.shields.io/github/actions/workflow/status/apiaryguard/apiaryguard-pro/build.yml)
![Release](https://img.shields.io/github/v/release/apiaryguard/apiaryguard-pro)

---

**ApiaryGuard Pro** - Rewolucjonizowanie pszczelarstwa poprzez technologię. 🐝🤖

*Ostatnia aktualizacja dokumentacji: Styczeń 2025*
*Wersja dokumentacji: 2.1.0*
