# Hive Monitor — uzupełniający audyt i naprawy programowe

**Data:** 2026-11-06  
**Gałąź robocza:** `fix/hvm-014-software-hardening`  
**Baza:** `178c4883d81672d3d00bd423fb16b10f98c958d4`  
**Zakres:** dodatkowy przegląd parsera kolektora, inicjalizacji SQLite i domyślnych ustawień wdrożeniowych.  
**Ograniczenie:** bez testów na fizycznym sprzęcie.

## Dodatkowe ustalenia

### FU-001 — parser JSON wyszukiwał klucze tekstowo
Parser oparty na `std::string::find` mógł dopasować nazwę klucza wewnątrz innego pola lub obiektu zagnieżdżonego, nie obsługiwał poprawnie sekwencji ucieczki w łańcuchach i nie weryfikował całej składni dokumentu. Niepoprawne lub niejednoznaczne dane mogły prowadzić do błędnego mapowania pomiarów.

**Naprawa:** dodano niezależny parser strukturalny `json_object_parser.h`. Weryfikuje on pełny dokument JSON, składnię liczb, łańcuchy i sekwencje ucieczki (w tym pary surogatów Unicode), ogranicza głębokość i rozmiar wejścia, odrzuca powtórzone klucze i pobiera wyłącznie pola najwyższego poziomu. Kolektor odrzuca niepoprawny JSON przed mapowaniem wartości. Rozpoznawanie formatu rozpoczyna się od pierwszego niebiałego znaku, więc niekompletny obiekt nie jest przekazywany do parsera CSV.

**Testy:** `test_json_object_parser.cpp` obejmuje zagnieżdżone pola, cudzysłowy i ukośniki wsteczne, Unicode, tablice, wartości logiczne, duplikaty kluczy, niepoprawne liczby, błędne sekwencje ucieczki i nadmiarowy tekst po obiekcie.

### FU-002 — inicjalizacja SQLite ignorowała błędy konfiguracji i indeksów
Wyniki poleceń `PRAGMA` oraz tworzenia indeksów nie były sprawdzane. Proces mógł kontynuować inicjalizację mimo niepowodzenia części konfiguracji bazy lub utworzenia indeksu.

**Naprawa:** błędy wykonania `PRAGMA` i `CREATE INDEX` są teraz raportowane jako `DatabaseInitException`; pamięć komunikatów SQLite jest zwalniana. Zapis do niezainicjalizowanej bazy zwiększa licznik błędów i zgłasza `DatabaseWriteException`, zamiast pozornie akceptować rekord.

**Testy:** pełny zestaw testów SQLite i kolektora w workflow UI static checks.

### FU-003 — katalog logów miał uprawnienia zapisu dla wszystkich użytkowników
Cel instalacyjny Makefile nadawał `/var/log/apiaryguard` tryb `0777`.

**Naprawa:** tryb zmieniono na `0750`. W obecnym skrypcie usługi systemd kolektor jest uruchamiany jako root, więc katalog pozostaje dostępny dla procesu, a zapis dla pozostałych użytkowników jest zablokowany.

**Testy:** `test_deployment_security.py` sprawdza restrykcyjny tryb katalogu i brak dawnego `0777`.

### FU-004 — kopie zapasowe Android obejmowały lokalną historię pomiarów
Manifest Android miał `allowBackup=true`.

**Naprawa:** ustawiono `android:allowBackup="false"`, aby systemowa kopia zapasowa nie eksportowała lokalnej bazy Room.

**Testy:** kontrola statyczna manifestu oraz workflow Android build and tests.

## Wyniki CI

- UI static checks, run `42278883148`: **success** — testy PHP, statyczne kontrole bezpieczeństwa, kompilacja C++, testy loggera, HTTP, parsera JSON, sieci i SQLite.
- Android build and tests, run `42278475786`: **success** — testy jednostkowe Android i budowa debug APK. Ten przebieg obejmuje zmianę manifestu; późniejsze zmiany gałęzi dotyczyły wyłącznie kolektora C++/SQLite i testów TUI.
- Testy sprzętowe i HIL: **nie wykonano**.

## Ograniczenia pozostałe po tym etapie

- Parser JSON waliduje strukturę i typ składniowy wartości, ale nie ustanawia jeszcze pełnej domenowej walidacji zakresów wszystkich 338+ parametrów (np. zakresów temperatury, wilgotności i identyfikatora ula).
- Nie przeprowadzono pełnego audytu wszystkich skryptów instalacyjnych, bezpośrednich endpointów legacy, uprawnień systemd, TLS Android ani łańcucha dostaw CI.
- Nie wykonano testów współbieżności pod obciążeniem, awarii zasilania, HIL ani walidacji fizycznych czujników i efektorów.

**Wniosek:** FU-001–FU-004 zostały poprawione i przechodzą przypisane kontrole programowe. Nie oznacza to zakończenia pełnego audytu bezpieczeństwa ani gotowości sprzętu do sterowania obciążeniami.
