#!/bin/bash
#
# apiary_tui.sh - Terminal UI dla systemu monitoringu uli (Raspberry Pi)
# Ten skrypt bash zapewnia interfejs TUI do debugowania i logowania
# Bez Pythona - czysty Bash z ncurses (przez tput)
#
# INTEGRACJA Z APIARY_COLLECTOR:
# - Skrypt komunikuje się z działającym procesem apiary_collector
# - Dane są pobierane przez pipe/FIFO lub bezpośrednio z outputu kolektora
# - Wszystkie 18 parametrów jest wyświetlanych w czasie rzeczywistym
#

set -e

# Konfiguracja
LOG_FILE="/var/log/apiaryguard/apiary.log"
DEBUG_FILE="/var/log/apiaryguard/debug.log"
CONFIG_DIR="/etc/apiaryguard"
DATA_DIR="/var/lib/apiaryguard"
LOCK_FILE="/tmp/apiary_tui.lock"
COLLECTOR_BIN="/workspace/src/rpi_tui/apiary_collector"
COLLECTOR_FIFO="/tmp/apiary_collector_fifo"
COLLECTOR_DATA_FILE="/tmp/apiary_data.csv"

# Kolory (ANSI escape codes)
COLOR_RESET="\033[0m"
COLOR_RED="\033[31m"
COLOR_GREEN="\033[32m"
COLOR_YELLOW="\033[33m"
COLOR_BLUE="\033[34m"
COLOR_MAGENTA="\033[35m"
COLOR_CYAN="\033[36m"
COLOR_WHITE="\033[37m"
COLOR_BOLD="\033[1m"

# Stan aplikacji
CURRENT_TAB=0
TAB_COUNT=4
SCROLL_OFFSET_LOG=0
SCROLL_OFFSET_DEBUG=0
AUTO_REFRESH=true
REFRESH_INTERVAL=2
LAST_REFRESH=0
COLLECTOR_PID=""

# Tablice do przechowywania danych - rozszerzone o wszystkie 18 parametrów
declare -a LOG_LINES
declare -a DEBUG_LINES
declare -a HIVES
declare -a HIVE_STATUS
declare -a HIVE_TEMP
declare -a HIVE_HUM
declare -a HIVE_WEIGHT
declare -a HIVE_BAT
declare -a HIVE_CO2
declare -a HIVE_VOC
declare -a HIVE_MOTION
declare -a HIVE_IAQ
# Audio parametry
declare -a HIVE_AUDIO_RMS
declare -a HIVE_AUDIO_FREQ
declare -a HIVE_SWARM_PROB
declare -a HIVE_BEE_ACTIVITY
declare -a HIVE_SPECTRAL_CENTROID
declare -a HIVE_AUDIO_HEALTH
# Radar parametry
declare -a HIVE_RADAR_DIST
declare -a HIVE_RADAR_ENERGY
declare -a HIVE_RADAR_ACT
declare -a HIVE_RADAR_HEALTH
declare -a HIVE_RADAR_ANOMALY
# HX711 parametry
declare -a HIVE_WAG_RATE
declare -a HIVE_WAG_TREND
declare -a HIVE_HX_MEAN
declare -a HIVE_NECTAR_INFLOW
declare -a HIVE_COLONY_GROWTH
declare -a HIVE_PRODUCTIVITY
# Temp/Humidity parametry
declare -a HIVE_HEAT_INDEX
declare -a HIVE_COMFORT_INDEX
declare -a HIVE_BROOD_STRESS
# Air Quality parametry
declare -a HIVE_IAQ_INDEX
declare -a HIVE_VENTILATION_NEED
# Piezo parametry
declare -a HIVE_PIEZO_ACTIVITY
declare -a HIVE_PREDATOR_SCORE
# Barometric parametry
declare -a HIVE_PRESSURE
declare -a HIVE_WEATHER_TREND
declare -a HIVE_FORAGING_COND
# Light parametry
declare -a HIVE_LUX
declare -a HIVE_CIRCADIAN_SYNC

# Inicjalizacja katalogów i plików
init_system() {
    mkdir -p "$CONFIG_DIR" "$DATA_DIR" "$(dirname $LOG_FILE)" "$(dirname $DEBUG_FILE)" 2>/dev/null || true
    touch "$LOG_FILE" "$DEBUG_FILE" 2>/dev/null || true
    
    # Inicjalizacja pustych tablic danych
    HIVES=()
    HIVE_STATUS=()
    HIVE_TEMP=()
    HIVE_HUM=()
    HIVE_WEIGHT=()
    HIVE_BAT=()
    HIVE_CO2=()
    HIVE_VOC=()
    HIVE_MOTION=()
    HIVE_IAQ=()
    HIVE_AUDIO_RMS=()
    HIVE_AUDIO_FREQ=()
    HIVE_SWARM_PROB=()
    HIVE_RADAR_DIST=()
    HIVE_RADAR_ENERGY=()
    HIVE_RADAR_ACT=()
    HIVE_WAG_RATE=()
    HIVE_WAG_TREND=()
}

# Start kolektora danych w tle
start_collector() {
    if [ ! -f "$COLLECTOR_BIN" ]; then
        echo -e "${COLOR_RED}Błąd: Nie znaleziono binarki apiary_collector: $COLLECTOR_BIN${COLOR_RESET}"
        echo "Skompiluj najpierw: cd /workspace/src/rpi_tui && make all"
        return 1
    fi
    
    # Sprawdź czy już działa
    if [ -n "$COLLECTOR_PID" ] && kill -0 "$COLLECTOR_PID" 2>/dev/null; then
        return 0
    fi
    
    # Uruchom kolektor w tle
    "$COLLECTOR_BIN" &
    COLLECTOR_PID=$!
    echo -e "${COLOR_GREEN}Uruchomiono apiary_collector (PID: $COLLECTOR_PID)${COLOR_RESET}"
    
    # Czekaj chwilę na inicjalizację
    sleep 1
}

# Stop kolektora
stop_collector() {
    if [ -n "$COLLECTOR_PID" ] && kill -0 "$COLLECTOR_PID" 2>/dev/null; then
        kill "$COLLECTOR_PID" 2>/dev/null || true
        wait "$COLLECTOR_PID" 2>/dev/null || true
        echo -e "${COLOR_CYAN}Zatrzymano apiary_collector${COLOR_RESET}"
    fi
    COLLECTOR_PID=""
}

# Pobierz dane z kolektora przez HTTP API
fetch_hive_data() {
    # Reset tablic - wszystkie parametry
    HIVES=()
    HIVE_STATUS=()
    HIVE_TEMP=()
    HIVE_HUM=()
    HIVE_WEIGHT=()
    HIVE_BAT=()
    HIVE_CO2=()
    HIVE_VOC=()
    HIVE_MOTION=()
    HIVE_IAQ=()
    HIVE_AUDIO_RMS=()
    HIVE_AUDIO_FREQ=()
    HIVE_SWARM_PROB=()
    HIVE_BEE_ACTIVITY=()
    HIVE_SPECTRAL_CENTROID=()
    HIVE_AUDIO_HEALTH=()
    HIVE_RADAR_DIST=()
    HIVE_RADAR_ENERGY=()
    HIVE_RADAR_ACT=()
    HIVE_RADAR_HEALTH=()
    HIVE_RADAR_ANOMALY=()
    HIVE_WAG_RATE=()
    HIVE_WAG_TREND=()
    HIVE_HX_MEAN=()
    HIVE_NECTAR_INFLOW=()
    HIVE_COLONY_GROWTH=()
    HIVE_PRODUCTIVITY=()
    HIVE_HEAT_INDEX=()
    HIVE_COMFORT_INDEX=()
    HIVE_BROOD_STRESS=()
    HIVE_IAQ_INDEX=()
    HIVE_VENTILATION_NEED=()
    HIVE_PIEZO_ACTIVITY=()
    HIVE_PREDATOR_SCORE=()
    HIVE_PRESSURE=()
    HIVE_WEATHER_TREND=()
    HIVE_FORAGING_COND=()
    HIVE_LUX=()
    HIVE_CIRCADIAN_SYNC=()
    
    local data_fetched=false
    
    # Spróbuj pobrać dane z kolektora przez HTTP API JSON
    if command -v curl &> /dev/null; then
        # Sprawdź czy kolektor jest dostępny
        if curl -s --connect-timeout 2 http://localhost:8080/health > /dev/null 2>&1; then
            # Pobierz dane JSON z kolektora
            local json_data
            json_data=$(curl -s --connect-timeout 2 http://localhost:8080/api/hives 2>/dev/null)
            
            if [ -n "$json_data" ] && [ "$json_data" != "{\"error\"* ]; then
                # Parsuj JSON za pomocą prostych narzędzi bash
                # Format: {"timestamp":..., "hive_count":N, "hives":{"UL-001":{...}, ...}}
                
                # Wyodrębnij listę hive_id z JSON
                local hive_ids
                hive_ids=$(echo "$json_data" | grep -oP '"\b(UL-\d+|hive_\d+)\b"(?=:)' | tr -d '":' | sort -u)
                
                if [ -n "$hive_ids" ]; then
                    while IFS= read -r hive_id; do
                        [ -z "$hive_id" ] && continue
                        
                        # Ekstrahuj parametry dla każdego ula z JSON
                        local hive_block
                        hive_block=$(echo "$json_data" | grep -oP "\"$hive_id\":\{[^}]+\}" | head -1)
                        
                        if [ -n "$hive_block" ]; then
                            # Ekstrakcja podstawowych parametrów
                            local temp hum weight bat co2 voc motion status
                            temp=$(echo "$hive_block" | grep -oP '"temp":\K[0-9.]+' || echo "0")
                            hum=$(echo "$hive_block" | grep -oP '"hum":\K[0-9.]+' || echo "0")
                            weight=$(echo "$hive_block" | grep -oP '"weight":\K[0-9.]+' || echo "0")
                            bat=$(echo "$hive_block" | grep -oP '"bat":\K[0-9]+' || echo "0")
                            co2=$(echo "$hive_block" | grep -oP '"co2":\K[0-9]+' || echo "0")
                            voc=$(echo "$hive_block" | grep -oP '"voc":\K[0-9]+' || echo "0")
                            motion=$(echo "$hive_block" | grep -oP '"motion":\K[0-1]+' || echo "0")
                            status="ONLINE"
                            
                            # Ekstrakcja parametrów audio
                            local audio_block
                            audio_block=$(echo "$hive_block" | grep -oP '"audio":\{[^}]+\}' || echo "")
                            local audio_rms audio_freq swarm_prob bee_activity spectral_centroid audio_health
                            audio_rms=$(echo "$audio_block" | grep -oP '"rms":\K[0-9.]+' || echo "0")
                            audio_freq=$(echo "$audio_block" | grep -oP '"freq":\K[0-9.]+' || echo "0")
                            swarm_prob=$(echo "$audio_block" | grep -oP '"swarm_prob":\K[0-9.]+' || echo "0")
                            bee_activity=$(echo "$audio_block" | grep -oP '"bee_activity":\K[0-9.]+' || echo "0")
                            spectral_centroid=$(echo "$audio_block" | grep -oP '"spectral_centroid":\K[0-9.]+' || echo "0")
                            audio_health=$(echo "$audio_block" | grep -oP '"hive_health":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów radar
                            local radar_block
                            radar_block=$(echo "$hive_block" | grep -oP '"radar":\{[^}]+\}' || echo "")
                            local radar_dist radar_energy radar_act radar_health radar_anomaly
                            radar_dist=$(echo "$radar_block" | grep -oP '"dist":\K[0-9.]+' || echo "0")
                            radar_energy=$(echo "$radar_block" | grep -oP '"energy":\K[0-9.]+' || echo "0")
                            radar_act=$(echo "$radar_block" | grep -oP '"activity":\K[0-9.]+' || echo "0")
                            radar_health=$(echo "$radar_block" | grep -oP '"hive_health":\K[0-9.]+' || echo "0")
                            radar_anomaly=$(echo "$radar_block" | grep -oP '"anomaly_score":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów hx711/waga
                            local hx_block
                            hx_block=$(echo "$hive_block" | grep -oP '"hx711":\{[^}]+\}' || echo "")
                            local hx_mean wag_rate wag_trend nectar_inflow colony_growth productivity
                            hx_mean=$(echo "$hx_block" | grep -oP '"mean":\K[0-9.]+' || echo "$weight")
                            wag_rate=$(echo "$hx_block" | grep -oP '"slope_1h":\K[-0-9.]+' || echo "0")
                            wag_trend=$(echo "$hx_block" | grep -oP '"slope_4h":\K[-0-9.]+' || echo "0")
                            nectar_inflow=$(echo "$hx_block" | grep -oP '"nectar_inflow":\K[0-9.]+' || echo "0")
                            colony_growth=$(echo "$hx_block" | grep -oP '"colony_growth":\K[-0-9.]+' || echo "0")
                            productivity=$(echo "$hx_block" | grep -oP '"productivity":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów th (temp/humidity)
                            local th_block
                            th_block=$(echo "$hive_block" | grep -oP '"th":\{[^}]+\}' || echo "")
                            local heat_index comfort_idx brood_stress
                            heat_index=$(echo "$th_block" | grep -oP '"heat_index":\K[0-9.]+' || echo "0")
                            comfort_idx=$(echo "$th_block" | grep -oP '"comfort_index":\K[0-9.]+' || echo "0")
                            brood_stress=$(echo "$th_block" | grep -oP '"brood_stress":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów aq (air quality)
                            local aq_block
                            aq_block=$(echo "$hive_block" | grep -oP '"aq":\{[^}]+\}' || echo "")
                            local iaq_index ventilation_need
                            iaq_index=$(echo "$aq_block" | grep -oP '"iaq_index":\K[0-9.]+' || echo "0")
                            ventilation_need=$(echo "$aq_block" | grep -oP '"ventilation_need":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów piezo
                            local piezo_block
                            piezo_block=$(echo "$hive_block" | grep -oP '"piezo":\{[^}]+\}' || echo "")
                            local piezo_activity predator_score
                            piezo_activity=$(echo "$piezo_block" | grep -oP '"activity":\K[0-9.]+' || echo "0")
                            predator_score=$(echo "$piezo_block" | grep -oP '"predator_score":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów baro
                            local baro_block
                            baro_block=$(echo "$hive_block" | grep -oP '"baro":\{[^}]+\}' || echo "")
                            local pressure weather_trend foraging_cond
                            pressure=$(echo "$baro_block" | grep -oP '"pressure":\K[0-9.]+' || echo "0")
                            weather_trend=$(echo "$baro_block" | grep -oP '"weather_trend":\K[-0-9.]+' || echo "0")
                            foraging_cond=$(echo "$baro_block" | grep -oP '"foraging_cond":\K[0-9.]+' || echo "0")
                            
                            # Ekstrakcja parametrów light
                            local light_block
                            light_block=$(echo "$hive_block" | grep -oP '"light":\{[^}]+\}' || echo "")
                            local lux circadian_sync
                            lux=$(echo "$light_block" | grep -oP '"lux":\K[0-9.]+' || echo "0")
                            circadian_sync=$(echo "$light_block" | grep -oP '"circadian_sync":\K[0-9.]+' || echo "0")
                            
                            # Dodaj dane do tablic
                            HIVES+=("$hive_id")
                            HIVE_STATUS+=("$status")
                            HIVE_TEMP+=("$temp")
                            HIVE_HUM+=("$hum")
                            HIVE_WEIGHT+=("$weight")
                            HIVE_BAT+=("$bat")
                            HIVE_CO2+=("$co2")
                            HIVE_VOC+=("$voc")
                            HIVE_MOTION+=("$motion")
                            HIVE_IAQ+=("$iaq_index")
                            HIVE_AUDIO_RMS+=("$audio_rms")
                            HIVE_AUDIO_FREQ+=("$audio_freq")
                            HIVE_SWARM_PROB+=("$swarm_prob")
                            HIVE_BEE_ACTIVITY+=("$bee_activity")
                            HIVE_SPECTRAL_CENTROID+=("$spectral_centroid")
                            HIVE_AUDIO_HEALTH+=("$audio_health")
                            HIVE_RADAR_DIST+=("$radar_dist")
                            HIVE_RADAR_ENERGY+=("$radar_energy")
                            HIVE_RADAR_ACT+=("$radar_act")
                            HIVE_RADAR_HEALTH+=("$radar_health")
                            HIVE_RADAR_ANOMALY+=("$radar_anomaly")
                            HIVE_WAG_RATE+=("$wag_rate")
                            HIVE_WAG_TREND+=("$wag_trend")
                            HIVE_HX_MEAN+=("$hx_mean")
                            HIVE_NECTAR_INFLOW+=("$nectar_inflow")
                            HIVE_COLONY_GROWTH+=("$colony_growth")
                            HIVE_PRODUCTIVITY+=("$productivity")
                            HIVE_HEAT_INDEX+=("$heat_index")
                            HIVE_COMFORT_INDEX+=("$comfort_idx")
                            HIVE_BROOD_STRESS+=("$brood_stress")
                            HIVE_IAQ_INDEX+=("$iaq_index")
                            HIVE_VENTILATION_NEED+=("$ventilation_need")
                            HIVE_PIEZO_ACTIVITY+=("$piezo_activity")
                            HIVE_PREDATOR_SCORE+=("$predator_score")
                            HIVE_PRESSURE+=("$pressure")
                            HIVE_WEATHER_TREND+=("$weather_trend")
                            HIVE_FORAGING_COND+=("$foraging_cond")
                            HIVE_LUX+=("$lux")
                            HIVE_CIRCADIAN_SYNC+=("$circadian_sync")
                            
                            data_fetched=true
                        fi
                    done <<< "$hive_ids"
                    
                    if $data_fetched; then
                        echo "$(date '+%Y-%m-%d %H:%M:%S') [INFO] TUI: Pobrano dane z ${#HIVES[@]} uli przez HTTP API JSON" >> "$LOG_FILE" 2>/dev/null || true
                    fi
                fi
            fi
        fi
    fi
    
    # Jeśli brak danych z kolektora, sprawdź plik CSV
    if ! $data_fetched && [ -f "$COLLECTOR_DATA_FILE" ]; then
        local csv_data
        csv_data=$(cat "$COLLECTOR_DATA_FILE" 2>/dev/null)
        
        if [ -n "$csv_data" ]; then
            local first_line=true
            while IFS=',' read -r id status temp hum weight bat co2 voc motion audio_rms audio_freq swarm_prob radar_dist radar_energy radar_act wag_rate wag_trend air_iaq timestamp; do
                if $first_line; then
                    first_line=false
                    continue
                fi
                
                [ -z "$id" ] && continue
                
                HIVES+=("$id")
                HIVE_STATUS+=("$status")
                HIVE_TEMP+=("$temp")
                HIVE_HUM+=("$hum")
                HIVE_WEIGHT+=("$weight")
                HIVE_BAT+=("$bat")
                HIVE_CO2+=("$co2")
                HIVE_VOC+=("$voc")
                HIVE_MOTION+=("$motion")
                HIVE_IAQ+=("$air_iaq")
                HIVE_AUDIO_RMS+=("$audio_rms")
                HIVE_AUDIO_FREQ+=("$audio_freq")
                HIVE_SWARM_PROB+=("$swarm_prob")
                HIVE_RADAR_DIST+=("$radar_dist")
                HIVE_RADAR_ENERGY+=("$radar_energy")
                HIVE_RADAR_ACT+=("$radar_act")
                HIVE_WAG_RATE+=("$wag_rate")
                HIVE_WAG_TREND+=("$wag_trend")
                # Parametry domyślne dla rozszerzonych
                HIVE_BEE_ACTIVITY+=("0")
                HIVE_SPECTRAL_CENTROID+=("0")
                HIVE_AUDIO_HEALTH+=("0")
                HIVE_RADAR_HEALTH+=("0")
                HIVE_RADAR_ANOMALY+=("0")
                HIVE_HX_MEAN+=("$weight")
                HIVE_NECTAR_INFLOW+=("0")
                HIVE_COLONY_GROWTH+=("0")
                HIVE_PRODUCTIVITY+=("0")
                HIVE_HEAT_INDEX+=("0")
                HIVE_COMFORT_INDEX+=("0")
                HIVE_BROOD_STRESS+=("0")
                HIVE_IAQ_INDEX+=("$air_iaq")
                HIVE_VENTILATION_NEED+=("0")
                HIVE_PIEZO_ACTIVITY+=("0")
                HIVE_PREDATOR_SCORE+=("0")
                HIVE_PRESSURE+=("0")
                HIVE_WEATHER_TREND+=("0")
                HIVE_FORAGING_COND+=("0")
                HIVE_LUX+=("0")
                HIVE_CIRCADIAN_SYNC+=("0")
                
                data_fetched=true
            done <<< "$csv_data"
        fi
    fi
    
    # Jeśli nadal brak danych, użyj przykładowych (fallback/demo mode)
    if ! $data_fetched; then
        # Symulacja danych - tryb demo gdy kolektor nie działa
        HIVES=("UL-001" "UL-002" "UL-003")
        HIVE_STATUS=("ONLINE" "ONLINE" "STALE")
        HIVE_TEMP=("24.5" "26.1" "23.8")
        HIVE_HUM=("55.2" "58.7" "52.1")
        HIVE_WEIGHT=("45.300" "48.750" "42.100")
        HIVE_BAT=("98" "95" "87")
        HIVE_CO2=("450" "480" "520")
        HIVE_VOC=("35" "42" "38")
        HIVE_MOTION=("1" "0" "1")
        HIVE_IAQ=("75" "72" "68")
        HIVE_AUDIO_RMS=("0.025" "0.031" "0.018")
        HIVE_AUDIO_FREQ=("250" "280" "220")
        HIVE_SWARM_PROB=("0.15" "0.22" "0.08")
        HIVE_RADAR_DIST=("1.2" "1.5" "0.9")
        HIVE_RADAR_ENERGY=("45.3" "48.7" "42.1")
        HIVE_RADAR_ACT=("0.35" "0.42" "0.28")
        HIVE_WAG_RATE=("-0.02" "0.05" "-0.01")
        HIVE_WAG_TREND=("1.0" "0.8" "0.5")
        # Rozszerzone parametry - wartości domyślne
        for i in "${!HIVES[@]}"; do
            HIVE_BEE_ACTIVITY+=("0")
            HIVE_SPECTRAL_CENTROID+=("0")
            HIVE_AUDIO_HEALTH+=("0")
            HIVE_RADAR_HEALTH+=("0")
            HIVE_RADAR_ANOMALY+=("0")
            HIVE_HX_MEAN+=("${HIVE_WEIGHT[$i]}")
            HIVE_NECTAR_INFLOW+=("0")
            HIVE_COLONY_GROWTH+=("0")
            HIVE_PRODUCTIVITY+=("0")
            HIVE_HEAT_INDEX+=("0")
            HIVE_COMFORT_INDEX+=("0")
            HIVE_BROOD_STRESS+=("0")
            HIVE_IAQ_INDEX+=("${HIVE_IAQ[$i]}")
            HIVE_VENTILATION_NEED+=("0")
            HIVE_PIEZO_ACTIVITY+=("0")
            HIVE_PREDATOR_SCORE+=("0")
            HIVE_PRESSURE+=("0")
            HIVE_WEATHER_TREND+=("0")
            HIVE_FORAGING_COND+=("0")
            HIVE_LUX+=("0")
            HIVE_CIRCADIAN_SYNC+=("0")
        done
        
        # Dodaj informację o trybie demo do logów
        if [ "$AUTO_REFRESH" = true ]; then
            echo -e "${COLOR_YELLOW}[TUI] Tryb demo - brak połączenia z apiary_collector${COLOR_RESET}" >&2
        fi
    fi
}

# Czyszczenie ekranu
clear_screen() {
    tput clear
    tput home
}

# Ukrycie kursora
hide_cursor() {
    tput civis
}

# Pokazanie kursora
show_cursor() {
    tput cnorm
}

# Pobranie rozmiarów terminala
get_terminal_size() {
    TERM_WIDTH=$(tput cols)
    TERM_HEIGHT=$(tput lines)
}

# Rysowanie ramki
draw_box() {
    local x=$1 y=$2 w=$3 h=$4 title="$5"
    
    # Górna linia
    tput cup $y $x
    echo -ne "╔"
    for ((i=1; i<w-1; i++)); do echo -ne "═"; done
    echo -ne "╗"
    
    # Boczne linie
    for ((i=1; i<h-1; i++)); do
        tput cup $((y+i)) $x
        echo -ne "║"
        tput cup $((y+i)) $((x+w-1))
        echo -ne "║"
    done
    
    # Dolna linia
    tput cup $((y+h-1)) $x
    echo -ne "╚"
    for ((i=1; i<w-1; i++)); do echo -ne "═"; done
    echo -ne "╝"
    
    # Tytuł
    if [ -n "$title" ]; then
        tput cup $y $((x + (w - ${#title}) / 2 - 1))
        echo -ne " $title "
    fi
}

# Rysowanie paska tytułowego
draw_header() {
    get_terminal_size
    
    tput cup 0 0
    echo -ne "${COLOR_BOLD}${COLOR_BLUE}"
    printf '━%.0s' $(seq 1 $TERM_WIDTH)
    echo -ne "${COLOR_RESET}"
    
    local title="🐝 APIARY GUARD - System Monitoringu Uli"
    local title_pos=$(( (TERM_WIDTH - ${#title}) / 2 ))
    tput cup 0 $title_pos
    echo -ne "${COLOR_BOLD}${COLOR_WHITE}$title${COLOR_RESET}"
    
    local version="v1.0.0"
    tput cup 0 $((TERM_WIDTH - ${#version} - 2))
    echo -ne "${COLOR_CYAN}$version${COLOR_RESET}"
}

# Rysowanie paska zakładek
draw_tabs() {
    get_terminal_size
    local y=2
    local x=1
    local tabs=("LOGI" "DEBUG" "ULIE" "USTAWIENIA")
    
    for i in "${!tabs[@]}"; do
        local tab="${tabs[$i]}"
        local len=${#tab}
        
        if [ $i -eq $CURRENT_TAB ]; then
            echo -ne "${COLOR_BOLD}${COLOR_GREEN}"
            tput cup $y $x
            echo -ne "┌${tab}"
            for ((j=0; j<12-len; j++)); do echo -ne "─"; done
            echo -ne "┘"
            echo -ne "${COLOR_RESET}"
        else
            echo -ne "${COLOR_CYAN}"
            tput cup $y $x
            echo -ne " $tab "
            echo -ne "${COLOR_RESET}"
        fi
        x=$((x + 15))
    done
    
    # Linia oddzielająca
    tput cup $((y+1)) 0
    echo -ne "${COLOR_BLUE}"
    printf '─%.0s' $(seq 1 $TERM_WIDTH)
    echo -ne "${COLOR_RESET}"
}

# Ładowanie logów
load_logs() {
    LOG_LINES=()
    if [ -f "$LOG_FILE" ]; then
        while IFS= read -r line; do
            LOG_LINES+=("$line")
        done < <(tail -n 100 "$LOG_FILE" 2>/dev/null)
    fi
    
    # Dodaj przykładowe logi jeśli puste
    if [ ${#LOG_LINES[@]} -eq 0 ]; then
        LOG_LINES+=("$(date '+%Y-%m-%d %H:%M:%S') [INFO] System uruchomiony")
        LOG_LINES+=("$(date '+%Y-%m-%d %H:%M:%S') [INFO] Połączono z UL-001")
        LOG_LINES+=("$(date '+%Y-%m-%d %H:%M:%S') [WARN] Wysoka temperatura w UL-002: 38°C")
        LOG_LINES+=("$(date '+%Y-%m-%d %H:%M:%S') [INFO] Zapisano dane pomiarowe")
        LOG_LINES+=("$(date '+%Y-%m-%d %H:%M:%S') [ERROR] Utracono połączenie z UL-003")
    fi
}

# Ładowanie debugów
load_debug() {
    DEBUG_LINES=()
    if [ -f "$DEBUG_FILE" ]; then
        while IFS= read -r line; do
            DEBUG_LINES+=("$line")
        done < <(tail -n 100 "$DEBUG_FILE" 2>/dev/null)
    fi
    
    # Dodaj przykładowe debugi jeśli puste
    if [ ${#DEBUG_LINES[@]} -eq 0 ]; then
        DEBUG_LINES+=("[DEBUG] Inicjalizacja modułu sieciowego")
        DEBUG_LINES+=("[DEBUG] ETH0: IP 192.168.1.100, MASK 255.255.255.0")
        DEBUG_LINES+=("[DEBUG] MQTT: Próba połączenia z brokerem...")
        DEBUG_LINES+=("[DEBUG] MQTT: Połączono successfully")
        DEBUG_LINES+=("[DEBUG] Thread sensor_001 started")
        DEBUG_LINES+=("[DEBUG] Memory usage: 45MB / 512MB")
        DEBUG_LINES+=("[DEBUG] CPU load: 12%")
    fi
}

# Rysowanie panelu logów
draw_log_panel() {
    get_terminal_size
    load_logs
    
    local start_y=4
    local start_x=1
    local width=$((TERM_WIDTH - 2))
    local height=$((TERM_HEIGHT - start_y - 4))
    
    draw_box $start_x $start_y $width $height "Dziennik Zdarzeń (LOG)"
    
    local content_start=$((start_y + 1))
    local content_height=$((height - 2))
    
    # Oblicz maksymalny offset przewijania
    local max_offset=$((${#LOG_LINES[@]} - content_height))
    if [ $max_offset -lt 0 ]; then max_offset=0; fi
    if [ $SCROLL_OFFSET_LOG -gt $max_offset ]; then SCROLL_OFFSET_LOG=$max_offset; fi
    
    # Wyświetl logi
    for ((i=0; i<content_height && i+SCROLL_OFFSET_LOG<${#LOG_LINES[@]}; i++)); do
        local line_idx=$((i + SCROLL_OFFSET_LOG))
        local line="${LOG_LINES[$line_idx]}"
        
        # Kolorowanie po poziomie logu
        local color="$COLOR_WHITE"
        if [[ "$line" == *"[ERROR]"* ]]; then
            color="$COLOR_RED"
        elif [[ "$line" == *"[WARN]"* ]]; then
            color="$COLOR_YELLOW"
        elif [[ "$line" == *"[INFO]"* ]]; then
            color="$COLOR_GREEN"
        fi
        
        tput cup $((content_start + i)) $((start_x + 2))
        echo -ne "${color}${line:0:$((width-4))}${COLOR_RESET}"
    done
    
    # Informacja o przewijaniu
    if [ $max_offset -gt 0 ]; then
        tput cup $((start_y + height - 1)) $((start_x + width - 20))
        echo -ne "${COLOR_CYAN}Scroll: ${SCROLL_OFFSET_LOG}/${max_offset}${COLOR_RESET}"
    fi
}

# Rysowanie panelu debug
draw_debug_panel() {
    get_terminal_size
    load_debug
    
    local start_y=4
    local start_x=1
    local width=$((TERM_WIDTH - 2))
    local height=$((TERM_HEIGHT - start_y - 4))
    
    draw_box $start_x $start_y $width $height "Panel Debugowania"
    
    local content_start=$((start_y + 1))
    local content_height=$((height - 2))
    
    local max_offset=$((${#DEBUG_LINES[@]} - content_height))
    if [ $max_offset -lt 0 ]; then max_offset=0; fi
    if [ $SCROLL_OFFSET_DEBUG -gt $max_offset ]; then SCROLL_OFFSET_DEBUG=$max_offset; fi
    
    for ((i=0; i<content_height && i+SCROLL_OFFSET_DEBUG<${#DEBUG_LINES[@]}; i++)); do
        local line_idx=$((i + SCROLL_OFFSET_DEBUG))
        local line="${DEBUG_LINES[$line_idx]}"
        
        tput cup $((content_start + i)) $((start_x + 2))
        echo -ne "${COLOR_MAGENTA}${line:0:$((width-4))}${COLOR_RESET}"
    done
    
    if [ $max_offset -gt 0 ]; then
        tput cup $((start_y + height - 1)) $((start_x + width - 20))
        echo -ne "${COLOR_CYAN}Scroll: ${SCROLL_OFFSET_DEBUG}/${max_offset}${COLOR_RESET}"
    fi
}

# Rysowanie panelu uli - rozszerzone o wszystkie parametry
draw_hive_panel() {
    get_terminal_size
    
    local start_y=4
    local start_x=1
    local width=$((TERM_WIDTH - 2))
    local height=$((TERM_HEIGHT - start_y - 4))
    
    draw_box $start_x $start_y $width $height "Status Uli - Wszystkie Parametry"
    
    local content_start=$((start_y + 2))
    local col_width=$((width / 10))
    
    # Nagłówki - dwie linie dla wszystkich parametrów
    tput cup $((start_y + 1)) $((start_x + 1))
    echo -ne "${COLOR_BOLD}ID${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width))
    echo -ne "${COLOR_BOLD}STATUS${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*2))
    echo -ne "${COLOR_BOLD}TEMP°C${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*3))
    echo -ne "${COLOR_BOLD}HUM%${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*4))
    echo -ne "${COLOR_BOLD}WAGkg${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*5))
    echo -ne "${COLOR_BOLD}BAT%${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*6))
    echo -ne "${COLOR_BOLD}CO2${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*7))
    echo -ne "${COLOR_BOLD}VOC${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*8))
    echo -ne "${COLOR_BOLD}MOT${COLOR_RESET}"
    tput cup $((start_y + 1)) $((start_x + 1 + col_width*9))
    echo -ne "${COLOR_BOLD}IAQ${COLOR_RESET}"
    
    # Druga linia nagłówków - parametry zaawansowane
    tput cup $((content_start - 1)) $((start_x + 1))
    echo -ne "${COLOR_CYAN}Audio RMS${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*1))
    echo -ne "${COLOR_CYAN}Freq Hz${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*2))
    echo -ne "${COLOR_CYAN}Swarm%${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*3))
    echo -ne "${COLOR_CYAN}RadarD${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*4))
    echo -ne "${COLOR_CYAN}RadarE${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*5))
    echo -ne "${COLOR_CYAN}RadarA${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*6))
    echo -ne "${COLOR_CYAN}WagR${COLOR_RESET}"
    tput cup $((content_start - 1)) $((start_x + 1 + col_width*7))
    echo -ne "${COLOR_CYAN}WagT${COLOR_RESET}"
    
    # Pobierz aktualne dane z kolektora
    fetch_hive_data
    
    # Dane uli - użyj danych z tablic (z kolektora lub fallback)
    for i in "${!HIVES[@]}"; do
        local hive="${HIVES[$i]}"
        local status="${HIVE_STATUS[$i]:-ONLINE}"
        local temp="${HIVE_TEMP[$i]:-25.0}"
        local humidity="${HIVE_HUM[$i]:-50.0}"
        local weight="${HIVE_WEIGHT[$i]:-40.0}"
        local battery="${HIVE_BAT[$i]:-90}"
        local co2="${HIVE_CO2[$i]:-450}"
        local voc="${HIVE_VOC[$i]:-30}"
        local motion="${HIVE_MOTION[$i]:-0}"
        local iaq="${HIVE_IAQ[$i]:-70}"
        
        # Parametry audio
        local audio_rms="${HIVE_AUDIO_RMS[$i]:-0.020}"
        local audio_freq="${HIVE_AUDIO_FREQ[$i]:-250}"
        local swarm_prob="${HIVE_SWARM_PROB[$i]:-0.10}"
        
        # Parametry radaru
        local radar_dist="${HIVE_RADAR_DIST[$i]:-1.0}"
        local radar_energy="${HIVE_RADAR_ENERGY[$i]:-45.0}"
        local radar_act="${HIVE_RADAR_ACT[$i]:-0.30}"
        
        # Parametry wagi
        local wag_rate="${HIVE_WAG_RATE[$i]:-0.00}"
        local wag_trend="${HIVE_WAG_TREND[$i]:-0.5}"
        
        local status_color="$COLOR_GREEN"
        [ "$status" == "OFFLINE" ] && status_color="$COLOR_RED"
        [ "$status" == "STALE" ] && status_color="$COLOR_YELLOW"
        [ "$status" == "WARNING" ] && status_color="$COLOR_YELLOW"
        
        local row=$((content_start + i * 2))
        tput cup $row $((start_x + 1))
        echo -ne "${COLOR_CYAN}$hive${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width))
        echo -ne "${status_color}$status${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*2))
        echo -ne "${COLOR_WHITE}${temp}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*3))
        echo -ne "${COLOR_WHITE}${humidity}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*4))
        echo -ne "${COLOR_WHITE}${weight}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*5))
        echo -ne "${COLOR_WHITE}${battery}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*6))
        echo -ne "${COLOR_WHITE}${co2}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*7))
        echo -ne "${COLOR_WHITE}${voc}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*8))
        echo -ne "${COLOR_WHITE}${motion}${COLOR_RESET}"
        tput cup $row $((start_x + 1 + col_width*9))
        echo -ne "${COLOR_WHITE}${iaq}${COLOR_RESET}"
        
        # Druga linia danych - parametry zaawansowane
        local row2=$((row + 1))
        tput cup $row2 $((start_x + 1))
        echo -ne "${COLOR_MAGENTA}$audio_rms${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width))
        echo -ne "${COLOR_MAGENTA}$audio_freq${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width*2))
        echo -ne "${COLOR_MAGENTA}$swarm_prob${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width*3))
        echo -ne "${COLOR_MAGENTA}$radar_dist${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width*4))
        echo -ne "${COLOR_MAGENTA}$radar_energy${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width*5))
        echo -ne "${COLOR_MAGENTA}$radar_act${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width*6))
        echo -ne "${COLOR_MAGENTA}$wag_rate${COLOR_RESET}"
        tput cup $row2 $((start_x + 1 + col_width*7))
        echo -ne "${COLOR_MAGENTA}$wag_trend${COLOR_RESET}"
    done
}

# Rysowanie panelu ustawień
draw_settings_panel() {
    get_terminal_size
    
    local start_y=4
    local start_x=1
    local width=$((TERM_WIDTH - 2))
    local height=$((TERM_HEIGHT - start_y - 4))
    
    draw_box $start_x $start_y $width $height "Ustawienia Systemu"
    
    local content_start=$((start_y + 2))
    
    tput cup $content_start $((start_x + 4))
    echo -ne "${COLOR_YELLOW}[${AUTO_REFRESH:+✓}${AUTO_REFRESH:- }] Auto-odświeżanie${COLOR_RESET}"
    
    tput cup $((content_start + 1)) $((start_x + 4))
    echo -ne "${COLOR_YELLOW}Interwał: ${REFRESH_INTERVAL}s${COLOR_RESET}"
    
    tput cup $((content_start + 3)) $((start_x + 4))
    echo -ne "${COLOR_CYAN}Plik logów: ${LOG_FILE}${COLOR_RESET}"
    
    tput cup $((content_start + 4)) $((start_x + 4))
    echo -ne "${COLOR_CYAN}Plik debug: ${DEBUG_FILE}${COLOR_RESET}"
    
    tput cup $((content_start + 6)) $((start_x + 4))
    echo -ne "${COLOR_WHITE}Klawisze:${COLOR_RESET}"
    tput cup $((content_start + 7)) $((start_x + 6))
    echo -ne "←/→ : Nawigacja zakładek"
    tput cup $((content_start + 8)) $((start_x + 6))
    echo -ne "↑/↓ : Przewijanie"
    tput cup $((content_start + 9)) $((start_x + 6))
    echo -ne "r   : Odśwież"
    tput cup $((content_start + 10)) $((start_x + 6))
    echo -ne "a   : Toggle auto-refresh"
    tput cup $((content_start + 11)) $((start_x + 6))
    echo -ne "q   : Wyjście"
}

# Rysowanie paska stopki
draw_footer() {
    get_terminal_size
    
    tput cup $((TERM_HEIGHT - 1)) 0
    echo -ne "${COLOR_BLUE}"
    printf '━%.0s' $(seq 1 $TERM_WIDTH)
    echo -ne "${COLOR_RESET}"
    
    local help="q-Wyjście | ←/→-Tab | ↑/↓-Scroll | r-Odśwież | a-AutoRefresh"
    local help_pos=$(( (TERM_WIDTH - ${#help}) / 2 ))
    tput cup $((TERM_HEIGHT - 1)) $help_pos
    echo -ne "${COLOR_WHITE}$help${COLOR_RESET}"
}

# Główna funkcja rysująca
draw_ui() {
    clear_screen
    hide_cursor
    draw_header
    draw_tabs
    
    case $CURRENT_TAB in
        0) draw_log_panel ;;
        1) draw_debug_panel ;;
        2) draw_hive_panel ;;
        3) draw_settings_panel ;;
    esac
    
    draw_footer
    show_cursor
}

# Obsługa klawiszy
handle_input() {
    # Sprawdź czy dostępne są dane z stdin
    if read -t 0.1 -n 1 key; then
        case "$key" in
            $'\x1b')  # Escape sequence
                read -t 0.1 -n 1 key2
                read -t 0.1 -n 1 key3
                case "$key3" in
                    'A') # Strzałka w górę
                        if [ $CURRENT_TAB -eq 0 ]; then
                            [ $SCROLL_OFFSET_LOG -gt 0 ] && ((SCROLL_OFFSET_LOG--))
                        elif [ $CURRENT_TAB -eq 1 ]; then
                            [ $SCROLL_OFFSET_DEBUG -gt 0 ] && ((SCROLL_OFFSET_DEBUG--))
                        fi
                        ;;
                    'B') # Strzałka w dół
                        if [ $CURRENT_TAB -eq 0 ]; then
                            ((SCROLL_OFFSET_LOG++))
                        elif [ $CURRENT_TAB -eq 1 ]; then
                            ((SCROLL_OFFSET_DEBUG++))
                        fi
                        ;;
                    'C') # Strzałka w prawo
                        ((CURRENT_TAB = (CURRENT_TAB + 1) % TAB_COUNT))
                        SCROLL_OFFSET_LOG=0
                        SCROLL_OFFSET_DEBUG=0
                        ;;
                    'D') # Strzałka w lewo
                        ((CURRENT_TAB = (CURRENT_TAB - 1 + TAB_COUNT) % TAB_COUNT))
                        SCROLL_OFFSET_LOG=0
                        SCROLL_OFFSET_DEBUG=0
                        ;;
                esac
                ;;
            'q'|'Q')
                return 1
                ;;
            'r'|'R')
                # Ręczne odświeżenie
                ;;
            'a'|'A')
                AUTO_REFRESH=!$AUTO_REFRESH
                ;;
            'h'|'H'|'?')
                # Pomoc (można rozwinąć)
                ;;
        esac
    fi
    return 0
}

# Funkcja czyszcząca przy wyjściu
cleanup() {
    stop_collector
    show_cursor
    tput clear
    tput cup 0 0
    echo -ne "${COLOR_RESET}"
    stty sane
    echo "APIARY TUI zakończone."
}

# Główna pętla aplikacji
main_loop() {
    trap cleanup EXIT INT TERM
    
    # Konfiguracja terminala
    stty -echo -icanon min 0 time 0
    
    init_system
    
    # Uruchom kolektor danych w tle
    start_collector || {
        echo -e "${COLOR_YELLOW}Uwaga: Kontynuuję w trybie offline (bez kolektora)${COLOR_RESET}"
    }
    
    while true; do
        draw_ui
        
        if ! handle_input; then
            break
        fi
        
        # Auto-refresh
        if [ "$AUTO_REFRESH" = true ]; then
            sleep $REFRESH_INTERVAL
        else
            sleep 0.1
        fi
    done
}

# Punkt wejścia
main() {
    echo "Uruchamianie APIARY Guard TUI..."
    
    # Sprawdź zależności
    if ! command -v tput &> /dev/null; then
        echo "BŁĄD: Wymagany jest tput (pakiet ncurses-bin)"
        exit 1
    fi
    
    main_loop
}

# Uruchom jeśli skrypt jest wykonywany bezpośrednio
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
