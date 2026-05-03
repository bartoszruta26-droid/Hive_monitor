# ApiaryGuard HTTP Collector

## Opis

**apiary_http_collector.cpp** to program w C++ do **realnego zbierania danych** z Raspberry Pi Pico przez Ethernet, wykorzystujący **HTTP API** zdefiniowane w pliku `/src/pico/apiaryguard_pico.ino`.

Program łączy się z serwerem HTTP działającym na Raspberry Pi Pico (port 8080) i pobiera dane sensoryczne z endpointów API.

## Wymagania

### Biblioteki systemowe
```bash
# Debian/Ubuntu/Raspberry Pi OS
sudo apt-get update
sudo apt-get install -y libcurl4-openssl-dev nlohmann-json3-dev g++ make

# Fedora
sudo dnf install -y libcurl-devel nlohmann-json-devel gcc-c++ make

# Arch Linux
sudo pacman -S curl nlohmann-json gcc make
```

### Kompilacja
```bash
cd /workspace/src/rpi_tui

# Kompilacja
g++ -std=c++17 -pthread -o apiary_http_collector apiary_http_collector.cpp -lcurl

# Lub z Makefile (jeśli dostępny)
make apiary_http_collector
```

## Uruchomienie

### Tryb podstawowy - pojedyncze Pico
```bash
./apiary_http_collector --pico 192.168.1.177 --interval 5
```

### Tryb z plikiem konfiguracyjnym
```bash
./apiary_http_collector --config hives.conf
```

### Tryb jednorazowy (pojedyncze pobranie)
```bash
./apiary_http_collector --pico 192.168.1.177 --once --output json
```

### Pełna lista opcji
```bash
./apiary_http_collector --help
```

## Przykłady użycia

### 1. Podłączenie do jednego ula
```bash
./apiary_http_collector --pico 192.168.1.177 --id Ul-Matka --interval 5
```

### 2. Eksport danych do JSON
```bash
./apiary_http_collector --pico 192.168.1.177 --once --output json > dane_ula.json
```

### 3. Eksport danych do CSV
```bash
./apiary_http_collector --config hives.conf --once --output csv > pasieka.csv
```

### 4. Monitorowanie ciągłe z podsumowaniem
```bash
./apiary_http_collector --pico 192.168.1.177 --interval 10 --output summary
```

## Konfiguracja wielu uli

Utwórz plik `hives.conf`:
```
# Format: hive_id,ip_address,port,poll_interval_seconds
Ul-Matka,192.168.1.177,8080,5
Ul-1,192.168.1.178,8080,5
Ul-2,192.168.1.179,8080,5
Ul-3,192.168.1.180,8080,10
```

Uruchom:
```bash
./apiary_http_collector --config hives.conf
```

## Endpointy API Raspberry Pi Pico

Program odpytuje następujące endpointy na porcie 8080:

| Endpoint | Opis |
|----------|------|
| `/status` | Podstawowe dane: temperatura, wilgotność, waga, CO₂, VOC |
| `/hx711/status` | Status wagi - wartość surowa, trend, prognoza |
| `/hx711/metrics` | Szczegółowe metryki wagi (30+ parametrów) |
| `/hx711/events` | Zdarzenia związane z wagą |
| `/radar/status` | Status radaru MMWave - cele, dystans, zdrowie ula |
| `/radar/anomalies` | Wykryte anomalie ruchu pszczół |
| `/audio/status` | Analiza audio - RMS, częstotliwość, aktywność pszczół |
| `/audio/spectrum` | Widmo FFT sygnału audio |
| `/airquality/status` | Jakość powietrza - IAQ, wentylacja |
| `/airquality/metrics` | Szczegółowe metryki jakości powietrza |

## Struktura danych JSON

Przykładowy output JSON:
```json
{
  "Ul-Matka": {
    "temperature": 24.3,
    "humidity": 65.2,
    "weight": 45.67,
    "co2_eq": 450,
    "voc_idx": 85,
    "hx711_raw": 45670,
    "hx711_rate": 12.5,
    "honey_production_idx": 0.75,
    "colony_growth_rate": 0.08,
    "predicted_weight_24h": 46.2,
    "radar_target_count": 15,
    "radar_distance": 0.35,
    "hive_health_index": 0.89,
    "activity_ratio": 0.67,
    "last_anomaly_type": "NONE",
    "audio_rms": 0.025,
    "dominant_frequency": 250.5,
    "bee_activity_index": 0.72,
    "swarm_probability": 0.15,
    "iaq_index": 45.0,
    "ventilation_need": 0,
    "is_online": true,
    "last_update": 1704567890,
    "timestamp": 1704567890,
    "error_count": 0
  }
}
```

## Integracja z innymi systemami

### Wysyłanie danych do MQTT
```bash
#!/bin/bash
while true; do
    data=$(./apiary_http_collector --pico 192.168.1.177 --once --output json)
    echo "$data" | mosquitto_pub -t "apiary/ul-matka" -l
    sleep 60
done
```

### Zapis do bazy danych InfluxDB
```bash
#!/bin/bash
data=$(./apiary_http_collector --pico 192.168.1.177 --once --output json)

temp=$(echo "$data" | jq '.["Ul-Matka"].temperature')
hum=$(echo "$data" | jq '.["Ul-Matka"].humidity')
weight=$(echo "$data" | jq '.["Ul-Matka"].weight')

curl -i -XPOST 'http://localhost:8086/write?db=apiary' \
  --data-binary "hive_data temperature=$temp,humidity=$hum,weight=$weight"
```

### Cron job - zbieranie co 5 minut
```cron
*/5 * * * * /workspace/src/rpi_tui/apiary_http_collector --pico 192.168.1.177 --once --output json >> /var/log/apiary/data.json
```

## Rozwiązywanie problemów

### Błąd połączenia
```
[ERROR] [Ul-Matka] Brak odpowiedzi po 5 próbach
```
**Rozwiązanie:** Sprawdź czy Pico jest podłączone do sieci i czy serwer HTTP działa na porcie 8080.

```bash
ping 192.168.1.177
curl http://192.168.1.177:8080/status
```

### Błąd biblioteki CURL
```
error: 'CURL' was not declared in this scope
```
**Rozwiązanie:** Zainstaluj bibliotekę CURL:
```bash
sudo apt-get install libcurl4-openssl-dev
```

### Błąd parsowania JSON
```
error: 'nlohmann' has not been declared
```
**Rozwiązanie:** Zainstaluj bibliotekę nlohmann-json:
```bash
sudo apt-get install nlohmann-json3-dev
```

## Architektura

```
┌─────────────────┐      HTTP       ┌──────────────────┐
│  Raspberry Pi   │ ◄─────────────► │  PC / Serwer     │
│  Pico (W6100)   │    port 8080    │  apiary_http_    │
│                 │                 │  collector       │
│  - DHT22        │                 │                  │
│  - HX711        │                 │  - Wielowątkowe  │
│  - SGP41        │                 │    odpytywanie   │
│  - LD2410B      │                 │  - JSON/CSV      │
│  - MEMS Mic     │                 │  - Konfiguracja  │
└─────────────────┘                 └──────────────────┘
```

## Porównanie z apiary_collector.cpp

| Cecha | apiary_collector.cpp | apiary_http_collector.cpp |
|-------|---------------------|---------------------------|
| Protokół | UDP (nasłuchiwanie) | HTTP (odpytywanie) |
| Tryb pracy | Pasywny odbiór | Aktywne żądania |
| Wymagania | Brak zewnętrznych lib | libcurl, nlohmann-json |
| Dane | Podstawowe (5 pól) | Pełne (30+ parametrów) |
| Symulacja | Tak | Nie (realne API) |

## Licencja

MIT License - zobacz główny plik LICENSE w repozytorium.
