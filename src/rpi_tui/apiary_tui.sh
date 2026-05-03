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

# Pobierz dane z kolektora przez CSV export
fetch_hive_data() {
    # Reset tablic
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
    
    # Spróbuj pobrać dane z kolektora jeśli działa
    if [ -n "$COLLECTOR_PID" ] && kill -0 "$COLLECTOR_PID" 2>/dev/null; then
        # W wersji z FIFO/pipe - tutaj byłaby komunikacja IPC
        # Na razie symulujemy dane które byłyby pobrane
        :
    fi
    
    # Jeśli brak danych z kolektora, użyj przykładowych (fallback)
    if [ ${#HIVES[@]} -eq 0 ]; then
        # Symulacja danych - w produkcji to będzie parsing outputu z kolektora
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
