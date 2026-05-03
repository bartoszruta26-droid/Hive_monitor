# APIARY Guard TUI - Terminal UI dla Raspberry Pi

## Opis

Aplikacja terminalowa (TUI) do monitoringu i zarządzania ulami podłączonymi przez Ethernet z Raspberry Pi 2. Napisana w czystym Bashu i C++ bez zależności od Pythona.

## Struktura plików

```
src/rpi_tui/
├── apiary_tui.sh         # Główny skrypt Bash TUI
├── apiary_logger.cpp     # Moduł logowania C++
├── apiary_collector.cpp  # Kolektor danych z wielu uli (UDP Server + Symulator)
├── Makefile              # Plik budowania dla C++
└── README.md             # Ta dokumentacja
```

## Wymagania

### Dla wersji Bash:
- `bash` (wersja 4.0+)
- `ncurses-bin` (pakiet z tput)
- Standardowe narzędzia Unix (tail, date, etc.)

### Dla wersji C++:
- `g++` z obsługą C++17
- Biblioteka pthreads

## Instalacja na Raspberry Pi

```bash
# Instalacja zależności
sudo apt-get update
sudo apt-get install -y ncurses-bin build-essential

# Utwórz katalogi systemowe
sudo mkdir -p /var/log/apiaryguard
sudo mkdir -p /etc/apiaryguard
sudo mkdir -p /var/lib/apiaryguard
sudo chmod 777 /var/log/apiaryguard
```

## Uruchomienie

### Wersja Bash (zalecana dla prostoty):

```bash
cd /workspace/src/rpi_tui
chmod +x apiary_tui.sh
./apiary_tui.sh
```

### Wersja C++ (kompilacja i uruchomienie):

```bash
cd /workspace/src/rpi_tui

# Budowanie wszystkiego (logger + kolektor)
make

# Lub ręczna kompilacja samego kolektora
g++ -std=c++17 -pthread -o apiary_collector apiary_collector.cpp apiary_logger.cpp

# Uruchomienie kolektora w trybie symulacji (test bez fizycznych uli)
./apiary_collector --sim

# Uruchomienie kolektora w trybie sieciowym (odbiera dane z Pico przez UDP port 5005)
./apiary_collector
```

## Funkcje TUI

### Zakładki:
1. **LOGI** - Dziennik zdarzeń systemowych z kolorowaniem poziomów
2. **DEBUG** - Szczegółowe informacje debugowania
3. **ULIE** - Status podłączonych uli (temperatura, wilgotność, bateria)
4. **USTAWIENIA** - Konfiguracja aplikacji

### Klawisze sterujące:
- `←` / `→` - Nawigacja między zakładkami
- `↑` / `↓` - Przewijanie zawartości
- `r` - Ręczne odświeżenie
- `a` - Włącz/wyłącz auto-odświeżanie
- `q` - Wyjście z aplikacji

## Kolektor Danych z Uli (C++)

### Architektura:
- **Tryb sieciowy**: Serwer UDP nasłuchujący na porcie 5005, odbiera dane z mikrokontrolerów Pico
- **Tryb symulacji**: Generuje losowe dane testowe dla demonstration
- **Thread-safe**: Współdzielone dane między wątkami z mutexami
- **Logger**: Zintegrowany system logowania z rotacją plików

### Format danych z Pico:
Mikrokontrolery wysyłają dane w formacie CSV przez UDP:
```
ID,HUM,TEMP,WEIGHT,BAT
```
Przykład: `UL-1,65.5,24.3,45.2,98`

Gdzie:
- `ID` - identyfikator ula (np. UL-1, UL-2)
- `HUM` - wilgotność [%]
- `TEMP` - temperatura [°C]
- `WEIGHT` - waga [kg]
- `BAT` - poziom baterii [%]

### Przykładowe użycie:

```cpp
#include "apiary_logger.cpp"
using namespace apiary;

int main() {
    ApiaryCollector collector;
    
    // Konfiguracja listy uli (IP adresy)
    std::vector<std::string> hives = {"192.168.1.101", "192.168.1.102"};
    collector.configureHives(hives);
    
    // Start w trybie sieciowym
    collector.start(false);
    
    // Pobierz aktualny stan jako JSON
    std::string json = collector.getStatusJSON();
    std::cout << json << std::endl;
    
    // Pobierz stan jako CSV (dla Bash/TUI)
    std::string csv = collector.getStatusCSV();
    std::cout << csv << std::endl;
    
    return 0;
}
```

### Eksport danych:
- **JSON**: Dla integracji z serwerem online/API
- **CSV**: Dla skryptów Bash i TUI

## System Logowania C++

### Poziomy logowania:
- `DEBUG` - Szczegółowe informacje debugowania (cyan)
- `INFO` - Informacje o działaniu systemu (green)
- `WARNING` - Ostrzeżenia (yellow)
- `ERROR` - Błędy (red)
- `CRITICAL` - Błędy krytyczne (magenta)

### Przykładowe użycie w kodzie C++:

```cpp
#include "apiary_logger.cpp"

using namespace apiary;

int main() {
    // Inicjalizacja
    LoggerConfig config;
    config.log_file = "/var/log/apiaryguard/apiary.log";
    config.debug_file = "/var/log/apiaryguard/debug.log";
    Logger::getInstance().initialize(config);
    
    // Logowanie
    log_info("System uruchomiony", "MAIN");
    log_debug("Inicjalizacja sieci", "NETWORK");
    log_error("Błąd połączenia", "MQTT");
    
    // Specjalne logi dla uli
    Logger::getInstance().logHiveEvent("UL-001", "Pomiar", 35.5, 65.0);
    Logger::getInstance().logNetworkEvent("192.168.1.100", "Połączono");
    
    // Shutdown
    Logger::getInstance().shutdown();
    return 0;
}
```

### Funkcje C-style (dla integracji):

```c
extern "C" {
    void apiary_log_init(const char* log_file, const char* debug_file);
    void apiary_log_shutdown();
    void apiary_log_info(const char* message);
    void apiary_log_error(const char* message);
    void apiary_log_debug(const char* message);
    void apiary_log_hive_event(const char* hive_id, const char* event, 
                               double temp, double humidity);
}
```

## Format plików logów

### Logi systemowe (`/var/log/apiaryguard/apiary.log`):
```
2024-01-15 10:30:45.123 [INFO] [MAIN] System uruchomiony
2024-01-15 10:30:46.456 [WARN] [POWER] Niski poziom baterii w UL-003
2024-01-15 10:30:47.789 [ERROR] [NETWORK] Utracono połączenie z UL-004
```

### Debug (`/var/log/apiaryguard/debug.log`):
```
2024-01-15 10:30:45.100 [DEBUG] [NETWORK] Inicjalizacja modułu sieciowego
2024-01-15 10:30:45.200 [DEBUG] [ETH0] IP 192.168.1.100, MASK 255.255.255.0
2024-01-15 10:30:45.300 [DEBUG] [MQTT] Próba połączenia z brokerem...
```

### Logi kolektora (`/var/log/apiary/collector.log`):
```
2024-01-15 10:31:00.000 [INFO] Skonfigurowano 3 uli do monitorowania
2024-01-15 10:31:00.100 [INFO] Serwer nasłuchujący uruchomiony na porcie UDP 5005
2024-01-15 10:31:05.500 [DEBUG] Zaktualizowano dane dla UL-1 [T:24.3 H:65.5]
2024-01-15 10:31:10.200 [INFO] Wykryto nowy ul: UL-4 z IP 192.168.1.104
```

## Rotacja logów

System automatycznie rotuje pliki logów gdy osiągną rozmiar 10MB:
- `apiary.log` → `apiary.log.1` → `apiary.log.2` → ... → `apiary.log.5`

## Integracja z systemem

### Usługa systemd (opcjonalnie):

Utwórz plik `/etc/systemd/system/apiary-collector.service`:

```ini
[Unit]
Description=APIARY Guard Data Collector
After=network.target

[Service]
Type=simple
User=pi
ExecStart=/usr/local/bin/apiary-collector
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable apiary-collector
sudo systemctl start apiary-collector
```

## Makefile - Cele

```bash
make              # Buduj wszystko (logger + kolektor)
make clean        # Wyczyść pliki tymczasowe
make test         # Testuj logger
make test-collector # Testuj kolektor w trybie symulacji
sudo make install # Instaluj w systemie
make uninstall    # Odinstaluj
make debug        # Build z symbolami debugowania
```

## Rozszerzenia (przyszłe)

Planowane funkcje w kolejnych wersjach:
- ~~Komunikacja UDP z ulami (Pico + W5100/W6100)~~ ✓
- ~~Symulacja danych dla testów~~ ✓
- ~~Eksport JSON/CSV~~ ✓
- Serwer HTTP/API dostępny online
- Wysyłka danych do chmury (MQTT/HTTP)
- Powiadomienia email/SMS
- Wykresy i statystyki w TUI
- Konfiguracja przez Web UI

## Licencja

Projekt na licencji MIT - zobacz plik LICENSE w głównym katalogu.

## Autor

APIARY Guard Team © 2024

