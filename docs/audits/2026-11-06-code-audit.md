# Hive Monitor — audyt kodu po scaleniu PR #107

**Data:** 2026-11-06  
**Gałąź:** `main`  
**Audytowana rewizja:** `01587e192b89d573ec78b57ae24d1aecdc7a5b25`  
**Zakres:** 55 plików zmienionych w PR #107; przegląd statyczny implementacji, przepływów danych, obsługi błędów, bezpieczeństwa API i efektorów.  
**Status:** audyt statyczny; nie jest to certyfikacja bezpieczeństwa ani walidacja sprzętowa.

## 1. Podsumowanie wykonawcze

PR #107 został scalony metodą squash do `main`. Kontrole uruchomione dla PR zakończyły się sukcesem: kompilacja RP2040, PHP lint, Android test/build oraz kontrole UI/TUI/kolektora i SQLite. Po scaleniu ponownie uruchomiono workflow na `main`; w chwili zapisu raportu PHP lint i syntax/UI zakończyły się sukcesem, a Android i firmware nadal działały.

**Wniosek:** zmiany znacząco poprawiają wiarygodność danych, obsługę błędów i ograniczają pozorowane funkcje. Nie należy jednak uznawać systemu za gotowy produkcyjnie. W przeglądzie kodu znaleziono poniższe problemy wymagające poprawy.

## 2. Znaleziska

### HVM-001 — API PHP zwraca szczegóły wyjątków w odpowiedzi produkcyjnej
- **Priorytet:** wysoki
- **Plik:** `src/webui/api.php`, końcowy blok `catch (Exception $e)` (około linii 729).
- **Obserwacja:** odpowiedź HTTP zawiera `details => $e->getMessage()` bez warunku `DEBUG_MODE`. Wcześniejszy globalny handler wyjątku maskuje szczegóły w produkcji, ale ten lokalny handler nadal je ujawnia.
- **Ryzyko:** ujawnienie ścieżek, nazw plików, nazw tabel, szczegółów konfiguracji lub błędów połączenia.
- **Zalecenie:** w odpowiedzi produkcyjnej zwracać wyłącznie stały kod błędu i identyfikator zdarzenia; szczegóły zapisywać w logu. Dodać test sprawdzający brak tekstu wyjątku w odpowiedzi przy `APIARY_ENVIRONMENT=production`.

### HVM-002 — niezabezpieczony odczyt konfiguracji przez endpoint config
- **Priorytet:** wysoki, zależny od zawartości plików konfiguracyjnych.
- **Plik:** `src/webui/api.php`, kontrola klucza API około linii 309–320 oraz obsługa `case 'config'` około linii 685–710.
- **Obserwacja:** klucz API jest wymagany dla POST/PUT/DELETE, lecz `GET config` wywołuje `loadConfig()` bez uwierzytelniania. Funkcja odczytuje m.in. ustawienia API, bazy danych i sieci.
- **Ryzyko:** ujawnienie sekretów lub wewnętrznych parametrów infrastruktury, jeśli zostały zapisane w plikach konfiguracyjnych.
- **Zalecenie:** wymagać uwierzytelnienia również dla odczytu konfiguracji; w odpowiedziach maskować pola sekretne. Rozdzielić konfigurację publiczną od sekretów.

### HVM-003 — zapis konfiguracji nie waliduje kluczy i wartości oraz nie zapisuje atomowo
- **Priorytet:** wysoki.
- **Plik:** `src/webui/api.php`, `saveConfig()` około linii 520–550.
- **Obserwacja:** klucze i wartości z JSON są składane bezpośrednio w linie `key=value`; nie widać ograniczenia znaków nowej linii ani schematu dozwolonych kluczy. Zapis odbywa się bezpośrednio przez `file_put_contents`, bez pliku tymczasowego, blokady i jawnego ustawienia uprawnień.
- **Ryzyko:** wstrzyknięcie dodatkowych wpisów konfiguracyjnych, uszkodzenie pliku przy przerwanym zapisie oraz niekontrolowane uprawnienia pliku zależne od umask.
- **Zalecenie:** zdefiniować schemat sekcji i kluczy, typy oraz zakresy; odrzucać CR/LF i nieznane klucze; zapisywać do pliku tymczasowego, wykonać flush i atomowe rename; ustawić restrykcyjne prawa katalogu i plików.

### HVM-004 — odbiór UDP ma martwą kontrolę obcięcia datagramu
- **Priorytet:** średni/wysoki.
- **Plik:** `src/rpi_tui/apiary_collector.cpp`, `receiveLoop()` około linii 670.
- **Obserwacja:** `recvfrom()` otrzymuje pojemność `sizeof(buffer)-1` (1023 bajty), a następnie kod sprawdza, czy `recv_len >= sizeof(buffer)` (1024). Ten warunek nie może zostać spełniony przy takim wywołaniu.
- **Ryzyko:** zbyt duży datagram może zostać obcięty i przekazany do parsera jako częściowa wiadomość zamiast zostać jawnie odrzucony.
- **Zalecenie:** użyć `recvmsg()` z `MSG_TRUNC` lub bufora o kontrolowanej maksymalnej długości i odrzucać datagramy obcięte. Dodać test datagramu o rozmiarze większym niż limit.

### HVM-005 — ogranicznik UDP/HTTP może rosnąć bez ograniczenia pamięci
- **Priorytet:** średni.
- **Plik:** `src/rpi_tui/apiary_collector.cpp`, struktura `RateLimiter` około linii 140–180.
- **Obserwacja:** mapa `connection_times` przechowuje wpis dla każdego adresu IP. Istnieje metoda `cleanup()`, ale w przeglądanym przepływie nie znalazłem jej wywołania. Klient może generować wpisy z wielu adresów źródłowych.
- **Ryzyko:** nieograniczony wzrost pamięci procesu; w UDP łatwiej też podszyć się pod źródłowy adres IP.
- **Zalecenie:** wywoływać okresowe czyszczenie, ograniczyć maksymalną liczbę kluczy i zastosować strukturę o ograniczonej pojemności/TTL. Nie traktować rate limitingu po IP jako uwierzytelnienia.

### HVM-006 — datagramy UDP i HTTP są przyjmowane z każdego interfejsu
- **Priorytet:** wysoki, zależny od topologii sieci.
- **Plik:** `src/rpi_tui/apiary_collector.cpp`, `initNetwork()` około linii 647 i serwer HTTP około linii 1490.
- **Obserwacja:** oba serwery wiążą gniazda do `INADDR_ANY`. Odpowiedź HTTP dodatkowo emituje `Access-Control-Allow-Origin: *`.
- **Ryzyko:** usługa monitoringu jest dostępna ze wszystkich interfejsów urządzenia, w tym potencjalnie z sieci niezaufanej. Endpointy są tylko do odczytu, ale udostępniają dane pasieki i stan urządzeń.
- **Zalecenie:** konfigurowalny adres nasłuchu z bezpiecznym domyślnym ustawieniem; jawna polityka zapory; CORS ograniczony do origin WebUI. Wystawiać zdalnie wyłącznie przez kontrolowany segment sieci lub reverse proxy TLS.

### HVM-007 — rate limiter używa `inet_ntoa()` w wielu wątkach
- **Priorytet:** średni.
- **Plik:** `src/rpi_tui/apiary_collector.cpp`, odbiór UDP i obsługa HTTP.
- **Obserwacja:** `inet_ntoa()` zwraca wskaźnik do współdzielonego statycznego bufora. Wywołania występują z wątku odbioru UDP i głównego wątku HTTP.
- **Ryzyko:** wyścig danych i przypisanie nieprawidłowego IP do limitera/logu.
- **Zalecenie:** użyć `inet_ntop()` z lokalnym buforem w każdym wątku.

### HVM-008 — kontrola stanu HTTP nie obejmuje pełnej dostępności usługi
- **Priorytet:** średni.
- **Plik:** `src/rpi_tui/apiary_collector.cpp`, trasa `/health` około linii 1590.
- **Obserwacja:** odpowiedź zwraca `status: ok` i stały `collector_url`, niezależnie od tego, czy odbiór UDP jest aktywny, czy dane są aktualne. Trasy `/api/sensors`, `/api/effectors`, `/api/history` i `/api/logs` zwracają syntetyczne puste tablice z opisem, a nie rzeczywiste dane.
- **Ryzyko:** monitoring zewnętrzny może uznać proces za zdrowy mimo braku źródła danych. Puste listy mogą być mylone z rzeczywistym brakiem sensorów/efektorów.
- **Zalecenie:** rozdzielić liveness i readiness; raportować stan UDP, wiek ostatniego poprawnego pomiaru i dostępność bazy. Dla niezaimplementowanych endpointów zwracać 501 lub jednoznaczny status `UNAVAILABLE`, nie pozorowane dane.

### HVM-009 — detekcja DHT wykonuje ponowne próby co 100 ms
- **Priorytet:** średni, do potwierdzenia z biblioteką i modelem czujnika.
- **Plik:** `src/pico_refactored/src/sensors.cpp`, `detectDHT()` około linii 244–280.
- **Obserwacja:** detekcja ponawia odczyt DHT w pętli z `delay(100)`, natomiast normalny cykl odczytu jest ograniczony do 2 sekund.
- **Ryzyko:** dla DHT22/AM2302 wymagany odstęp pomiarowy jest znacznie dłuższy niż 100 ms; ponowienia mogą korzystać z buforowanego wyniku biblioteki i nie stanowią niezależnych prób detekcji.
- **Zalecenie:** użyć pojedynczej próby po inicjalizacji i odczekać pełny interwał pomiarowy przed kolejną; testować zachowanie dla odłączonego, zwartego i niestabilnego czujnika.

### HVM-010 — status czujnika przepływu oznacza gotowość przerwania jako obecność sprzętu
- **Priorytet:** niski/średni.
- **Plik:** `src/pico_refactored/src/effectors_flowing_hive.cpp`, `initFlowSensor()` około linii 500.
- **Obserwacja:** po wywołaniu `attachInterrupt()` flaga `flowSensorInterruptReady` jest ustawiana na true. Nie ma potwierdzenia fizycznego podłączenia czujnika ani walidacji impulsów.
- **Ryzyko:** interfejs diagnostyczny może prezentować „ready” mimo braku czujnika.
- **Zalecenie:** zmienić etykietę na „interrupt configured”; osobno raportować wykryty ruch/impulsy i timeout braku sygnału.

### HVM-011 — SQLite: statystyki liczone przed potwierdzeniem zapisu
- **Priorytet:** średni.
- **Plik:** `src/rpi_tui/apiary_database.cpp`, `storeRawData()` około linii 260–280.
- **Obserwacja:** `total_records_` i `raw_records_` są zwiększane po dodaniu elementu do bufora, zanim transakcja SQLite zakończy się sukcesem.
- **Ryzyko:** statystyki mogą raportować rekordy jako zapisane, mimo że bufor nie został zatwierdzony; przy błędzie flush rozjazd narasta.
- **Zalecenie:** rozdzielić liczniki accepted/buffered/committed/failed; zwiększać committed dopiero po udanym COMMIT.

### HVM-012 — eksport CSV nie escapuje wartości tekstowej hive_id
- **Priorytet:** średni.
- **Plik:** `src/rpi_tui/apiary_database.cpp`, `exportToCSV()` około linii 690–730.
- **Obserwacja:** `hive_id` jest wypisywany bez cytowania i escapowania przecinków, cudzysłowów ani znaków nowej linii.
- **Ryzyko:** niepoprawny CSV, przesunięcie kolumn lub wstrzyknięcie kolejnych wierszy przy identyfikatorze pochodzącym z danych wejściowych.
- **Zalecenie:** użyć funkcji serializacji CSV zgodnej z RFC 4180 i testów dla przecinków, cudzysłowów, CR/LF oraz UTF-8.

### HVM-013 — obsługa błędu lokalnego endpointu API PHP może ujawniać szczegóły
- **Priorytet:** wysoki.
- **Plik:** `src/webui/api.php`, końcowy handler wyjątku oraz `handleException()`.
- **Obserwacja:** handler lokalny zwraca treść wyjątku bez sprawdzenia środowiska, a handler globalny dopuszcza szczegóły tylko przy debug. Polityki są niespójne.
- **Zalecenie:** jeden wspólny serializer błędów i testy dla debug/production.

## 3. Przegląd komponentów

| Komponent | Zakres sprawdzony | Ocena |
|---|---|---|
| API PHP | CORS, metody, klucz API, demo mode, config, proxy do kolektora, błędy | Wymaga poprawek HVM-001–003 |
| Firmware efektorów | safe mode, blokady komend, stan wyjść, reset blokady | Dobra poprawa; nadal brak dowodu elektrycznego |
| Flowing Hive | Servo, sekwencja opróżniania, HX711 #2, przepływ i ISR | Wymaga doprecyzowania diagnostyki sprzętu |
| Czujniki | detekcja, inicjalizacja, odczyt, świeżość temperatury | DHT wymaga korekty interwałów detekcji |
| Sieć Pico | HTTP/UDP, trasy, status, wysyłanie UDP | Tylko odczyt; wymaga walidacji sieciowej |
| Kolektor C++ | UDP, parser, rate limiter, HTTP, wątki, zamykanie | HVM-004–008 |
| SQLite | bufor, transakcje, agregacje, retencja, historia, eksport | Dobre transakcje; HVM-011–012 |
| Android | adres bazowy, mapowanie JSON, cache, polling, stan UI | Testy jednostkowe są; potrzebne testy TLS i urządzenia |
| WebUI | walidacja odpowiedzi, escapowanie, widoki pustych danych, odświeżanie | Widoki przestały symulować dane; kontynuować testy XSS |
| TUI/Makefile | uruchamianie usługi, binarka, testy kontraktu CSV | Wymaga integracyjnego testu start/stop i uprawnień |
| Instalator | instalacja, aktualizacja, kopie, uprawnienia, kompilacja | Osobny pełny audyt instalatora nadal wymagany |
| CI | Android, firmware, PHP, UI/TUI | Kontrole działają; przypiąć akcje do pełnych SHA |

## 4. Uwagi do CI i łańcucha dostaw

Workflow używają akcji GitHub przez ruchome etykiety wersji, np. `actions/checkout@v4`, `actions/setup-java@v4` i `actions/upload-artifact@v4`. To powszechna praktyka, ale dla utwardzonego łańcucha dostaw zalecane jest przypinanie akcji do pełnych SHA commitów i okresowa aktualizacja przez kontrolowany proces.

Kompilacja firmware potwierdza zgodność kodu z toolchainem i bibliotekami przypiętymi w workflow. Nie potwierdza polaryzacji przekaźników, zachowania przy brownout/reset, temperatury grzałki, mechanicznego zakresu serwa ani bezpieczeństwa obciążeń.

## 5. Zalecana kolejność napraw

1. **P0:** HVM-001, HVM-002, HVM-003 — wycieki szczegółów i ochrona konfiguracji.
2. **P0:** HVM-006 — ograniczyć ekspozycję sieciową usług.
3. **P1:** HVM-004, HVM-005, HVM-007, HVM-008 — odporność i wiarygodność kolektora.
4. **P1:** HVM-009, HVM-010 — prawdziwa diagnostyka czujników.
5. **P1:** HVM-011, HVM-012 — spójność statystyk i eksportu.
6. **P2:** przypięcie akcji CI do SHA, testy TLS Android, testy instalatora i testy integracyjne.

## 6. Zakres niepotwierdzony

- Brak testów na fizycznym Raspberry Pi, Pico, module przekaźników, grzałce, wentylatorze, pompie i serwie.
- Brak HIL i pomiarów oscyloskopem.
- Brak testu produkcyjnego Android z zaufanym TLS.
- Brak pełnego audytu uprawnień instalatora, wszystkich skryptów pomocniczych i konfiguracji systemd.
- Brak testu odtwarzania kopii SQLite po awarii zasilania.
- Brak testów penetracyjnych sieci i API.

**Decyzja:** `main` może być bazą dalszego rozwoju i napraw programowych. Nie oznacza to zgody na uruchomienie automatycznego sterowania obciążeniami w pasiece. Najpierw należy zamknąć HVM-001–009, następnie przeprowadzić walidację sprzętową.
