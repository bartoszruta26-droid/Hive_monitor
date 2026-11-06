# ApiaryGuard - Aplikacja Android do obsługi uli

Aplikacja mobilna na system Android służąca do monitorowania uli pszczelich poprzez połączenie z Raspberry Pi.

## Funkcje

- **Konfiguracja połączenia**: Przy pierwszym uruchomieniu aplikacja prosi o podanie statycznego adresu IP Raspberry Pi i portu API
- **Weryfikacja połączenia**: Automatyczne sprawdzenie poprawności połączenia z endpointem `/health` przed zapisaniem konfiguracji
- **Zapisywanie konfiguracji**: Adres IP i port są zapamiętywane w SharedPreferences tylko po pomyślnej weryfikacji
- **Pobieranie danych uli**: Temperatura, wilgotność, waga, poziom baterii z API Raspberry Pi
- **Cache danych historycznych**: Wszystkie pobrane dane są zapisywane lokalnie w bazie Room dla szybkiego dostępu offline i analizy historycznej
- **Statusy kolorystyczne**:
  - 🟢 OK - wszystkie parametry w normie
  - 🟡 UWAGA - ostrzeżenie (np. niska bateria, ekstremalna wilgotność)
  - 🔴 ALERT - alert (temperatura poza zakresem 10-40°C)
- **Pull-to-refresh**: Odświeżanie danych przez przeciągnięcie
- **Architektura MVVM**: ViewModel, Repository, LiveData, Room Database

## Struktura projektu

```
app/src/main/java/com/apiguard/apiary/
├── model/
│   └── ApiaryData.kt          # Model danych uli z API
├── data/
│   ├── local/
│   │   ├── ApiaryDao.kt       # DAO dla Room Database
│   │   └── ApiaryDatabase.kt  # Baza danych Room
│   ├── remote/
│   │   └── ApiaryApiService.kt# Interfejs Retrofit API
│   └── model/
│       └── ApiaryReading.kt   # Encja Room dla cache'owanych danych
├── network/
│   └── RetrofitClient.kt      # Klient HTTPS z walidacją hosta i certyfikatu
├── repository/
│   └── ApiaryRepository.kt    # Warstwa danych: API + cache Room
└── ui/
    ├── MainActivity.kt        # Główna aktywność
    ├── MainViewModel.kt       # ViewModel z obsługą cache
    └── ApiaryAdapter.kt       # Adapter RecyclerView
```

## Wymagania API (Raspberry Pi)

Aplikacja oczekuje API REST na Raspberry Pi z następującymi endpointami:

### Health Check
```
GET https://{HOST}:{PORT}/health
Response: 200 OK (puste body lub JSON)
```

### Lista uli
```
GET https://{HOST}:{PORT}/api/hives
Response: {
  "timestamp": 1704067200,
  "hive_count": 1,
  "hives": {
    "hive_1": {
      "temp": 35.5,
      "hum": 60.0,
      "weight": 45.2,
      "bat": 85,
      "online": true,
      "last_seen": 1704067200
    }
  }
}
```

### Historia

Bieżący kolektor nie udostępnia zdalnego endpointu historii. Aplikacja zapisuje poprawne, aktualne odczyty w Room; prezentacja i przegląd historii lokalnej są odrębnym zakresem rozwoju.

## Technologia

- **Język**: Kotlin
- **Minimalna wersja Android**: API 24 (Android 7.0)
- **Target SDK**: API 34 (Android 14)
- **Biblioteki**:
  - Retrofit 2 - komunikacja HTTPS z weryfikacją certyfikatu
  - Room - baza danych SQLite dla cache
  - ViewModel & LiveData - architektura MVVM
  - Coroutines - operacje asynchroniczne
  - PreferenceManager - zapis hosta i portu (bez sekretów)
  - Material Design - komponenty UI

## Użycie

1. **Pierwsze uruchomienie**:
   - Aplikacja wyświetli dialog z prośbą o adres IP Raspberry Pi
   - Domyślny port to 8080 (można zmienić)
   - Kliknij "Połącz" aby zweryfikować połączenie

2. **Połączenie zakończone sukcesem**:
   - Konfiguracja jest zapisywana
   - Aplikacja pobiera dane z API
   - Dane są cache'owane w bazie lokalnej

3. **Kolejne uruchomienia**:
   - Aplikacja automatycznie łączy się z zapisanym adresem IP
   - Pobiera aktualne dane z API; przy błędzie usuwa poprzednie pomiary z widoku, aby nie przedstawiać ich jako bieżących

4. **Zmiana adresu IP**:
   - Kliknij przycisk FAB (ikona ustawień) w prawym dolnym rogu
   - Podaj nową nazwę hosta lub adres IP i zweryfikuj połączenie

## Buildowanie

Otwórz projekt w Android Studio i:
1. Zsynchronizuj Gradle
2. Uruchom na emulatorze lub urządzeniu

Lub z linii poleceń:
```bash
gradle -p android_app testDebugUnitTest assembleDebug
```

APK zostanie utworzony w `app/build/outputs/apk/debug/`


## Zgodność z kolektorem Raspberry Pi

Aplikacja korzysta z tras kolektora: `GET /health` i `GET /api/hives` przez reverse proxy TLS, domyślnie na porcie `443`. Sam kolektor pozostaje dostępny lokalnie pod `127.0.0.1:8080`. Odpowiedź `/api/hives` zawiera obiekt `hives` indeksowany identyfikatorem ula; pola `temp`, `hum`, `weight`, `bat`, `online` i `last_seen` są mapowane na model aplikacji. Czas kolektora jest podawany w sekundach i konwertowany na milisekundy dla Androida. Rekordy niekompletne lub nieaktualne nie są zapisywane do lokalnego cache. Historia zdalna pozostaje nieobsługiwana przez bieżący kolektor; dostępne są lokalnie zapisane odczyty.


### Polityka HTTPS

Warianty `debug` i `release` wymagają HTTPS; Android blokuje cleartext traffic w manifeście i konfiguracji bezpieczeństwa sieci. Klient weryfikuje certyfikat i nazwę hosta standardowym mechanizmem Android/OkHttp. Nie akceptuje certyfikatów samopodpisanych ani dowolnych certyfikatów.

Kolektor C++ nasłuchuje lokalnie na `127.0.0.1:8080`. Przed połączeniem aplikacji należy skonfigurować reverse proxy TLS na porcie 443 z certyfikatem zaufanym przez Androida i poprawnym SAN dla wpisanej nazwy DNS lub adresu IP. Port 8080 nie może być wystawiony w LAN/WAN. Szczegóły: [ANDROID_TLS_DEPLOYMENT.md](../docs/ANDROID_TLS_DEPLOYMENT.md).

Room zapisuje odczyty w prywatnym katalogu aplikacji. Kopie zapasowe systemu Android są wyłączone (`allowBackup=false`). Aplikacja zapisuje tylko host i port; nie zawiera osadzonego wspólnego klucza API.
