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

