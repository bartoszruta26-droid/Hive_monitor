## 🛠️ Instalacja i Konfiguracja

### Wymagania Wstępne

**Sprzętowe:**
- Raspberry Pi 2 Model B (lub nowszy: Pi 3/4/Zero 2 W)
- Raspberry Pi Pico / Pico W (RP2040)
- Modem LTE USB compatible z Aero2
- Karta microSD 16GB+ Class 10
- Czujniki i efektory zgodnie z BOM
- Zasilacz PoE injector + switch lub PoE splitter HAT

**Programowe:**
- Raspberry Pi OS Lite (64-bit) Bullseye/Bookworm
- Apache2 + libapache2-mod-proxy
- SQLite3 / InfluxDB
- GCC/G++ dla C++ (kompilacja TUI/GUI)
- Bash skrypty systemowe
- Raspberry Pi Pico SDK
- Git

### Krok-po-Kroku Instalacja

#### 1. Przygotowanie Raspberry Pi OS

```bash
# Flash Raspberry Pi Imager lub manual:
wget https://downloads.raspberrypi.org/raspios_lite_arm64/images/raspios_lite_arm64-2023-10-10/2023-10-10-raspios-bookworm-arm64-lite.img.xz
unxz 2023-10-10-raspios-bookworm-arm64-lite.img.xz
sudo dd if=2023-10-10-raspios-bookworm-arm64-lite.img of=/dev/sdX bs=4M conv=fsync

# Boot i podstawowa konfiguracja
ssh pi@raspberrypi.local
# Hasło: raspberry (zmienić natychmiast!)

# Update systemu
sudo apt update && sudo apt upgrade -y
sudo raspi-config nonint do_hostname apiaryguard-gateway
sudo raspi-config nonint do_ssh 0
sudo raspi-config nonint do_i2c 0
sudo raspi-config nonint do_spi 0
```

#### 2. Instalacja Zależności

```bash
# Apache2 + modules
sudo apt install -y apache2 libapache2-mod-proxy-html libapache2-mod-proxy-http
sudo a2enmod proxy proxy_http ssl headers rewrite
sudo systemctl enable apache2

# SQLite + narzędzia
sudo apt install -y sqlite3 libsqlite3-dev

# I2C tools
sudo apt install -y i2c-tools

# Git i build essentials dla C++
sudo apt install -y git build-essential cmake libfftw3-dev libncurses5-dev

# Raspberry Pi Pico SDK (kompilacja z RPi2)
cd /opt
sudo git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=/opt/pico-sdk
```

#### 3. Klonowanie Repozytorium i Build

```bash
cd /opt
sudo git clone https://github.com/apiaryguard/apiaryguard-pro.git
sudo chown -R pi:pi apiaryguard-pro
cd apiaryguard-pro

# Build firmware Raspberry Pi Pico (C++ z Pico SDK)
cd hardware/pico
mkdir build && cd build
cmake ..
make -j4
# Wgranie przez USB (podłącz Pico z wciśniętym BOOTSEL)
cp apiaryguard_firmware.uf2 /media/pi/RPI-RP2/
cd ../..

# Build aplikacji C++ TUI/GUI dla Raspberry Pi 2
cd software/rpi_tui
mkdir build && cd build
cmake ..
make -j4
sudo make install
cd ../..

# Build skryptów Bash i backendu
cd software/bash_backend
chmod +x *.sh
sudo cp *.sh /usr/local/bin/
```

#### 4. Konfiguracja Systemd Services

```bash
# Kopiowanie unit files
sudo cp /opt/apiaryguard-pro/config/systemd/*.service /etc/systemd/system/

# Reload i enable
sudo systemctl daemon-reload
sudo systemctl enable apiaryguard-tui
sudo systemctl enable apiaryguard-backend
sudo systemctl enable apiaryguard-http-server

# Start services
sudo systemctl start apiaryguard-tui
sudo systemctl start apiaryguard-backend
sudo systemctl start apiaryguard-http-server

# Verify status
systemctl status apiaryguard-*
```

#### 5. Konfiguracja Apache Virtual Host

```bash
sudo nano /etc/apache2/sites-available/apiaryguard.conf

# Content:
<VirtualHost *:80>
    ServerName apiaryguard.local
    
    # Proxy do lokalnego backendu HTTP (C++/Bash)
    ProxyPreserveHost On
    ProxyPass / http://localhost:8080/
    ProxyPassReverse / http://localhost:8080/
    
    # API endpoint dla Raspberry Pi Pico
    ProxyPass /pico/api http://localhost:8081/
    ProxyPassReverse /pico/api http://localhost:8081/
    
    # WebSocket support (opcjonalne)
    RewriteEngine On
    RewriteCond %{HTTP:Upgrade} websocket [NC]
    RewriteCond %{HTTP:Connection} upgrade [NC]
    RewriteRule ^/?(.*) "ws://localhost:8080/$1" [P,L]
    
    ErrorLog ${APACHE_LOG_DIR}/apiaryguard_error.log
    CustomLog ${APACHE_LOG_DIR}/apiaryguard_access.log combined
</VirtualHost>

# Enable site
sudo a2ensite apiaryguard
sudo a2dissite 000-default
sudo systemctl reload apache2
```

#### 6. Konfiguracja LTE Aero2

```bash
# Uruchomienie skryptu instalacyjnego
sudo bash /opt/apiaryguard-pro/scripts/bash/network/lte_setup.sh

# Weryfikacja
ip addr show ppp0
ping -c 4 8.8.8.8

# Dodanie watchdoga
sudo cp /opt/apiaryguard-pro/config/systemd/lte-watchdog.service /etc/systemd/system/
sudo systemctl enable lte-watchdog
sudo systemctl start lte-watchdog
```

#### 7. Inicjalizacja Bazy Danych

```bash
cd /opt/apiaryguard-pro/config/database
sudo sqlite3 /var/lib/apiaryguard/data.db < schema.sql
sudo sqlite3 /var/lib/apiaryguard/data.db < indexes.sql
sudo sqlite3 /var/lib/apiaryguard/data.db < seed_data.sql

# Set permissions
sudo chown pi:pi /var/lib/apiaryguard/data.db
```

#### 8. Kalibracja Sensorów

```bash
# Kalibracja wagi
sudo bash /opt/apiaryguard-pro/scripts/bash/sensors/calibrate_scale.sh --known-weight 10.0

# Test mikrofonu
sudo bash /opt/apiaryguard-pro/scripts/bash/sensors/test_microphone.sh --duration 30

# Pełna diagnostyka
sudo bash /opt/apiaryguard-pro/scripts/bash/system/health_check.sh
```

#### 9. Konfiguracja SSL (Production)

```bash
# Let's Encrypt certificate
sudo apt install -y certbot python3-certbot-apache
sudo certbot --apache -d apiaryguard.yourdomain.com

# Auto-renewal
sudo systemctl enable certbot.timer
```

#### 10. Final Verification

```bash
# Sprawdź wszystkie usługi
systemctl status apache2 apiaryguard-tui apiaryguard-backend apiaryguard-http-server lte-watchdog

# Dostęp do TUI/GUI dashboardu
# Jeśli masz monitor: uruchomiono na tty1 lub jako aplikacja X11
# Jeśli headless: SSH z X11 forwarding lub VNC

# Test API
curl http://localhost:8080/api/hives
curl http://localhost:8081/pico/status

# Test komunikacji z Pico
# Pico wysyła dane przez HTTP POST na RPi2
# Sprawdź logi:
tail -f /var/log/apiaryguard/pico_comm.log
```

---

