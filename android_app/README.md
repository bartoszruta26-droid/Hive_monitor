# ApiaryGuard - Aplikacja Android do obsługi uli

Aplikacja mobilna na system Android służąca do monitorowania uli pszczelich poprzez połączenie z Raspberry Pi.

## Funkcje

- **Konfiguracja połączenia**: Przy pierwszym uruchomieniu aplikacja prosi o podanie statycznego adresu IP Raspberry Pi i portu API
- **Weryfikacja połączenia**: Automatyczne sprawdzenie poprawności połączenia z endpointem `/api/health` przed zapisaniem konfiguracji
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
│   └── RetrofitClient.kt      # Klient HTTP z dynamicznym baseUrl
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
GET http://{IP}:{PORT}/api/health
Response: 200 OK (puste body lub JSON)
```

### Lista uli
```
GET http://{IP}:{PORT}/api/apiaries
Response: [
  {
    "id": "hive_1",
    "name": "Ul 1",
    "temperature": 35.5,
    "humidity": 60.0,
    "weight": 45.2,
    "battery": 85,
    "timestamp": 1704067200000
  }
]
```

### Historia ula (opcjonalne)
```
GET http://{IP}:{PORT}/api/apiaries/{id}/history?since={timestamp}
Response: [ ... ] // Lista obiektów jak wyżej
```

## Technologia

- **Język**: Kotlin
- **Minimalna wersja Android**: API 24 (Android 7.0)
- **Target SDK**: API 34 (Android 14)
- **Biblioteki**:
  - Retrofit 2 - komunikacja HTTP
  - Room - baza danych SQLite dla cache
  - ViewModel & LiveData - architektura MVVM
  - Coroutines - operacje asynchroniczne
  - PreferenceManager - zapis konfiguracji
  - Material Design - komponenty UI

## Użycie

1. **Pierwsze uruchomienie**:
   - Aplikacja wyświetli dialog z prośbą o adres IP Raspberry Pi
   - Domyślny port to 5000 (można zmienić)
   - Kliknij "Połącz" aby zweryfikować połączenie

2. **Połączenie zakończone sukcesem**:
   - Konfiguracja jest zapisywana
   - Aplikacja pobiera dane z API
   - Dane są cache'owane w bazie lokalnej

3. **Kolejne uruchomienia**:
   - Aplikacja automatycznie łączy się z zapisanym adresem IP
   - Wyświetla dane z API oraz dane historyczne z cache

4. **Zmiana adresu IP**:
   - Kliknij przycisk FAB (ikona ustawień) w prawym dolnym rogu
   - Podaj nowy adres IP i zweryfikuj połączenie

## Buildowanie

Otwórz projekt w Android Studio i:
1. Zsynchronizuj Gradle
2. Uruchom na emulatorze lub urządzeniu

Lub z linii poleceń:
```bash
./gradlew assembleDebug
```

APK zostanie utworzony w `app/build/outputs/apk/debug/`
