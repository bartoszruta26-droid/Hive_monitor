# Firmware: bezpieczeństwo efektorów

## Zakres zmian

Ta poprawka wzmacnia programową blokadę wyjść w module `src/pico_refactored`:

- wyjścia PWM grzałki i wentylatora otrzymują deterministyczny stan początkowy 0;
- wejście w `safe mode` natychmiast ustawia grzałkę na 0, wentylator na skonfigurowany poziom bezpieczny i wyłącza wyjścia przekaźnikowe;
- `updateOutputs()` ponawia wymuszenie stanu bezpiecznego w każdej iteracji pętli, więc zwykła aktualizacja wyjść nie przywróci sterowania przekaźnikami;
- bezpośrednie funkcje sterujące wentylatorem, pompą, zaworami i kanałami przekaźników odrzucają polecenia podczas aktywnego trybu bezpiecznego;
- główny tryb bezpieczny aktywuje również zatrzask Flowing Hive i anuluje sekwencję opróżniania;
- wyłączenie głównego trybu bezpiecznego nie kasuje automatycznie niezależnego zatrzasku Flowing Hive.

## Ważne ograniczenie sprzętowe

W konfiguracji przekazanej przez operatora moduł przekaźnikowy jest aktywny stanem HIGH, a nieaktywny stanem LOW. Firmware ustawia więc LOW jako stan wyłączenia. Potwierdzenie dotyczy wskazanego modułu; przed wdrożeniem należy jeszcze sprawdzić każdy kanał i stan wejść podczas resetu MCU.

Wprowadzono automatyczną blokadę grzałki: gdy operator żąda grzania, firmware sprawdza obecność i aktywność DHT oraz świeżość pomiaru temperatury. Trzy kolejne nieprawidłowe próbki temperatury albo brak poprawnego pomiaru przez ponad 6 sekund powodują aktywację trybu bezpiecznego. Brak lub nieaktywność czujnika również blokuje grzanie. Blokada jest zatrzaskiwana; odzyskanie odczytów nie wznawia automatycznie grzania. Progi 3 próbki / 6 sekund są wstępnymi progami programowymi i wymagają weryfikacji na docelowym sprzęcie. Nie jest to zabezpieczenie przed rzeczywistym przegrzaniem elementu grzejnego: DHT mierzy warunki otoczenia, a nie temperaturę powierzchni grzałki. Tryb programowy nie zastępuje zabezpieczeń sprzętowych. Grzałka wymaga niezależnego ogranicznika temperatury / bezpiecznika termicznego dobranego do instalacji, a obwody mocy powinny przechodzić do stanu bezpiecznego także przy resecie MCU, utracie zasilania, zawieszeniu programu lub uszkodzeniu tranzystora. Nie należy traktować tego firmware jako układu bezpieczeństwa certyfikowanego.

Poziom wentylatora w trybie bezpiecznym pozostaje wartością konfiguracyjną `SAFE_MODE_FAN_DUTY`. Wartość 128 nie jest uniwersalnie bezpieczna: jej właściwość zależy od wentylatora, sterownika, polaryzacji PWM i warunków ula. Przed uruchomieniem trzeba ją zweryfikować.


## Dodatkowe zabezpieczenia wejścia i Flowing Hive

- Parser poleceń USB akceptuje wyłącznie pełne nazwy komend i argumenty dziesiętne z zakresów opisanych w `HELP`; błędne prefiksy, dodatkowe znaki i wartości poza zakresem są odrzucane.
- Bufor komend USB ma limit 96 znaków. Przekroczenie limitu powoduje odrzucenie całej linii do najbliższego znaku końca wiersza.
- Podczas aktywnego głównego trybu bezpiecznego polecenia zmieniające nastawy efektorów są odrzucane.
- Sekwencja Flowing Hive jest ograniczona do 1–1800 sekund. Żądanie jest odrzucane, gdy sterownik serwa nie został poprawnie przyłączony, zatrzask Flowing Hive jest aktywny lub czas jest poza zakresem.
- Serwo korzysta z wbudowanej w rdzeń Arduino-Pico biblioteki `Servo`, generującej impulsy w tle przez PIO. Poprawne przyłączenie sterownika nie potwierdza obecności fizycznego serwa ani bezpieczeństwa mechaniki.
- Licznik impulsów czujnika przepływu jest odczytywany i zerowany w sekcji krytycznej, aby nie gubić impulsów z przerwania podczas próbkowania.

Zatrzask Flowing Hive pozostaje niezależny po skasowaniu głównego trybu bezpiecznego. Nie ma automatycznego wznowienia ruchu serwa po odzyskaniu komunikacji lub pomiarów.

## Testy wymagane przed wdrożeniem

Testy wykonywać najpierw bez podłączonych obciążeń (miernik / oscyloskop / diody testowe), a następnie z obciążeniami zastępczymi. Nie testować pierwszej wersji na rodzinie pszczelej.

1. Potwierdzić mapę pinów, typ modułu przekaźnikowego, polaryzację wejść i stan po resecie.
2. Włączyć zasilanie i sprawdzić, że PWM grzałki oraz wentylatora ma stan 0, a wszystkie wyjścia przekaźnikowe są nieaktywne (moduł aktywny HIGH, stan LOW = OFF).
3. Aktywować `activateSafeMode()`; sprawdzić wszystkie 11 wyjść przekaźnikowych, grzałkę i wentylator.
4. Podczas aktywnego trybu bezpiecznego wywołać kolejno funkcje `setFanPWM`, `setPump`, `setValve1`, `setValve2`, `setRelay`; żadne polecenie nie może załączyć wyjścia.
5. Wielokrotnie wywołać `updateOutputs()` w trybie bezpiecznym; stan wyjść ma pozostać bezpieczny.
6. Sprawdzić reset watchdogiem, odłączenie Ethernetu, utratę komunikacji i ponowne uruchomienie zasilania.
7. Wymusić odłączenie DHT, błędny odczyt temperatury i brak aktualizacji pomiaru; po żądaniu grzania blokada ma zadziałać, a ponowny prawidłowy odczyt nie może samodzielnie wznowić grzania.
8. Dopiero po pozytywnych testach elektrycznych wykonać test obciążeniowy z ograniczeniem prądowym i niezależnym odcięciem grzałki.

## Pozostałe ryzyka do zamknięcia

- brak sprzężenia zwrotnego potwierdzającego rzeczywisty stan przekaźników i elementów wykonawczych;
- brak niezależnego czujnika temperatury elementu grzejnego i sprzętowego odcięcia grzałki w torze bezpieczeństwa;
- komendy USB mają walidację składni i zakresów, ale nadal nie mają uwierzytelniania operatora ani kompletnego modelu uprawnień;
- sekwencja serwa Flowing Hive ma limit czasu, ale nadal wymaga osobnej walidacji mechanicznej, limitów kąta/ruchu i procedury awaryjnego zatrzymania;
- brak potwierdzonej kompilacji w środowisku docelowym, testów HIL (hardware-in-the-loop) i testów na docelowej płytce.

## Kryterium akceptacji

Nie scalać zmian do gałęzi produkcyjnej i nie podłączać grzałki, pompy, zaworów ani serwa, dopóki testy elektryczne nie potwierdzą stanów aktywnych/nieaktywnych i zachowania przy resecie oraz utracie zasilania.
