#!/bin/bash

# Skrypt instalacyjny dla Apiary Collector na Raspberry Pi
# Instaluje zależności, konfiguruje bazę danych, Apache2 i usługi systemd
# UWAGA: Ten skrypt wykonuje PEŁNĄ instalację z WebUI i API

set -e

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

make clean 2>/dev/null || true
make all -j$(nproc) || {
    echo -e "${RED}❌ Kompilacja nieudana${NC}"
    exit 1
}

echo -e "${GREEN}✅ Kompilacja zakończona pomyślnie${NC}"
echo ""

# 3. Instalacja binarek w systemie
echo "⚙️ Instalowanie binarek w systemie..."
sudo make install || {
    echo -e "${YELLOW}⚠️ Instalacja binarek miała problemy, ale kompilacja succeeded${NC}"
}

echo -e "${GREEN}✅ Binarki zainstalowane${NC}"
echo ""

# 4. Konfiguracja bazy danych
echo "💾 Konfigurowanie bazy danych SQLite..."
DB_PATH="/var/lib/apiaryguard/apiary.db"
sudo mkdir -p /var/lib/apiaryguard
sudo touch $DB_PATH
sudo chown www-data:www-data $DB_PATH
sudo chmod 664 $DB_PATH

# Inicjalizacja schematu bazy danych z pełną obsługą agregacji
sudo -u www-data sqlite3 $DB_PATH <<EOF
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

# Zaawansowany plik index.php z obsługą wszystkich poziomów agregacji
sudo tee /var/www/html/apiary/index.php > /dev/null <<'PHP_EOF'
<?php
header('Content-Type: application/json');

$db = new SQLite3('/var/lib/apiaryguard/apiary.db');
if (!$db) {
    echo json_encode(['error' => 'Database connection failed']);
    exit;
}

$action = $_GET['action'] ?? 'latest';
$hive_id = $_GET['hive_id'] ?? '%';
$limit = intval($_GET['limit'] ?? 100);

switch ($action) {
    case 'latest':
        // Najnowsze dane zagregowane (minutowe)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 1 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT 1");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $result = $stmt->execute()->fetchArray(SQLITE3_ASSOC);
        echo json_encode($result ?: ['status' => 'no_data']);
        break;
        
    case 'history_minute':
        // Historia minutowa (ostatnie 60 minut)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 1 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'history_hour':
        // Historia godzinowa (ostatnie 168 godzin = tydzień)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 2 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'history_day':
        // Historia dzienna (ostatnie 365 dni)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 3 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'history_week':
        // Historia tygodniowa (ostatnie 52 tygodnie)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 4 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'history_month':
        // Historia miesięczna (ostatnie 60 miesięcy = 5 lat)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 5 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'history_quarter':
        // Historia kwartalna (ostatnie 40 kwartałów = 10 lat)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 6 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'history_year':
        // Historia roczna (ostatnie 100 lat)
        $stmt = $db->prepare("SELECT * FROM aggregated_data WHERE agg_type = 7 AND hive_id LIKE :hive ORDER BY timestamp_start DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'raw':
        // Dane surowe (ostatnie 7 dni)
        $stmt = $db->prepare("SELECT * FROM raw_data WHERE hive_id LIKE :hive AND timestamp > (strftime('%s', 'now') - 604800) ORDER BY timestamp DESC LIMIT :limit");
        $stmt->bindValue(':hive', $hive_id, SQLITE3_TEXT);
        $stmt->bindValue(':limit', $limit, SQLITE3_INTEGER);
        $results = [];
        $query = $stmt->execute();
        while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
            $results[] = $row;
        }
        echo json_encode($results);
        break;
        
    case 'stats':
        // Statystyki bazy danych
        $stats = [];
        $stats['raw_count'] = $db->querySingle("SELECT COUNT(*) FROM raw_data");
        $stats['minute_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 1");
        $stats['hour_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 2");
        $stats['day_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 3");
        $stats['week_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 4");
        $stats['month_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 5");
        $stats['quarter_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 6");
        $stats['year_count'] = $db->querySingle("SELECT COUNT(*) FROM aggregated_data WHERE agg_type = 7");
        $stats['oldest_raw'] = $db->querySingle("SELECT datetime(timestamp, 'unixepoch') FROM raw_data ORDER BY timestamp ASC LIMIT 1");
        $stats['newest_raw'] = $db->querySingle("SELECT datetime(timestamp, 'unixepoch') FROM raw_data ORDER BY timestamp DESC LIMIT 1");
        echo json_encode($stats);
        break;
        
    default:
        echo json_encode(['error' => 'Unknown action. Use: latest, history_minute, history_hour, history_day, history_week, history_month, history_quarter, history_year, raw, stats']);
        break;
}

$db->close();
?>
PHP_EOF

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
After=network.target

[Service]
Type=simple
User=root
ExecStart=${COLLECTOR_BIN}
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable apiary-collector

echo "✅ Instalacja zakończona pomyślnie!"
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
