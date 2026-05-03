# ApiaryGuard - Kod Arduino IDE dla W6100

## 📋 Opis

Ten folder zawiera kompletny kod firmware dla Arduino Nano (ATmega328P) obsługujący:

- **Moduł Ethernet W6100** - komunikacja TCP/IP przez SPI
- **Sensory**:
  - HX711 + Strain Gauge (waga 24-bit ADC)
  - DHT22/AM2302 (temperatura i wilgotność)
  - Mikrofon MEMS (analiza brzmienia rodziny pszczelej)
  - Czujnik piezoelektryczny (wibracje)
  - SGP41/BME688 (CO2, VOC, gazy)
  - LD2410B (radar MMWave GHz)
- **Efektory**:
  - Grzałka rezystancyjna 10W (PWM)
  - Wentylator axial 12V (PWM)
  - Pompa perystaltyczna (dozowanie terapeutyczne)
  - Zawory elektromagnetyczne
  - Moduł przekaźnikowy 8-kanałowy

## 🔧 Wymagania

### Arduino IDE
- Arduino IDE 1.8.x lub nowsze / Arduino IDE 2.x
- Board: Arduino Nano (ATmega328P)

### Biblioteki
Zainstaluj następujące biblioteki przez Library Manager:

```
Ethernet (by Arduino) - do obsługi W6100
SPI (wbudowana)
Wire (wbudowana)
EEPROM (wbudowana)
```

**Uwaga:** Moduł W6100 jest kompatybilny z biblioteką Ethernet Arduino, ponieważ używa tego samego interfejsu SPI co W5100/W5500.

## 📐 Połączenia Sprzętowe

### W6100 Ethernet Module
```
W6100      -> Arduino Nano
VCC        -> 5V
GND        -> GND
MISO       -> D12 (ICSP-4)
MOSI       -> D11 (ICSP-5)
SCK        -> D13 (ICSP-3)
CS         -> D10
RST        -> D9
INT        -> NC (nie używane)
```

### HX711 (Waga)
```
HX711      -> Arduino Nano
VCC        -> 5V
GND        -> GND
DT (DOUT)  -> A0
SCK        -> A1
```

### DHT22 (Temperatura/Wilgotność)
```
DHT22      -> Arduino Nano
VCC        -> 5V
GND        -> GND
DATA       -> D2
(4.7kΩ pull-up between VCC and DATA)
```

### Mikrofon MEMS
```
MIC        -> Arduino Nano
VCC        -> 5V
GND        -> GND
OUT        -> A2
```

### Piezo (Wibracje)
```
Piezo      -> Arduino Nano
+          -> A3
-          -> GND
(1MΩ resistor parallel)
```

### SGP41/BME688 (Gazy - I2C)
```
Sensor     -> Arduino Nano
VCC        -> 3.3V lub 5V
GND        -> GND
SDA        -> A4
SCL        -> A5
```

### LD2410B (Radar MMWave - UART)
```
Radar      -> Arduino Nano
VCC        -> 5V
GND        -> GND
TX         -> D3 (RX w Arduino)
RX         -> D4 (TX w Arduino)
```

### Efektory
```
Heater PWM -> D5 (przez MOSFET/relay)
Fan PWM    -> D6 (przez MOSFET)
Pump       -> D7 (przez relay)
Valve      -> D8 (przez relay)
Relay 1    -> A6
Relay 2    -> A7
```

## ⚙️ Konfiguracja Sieciowa

Edytuj sekcję `KONFIGURACJA SIECIOWA W6100` w pliku `.ino`:

```cpp
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);      // IP Arduino
IPAddress gateway(192, 168, 1, 1);   // Brama
IPAddress subnet(255, 255, 255, 0);  // Maska
IPAddress dns(8, 8, 8, 8);           // DNS

IPAddress serverIP(192, 168, 1, 50); // IP Raspberry Pi / serwera
const uint16_t serverPort = 1883;    // Port MQTT/TCP
```

## 🚀 Instalacja i Kompilacja

1. **Otwórz Arduino IDE**
2. **Zainstaluj bibliotekę Ethernet**:
   - Tools → Manage Libraries
   - Search "Ethernet"
   - Install "Ethernet by Arduino"

3. **Otwórz projekt**:
   - File → Open
   - Wybierz `apiaryguard_ino.ino`

4. **Skonfiguruj board**:
   - Tools → Board → Arduino Nano
   - Tools → Processor → ATmega328P
   - Tools → Port → (wybierz odpowiedni port COM)

5. **Kompiluj i wgraj**:
   - Kliknij ✓ (Verify/Compile)
   - Kliknij → (Upload)

## 📊 Protokół Komunikacyjny

### Format Pakietu Sensorów (TX do serwera)
```
Header (1 byte): 0x55
Data (32 bytes): struct SensorData
Checksum (1 byte): XOR wszystkich bajtów danych
```

### Struktura SensorData
```cpp
struct SensorData {
  uint32_t timestamp;        // 4 bajty
  float weight_kg;           // 4 bajty
  float temperature_c;       // 4 bajty
  float humidity_percent;    // 4 bajty
  uint16_t audio_rms;        // 2 bajty
  uint16_t piezo_activity;   // 2 bajty
  uint16_t co2_ppm;          // 2 bajty
  uint16_t tvoc_ppb;         // 2 bajty
  uint8_t radar_motion;      // 1 bajt
  uint8_t status_flags;      // 1 bajt
};                           // Razem: 24 bajty
```

### Format Komend Efektorów (RX z serwera)
```
Header (1 byte): 0xAA
Data (8 bytes): struct ActuatorCommand
```

### Struktura ActuatorCommand
```cpp
struct ActuatorCommand {
  uint8_t heater_pwm;      // 0-255
  uint8_t fan_pwm;         // 0-255
  uint8_t pump_duration;   // sekundy (0 = wyłącz)
  uint8_t valve_state;     // 0=OFF, 1=ON
  uint8_t relay_mask;      // bitmaska 8 przekaźników
};
```

## 🔍 Debugowanie

### Włącz tryb debugowania
Dodaj definicję przed kompilacją:
```cpp
#define DEBUG_SENSORS
```

### Monitorowanie Serial
Otwórz Serial Monitor (115200 baud):
```
=== ApiaryGuard Arduino Nano ===
Inicjalizacja systemu...
Inicjalizacja W6100 Ethernet...
W6100: Połączenie fizyczne OK
W6100 IP: 192.168.1.100
Inicjalizacja sensorów...
Sensory zainicjalizowane
Inicjalizacja efektorów...
Efektory zainicjalizowane
System gotowy!
```

## 🛠️ Kalibracja Wagi

1. Wgraj kod z włączonym `DEBUG_SENSORS`
2. Otwórz Serial Monitor
3. Wywołaj funkcję kalibracji przez komendę tekstową:
   ```
   CALIBRATE
   ```
4. Postępuj zgodnie z instrukcjami:
   - Najpierw zerowanie (pusty ul)
   - Następnie umieść znany ciężar (np. 1kg)
   - Naciśnij dowolny klawisz

## 📝 Uwagi Techniczne

### W6100 vs W5100/W5500
- W6100 jest nowszym modułem Wiznet z lepszą wydajnością
- Kompatybilny z biblioteką Ethernet Arduino
- Obsługuje TCP/IP hardware offload
- Niższe zużycie pamięci Flash i RAM

### Ograniczenia Arduino Nano
- Flash: 32KB (ten projekt używa ~24KB)
- SRAM: 2KB (używane ~1.2KB)
- EEPROM: 1KB (kalibracja)

### Zalecenia
- Używaj zewnętrznej anteny Ethernet dla lepszej EMC
- Dodaj kondensatory odsprzęgające (100nF) przy każdym sensorze
- Stosuj separację galwaniczną dla efektorów wysokiej mocy
- Chroń przed przepięciami (TVS diodes, varistory)

## 📄 Licencja

MIT License - ApiaryGuard Team

## 🤝 Współpraca

Więcej informacji w dokumentacji projektu:
- `/docs/03_specyfikacja_sprzetowa.md`
- `/docs/05_opis_modulow_programowych.md`
