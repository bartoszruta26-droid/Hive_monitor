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

#### 5. Zaawansowane Czujniki Specjalistyczne (NEW)

##### a) Radar MMWave GHz Human Presence Sensor (np. LD2410B / RCWL-9600)
- **Technologia**: FMCW radar 24GHz lub 60GHz z detekcją mikro-ruchów
- **Zakres detekcji**: 0.2m - 8m, kąt 120°
- **Precyzja**: Wykrywa ruchy rzędu milimetrów (oddech, bicie serca)
- **Częstotliwość**: Continuous wave monitoring
- **Zastosowanie w ulu**:
  - **Detekcja obecności pszczelarza**: Automatyczne logowanie wizyt serwisowych
  - **Monitoring aktywności na wylotku**: Precyzyjny pomiar ruchu bez inwazji do wnętrza
  - **Wykrywanie drapieżników**: Detekcja dużych obiektów (niedźwiedzie, kuny) przed ulem
  - **Analiza wzorców wylotowych**: Liczenie pszczół wylatujących/wracających bez kontaktu fizycznego
- **UZASADNIENIE POTRZEBY**: Tradycyjne czujniki PIR nie wykrywają małych obiektów jak pszczoły. Radar MMWave umożliwia:
  - Monitorowanie bez ingerencji w strukturę ula (montaż zewnętrzny)
  - Detekcję nawet pojedynczych pszczół dzięki wysokiej częstotliwości
  - Pracę w całkowitej ciemności i przez ścianki ula (nieinwazyjne)
  - Pomiar odległości do obiektu - można określić czy pszczoła jest przy wylotku czy w locie
- **EMF Shielding**: Wymagane ekranowanie kierunku wnętrza ula aby minimalizować promieniowanie RF wewnątrz gniazda

##### b) Wielogazowy Sensor CO2/VOC/Gas (np. SGP41 / BME688 / CCS811 z rozszerzeniem)
- **Technologia**: MOX (Metal Oxide Semiconductor) + NDIR dla CO2
- **Zakres pomiarowy**: 
  - CO2: 400-65535 ppm
  - VOC (TVOC): 0-60000 ppb
  - NOx: 0-10 ppm
  - Etanol/Metanol: 0-500 ppm
- **Precyzja**: ±5% dla CO2, ±10% dla VOC
- **Zastosowanie w ulu**:
  - **Monitorowanie metabolizmu rodziny**: CO2 jako wskaźnik aktywności i wielkości populacji
  - **Detekcja chorób**: Specyficzne profile VOC przy infekcjach grzybiczych (Ascospahaera apis - kamienica)
  - **Fermentacja syropu**: Wykrywanie etanolu przy niewłaściwym dokarmianiu
  - **Jakość powietrza**: Monitoring wentylacji i świeżości atmosfery w ulu
  - **Stres chemiczny**: Detekcja pestycydów принесionych z pola (nowe badania)
- **UZASADNIENIE POTRZEBY**: Pojedynczy sensor CO2 (MH-Z19B) to za mało. Kompleksowy sensor gazów:
  - Pozwala na wczesną detekcję chorób po profilu lotnych związków organicznych
  - Monitoruje jakość środowiska dla optymalnego rozwoju czerwiu
  - Wykrywa anomalie fermentacyjne przy dokarmianiu
  - Umożliwia badania nad stresem chemicznym pszczół w rolnictwie przemysłowym

##### c) Kamera Wizyjna HD z Analizą Edge AI
- **Sprzęt**: Kamera PoE 2MP lub Raspberry Pi Camera V2 z oświetleniem IR
- **Rozdzielczość**: 1080p @ 30fps lub 5MP still images
- **Zastosowanie**:
  - **Liczenie pszczół na wylotku**: Computer vision co 60 sekund
  - **Detekcja intruzów**: Osy szerszenie, mrówki, inne zwierzęta
  - **Monitoring pogody**: Opady, śnieg, warunki wokół pasieki
  - **Inspekcja wizualna**: Zdjęcia plastrów przy otwartym ulu (tryb serwisowy)
  - **Nagrywanie time-lapse**: Dzienna aktywność pasieki
- **UZASADNIENIE POTRZEBY**: Monitoring wizyjny dostarcza danych niedostępnych dla sensorów:
  - Bezpośrednia obserwacja behawioralna bez otwierania ula
  - Dokumentacja fotograficzna dla analizy długoterminowej
  - Detekcja wzrokowa drapieżników i szkodników
  - Walidacja innych sensorów (korelacja wagowych spadków z wizją rojenia)

##### d) Upgrade Mikrokontrolera: ESP32-WROOM-32 lub Raspberry Pi Pico W
- **Problem Arduino Nano**: ATmega328P ma zbyt ograniczone zasoby (2KB RAM, 32KB Flash) dla nowych sensorów
- **ESP32-WROOM-32**:
  - Dual-core Tensilica LX6 @ 240 MHz
  - 520 KB SRAM, 4MB Flash
  - WiFi 802.11 b/g/n + Bluetooth 4.2 BR/EDR + BLE
  - 18x 12-bit ADC, 10x touch sensor
  - Hardware encryption for security
- **Raspberry Pi Pico W** (opcja ostateczna z lepszym supportem):
  - RP2040 dual-core ARM Cortex-M0+ @ 133 MHz
  - 264 KB SRAM, 2MB Flash
  - WiFi 802.11n (model W)
  - 3x 12-bit ADC, extensive GPIO
  - MicroPython/C++ support
- **UZASADNIENIE POTRZEBY**: Arduino Nano jest niewystarczające dla:
  - Przetwarzania FFT audio w czasie rzeczywistym
  - Obsługi wielu sensorów I2C jednocześnie (CO2+VOC+radar+kamera)
  - Komunikacji WiFi dla lokalnego dashboardu
  - Edge AI inference dla prostych modeli ML
  - Większej pamięci na buffering danych sensorycznych
- **Rekomendacja**: ESP32 jako główny MCU, Pico W jako backup lub dla mniejszych jednostek satelitarnych

##### e) Ochrona EMF Shield Protection
- **Problem**: Radary MMWave, WiFi ESP32 i transmisja LTE generują pole elektromagnetyczne
- **Ryzyko**: Potencjalny wpływ RF na orientację pszczół, zdrowie rodziny, produkcję miodu
- **Rozwiązanie**:
  - **Mu-metal shielding**: Ekranowanie kierunkowe sensorów RF
  - **Faraday cage compartment**: Oddzielna przegroda dla modułów radiowych
  - **Directional antennas**: Skierowanie promieniowania z dala od gniazda
  - **Power management**: Wyłączanie transmitterów gdy niepotrzebne
  - **Distance isolation**: Montaż sensorów RF minimum 30cm od ściany ula
- **UZASADNIENIE POTRZEBY**: Badania naukowe sugerują że pole elektromagnetyczne może:
  - Zaburzać nawigację pszczół (magnetorecepcja)
  - Wpływać na produkcję mleczki pszczelej
  - Zwiększać agresję rodzin
  - Projekt musi minimalizować własny wpływ na środowisko które monitoruje

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

#### Mechanical Design & Enclosure - UPGRADED FOR NEW SENSORS

**Wymagania Projektowe dla Obudowy:**
- **Material**: ABS plastic UV-resistant + aluminum heat sink + **EMF shielding compartments**
- **IP Rating**: **IP66/IP67/IP68** waterproof enclosure (upgrade from IP65)
  - IP66: Protection against powerful water jets
  - IP67: Immersion up to 1m for 30 minutes
  - IP68: Continuous immersion >1m (recommended for flood-prone areas)
- **Dimensions**: 250x180x100 mm (RPi + ESP32/Pico + sensors + shielding)
- **Mounting**: External bracket on hive back panel with vibration isolation
- **Thermal**: Passive cooling with rain shield + thermal analysis for sensor placement
- **Security**: Lockable enclosure with tamper switch + GPS anti-theft mount

**EMF Shielding Architecture:**
```
┌─────────────────────────────────────────────────────────────┐
│                    EXTERNAL ENCLOSURE IP68                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │  RF COMPARTMENT │  │ SENSOR COMPART. │  │ POWER UNIT  │ │
│  │  (Faraday Cage) │  │  (Mu-metal)     │  │             │ │
│  │  - ESP32 WiFi   │  │  - MMWave Radar │  │  - PoE      │ │
│  │  - LTE Dongle   │  │  - Gas Sensors  │  │  - Battery  │ │
│  │  - GPS Module   │  │  - Temp/Hum     │  │  - Relays   │ │
│  │  Directional →  │  │  ← Shielded     │  │             │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
│         ↑                     ↑                              │
│    Antenna ports        Sensor ports                         │
│    (external)           (directed away from hive)            │
└─────────────────────────────────────────────────────────────┘
```

**Projekt Mechaniczny - Wymagania:**
1. **CAD Design Required**: Konieczny profesjonalny projekt 3D (Fusion 360/SolidWorks)
   - Kompartmentalizacja dla EMF shielding
   - Kanały kablowe z uszczelkami IP68
   - Mounting points dla sensorów zewnętrznych
   - Heat dissipation analysis

2. **3D Printing/Machining**: 
   - Material: ASA lub PETG-CF (UV resistant, high temp)
   - Wall thickness: minimum 3mm dla IP68
   - Gasket grooves dla silicone seals

3. **Sensor Placement Strategy**:
   - MMWave radar: Montaż z przodu obudowy, skierowany NA ZEWNĄTRZ od ula
   - Gas sensors: Intake tubes z filtrem membranowym (IP68 breathable)
   - Camera: Waterproof housing z IR cut filter
   - All RF sources: Minimum 30cm od ściany ula + directional shielding

4. **Certification Requirements**:
   - IP66/67/68 testing (water ingress, dust)
   - EMC/EMI compliance (CE marking)
   - Temperature cycling (-20°C to +60°C)
   - Vibration testing (transport, wind)

**UZASADNIENIE POTRZEBY LEPSZEJ OBUDOWY:**
- Nowe sensory (MMWave, kamera, CO2/VOC) wymagają szczelniejszej ochrony niż IP65
- EMF shielding jest krytyczny dla minimalizacji wpływu na pszczoły
- Kompartmentalizacja zapobiega interferencjom między modułami RF a sensorami analogowymi
- Wyższy rating IP zapewnia długoterminową niezawodność w ekstremalnych warunkach pasiecznych
- Projekt mechaniczny musi uwzględniać chłodzenie przy zachowaniu szczelności

---

