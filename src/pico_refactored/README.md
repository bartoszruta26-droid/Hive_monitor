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
