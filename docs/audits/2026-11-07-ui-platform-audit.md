# Audyt działania interfejsów użytkownika — 2026-11-07

## Zakres i podstawa audytu

Audyt źródłowy wykonano dla repozytorium `bartoszruta26-droid/Hive_monitor`, na stanie `main`:

- commit bazowy: `fc44c7e91511c110e851d93f7551f30fda4603de`;
- porównawczy stan roboczy: `912bf50aacaf8ddc5aa5d09c3a432fefe53661ae`;
- porównanie wykazało, że zmiany gałęzi Safe Mode nie modyfikują plików TUI, WebUI ani Androida; audyt UI odnosi się więc do kodu z `main`.

Zakres: TUI Bash, WebUI HTML/JavaScript/PHP, aplikacja Android/Kotlin, integracja z HTTP API i kolektorem, testy automatyczne oraz obecność klienta desktopowego.

**Ograniczenie:** jest to audyt statyczny kodu i konfiguracji. Nie wykonano testu na fizycznym Raspberry Pi, w przeglądarce podłączonej do działającego kolektora ani na urządzeniu/emulatorze Android. Nie należy utożsamiać poprawnej kompilacji z potwierdzeniem działania interfejsu w środowisku docelowym.

## Podsumowanie

| Interfejs | Stan w repozytorium | Ocena |
|---|---|---|
| TUI | Bash, 4 zakładki, odczyt HTTP/CSV i logów | Działa częściowo; znaleziono błędy obsługi klawiatury i mylące wartości zastępcze |
| WebUI | HTML/CSS/JS, PHP API, dashboard | Dashboard odczytowy częściowo zaimplementowany; historia, logi i konfiguracja sensorów/efektorów są niepełne lub pozorne |
| Desktop GUI | Brak osobnego projektu desktopowego w drzewie | Niezaimplementowany w audytowanym stanie |
| Android GUI | Natywna aplikacja Kotlin, jedna główna aktywność | Działa jako podstawowy podgląd uli; brak wielu funkcji i niepełne wykorzystanie pamięci lokalnej |

## Ustalenia

### UI-01 — P1: TUI może zakończyć działanie po typowych operacjach klawiaturą

**Plik:** `src/rpi_tui/apiary_tui.sh`

Skrypt włącza `set -e`, a następnie używa arytmetycznych poleceń Bash, których status wyjścia jest równy 1, gdy wartość wyrażenia wynosi zero. Przykłady:

- `((SCROLL_OFFSET_LOG++))` — przy pierwszym przewinięciu z wartości 0;
- `((SCROLL_OFFSET_DEBUG++))` — analogicznie na zakładce DEBUG;
- `((CURRENT_TAB = (CURRENT_TAB + 1) % TAB_COUNT))` — przy przejściu z ostatniej zakładki na pierwszą, gdy wynik wynosi 0.

W połączeniu z `set -e` może to zakończyć proces TUI zamiast wykonać nawigację. Sam test `bash -n` tego błędu nie wykrywa.

**Zalecenie:** zamienić te operacje na bezpieczne przypisania arytmetyczne, np. `SCROLL_OFFSET_LOG=$((SCROLL_OFFSET_LOG + 1))`, albo jawnie obsłużyć status; dodać testy automatyczne sekwencji klawiszy w pseudo-terminalu.

### UI-02 — P1: TUI wyświetla przykładowe wpisy i wartości zastępcze jako zwykłe dane

**Plik:** `src/rpi_tui/apiary_tui.sh`

- `load_logs()` dopisuje fikcyjne zdarzenia, jeżeli plik logu jest pusty (m.in. połączenie z UL-001 i alarm temperatury UL-002).
- `load_debug()` dopisuje przykładowe komunikaty, jeżeli plik debug jest pusty.
- `draw_hive_panel()` stosuje wartości zastępcze dla pustych pól, m.in. temperaturę 25.0, wilgotność 50.0, wagę 40.0 i baterię 90.

To może spowodować, że operator odczyta dane demonstracyjne lub brak odczytu jako rzeczywisty pomiar/zdarzenie. W kolektorze i innych klientach przyjęto już zasadę odróżniania danych rzeczywistych od demonstracyjnych; TUI nie jest z nią spójne.

**Zalecenie:** w trybie produkcyjnym pokazywać `BRAK DANYCH` / `BRAK LOGÓW`; dane demo udostępniać wyłącznie przez jawny przełącznik i stale widoczną etykietę `DEMO`. Brak wartości musi być reprezentowany jako brak, nie jako liczba domyślna.

### UI-03 — P2: wyłączenie automatycznego odświeżania nie wyłącza pobierania danych

**Plik:** `src/rpi_tui/apiary_tui.sh`

`draw_hive_panel()` wywołuje `fetch_hive_data()` przy każdym renderowaniu. Po ustawieniu `AUTO_REFRESH=false` pętla główna nadal renderuje ekran, a jedynie skraca przerwę do 0,1 sekundy. Na zakładce ULIE oznacza to wielokrotne zapytania HTTP w ciągu sekundy zamiast zatrzymania odświeżania. Klawisz `r` ma pustą obsługę; ręczne odświeżenie nie jest odrębną operacją.

**Zalecenie:** oddzielić pobieranie danych od renderowania. Pętla powinna pobierać dane wyłącznie po upływie interwału, po żądaniu ręcznym lub po wznowieniu odświeżania. Dodać licznik zapytań i test częstotliwości.

### UI-04 — P1/P2: WebUI omija własne PHP API i łączy się bezpośrednio z portem 8080

**Pliki:** `src/webui/app.js`, `src/webui/api.php`

`app.js` buduje adres API jako `<protokół strony>://<host>:8080` i wykonuje bezpośrednio żądania do `/health` oraz `/api/hives`. Jednocześnie repozytorium zawiera `api.php`, który pełni rolę pośrednika, waliduje metody, obsługuje klucz API dla operacji zapisu i przekazuje żądania do kolektora.

Frontend nie korzysta z tego pośrednika. Wdrożenie WebUI za HTTPS może więc próbować zestawić HTTPS bezpośrednio z portem kolektora 8080, mimo że kolektor jest serwerem HTTP. Omija też warstwę PHP, w której skoncentrowano część polityk dostępu i obsługi błędów.

**Zalecenie:** ustalić jeden kontrakt komunikacyjny. Preferowane jest użycie tego samego originu i `api.php` jako kontrolowanego pośrednika albo poprawnie skonfigurowanego reverse proxy. Dodać testy dla HTTP i HTTPS oraz sprawdzenie, że frontend nie łączy się bezpośrednio z portem kolektora w środowisku produkcyjnym.

### UI-05 — P2: część WebUI to makiety lub komunikaty o braku implementacji

**Pliki:** `src/webui/app.js`, `src/webui/index.html`

- Historia: `loadHistoryData()` ukrywa wykres i zawsze pokazuje komunikat o braku historii.
- Logi: `loadLogs()` pokazuje stały komunikat o braku endpointu; `clearLogs()` nie czyści dziennika.
- Sensory i efektory: tabele są podglądowe; dodawanie i sterowanie kończy się komunikatem `alert`, bez zapisu.
- Ustawienia: `saveConfigToBackend()` zapisuje dane do `localStorage` w bieżącej przeglądarce, a nie do backendu. Komunikat sukcesu może sugerować zapis centralny.

Wyłączenie sterowania efektorami jest obecnie właściwym ograniczeniem bezpieczeństwa, ponieważ kolektor nie udostępnia bezpiecznego, autoryzowanego sterowania. Problemem jest rozbieżność między elementami interfejsu a ich rzeczywistą funkcją.

**Zalecenie:** oznaczyć niezaimplementowane sekcje jako `NIEDOSTĘPNE`/„w przygotowaniu”, usunąć aktywne formularze bez obsługi albo rzeczywiście je wyłączyć. Ustawienia lokalne nazwać ustawieniami tej przeglądarki. Historię i logi włączyć dopiero po wdrożeniu odpowiednich usług.

### UI-06 — P2: Android zapisuje cache Room, ale ekran nie korzysta z niego w trybie offline

**Pliki:** `android_app/app/src/main/java/com/apiguard/apiary/repository/ApiaryRepository.kt`, `ui/MainViewModel.kt`, `ui/MainActivity.kt`

Repozytorium zapisuje poprawne odczyty do Room i udostępnia funkcje pobierania historii lokalnej. Jednak `MainViewModel` przy błędzie sieci ustawia listę danych na pustą, a `MainActivity` pokazuje stan pusty. Nie odczytuje wtedy danych z Room. W praktyce cache istnieje, ale nie zapewnia podglądu offline ani historii w interfejsie.

**Zalecenie:** wprowadzić jawny model stanu: `LIVE`, `CACHED`, `OFFLINE_NO_DATA`. Po utracie połączenia wyświetlać ostatnie zapisane pomiary z czasem ostatniego odczytu i wyraźną etykietą `DANE Z CACHE — NIEBIEŻĄCE`. Nie przedstawiać cache jako danych bieżących.

### UI-07 — P2: konfiguracja cleartext HTTP Androida wymaga weryfikacji na urządzeniu

**Pliki:** `android_app/app/src/main/res/xml/network_security_config.xml`, `AndroidManifest.xml`, `app/build.gradle`

Konfiguracja deklaruje wzorce takie jak `10.*`, `172.16.*` i `192.168.*` jako domeny dozwolone dla HTTP. Nie są to wzorce CIDR; składnia konfiguracji bezpieczeństwa Androida nie zapewnia w ten sposób ogólnego dopasowania zakresów IPv4. Jednocześnie aplikacja przyjmuje dowolny adres IPv4 i port od użytkownika. Test kompilacji APK nie dowodzi, że połączenie HTTP do każdego prywatnego adresu IP działa na docelowych wersjach Androida.

**Zalecenie:** przetestować połączenia na rzeczywistych wersjach Androida i zastąpić niejednoznaczną listę wildcardów kontrolowanym rozwiązaniem. Preferować HTTPS z zaufanym certyfikatem; jeśli HTTP jest potrzebne wyłącznie w development, ograniczyć wyjątek do wariantu debug i jasno oddzielić go od release.

### UI-08 — P2: aplikacja Android ma ograniczony zakres funkcjonalny względem WebUI/TUI

**Pliki:** `android_app/app/src/main/java/com/apiguard/apiary/ui/MainActivity.kt`, `ApiaryAdapter.kt`

Obecny klient mobilny udostępnia listę uli, cztery podstawowe pomiary i prosty dialog szczegółów. Nie ma widoków historii/wykresów, dziennika, konfiguracji sensorów, statusu efektorów ani rozbudowanego centrum alarmów. Nie jest to błąd kompilacji, lecz niepełna zgodność funkcjonalna z pozostałymi klientami.

**Zalecenie:** ustalić wspólną macierz funkcji i zaplanować moduły mobilne. Sterowanie efektorami pozostawić wyłączone, dopóki nie będzie bezpiecznego API, autoryzacji, potwierdzenia stanu i procedury awaryjnej.

### UI-09 — P2: brak testów interakcji UI i testów end-to-end

**Pliki:** `.github/workflows/ui-static-checks.yml`, `.github/workflows/android-build.yml`

Obecne CI sprawdza składnię JavaScript/PHP/Bash, kontrakt kolumn CSV, wybrane zabezpieczenia i kompilację kolektora. Android CI uruchamia testy jednostkowe i buduje APK. Nie ma testów TUI w pseudo-terminalu, testów przeglądarkowych end-to-end ani uruchamianych testów Android instrumentation/Espresso. Nie ma więc automatycznej kontroli rzeczywistej nawigacji, kliknięć, odświeżania, utraty łączności, rotacji ekranu ani zachowania offline.

**Zalecenie:** dodać:
1. testy TUI z pseudo-terminalem (nawigacja, przewijanie, resize, wyjście, tryb offline);
2. testy WebUI w przeglądarce z mockowanym i rzeczywistym kontraktem API;
3. testy Android UI/Espresso oraz testy sieciowe z serwerem testowym;
4. wspólne testy kontraktowe JSON dla WebUI i Androida.

### UI-10 — P1: brak aplikacji desktopowej

W drzewie repozytorium nie ma osobnego projektu desktopowego ani katalogu/konfiguracji dla Qt, GTK, Electron lub innego natywnego GUI. Lista gałęzi również nie wykazała gałęzi nazwanej wprost desktop/Qt. TUI na Raspberry Pi nie jest desktop GUI.

**Zalecenie:** potraktować desktop GUI jako brakującą platformę, nie jako gotowy klient. Przed implementacją wybrać technologię (jeśli wymaganiem pozostaje Qt — Qt 6), zdefiniować wspólny klient API, model danych, nawigację i zakres funkcji. Nie kopiować logiki domenowej osobno do czterech interfejsów.

## Kontrola CI

Ostatnie znalezione udane przebiegi:

- [UI static checks — run 42344987347](https://github.com/bartoszruta26-droid/Hive_monitor/actions/runs/42344987347) — SUCCESS, commit `ae32bb9e85e5cf829b7781e146464f0308e64e23`;
- [Android build and tests — run 42344987315](https://github.com/bartoszruta26-droid/Hive_monitor/actions/runs/42344987315) — SUCCESS, ten sam commit.

To potwierdza wykonanie testów i kompilacji przewidzianych w tych workflow, nie działanie interfejsów w przeglądarce/terminalu/urządzeniu. UI nie było zmieniane między bazowym `main` a audytowanym stanem gałęzi Safe Mode, dlatego wyniki są użyteczne jako ostatnia kontrola CI kodu UI, ale nie są testami runtime.

## Zalecana kolejność napraw

1. **P1 — TUI:** naprawić kończenie procesu przez `set -e` i arytmetykę; usunąć domyślne fikcyjne pomiary i przykładowe logi z trybu produkcyjnego.
2. **P1 — WebUI/API:** ujednolicić ścieżkę komunikacji i przetestować wdrożenie HTTPS.
3. **P1 — Desktop:** podjąć decyzję technologiczną i zaplanować osobny projekt desktopowy.
4. **P2 — TUI:** rozdzielić renderowanie od odświeżania; poprawić klawisz ręcznego odświeżenia i tryb pauzy.
5. **P2 — WebUI:** uczciwie oznaczyć makiety, wdrożyć lub usunąć nieaktywne formularze i wyjaśnić lokalny charakter ustawień.
6. **P2 — Android:** wyświetlać cache jako niebieżące dane, zweryfikować politykę HTTP i rozszerzyć testy UI.
7. **P2 — wspólna architektura:** ustanowić jeden kontrakt API/JSON, wspólny słownik stanów i macierz zgodności funkcji wszystkich klientów.

## Kryteria odbioru po naprawach

- TUI przechodzi pełny zestaw sekwencji klawiszy bez zamknięcia procesu; pauza rzeczywiście zatrzymuje pobieranie; brak pomiaru nigdy nie jest prezentowany jako wartość zastępcza.
- WebUI działa przez jeden udokumentowany kanał API zarówno w HTTP development, jak i HTTPS produkcyjnym; każda aktywna kontrolka ma działanie albo jest wyraźnie wyłączona.
- Android poprawnie działa na wspieranych wersjach systemu, wyświetla status LIVE/CACHED/OFFLINE, a testy potwierdzają obsługę utraty łączności.
- Desktop GUI ma osobny build, testy uruchomieniowe i pokrycie podstawowych przypadków użycia.
- Wszystkie cztery interfejsy prezentują ten sam stan i znaczenie pomiarów, timestampów, jakości danych oraz alarmów.
