# ApiaryGuard - Raspberry Pi Pico + W6100

## 📋 Opis systemu
Kompletny firmware dla Raspberry Pi Pico z modułem Ethernet W6100 do monitoringu i sterowania ulami pszczelimi.

**Nowość:** Profesjonalny moduł analizy jakości powietrza (SGP41) z 28+ parametrami!

## 🔌 Połączenia GPIO

### Moduł W6100 (SPI1)
| W6100 Pin | Raspberry Pico GPIO | Funkcja |
|-----------|---------------------|---------|
| CS        | GP5                 | Chip Select |
| MOSI      | GP7                 | SPI1 TX |
| MISO      | GP8                 | SPI1 RX |
| SCK       | GP6                 | SPI1 Clock |
| RST       | GP4                 | Reset |
| INT       | GP3                 | Interrupt |
| 3.3V      | 3.3V                | Zasilanie |
| GND       | GND                 | Masa |

### Sensory
| Sensor | GPIO | Typ | Uwagi |
|--------|------|-----|-------|
| HX711 (DT) | GP9  | Digital | Waga 24-bit |
| HX711 (SCK)| GP10 | Digital | Clock |
| DHT22      | GP11 | Digital | Temp/Wilg |
| MEMS Mic   | GP26 | ADC0    | Audio RMS |
| Piezo      | GP27 | ADC1    | Wibracje |
| SGP41 (SDA)| GP0  | I2C     | CO2/VOC (zmienione z GP2!) |
| SGP41 (SCL)| GP1  | I2C     | Clock (zmienione z GP3!) |
| LD2410B TX | GP28 | UART1 RX| Radar MMWave |
| LD2410B RX | GP29 | UART1 TX| Radar MMWave |

**UWAGA:** Połączenia I2C dla SGP41 zostały zmienione z GP2/GP3 na GP0/GP1, aby uniknąć konfliktu z pinem INT W6100 (GP3).

### Efektory
| Efektor | GPIO | Typ | PWM |
|---------|------|-----|-----|
| Grzałka 10W | GP12 | PWM | Tak |
| Wentylator 12V | GP13 | PWM | Tak |
| Pompa perystaltyczna | GP14 | PWM | Tak |
| Przekaźnik 1-8 | GP15-GP22 | Digital | Nie |

## 📦 Wymagane biblioteki Arduino

Zainstaluj przez **Arduino IDE → Tools → Board → Boards Manager** oraz **Sketch → Include Library → Manage Libraries**:

1. **Raspberry Pi Pico/RP2040** by Earle F. Philhower III (Boards Manager)
2. **W6100Ethernet** by WIZnet LUB **Ethernet** by Arduino z driverem W6100 (Library Manager)
   - *Uwaga: Standardowa biblioteka Ethernet może nie obsługiwać W6100! Użyj dedykowanej biblioteki W6100Ethernet.*
3. **DHT sensor library** by Adafruit (Library Manager)
4. **Adafruit SGP41 Library** (Library Manager)
5. **ArduinoJson** by Benoit Blanchon (Library Manager, wersja 7.x)

## ⚙️ Konfiguracja sieciowa

Edytuj na początku pliku `apiaryguard_pico.ino`:

```cpp
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);      // Twój adres IP
IPAddress gateway(192, 168, 1, 1);   // Brama
IPAddress subnet(255, 255, 255, 0);  // Maska podsieci
```

## 🌐 API HTTP (Port 8080)

### Pobieranie danych z sensorów
```
GET http://192.168.1.100:8080/api/data
```

**Odpowiedź JSON:**
```json
{
  "temp": 24.5,
  "hum": 65.2,
  "weight": 45000,
  "audio": 125,
  "vibration": 2048,
  "co2": 450,
  "voc": 150,
  "motion": true,
  "relays": 0,
  "timestamp": 12345678
}
```

### Jakość powietrza (28+ parametrów)
```
GET http://192.168.1.100:8080/api/aq
```

**Odpowiedź JSON - wszystkie parametry jakości powietrza:**
```json
{
  "co2_current": 450,
  "voc_current": 150,
  "nox_equivalent": 300,
  "co2_mean": 465.50,
  "co2_std": 25.30,
  "co2_min": 420,
  "co2_max": 510,
  "co2_range": 90,
  "voc_mean": 145.20,
  "voc_std": 18.50,
  "voc_min": 120,
  "voc_max": 180,
  "co2_slope_1h": 5.25,
  "trend_direction": 1,
  "trend_strength": 0.26,
  "iaq_index": 85.5,
  "air_quality_level": 1,
  "ventilation_need": 12.5,
  "stress_from_air": 15.0,
  "hive_comfort_index": 85.0,
  "poor_ventilation_alert": false,
  "contamination_risk": false,
  "mold_risk": false,
  "high_co2_alert": false,
  "combined_risk_score": 0.0,
  "variability_index": 5.44,
  "stability_score": 94.56,
  "change_rate": 5.25,
  "volatility_index": 25.30,
  "comfort_zone_percent": 85.0,
  "co2_warning_level": 0,
  "voc_alert_level": 1
}
```

**Opis parametrów jakości powietrza:**

| Kategoria | Parametry | Opis |
|-----------|-----------|------|
| **Podstawowe (3)** | co2_current, voc_current, nox_equivalent | Aktualne wartości CO2 [ppm], VOC Index, NOx [ppb] |
| **Statystyczne CO2 (6)** | co2_mean, co2_std, co2_min, co2_max, co2_range, co2_cv | Średnia, odchylenie, min, max, zakres, współczynnik zmienności |
| **Statystyczne VOC (6)** | voc_mean, voc_std, voc_min, voc_max, voc_range, voc_cv | Statystyki dla VOC Index |
| **Trendy (3)** | co2_slope_1h, trend_direction, trend_strength | Nachylenie trendu 1h, kierunek (-1/0/1), siła (0-1) |
| **Indeksy jakości (5)** | iaq_index, air_quality_level, ventilation_need, stress_from_air, hive_comfort_index | IAQ (0-500), poziom (1-4), potrzeba wentylacji %, stres %, komfort % |
| **Zagrożenia (5)** | poor_ventilation_alert, contamination_risk, mold_risk, high_co2_alert, combined_risk_score | Alerty boolean, łączny wynik ryzyka (0-100%) |
| **Temporalne (4)** | variability_index, stability_score, change_rate, volatility_index | Zmienność %, stabilność %, szybkość zmian, niestabilność |
| **Korelacje (1)** | comfort_zone_percent | Procent czasu w strefie komfortu |
| **Progi (2)** | co2_warning_level, voc_alert_level | Poziom ostrzeżenia (0-normalny, 1-ostrzeżenie, 2-krytyczny) |

**Interpretacja poziomów jakości powietrza:**
- **Level 1 (iaq_index < 100)**: Dobra jakość - warunki optymalne dla pszczół
- **Level 2 (100-200)**: Średnia jakość - monitorować, rozważyć wentylację
- **Level 3 (200-300)**: Zła jakość - wymagana interwencja (wentylacja)
- **Level 4 (> 300)**: Bardzo zła - krytyczne, natychmiastowa reakcja

**Alerty:**
- `poor_ventilation_alert`: CO2 > 2500 ppm - słaba wentylacja ula
- `high_co2_alert`: CO2 > 5000 ppm - krytyczne stężenie CO2
- `contamination_risk`: VOC > 300 - możliwe zanieczyszczenie chemiczne
- `mold_risk`: Wilgotność > 75% - ryzyko rozwoju pleśni

### Komendy sterujące
```
GET http://192.168.1.100:8080/api/cmd?SET_RELAYS:255
GET http://192.168.1.100:8080/api/cmd?SET_HEATER:128
GET http://192.168.1.100:8080/api/cmd?SET_FAN:200
GET http://192.168.1.100:8080/api/cmd?SET_PUMP:100
GET http://192.168.1.100:8080/api/cmd?CALIB_WEIGHT
GET http://192.168.1.100:8080/api/cmd?GET_AQ_STATUS  // Status jakości powietrza (Serial)
```

## 🔧 Kalibracja wagi

1. Upewnij się, że ula jest pusta
2. Wyślij komendę: `CALIB_WEIGHT`
3. System zapisze nowy offset zerowy

## 🛠️ Debugowanie

### Monitor portu szeregowego
- Baudrate: **115200**
- Pokazuje inicjalizację, odczyty sensorów i heartbeat

### Typowe problemy

#### W6100 nie wykryty
```
W6100: Nie wykryto sprzętu!
```
**Rozwiązanie:** Sprawdź połączenia SPI (GP5-GP8) i zasilanie 3.3V

#### Brak linku Ethernet
```
W6100: Kabel niepodłączony
```
**Rozwiązanie:** Podłącz kabel Ethernet, sprawdź diody na module W6100

#### SGP41 nie wykryty
```
SGP41 nie wykryty! Sprawdź połączenia I2C.
```
**Rozwiązanie:** Sprawdź połączenia I2C (GP0/GP1) i rezystory pull-up (4.7kΩ). Upewnij się, że używasz GP0 i GP1, a NIE GP2/GP3!

## 📊 Struktura kodu

```
setup()
├── Inicjalizacja Serial USB
├── Inicjalizacja W6100 (SPI1)
├── Start serwera HTTP (port 8080)
├── Inicjalizacja DHT22
├── Inicjalizacja SGP41 (I2C)
├── Inicjalizacja LD2410B (UART1)
├── Kalibracja HX711
├── Inicjalizacja modułu jakości powietrza (AQ Buffer, progi, metryki)
└── Konfiguracja GPIO wyjść

loop()
├── handleServer() - obsługa żądań HTTP
│   ├── GET /api/data - dane z sensorów
│   ├── GET /api/aq - jakość powietrza (28+ parametrów)
│   └── GET /api/cmd?XXX - komendy sterujące
├── readAllSensors() co 1s
│   ├── DHT22 (temp/hum)
│   ├── HX711 (waga)
│   ├── MEMS Mic (audio RMS)
│   ├── Piezo (wibracje)
│   ├── SGP41 (CO2/VOC)
│   └── LD2410B (ruch)
├── addAirQualityData() co 10min - buforowanie danych AQ
├── calculateAirQualityMetrics() co 10min - obliczanie 28+ parametrów
├── checkAirQualityAlerts() co 10min - detekcja zagrożeń
└── Heartbeat co 10s
```

## ⚡ Zużycie pamięci

- **Flash:** ~45% (RP2040 ma 2MB)
- **RAM:** ~35% (264KB SRAM)
- **EEPROM:** Emulowana w Flash (512 bajtów)

## 🔒 Watchdog Timer

Kod używa hardware watchdog RP2040 (automatyczny reset przy zawieszeniu).

## 📝 Licencja

MIT License - użyj zgodnie z potrzebami.
