## 🛠️ Instalacja i Konfiguracja

### Wymagania Wstępne

**Sprzętowe:**
- Raspberry Pi 2 Model B (lub nowszy: Pi 3/4/Zero 2 W)
- Arduino Nano V3.0 (ATmega328P)
- Modem LTE USB compatible z Aero2
- Karta microSD 16GB+ Class 10
- Czujniki i efektory zgodnie z BOM
- Zasilacz PoE injector + switch lub PoE splitter HAT

**Programowe:**
- Raspberry Pi OS Lite (64-bit) Bullseye/Bookworm
- .NET 6.0 SDK
- PlatformIO CLI (dla Arduino)
- Apache2 + libapache2-mod-proxy
- SQLite3 / InfluxDB
- Mosquitto MQTT broker
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
# .NET 6.0 SDK
wget https://packages.microsoft.com/config/debian/12/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo apt update
sudo apt install -y dotnet-sdk-6.0 aspnetcore-runtime-6.0

# Apache2 + modules
sudo apt install -y apache2 libapache2-mod-proxy-html libapache2-mod-proxy-http
sudo a2enmod proxy proxy_http ssl headers rewrite
sudo systemctl enable apache2

# Mosquitto MQTT
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto

# SQLite + narzędzia
sudo apt install -y sqlite3 libsqlite3-dev

# I2C tools
sudo apt install -y i2c-tools

# Git i build essentials
sudo apt install -y git build-essential cmake libfftw3-dev

# PlatformIO dla Arduino
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

#### 3. Klonowanie Repozytorium i Build

```bash
cd /opt
sudo git clone https://github.com/apiaryguard/apiaryguard-pro.git
sudo chown -R pi:pi apiaryguard-pro
cd apiaryguard-pro

# Build Arduino firmware
cd hardware/arduino_nano
pio run --target upload
cd ../..

# Build C# applications
cd software/raspberry_pi
dotnet restore
dotnet build --configuration Release

# Publish
dotnet publish ApiaryGuard.WebApi -c Release -o /opt/apiaryguard/publish/webapi
dotnet publish ApiaryGuard.Core -c Release -o /opt/apiaryguard/publish/core
dotnet publish ApiaryGuard.Worker -c Release -o /opt/apiaryguard/publish/worker
```

#### 4. Konfiguracja Systemd Services

```bash
# Kopiowanie unit files
sudo cp /opt/apiaryguard-pro/config/systemd/*.service /etc/systemd/system/

# Reload i enable
sudo systemctl daemon-reload
sudo systemctl enable apiaryguard-core
sudo systemctl enable apiaryguard-worker
sudo systemctl enable apiaryguard-webapi

# Start services
sudo systemctl start apiaryguard-core
sudo systemctl start apiaryguard-worker
sudo systemctl start apiaryguard-webapi

# Verify status
systemctl status apiaryguard-*
```

#### 5. Konfiguracja Apache Virtual Host

```bash
sudo nano /etc/apache2/sites-available/apiaryguard.conf

# Content:
<VirtualHost *:80>
    ServerName apiaryguard.local
    
    ProxyPreserveHost On
    ProxyPass / http://localhost:5000/
    ProxyPassReverse / http://localhost:5000/
    
    # WebSocket support
    RewriteEngine On
    RewriteCond %{HTTP:Upgrade} websocket [NC]
    RewriteCond %{HTTP:Connection} upgrade [NC]
    RewriteRule ^/?(.*) "ws://localhost:5000/$1" [P,L]
    
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
systemctl status apache2 mosquitto apiaryguard-* lte-watchdog

# Dostęp do dashboardu
firefox http://apiaryguard.local

# Test API
curl http://localhost:5000/api/hives

# MQTT test
mosquitto_sub -t "apiaryguard/#" -v
```

---

