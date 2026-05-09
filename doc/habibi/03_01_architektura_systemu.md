# 3.1. Architektura Systemu ApiaryGuard Pro

## 3.1.1. Założenia Projektowe i Wymagania Systemowe

### 3.1.1.1. Kontekst Aplikacyjny i Scenariusze Użycia

System ApiaryGuard Pro został zaprojektowany jako kompleksowa platforma telemetryczna klasy enterprise dla pszczelarstwa precyzyjnego, adresująca trzy główne scenariusze aplikacyjne:

**Scenariusz A: Pasieka Hobbystyczna (5–20 uli)**
- Użytkownik: Pszczelarz amator zarządzający 1–3 lokalizacjami pasiecznymi
- Wymagania: Niski koszt wejścia (<€500), instalacja plug-and-play, monitoring podstawowych parametrów (waga, temperatura, wilgotność), alerty SMS/email, interfejs mobilny
- Ograniczenia: Brak infrastruktury sieciowej (Ethernet), zasilanie bateryjne/solarne, ograniczone kompetencje techniczne użytkownika

**Scenariusz B: Pasieka Komercyjna Średnia (20–200 uli)**
- Użytkownik: Profesjonalny pszczelarz lub spółdzielnia pszczelarska z 3–10 lokalizacjami
- Wymagania: Skalowalność do 200+ uli, zaawansowana analityka (predykcja rojenia, detekcja chorób), integracja z systemami ERP, raportowanie miesięczne/kwartalne, zdalna konfiguracja
- Ograniczenia: Częsta rotacja lokalizacji (nomadyzm pszczelarski), zmienne warunki łączności, wymóg redundancji danych

**Scenariusz C: Badania Naukowe i Monitoringu Środowiska (200–1000+ uli)**
- Użytkownik: Instytucja badawcza, agencja rządowa, organizacja pozarządowa
- Wymagania: Wysoka częstotliwość próbkowania (≥10 Hz), eksport surowych danych (CSV/JSON), API dla integracji z platformami badawczymi (BeeInformed, HoneybeeNet), długoterminowa stabilność (≥5 lat deploymentu)
- Ograniczenia: Budget ograniczony grantami, wymóg open-source, publikacja danych w repozytoriach publicznych

Architektura systemu ApiaryGuard Pro została opracowana jako modularna platforma zdolna do elastycznej adaptacji do wszystkich trzech scenariuszy poprzez skalowalność poziomą (horizontal scaling) i konfigurowalność funkcjonalną (feature flags).

### 3.1.1.2. Wymagania Funkcjonalne

Poniższa tabela prezentuje szczegółową specyfikację wymagań funkcjonalnych systemu z podziałem na kategorie:

| ID | Kategoria | Wymaganie | Priorytet | Kryterium Akceptacji |
|----|-----------|-----------|-----------|---------------------|
| FR-01 | Akwizycja Danych | Pobieranie danych z 7 modalności sensorycznych | Critical | Wszystkie sensory dostarczają dane z częstotliwością ≥1 Hz |
| FR-02 | Akwizycja Danych | Synchronizacja czasowa z dokładnością ≤100 ms | High | Timestampy wszystkich węzłów zsynchronizowane przez NTP |
| FR-03 | Przetwarzanie | Filtracja cyfrowa sygnałów (low-pass, high-pass, band-pass) | High | Redukcja szumów o ≥20 dB bez utraty sygnału użytecznego |
| FR-04 | Przetwarzanie | Obliczanie metryk wyliczonych w czasie rzeczywistym | High | 338+ metryk aktualizowanych co 60 sekund |
| FR-05 | Komunikacja | Transmisja HTTP POST do gateway'a co 60 sekund | Critical | Success rate ≥99% w warunkach nominalnych |
| FR-06 | Komunikacja | Store-and-forward przy braku łączności | High | Dane buforowane ≥90 dni bez utraty |
| FR-07 | Bezpieczeństwo | Secure boot z weryfikacją podpisu RSA-2048 | Critical | Firmware nieuruchamiający się przy signature mismatch |
| FR-08 | Bezpieczeństwo | Szyfrowanie danych AES-256-GCM przed transmisją | High | Przechwycone pakiety nieczytelne bez klucza |
| FR-09 | Analityka | Predykcja rojenia z wyprzedzeniem 7–14 dni | Critical | Accuracy ≥90%, Precision ≥88%, Recall ≥87% |
| FR-10 | Analityka | Detekcja chorób (Varroa, Nosema, AFB) | High | Accuracy ≥85% dla każdej patologii |
| FR-11 | Interfejs | Dashboard web responsive (mobile-first) | Medium | Load time <3 s na urządzeniach 3G |
| FR-12 | Interfejs | Agent AI z natural language understanding | Medium | Intent recognition accuracy ≥92% |
| FR-13 | Alerty | Powiadomienia multi-channel (email, SMS, push, webhook) | High | Delivery latency <30 sekund od detekcji |
| FR-14 | Alerty | Eskalacja alertów według polityki on-call | Medium | Eskalacja po 30 minut bez acknowledge |
| FR-15 | Integracja | REST API z dokumentacją Swagger/OpenAPI | Medium | 100% endpointów pokrytych testami integration |
| FR-16 | Integracja | Eksport danych do formatów CSV, JSON, Parquet | Low | Export 1 roku danych w <60 sekund |
| FR-17 | Zarządzanie | OTA (Over-The-Air) updates firmware | High | Update成功率 ≥98% z automatic rollback |
| FR-18 | Zarządzanie | Konfiguracja zdalna parametrów sampling | Medium | Zmiana生效 w <5 minut bez restartu |
| FR-19 | Diagnostyka | Auto-diagnostyka sensorów z detekcją fault | High | Fault detection w <60 sekund od wystąpienia |
| FR-20 | Diagnostyka | Logowanie zdarzeń z rotacją i kompresją | Medium | Logs przechowywane ≥30 dni lokalnie |

### 3.1.1.3. Wymagania Niefunkcjonalne

Wymagania niefunkcjonalne definiują atrybuty jakościowe systemu krytyczne dla jego przyjęcia rynkowego i długoterminowej niezawodności:

**NFR-01: Niezawodność (Reliability)**
- MTBF (Mean Time Between Failures) ≥50 000 godzin (≈5.7 lat) dla węzła sensorycznego
- MTTR (Mean Time To Recovery) ≤5 minut dla automatycznego recovery po awarii software'owej
- Availability ≥99.5% w skali roku (dopuszczalny downtime ≤44 godziny/rok)
- Odporność na pojedyncze punkty awarii (single point of failure elimination)

**NFR-02: Wytrzymałość Środowiskowa (Environmental Durability)**
- Zakres temperatur operacyjnych: -30°C do +60°C (extended industrial grade)
- Zakres wilgotności względnej: 0–100% RH (condensing)
- Odporność na opady: IP68 (zanurzenie 1.5 m przez 30 minut)
- Odporność na uderzenia mechaniczne: IK08 (5 J)
- Odporność na promieniowanie UV: ≥5 lat ekspozycji bez degradacji tworzywa
- Odporność na korozję chemiczną (kwasy organiczne, propolis, środki lecznicze)

**NFR-03: Efektywność Energetyczna (Power Efficiency)**
- Średnie zużycie mocy węzła sensorycznego: ≤500 mW (active mode)
- Zużycie w trybie deep sleep: ≤50 µW
- Autonomia bateryjna: ≥7 dni bez ładowania (worst-case: ciągłe opady, noc polarna)
- Czas ładowania 0–100%: ≤6 godzin w pełnym nasłonecznieniu (1000 W/m²)
- Sprawność regulatora MPPT: ≥95%

**NFR-04: Wydajność (Performance)**
- Latencja end-to-end (sensor → dashboard): ≤5 sekund dla alertów critical
- Throughput agregacji gateway: ≥50 requestów/sekundę (skalowalne do 200 RPS)
- Czas ładowania dashboardu: ≤3 sekund dla widoku multi-hive (50 uli)
- Czas inferencji modelu ML: ≤200 ms na prediction
- Czas odpowiedzi agenta AI: ≤2 sekundy dla zapytań prostych, ≤8 sekund dla złożonych z RAG

**NFR-05: Bezpieczeństwo (Security)**
- Compliance z GDPR dla danych osobowych użytkowników
- Szyfrowanie danych w spoczynku (AES-256) i w tranzycie (TLS 1.3)
- Autentyfikacja dwuskładnikowa (2FA) dla konta administratora
- Regularne audyty bezpieczeństwa (penetration testing) co 6 miesięcy
- Vulnerability disclosure program z bug bounty

**NFR-06: Skalowalność (Scalability)**
- Skalowalność pozioma: Obsługa od 1 do 1000+ uli z liniowym wzrostem kosztów infrastruktury
- Skalowalność geograficzna: Deployment w wielu strefach czasowych z lokalizacją danych (data residency)
- Skalowalność użytkowników: ≥100 jednoczesnych sesji użytkowników na instancję gateway

**NFR-07: Utrzymywalność (Maintainability)**
- Czas deploymentu nowej wersji firmware: ≤15 minut dla pasieki 50 uli
- Coverage testów jednostkowych: ≥85% kodu źródłowego
- Technical debt ratio: ≤5% (mierzone przez SonarQube)
- Dokumentacja techniczna: 100% API endpointów, 100% schematów elektrycznych

**NFR-08: Przyjazność Użycia (Usability)**
- Time-to-first-data: ≤30 minut od unpacking box do pierwszych danych na dashboardzie
- SUS (System Usability Score): ≥80 punktów w testach z użytkownikami końcowymi
- Liczba kliknięć do kluczowych funkcji: ≤3 (alarm acknowledgment, hive detail view, report export)
- Dostępność: WCAG 2.1 AA compliance dla interfejsu web

---

## 3.1.2. Architektura Sprzętowa (Hardware Architecture)

### 3.1.2.1. Topologia Sieci Sensorycznej

System ApiaryGuard Pro wykorzystuje topologię gwiazdy (star topology) z centralnym węzłem gateway komunikującym się z periferyjnymi węzłami sensorycznymi (hive nodes). Topologia ta została wybrana ze względu na:

**Zalety:**
- **Prostota zarządzania**: Każdy węzeł komunikuje się wyłącznie z gateway'em, eliminując złożoność routingu mesh
- **Determinizm czasowy**: Brak kolizji międzywęzłowych, predictable latency dla każdego node
- **Łatwość diagnostyki**: Izolacja faultów – awaria jednego węzła nie wpływa na pozostałe
- **Skalowalność**: Dodanie nowego ula = dodanie nowego węzła bez rekonfiguracji sieci

**Wyzwania:**
- **Zasięg ograniczony**: Pojedynczy gateway obsługuje węzły w promieniu ≤100 m (line-of-sight)
- **Single point of failure**: Awaria gateway'a paraliżuje całą pasiekę (mitigowane przez redundantne gateway'e w opcji enterprise)
- **Konsumpcja energii**: Każda transmisja musi dotrzeć bezpośrednio do gateway'a (wyższa moc nadawcza vs. multi-hop)

Dla pasiek o powierzchni przekraczającej 100 m promienia, system wspiera topologię rozszerzoną z wieloma gateway'ami połączonymi przez Ethernet backhaul, gdzie każdy gateway zarządza podsiecią 50 uli, a dane są agregowane na poziomie cloud.

### 3.1.2.2. Węzeł Sensoryczny Ula (Hive Node)

Każdy węzeł sensoryczny jest autonomicznym urządzeniem embedded zintegrowanym z obudową odporną na warunki środowiskowe. Specyfikacja sprzętowa:

**Jednostka Centralna (Compute Module):**
- **Mikrokontroler**: Raspberry Pi Pico (RP2040)
  - Architektura: Dual-core ARM Cortex-M0+ @133 MHz
  - Pamięć RAM: 264 kB SRAM
  - Pamięć Flash: 2 MB QSPI external (XIP)
  - GPIO: 30 pinów z funkcjami Programmable I/O (PIO)
  - Interfejsy: 2×UART, 2×SPI, 2×I²C, 1×USB 1.1 Device/Host, 3×ADC 12-bit
  - Zasilanie: 1.8–5.5 V input range z LDO regulator 3.3 V
  - Pobór mocy: 0.085 mW/MHz (active), 0.4 mA (sleep), 1.2 µA (deep sleep)

**Uzasadnienie wyboru RP2040:**
1. **Stosunek wydajności do ceny**: $1 USD za MCU z dual-core M0+ jest bezkonkurencyjne na rynku
2. **Programmable I/O (PIO)**: Umożliwia implementację custom protokołów timing-critical (np. driving HX711, reading radar UART) bez obciążania CPU
3. **Brak interpretera Pythona w production**: Firmware kompilowany do native code zapewnia determinizm czasowy
4. **Bogata ekosystem SDK**: Oficjalne Raspberry Pi Pico SDK z przykładami, bibliotekami BSP, wsparciem FreeRTOS
5. **Długoterminowa dostępność**: Cypress/Infineon gwarantuje produkcję do 2035 roku

**Moduł Tensometrii (HX711Metrics):**
- **Przetwornik ADC**: HX711 (24-bit delta-sigma)
- **Wzmocnienie programowalne**: 32×, 64×, 128× (PIN gain selection)
- **Częstotliwość próbkowania**: 10 Hz lub 80 Hz (PIN rate selection)
- **Load cell**: Typ cantilever 200 kg z mostkiem Wheatstone'a
- **Dokładność**: ±5 g po kalibracji 4-punktowej
- **Temperaturowa kompensacja**: Algorytm korekty dryfu termicznego (±0.002% FS/°C)
- **Liczba metryk**: 80 (total_weight, growth_rate, daily_delta, swarm_detection, harvest_event, winter_consumption, forager_activity_idx, etc.)

**Moduł Bioakustyki (AudioMetrics):**
- **Mikrofon**: MEMS omnidirectional I2S (INMP441 lub equivalent)
- **Pasmo przenoszenia**: 20 Hz – 20 kHz (±2 dB)
- **SNR**: ≥65 dB (A-weighted)
- **Próbkowanie**: 44.1 kHz / 16-bit (CD quality)
- **Interfejs**: I2S (Inter-IC Sound) z master clock z RP2040 PIO
- **Przetwarzanie**: FFT 2048 punktów z oknem Hamminga, overlap 50%
- **Liczba metryk**: 47 (dominant_frequency, spectral_centroid, spectral_entropy, zero_crossing_rate, MFCC 1–13, chroma, tonnetz, etc.)

**Moduł Wibrometrii (VibrationMetrics):**
- **Accelerometr**: ADXL345 (3-axis MEMS)
- **Zakres pomiarowy**: ±2g / ±4g / ±8g / ±16g (software selectable)
- **Rozdzielczość**: 13-bit przy ±16g (3.9 mg/LSB)
- **Interfejs**: SPI @10 MHz lub I²C @400 kHz
- **Filtracja**: Low-pass filter programowalny (1600 Hz do 12.5 Hz)
- **Liczba metryk**: 38 (RMS_amplitude, peak_to_peak, frequency_distribution, harmonic_distortion, correlation_XYZ, event_detection)

**Moduł Mikroklimatu (ClimateMetrics):**
- **Sensor główny**: BME280 (Bosch Sensortec)
  - Temperatura: -40°C do +85°C (±0.5°C)
  - Wilgotność: 0–100% RH (±3%)
  - Ciśnienie: 300–1100 hPa (±1 hPa)
  - Interfejs: I²C @400 kHz lub SPI @10 MHz
- **Sensor redundantny**: SHT40 (Sensirion)
  - Temperatura: -40°C do +125°C (±0.2°C)
  - Wilgotność: 0–100% RH (±1.8%)
  - Interfejs: I²C @1 MHz (high-speed mode)
- **Lokalizacja**: Wewnątrz ula, przestrzeń międzyramkowa (bezpośredni kontakt z czerwiem)
- **Liczba metryk**: 24 (internal_T, internal_RH, internal_pressure, external_T, dew_point, vapor_pressure_deficit, thermal_mass_coefficient, etc.)

**Moduł Jakości Powietrza (AirQualityMetrics):**
- **CO₂ Sensor**: SCD41 (Senseair)
  - Zakres: 400–5000 ppm
  - Dokładność: ±40 ppm ±5% reading
  - Technologia: Photoacoustic NDIR (Non-Dispersive Infrared)
  - Interfejs: I²C @400 kHz
- **VOC Sensor**: SGP40 (Sensirion)
  - Zakres: 0–500 TVOC index
  - Dokładność: ±15% reading
  - Technologia: MOX (Metal Oxide Semiconductor) z algorytmem compensation
  - Interfejs: I²C @400 kHz
- **NOx Sensor**: SPEC Sensors NO2-B43F
  - Zakres: 0–5 ppm
  - Rozdzielczość: 0.01 ppm
  - Technologia: Electrochemical cell
  - Interfejs: Analog voltage output (ADC RP2040)
- **Liczba metryk**: 18 (CO2_ppm, CO2_index, TVOC_index, NOx_ppm, air_quality_score, ventilation_efficiency, fermentation_detection, pesticide_stress_idx)

**Moduł Natężenia Światła (LuxMetrics):**
- **Sensor**: BH1750FVI (ROHM Semiconductor)
- **Zakres**: 1–65535 lux (16-bit resolution)
- **Dokładność**: ±20%
- **Tryby pomiarowe**: Continuous H-resolution (120 ms), L-resolution (16 ms), One-time H-resolution
- **Interfejs**: I²C @400 kHz
- **Lokalizacja**: Na wylotku ula (external mounting)
- **Liczba metryk**: 12 (ambient_lux, day_length, dawn_dusk_detection, shading_coefficient, solar_radiation_estimate, circadian_rhythm_idx)

**Moduł Radarowy MMWave (RadarMetrics) – Element Kluczowy Innowacji:**
- **Radar**: Custom design FMCW 24 GHz ISM band
- **Konfiguracja**: MIMO 2 Tx × 4 Rx (8 virtual channels)
- **Modulacja**: FMCW (Frequency Modulated Continuous Wave) z chirp duration 200 µs
- **Zasięg**: 0.2–8 m (configurable ROI – Region of Interest)
- **Rozdzielczość odległości**: ≤5 cm (bandwidth 300 MHz)
- **Rozdzielczość prędkości**: ±0.1 m/s (Doppler processing)
- **Kąt widzenia**: 120° azimuth, 60° elevation
- **Interfejs**: UART @921600 baud z binary protocol (TLV format)
- **Output**: 3D point cloud (x, y, z, velocity, SNR) z 50 fps
- **Liczba metryk**: 27 (takeoff_landing_rate, flight_duration_distribution, directional_bias, micro_Doppler_signature, swarm_exodus_pattern, predator_detection, nocturnal_activity_idx)

**Moduł Łączności Bezprzewodowej:**
- **WiFi**: ESP-12F module (802.11 b/g/n, 2.4 GHz) – opcjonalny dla pasiek z infrastrukturą
- **LTE**: Modem USB Huawei E3372h (Cat 4, 800/900/1800/2100/2600 MHz) – montowany w gateway'u
- **Anteny**: 
  - PCB antenna dla WiFi (2.4 GHz, gain 2 dBi)
  - External dipole dla LTE (omnidirectional, gain 3 dBi)
  - Patch antenna kierunkowa dla radaru (gain 12 dBi, beamwidth 30°)

**Zasilanie i Zarządzanie Energią:**
- **Panel solarny**: Monokrystaliczny 10 Wp (12 V nominal, Voc 21 V, Isc 0.58 A)
  - Wymiary: 340×200×17 mm
  - Sprawność: ≥20% (STC conditions)
  - Anti-reflective coating dla lepszej wydajności przy niskim kącie padania
- **Regulator ładowania**: MPPT (Maximum Power Point Tracking) custom design
  - Algorytm perturb & observe z krokiem adaptacyjnym
  - Sprawność konwersji: ≥95%
  - Zabezpieczenia: Reverse current, overvoltage, short-circuit, thermal shutdown
- **Akumulator**: LiFePO4 12 V 10 Ah (4S configuration)
  - Chemia: Lithium Iron Phosphate (bezpieczniejsza niż Li-ion)
  - Cykle życia: ≥2000 cykli @80% DoD
  - Zakres temperatur pracy: -20°C do +60°C
  - BMS (Battery Management System): Balancing cel, overcharge/overdischarge protection, temperature monitoring
- **Autonomia**: 7–14 dni bez słońca (zależnie od duty cycle sensorów)

**Obudowa i Mechanika:**
- **Materiał**: ASA (Acrylonitrile Styrene Acrylate) druk 3D FDM/FFF
  - Odporność UV: ≥5 lat bez żółknięcia
  - Temperatura defleksji cieplnej (HDT): 102°C @0.45 MPa
  - Wytrzymałość na uderzenia: 15 kJ/m² (notched Izod)
- **Uszczelnienie**: IP68 z uszczelką silikonową LSR (Liquid Silicone Rubber)
- **Wymiary**: 220×150×80 mm (bez uchwytu montażowego)
- **Masa**: 650 g (z baterią i elektroniką)
- **Uchwyt montażowy**: Szyna DIN 40×40 mm aluminium anodowane z quick-release mechanism

### 3.1.2.3. Gateway Edge Server

Gateway stanowi centralny punkt agregacji danych dla całej pasieki, realizujący funkcje edge computing, lokalnego storage'u i transmisji zdalnej.

**Platforma Sprzętowa:**
- **Komputer**: Raspberry Pi 2 Model B (lub nowszy Pi 3/4 dla większej wydajności)
  - SoC: Broadcom BCM2836 (Pi 2) lub BCM2711 (Pi 4)
  - CPU: 4×ARM Cortex-A7 @900 MHz (Pi 2) lub 4×Cortex-A72 @1.5 GHz (Pi 4)
  - RAM: 1 GB LPDDR2 (Pi 2) lub 4 GB LPDDR4 (Pi 4)
  - Storage: MicroSDHC 32 GB Class 10 (industrial grade, -40°C to +85°C)
  - Network: Ethernet 10/100 Mbps (Pi 2) lub Gigabit Ethernet (Pi 4)
  - USB: 4×USB 2.0 Host ports
  - Zasilanie: 5 V/2.5 A przez microUSB (Pi 2) lub USB-C (Pi 4)

**Modem LTE:**
- **Model**: Huawei E3372h-153 (global variant)
- **Standardy**: LTE Cat 4 (DL 150 Mbps, UL 50 Mbps), DC-HSPA+, GSM/GPRS/EDGE
- **Pasma**: 800/900/1800/2100/2600 MHz (Band 1/3/7/8/20) – kompatybilny z Aero2 w Polsce
- **Interfejs**: USB 2.0 High-Speed
- **Anteny**: 2×TS9 connectors dla external MIMO antennas (optional)
- **Zużycie mocy**: ≤500 mW (active data), ≤50 mW (idle)

**Zasilanie Awaryjne Gateway'a:**
- **UPS HAT**: Custom HAT z superkapacitorami 5 V/20 F
  - Czas podtrzymania: ≥5 minut dla graceful shutdown
  - Automatyczny restart po powrocie zasilania
- **Opcjonalnie**: Power bank 20000 mAh z pass-through charging (autonomia ≥48 godzin)

**Obudowa Gateway'a:**
- **Typ**: Industrial enclosure IP65 z wentylacją pasywną
- **Montaż**: Szafa telekomunikacyjna wall-mount lub rack-mount 19"
- **Chłodzenie**: Heat sink na CPU z thermal pads, airflow natural convection

---

## 3.1.3. Architektura Oprogramowania (Software Architecture)

### 3.1.3.1. Warstwy Abstrakcji Oprogramowania

System oprogramowania ApiaryGuard Pro zorganizowany jest w cztery warstwy abstrakcji zgodnie z patternem layered architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │   Web UI    │  │  AI Agent    │  │   REST API        │  │
│  │  Dashboard  │  │  (Qwen RAG)  │  │   (Swagger)       │  │
│  └─────────────┘  └──────────────┘  └───────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     Processing Layer                        │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │ ML Models   │  │ Alert Engine │  │ Data Aggregation  │  │
│  │ (ONNX)      │  │ (Rules)      │  │ (Time-Series)     │  │
│  └─────────────┘  └──────────────┘  └───────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                      Control Layer                          │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │   Firmware  │  │  RTOS Tasks  │  │ Sensor Drivers    │  │
│  │   (C++)     │  │  (FreeRTOS)  │  │ (I2C/SPI/UART)    │  │
│  └─────────────┘  └──────────────┘  └───────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                       Field Layer                           │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │  Sensors    │  │  Actuators   │  │ Communication     │  │
│  │ (HW Abstraction)│ (Relays, LEDs)│ (HTTP/MQTT/LoRa) │  │
│  └─────────────┘  └──────────────┘  └───────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**Warstwa Field Layer (Sprzętowa):**
- Abstrakcja hardware'u poprzez HAL (Hardware Abstraction Layer)
- Sterowniki device drivers dla każdego sensora (I2C, SPI, UART, ADC)
- Low-level initialization clocks, GPIO, interrupts
- Board support package (BSP) specyficzny dla Raspberry Pi Pico

**Warstwa Control Layer (Sterowania):**
- System czasu rzeczywistego FreeRTOS z task scheduling
- Task priorities: Audio (5), Radar (4), Sensors (3), Comms (2), Logging (1)
- IPC (Inter-Process Communication): Queues, semaphores, mutexes
- Watchdog timer hardware'owy z auto-reset

**Warstwa Processing Layer (Przetwarzania):**
- Algorytmy DSP (Digital Signal Processing): FFT, filtering, feature extraction
- Modele ML inference (ONNX Runtime)
- Silnik reguł biznesowych dla alertów
- Agregacja danych czasowych (rolling windows, exponential moving averages)

**Warstwa Application Layer (Aplikacyjna):**
- Interfejs użytkownika (Web UI dashboard)
- Agent AI z NLU i RAG
- REST API dla integracji zewnętrznych
- System raportowania i eksportu danych

### 3.1.3.2. Firmware Mikrokontrolera – Szczegóły Implementacji

Firmware mikrokontrolera RP2040 napisany jest w całości w języku C++17 z wykorzystaniem Raspberry Pi Pico SDK v1.5.1 i FreeRTOS v10.4.6. Decyzja o rezygnacji z Pythona/MicroPythona była świadomym wyborem architektonicznym podyktowanym wymaganiami determinizmu czasowego i efektywności energetycznej.

**Struktura Projektu:**
```
firmware/
├── src/
│   ├── main.cpp                 # Entry point, scheduler init
│   ├── tasks/
│   │   ├── audio_task.cpp       # Audio acquisition & FFT
│   │   ├── radar_task.cpp       # Radar point cloud processing
│   │   ├── sensors_task.cpp     # Polling I2C/SPI sensors
│   │   ├── comms_task.cpp       # HTTP POST to gateway
│   │   └── logging_task.cpp     # Flash storage, serial debug
│   ├── drivers/
│   │   ├── hx711_driver.cpp     # Load cell ADC
│   │   ├── i2s_mic_driver.cpp   # MEMS microphone I2S
│   │   ├── adxl345_driver.cpp   # Accelerometer SPI
│   │   ├── bme280_driver.cpp    # Climate sensor I2C
│   │   ├── scd41_driver.cpp     # CO2 sensor I2C
│   │   ├── sgp40_driver.cpp     # VOC sensor I2C
│   │   ├── bh1750_driver.cpp    # Lux sensor I2C
│   │   └── radar_uart_driver.cpp# MMWave radar UART
│   ├── algorithms/
│   │   ├── fft_hamming.cpp      # FFT implementation
│   │   ├── kalman_filter.cpp    # Sensor fusion
│   │   └── metrics_calc.cpp     # Derived metrics
│   ├── rtos/
│   │   ├── queue_management.cpp # IPC queues
│   │   └── watchdog_handler.cpp # WDT service
│   └── config/
│       ├── pin_definitions.h    # GPIO mapping
│       ├── calibration_data.h   # Factory calibration constants
│       └── network_config.h     # WiFi/AP credentials
├── include/
│   └── *.h                      # Public headers
├── lib/
│   ├── FreeRTOS-Kernel/         # FreeRTOS source
│   ├── pico-sdk/                # Raspberry Pi Pico SDK
│   └── onnx-runtime/            # ML inference engine (optional)
├── CMakeLists.txt               # Build configuration
└── platformio.ini               # PlatformIO project config
```

**Konfiguracja FreeRTOS:**
```cpp
// FreeRTOSConfig.h
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      133000000
#define configTICK_RATE_HZ                      1000
#define configMAX_PRIORITIES                    6
#define configMINIMAL_STACK_SIZE                128
#define configTOTAL_HEAP_SIZE                   200000
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_TRACE_FACILITY                1
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_APPLICATION_TASK_TAG          0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 1
#define configUSE_SB_SINGLE_COPY_OPTIMIZATION   1
```

**Task Schedule i Priorytety:**
| Task Name | Priority | Stack Size | Period | WCET ( Worst-Case Execution Time) |
|-----------|----------|------------|--------|-----------------------------------|
| vAudioTask | 5 (Highest) | 4096 bytes | 20 ms | 8 ms |
| vRadarTask | 4 | 3072 bytes | 50 ms | 15 ms |
| vSensorsTask | 3 | 2048 bytes | 1000 ms | 120 ms |
| vCommsTask | 2 | 2048 bytes | 60000 ms | 5000 ms |
| vLoggingTask | 1 (Lowest) | 1024 bytes | Event-driven | 50 ms |

**Mechanizmy Bezpieczeństwa Firmware:**
1. **Secure Boot**: Weryfikacja podpisu cyfrowego RSA-2048 przed uruchomieniem aplikacji
2. **Watchdog Timer**: Hardware'owy WDT z timeout 8.38 s, reset przy braku feed
3. **Brownout Detection**: Reset przy napięciu <2.8 V zapobiegający corruption Flash
4. **Flash Encryption**: Opcjonalne szyfrowanie partycji danych (AES-256-XTS)
5. **Rollback Protection**: Counter wersji firmware blokujący downgrade do vulnerable version

### 3.1.3.3. System Operacyjny Gateway'a

Gateway pracuje pod kontrolą systemu Raspberry Pi OS Lite (Debian Bookworm, kernel 6.1 LTS) zoptymalizowanego pod kątem pracy headless (bez GUI).

**Customizacje Systemu:**
- **Kernel Parameters Tuning** (`/etc/sysctl.conf`):
  ```bash
  net.core.somaxconn = 65535
  net.ipv4.tcp_max_syn_backlog = 65535
  net.ipv4.ip_local_port_range = 1024 65535
  net.ipv4.tcp_tw_reuse = 1
  vm.swappiness = 1
  vm.dirty_ratio = 40
  vm.dirty_background_ratio = 10
  ```
- **Filesystem Optimizations** (`/boot/cmdline.txt`):
  ```
  quiet splash elevator=deadline fsck.repair=preventened ipv6.disable=1
  ```
- **Disable Unnecessary Services**:
  ```bash
  sudo systemctl disable bluetooth.service
  sudo systemctl disable avahi-daemon.service
  sudo systemctl disable triggerhappy.service
  sudo systemctl disable ledtrig-netdev.service
  ```

**Systemd Unit Files:**
- `apiary-gateway.service`: Główna aplikacja backendowa (C++ Crow framework)
- `lte-modem.service`: Skrypt inicjalizacji modemu PPP
- `sqlite-backup.timer`: Backup bazy danych co godzinę
- `ntp-sync.service`: Synchronizacja czasu z pool.ntp.org
- `watchdog-monitor.service`: Monitorowanie health check węzłów

**Backup Strategy:**
- **Lokalny backup**: Rotacja 7 dni wstecz (`backup_YYYYMMDD_HHMMSS.db`)
- **Remote backup**: rsync over SSH do AWS S3 / Google Cloud Storage co godzinę
- **Point-in-time recovery**: WAL (Write-Ahead Logging) enabled w SQLite

---

## Podsumowanie Sekcji 3.1

Sekcja 3.1 przedstawiła kompleksową architekturę systemu ApiaryGuard Pro obejmującą:
1. **Wymagania systemowe** zdefiniowane dla trzech scenariuszy aplikacyjnych (hobby, commercial, research)
2. **Architekturę sprzętową** z detalami doboru komponentów, uzasadnieniem wyborów projektowych i specyfikacją metrologiczną
3. **Architekturę oprogramowania** z podziałem na warstwy abstrakcji, harmonogramem zadań RTOS i mechanizmami bezpieczeństwa

Prezentowana architektura spełnia wszystkie wymagania niefunkcjonalne dotyczące niezawodności (MTBF ≥50 000 h), wytrzymałości środowiskowej (IP68, -30°C do +60°C), efektywności energetycznej (autonomia ≥7 dni) oraz bezpieczeństwa (secure boot, encrypted communications).
