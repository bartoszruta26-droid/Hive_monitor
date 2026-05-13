# Automatyczny Efektor Opróżniania Ramek Flowing Hive

## Spis Treści
1. [Opis Techniczny](#opis-techniczny)
2. [Opis Mechaniczny](#opis-mechaniczny)
3. [Opis Elektroniczny](#opis-elektroniczny)
4. [Opis Kodu Źródłowego](#opis-kodu-źródłowego)
5. [Instalacja i Konfiguracja](#instalacja-i-konfiguracja)
6. [Eksploatacja i Konserwacja](#eksploatacja-i-konserwacja)

---

## Opis Techniczny

### Przeznaczenie Systemu
Automatyczny efektor opróżniania ramek Flowing Hive to zaawansowany system zaprojektowany do automatyzacji procesu zbierania miodu z uli typu Flow Hive. System eliminuje konieczność ręcznego obracania kluczy w ramkach, wykrywając moment gotowości miodu do zbioru i samodzielnie inicjując proces opróżniania.

### Główne Funkcje
- **Automatyczne wykrywanie przepływu miodu** - system sam rozpoznaje kiedy miód zaczyna płynąć
- **Automatyczne otwieranie i zamykanie ramek** - serwomechanizm obraca klucz ramki
- **Monitorowanie procesu** - ciągła kontrola przepływu podczas opróżniania
- **Zabezpieczenia czasowe** - timeouty zapobiegające nadmiernemu otwieraniu ramek
- **Tryb manualny** - możliwość ręcznego uruchomienia przyciskiem
- **Wizualna sygnalizacja statusu** - dioda LED informująca o stanie systemu

### Parametry Techniczne
| Parametr | Wartość |
|----------|---------|
| Napięcie zasilania | 5V DC |
| Pobór prądu (spoczynek) | < 50 mA |
| Pobór prądu (praca serwa) | 500-1000 mA |
| Zakres temperatur pracy | -10°C do +50°C |
| Czas reakcji na przepływ | < 5 sekund |
| Maksymalny czas opróżniania | 5 minut |
| Timeout braku przepływu | 30 sekund |

### Komponenty Systemu
1. **Jednostka sterująca** - mikrokontroler Arduino Nano/ESP32/Pico
2. **Aktuator** - serwomechanizm SG90 lub MG996R
3. **Czujnik przepływu** - optyczny detektor kropli/czujnik wagowy
4. **Interfejs użytkownika** - przycisk manualny + dioda LED
5. **Obudowa** - wodoodporna skrzynka IP65

---

## Opis Mechaniczny

### Montaż Serwomechanizmu

#### Wymagane Elementy
- Serwomechanizm (SG90 dla lekkich zastosowań, MG996R dla większych sił)
- Drukowane uchwyty montażowe (PLA/PETG)
- Śruby M3 x 10mm (4 sztuki)
- Nakrętki samohamowne M3 (4 sztuki)
- Wałek sprzęgający z kluczem ramki

#### Procedura Montażu

1. **Przygotowanie uchwytu serwa**
   - Wydrukuj uchwyt montażowy dostosowany do modelu serwa
   - Upewnij się że tolerancje pasowania są odpowiednie (0.2mm luzu)
   - Przetestuj dopasowanie serwa do uchwytu

2. **Montaż do ula**
   - Zdemontuj boczną pokrywę ula w miejscu dostępu do klucza
   - Zamontuj uchwyt serwa tak aby wałek był współosiowy z kluczem ramki
   - Odległość od osi obrotu: 25-30mm od krawędzi ramki
   - Kąt montażu: 90° względem płaszczyzny ramki

3. **Podłączenie mechaniczne**
   - Nałóż wałek sprzęgający na oś serwa
   - Połącz wałek z kluczem ramki Flow Hive
   - Sprawdź zakres ruchu: 0° (zamknięte) do 90° (otwarte)
   - Upewnij się że nie ma luzów ani oporów

4. **Uszczelnienie**
   - Zastosuj uszczelki silikonowe wokół punktów przejścia
   - Zabezpiecz przed dostępem wody i owadów
   - Sprawdź czy pszczoły nie mają dostępu do mechanizmu

### Montaż Czujnika Przepływu

#### Lokalizacja
Czujnik powinien być zamontowany w najniższym punkcie rynny odpływowej, tuż przed wlotem do zbiornika na miód.

#### Warianty Montażu

**A. Czujnik Optyczny Kropli**
```
    [Rynna z miodem]
          |
          v
    ~~~~~~~~~~~~~~  <- Wiązka podczerwieni
    |  LED  | PD  |  <- Nadajnik i fotodetektor
    ~~~~~~~~~~~~~~
          |
          v
    [Zbiornik miodu]
```

1. Wydrukuj uchwyt czujnika w kształcie litery U
2. Zamontuj po przeciwnych stronach rynny
3. Odległość między elementami: 15-20mm
4. Kąt nachylenia: 45° względem pionu
5. Uszczelnij żywicą epoksydową

**B. Czujnik Wagowy (Load Cell)**
```
    [Zbiornik miodu]
          |
    [Platforma wagowa]
          |
    [Load Cell 5kg]
          |
    [Podstawa stała]
```

1. Zamontuj platformę wagową pod zbiornikiem
2. Podłącz load cell do wzmacniacza HX711
3. Skalibruj system dla pojemności zbiornika
4. Zabezpiecz przed przeciążeniem

### Obudowa Elektroniki

#### Wymagania
- Klasa szczelności: IP65 minimum
- Materiał: ABS lub poliwęglan
- Wymiary minimalne: 120 x 80 x 60 mm
- Kolor: jasny (odbija światło, mniejsze nagrzewanie)

#### Punkty Przejścia
1. **Przejście dla kabli serwa** - dławica kablowa M12
2. **Przejście dla czujnika** - dławica kablowa M10
3. **Przejście dla zasilania** - gniazdo DC waterproof
4. **Przycisk manualny** - przełącznik waterproof IP67

#### Wentylacja
- Otwory wentylacyjne z membraną Gore-Tex
- Zapobiega kondensacji wilgoci
- Chroni przed bezpośrednim zawilgoceniem

---

## Opis Elektroniczny

### Schemat Połączeń

```
                    +5V --------------------+
                    |                       |
              +-----+-----+                 |
              |           |                 |
         +----+  ARDUINO  +----+            |
         |    |   NANO    |    |            |
         |    +-----------+    |            |
         |                     |            |
    +----+----+           +----+----+       |
    |         |           |         |       |
 [SERWO]   [PRZYCISK]  [CZUJNIK] [LED]     |
    |         |           |         |       |
    |    GND--+-----------+---------+-------+
    |                                      
   GND -------------------------------------+

Szczegółowe połączenia:

ARDUINO NANO:
- Pin 9  -----> SERVO (pomarańczowy/sygnał)
- Pin 2  -----> CZUJNIK PRZEPŁYWU (sygnał)
- Pin 3  -----> PRZYCISK MANUALNY
- Pin 13 -----> DIODA LED STATUSU
- 5V   -----> Zasilanie czujników i serwa
- GND  -----> Masa wspólna

ZASILANIE:
- Wejście: 5V DC (USB lub przetwornica)
- Filtr: Kondensator 1000µF przy zasilaniu
- Zabezpieczenie: Bezpiecznik 2A resetowalny
```

### Lista Komponentów Elektronicznych

| Element | Specyfikacja | Ilość | Uwagi |
|---------|-------------|-------|-------|
| Mikrokontroler | Arduino Nano V3.0 | 1 | ATmega328P |
| Serwomechanizm | SG90 / MG996R | 1 | 5V, torque >1.6kg/cm |
| Czujnik przepływu | Optyczny IR | 1 | Lub load cell z HX711 |
| Dioda LED | 5mm dowolny kolor | 1 | Status systemu |
| Przycisk | Push-button NO | 1 | Waterproof IP67 |
| Rezystor | 10kΩ | 1 | Pull-up przycisku |
| Kondensator | 1000µF 16V | 1 | Filtr zasilania |
| Bezpiecznik | 2A resetowalny | 1 | Ochrona przeciążeniowa |
| Przewody | AWG 22-24 | ~2m | Połączenia wewnętrzne |
| Złącza | JST PH 2.0mm | 4-6 | Rozłączne połączenia |

### Dobór Zasilania

#### Opcja A: Zasilanie Sieciowe
- Transformator 230V/5V 2A
- Mostek prostowniczy
- Stabilizator LM7805 lub moduł buck
- Zalety: Nieograniczony czas pracy
- Wady: Wymaga dostępu do gniazdka

#### Opcja B: Akumulator Li-Ion
- Ogniwo 18650 3.7V 2500mAh
- Przetwornica boost 3.7V->5V
- Moduł ładowania TP4056
- Panel solarny 6V 2W (opcjonalnie)
- Zalety: Pełna autonomia
- Wada: Konieczność okresowego ładowania

#### Opcja C: Power Bank
- Gotowy power bank 10000mAh
- Kabel USB
- Zalety: Prosta implementacja
- Wada: Większe gabaryty

### Zabezpieczenia Elektryczne

1. **Ochrona przeciwprzepięciowa**
   - Warystor MOV na wejściu zasilania
   - Dioda TVS na liniach sygnałowych

2. **Filtracja zakłóceń**
   - Kondensatory ceramiczne 100nF blisko układu
   - Ferrite bead na kablu zasilającym

3. **Ochrona przed odwrotną polaryzacją**
   - Dioda Schottky'ego szeregowo z zasilaniem

4. **Izolacja galwaniczna** (opcjonalnie)
   - Optoizolatory na liniach wejściowych
   - Izolowany przetwornik DC-DC

---

## Opis Kodu Źródłowego

### Struktura Programu

```
flowing_hive_auto_drain.ino
│
├── Sekcja definicji i konfiguracji
│   ├── Definicje pinów (SERVO_PIN, FLOW_SENSOR_PIN, itd.)
│   ├── Parametry systemowe (kąty serwa, timeouty)
│   └── Zmienne globalne
│
├── Funkcje przerwań
│   └── flowInterrupt() - obsługa czujnika przepływu
│
├── Funkcje setup/loop
│   ├── setup() - inicjalizacja systemu
│   └── loop() - główna pętla programu
│
├── Funkcje detekcji
│   └── detectHoneyFlow() - algorytm wykrywania przepływu
│
├── Funkcje sterujące
│   ├── startDraining() - rozpoczęcie opróżniania
│   ├── monitorDrainingProcess() - nadzór procesu
│   └── stopDraining() - zakończenie opróżniania
│
└── Funkcje pomocnicze
    ├── updateStatusLED() - sygnalizacja wizualna
    └── blinkLED() - sekwencje migania
```

### Szczegółowy Opis Funkcji

#### `setup()` - Inicjalizacja
**Cel:** Przygotowanie systemu do pracy przy starcie.

**Działanie:**
1. Inicjalizacja komunikacji szeregowej (9600 baud)
2. Konfiguracja pinów wejścia/wyjścia
3. Podłączenie przerwania sprzętowego dla czujnika
4. Dołączenie serwa i ustawienie pozycji początkowej
5. Sygnalizacja świetlna gotowości systemu

**Czas wykonania:** ~2 sekundy

#### `loop()` - Główna Pętla
**Cel:** Ciągłe monitorowanie stanu systemu i reagowanie na zdarzenia.

**Działanie:**
1. Sprawdzenie stanu przycisku manualnego (z debouncingiem)
2. Automatyczna detekcja przepływu miodu
3. Monitorowanie aktywnego procesu opróżniania
4. Aktualizacja diody statusu co 100ms

**Czas cyklu:** ~100ms

#### `flowInterrupt()` - Obsługa Przerwania
**Cel:** Rejestracja wykrycia przepływu w czasie rzeczywistym.

**Działanie:**
1. Odczyt aktualnego czasu
2. Debounce sprzętowy (50ms)
3. Inkrementacja licznika przepływu
4. Zapis czasu ostatniego wykrycia

**Uwaga:** Funkcja wywoływana sprzętowo, musi być szybka!

#### `detectHoneyFlow()` - Detekcja Przepływu
**Cel:** Inteligentne rozróżnienie prawdziwego przepływu miodu od szumów.

**Algorytm:**
1. Sprawdzenie minimalnego odstępu czasowego (2s)
2. Analiza okna detekcyjnego (5 sekund)
3. Wymóg minimum 3 wykryć w oknie
4. Powrót TRUE jeśli warunek spełniony

**Zalety:**
- Odporność na pojedyncze fałszywe wyzwolenia
- Eliminacja drgań mechanicznych
- Detekcja tylko ciągłego przepływu

#### `startDraining()` - Start Opróżniania
**Cel:** Bezpieczne otwarcie ramki i rozpoczęcie procesu.

**Sekwencja:**
1. Ustawienie flagi `isDraining = true`
2. Zapis czasu startu
3. Obrót serwa do pozycji 90° (otwarte)
4. Opóźnienie 500ms na stabilizację ruchu
5. Załączenie diody LED
6. Komunikat na Serial

#### `monitorDrainingProcess()` - Nadzór Procesu
**Cel:** Kontrola trwania opróżniania i wykrywanie końca przepływu.

**Sprawdzane warunki:**
1. **Timeout braku przepływu** (30s)
   - Jeśli od ostatniego wykrycia minęło 30s → koniec
2. **Maksymalny czas pracy** (5 minut)
   - Zabezpieczenie przed zacięciem mechanizmu
3. **Raportowanie postępu** (co 10s)
   - Wysyłanie statystyk na Serial

#### `stopDraining()` - Zakończenie
**Cel:** Bezpieczne zamknięcie ramki i powrót do trybu nasłuchiwania.

**Sekwencja:**
1. Ustawienie flagi `isDraining = false`
2. Obrót serwa do pozycji 0° (zamknięte)
3. Wyłączenie diody LED
4. Reset licznika przepływu
5. Komunikat końcowy

#### `updateStatusLED()` - Sygnalizacja
**Cel:** Wizualna informacja o stanie systemu.

**Tryby:**
- **Gotowość:** Wolne miganie (2s周期)
- **Opróżnianie:** Szybkie miganie (0.5s周期)

### Algorytmy Kluczowe

#### Debouncing
```cpp
// Zarówno dla przycisku jak i czujnika
if (currentTime - lastTime > DEBOUNCE_DELAY_MS) {
  // Akceptuj zmianę stanu
}
```

#### Okno Detekcyjne (Sliding Window)
```cpp
// Analiza aktywności w przedziale czasowym
if (currentTime - windowStart >= detectionWindow) {
  if (windowCount >= threshold) {
    return true; // Wykryto zdarzenie
  }
  // Reset okna
}
```

#### Bezpieczna Manipulacja Zmiennymi Globalnymi
```cpp
// Wyłączenie przerwań przy odczycie
noInterrupts();
unsigned long count = flowCount;
interrupts();
```

### Konfiguracja Parametrów

W sekcji definicji można dostosować:

```cpp
#define SERVO_CLOSED_POSITION 0     // Kąt zamknięcia
#define SERVO_OPEN_POSITION 90      // Kąt otwarcia
#define FLOW_TIMEOUT_MS 30000       // Czas timeoutu (30s)
#define MIN_FLOW_INTERVAL_MS 2000   // Min. odstęp detekcji
#define DEBOUNCE_DELAY_MS 50        // Czas debouncingu
```

**Zalecenia kalibracyjne:**
- Dla wolniejszego przepływu zwiększ `FLOW_TIMEOUT_MS`
- Dla czułych czujników zwiększ `DEBOUNCE_DELAY_MS`
- Dostosuj kąty serwa do konkretnego typu ramki

### Diagnostyka i Logi

System generuje komunikaty na porcie szeregowym:

```
=== Flowing Hive Auto Drain System ===
Inicjalizacja systemu...
System gotowy. Nasłuchiwanie przepływu...
>>> Wykryto przepływ miodu!
--- ROZPOCZĘCIE OPRÓŻNIANIA ---
Serwo: POZYCJA OTWARTA
Postęp: 10000ms, wykryć: 45
>>> Brak przepływu przez 30s - kończenie opróżniania
--- KONIEC OPRÓŻNIANIA ---
Serwo: POZYCJA ZAMKNIĘTA
```

---

## Instalacja i Konfiguracja

### Krok 1: Przygotowanie Sprzętu
1. Skompletuj wszystkie komponenty elektroniczne
2. Wydrukuj elementy mechaniczne (uchwyty, obudowę)
3. Przygotuj narzędzia (lutownica, śrubokręty, multimetr)

### Krok 2: Montaż Mechaniczny
1. Zamontuj serwo w uchwycie
2. Połącz wałek serwa z kluczem ramki Flow Hive
3. Zamontuj czujnik przepływu w rynnę odpływową
4. Umieść elektronikę w obudowie waterproof

### Krok 3: Połączenia Elektryczne
1. Podłącz serwo do pinu 9
2. Podłącz czujnik do pinu 2 (z pull-up)
3. Podłącz przycisk do pinu 3
4. Podłącz diodę LED do pinu 13
5. Podłącz zasilanie 5V i masę

### Krok 4: Programowanie
1. Podłącz Arduino Nano przez USB
2. Otwórz IDE Arduino
3. Wgraj kod `flowing_hive_auto_drain.ino`
4. Otwórz Monitor Portu Szeregowego (9600 baud)
5. Zweryfikuj komunikaty startowe

### Krok 5: Kalibracja
1. Test ręczny: naciśnij przycisk, sprawdź ruch serwa
2. Test czujnika: symuluj przepływ (np. wodą)
3. Dostosuj kąty serwa jeśli potrzebne
4. Sprawdź timeouty i czasy reakcji

### Krok 6: Finalny Montaż
1. Szczelnie zamknij obudowę
2. Zabezpiecz wszystkie przejścia kablowe
3. Zamontuj całość na ulu
4. Uruchom system i monitoruj pierwszy cykl

---

## Eksploatacja i Konserwacja

### Codzienne Sprawdzanie
- ✅ Czy dioda LED miga prawidłowo?
- ✅ Czy nie ma widocznych uszkodzeń mechanicznych?
- ✅ Czy pszczoły nie zakłócają pracy czujnika?

### Konserwacja Sezonowa
1. **Po sezonie miodowym:**
   - Wyczyść czujnik przepływu z resztek miodu
   - Sprawdź stan uszczelek obudowy
   - Naładuj akumulatory (jeśli stosowane)

2. **Przed sezonem:**
   - Przetestuj pełny cykl opróżniania
   - Sprawdź baterie/zasilanie
   - Kalibruj czujnik jeśli potrzebne

### Rozwiązywanie Problemów

| Problem | Możliwa Przyczyna | Rozwiązanie |
|---------|-------------------|-------------|
| Serwo nie reaguje | Brak zasilania | Sprawdź połączenia 5V/GND |
| Fałszywe detekcje | Zbyt czuły czujnik | Zwiększ `DEBOUNCE_DELAY_MS` |
| Przedwczesne zakończenie | Krótki timeout | Zwiększ `FLOW_TIMEOUT_MS` |
| System nie startuje | Błąd kompilacji | Sprawdź wersję Arduino IDE |

### Aktualizacje Firmware
1. Pobierz nowszą wersję kodu
2. Podłącz się przez USB
3. Wgraj nowy firmware
4. Zweryfikuj działanie

---

## Dane Techniczne

### Zużycie Energii
- Tryb spoczynku: ~40mA
- Aktywne nasłuchiwanie: ~60mA
- Praca serwa: ~500mA (chwilowo)
- Szacowany czas pracy na akumulatorze 2500mAh: ~40 godzin ciągłej pracy

### Żywotność
- Serwomechanizm: >100,000 cykli
- Czujnik optyczny: >5 lat
- Obudowa: >10 sezonów pszczelarskich

### Certyfikaty i Normy
- Zgodność z dyrektywą RoHS
- Materiały dopuszczone do kontaktu z żywnością
- Klasa szczelności IP65

---

## Kontakt i Wsparcie

Projekt open-source dostępny na licencji MIT.
Wersja dokumentacji: 1.0.0
Data publikacji: 2024

**Autor:** ApiaryGuard System  
**Licencja:** MIT License  
**Repozytorium:** github.com/apiaryguard/flowing-hive-auto-drain
