# ApiaryGuard Pico - Refactored Firmware

## Struktura projektu

```
pico_refactored/
├── apiaryguard_pico.ino      # Główny plik (116 linii)
├── include/                   # Nagłówki
│   ├── config.h              # Konfiguracja pinów i stałe
│   ├── sensors.h             # Wykrywanie sensorów
│   ├── hx711.h               # Waga HX711
│   ├── audio_analysis.h      # Analiza audio
│   ├── weight_analysis.h     # Analiza wagi
│   ├── air_quality.h         # Jakość powietrza SGP41
│   ├── radar_analysis.h      # Radar LD2410B
│   ├── network.h             # Ethernet W6100
│   └── effectors.h           # Sterowanie wykonawcami
└── src/                      # Implementacje
    ├── sensors.cpp           # 216 linii
    ├── hx711.cpp             # 65 linii
    ├── audio_analysis.cpp    # 173 linie
    ├── weight_analysis.cpp   # 191 linii
    ├── air_quality.cpp       # 150 linii
    ├── radar_analysis.cpp    # 212 linii
    ├── network.cpp           # 140 linii
    └── effectors.cpp         # 38 linii
```

**Łącznie: ~1740 linii** (vs. 4700+ w oryginalnym monolicie)

## Kluczowe poprawki

### 1. HX711 - Stabilna implementacja
- Timeout 50ms przy oczekiwaniu na dane
- Feed watchdog podczas czekania
- DelayMicroseconds(1) dla stabilności na 133 MHz
- Error logging z throttlingiem

### 2. Sieć - DHCP Fallback
```cpp
#if USE_DHCP_FALLBACK
    if (Ethernet.begin(macAddress)) {
        // DHCP成功了
    } else {
        // Fallback do statycznego IP
        Ethernet.begin(macAddress, staticIP, gateway, subnet);
    }
#endif
```

### 3. Watchdog Timer
- Włączony w setup(): `rp2040.wdtEnable(8000)`
- Resetowany w każdej iteracji loop(): `rp2040.wdtReset()`
- Timeout: 8 sekund

### 4. Wykrywanie sensorów
- Dynamiczne wykrywanie przy starcie
- Flagi `detected` i `active` dla każdego sensora
- Warunkowe czytanie tylko wykrytych sensorów
- Retry mechanizm dla DHT22 (3 próby)

### 5. Fix konfliktu pinów
- GPIO26/28 zmienione na ADC-only
- RELAY_CH6 przeniesiony z GPIO26 na GPIO28

### 6. Zarządzanie pamięcią
- Zredukowane struktury danych (mniej pól)
- Bufory o rozsądnych rozmiarach
- Unika się alokacji dynamicznej

## Kompilacja

Wymagane biblioteki Arduino:
- Raspberry Pi RP2040 Boards
- Ethernet (W6100)
- DHT sensor library
- Adafruit SGP41 Library
- Sensirion Gas Index Algorithm (VOC/NOx index calculation)

## Użycie

1. Skonfiguruj piny w `include/config.h`
2. Ustaw adresację sieciową (DHCP lub statyczne)
3. Skalibruj HX711 (tare i scale)
4. Wgraj na Pico

## Debugowanie

Odkomentuj definicje w config.h:
```cpp
#define DEBUG_AUDIO
#define DEBUG_WEIGHT
#define DEBUG_AIR
#define DEBUG_RADAR
```

## SGP41 data semantics

The SGP41 supplies raw VOC and NOx signals; the firmware converts them to VOC and NOx indices using Sensirion Gas Index Algorithm. The SGP41 does **not** measure CO2 or eCO2. The API therefore exposes `co2_available=false` and `co2=null` rather than reporting a fabricated CO2 value. A separate CO2-capable sensor is required if CO2 data is needed.

## Actuator interlocks and status reporting

The read-only JSON status endpoints also expose:
- `pump`, `relay1`, and `relay2` as current commanded states.
- `safe_mode` for the main actuator interlock.
- `flowing_hive_safe_mode` for the independent Flowing Hive servo interlock.

The main safe-mode fault reason is latched and exposed through the local USB `SAFE_MODE_STATUS` command. Recovery requires `SAFE_MODE_RELEASE CONFIRM`, a detected and active temperature sensor, fewer than the configured consecutive invalid samples, and a fresh valid sample. Release clears remembered heater, fan, pump, and relay 1/2 setpoints and forces all relay outputs LOW. The operator must issue fresh commands; previous actuator requests are never resumed automatically. The Flowing Hive latch and its fault reason remain independent until separately released with `FLOW_SAFE_MODE_RELEASE CONFIRM`; release is rejected while the main latch remains active. Interrupted auto-empty sequences are never resumed.

These software interlocks are not a substitute for independent hardware protection. In particular, a DHT22 ambient reading is not a heater-surface measurement. A separately mounted heater sensor and independent thermal fuse/thermostat are required for a heater installation. Verify relay polarity, fan behavior, and all inactive states on the actual hardware before connecting loads.

## HTTP read-only endpoints

- `GET /` and `GET /index.html` return the read-only HTML dashboard.
- `GET /status` and `GET /api/status` return JSON sensor status.
- Other paths return 404; methods other than GET return 405.
- The request line is limited to 95 bytes, the total headers to 512 bytes, and the connection has a 5-second timeout.
- HTTP actuator controls are intentionally absent. Actuator changes are accepted only through the locally connected USB command interface.
