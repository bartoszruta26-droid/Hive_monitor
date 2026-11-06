#!/bin/bash

# Skrypt instalacyjny dla Apiary Collector na Raspberry Pi
# Instaluje zależności, konfiguruje bazę danych, Apache2 i usługi systemd
# UWAGA: Ten skrypt wykonuje PEŁNĄ instalację z WebUI i API

set -euo pipefail

echo "🚀 Rozpoczynanie PEŁNEJ instalacji Apiary Collector..."
echo ""

# Kolorы
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo -e "${CYAN}Script directory: $SCRIPT_DIR${NC}"
echo -e "${CYAN}Project root: $PROJECT_ROOT${NC}"
echo ""

# 1. Aktualizacja systemu i instalacja ZAANGANSOWANYCH zależności
echo "📦 Aktualizacja listy pakietów i instalacja zaawansowanych zależności..."
echo "   (git, build-essential, sqlite3, libsqlite3-dev, apache2, php, libapache2-mod-php)"
echo ""

sudo apt-get update
sudo apt-get install -y \
    git \
    build-essential \
    sqlite3 \
    libsqlite3-dev \
    libncurses5-dev \
    libncursesw5-dev \
    apache2 \
    php \
    libapache2-mod-php \
    php-sqlite3 \
    openssl \
    || {
        echo -e "${RED}❌ Failed to install dependencies${NC}"
        exit 1
    }

echo -e "${GREEN}✅ Zależności zainstalowane${NC}"
echo ""

# 2. Kompilacja projektu przy użyciu Makefile (NIE cmake)
echo "🔨 Kompilowanie projektu przy użyciu Makefile..."
cd "$SCRIPT_DIR"

if [[ ! -f "Makefile" ]]; then
    echo -e "${RED}❌ Makefile not found in $SCRIPT_DIR${NC}"
    exit 1
fi

make clean
make all -j"$(nproc)" || {
    echo -e "${RED}❌ Kompilacja nieudana${NC}"
    exit 1
}

echo -e "${GREEN}✅ Kompilacja zakończona pomyślnie${NC}"
echo ""

# 3. Instalacja binarek w systemie
echo "⚙️ Instalowanie binarek w systemie..."
sudo make install

echo -e "${GREEN}✅ Binarki zainstalowane${NC}"
echo ""

# Klucz API: jeden losowy sekret współdzielony przez Apache i usługę kolektora.
# Plik jest czytelny wyłącznie dla roota i grupy serwera WWW.
sudo install -d -o root -g www-data -m 0750 /etc/apiaryguard
if ! sudo test -s /etc/apiaryguard/apiary_api_key; then
    umask 077
    openssl rand -hex 32 | sudo tee /etc/apiaryguard/apiary_api_key >/dev/null
fi
sudo chown root:www-data /etc/apiaryguard/apiary_api_key
sudo chmod 0640 /etc/apiaryguard/apiary_api_key
API_KEY="$(sudo cat /etc/apiaryguard/apiary_api_key)"

# Apache udostępnia sekret wyłącznie procesom PHP przez lokalną konfigurację serwera.
sudo tee /etc/apache2/conf-available/apiaryguard-env.conf >/dev/null <<EOF
SetEnv APIARY_API_KEY "$API_KEY"
EOF
sudo chown root:www-data /etc/apache2/conf-available/apiaryguard-env.conf
sudo chmod 0640 /etc/apache2/conf-available/apiaryguard-env.conf
sudo a2enconf apiaryguard-env

# 4. Konfiguracja bazy danych
echo "💾 Konfigurowanie bazy danych SQLite..."
DB_PATH="/var/lib/apiaryguard/apiary.db"
sudo install -d -o www-data -g www-data -m 0750 /var/lib/apiaryguard
sudo touch "$DB_PATH"
sudo chown www-data:www-data "$DB_PATH"
sudo chmod 0660 "$DB_PATH"
sudo install -d -o www-data -g www-data -m 0750 /var/log/apiaryguard

# Inicjalizacja schematu bazy danych z pełną obsługą agregacji
sudo -u www-data sqlite3 "$DB_PATH" <<EOF
-- Tabela dla danych surowych (sekundowych)
CREATE TABLE IF NOT EXISTS raw_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    hive_id TEXT NOT NULL,
    
    -- Podstawowe parametry
    temperature REAL,
    humidity REAL,
    weight REAL,
    battery_level INTEGER,
    co2_eq INTEGER,
    voc_idx INTEGER,
    motion_detected INTEGER,
    
    -- Audio
    audio_rms REAL,
    audio_dominant_freq REAL,
    audio_swarm_prob REAL,
    audio_bee_activity REAL,
    
    -- Radar
    radar_distance REAL,
    radar_energy REAL,
    radar_activity REAL,
    
    -- HX711 Waga
    hx711_current REAL,
    hx711_slope_24h REAL,
    
    -- Temp/Humidity rozszerzone
    th_heat_index REAL,
    th_dew_point REAL,
    th_vpd REAL,
    
    -- Air Quality
    aq_iaq_index REAL,
    
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Tabela dla danych zagregowanych (wszystkie poziomy: MINUTE, HOUR, DAY, WEEK, MONTH, QUARTER, YEAR)
CREATE TABLE IF NOT EXISTS aggregated_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_start INTEGER NOT NULL,
    timestamp_end INTEGER NOT NULL,
    hive_id TEXT NOT NULL,
    agg_type INTEGER NOT NULL,
    
    -- Temperatury
    temperature_avg REAL,
    temperature_min REAL,
    temperature_max REAL,
    
    -- Wilgotność
    humidity_avg REAL,
    humidity_min REAL,
    humidity_max REAL,
    
    -- Waga
    weight_avg REAL,
    weight_min REAL,
    weight_max REAL,
    
    -- Pozostałe średnie
    battery_avg INTEGER,
    co2_avg INTEGER,
    voc_avg INTEGER,
    
    -- Audio
    audio_rms_avg REAL,
    audio_dominant_freq_avg REAL,
    audio_swarm_prob_avg REAL,
    audio_bee_activity_avg REAL,
    
    -- Radar
    radar_distance_avg REAL,
    radar_energy_avg REAL,
    radar_activity_avg REAL,
    
    -- HX711
    hx711_current_avg REAL,
    hx711_slope_24h_avg REAL,
    
    -- Liczniki
    record_count INTEGER,
    
    created_at INTEGER DEFAULT (strftime('%s', 'now'))
);

-- Tabela metadanych agregacji
CREATE TABLE IF NOT EXISTS aggregation_meta (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hive_id TEXT NOT NULL,
    agg_type INTEGER NOT NULL,
    last_timestamp INTEGER,
    last_aggregation INTEGER,
    record_count INTEGER DEFAULT 0,
    UNIQUE(hive_id, agg_type)
);

-- Indeksy dla wydajności
CREATE INDEX IF NOT EXISTS idx_raw_timestamp ON raw_data(timestamp);
CREATE INDEX IF NOT EXISTS idx_raw_hive ON raw_data(hive_id);
CREATE INDEX IF NOT EXISTS idx_raw_hive_timestamp ON raw_data(hive_id, timestamp);

CREATE INDEX IF NOT EXISTS idx_agg_timestamp ON aggregated_data(timestamp_start);
CREATE INDEX IF NOT EXISTS idx_agg_hive ON aggregated_data(hive_id);
CREATE INDEX IF NOT EXISTS idx_agg_type ON aggregated_data(agg_type);
CREATE INDEX IF NOT EXISTS idx_agg_hive_type ON aggregated_data(hive_id, agg_type);
CREATE INDEX IF NOT EXISTS idx_agg_hive_type_ts ON aggregated_data(hive_id, agg_type, timestamp_start);

-- Widok dla najnowszych danych zagregowanych (MINUTE)
CREATE VIEW IF NOT EXISTS latest_aggregated AS
SELECT * FROM aggregated_data 
WHERE agg_type = 1 
ORDER BY timestamp_start DESC 
LIMIT 1;

-- Widok dla historii godzinowej (ostatnie 168 godzin = tydzień)
CREATE VIEW IF NOT EXISTS history_hourly AS
SELECT * FROM aggregated_data 
WHERE agg_type = 2 
ORDER BY timestamp_start DESC 
LIMIT 168;

-- Widok dla historii dziennej (ostatnie 365 dni)
CREATE VIEW IF NOT EXISTS history_daily AS
SELECT * FROM aggregated_data 
WHERE agg_type = 3 
ORDER BY timestamp_start DESC 
LIMIT 365;

-- Widok dla historii tygodniowej (ostatnie 52 tygodnie)
CREATE VIEW IF NOT EXISTS history_weekly AS
SELECT * FROM aggregated_data 
WHERE agg_type = 4 
ORDER BY timestamp_start DESC 
LIMIT 52;

-- Widok dla historii miesięcznej (ostatnie 60 miesięcy = 5 lat)
CREATE VIEW IF NOT EXISTS history_monthly AS
SELECT * FROM aggregated_data 
WHERE agg_type = 5 
ORDER BY timestamp_start DESC 
LIMIT 60;

-- Widok dla historii kwartalnej (ostatnie 40 kwartałów = 10 lat)
CREATE VIEW IF NOT EXISTS history_quarterly AS
SELECT * FROM aggregated_data 
WHERE agg_type = 6 
ORDER BY timestamp_start DESC 
LIMIT 40;

-- Widok dla historii rocznej (ostatnie 100 lat)
CREATE VIEW IF NOT EXISTS history_yearly AS
SELECT * FROM aggregated_data 
WHERE agg_type = 7 
ORDER BY timestamp_start DESC 
LIMIT 100;
EOF

# 4. Konfiguracja Apache2 dla WebUI i API
echo "🌐 Konfigurowanie serwera Apache2..."

# Tworzenie katalogu dla aplikacji webowej
WEB_DIR="/var/www/html/apiary"
sudo mkdir -p $WEB_DIR
sudo chown www-data:www-data $WEB_DIR

# Kopiowanie plików WebUI z katalogu źródłowego
echo "📦 Kopiowanie plików WebUI (index.html, app.js, api.php)..."
WEBUI_SOURCE="$(cd "$SCRIPT_DIR/../webui" && pwd)"

if [[ -d "$WEBUI_SOURCE" ]]; then
    # Kopiowanie wszystkich plików z katalogu webui
    sudo cp "$WEBUI_SOURCE/index.html" "$WEB_DIR/" || {
        echo -e "${YELLOW}⚠️ Nie udało się skopiować index.html${NC}"
    }
    sudo cp "$WEBUI_SOURCE/app.js" "$WEB_DIR/" || {
        echo -e "${YELLOW}⚠️ Nie udało się skopiować app.js${NC}"
    }
    sudo cp "$WEBUI_SOURCE/api.php" "$WEB_DIR/" || {
        echo -e "${YELLOW}⚠️ Nie udało się skopiować api.php${NC}"
    }
    
    # Ustawienie właściwych uprawnień
    sudo chown www-data:www-data "$WEB_DIR"/*.html "$WEB_DIR"/*.js "$WEB_DIR"/*.php 2>/dev/null || true
    sudo chmod 644 "$WEB_DIR"/*.html "$WEB_DIR"/*.js "$WEB_DIR"/*.php 2>/dev/null || true
    
    echo -e "${GREEN}✅ Pliki WebUI skopiowane do $WEB_DIR${NC}"
else
    echo -e "${YELLOW}⚠️ Katalog WebUI nie znaleziony w $WEBUI_SOURCE${NC}"
    echo "   Pliki WebUI nie zostaną skopiowane"
fi

# Legacy endpoint: read-only, authenticated and bounded API.
sudo install -o root -g www-data -m 0640 "$WEBUI_SOURCE/legacy_api.php" "$WEB_DIR/index.php"
sudo chown www-data:www-data "$WEB_DIR"
sudo chmod 0750 "$WEB_DIR"



# Konfiguracja uprawnień
sudo chmod 644 /var/www/html/apiary/index.php

# Restart Apache2
echo "🔄 Restartowanie usługi Apache2..."
sudo systemctl restart apache2
sudo systemctl enable apache2

# 5. Instalacja usługi systemd dla kolektora
echo "⚙️ Instalowanie usługi systemd..."

# Użyj właściwej ścieżki do skompilowanego binarnego
COLLECTOR_BIN=$(which apiary-collector 2>/dev/null || echo "/usr/local/bin/apiary-collector")

sudo tee /etc/systemd/system/apiary-collector.service > /dev/null <<EOF
[Unit]
Description=Apiary Data Collector Service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=www-data
Group=www-data
ExecStart=${COLLECTOR_BIN}
Restart=on-failure
RestartSec=5
TimeoutStopSec=20
KillSignal=SIGTERM
UMask=0027
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true
RestrictSUIDSGID=true
LockPersonality=true
MemoryDenyWriteExecute=true
RestrictRealtime=true
CapabilityBoundingSet=
ReadWritePaths=/var/lib/apiaryguard /var/log/apiaryguard

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo apache2ctl configtest
sudo systemctl restart apache2
sudo systemctl enable apiary-collector

echo "✅ Instalacja zakończona pomyślnie!"
echo ""
echo "🌐 Dostęp do WebUI (interfejs graficzny):"
echo "   http://$(hostname -I | awk '{print $1}')/apiary/index.html"
echo ""
echo "📊 Dostęp do danych JSON:"
echo "   http://$(hostname -I | awk '{print $1}')/apiary/index.php?action=latest"
echo ""
echo "📱 API dla aplikacji Android i WebUI:"
echo "   - Najnowsze dane: ?action=latest"
echo "   - Historia minut: ?action=history_minute&limit=60"
echo "   - Historia godzin: ?action=history_hour&limit=168"
echo "   - Historia dni: ?action=history_day&limit=365"
echo "   - Historia tygodni: ?action=history_week&limit=52"
echo "   - Historia miesięcy: ?action=history_month&limit=60 (5 lat)"
echo "   - Historia kwartałów: ?action=history_quarter&limit=40 (10 lat)"
echo "   - Historia lat: ?action=history_year&limit=100"
echo "   - Dane surowe: ?action=raw&limit=1000"
echo "   - Statystyki DB: ?action=stats"
echo ""
echo "🔧 Aby uruchomić kolektor: sudo systemctl start apiary-collector"
echo "📋 Aby sprawdzić status: sudo systemctl status apiary-collector"
echo "🛑 Aby zatrzymać: sudo systemctl stop apiary-collector"
