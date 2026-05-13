# ApiaryGuard - Aplikacja Android do obsługi uli

Aplikacja mobilna na system Android służąca do monitorowania uli pszczelich poprzez połączenie z Raspberry Pi.

## Funkcje

- **Konfiguracja połączenia**: Wprowadź statyczny adres IP Raspberry Pi i port API
- **Weryfikacja połączenia**: Automatyczne sprawdzenie poprawności połączenia z API przed zapisaniem
- **Zapisywanie konfiguracji**: Adres IP i port są zapamiętywane w preferencjach aplikacji
- **Wyświetlanie danych uli**: Temperatura, wilgotność, waga, poziom baterii
- **Statusy kolorystyczne**: 
  - 🟢 OK - wszystkie parametry w normie
  - 🟡 UWAGA - ostrzeżenie (np. niska bateria, ekstremalna wilgotność)
  - 🔴 ALERT - alert (temperatura poza zakresem 10-40°C)
- **Pull-to-refresh**: Odświeżanie danych przez przeciągnięcie
- **Architektura MVVM**: ViewModel, Repository, LiveData

## Struktura projektu

```
app/src/main/java/com/apiguard/apiary/
├── model/
│   └── ApiaryData.kt          # Model danych uli
├── network/
│   ├── ApiaryApiService.kt    # Interfejs Retrofit API
│   └── RetrofitClient.kt      # Klient HTTP
├── repository/
│   └── ApiaryRepository.kt    # Warstwa danych i logiki biznesowej
└── ui/
    ├── MainActivity.kt        # Główna aktywność
    ├── MainViewModel.kt       # ViewModel
    └── ApiaryAdapter.kt       # Adapter RecyclerView
```

## Wymagania API (Raspberry Pi)

Aplikacja oczekuje API REST na Raspberry Pi z następującymi endpointami:

### Health Check
```
GET http://{IP}:{PORT}/api/health
Response: {"status": "ok"}
```

### Dane uli
```
GET http://{IP}:{PORT}/api/data
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

## Konfiguracja

### Minimalne wymagania systemowe
- Android 7.0 (API 24) lub wyższy
- Połączenie internetowe (lokalna sieć WiFi)

### Port domyślny
- **5000** (możliwość zmiany w dialogu konfiguracji)

## Budowanie

1. Otwórz projekt w Android Studio
2. Poczekaj na synchronizację Gradle
3. Podłącz urządzenie lub uruchom emulator
4. Kliknij Run (Shift+F10)

## Technologia

- **Język**: Kotlin
- **Minimalne SDK**: 24 (Android 7.0)
- **Docelowe SDK**: 34 (Android 14)
- **Biblioteki**:
  - Retrofit 2 - komunikacja HTTP
  - Room - baza danych lokalnych (opcjonalnie)
  - Material Design Components - UI
  - Lifecycle & ViewModel - architektura
  - Preference - zapis ustawień

## Używanie aplikacji

1. **Pierwsze uruchomienie**:
   - Aplikacja poprosi o podanie adresu IP Raspberry Pi
   - Wpisz adres (np. `192.168.1.100`) i port (domyślnie `5000`)
   - Kliknij "Połącz" - aplikacja zweryfikuje połączenie
   - Jeśli sukces - dane zostaną pobrane i wyświetlone

2. **Odświeżanie danych**:
   - Przeciągnij listę w dół (pull-to-refresh)
   - Lub kliknij przycisk FAB w prawym dolnym rogu

3. **Zmiana adresu IP**:
   - Kliknij przycisk preferencji (FAB)
   - Wprowadź nowy adres i zatwierdź

## Licencja

MIT License
