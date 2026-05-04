# ApiaryGuard WebUI - Instrukcja Instalacji i Konfiguracji

## Opis systemu

WebUI dla systemu monitoringu uli ApiaryGuard, komunikujący się z:
- **APIARY_COLLECTOR** (C++ daemon) - zbieranie danych z sensorów przez HTTP API JSON na porcie 8080
- **apiary_tui.sh** (Bash TUI) - terminal UI do debugowania i logowania

## Architektura

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│   Przeglądarka  │────▶│   Apache2 + PHP  │────▶│ APIARY_COLLECTOR│
│   (WebUI)       │◀────│   (port 80/443)  │◀────│ (port 8080)     │
└─────────────────┘     └──────────────────┘     └─────────────────┘
                               │
                               ▼
                        ┌──────────────────┐
                        │  apiary_tui.sh   │
                        │  (Terminal UI)   │
                        └──────────────────┘
```

## Wymagania

- Raspberry Pi 2/3/4 lub inny komputer z Linux
- Apache2 z PHP i modułem cURL
- Skompilowany `apiary_collector` z `/workspace/src/rpi_tui/`

## Instalacja

### 1. Instalacja zależności

```bash
sudo apt-get update
sudo apt-get install -y apache2 libapache2-mod-php php-curl
```

### 2. Kompilacja APIARY_COLLECTOR

```bash
cd /workspace/src/rpi_tui
make all
```

### 3. Wdrożenie plików WebUI

Pliki są już skopiowane do `/var/www/html/`:
- `index.html` - główny interfejs użytkownika
- `app.js` - logika JavaScript frontendu
- `api.php` - backend PHP proxy do kolektora

### 4. Konfiguracja Apache (opcjonalnie)

Jeśli chcesz użyć dedykowanego vhost:

```bash
sudo nano /etc/apache2/sites-available/apiary.conf
```

```apache
<VirtualHost *:80>
    ServerName apiary.local
    DocumentRoot /var/www/html
    
    <Directory /var/www/html>
        Options Indexes FollowSymLinks
        AllowOverride All
        Require all granted
    </Directory>
    
    ErrorLog ${APACHE_LOG_DIR}/apiary_error.log
    CustomLog ${APACHE_LOG_DIR}/apiary_access.log combined
</VirtualHost>
```

```bash
sudo a2ensite apiary.conf
sudo systemctl restart apache2
```

### 5. Uruchomienie APIARY_COLLECTOR

```bash
# W trybie demo (bez rzeczywistych sensorów)
/workspace/src/rpi_tui/apiary_collector &

# Lub jako usługa systemd
sudo nano /etc/systemd/system/apiary-collector.service
```

```ini
[Unit]
Description=ApiaryGuard Data Collector
After=network.target

[Service]
Type=simple
ExecStart=/workspace/src/rpi_tui/apiary_collector
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable apiary-collector
sudo systemctl start apiary-collector
```

### 6. Dostęp do WebUI

Otwórz przeglądarkę i przejdź pod adres:
- `http://<IP_RASPBERRY>/` lub
- `http://localhost/` (jeśli jesteś na Raspberry)

## Funkcje WebUI

### 📊 Dashboard
- Podgląd wszystkich uli w czasie rzeczywistym
- Karty z podstawowymi parametrami (temp, wilgotność, waga, bateria)
- Podsumowanie systemu (średnie wartości, alerty)
- Status online/offline każdego ula

### 🔧 Zarządzanie Sensorami
- Lista wszystkich sensorów
- Dodawanie nowych sensorów (modal)
- Edycja i usuwanie sensorów
- Przypisywanie sensorów do uli
- Typy sensorów: temperatura, wilgotność, waga, audio, radar, CO2, VOC, ruch

### ⚙️ Zarządzanie Efektorami
- Lista efektorów (wentylatory, grzałki, nawilżacze, podkarmiacze)
- Przełączanie stanów (WŁ/WYŁ)
- Dodawanie nowych efektorów
- Przypisywanie do uli

### 📈 Dane Historyczne
- Wykresy historyczne (Chart.js)
- Wybór metryki (temp, wilgotność, waga, bateria)
- Wybór ula lub wszystkie ule
- Dane z ostatnich 24 godzin

### 📋 Logi
- Dziennik zdarzeń z kolorowaniem poziomów (INFO, WARN, ERROR, DEBUG)
- Możliwość czyszczenia logów
- Odświeżanie na żądanie

### ⚙️ Ustawienia
- Częstotliwość odświeżania danych
- Próg alarmowy temperatury
- Endpoint API
- Status połączenia z APIARY_COLLECTOR

## Komunikacja z TUI

WebUI współdzieli zasoby z `apiary_tui.sh`:

### Wspólne pliki logów:
- `/var/log/apiaryguard/apiary.log` - logi zdarzeń
- `/var/log/apiaryguard/debug.log` - logi debugowania

### Wspólna konfiguracja:
- `/etc/apiaryguard/` - katalog konfiguracyjny
- `/var/lib/apiaryguard/` - dane aplikacji

### Integracja:
Oba interfejsy (WebUI i TUI) mogą działać jednocześnie, korzystając z tych samych danych z `apiary_collector`.

## API Endpoints

WebUI korzysta z następujących endpointów:

| Endpoint | Metoda | Opis |
|----------|--------|------|
| `/api.php?endpoint=health` | GET | Health check |
| `/api.php?endpoint=hives` | GET | Lista uli z danymi |
| `/api.php?endpoint=status` | GET | Status systemu |
| `/api.php?endpoint=sensors` | GET/POST | Zarządzanie sensorami |
| `/api.php?endpoint=effectors` | GET/POST/PUT | Zarządzanie efektorami |
| `/api.php?endpoint=history` | GET | Dane historyczne |
| `/api.php?endpoint=logs` | GET | Logi systemu |

## Tryb Demo

Gdy `apiary_collector` nie jest dostępny, WebUI automatycznie przechodzi w tryb demo z przykładowymi danymi:
- 3 ule (UL-001, UL-002, UL-003)
- Symulowane odczyty sensorów
- Generowane dane historyczne

## Rozwiązywanie problemów

### WebUI nie ładuje się
```bash
# Sprawdź status Apache
sudo systemctl status apache2

# Sprawdź logi
sudo tail -f /var/log/apache2/error.log
```

### Brak połączenia z kolektorem
```bash
# Sprawdź czy kolektor działa
ps aux | grep apiary_collector

# Sprawdź port 8080
netstat -tlnp | grep 8080

# Uruchom kolektor ręcznie
/workspace/src/rpi_tui/apiary_collector
```

### Błędy PHP
```bash
# Włącz display_errors w development
sudo nano /etc/php/8.2/apache2/php.ini
# display_errors = On

# Restart Apache
sudo systemctl restart apache2
```

## Bezpieczeństwo

W produkcji rozważ:
1. Włączenie HTTPS (Let's Encrypt)
2. Autentykację użytkowników
3. Ograniczenie dostępu do API
4. Firewall (ufw)

```bash
# Przykład ufw
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable
```

## Autor

ApiaryGuard Pro Team
Licencja: MIT
