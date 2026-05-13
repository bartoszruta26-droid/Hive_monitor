# ApiaryGuard - Flowing Hive Effector System

## Dokumentacja Techniczna Efektorów Automatycznego Opróżniania Ramek

### Spis treści
1. [Opis Ogólny](#opis-ogólny)
2. [Specyfikacja Mechaniczna](#specyfikacja-mechaniczna)
3. [Specyfikacja Elektroniczna](#specyfikacja-elektroniczna)
4. [Opis Kodu Źródłowego](#opis-kodu-źródłowego)
5. [Konfiguracja i Kalibracja](#konfiguracja-i-kalibracja)
6. [API i Komendy](#api-i-komendy)
7. [Rozwiązywanie Problemów](#rozwiązywanie-problemów)

---

## Opis Ogólny

System efektorów Flowing Hive to zaawansowany moduł automatyzacji procesu zbierania miodu z uli typu Flow Hive. System składa się z trzech głównych komponentów:

1. **Serwo mechanizmu obracania ramek** - automatyczne przechylanie ramek w celu spłynięcia miodu
2. **Drugi czujnik HX711** - monitorowanie wagi nadstawki ula (superstructure)
3. **Czujnik przepływu** - pomiar ilości wypływającego miodu

### Główne Funkcje

- **Automatyczne opróżnianie ramek**: Sekwencja ruchów serwo z możliwością programowania czasu trwania
- **Monitorowanie wagi w czasie rzeczywistym**: Dwa niezależne czujniki wagi (uli i nadstawki)
- **Pomiar przepływu miodu**: Dokładny pomiar objętości zebranego miodu
- **Tryb bezpieczny**: Automatyczne wyłączenie w przypadku wykrycia anomalii
- **Kompleksowe logowanie**: Szczegółowe informacje diagnostyczne

---

## Specyfikacja Mechaniczna

### 1. Serwo Mechanizmu Obracania Ramek

#### Wymagania Mechaniczne
- **Typ serwa**: Standardowe serwo RC 180° (np. SG90, MG996R)
- **Moment obrotowy**: Minimalnie 2.5 kg·cm dla ramek Flow Hive
- **Zakres ruchu**: 0-180 stopni
- **Czas reakcji**: < 0.2 sekundy na 60°

#### Montaż
```
[rama Flow Hive] ---- [uchwyt serwo] ---- [serwo SG90/MG996R]
                              |
                       [GPIO 12 - PWM]
```

#### Pozycje Serwo
| Pozycja | Kąt (°) | Opis |
|---------|---------|------|
| REST | 10° | Normalna pozycja - pszczoły mają dostęp |
| EMPTY | 160° | Pozycja opróżniania - ramki przechylone |
| MIN | 0° | Granica dolna |
| MAX | 180° | Granica górna |

### 2. Czujnik Wagi Nadstawki (HX711 #2)

#### Specyfikacja
- **Model**: HX711 (drugi moduł)
- **Przetwornik**: 24-bit ADC
- **Częstotliwość próbkowania**: 10/80 Hz
- **Maksymalne obciążenie**: Zależne od zastosowanych tensometrów (typowo 50kg)

#### Montaż Tensometrów
```
[nadstawka ula] 
      |
[4x tensometr 50kg] --- [mostek Wheatstone'a] --- [HX711 #2]
                                                      |
                                               [GPIO 10 - DT]
                                               [GPIO 11 - SCK]
```

### 3. Czujnik Przepływu

#### Specyfikacja
- **Model**: YF-S201 lub kompatybilny
- **Zakres przepływu**: 0.3-5 L/min
- **Impulsy na litr**: 450 (kalibrowalne)
- **Ciśnienie robocze**: < 1.75 MPa
- **Temperatura pracy**: -25°C do +80°C

#### Instalacja Hydrauliczna
```
[ramki Flow Hive] --> [czujnik przepływu] --> [zbiornik miodu]
                              |
                       [GPIO 25 - INT]
```

---

## Specyfikacja Elektroniczna

### Pinout Raspberry Pi Pico (RP2040)

| Komponent | GPIO | Pin Pico | Typ | Opis |
|-----------|------|----------|-----|------|
| **Serwo** | 12 | GP12 | PWM | Sterowanie kątem serwa |
| **HX711 #2 DT** | 10 | GP10 | INPUT | Dane z czujnika wagi #2 |
| **HX711 #2 SCK** | 11 | GP11 | OUTPUT | Zegar czujnika wagi #2 |
| **Flow Sensor** | 25 | GP25 | INT | Przerwanie od czujnika przepływu |
| **HX711 #1 DT** | 3 | GP3 | INPUT | Dane z czujnika wagi #1 (uli) |
| **HX711 #1 SCK** | 22 | GP22 | OUTPUT | Zegar czujnika wagi #1 |

### Połączenia Elektryczne

#### Serwo
```
Pico GPIO 12 (PWM) -----> Pomarańczowy (sygnał)
Pico 5V VOUT ----------> Czerwony (zasilanie)
Pico GND --------------> Brązowy (masa)
```

**UWAGA**: Dla serw o wysokim momencie obrotowym (MG996R) zalecane jest zewnętrzne zasilanie 5V/2A.

#### HX711 #2 (Nadstawka)
```
Pico GPIO 10 ---------> DT (Data)
Pico GPIO 11 ---------> SCK (Clock)
Pico 5V ---------------> VCC
Pico GND --------------> GND
```

#### Czujnik Przepływu YF-S201
```
Pico GPIO 25 ---------> Żółty (sygnał)
Pico 5V ---------------> Czerwony (zasilanie)
Pico GND --------------> Czarny (masa)
```

### Zabezpieczenia

1. **Separacja galwaniczna**: Zalecana dla czujników przepływu
2. **Filtracja zasilania**: Kondensator 100µF przy każdym module HX711
3. **Pull-up resistor**: 10kΩ na linii sygnałowej czujnika przepływu
4. **Ochrona przed przepięciem**: Diody TVS na liniach GPIO

---

## Opis Kodu Źródłowego

### Struktura Plików

```
src/pico_refactored/
├── include/
│   ├── config.h                    # Konfiguracja pinów i stałych
│   ├── effectors_flowing_hive.h    # Deklaracje funkcji efektorów
│   └── ...
├── src/
│   ├── effectors_flowing_hive.cpp  # Implementacja efektorów
│   └── ...
└── apiaryguard_pico.ino            # Główny plik programu
```

### Kluczowe Stałe (config.h)

```cpp
// Piny GPIO
#define SERVO_EMPTY_PIN     12    // PWM dla serwa
#define HX711_2_DT          10    // Data drugiego HX711
#define HX711_2_SCK         11    // Clock drugiego HX711
#define FLOW_SENSOR_PIN     25    // Przerwanie czujnika przepływu

// Stałe serwo
#define SERVO_MIN_ANGLE     0
#define SERVO_MAX_ANGLE     180
#define SERVO_EMPTY_ANGLE   160   // Pozycja opróżniania
#define SERVO_REST_ANGLE    10    // Pozycja spoczynku
#define SERVO_PULSE_WIDTH_MIN   500   // µs
#define SERVO_PULSE_WIDTH_MAX   2500  // µs

// Czujnik przepływu
#define FLOW_SENSOR_PULSES_PER_LITER  450
#define FLOW_SAMPLE_INTERVAL_MS       1000
#define FLOW_MIN_RATE                 0.01f  // L/min
#define FLOW_MAX_RATE                 5.0f   // L/min
```

### Funkcje API - Serwo

#### `void initServoControl()`
Inicjalizuje sterowanie serwem:
- Konfiguruje GPIO 12 jako wyjście PWM
- Ustawia częstotliwość 50Hz (okres 20ms)
- Inicjalizuje pozycję spoczynkową

#### `void setServoAngle(uint16_t angle)`
Ustawia kąt serwa:
- **Parametr**: `angle` - kąt w stopniach (0-180)
- Walidacja zakresu
- Konwersja na sygnał PWM (500-2500µs)
- Blokada w trybie safe mode

#### `void executeAutoEmptySequence(unsigned long duration_ms)`
Uruchamia sekwencję automatycznego opróżniania:
1. Przejście do pozycji EMPTY (160°)
2. Oczekiwanie przez określony czas
3. Powrót do pozycji REST (10°)

**Przykład użycia:**
```cpp
// Opróżnianie przez 30 minut
executeAutoEmptySequence(1800000);
```

### Funkcje API - HX711 #2

#### `void initHX711_2()`
Inicjalizuje drugi czujnik wagi:
- Konfiguruje piny DT i SCK
- Wykonuje test komunikacji
- Ustawia flagę initialized

#### `long readHX711_2()`
Odczytuje surową wartość z przetwornika:
- 24-bitowy odczyt z timeout
- Walidacja zakresu
- Obsługa błędów komunikacji

#### `float getSuperstructureWeightGrams()`
Zwraca skalowaną wagę w gramach:
- Aplikuje offset (tare) i współczynnik skali
- Zwraca 0.0f jeśli czujnik niezinicjalizowany

#### `void tareHX711_2()`
Wykonuje kalibrację zerową (tare):
- Pobiera 10 średnich odczytów
- Ustawia offset dla zerowej wagi

### Funkcje API - Czujnik Przepływu

#### `void initFlowSensor()`
Inicjalizuje czujnik przepływu:
- Konfiguruje GPIO 25 jako wejście z pull-up
- Podłącza przerwanie na zboczu narastającym
- Resetuje liczniki

#### `void updateFlowSensor()`
Aktualizuje obliczenia przepływu (wywoływać co 1s):
- Oblicza przepływ z liczby impulsów
- Aktualizuje całkowitą objętość
- Waliduje zakres przepływu

**Formuła:**
```
Flow Rate (L/min) = (pulses / PULSES_PER_LITER) / time_minutes
```

#### `float getFlowRateLPM()`
Zwraca aktualny przepływ w L/min.

#### `float getTotalVolumeLiters()`
Zwraca całkowitą objętość miodu od ostatniego resetu.

#### `bool isHoneyFlowing()`
Sprawdza czy miód aktualnie wypływa:
- Zwraca `true` jeśli przepływ >= FLOW_MIN_RATE

### Tryb Bezpieczny (Safe Mode)

#### `void activateFlowingHiveSafeMode(const char* reason)`
Aktywuje tryb bezpieczny:
- Natychmiast przesuwa serwo do pozycji REST
- Wyłącza sekwencje automatyczne
- Loguje powód aktywacji

**Przyczyny aktywacji:**
- Wykrycie anomalii w przepływie
- Błąd czujnika wagi
- Przeciążenie systemu
- Niski poziom zasilania

---

## Konfiguracja i Kalibracja

### Kalibracja HX711 #2

1. **Montaż bez obciążenia**
```cpp
void setup() {
    initHX711_2();
    delay(2000);  // Nagrzewanie
    tareHX711_2();  // Zerowanie
}
```

2. **Wyznaczanie współczynnika skali**
```cpp
// Umieść znany ciężar (np. 5000g)
long raw = readHX711_2();
float scale = (raw - offset) / 5000.0;
// Zapisz scale do EEPROM
```

### Kalibracja Czujnika Przepływu

1. **Metoda objętościowa:**
```cpp
resetFlowCounter();
// Przepuść dokładnie 1 litr wody
delay(10000);
float measured = getTotalVolumeLiters();
// Skoryguj FLOW_SENSOR_PULSES_PER_LITER
// Nowa wartość = 450 * (measured / 1.0)
```

### Testowanie Serwo

```cpp
void testServo() {
    initServoControl();
    
    // Test pełnego zakresu
    setServoAngle(0);
    delay(1000);
    setServoAngle(90);
    delay(1000);
    setServoAngle(180);
    delay(1000);
    setServoRestPosition();
}
```

---

## API i Komendy

### Komendy Serial/USB

Dostępne przez interfejs USB lub Ethernet:

| Komenda | Opis | Przykład |
|---------|------|----------|
| `SERVO_ANGLE [0-180]` | Ustaw kąt serwa | `SERVO_ANGLE 160` |
| `SERVO_REST` | Pozycja spoczynkowa | `SERVO_REST` |
| `SERVO_EMPTY` | Pozycja opróżniania | `SERVO_EMPTY` |
| `AUTO_EMPTY [ms]` | Start sekwencji | `AUTO_EMPTY 1800000` |
| `TARE_WEIGHT2` | Zeruj HX711 #2 | `TARE_WEIGHT2` |
| `WEIGHT2` | Odczytaj wagę #2 | `WEIGHT2` |
| `FLOW_RESET` | Reset licznika | `FLOW_RESET` |
| `FLOW_STATUS` | Status przepływu | `FLOW_STATUS` |
| `STATUS_ALL` | Pełny status | `STATUS_ALL` |

### Przykładowa Sesja

```
> STATUS_ALL

>> Flowing Hive Effector Status:
  Servo:
    Initialized: YES
    Current angle: 10 degrees
    Auto-empty active: NO
    Operations: 15, Errors: 0
  HX711 #2 (Superstructure):
    Initialized: YES
    Weight: 12500.5 g
    Reads: 1250, Errors: 2
  Flow Sensor:
    Initialized: YES
    Flow rate: 0.350 L/min
    Total volume: 2.450 L
    Honey flowing: YES
    Updates: 125, Errors: 0
  Safe mode: inactive

> AUTO_EMPTY 600000
OK: Started auto-empty sequence (600000 ms)

> FLOW_STATUS
Flow: 0.420 L/min, Total: 2.870 L
```

---

## Rozwiązywanie Problemów

### Problemy z Serwo

| Objaw | Możliwa Przyczyna | Rozwiązanie |
|-------|-------------------|-------------|
| Serwo drży | Złe zasilanie | Dodaj kondensator 1000µF |
| Nie osiąga kąta | Za niskie napięcie | Sprawdź zasilanie 5V |
| Brak reakcji | Uszkodzone GPIO | Sprawdź połączenia |

### Problemy z HX711 #2

| Objaw | Możliwa Przyczyna | Rozwiązanie |
|-------|-------------------|-------------|
| Timeout | Źle podłączone piny | Sprawdź DT/SCK |
| Losowe wartości | Zakłócenia EMI | Ekranuj przewody |
| Dryft zera | Zmiana temperatury | Wykonaj ponowne tare |

### Problemy z Czujnikiem Przepływu

| Objaw | Możliwa Przyczyna | Rozwiązanie |
|-------|-------------------|-------------|
| Brak impulsów | Zablokowany wirnik | Oczyść czujnik |
| Zawyżony przepływ | Błędna kalibracja | Przekalibruj PULSES_PER_LITER |
| Szumy | Zakłócenia | Dodaj filtr RC |

### Diagnostyka przez Serial

```cpp
// Włącz debugowanie w config.h:
#define DEBUG_SERVO
#define DEBUG_FLOW
#define DEBUG_HX711

// Uruchom ponownie i obserwuj output:
[SERVO] Initialized on GPIO 12
[FLOW] Initialized on GPIO 25 (interrupt-enabled)
[HX711_2] Initialized successfully (test reading: 123456)
```

---

## Integracja z Głównym Programem

### Modyfikacja `apiaryguard_pico.ino`

Dodaj do sekcji include:
```cpp
#include "effectors_flowing_hive.h"
```

W `setup()`:
```cpp
initServoControl();
initHX711_2();
initFlowSensor();
```

W `loop()`:
```cpp
updateServoLoop();
updateFlowSensor();

// Co 60 sekund raportuj status
static unsigned long lastStatus = 0;
if (millis() - lastStatus > 60000) {
    printFlowingHiveEffectorStatus();
    lastStatus = millis();
}
```

---

## Bezpieczeństwo i konserwacja

### Środki Ostrożności

1. **Zasilanie**: Odłącz przed serwisowaniem
2. **Pszoły**: Pracuj w godzinach wieczornych/nocnych
3. **Czystość**: Regularnie czyść czujnik przepływu
4. **Kalibracja**: Sprawdzaj raz w sezonie

### Harmonogram Konserwacji

| Częstotliwość | Zadanie |
|---------------|---------|
| Przed sezonem | Kalibracja HX711, test serwo |
| Co 2 tygodnie | Czyszczenie czujnika przepływu |
| Po sezonie | Dezynfekcja, storage |

---

## Historia Wersji

| Wersja | Data | Zmiany |
|--------|------|--------|
| 1.0.0 | 2024-01 | Pierwsza implementacja |
| 1.1.0 | 2024-02 | Dodano safe mode |
| 1.2.0 | 2024-03 | Poprawiona kalibracja |

---

## Kontakt i Wsparcie

Projekt ApiaryGuard - Open Source Beekeeping Monitoring System

**Licencja**: MIT License  
**Repozytorium**: GitHub  
**Dokumentacja**: `/workspace/doc/`

