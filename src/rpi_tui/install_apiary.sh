#!/bin/bash

# Skrypt instalacyjny dla Apiary Collector na Raspberry Pi
# Instaluje zależności, konfiguruje bazę danych, Apache2 i usługi

set -e

echo "🚀 Rozpoczynanie instalacji Apiary Collector..."

# 1. Aktualizacja systemu i instalacja podstawowych zależności
echo "📦 Aktualizacja listy pakietów..."
sudo apt-get update
sudo apt-get install -y git build-essential cmake sqlite3 libsqlite3-dev libncurses5-dev libncursesw5-dev apache2 php libapache2-mod-php python3-pip

# 2. Kompilacja projektu
echo "🔨 Kompilowanie projektu..."
if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ..
make -j$(nproc)
cd ..

# 3. Konfiguracja bazy danych
echo "💾 Konfigurowanie bazy danych SQLite..."
DB_PATH="/var/lib/apiary/apiary.db"
sudo mkdir -p /var/lib/apiary
sudo touch $DB_PATH
sudo chown www-data:www-data $DB_PATH
sudo chmod 664 $DB_PATH

# Inicjalizacja schematu bazy danych (jeśli pusta)
sudo -u www-data sqlite3 $DB_PATH <<EOF
CREATE TABLE IF NOT EXISTS hive_data_raw (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    temperature REAL,
    humidity REAL,
    weight REAL,
    battery_voltage REAL,
    mac_address TEXT
);

CREATE TABLE IF NOT EXISTS hive_data_aggregated (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    avg_temperature REAL,
    avg_humidity REAL,
    avg_weight REAL,
    min_temperature REAL,
    max_temperature REAL,
    sample_count INTEGER
);

CREATE INDEX IF NOT EXISTS idx_raw_timestamp ON hive_data_raw(timestamp);
CREATE INDEX IF NOT EXISTS idx_agg_timestamp ON hive_data_aggregated(timestamp);
EOF

# 4. Konfiguracja Apache2 dla WebUI i API
echo "🌐 Konfigurowanie serwera Apache2..."

# Tworzenie katalogu dla aplikacji webowej
WEB_DIR="/var/www/html/apiary"
sudo mkdir -p $WEB_DIR
sudo chown www-data:www-data $WEB_DIR

# Przykładowy plik index.php działający jako API i prosty frontend
sudo tee /var/www/html/apiary/index.php > /dev/null <<'PHP_EOF'
<?php
header('Content-Type: application/json');
$db = new SQLite3('/var/lib/apiary/apiary.db');

$action = $_GET['action'] ?? 'latest';

if ($action === 'latest') {
    $result = $db->querySingle("SELECT * FROM hive_data_aggregated ORDER BY timestamp DESC LIMIT 1", true);
    echo json_encode($result ?: ['status' => 'no_data']);
} elseif ($action === 'history') {
    $limit = intval($_GET['limit'] ?? 100);
    $results = [];
    $query = $db->query("SELECT * FROM hive_data_aggregated ORDER BY timestamp DESC LIMIT $limit");
    while ($row = $query->fetchArray(SQLITE3_ASSOC)) {
        $results[] = $row;
    }
    echo json_encode($results);
} else {
    echo json_encode(['error' => 'Unknown action']);
}
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
sudo tee /etc/systemd/system/apiary-collector.service > /dev/null <<EOF
[Unit]
Description=Apiary Data Collector Service
After=network.target sqlite3.service

[Service]
Type=simple
User=root
WorkingDirectory=$(pwd)/build
ExecStart=$(pwd)/build/apiary_collector
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable apiary-collector

echo "✅ Instalacja zakończona pomyślnie!"
echo "📊 Dostęp do danych JSON: http://$(hostname -I | awk '{print $1}')/apiary/index.php?action=latest"
echo "📱 Aplikacja Android i WebUI mogą pobierać dane z powyższego adresu."
echo "🔧 Aby uruchomić kolektor: sudo systemctl start apiary-collector"
