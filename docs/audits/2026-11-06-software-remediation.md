# Hive Monitor — naprawy programowe bez testów sprzętowych

**Data:** 2026-11-06  
**Gałąź:** `main`  
**Rewizja stanu:** `8f10116c04fb582670ba48a5923ee291cada193e`  
**Zakres:** kontynuacja ustaleń HVM-009–013 oraz uzupełnienie HVM-006.  
**Ograniczenie:** bez fizycznego Raspberry Pi, Pico, czujników, magistral i elementów wykonawczych.

## Wdrożone poprawki

### HVM-006 — ograniczenie ekspozycji HTTP
- Usunięto z odpowiedzi kolektora nagłówek `Access-Control-Allow-Origin: *`.
- Kolektor nie udostępnia już API przeglądarkom z dowolnego źródła przez permissive CORS.
- Przeglądarkowy WebUI powinien korzystać z pośrednictwa PHP; klienci natywni nie podlegają ograniczeniom CORS przeglądarki.

### HVM-009 — detekcja DHT
- Usunięto szybkie ponawianie odczytu co 100 ms.
- Detekcja wykonuje pojedynczy odczyt po odczekaniu 2 sekund od inicjalizacji, zgodnie z minimalnym odstępem typowym dla DHT22/AM2302.
- Niepowodzenie jest jawnie raportowane i zwiększa liczniki błędów.
- Nie potwierdzono zachowania na fizycznym czujniku ani czasu startu konkretnej instalacji.

### HVM-010 — diagnostyka przepływu
- Zmieniono nazwę flagi z `flowSensorInterruptReady` na `flowSensorInterruptConfigured`.
- Komunikat statusu opisuje konfigurację przerwania, a nie wykrycie czujnika.
- Diagnostyka jawnie informuje, że fizyczna obecność czujnika pozostaje niezweryfikowana.

### HVM-011 — statystyki SQLite
- Rozdzielono liczbę rekordów przyjętych do bufora, oczekujących w buforze oraz zatwierdzonych w SQLite.
- `raw_records` i `total_records` zwiększają się dopiero po udanym COMMIT.
- Dodano test, który sprawdza, że rekord w buforze nie jest liczony jako zatwierdzony, a po opróżnieniu bufora liczniki są zgodne ze stanem trwałym.
- Wartość `total_records` oznacza teraz liczbę zatwierdzonych rekordów surowych; agregaty mają oddzielny licznik.

### HVM-012 — eksport CSV
- Dodano `csvEscape()` zgodne z regułami cytowania RFC 4180.
- Pole `hive_id` jest cytowane, gdy zawiera przecinek, cudzysłów, CR lub LF; cudzysłowy wewnętrzne są podwajane.
- Dodano testy dla przecinków, cudzysłowów i znaków końca linii.

### HVM-013 — timeout kontroli kolektora
- Health check PHP korzysta ze wspólnej funkcji cURL.
- Limit połączenia wynosi maksymalnie 2 sekundy, a limit całego żądania dla health check 3 sekundy.
- Dodano test integracyjny z lokalnym serwerem kolektora, który celowo opóźnia odpowiedź; WebUI ma szybko zgłosić stan rozłączony zamiast blokować żądanie.

## Testy bez sprzętu

- Testy regresyjne PHP: autoryzacja, walidacja konfiguracji i timeout kolektora.
- Testy kontraktów bezpieczeństwa w Pythonie: UDP, CORS, wiązanie adresów, rate limiter, diagnostyka firmware, statystyki SQLite i CSV.
- Kompilacja kolektora C++ oraz testy loggera, HTTP, sieci i SQLite.
- Kompilacja firmware RP2040 w Arduino CLI.

## Status CI

- UI static checks, run `42277139682`: **success** — wszystkie kroki, w tym nowe testy regresyjne, zakończone powodzeniem.
- RP2040 firmware build, run `42277186059`: oczekuje na zakończenie w chwili sporządzenia tego wpisu.

## Zakres nadal niepotwierdzony

Nie wykonano testów elektrycznych, pomiarów impulsów czujnika przepływu, walidacji DHT na fizycznym czujniku, testów brownout/reset, testów mechaniki ani testów HIL. Kompilacja firmware nie jest dowodem poprawnego działania sprzętu.

## Uwaga do numeracji audytu

Plik `docs/audits/2026-11-06-code-audit.md` zawiera szczegółowe ustalenia HVM-001–013. Nie zawiera opisu HVM-014; nie przypisano więc temu numerowi nowego problemu bez materiału źródłowego.
