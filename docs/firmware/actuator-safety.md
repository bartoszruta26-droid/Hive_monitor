# Firmware: bezpieczeństwo efektorów

## Zakres zmian

Ta poprawka wzmacnia programową blokadę wyjść w module `src/pico_refactored`:

- wyjścia PWM grzałki i wentylatora otrzymują deterministyczny stan początkowy 0;
- wejście w `safe mode` natychmiast ustawia grzałkę na 0, wentylator na skonfigurowany poziom bezpieczny i wyłącza wyjścia przekaźnikowe;
- `updateOutputs()` ponawia wymuszenie stanu bezpiecznego w każdej iteracji pętli, więc zwykła aktualizacja wyjść nie przywróci sterowania przekaźnikami;
- bezpośrednie funkcje sterujące wentylatorem, pompą, zaworami i kanałami przekaźników odrzucają polecenia podczas aktywnego trybu bezpiecznego.

## Ważne ograniczenie sprzętowe

W aktualnej konfiguracji firmware przyjmuje, że stan LOW oznacza wyłączenie przekaźnika. Kod bazowy już używał tego założenia podczas inicjalizacji GPIO. **Nie wolno wdrażać tej poprawki na rzeczywistym ulu, dopóki nie zostanie potwierdzona polaryzacja konkretnej płytki przekaźnikowej oraz sposób sterowania wejściami modułów mocy.** Moduły aktywne stanem LOW mogą mieć odwrotną logikę.

Tryb programowy nie zastępuje zabezpieczeń sprzętowych. Grzałka wymaga niezależnego ogranicznika temperatury / bezpiecznika termicznego dobranego do instalacji, a obwody mocy powinny przechodzić do stanu bezpiecznego także przy resecie MCU, utracie zasilania, zawieszeniu programu lub uszkodzeniu tranzystora. Nie należy traktować tego firmware jako układu bezpieczeństwa certyfikowanego.

Poziom wentylatora w trybie bezpiecznym pozostaje wartością konfiguracyjną `SAFE_MODE_FAN_DUTY`. Wartość 128 nie jest uniwersalnie bezpieczna: jej właściwość zależy od wentylatora, sterownika, polaryzacji PWM i warunków ula. Przed uruchomieniem trzeba ją zweryfikować.

## Testy wymagane przed wdrożeniem

Testy wykonywać najpierw bez podłączonych obciążeń (miernik / oscyloskop / diody testowe), a następnie z obciążeniami zastępczymi. Nie testować pierwszej wersji na rodzinie pszczelej.

1. Potwierdzić mapę pinów, typ modułu przekaźnikowego, polaryzację wejść i stan po resecie.
2. Włączyć zasilanie i sprawdzić, że PWM grzałki oraz wentylatora ma stan 0, a wszystkie wyjścia przekaźnikowe są nieaktywne.
3. Aktywować `activateSafeMode()`; sprawdzić wszystkie 11 wyjść przekaźnikowych, grzałkę i wentylator.
4. Podczas aktywnego trybu bezpiecznego wywołać kolejno funkcje `setFanPWM`, `setPump`, `setValve1`, `setValve2`, `setRelay`; żadne polecenie nie może załączyć wyjścia.
5. Wielokrotnie wywołać `updateOutputs()` w trybie bezpiecznym; stan wyjść ma pozostać bezpieczny.
6. Sprawdzić reset watchdogiem, odłączenie Ethernetu, utratę komunikacji i ponowne uruchomienie zasilania.
7. Dopiero po pozytywnych testach elektrycznych wykonać test obciążeniowy z ograniczeniem prądowym i niezależnym odcięciem grzałki.

## Pozostałe ryzyka do zamknięcia

- brak sprzężenia zwrotnego potwierdzającego rzeczywisty stan przekaźników i elementów wykonawczych;
- brak niezależnego pomiaru temperatury / sprzętowego odcięcia grzałki w torze bezpieczeństwa potwierdzonego w kodzie;
- komendy USB nie mają jeszcze kompletnej walidacji składni, limitów czasu i uprawnień operatora;
- sekwencja serwa Flowing Hive wymaga osobnej walidacji mechanicznej, limitów ruchu i procedury awaryjnego zatrzymania;
- brak testów HIL (hardware-in-the-loop) i testów na docelowej płytce.

## Kryterium akceptacji

Nie scalać zmian do gałęzi produkcyjnej i nie podłączać grzałki, pompy, zaworów ani serwa, dopóki testy elektryczne nie potwierdzą stanów aktywnych/nieaktywnych i zachowania przy resecie oraz utracie zasilania.
