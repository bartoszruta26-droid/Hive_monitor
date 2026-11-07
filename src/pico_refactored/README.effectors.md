# ApiaryGuard - Flowing Hive Effector System

## Dokumentacja Techniczna Efektorów Automatycznego Opróżniania Ramek

### Spis treści
1. [Opis Ogólny](#opis-ogólny)
2. [Specyfikacja Mechaniczna](#specyfikacja-mechaniczna)
3. [Specyfikacja Elektroniczna](#specyfikacja-elektroniczna)
4. [Opis Kodu Źródłowego](#opis-kodu-źródłowego)
5. [Gentle Code Principles](#gentle-code-principles)
6. [Error Handling Strategy](#error-handling-strategy)
7. [Debug Log System](#debug-log-system)
8. [Konfiguracja i Kalibracja](#konfiguracja-i-kalibracja)
9. [API i Komendy](#api-i-komendy)
10. [Rozwiązywanie Problemów](#rozwiązywanie-problemów)

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
- **Tryb bezpieczny**: Automatyczne wyłączenie w przypadku wykryciu anomalii
- **Kompleksowe logowanie**: Szczegółowe informacje diagnostyczne
- **Hardware Detection**: Weryfikacja dostępności sterownika; nie jest to fizyczne wykrywanie serwa
- **Gentle Code**: Brak crashów - graceful degradation przy błędach

---

## Gentle Code Principles

System Flowing Hive implementuje filozofię **Gentle Code** - kodu odpornego na błędy, który nie powoduje crashów systemu.

### Zasady Gentle Code

1. **Hardware Detection Before Operation**
   - Każda funkcja sprawdza flagi `*Detected` przed wykonaniem operacji
   - Jeśli hardware nie jest wykryty - funkcja zwraca gracefully bez błędu
   - System kontynuuje działanie z pominięciem niedostępnego komponentu

2. **Input Validation with Graceful Degradation**
   - Wszystkie parametry wejściowe są walidowane
   - Nieprawidłowe wartości są automatycznie korygowane (constrain)
   - Zamiast exceptionów - logowanie warningów

3. **No Exceptions, Only Return Codes**
   - Brak rzucania wyjątków w kodzie embedded
   - Funkcje zwracają 0 lub false w przypadku błędu
   - Error counters do monitorowania zdrowia systemu

4. **Safe Mode Automatic Activation**
   - Przy krytycznych błędach aktywuje się zatrzask trybu bezpiecznego
   - Sterownik otrzymuje polecenie pozycji REST, o ile jest gotowy; brak sprzężenia zwrotnego oznacza, że ruch mechaniczny nie jest potwierdzony
   - Sekwencje automatyczne są przerywane i nie są automatycznie wznawiane

### Przykład: Sprawdzenie Hardware Before Operation

```cpp
bool setServoAngle(uint16_t angle) {
    // GENTLE CODE: Check hardware presence before operation
    if (!servoDetected || !servoInitialized) {
        DBG_SERVO("[SERVO] Command ignored - servo not detected/initialized\n");
        return;  // Graceful exit - no error thrown
    }
    
    // GENTLE CODE: Respect safe mode
    if (flowingHiveSafeMode) {
        DBG_SERVO("[SERVO] Command ignored - safe mode active\n");
        return false;  // Blocked for safety
    }
    
    // ... rest of operation
}
```

---

## Error Handling Strategy

### Poziomy Obsługi Błędów

| Poziom | Opis | Akcja |
|--------|------|-------|
| **INFO** | Normalne operacje | Logowanie debug |
| **WARNING** | Nieprawidłowe parametry | Auto-korekta + log |
| **ERROR** | Błąd hardware/timeout | Increment error counter |
| **CRITICAL** | Wielokrotne błędy | Activate safe mode |

### Timeout Protection

Wszystkie operacje I/O mają zabezpieczenie timeout:

```cpp
// HX711 read with timeout
unsigned long start_time = millis();
while(digitalRead(HX711_2_DT)) {
    if (millis() - start_time > HX711_2_TIMEOUT_MS) {
        hx711_2_error_count++;
        return 0;  // Timeout - graceful failure
    }
    delayMicroseconds(10);
}
```

### Validation Ranges

Każdy sensor reading jest walidowany:

```cpp
// Validate HX711 reading
if (count < HX711_2_MIN_VALID_VALUE || count > HX711_2_MAX_VALID_VALUE) {
    hx711_2_error_count++;
    return 0;  // Invalid reading rejected
}
```

### Error Counters

System zbiera statystyki błędów dla każdego komponentu:

- `servo_op_count` / `servo_error_count`
- `hx711_2_read_count` / `hx711_2_error_count`
- `flow_read_count` / `flow_error_count`

Dostępne przez komendę `STATUS_FLOWING`.

---

## Debug Log System

### Poziomy Debugowania

Zdefiniowane w `config.h`:

| Macro | Opis | Komponent |
|-------|------|-----------|
| `DEBUG_SERVO` | Logowanie operacji serwa | Servo control |
| `DEBUG_FLOW` | Logowanie czujnika przepływu | Flow sensor |
| `DEBUG_HX711` | Logowanie wag | HX711 #1 & #2 |
| `DEBUG_VERBOSE` | Szczegółowe trace enter/exit | All modules |
| `DEBUG_PERF` | Monitoring czasu wykonania | Performance |

### Przykłady Outputu Debug

**Serwo Initialization:**
```
[SERVO] Initialized on GPIO 12
[SERVO] Rest position: 10 degrees
[SERVO] PWM: 50Hz, pulse width 500-2500 us
```

**Flow Sensor Update:**
```
[FLOW] Initialized on GPIO 25 (interrupt-enabled)
[FLOW] Pulses per liter: 450
[FLOW] Rate: 0.125 L/min, Total: 1.250 L
```

**Error Logging:**
```
[ERROR] HX711_2: Initialization failed - sensor not detected
[WARN] SERVO: Assertion failed (non-fatal): Invalid angle 200
```

### Trace Enter/Exit (DEBUG_VERBOSE)

Przy pełnym debugowaniu każda funkcja loguje wejście/wyjście:
```
[TRACE] ENTER: initServoControl
[TRACE] EXIT: initServoControl
```

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

#### `bool setServoAngle(uint16_t angle)`
Ustawia kąt serwa:
- **Parametr**: `angle` - kąt w stopniach (0-180)
- Walidacja zakresu
- Sterowanie przez wbudowaną w rdzeń Arduino-Pico bibliotekę `Servo`, wykorzystującą PIO do generowania impulsów z okresem 20 ms (50 Hz).
- Zakres impulsów jest ustawiany przez `SERVO_PULSE_WIDTH_MIN/MAX` (obecnie 500-2500 µs); wymaga kalibracji dla konkretnego modelu serwa i mechanizmu.
- Blokada w trybie safe mode
- Błąd przydzielenia kanału PIO powoduje odrzucenie poleceń ruchu. Flaga oznacza gotowość sterownika, a nie fizyczne wykrycie serwa.



**Uwaga sprzętowa:** biblioteka `Servo` w rdzeniu Arduino-Pico wykorzystuje PIO i generuje przebieg w tle, niezależnie od obciążenia pętli głównej. Nie potwierdza to jednak prawidłowości mechanicznego zakresu ruchu. Pierwsze uruchomienie należy wykonać bez obciążenia mechanizmu, z ograniczonym zakresem impulsów i kontrolą poboru prądu.

#### `bool executeAutoEmptySequence(unsigned long duration_ms)`
Uruchamia sekwencję automatycznego opróżniania:
1. Sprawdza tryb bezpieczny i dozwolony czas pracy.
2. Przechodzi do pozycji EMPTY (160°).
3. Oczekuje przez określony czas.
4. Wraca do pozycji REST (10°).

**Ograniczenie czasu:** od 1 do 1800 sekund (maksymalnie 30 minut). Wartości spoza zakresu są odrzucane bez uruchomienia serwa.

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

Dostępne komendy sterujące i diagnostyczne są przyjmowane przez lokalny port USB Serial. Panel HTTP/Ethernet w tej wersji udostępnia wyłącznie odczyt statusu; nie należy traktować go jako interfejsu sterowania.

| Komenda | Opis | Przykład |
|---------|------|----------|
| `SERVO_ANGLE [0-180]` | Ustaw kąt serwa, jeśli blokada Flowing Hive jest zwolniona | `SERVO_ANGLE 160` |
| `SERVO_EMPTY [1-1800]` | Uruchom sekwencję opróżniania na podaną liczbę sekund; bez argumentu 1800 s | `SERVO_EMPTY 600` |
| `FLOW_RESET` | Reset licznika przepływu | `FLOW_RESET` |
| `STATUS_FLOWING` | Status sterownika, sekwencji i przepływu | `STATUS_FLOWING` |
| `SAFE_MODE_STATUS` | Status obu zatrzasków i przyczyny ostatnich błędów | `SAFE_MODE_STATUS` |
| `SAFE_MODE_RELEASE CONFIRM` | Zwolnij główną blokadę po potwierdzeniu operatora i sprawdzeniu czujnika temperatury | `SAFE_MODE_RELEASE CONFIRM` |
| `FLOW_SAFE_MODE_RELEASE INSPECTED CONFIRM` | Zwolnij blokadę Flowing Hive po fizycznej kontroli pozycji REST i zwolnieniu głównej blokady | `FLOW_SAFE_MODE_RELEASE INSPECTED CONFIRM` |
| `HELP` | Lista aktualnych komend | `HELP` |

### Przykładowa Sesja

```
> STATUS_FLOWING

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

> SERVO_EMPTY 600
OK: Rozpoczęto sekwencję opróżniania (600s).

> SAFE_MODE_STATUS
MAIN_SAFE_MODE=INACTIVE
FLOWING_HIVE_SAFE_MODE=INACTIVE
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



### Ograniczenia odzyskiwania i potwierdzania stanu

Sterownik serwa nie ma czujnika położenia. `Controller ready`, `Initialized`, `Rest command accepted by driver` oraz `Current angle` opisują stan sterownika i ostatnie polecenie, a nie zmierzone położenie mechanizmu. Przed wydaniem `FLOW_SAFE_MODE_RELEASE INSPECTED CONFIRM` operator musi fizycznie sprawdzić, czy mechanizm znajduje się w bezpiecznej, dostępnej dla pszczół pozycji REST. Potwierdzenie jest oświadczeniem operatora, nie wynikiem automatycznego pomiaru.

Główna blokada i blokada Flowing Hive są niezależne. Zwolnienie głównej blokady nie zwalnia blokady Flowing Hive. Zwolnienie Flowing Hive jest odrzucane, gdy główna blokada pozostaje aktywna. Po odzyskaniu systemu wcześniejsze polecenia i przerwane sekwencje nie są automatycznie odtwarzane.

Przed wdrożeniem wymagane są testy na docelowym sprzęcie: polaryzacji przekaźników, zachowania wyjść po resecie i utracie zasilania, kalibracji zakresu serwa bez obciążenia, rzeczywistego położenia REST oraz niezależnych zabezpieczeń termicznych. Testy hostowe i kompilacja firmware nie potwierdzają bezpieczeństwa mechanicznego ani elektrycznego.
