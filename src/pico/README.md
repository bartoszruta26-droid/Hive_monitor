# 🍯 ApiaryGuard Pro - Firmware dla Raspberry Pi Pico

## 📋 Opis

Firmware **ApiaryGuard Pro** dla mikrokontrolera **Raspberry Pi Pico (RP2040)** to zaawansowane oprogramowanie do kompleksowego monitoringu pasieki. System obsługuje wiele sensorów środowiskowych i efektorów, zapewniając pełną kontrolę nad warunkami w ulu.

## 🔧 Środowisko Programistyczne

- **Platforma**: Arduino IDE
- **Core**: Raspberry Pi RP2040 Boards
- **Język**: C++
- **Port komunikacyjny**: USB-C lub UART

## 📦 Wymagane Biblioteki

Zainstaluj następujące biblioteki przez Library Manager w Arduino IDE:

```
Ethernet (by Arduino)
Wire (by Arduino)
DHT sensor library (by Adafruit)
Adafruit SGP41 (by Adafruit)
Adafruit BMP280 (by Adafruit)
Adafruit BH1750 (by Adafruit)
FFT (by Arduino)
```

## 🔌 Połączenia Sprzętowe (Pinout Pico)

### Moduł Ethernet W6100 (SPI1)
| Pin W6100 | GPIO Pico | Opis |
|-----------|-----------|------|
| CS | GPIO 9 | Chip Select |
| MOSI | GPIO 11 | Master Out Slave In |
| MISO | GPIO 12 | Master In Slave Out |
| SCK | GPIO 10 | Clock |
| RST | GPIO 8 | Reset |
| INT | GPIO 13 | Interrupt (opcjonalny) |

### Radar MMWave LD2410B (UART1)
| Pin Radar | GPIO Pico | Opis |
|-----------|-----------|------|
| TX | GPIO 4 | RX1 |
| RX | GPIO 5 | TX1 |
| VCC | 5V | Zasilanie |
| GND | GND | Masa |

### Sensory I2C (SGP41/BME688/BMP280/BH1750) (I2C0)
| Pin Sensora | GPIO Pico | Opis |
|-------------|-----------|------|
| SDA | GPIO 0 | Data |
| SCL | GPIO 1 | Clock |
| VCC | 3.3V | Zasilanie |
| GND | GND | Masa |

### DHT22 (Temperatura + Wilgotność)
| Pin DHT22 | GPIO Pico | Opis |
|-----------|-----------|------|
| DATA | GPIO 2 | Dane |
| VCC | 3.3V | Zasilanie |
| GND | GND | Masa |

### HX711 (Waga Tensometryczna)
| Pin HX711 | GPIO Pico | Opis |
|-----------|-----------|------|
| DT | GPIO 3 | Data |
| SCK | GPIO 22 | Clock |
| VCC | 5V | Zasilanie |
| GND | GND | Masa |

### MEMS Microphone & Piezo (ADC)
| Sensor | GPIO Pico | Opis |
|--------|-----------|------|
| MIC | GPIO 26 | ADC0 |
| PIEZO | GPIO 27 | ADC1 |

### Efektory (PWM/GPIO)
| Efektor | GPIO Pico | Opis |
|---------|-----------|------|
| Grzałka PWM | GPIO 6 | PWM Output |
| Wentylator PWM | GPIO 7 | PWM Output |
| Pompa perystaltyczna | GPIO 14 | Relay |
| Zawór elektromagnetyczny 1 | GPIO 15 | Solenoid |
| Zawór elektromagnetyczny 2 | GPIO 16 | Solenoid |
| Przekaźnik CH1-CH8 | GPIO 17-24 | Relay Module |
| LED Status | GPIO 25 | WBUDOWANY LED |

## 📊 Obsługiwane Sensory

| Sensor | Typ | Parametry |
|--------|-----|-----------|
| **HX711 + Strain Gauge** | Waga | 80 parametrów (waga, trendy, prognozy) |
| **MEMS Microphone** | Audio | 47+ parametrów akustycznych (FFT, MFCC) |
| **DHT22** | Temp/Wilg | 28 parametrów środowiskowych |
| **SGP41** | Jakość powietrza | 24 parametry (CO₂, VOC, NOx) |
| **LD2410B** | Radar MMWave | 27 parametrów ruchu i odległości |
| **Piezo** | Wibracje | 22 parametry wibracji |
| **BMP280** | Ciśnienie | 18 parametrów barometrycznych |
| **BH1750** | Światło | 17 parametrów natężenia |

**Łącznie: 263+ parametry analityczne!**

## 🚀 Funkcje Oprogramowania

### Moduły Analityczne

1. **Audio Analysis Module**
   - FFT (256 punktów)
   - Detekcja częstotliwości pszczół (80-800 Hz)
   - Wykrywanie rojenia, queen piping
   - 47+ parametrów akustycznych

2. **HX711 Weight Analysis**
   - Bufor 288 punktów (24h)
   - Filtrowanie EMA
   - Trendy 1h/4h/24h
   - Prognoza zbiorów miodu
   - 80 parametrów wagi

3. **Radar MMWave Analysis**
   - Bufor 120 punktów
   - Detekcja anomalii Z-score
   - Klasyfikacja zdarzeń
   - 27 parametrów ruchu

4. **Environmental Sensors**
   - Temperatura + Wilgotność (28 parametrów)
   - Jakość powietrza (24 parametry)
   - Wibracje (22 parametry)
   - Ciśnienie (18 parametrów)
   - Światło (17 parametrów)

### Endpointy API HTTP

System udostępnia następujące endpointy na porcie **8080**:

| Endpoint | Opis |
|----------|------|
| `GET /` | Strona statusu |
| `GET /hx711/status` | Status wagi + 80 parametrów |
| `GET /hx711/metrics` | Historia metryk wagi |
| `GET /hx711/events` | Zdarzenia wagowe |
| `GET /hx711/forecast` | Prognoza wagi |
| `GET /audio/status` | Status audio + 47 parametrów |
| `GET /audio/spectrum` | Widmo FFT |
| `GET /audio/events` | Zdarzenia akustyczne |
| `GET /radar/status` | Status radaru + 27 parametrów |
| `GET /radar/anomalies` | Anomalie radaru |
| `GET /environment/status` | Wszystkie sensory środowiskowe |
| `GET /temperature/metrics` | Metryki temp/wilg |
| `GET /airquality/metrics` | Jakość powietrza |
| `GET /vibration/metrics` | Wibrometria |
| `GET /barometric/metrics` | Ciśnienie atmosferyczne |
| `GET /light/metrics` | Natężenie światła |

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
  "weight": {
    "current_weight": 45.6,
    "trend_slope_24h": 0.85,
    "foraging_efficiency": 78.5,
    "predicted_weight_24h": 46.2
  },
  "audio": {
    "bee_activity_index": 82.0,
    "swarm_probability": 0.15,
    "dominant_frequency": 285.5
  }
}
```

## 🔨 Kompilacja i Wgrywanie

### Krok 1: Instalacja Core RP2040

W Arduino IDE:
1. Otwórz **File → Preferences**
2. Dodaj URL w **Additional Board Manager URLs**:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. Otwórz **Tools → Board → Board Manager**
4. Wyszukaj **"Raspberry Pi Pico/RP2040"** i zainstaluj

### Krok 2: Konfiguracja Płytki

```
Board: "Raspberry Pi Pico"
CPU Speed: "133 MHz"
Optimize: "Small (-Os)"
Debug Port: "Disabled"
Debug Level: "None"
USB Stack: "Pico SDK"
IP/Bluetooth Stack: "IPv4 Only"
```

### Krok 3: Kompilacja

1. Otwórz plik `apiaryguard_pico.ino`
2. Wybierz port (np. `/dev/ttyACM0`)
3. Kliknij **Verify/Compile** (Ctrl+R)
4. Kliknij **Upload** (Ctrl+U)

### Krok 4: Tryb Bootloader

Jeśli wgrywanie nie działa:
1. Odłącz Pico od USB
2. Przytrzymaj przycisk **BOOTSEL**
3. Podłącz USB (nadal trzymając BOOTSEL)
4. Zwolnij przycisk gdy pojawi się dysk **RPI-RP2**
5. Spróbuj wgrać ponownie

## ⚙️ Konfiguracja Sieci

### Statyczne IP (domyślne)

```cpp
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
```

### DHCP

Zakomentuj linie ze statycznym IP i użyj:

```cpp
Ethernet.begin(mac); // DHCP
```

## 🛡️ Bezpieczeństwo

- Watchdog timer aktywny
- Auto-restart przy zawieszeniu
- Brownout detection
- Safe mode przy błędach sensorów
- Failsafe dla efektorów (max temperatura)

## 📝 Logika Sterowania

### Przykład: Kontrola Temperatury

```cpp
if(temperature < 15.0) {
  analogWrite(HEATER_PWM, 150); // Grzanie
} else {
  analogWrite(HEATER_PWM, 0);
}
```

### Przykład: Wentylacja

```cpp
if(motion_detected && humidity > 80) {
  digitalWrite(VALVE_1, HIGH); // Wentylacja
} else {
  digitalWrite(VALVE_1, LOW);
}
```

## 🐛 Troubleshooting

| Problem | Rozwiązanie |
|---------|-------------|
| Brak połączenia Ethernet | Sprawdź kabel, ping do gateway |
| Błędy kompilacji | Zainstaluj wszystkie biblioteki |
| Sensor nie odpowiada | Sprawdź połączenia I2C (pull-up resistors) |
| Wgrywanie nie działa | Użyj trybu BOOTSEL |
| Resetowanie się | Sprawdź zasilanie (min 500mA) |

## 📈 Wydajność

- **Cykl główny**: ~100ms
- **Sampling audio**: 4kHz, 256-point FFT
- **Sampling wagi**: 10Hz z filtrowaniem
- **Aktualizacja API**: On-demand
- **Zużycie pamięci**: ~180KB Flash, ~200KB RAM

## 📄 Licencja

MIT License - zobacz główny plik LICENSE w repozytorium.

## 🤝 Współpraca

Projekt open-source. Zachęcamy do:
- Reportowania błędów (Issues)
- Propozycji funkcji (Feature Requests)
- Pull Requestów z poprawkami

## 🔗 Linki

- [Dokumentacja główna](../../docs/)
- [Nowe parametry sensorów](../../docs/14_nowe_parametry_sensorow.md)
- [API Reference](../../docs/09_api_i_integracje.md)
- [Repozytorium GitHub](https://github.com/apiaryguard/apiaryguard-pro)

---

**ApiaryGuard Pro** - Professional Beehive Monitoring System  
© 2025 ApiaryGuard Team
