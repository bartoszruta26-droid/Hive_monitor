# APIARY Guard TUI - Terminal UI dla Raspberry Pi

## Opis

Aplikacja terminalowa (TUI) do monitoringu i zarządzania ulami podłączonymi przez Ethernet z Raspberry Pi 2. Napisana w czystym Bashu i C++ bez zależności od Pythona.

## Struktura plików

```
src/rpi_tui/
├── apiary_tui.sh        # Główny skrypt Bash TUI
├── apiary_logger.cpp    # Moduł logowania C++
├── Makefile             # Plik budowania dla C++
└── README.md            # Ta dokumentacja
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

# Kompilacja
g++ -std=c++17 -pthread -DSTANDALONE_TEST -o apiary_logger apiary_logger.cpp

# Uruchomienie testowe
./apiary_logger
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

## Rotacja logów

System automatycznie rotuje pliki logów gdy osiągną rozmiar 10MB:
- `apiary.log` → `apiary.log.1` → `apiary.log.2` → ... → `apiary.log.5`

## Integracja z systemem

### Usługa systemd (opcjonalnie):

Utwórz plik `/etc/systemd/system/apiary-tui.service`:

```ini
[Unit]
Description=APIARY Guard TUI Service
After=network.target

[Service]
Type=simple
User=pi
ExecStart=/workspace/src/rpi_tui/apiary_tui.sh
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable apiary-tui
sudo systemctl start apiary-tui
```

## Rozszerzenia (przyszłe)

Planowane funkcje w kolejnych wersjach:
- Komunikacja MQTT z ulami (Pico + W5100/W6100)
- Serwer HTTP/API dostępny online
- Powiadomienia email/SMS
- Wykresy i statystyki
- Konfiguracja przez Web UI
- Obsługa wielu Raspberry Pi jako klastra

## Licencja

Projekt na licencji MIT - zobacz plik LICENSE w głównym katalogu.

## Autor

APIARY Guard Team © 2024
