# Hive Monitor — drugi etap napraw programowych

**Zakres:** walidacja wejścia pomiarowego, starsze API PHP, instalatory/systemd, Android TLS, CI supply-chain oraz SQLite lifecycle/concurrency.  
**Tryb:** wyłącznie testy programowe; bez hardware i HIL.

## Naprawy

### FU-005 — zakresy domenowe wejścia telemetrycznego
- Dodano `measurement_validation.h` z jawnie zdefiniowanymi zakresami dla wszystkich pól liczbowych aktualnie odczytywanych przez `parseJSON`.
- Walidacja wymaga identyfikatora ula o ograniczonej długości i dozwolonych znakach, dozwolonego `data_source`, skończonych wartości, właściwych zakresów i całkowitości pól dyskretnych.
- Odrzucenie następuje przed konwersją i zapisaniem rekordu.
- Testy: wartości graniczne, NaN, niepełny token liczbowy, zakres wilgotności, bateria, ruch, ciśnienie, częstotliwość, identyfikator i źródło.

**Granica zakresu:** schemat obejmuje pola faktycznie mapowane przez obecny parser kolektora. Nie deklaruje pełnej walidacji wszystkich 338+ pól struktury `HiveData`, których bieżący parser nie mapuje do rekordu. Zakresy dla pól indeksowych o niejednoznacznej jednostce są celowo szerokie i wymagają potwierdzenia na podstawie kontraktu firmware.

### FU-006 — legacy PHP API
- Zastąpiono osadzony w instalatorze skrypt endpointem `src/webui/legacy_api.php`.
- Każde żądanie wymaga `X-API-Key`; brak konfiguracji zamyka dostęp kodem 503, błędny klucz zwraca 401.
- Endpoint jest tylko do odczytu, otwiera SQLite w trybie read-only, używa zapytań przygotowanych, ścisłego identyfikatora ula, listy dozwolonych akcji i limitu 1–1000.
- Błędy bazy nie ujawniają szczegółów w odpowiedzi HTTP.
- Instalator tworzy losowy klucz 256-bitowy, przechowuje go z uprawnieniami root:www-data 0640 i przekazuje PHP przez konfigurację Apache.
- Dodano test integracyjny PHP/SQLite: brak i błędny klucz, prawidłowy klucz, filtrowanie ula, próby wildcard, limity, tablicowy parametr, nieznana akcja i niedozwolona metoda.

### FU-007 — instalator i systemd
- Kolektor działa jako osobny użytkownik systemowy `apiaryguard`, a nie root ani użytkownik Apache.
- Baza należy do `apiaryguard`, grupa `www-data` ma wyłącznie odczyt; katalog logów kolektora jest odseparowany od logów PHP.
- Usługa ma `ProtectSystem=strict`, `ProtectHome`, `NoNewPrivileges`, ograniczony zestaw capabilities, prywatny katalog tymczasowy i jawne ścieżki zapisu.
- Klucz API PHP nie jest przekazywany do środowiska procesu kolektora.
- Główny instalator odmawia pracy na niebezpiecznym `HOME`, symlinkach katalogów konfiguracyjnych, nieoczekiwanym `origin` Git i niebezpiecznym celu usunięcia. Aktualizacja używa `git pull --ff-only`.
- Kopie zawierające sekrety są tworzone w katalogu 0700 i jako archiwa 0600; plik dziennika instalatora ma tryb 0600.

### FU-008 — Android TLS i dane lokalne
- Cleartext traffic jest zablokowany w manifeście i Network Security Config dla debug/release.
- Retrofit buduje wyłącznie URL HTTPS, waliduje host DNS/IPv4 i port; standardowa weryfikacja certyfikatu i nazwy hosta pozostaje aktywna.
- Domyślny port aplikacji zmieniono na 443; dokumentacja wymaga reverse proxy TLS z zaufanym certyfikatem i pozostawienia kolektora na loopback.
- `allowBackup=false`; aplikacja zapisuje tylko host i port, nie wspólny sekret API.
- Nie dodano cert pinningu bez procedury rotacji certyfikatów.

### FU-009 — przypięcie akcji CI
- Zewnętrzne GitHub Actions w workflow zostały przypięte do pełnych identyfikatorów commit SHA.
- Dodano test statyczny, który odrzuca nieprzypięte akcje.

### FU-010 — SQLite concurrency i awarie
- Dodano mutex cyklu życia serializujący `initialize`, przyjmowanie rekordów i `shutdown`.
- Nowe zapisy po rozpoczęciu zamykania są odrzucane.
- `flushBuffer` zwraca stan powodzenia; błędny batch pozostaje w pamięci, a shutdown nie zamyka połączenia przed skutecznym zatwierdzeniem danych.
- Można ponowić `shutdown` po usunięciu przyczyny błędu.
- Dodano timeout oczekiwania SQLite, obsługę błędów rollback i czyszczenia.
- Testy obejmują wielu producentów, równoległy odczyt statystyk, wymuszone przerwanie transakcji, odzyskanie batcha po usunięciu triggera oraz wyścig producenta z zamykaniem.

## Walidacja CI
Workflow po zmianach musi potwierdzić:
- PHP lint i testy integracyjne endpointów;
- składnię obu instalatorów;
- kontrakty bezpieczeństwa i przypięcie akcji CI;
- kompilację kolektora oraz testy parsera JSON i walidacji domenowej;
- testy HTTP, sieci i SQLite;
- testy jednostkowe Android oraz budowę debug APK.

## Ograniczenia i decyzje wymagające potwierdzenia
1. Nie wszystkie 338+ pól `HiveData` są obecnie mapowane przez parser; pełna walidacja każdego pola wymaga zdefiniowania kontraktu firmware dla jednostek i zakresów.
2. Instalator nie wystawia automatycznie publicznego certyfikatu TLS. Wymaga domeny, DNS i procesu ACME/certyfikacji; dokumentacja opisuje wymagany reverse proxy.
3. Legacy PHP API jest read-only, ale klucz API musi zostać przekazany klientom administracyjnym w bezpiecznym kanale.
4. Baza SQLite jest chroniona uprawnieniami systemowymi; nie wdrożono SQLCipher ani szyfrowania pojedynczej bazy kluczem aplikacyjnym.
5. Nie wykonano testów sprzętowych, elektrycznych, HIL ani prób rzeczywistej sieci radiowej.
