# ApiaryGuard - Firmware dla Raspberry Pi Pico (RP2040)

Kompletny kod Arduino IDE (C++) dla systemu monitoringu ula z obsługą modułu Ethernet **W6100** oraz wszystkich sensorów i efektorów.

## 📋 Wymagania

### Sprzęt
- **Płytka:** Raspberry Pi Pico (RP2040)
- **Moduł Ethernet:** W6100 (SPI)
- **Sensory:** HX711, DHT22, SGP41/BME688, LD2410B, MEMS Mic, Piezo
- **Efektory:** Grzałka 10W PWM, Wentylator 12V PWM, Pompa perystaltyczna, Zawory elektromagnetyczne, Przekaźnik 8-kanałowy

### Oprogramowanie
- **Arduino IDE** (wersja 2.x lub 1.8.x)
- **Core RP2040:** "Raspberry Pi RP2040 Boards" by Earle F. Philhower III
- **Biblioteki Arduino:**
  - `Ethernet` (by Arduino) - obsługa W6100
  - `DHT sensor library` (by Adafruit)
  - `Adafruit SGP41` (by Adafruit)
  - `Wire` (wbudowana)
  - `SPI` (wbudowana)
  - `HardwareSerial` (wbudowana)

## 🔌 Schemat Połączeń

### Moduł W6100 (SPI1)
| W6100 Pin | Pico GPIO | Opis |
|-----------|-----------|------|
| CS        | GPIO 9    | Chip Select |
| MOSI      | GPIO 11   | Master Out Slave In |
| MISO      | GPIO 12   | Master In Slave Out |
| SCK       | GPIO 10   | Clock |
| RST       | GPIO 8    | Reset |
| INT       | GPIO 13   | Interrupt (opcjonalny) |
| 3.3V      | 3.3V      | Zasilanie |
| GND       | GND       | Masa |

### Radar LD2410B (UART1)
| LD2410B Pin | Pico GPIO | Opis |
|-------------|-----------|------|
| TX          | GPIO 4    | RX1 (odbiór) |
| RX          | GPIO 5    | TX1 (wysyłanie) |
| VCC         | 5V        | Zasilanie |
| GND         | GND       | Masa |

### Czujniki I2C (SGP41/BME688)
| Sensor Pin | Pico GPIO | Opis |
|------------|-----------|------|
| SDA        | GPIO 0    | Data |
| SCL        | GPIO 1    | Clock |
| VCC        | 3.3V      | Zasilanie |
| GND        | GND       | Masa |

### DHT22 (Temperatura/Wilgotność)
| DHT22 Pin | Pico GPIO | Opis |
|-----------|-----------|------|
| DATA      | GPIO 2    | Dane cyfrowe |
| VCC       | 3.3V      | Zasilanie |
| GND       | GND       | Masa |
*Uwaga: Dodaj rezystor 10kΩ między DATA a VCC*

### HX711 (Waga 24-bit)
| HX711 Pin | Pico GPIO | Opis |
|-----------|-----------|------|
| DT        | GPIO 3    | Data |
| SCK       | GPIO 22   | Clock |
| VCC       | 5V        | Zasilanie |
| GND       | GND       | Masa |

### MEMS Microphone & Piezo (ADC)
| Sensor | Pico GPIO | ADC Kanal |
|--------|-----------|-----------|
| MIC OUT| GPIO 26   | ADC0 |
| PIEZO  | GPIO 27   | ADC1 |

### Efektory (PWM/GPIO)
| Efektor | Pico GPIO | Typ |
|---------|-----------|-----|
| Grzałka PWM | GPIO 6 | PWM |
| Wentylator PWM | GPIO 7 | PWM |
| Pompa (Relay) | GPIO 14 | GPIO |
| Zawór 1 | GPIO 15 | GPIO |
| Zawór 2 | GPIO 16 | GPIO |
| Przekaźnik CH1-CH8 | GPIO 17-24 | GPIO |
| LED Status | GPIO 25 | WBUDOWANY |

## ⚙️ Konfiguracja

### Adresacja IP
Edytuj w pliku `.ino`:
```cpp
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);   // Zmień na swoją sieć
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
```

### Kalibracja Wagi HX711
```cpp
long hx711_offset = 0;     // Wartość tare (pusta waga)
float hx711_scale = 1.0;   // Współczynnik kalibracyjny
```
Aby skalibrować:
1. Zważ znany obiekt
2. Odczytaj `hx711_value` z Serial Monitor
3. Oblicz: `scale = raw_value / known_weight`

## 🌐 Protokół Komunikacyjny

### HTTP API (Port 8080)

#### Pobierz status sensorów
```
GET http://192.168.1.177:8080/status
```
**Odpowiedź:**
```json
{
  "temp": 23.5,
  "hum": 65.2,
  "weight": 1250,
  "co2": 450,
  "voc": 85,
  "audio": 0.15,
  "motion": 0
}
```

#### Sterowanie grzałką
```
GET http://192.168.1.177:8080/heater/on
GET http://192.168.1.177:8080/heater/off
```

#### Sterowanie wentylatorem
```
GET http://192.168.1.177:8080/fan/on
GET http://192.168.1.177:8080/fan/off
```

#### Sterowanie pompą
```
GET http://192.168.1.177:8080/pump/on
GET http://192.168.1.177:8080/pump/off
```

## 🔧 Instalacja w Arduino IDE

1. **Zainstaluj core RP2040:**
   - Plik → Preferencje → Dodatkowe adresy URL menedżera płytek:
     ```
     https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
     ```
   - Narzędzia → Płytka → Menadżer płytek → wyszukaj "RP2040" → zainstaluj

2. **Zainstaluj biblioteki:**
   - Szkic → Dołącz bibliotekę → Zarządzaj bibliotekami
   - Wyszukaj i zainstaluj:
     - `Ethernet` by Arduino
     - `DHT sensor library` by Adafruit
     - `Adafruit SGP41` by Adafruit

3. **Skonfiguruj płytkę:**
   - Narzędzia → Płytka → Raspberry Pi RP2040 Boards → "Raspberry Pi Pico"
   - Narzędzia → USB Stack → "TinyUSB" (lub "PicoSDK")
   - Narzędzia → Flash Size → odpowiednio do Twojej płytki

4. **Wgraj kod:**
   - Podłącz Pico przez USB (przytrzymaj BOOTSEL podczas podłączania)
   - Wybierz port: Narzędzia → Port → odpowiedni COM/USB
   - Kliknij "Wgraj" (strzałka w prawo)

## 🐛 Debugowanie

### Monitor Szeregowy
- Baudrate: **115200**
- Format: Both NL & CR

### Przykładowy output:
```
=== ApiaryGuard Pico Start ===
Inicjalizacja W6100...
W6100 połączono. IP: 192.168.1.177
System gotowy.
T:23.5 H:65.2 Waga:1250 CO2:450 VOC:85 Audio:0.15 Ruch:0
T:23.6 H:65.0 Waga:1248 CO2:455 VOC:88 Audio:0.12 Ruch:1
```

### Rozwiązywanie problemów

**W6100 nie wykrywa sieci:**
- Sprawdź połączenia SPI (GPIO 9-12)
- Upewnij się że reset (GPIO 8) jest poprawnie podłączony
- Zweryfikuj adres IP w Twojej sieci

**HX711 zwraca losowe wartości:**
- Sprawdź długość przewodów (powinny być krótkie)
- Dodaj kondensator 100nF między VCC a GND przy module
- Skalibruj offset i scale

**SGP41 nie odpowiada:**
- Sprawdź adres I2C (domyślnie 0x59)
- Upewnij się o podciągnięciach SDA/SCL (rezystory 4.7kΩ)

## 📝 Licencja
MIT License - użyj zgodnie z potrzebami.
