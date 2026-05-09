#!/bin/bash

# Hive Monitor Installer & Configurator
# Shell script with TUI menu system

# Note: We don't use set -e because we handle errors manually in critical sections
# This allows proper error handling and user feedback instead of abrupt exits

# ============================================================================
# GLOBAL VARIABLES
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="${HOME}/.hive_monitor"
BACKUP_DIR="${CONFIG_DIR}/backups"
LOG_FILE="${CONFIG_DIR}/installer.log"
CURRENT_LANG="en"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ============================================================================
# LANGUAGE STRINGS
# ============================================================================

declare -A LANG_EN=(
    [TITLE]="Hive Monitor Installer & Configurator"
    [MENU_1]="Language / Język"
    [MENU_2]="Install Dependencies"
    [MENU_3]="Install Software from GitHub"
    [MENU_4]="Backup Configuration Files"
    [MENU_5]="Configure Application"
    [MENU_6]="Live Microcontroller Data"
    [MENU_7]="Historical Data Browser"
    [MENU_8]="System Status Check"
    [MENU_9]="Reset to Defaults"
    [MENU_0]="Exit"
    [SELECTED]="Selected language: English"
    [PRESS_KEY]="Press any key to continue..."
    [INVALID]="Invalid option"
    [WELCOME]="Welcome to Hive Monitor Setup"
)

declare -A LANG_PL=(
    [TITLE]="Instalator i Konfigurator Hive Monitor"
    [MENU_1]="Język / Language"
    [MENU_2]="Instaluj zależności"
    [MENU_3]="Instaluj oprogramowanie z GitHub"
    [MENU_4]="Backup plików konfiguracyjnych"
    [MENU_5]="Konfiguruj aplikację"
    [MENU_6]="Dane z mikrokontrolera na żywo"
    [MENU_7]="Przeglądaj dane historyczne"
    [MENU_8]="Sprawdź status systemu"
    [MENU_9]="Przywróć ustawienia domyślne"
    [MENU_0]="Wyjście"
    [SELECTED]="Wybrany język: Polski"
    [PRESS_KEY]="Naciśnij dowolny klawisz, aby kontynuować..."
    [INVALID]="Nieprawidłowa opcja"
    [WELCOME]="Witaj w konfiguracji Hive Monitor"
)

# Current language dictionary - properly copy associative array
declare -A LANG
for key in "${!LANG_EN[@]}"; do
    LANG[$key]="${LANG_EN[$key]}"
done

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

log_message() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" >> "$LOG_FILE" 2>/dev/null || true
}

clear_screen() {
    clear
    echo -e "${CYAN}================================================================${NC}"
    echo -e "${CYAN}  ${LANG[TITLE]}${NC}"
    echo -e "${CYAN}================================================================${NC}"
    echo ""
}

wait_for_key() {
    echo ""
    echo -e "${YELLOW}${LANG[PRESS_KEY]}${NC}"
    read -n 1 -s
}

print_header() {
    clear_screen
    echo -e "${GREEN}>>> ${1}${NC}"
    echo ""
}

# ============================================================================
# MENU OPTION IMPLEMENTATIONS (STUBS)
# ============================================================================

option_language() {
    print_header "${LANG[MENU_1]}"
    echo "1) English"
    echo "2) Polski"
    echo ""
    read -p "Select language (1-2): " lang_choice
    
    case $lang_choice in
        1)
            # Properly copy associative array for English
            for key in "${!LANG_EN[@]}"; do
                LANG[$key]="${LANG_EN[$key]}"
            done
            CURRENT_LANG="en"
            echo -e "${GREEN}${LANG[SELECTED]}${NC}"
            log_message "INFO" "Language changed to English"
            ;;
        2)
            # Properly copy associative array for Polish
            for key in "${!LANG_PL[@]}"; do
                LANG[$key]="${LANG_PL[$key]}"
            done
            CURRENT_LANG="pl"
            echo -e "${GREEN}${LANG[SELECTED]}${NC}"
            log_message "INFO" "Language changed to Polish"
            ;;
        *)
            echo -e "${RED}${LANG[INVALID]}${NC}"
            ;;
    esac
    
    wait_for_key
}

option_install_dependencies() {
    print_header "${LANG[MENU_2]}"
    
    echo "Installing required system dependencies..."
    echo "Dependencies: git, curl, wget, python3-pip, python3-serial, etc."
    echo ""
    
    # Check if running as root or with sudo
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}Note: You may be prompted for sudo password${NC}"
    fi
    
    # Helper function to run commands with proper privilege handling
    run_cmd() {
        if [[ $EUID -eq 0 ]]; then
            eval "$@"
        else
            sudo "$@"
        fi
    }
    
    # Detect package manager and install dependencies
    if command -v apt-get &> /dev/null; then
        echo "Detected Debian/Ubuntu package manager (apt-get)"
        run_cmd apt-get update || {
            echo -e "${RED}Failed to update package lists${NC}"
            log_message "ERROR" "Failed to update apt package lists"
            wait_for_key
            return 1
        }
        run_cmd apt-get install -y git curl wget python3-pip python3-serial python3-dev build-essential || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via apt-get"
            wait_for_key
            return 1
        }
    elif command -v yum &> /dev/null; then
        echo "Detected RHEL/CentOS package manager (yum)"
        run_cmd yum install -y git curl wget python3-pip python3-serial python3-devel gcc make || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via yum"
            wait_for_key
            return 1
        }
    elif command -v dnf &> /dev/null; then
        echo "Detected Fedora package manager (dnf)"
        run_cmd dnf install -y git curl wget python3-pip python3-serial python3-devel gcc make || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via dnf"
            wait_for_key
            return 1
        }
    elif command -v pacman &> /dev/null; then
        echo "Detected Arch Linux package manager (pacman)"
        run_cmd pacman -Sy --noconfirm git curl wget python python-pip python-serial base-devel || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via pacman"
            wait_for_key
            return 1
        }
    else
        echo -e "${RED}Error: Unsupported package manager. Please install dependencies manually.${NC}"
        echo "Required packages: git, curl, wget, python3-pip, python3-serial"
        log_message "ERROR" "Unsupported package manager"
        wait_for_key
        return 1
    fi
    
    echo ""
    echo -e "${GREEN}Dependencies installed successfully!${NC}"
    log_message "INFO" "Dependencies installation completed"
    wait_for_key
}

option_install_software() {
    print_header "${LANG[MENU_3]}"
    
    GITHUB_REPO="https://github.com/bartoszruta26-droid/Hive_monitor"
    INSTALL_DIR="${HOME}/hive_monitor"
    
    echo "This option will clone and install software from:"
    echo "$GITHUB_REPO"
    echo ""
    
    # Check if git is installed
    if ! command -v git &> /dev/null; then
        echo -e "${RED}Error: Git is not installed. Please run 'Install Dependencies' first.${NC}"
        log_message "ERROR" "Git not found during software installation"
        wait_for_key
        return 1
    fi
    
    # Check if directory already exists
    if [[ -d "$INSTALL_DIR" ]]; then
        echo -e "${YELLOW}Directory $INSTALL_DIR already exists.${NC}"
        echo ""
        echo "1) Update existing installation (git pull)"
        echo "2) Remove and reinstall"
        echo "3) Cancel"
        echo ""
        read -p "Select option (1-3): " update_choice
        
        case $update_choice in
            1)
                echo "Updating existing installation..."
                cd "$INSTALL_DIR"
                if ! git pull origin main 2>/dev/null; then
                    if ! git pull origin master 2>/dev/null; then
                        echo -e "${RED}Failed to update repository${NC}"
                        log_message "ERROR" "Failed to update repository from GitHub"
                        wait_for_key
                        return 1
                    fi
                fi
                echo -e "${GREEN}Update completed!${NC}"
                log_message "INFO" "Software updated from GitHub"
                ;;
            2)
                echo "Removing existing installation..."
                rm -rf "$INSTALL_DIR"
                echo "Cloning repository..."
                if ! git clone "$GITHUB_REPO" "$INSTALL_DIR"; then
                    echo -e "${RED}Failed to clone repository${NC}"
                    log_message "ERROR" "Failed to clone repository from GitHub during reinstall"
                    wait_for_key
                    return 1
                fi
                echo -e "${GREEN}Installation completed!${NC}"
                log_message "INFO" "Software reinstalled from GitHub"
                ;;
            3)
                echo "Operation cancelled."
                log_message "INFO" "Software installation cancelled by user"
                wait_for_key
                return 0
                ;;
            *)
                echo -e "${RED}Invalid option${NC}"
                wait_for_key
                return 1
                ;;
        esac
    else
        echo "Cloning repository..."
        if ! git clone "$GITHUB_REPO" "$INSTALL_DIR"; then
            echo -e "${RED}Failed to clone repository${NC}"
            log_message "ERROR" "Failed to clone repository from GitHub"
            wait_for_key
            return 1
        fi
        
        echo ""
        echo -e "${GREEN}Repository cloned successfully to $INSTALL_DIR${NC}"
        echo ""
        
        # Check if there's an install script
        if [[ -f "$INSTALL_DIR/install.sh" ]]; then
            echo "Found install.sh script. Would you like to run it?"
            read -p "Run install script? (y/N): " run_install
            if [[ $run_install =~ ^[Yy]$ ]]; then
                chmod +x "$INSTALL_DIR/install.sh"
                cd "$INSTALL_DIR" && ./install.sh
            fi
        fi
        
        # Install Python dependencies if requirements.txt exists
        if [[ -f "$INSTALL_DIR/requirements.txt" ]]; then
            echo ""
            echo "Installing Python dependencies..."
            pip3 install -r "$INSTALL_DIR/requirements.txt" || {
                echo -e "${YELLOW}Warning: Some Python dependencies may have failed to install${NC}"
                log_message "WARN" "Some Python dependencies failed to install"
            }
        fi
        
        log_message "INFO" "Software installation from GitHub completed"
    fi
    
    wait_for_key
}

option_backup_config() {
    print_header "${LANG[MENU_4]}"
    
    TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
    BACKUP_FILE="${BACKUP_DIR}/hive_monitor_backup_${TIMESTAMP}.tar.gz"
    
    echo "This option will create backups of edited/replaced configuration files."
    echo "Backup location: ${BACKUP_DIR}"
    echo ""
    
    # Create backup directory if it doesn't exist
    mkdir -p "$BACKUP_DIR"
    
    # Define config files to backup with their relative paths for preservation
    declare -A CONFIG_FILES_MAP=(
        ["${CONFIG_DIR}/config.ini"]="config_ini"
        ["${CONFIG_DIR}/settings.json"]="settings_json"
        ["${CONFIG_DIR}/database.conf"]="database_conf"
        ["${CONFIG_DIR}/api_keys.conf"]="api_keys_conf"
        ["${HOME}/hive_monitor/config.ini"]="hive_monitor_config_ini"
        ["${HOME}/hive_monitor/.env"]="hive_monitor_env"
    )
    
    # Create temporary directory for backup preserving structure
    TEMP_BACKUP_DIR=$(mktemp -d)
    FILES_FOUND=0
    
    echo "Searching for configuration files..."
    for file in "${!CONFIG_FILES_MAP[@]}"; do
        if [[ -f "$file" ]]; then
            echo "  Found: $file"
            # Use unique name to avoid overwrites from files with same basename
            unique_name="${CONFIG_FILES_MAP[$file]}"
            cp "$file" "$TEMP_BACKUP_DIR/$unique_name"
            # Also store original path for reference
            echo "$file" > "$TEMP_BACKUP_DIR/${unique_name}.original_path"
            FILES_FOUND=$((FILES_FOUND + 1))
        fi
    done
    
    # Also backup entire config directory if it exists
    if [[ -d "$CONFIG_DIR" ]]; then
        echo "  Backing up entire config directory: $CONFIG_DIR"
        cp -r "$CONFIG_DIR" "$TEMP_BACKUP_DIR/config_dir_backup"
        FILES_FOUND=$((FILES_FOUND + 1))
    fi
    
    # Backup hive_monitor directory if it exists and is different from CONFIG_DIR
    if [[ -d "${HOME}/hive_monitor" ]] && [[ "${HOME}/hive_monitor" != "$CONFIG_DIR" ]]; then
        echo "  Backing up hive_monitor directory: ${HOME}/hive_monitor"
        cp -r "${HOME}/hive_monitor" "$TEMP_BACKUP_DIR/hive_monitor_backup"
        FILES_FOUND=$((FILES_FOUND + 1))
    fi
    
    if [[ $FILES_FOUND -eq 0 ]]; then
        echo -e "${YELLOW}No configuration files found to backup.${NC}"
        rm -rf "$TEMP_BACKUP_DIR"
        log_message "INFO" "No config files found for backup"
        wait_for_key
        return 0
    fi
    
    echo ""
    echo "Creating backup archive: $BACKUP_FILE"
    tar -czf "$BACKUP_FILE" -C "$TEMP_BACKUP_DIR" .
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}Backup created successfully!${NC}"
        echo "Backup file: $BACKUP_FILE"
        echo "Size: $(du -h "$BACKUP_FILE" | cut -f1)"
        log_message "INFO" "Configuration backup created: $BACKUP_FILE"
        
        # Cleanup old backups (keep last 5)
        echo ""
        echo "Cleaning up old backups (keeping last 5)..."
        cd "$BACKUP_DIR" && ls -t hive_monitor_backup_*.tar.gz 2>/dev/null | tail -n +6 | xargs -r rm
    else
        echo -e "${RED}Failed to create backup${NC}"
        log_message "ERROR" "Failed to create configuration backup"
    fi
    
    # Cleanup temp directory
    rm -rf "$TEMP_BACKUP_DIR"
    
    wait_for_key
}

# ============================================================================
# HELPER FUNCTIONS FOR SAFE CONFIG HANDLING
# ============================================================================

# Bezpieczne odczytanie wartości z pliku konfiguracyjnego (bez source!)
get_config_value() {
    local file="$1"
    local key="$2"
    local default="$3"
    
    if [[ -f "$file" ]]; then
        local value
        # Odczytaj wartość po znaku =, usuń cudzysłowy
        value=$(grep "^${key}=" "$file" 2>/dev/null | head -n1 | cut -d'=' -f2-)
        if [[ -n "$value" ]]; then
            # Usuń otaczające cudzysłowy
            value="${value#\"}"
            value="${value%\"}"
            value="${value#\'}"
            value="${value%\'}"
            echo "$value"
            return 0
        fi
    fi
    echo "$default"
    return 1
}

# Bezpieczny zapis pojedynczej wartości do pliku konfiguracyjnego
set_config_value() {
    local file="$1"
    local key="$2"
    local value="$3"
    
    mkdir -p "$(dirname "$file")"
    
    if [[ -f "$file" ]]; then
        if grep -q "^${key}=" "$file" 2>/dev/null; then
            # Aktualizuj istniejącą wartość
            sed -i "s|^${key}=.*|${key}=\"${value}\"|" "$file"
        else
            # Dodaj nową wartość
            echo "${key}=\"${value}\"" >> "$file"
        fi
    else
        # Utwórz nowy plik
        echo "${key}=\"${value}\"" > "$file"
    fi
}

# ============================================================================
# CONFIGURATION SUB-MENU FUNCTIONS
# ============================================================================

configure_api_endpoints() {
    print_header "Configure API Endpoints"
    
    local config_file="${CONFIG_DIR}/api_endpoints.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local LOCAL_API_URL
    LOCAL_API_URL=$(get_config_value "$config_file" "LOCAL_API_URL" "http://localhost:8080/api")
    
    local REMOTE_API_URL
    REMOTE_API_URL=$(get_config_value "$config_file" "REMOTE_API_URL" "https://hive-monitor.example.com/api")
    
    local MQTT_BROKER
    MQTT_BROKER=$(get_config_value "$config_file" "MQTT_BROKER" "localhost")
    
    local MQTT_PORT
    MQTT_PORT=$(get_config_value "$config_file" "MQTT_PORT" "1883")
    
    echo "Current API Configuration:"
    echo "--------------------------"
    echo "Local API URL: $LOCAL_API_URL"
    echo "Remote API URL: $REMOTE_API_URL"
    echo "MQTT Broker: $MQTT_BROKER"
    echo "MQTT Port: $MQTT_PORT"
    echo ""
    
    read -p "Enter Local API URL [$LOCAL_API_URL]: " new_local
    [[ -n "$new_local" ]] && LOCAL_API_URL="$new_local"
    
    read -p "Enter Remote API URL [$REMOTE_API_URL]: " new_remote
    [[ -n "$new_remote" ]] && REMOTE_API_URL="$new_remote"
    
    read -p "Enter MQTT Broker [$MQTT_BROKER]: " new_broker
    [[ -n "$new_broker" ]] && MQTT_BROKER="$new_broker"
    
    read -p "Enter MQTT Port [$MQTT_PORT]: " new_port
    [[ -n "$new_port" ]] && MQTT_PORT="$new_port"
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "LOCAL_API_URL" "$LOCAL_API_URL"
    set_config_value "$config_file" "REMOTE_API_URL" "$REMOTE_API_URL"
    set_config_value "$config_file" "MQTT_BROKER" "$MQTT_BROKER"
    set_config_value "$config_file" "MQTT_PORT" "$MQTT_PORT"
    
    echo ""
    echo -e "${GREEN}API endpoints configuration saved!${NC}"
    log_message "INFO" "API endpoints configured"
    wait_for_key
}

configure_database() {
    print_header "Configure Database Settings"
    
    local config_file="${CONFIG_DIR}/database.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości z zachowaniem istniejącego typu bazy
    local DB_TYPE
    DB_TYPE=$(get_config_value "$config_file" "DB_TYPE" "sqlite")
    
    local DB_HOST
    DB_HOST=$(get_config_value "$config_file" "DB_HOST" "localhost")
    
    local DB_PORT
    DB_PORT=$(get_config_value "$config_file" "DB_PORT" "5432")
    
    local DB_NAME
    DB_NAME=$(get_config_value "$config_file" "DB_NAME" "hive_monitor")
    
    local DB_USER
    DB_USER=$(get_config_value "$config_file" "DB_USER" "hiveuser")
    
    local DB_PASSWORD
    DB_PASSWORD=$(get_config_value "$config_file" "DB_PASSWORD" "")
    
    local DB_PATH
    DB_PATH=$(get_config_value "$config_file" "DB_PATH" "${CONFIG_DIR}/data/hive_monitor.db")
    
    echo "Current Database Configuration:"
    echo "--------------------------------"
    echo "Database Type: $DB_TYPE"
    echo "Host: $DB_HOST"
    echo "Port: $DB_PORT"
    echo "Database Name: $DB_NAME"
    echo "Username: $DB_USER"
    if [[ -n "$DB_PASSWORD" ]]; then
        echo "Password: ${DB_PASSWORD:0:3}***"
    else
        echo "Password: (not set)"
    fi
    echo "Path (for SQLite): $DB_PATH"
    echo ""
    
    echo "Select database type:"
    echo "1) SQLite (default, file-based)"
    echo "2) PostgreSQL"
    echo "3) MySQL/MariaDB"
    read -p "Choice [1-3, Enter to keep current]: " db_choice
    
    case $db_choice in
        2) DB_TYPE="postgresql" ;;
        3) DB_TYPE="mysql" ;;
        *) 
            # Pozostaw obecny typ bazy przy pustym wejściu
            if [[ -z "$db_choice" ]]; then
                echo "Keeping current database type: $DB_TYPE"
            else
                DB_TYPE="sqlite"
            fi
            ;;
    esac
    
    if [[ "$DB_TYPE" != "sqlite" ]]; then
        read -p "Enter database host [$DB_HOST]: " new_host
        [[ -n "$new_host" ]] && DB_HOST="$new_host"
        
        read -p "Enter database port [$DB_PORT]: " new_port
        [[ -n "$new_port" ]] && DB_PORT="$new_port"
        
        read -p "Enter database name [$DB_NAME]: " new_name
        [[ -n "$new_name" ]] && DB_NAME="$new_name"
        
        read -p "Enter database username [$DB_USER]: " new_user
        [[ -n "$new_user" ]] && DB_USER="$new_user"
        
        read -sp "Enter database password [press Enter to keep current]: " new_pass
        echo ""
        [[ -n "$new_pass" ]] && DB_PASSWORD="$new_pass"
    else
        read -p "Enter database file path [$DB_PATH]: " new_path
        [[ -n "$new_path" ]] && DB_PATH="$new_path"
    fi
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "DB_TYPE" "$DB_TYPE"
    set_config_value "$config_file" "DB_HOST" "$DB_HOST"
    set_config_value "$config_file" "DB_PORT" "$DB_PORT"
    set_config_value "$config_file" "DB_NAME" "$DB_NAME"
    set_config_value "$config_file" "DB_USER" "$DB_USER"
    set_config_value "$config_file" "DB_PASSWORD" "$DB_PASSWORD"
    set_config_value "$config_file" "DB_PATH" "$DB_PATH"
    
    echo ""
    echo -e "${GREEN}Database configuration saved!${NC}"
    
    # Create directory for SQLite database if needed
    if [[ "$DB_TYPE" == "sqlite" ]]; then
        mkdir -p "$(dirname "$DB_PATH")"
    fi
    
    log_message "INFO" "Database configured: $DB_TYPE"
    wait_for_key
}

configure_intervals() {
    print_header "Configure Update Intervals"
    
    local config_file="${CONFIG_DIR}/intervals.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local SENSOR_INTERVAL
    SENSOR_INTERVAL=$(get_config_value "$config_file" "SENSOR_INTERVAL" "30")
    
    local DISPLAY_INTERVAL
    DISPLAY_INTERVAL=$(get_config_value "$config_file" "DISPLAY_INTERVAL" "5")
    
    local LOG_INTERVAL
    LOG_INTERVAL=$(get_config_value "$config_file" "LOG_INTERVAL" "300")
    
    local SYNC_INTERVAL
    SYNC_INTERVAL=$(get_config_value "$config_file" "SYNC_INTERVAL" "3600")
    
    local BACKUP_INTERVAL
    BACKUP_INTERVAL=$(get_config_value "$config_file" "BACKUP_INTERVAL" "86400")
    
    echo "Current Update Intervals (in seconds):"
    echo "---------------------------------------"
    echo "Sensor reading interval: $SENSOR_INTERVAL s"
    echo "Display refresh interval: $DISPLAY_INTERVAL s"
    echo "Log write interval: $LOG_INTERVAL s"
    echo "Cloud sync interval: $SYNC_INTERVAL s"
    echo "Auto-backup interval: $BACKUP_INTERVAL s"
    echo ""
    
    read -p "Enter sensor reading interval (5-300) [$SENSOR_INTERVAL]: " new_sensor
    if [[ -n "$new_sensor" && "$new_sensor" =~ ^[0-9]+$ && "$new_sensor" -ge 5 && "$new_sensor" -le 300 ]]; then
        SENSOR_INTERVAL="$new_sensor"
    elif [[ -n "$new_sensor" ]]; then
        echo -e "${YELLOW}Warning: Value out of range, using default${NC}"
    fi
    
    read -p "Enter display refresh interval (1-60) [$DISPLAY_INTERVAL]: " new_display
    if [[ -n "$new_display" && "$new_display" =~ ^[0-9]+$ && "$new_display" -ge 1 && "$new_display" -le 60 ]]; then
        DISPLAY_INTERVAL="$new_display"
    fi
    
    read -p "Enter log write interval (60-3600) [$LOG_INTERVAL]: " new_log
    if [[ -n "$new_log" && "$new_log" =~ ^[0-9]+$ && "$new_log" -ge 60 && "$new_log" -le 3600 ]]; then
        LOG_INTERVAL="$new_log"
    fi
    
    read -p "Enter cloud sync interval (300-86400) [$SYNC_INTERVAL]: " new_sync
    if [[ -n "$new_sync" && "$new_sync" =~ ^[0-9]+$ && "$new_sync" -ge 300 && "$new_sync" -le 86400 ]]; then
        SYNC_INTERVAL="$new_sync"
    fi
    
    read -p "Enter auto-backup interval (3600-604800) [$BACKUP_INTERVAL]: " new_backup
    if [[ -n "$new_backup" && "$new_backup" =~ ^[0-9]+$ && "$new_backup" -ge 3600 && "$new_backup" -le 604800 ]]; then
        BACKUP_INTERVAL="$new_backup"
    fi
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "SENSOR_INTERVAL" "$SENSOR_INTERVAL"
    set_config_value "$config_file" "DISPLAY_INTERVAL" "$DISPLAY_INTERVAL"
    set_config_value "$config_file" "LOG_INTERVAL" "$LOG_INTERVAL"
    set_config_value "$config_file" "SYNC_INTERVAL" "$SYNC_INTERVAL"
    set_config_value "$config_file" "BACKUP_INTERVAL" "$BACKUP_INTERVAL"
    
    echo ""
    echo -e "${GREEN}Update intervals configuration saved!${NC}"
    log_message "INFO" "Update intervals configured"
    wait_for_key
}

configure_notifications() {
    print_header "Configure Notifications"
    
    local config_file="${CONFIG_DIR}/notifications.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local EMAIL_ENABLED
    EMAIL_ENABLED=$(get_config_value "$config_file" "EMAIL_ENABLED" "false")
    
    local EMAIL_ADDRESS
    EMAIL_ADDRESS=$(get_config_value "$config_file" "EMAIL_ADDRESS" "")
    
    local SMTP_SERVER
    SMTP_SERVER=$(get_config_value "$config_file" "SMTP_SERVER" "smtp.gmail.com")
    
    local SMTP_PORT
    SMTP_PORT=$(get_config_value "$config_file" "SMTP_PORT" "587")
    
    local PUSH_ENABLED
    PUSH_ENABLED=$(get_config_value "$config_file" "PUSH_ENABLED" "false")
    
    local PUSH_TOKEN
    PUSH_TOKEN=$(get_config_value "$config_file" "PUSH_TOKEN" "")
    
    local TEMP_THRESHOLD_HIGH
    TEMP_THRESHOLD_HIGH=$(get_config_value "$config_file" "TEMP_THRESHOLD_HIGH" "35")
    
    local TEMP_THRESHOLD_LOW
    TEMP_THRESHOLD_LOW=$(get_config_value "$config_file" "TEMP_THRESHOLD_LOW" "10")
    
    local HUMIDITY_THRESHOLD
    HUMIDITY_THRESHOLD=$(get_config_value "$config_file" "HUMIDITY_THRESHOLD" "80")
    
    echo "Current Notification Settings:"
    echo "-------------------------------"
    echo "Email notifications: $EMAIL_ENABLED"
    [[ -n "$EMAIL_ADDRESS" ]] && echo "Email address: $EMAIL_ADDRESS"
    echo "Push notifications: $PUSH_ENABLED"
    echo ""
    echo "Alert Thresholds:"
    echo "Temperature high: ${TEMP_THRESHOLD_HIGH}°C"
    echo "Temperature low: ${TEMP_THRESHOLD_LOW}°C"
    echo "Humidity high: ${HUMIDITY_THRESHOLD}%"
    echo ""
    
    echo "Enable email notifications?"
    read -p "(y/N): " email_choice
    if [[ "$email_choice" =~ ^[Yy]$ ]]; then
        EMAIL_ENABLED="true"
        read -p "Enter email address: " EMAIL_ADDRESS
        read -p "Enter SMTP server [$SMTP_SERVER]: " new_smtp
        [[ -n "$new_smtp" ]] && SMTP_SERVER="$new_smtp"
        read -p "Enter SMTP port [$SMTP_PORT]: " new_smtp_port
        [[ -n "$new_smtp_port" ]] && SMTP_PORT="$new_smtp_port"
    else
        EMAIL_ENABLED="false"
    fi
    
    echo ""
    echo "Enable push notifications?"
    read -p "(y/N): " push_choice
    if [[ "$push_choice" =~ ^[Yy]$ ]]; then
        PUSH_ENABLED="true"
        read -p "Enter push notification token: " PUSH_TOKEN
    else
        PUSH_ENABLED="false"
    fi
    
    echo ""
    echo "Set alert thresholds:"
    read -p "Temperature high threshold (°C) [$TEMP_THRESHOLD_HIGH]: " new_temp_high
    [[ -n "$new_temp_high" ]] && TEMP_THRESHOLD_HIGH="$new_temp_high"
    
    read -p "Temperature low threshold (°C) [$TEMP_THRESHOLD_LOW]: " new_temp_low
    [[ -n "$new_temp_low" ]] && TEMP_THRESHOLD_LOW="$new_temp_low"
    
    read -p "Humidity threshold (%) [$HUMIDITY_THRESHOLD]: " new_humidity
    [[ -n "$new_humidity" ]] && HUMIDITY_THRESHOLD="$new_humidity"
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "EMAIL_ENABLED" "$EMAIL_ENABLED"
    set_config_value "$config_file" "EMAIL_ADDRESS" "$EMAIL_ADDRESS"
    set_config_value "$config_file" "SMTP_SERVER" "$SMTP_SERVER"
    set_config_value "$config_file" "SMTP_PORT" "$SMTP_PORT"
    set_config_value "$config_file" "PUSH_ENABLED" "$PUSH_ENABLED"
    set_config_value "$config_file" "PUSH_TOKEN" "$PUSH_TOKEN"
    set_config_value "$config_file" "TEMP_THRESHOLD_HIGH" "$TEMP_THRESHOLD_HIGH"
    set_config_value "$config_file" "TEMP_THRESHOLD_LOW" "$TEMP_THRESHOLD_LOW"
    set_config_value "$config_file" "HUMIDITY_THRESHOLD" "$HUMIDITY_THRESHOLD"
    
    echo ""
    echo -e "${GREEN}Notifications configuration saved!${NC}"
    log_message "INFO" "Notifications configured"
    wait_for_key
}

configure_data_retention() {
    print_header "Configure Data Retention Policy"
    
    local config_file="${CONFIG_DIR}/retention.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local RAW_DATA_RETENTION
    RAW_DATA_RETENTION=$(get_config_value "$config_file" "RAW_DATA_RETENTION" "30")
    
    local HOURLY_DATA_RETENTION
    HOURLY_DATA_RETENTION=$(get_config_value "$config_file" "HOURLY_DATA_RETENTION" "365")
    
    local DAILY_DATA_RETENTION
    DAILY_DATA_RETENTION=$(get_config_value "$config_file" "DAILY_DATA_RETENTION" "730")
    
    local ENABLE_COMPRESSION
    ENABLE_COMPRESSION=$(get_config_value "$config_file" "ENABLE_COMPRESSION" "true")
    
    local COMPRESSION_AFTER_DAYS
    COMPRESSION_AFTER_DAYS=$(get_config_value "$config_file" "COMPRESSION_AFTER_DAYS" "7")
    
    echo "Current Data Retention Policy:"
    echo "-------------------------------"
    echo "Raw data retention: $RAW_DATA_RETENTION days"
    echo "Hourly aggregated data: $HOURLY_DATA_RETENTION days"
    echo "Daily aggregated data: $DAILY_DATA_RETENTION days"
    echo "Enable compression: $ENABLE_COMPRESSION"
    echo "Compress after: $COMPRESSION_AFTER_DAYS days"
    echo ""
    
    read -p "Raw data retention days (7-365) [$RAW_DATA_RETENTION]: " new_raw
    if [[ -n "$new_raw" && "$new_raw" =~ ^[0-9]+$ && "$new_raw" -ge 7 && "$new_raw" -le 365 ]]; then
        RAW_DATA_RETENTION="$new_raw"
    fi
    
    read -p "Hourly data retention days (30-1095) [$HOURLY_DATA_RETENTION]: " new_hourly
    if [[ -n "$new_hourly" && "$new_hourly" =~ ^[0-9]+$ && "$new_hourly" -ge 30 && "$new_hourly" -le 1095 ]]; then
        HOURLY_DATA_RETENTION="$new_hourly"
    fi
    
    read -p "Daily data retention days (365-3650) [$DAILY_DATA_RETENTION]: " new_daily
    if [[ -n "$new_daily" && "$new_daily" =~ ^[0-9]+$ && "$new_daily" -ge 365 && "$new_daily" -le 3650 ]]; then
        DAILY_DATA_RETENTION="$new_daily"
    fi
    
    echo ""
    echo "Enable data compression?"
    read -p "(Y/n): " compress_choice
    if [[ "$compress_choice" =~ ^[Nn]$ ]]; then
        ENABLE_COMPRESSION="false"
    else
        ENABLE_COMPRESSION="true"
        read -p "Compress data after days (1-30) [$COMPRESSION_AFTER_DAYS]: " new_compress
        if [[ -n "$new_compress" && "$new_compress" =~ ^[0-9]+$ ]]; then
            COMPRESSION_AFTER_DAYS="$new_compress"
        fi
    fi
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "RAW_DATA_RETENTION" "$RAW_DATA_RETENTION"
    set_config_value "$config_file" "HOURLY_DATA_RETENTION" "$HOURLY_DATA_RETENTION"
    set_config_value "$config_file" "DAILY_DATA_RETENTION" "$DAILY_DATA_RETENTION"
    set_config_value "$config_file" "ENABLE_COMPRESSION" "$ENABLE_COMPRESSION"
    set_config_value "$config_file" "COMPRESSION_AFTER_DAYS" "$COMPRESSION_AFTER_DAYS"
    
    echo ""
    echo -e "${GREEN}Data retention policy saved!${NC}"
    log_message "INFO" "Data retention policy configured"
    wait_for_key
}

configure_network() {
    print_header "Configure Network Settings"
    
    local config_file="${CONFIG_DIR}/network.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local DEVICE_HOSTNAME
    DEVICE_HOSTNAME=$(get_config_value "$config_file" "DEVICE_HOSTNAME" "hive-monitor")
    
    local USE_STATIC_IP
    USE_STATIC_IP=$(get_config_value "$config_file" "USE_STATIC_IP" "false")
    
    local STATIC_IP
    STATIC_IP=$(get_config_value "$config_file" "STATIC_IP" "192.168.1.100")
    
    local STATIC_NETMASK
    STATIC_NETMASK=$(get_config_value "$config_file" "STATIC_NETMASK" "255.255.255.0")
    
    local STATIC_GATEWAY
    STATIC_GATEWAY=$(get_config_value "$config_file" "STATIC_GATEWAY" "192.168.1.1")
    
    local STATIC_DNS
    STATIC_DNS=$(get_config_value "$config_file" "STATIC_DNS" "8.8.8.8")
    
    local ENABLE_WEBSERVER
    ENABLE_WEBSERVER=$(get_config_value "$config_file" "ENABLE_WEBSERVER" "true")
    
    local WEBSERVER_PORT
    WEBSERVER_PORT=$(get_config_value "$config_file" "WEBSERVER_PORT" "8080")
    
    echo "Current Network Settings:"
    echo "-------------------------"
    echo "Hostname: $DEVICE_HOSTNAME"
    echo "Static IP: $USE_STATIC_IP"
    [[ "$USE_STATIC_IP" == "true" ]] && echo "  IP: $STATIC_IP"
    [[ "$USE_STATIC_IP" == "true" ]] && echo "  Netmask: $STATIC_NETMASK"
    [[ "$USE_STATIC_IP" == "true" ]] && echo "  Gateway: $STATIC_GATEWAY"
    [[ "$USE_STATIC_IP" == "true" ]] && echo "  DNS: $STATIC_DNS"
    echo "Web server enabled: $ENABLE_WEBSERVER"
    [[ "$ENABLE_WEBSERVER" == "true" ]] && echo "Web server port: $WEBSERVER_PORT"
    echo ""
    
    read -p "Enter device hostname [$DEVICE_HOSTNAME]: " new_hostname
    [[ -n "$new_hostname" ]] && DEVICE_HOSTNAME="$new_hostname"
    
    echo ""
    echo "Use static IP address?"
    read -p "(y/N): " static_choice
    if [[ "$static_choice" =~ ^[Yy]$ ]]; then
        USE_STATIC_IP="true"
        read -p "Enter static IP address [$STATIC_IP]: " new_ip
        [[ -n "$new_ip" ]] && STATIC_IP="$new_ip"
        read -p "Enter netmask [$STATIC_NETMASK]: " new_netmask
        [[ -n "$new_netmask" ]] && STATIC_NETMASK="$new_netmask"
        read -p "Enter gateway [$STATIC_GATEWAY]: " new_gateway
        [[ -n "$new_gateway" ]] && STATIC_GATEWAY="$new_gateway"
        read -p "Enter DNS server [$STATIC_DNS]: " new_dns
        [[ -n "$new_dns" ]] && STATIC_DNS="$new_dns"
    else
        USE_STATIC_IP="false"
    fi
    
    echo ""
    echo "Enable built-in web server?"
    read -p "(Y/n): " web_choice
    if [[ "$web_choice" =~ ^[Nn]$ ]]; then
        ENABLE_WEBSERVER="false"
    else
        ENABLE_WEBSERVER="true"
        read -p "Enter web server port (1024-65535) [$WEBSERVER_PORT]: " new_port
        if [[ -n "$new_port" && "$new_port" =~ ^[0-9]+$ && "$new_port" -ge 1024 && "$new_port" -le 65535 ]]; then
            WEBSERVER_PORT="$new_port"
        fi
    fi
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "DEVICE_HOSTNAME" "$DEVICE_HOSTNAME"
    set_config_value "$config_file" "USE_STATIC_IP" "$USE_STATIC_IP"
    set_config_value "$config_file" "STATIC_IP" "$STATIC_IP"
    set_config_value "$config_file" "STATIC_NETMASK" "$STATIC_NETMASK"
    set_config_value "$config_file" "STATIC_GATEWAY" "$STATIC_GATEWAY"
    set_config_value "$config_file" "STATIC_DNS" "$STATIC_DNS"
    set_config_value "$config_file" "ENABLE_WEBSERVER" "$ENABLE_WEBSERVER"
    set_config_value "$config_file" "WEBSERVER_PORT" "$WEBSERVER_PORT"
    
    echo ""
    echo -e "${GREEN}Network settings saved!${NC}"
    echo -e "${YELLOW}Note: Network changes may require a reboot to take effect.${NC}"
    log_message "INFO" "Network settings configured"
    wait_for_key
}

configure_user_preferences() {
    print_header "Configure User Preferences"
    
    local config_file="${CONFIG_DIR}/user_prefs.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local PREFERRED_LANG
    PREFERRED_LANG=$(get_config_value "$config_file" "PREFERRED_LANG" "en")
    
    local TEMP_UNIT
    TEMP_UNIT=$(get_config_value "$config_file" "TEMP_UNIT" "celsius")
    
    local DATE_FORMAT
    DATE_FORMAT=$(get_config_value "$config_file" "DATE_FORMAT" "YYYY-MM-DD")
    
    local TIME_FORMAT
    TIME_FORMAT=$(get_config_value "$config_file" "TIME_FORMAT" "24h")
    
    local THEME
    THEME=$(get_config_value "$config_file" "THEME" "dark")
    
    local SHOW_GRAPHS
    SHOW_GRAPHS=$(get_config_value "$config_file" "SHOW_GRAPHS" "true")
    
    local AUTO_REFRESH
    AUTO_REFRESH=$(get_config_value "$config_file" "AUTO_REFRESH" "true")
    
    echo "Current User Preferences:"
    echo "-------------------------"
    echo "Language: $PREFERRED_LANG"
    echo "Temperature unit: $TEMP_UNIT"
    echo "Date format: $DATE_FORMAT"
    echo "Time format: $TIME_FORMAT"
    echo "Theme: $THEME"
    echo "Show graphs: $SHOW_GRAPHS"
    echo "Auto-refresh: $AUTO_REFRESH"
    echo ""
    
    echo "Select language:"
    echo "1) English"
    echo "2) Polski"
    read -p "Choice [1]: " lang_choice
    case $lang_choice in
        2) PREFERRED_LANG="pl" ;;
        *) PREFERRED_LANG="en" ;;
    esac
    
    echo ""
    echo "Temperature unit:"
    echo "1) Celsius"
    echo "2) Fahrenheit"
    read -p "Choice [1]: " temp_choice
    case $temp_choice in
        2) TEMP_UNIT="fahrenheit" ;;
        *) TEMP_UNIT="celsius" ;;
    esac
    
    echo ""
    echo "Date format:"
    echo "1) YYYY-MM-DD (ISO)"
    echo "2) DD/MM/YYYY"
    echo "3) MM/DD/YYYY"
    read -p "Choice [1]: " date_choice
    case $date_choice in
        2) DATE_FORMAT="DD/MM/YYYY" ;;
        3) DATE_FORMAT="MM/DD/YYYY" ;;
        *) DATE_FORMAT="YYYY-MM-DD" ;;
    esac
    
    echo ""
    echo "Time format:"
    echo "1) 24-hour"
    echo "2) 12-hour (AM/PM)"
    read -p "Choice [1]: " time_choice
    case $time_choice in
        2) TIME_FORMAT="12h" ;;
        *) TIME_FORMAT="24h" ;;
    esac
    
    echo ""
    echo "Theme:"
    echo "1) Dark"
    echo "2) Light"
    read -p "Choice [1]: " theme_choice
    case $theme_choice in
        2) THEME="light" ;;
        *) THEME="dark" ;;
    esac
    
    echo ""
    echo "Show graphs in interface?"
    read -p "(Y/n): " graph_choice
    [[ "$graph_choice" =~ ^[Nn]$ ]] && SHOW_GRAPHS="false" || SHOW_GRAPHS="true"
    
    echo "Enable auto-refresh?"
    read -p "(Y/n): " refresh_choice
    [[ "$refresh_choice" =~ ^[Nn]$ ]] && AUTO_REFRESH="false" || AUTO_REFRESH="true"
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "PREFERRED_LANG" "$PREFERRED_LANG"
    set_config_value "$config_file" "TEMP_UNIT" "$TEMP_UNIT"
    set_config_value "$config_file" "DATE_FORMAT" "$DATE_FORMAT"
    set_config_value "$config_file" "TIME_FORMAT" "$TIME_FORMAT"
    set_config_value "$config_file" "THEME" "$THEME"
    set_config_value "$config_file" "SHOW_GRAPHS" "$SHOW_GRAPHS"
    set_config_value "$config_file" "AUTO_REFRESH" "$AUTO_REFRESH"
    
    echo ""
    echo -e "${GREEN}User preferences saved!${NC}"
    log_message "INFO" "User preferences configured"
    wait_for_key
}

configure_advanced() {
    print_header "Advanced Configuration Options"
    
    local config_file="${CONFIG_DIR}/advanced.conf"
    mkdir -p "$CONFIG_DIR"
    
    # Bezpieczne ładowanie obecnych wartości
    local DEBUG_MODE
    DEBUG_MODE=$(get_config_value "$config_file" "DEBUG_MODE" "false")
    
    local VERBOSE_LOGGING
    VERBOSE_LOGGING=$(get_config_value "$config_file" "VERBOSE_LOGGING" "false")
    
    local MAX_LOG_SIZE
    MAX_LOG_SIZE=$(get_config_value "$config_file" "MAX_LOG_SIZE" "10")
    
    local LOG_ROTATION_COUNT
    LOG_ROTATION_COUNT=$(get_config_value "$config_file" "LOG_ROTATION_COUNT" "5")
    
    local WATCHDOG_ENABLED
    WATCHDOG_ENABLED=$(get_config_value "$config_file" "WATCHDOG_ENABLED" "true")
    
    local WATCHDOG_TIMEOUT
    WATCHDOG_TIMEOUT=$(get_config_value "$config_file" "WATCHDOG_TIMEOUT" "30")
    
    local SAFE_MODE
    SAFE_MODE=$(get_config_value "$config_file" "SAFE_MODE" "false")
    
    echo "Current Advanced Settings:"
    echo "--------------------------"
    echo "Debug mode: $DEBUG_MODE"
    echo "Verbose logging: $VERBOSE_LOGGING"
    echo "Max log size (MB): $MAX_LOG_SIZE"
    echo "Log rotation count: $LOG_ROTATION_COUNT"
    echo "Watchdog enabled: $WATCHDOG_ENABLED"
    echo "Watchdog timeout (sec): $WATCHDOG_TIMEOUT"
    echo "Safe mode: $SAFE_MODE"
    echo ""
    
    echo "Enable debug mode? (for development)"
    read -p "(y/N): " debug_choice
    [[ "$debug_choice" =~ ^[Yy]$ ]] && DEBUG_MODE="true" || DEBUG_MODE="false"
    
    echo "Enable verbose logging?"
    read -p "(y/N): " verbose_choice
    [[ "$verbose_choice" =~ ^[Yy]$ ]] && VERBOSE_LOGGING="true" || VERBOSE_LOGGING="false"
    
    read -p "Max log file size in MB (1-100) [$MAX_LOG_SIZE]: " new_max_log
    if [[ -n "$new_max_log" && "$new_max_log" =~ ^[0-9]+$ && "$new_max_log" -ge 1 && "$new_max_log" -le 100 ]]; then
        MAX_LOG_SIZE="$new_max_log"
    fi
    
    read -p "Number of log files to keep (1-20) [$LOG_ROTATION_COUNT]: " new_rotation
    if [[ -n "$new_rotation" && "$new_rotation" =~ ^[0-9]+$ && "$new_rotation" -ge 1 && "$new_rotation" -le 20 ]]; then
        LOG_ROTATION_COUNT="$new_rotation"
    fi
    
    echo ""
    echo "Enable hardware watchdog?"
    read -p "(Y/n): " watchdog_choice
    if [[ "$watchdog_choice" =~ ^[Nn]$ ]]; then
        WATCHDOG_ENABLED="false"
    else
        WATCHDOG_ENABLED="true"
        read -p "Watchdog timeout in seconds (10-120) [$WATCHDOG_TIMEOUT]: " new_timeout
        if [[ -n "$new_timeout" && "$new_timeout" =~ ^[0-9]+$ && "$new_timeout" -ge 10 && "$new_timeout" -le 120 ]]; then
            WATCHDOG_TIMEOUT="$new_timeout"
        fi
    fi
    
    echo ""
    echo "Enable safe mode? (disables non-critical features)"
    read -p "(y/N): " safe_choice
    [[ "$safe_choice" =~ ^[Yy]$ ]] && SAFE_MODE="true" || SAFE_MODE="false"
    
    # Zapisz konfigurację używając bezpiecznej funkcji
    set_config_value "$config_file" "DEBUG_MODE" "$DEBUG_MODE"
    set_config_value "$config_file" "VERBOSE_LOGGING" "$VERBOSE_LOGGING"
    set_config_value "$config_file" "MAX_LOG_SIZE" "$MAX_LOG_SIZE"
    set_config_value "$config_file" "LOG_ROTATION_COUNT" "$LOG_ROTATION_COUNT"
    set_config_value "$config_file" "WATCHDOG_ENABLED" "$WATCHDOG_ENABLED"
    set_config_value "$config_file" "WATCHDOG_TIMEOUT" "$WATCHDOG_TIMEOUT"
    set_config_value "$config_file" "SAFE_MODE" "$SAFE_MODE"
    
    echo ""
    echo -e "${GREEN}Advanced settings saved!${NC}"
    if [[ "$DEBUG_MODE" == "true" ]]; then
        echo -e "${YELLOW}Warning: Debug mode is enabled. This may impact performance.${NC}"
    fi
    log_message "INFO" "Advanced settings configured"
    wait_for_key
}

option_configure_app() {
    while true; do
        print_header "${LANG[MENU_5]}"
        echo "Application Configuration Menu"
        echo "================================"
        echo "1) Set API endpoints"
        echo "2) Configure database settings"
        echo "3) Set update intervals"
        echo "4) Configure notifications"
        echo "5) Set data retention policy"
        echo "6) Network settings"
        echo "7) User preferences"
        echo "8) Advanced options"
        echo "0) Back to main menu"
        echo ""
        
        read -p "Select option (0-8): " config_choice
        
        case $config_choice in
            1) configure_api_endpoints ;;
            2) configure_database ;;
            3) configure_intervals ;;
            4) configure_notifications ;;
            5) configure_data_retention ;;
            6) configure_network ;;
            7) configure_user_preferences ;;
            8) configure_advanced ;;
            0) break ;;
            *)
                echo -e "${RED}Invalid option${NC}"
                wait_for_key
                ;;
        esac
    done
}

option_live_data() {
    print_header "${LANG[MENU_6]}"
    
    # ============================================================================
    # LIVE MICROCONTROLLER DATA READER - ROZBUDOWANA IMPLEMENTACJA
    # ============================================================================
    
    local selected_port=""
    local baud_rate="9600"
    local monitoring_active=false
    local monitor_pid=""
    local raw_mode=false
    
    # Funkcja pomocnicza: Wykrywanie dostępnych portów szeregowych
    detect_serial_ports() {
        local ports=()
        
        # Sprawdź porty USB CDC (Arduino, Pico, itp.)
        for port in /dev/ttyUSB* /dev/ttyACM*; do
            if [[ -e "$port" ]]; then
                ports+=("$port")
            fi
        done
        
        # Sprawdź porty GPIO UART (Raspberry Pi)
        if [[ -e "/dev/serial0" ]]; then
            ports+=("/dev/serial0")
        fi
        if [[ -e "/dev/ttyAMA0" ]] && [[ ! " ${ports[*]} " =~ " /dev/ttyAMA0 " ]]; then
            ports+=("/dev/ttyAMA0")
        fi
        
        # Sprawdź porty Bluetooth HCI UART
        if [[ -e "/dev/ttyS0" ]] && [[ ! " ${ports[*]} " =~ " /dev/ttyS0 " ]]; then
            ports+=("/dev/ttyS0")
        fi
        
        echo "${ports[@]}"
    }
    
    # Funkcja pomocnicza: Pobierz informacje o porcie
    get_port_info() {
        local port="$1"
        if [[ -e "$port" ]]; then
            local driver=$(udevadm info --query=property --name="$port" 2>/dev/null | grep -E "ID_VENDOR|ID_MODEL|PRODUCT" | head -3 | tr '\n' ' ')
            echo "$port ${driver:-[Unknown device]}"
        else
            echo "$port [Not found]"
        fi
    }
    
    # Funkcja pomocnicza: Automatyczna detekcja mikrokontrolera
    auto_detect_microcontroller() {
        print_header "${LANG[MENU_6]} - Auto Detection"
        echo "Scanning for connected microcontrollers..."
        echo ""
        
        local ports=($(detect_serial_ports))
        
        if [[ ${#ports[@]} -eq 0 ]]; then
            echo -e "${YELLOW}No serial ports detected.${NC}"
            echo ""
            echo "Troubleshooting tips:"
            echo "  1. Check USB cable connection"
            echo "  2. Ensure the device is powered on"
            echo "  3. Try a different USB port"
            echo "  4. Install required drivers:"
            echo "     sudo apt-get install modemmanager"
            echo ""
            return 1
        fi
        
        echo "Found ${#ports[@]} serial port(s):"
        echo ""
        
        local i=1
        local detected_devices=()
        for port in "${ports[@]}"; do
            echo "  [$i] $(get_port_info "$port")"
            detected_devices+=("$port")
            ((i++))
        done
        echo ""
        
        # Spróbuj zidentyfikować typ mikrokontrolera
        echo "Attempting to identify device type..."
        for port in "${detected_devices[@]}"; do
            local vendor_id=""
            local product_id=""
            
            # Pobierz ID urządzenia z udev
            if command -v udevadm &> /dev/null; then
                vendor_id=$(udevadm info --query=property --name="$port" 2>/dev/null | grep "ID_VENDOR_ID" | cut -d'=' -f2)
                product_id=$(udevadm info --query=property --name="$port" 2>/dev/null | grep "ID_MODEL_ID" | cut -d'=' -f2)
            fi
            
            # Identyfikacja na podstawie VID/PID
            local device_type="Unknown"
            case "$vendor_id:$product_id" in
                2341:*|2a03:*)
                    device_type="Arduino (Atmel)"
                    ;;
                0403:*)
                    device_type="FTDI USB-Serial"
                    ;;
                1a86:*)
                    device_type="CH340/CH341 USB-Serial"
                    ;;
                10c4:*)
                    device_type="Silicon Labs CP210x"
                    ;;
                2e8a:*)
                    device_type="Raspberry Pi Pico"
                    ;;
                0483:*)
                    device_type="STM32"
                    ;;
                *)
                    # Spróbuj odczytać nazwę z sysfs
                    if [[ -e "/sys/class/tty/$(basename $port)/device/../product" ]]; then
                        local product_name=$(cat "/sys/class/tty/$(basename $port)/device/../product" 2>/dev/null)
                        if [[ -n "$product_name" ]]; then
                            device_type="$product_name"
                        fi
                    fi
                    ;;
            esac
            
            echo -e "  ${GREEN}✓${NC} $port: ${CYAN}$device_type${NC}"
        done
        echo ""
        
        # Wybierz pierwszy wykryty port lub pozwól użytkownikowi wybrać
        if [[ ${#detected_devices[@]} -eq 1 ]]; then
            selected_port="${detected_devices[0]}"
            echo -e "${GREEN}Auto-selected: $selected_port${NC}"
        else
            read -p "Select port number (1-${#detected_devices[@]}) or 0 to cancel: " port_choice
            if [[ $port_choice -gt 0 ]] && [[ $port_choice -le ${#detected_devices[@]} ]]; then
                selected_port="${detected_devices[$((port_choice-1))]}"
            else
                echo "Selection cancelled."
                return 1
            fi
        fi
        
        return 0
    }
    
    # Funkcja pomocnicza: Ręczny wybór portu
    manual_select_port() {
        print_header "${LANG[MENU_6]} - Manual Selection"
        echo "Available serial ports:"
        echo ""
        
        local ports=($(detect_serial_ports))
        
        if [[ ${#ports[@]} -eq 0 ]]; then
            echo -e "${YELLOW}No serial ports found.${NC}"
            echo ""
            echo "You can still enter a port path manually (e.g., /dev/ttyUSB0)"
        else
            local i=1
            for port in "${ports[@]}"; do
                echo "  [$i] $(get_port_info "$port")"
                ((i++))
            done
            echo "  [${#ports[@]}+1] Enter custom path"
        fi
        echo "  [0] Cancel"
        echo ""
        
        read -p "Select port: " port_choice
        
        if [[ $port_choice -eq 0 ]]; then
            return 1
        elif [[ $port_choice -gt 0 ]] && [[ $port_choice -le ${#ports[@]} ]]; then
            selected_port="${ports[$((port_choice-1))]}"
        else
            read -p "Enter custom port path: " custom_path
            if [[ -e "$custom_path" ]]; then
                selected_port="$custom_path"
            else
                echo -e "${RED}Port $custom_path does not exist${NC}"
                return 1
            fi
        fi
        
        # Wybór prędkości transmisji
        echo ""
        echo "Select baud rate:"
        echo "  1) 9600"
        echo "  2) 19200"
        echo "  3) 38400"
        echo "  4) 57600"
        echo "  5) 115200 (default for most devices)"
        echo "  6) 230400"
        echo "  7) 250000"
        echo "  8) 500000"
        echo "  9) 921600"
        echo "  10) Custom"
        echo ""
        read -p "Baud rate option: " baud_choice
        
        case $baud_choice in
            1) baud_rate="9600" ;;
            2) baud_rate="19200" ;;
            3) baud_rate="38400" ;;
            4) baud_rate="57600" ;;
            5) baud_rate="115200" ;;
            6) baud_rate="230400" ;;
            7) baud_rate="250000" ;;
            8) baud_rate="500000" ;;
            9) baud_rate="921600" ;;
            10) 
                read -p "Enter custom baud rate: " baud_rate
                if ! [[ "$baud_rate" =~ ^[0-9]+$ ]]; then
                    echo -e "${RED}Invalid baud rate${NC}"
                    return 1
                fi
                ;;
            *)
                baud_rate="115200"
                ;;
        esac
        
        echo ""
        echo -e "${GREEN}Selected: $selected_port @ $baud_rate baud${NC}"
        return 0
    }
    
    # Funkcja pomocnicza: Odczyt surowych danych z portu szeregowego
    read_raw_data() {
        local port="$1"
        local rate="$2"
        local duration="${3:-10}"  # Domyślnie 10 sekund
        
        if ! command -v python3 &> /dev/null; then
            echo -e "${RED}Error: python3 is required for serial communication${NC}"
            return 1
        fi
        
        # Sprawdź czy pyserial jest dostępny
        if ! python3 -c "import serial" 2>/dev/null; then
            echo -e "${YELLOW}Warning: pyserial not installed. Installing...${NC}"
            pip3 install pyserial --quiet 2>/dev/null || {
                echo -e "${RED}Failed to install pyserial. Please run:${NC}"
                echo "  pip3 install pyserial"
                return 1
            }
        fi
        
        print_header "${LANG[MENU_6]} - Raw Data Stream"
        echo "Reading from: $port @ $rate baud"
        echo "Press Ctrl+C to stop"
        echo "Duration: ${duration}s (or until interrupted)"
        echo "================================================================"
        echo ""
        
        # Skrypt Python do odczytu danych szeregowych
        python3 << EOF
import serial
import sys
import time
from datetime import datetime

try:
    ser = serial.Serial(
        port='$port',
        baudrate=$rate,
        timeout=1,
        write_timeout=1
    )
    print(f"Connected to {ser.portstr}")
    print(f"Baud rate: {ser.baudrate}")
    print("-" * 64)
    
    start_time = time.time()
    line_count = 0
    
    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                
                # Analiza podstawowych formatów danych
                if line:
                    line_count += 1
                    
                    # Wykryj format danych
                    if ',' in line and line.replace(',', '').replace('.', '').replace('-', '').isdigit():
                        # Dane CSV/numeryczne
                        values = line.split(',')
                        print(f"[{timestamp}] CSV[{len(values)}]: {line}")
                    elif '=' in line:
                        # Dane w formacie key=value
                        pairs = line.split()
                        print(f"[{timestamp}] KEY-VALUE: {line}")
                    elif line.startswith('{') and line.endswith('}'):
                        # JSON
                        print(f"[{timestamp}] JSON: {line}")
                    elif line.startswith('<') and line.endswith('>'):
                        # XML lub tagowany format
                        print(f"[{timestamp}] TAGGED: {line}")
                    else:
                        # Tekst ogólny
                        print(f"[{timestamp}] TEXT: {line}")
                        
        except KeyboardInterrupt:
            print("\\nInterrupted by user")
            break
        except Exception as e:
            print(f"Error reading: {e}", file=sys.stderr)
            break
            
        # Sprawdź limit czasu
        if time.time() - start_time > $duration:
            print(f"\\nTimeout after {duration}s")
            break
    
    ser.close()
    print(f"Total lines received: {line_count}")
    
except serial.SerialException as e:
    print(f"Serial error: {e}", file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f"Error: {e}", file=sys.stderr)
    sys.exit(1)
EOF
    }
    
    # Funkcja pomocnicza: Ciągłe monitorowanie z wyświetlaniem parsed danych
    start_monitoring() {
        local port="$1"
        local rate="$2"
        
        if ! command -v python3 &> /dev/null; then
            echo -e "${RED}Error: python3 is required${NC}"
            return 1
        fi
        
        print_header "${LANG[MENU_6]} - Live Monitoring"
        echo "Monitoring: $port @ $rate baud"
        echo "Press Ctrl+C to stop"
        echo ""
        
        # Utwórz tymczasowy plik na dane
        local temp_data_file="/tmp/hive_monitor_live_${$}.csv"
        
        # Skrypt Python do monitorowania i parsowania danych
        python3 << EOF
import serial
import sys
import time
import os
from datetime import datetime
from collections import deque

# Konfiguracja
PORT = '$port'
BAUD = $rate
DATA_FILE = '$temp_data_file'
MAX_HISTORY = 100  # Liczba ostatnich odczytów do przechowywania

# Bufory na dane
sensor_history = deque(maxlen=MAX_HISTORY)
last_values = {}

def parse_sensor_data(line):
    """Parsuje różne formaty danych sensorów"""
    data = {}
    
    # Format 1: CSV z nagłówkiem lub bez
    if ',' in line:
        parts = line.strip().split(',')
        # Spróbuj zidentyfikować kolumny
        known_fields = ['temp', 'humidity', 'pressure', 'co2', 'voc', 
                       'weight', 'battery', 'motion', 'lux', 'sound']
        
        for i, val in enumerate(parts):
            try:
                num_val = float(val)
                field_name = known_fields[i] if i < len(known_fields) else f'sensor_{i}'
                data[field_name] = num_val
            except ValueError:
                pass
    
    # Format 2: key=value pairs
    elif '=' in line:
        for pair in line.strip().split():
            if '=' in pair:
                key, val = pair.split('=', 1)
                try:
                    data[key.lower()] = float(val)
                except ValueError:
                    data[key.lower()] = val
    
    # Format 3: JSON
    elif line.strip().startswith('{'):
        import json
        try:
            data = json.loads(line.strip())
        except:
            pass
    
    return data

def display_dashboard(data):
    """Wyświetla dashboard z danymi"""
    os.system('clear')
    
    print("=" * 70)
    print("  HIVE MONITOR - LIVE DASHBOARD")
    print("=" * 70)
    print(f"Port: {PORT} | Baud: {BAUD} | Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("-" * 70)
    
    # Sekcja: Temperatura i Wilgotność
    if 'temp' in data or 'temperature' in data:
        temp = data.get('temp', data.get('temperature', 'N/A'))
        unit = '°C' if isinstance(temp, (int, float)) else ''
        print(f"  Temperature:   {temp:>8}{unit}")
    
    if 'humidity' in data or 'hum' in data:
        hum = data.get('humidity', data.get('hum', 'N/A'))
        unit = '%' if isinstance(hum, (int, float)) else ''
        print(f"  Humidity:      {hum:>8}{unit}")
    
    # Sekcja: Jakość powietrza
    if 'co2' in data:
        co2 = data['co2']
        status = "Good" if co2 < 800 else "Moderate" if co2 < 1200 else "Poor"
        print(f"  CO2:           {co2:>8} ppm ({status})")
    
    if 'voc' in data:
        voc = data['voc']
        print(f"  VOC:           {voc:>8} ppb")
    
    # Sekcja: Waga i Aktywność
    if 'weight' in data:
        weight = data['weight']
        unit = 'kg' if isinstance(weight, (int, float)) else ''
        print(f"  Weight:        {weight:>8}{unit}")
    
    if 'motion' in data or 'activity' in data:
        motion = data.get('motion', data.get('activity', 'N/A'))
        print(f"  Motion:        {motion:>8}")
    
    # Sekcja: Zasilanie
    if 'battery' in data or 'vbat' in data:
        battery = data.get('battery', data.get('vbat', 'N/A'))
        unit = 'V' if isinstance(battery, (int, float)) else ''
        print(f"  Battery:       {battery:>8}{unit}")
    
    # Sekcja: Światło
    if 'lux' in data or 'light' in data:
        lux = data.get('lux', data.get('light', 'N/A'))
        unit = 'lx' if isinstance(lux, (int, float)) else ''
        print(f"  Light:         {lux:>8}{unit}")
    
    # Sekcja: Ciśnienie
    if 'pressure' in data or 'press' in data:
        press = data.get('pressure', data.get('press', 'N/A'))
        unit = 'hPa' if isinstance(press, (int, float)) else ''
        print(f"  Pressure:      {press:>8}{unit}")
    
    # Sekcja: Audio/Dźwięk
    if 'sound' in data or 'audio' in data or 'rms' in data:
        sound = data.get('sound', data.get('audio', data.get('rms', 'N/A')))
        print(f"  Sound Level:   {sound:>8}")
    
    print("-" * 70)
    
    # Ostatnie wartości
    if last_values:
        print("\\n  Last readings:")
        for key, val in list(last_values.items())[:5]:
            print(f"    {key}: {val}")
    
    print("\\n" + "=" * 70)
    print("  Press Ctrl+C to exit")
    print("=" * 70)

try:
    ser = serial.Serial(PORT, BAUD, timeout=0.5)
    print(f"Connected to {ser.portstr}")
    time.sleep(2)  # Czekaj na stabilizację połączenia
    
    # Wyczyść bufor
    ser.reset_input_buffer()
    
    last_display_update = 0
    display_interval = 1  # Aktualizuj dashboard co 1 sekundę
    
    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                
                if line:
                    # Parsuj dane
                    parsed = parse_sensor_data(line)
                    
                    if parsed:
                        # Aktualizuj ostatnie wartości
                        last_values.update(parsed)
                        
                        # Dodaj do historii z timestampem
                        sensor_history.append({
                            'time': datetime.now(),
                            'data': parsed
                        })
                        
                        # Zapisz do pliku CSV
                        with open(DATA_FILE, 'a') as f:
                            timestamp = datetime.now().isoformat()
                            values = [timestamp] + [str(parsed.get(k, '')) for k in sorted(parsed.keys())]
                            f.write(','.join(values) + '\\n')
                        
                        # Aktualizuj wyświetlacz
                        current_time = time.time()
                        if current_time - last_display_update > display_interval:
                            display_dashboard(last_values)
                            last_display_update = current_time
                            
        except KeyboardInterrupt:
            print("\\nStopping monitor...")
            break
        except Exception as e:
            print(f"Read error: {e}", file=sys.stderr)
            time.sleep(1)
    
    ser.close()
    
    # Podsumowanie
    print(f"\\nSession summary:")
    print(f"  Total readings: {len(sensor_history)}")
    print(f"  Data saved to: {DATA_FILE}")
    
    # Pokaż statystyki
    if sensor_history:
        print("\\n  Final sensor values:")
        for key, val in last_values.items():
            print(f"    {key}: {val}")
    
except serial.SerialException as e:
    print(f"Serial connection failed: {e}", file=sys.stderr)
    sys.exit(1)
except Exception as e:
    print(f"Error: {e}", file=sys.stderr)
    sys.exit(1)
finally:
    # Cleanup
    if os.path.exists(DATA_FILE):
        print(f"\\nData file available at: {DATA_FILE}")
EOF
    }
    
    # Funkcja pomocnicza: Symulacja danych (do testów bez sprzętu)
    simulate_data() {
        print_header "${LANG[MENU_6]} - Simulation Mode"
        echo "No physical device connected. Running simulation..."
        echo ""
        
        python3 << 'EOF'
import random
import time
from datetime import datetime
import os

print("SIMULATED HIVE SENSOR DATA")
print("=" * 70)

base_temp = 35.0
base_humidity = 55.0
base_weight = 45.0
base_co2 = 600

start_time = time.time()

try:
    while True:
        elapsed = time.time() - start_time
        
        # Symuluj zmiany parametrów
        temp = base_temp + random.gauss(0, 2) + 3 * (elapsed / 3600)
        humidity = base_humidity + random.gauss(0, 5) - 2 * (elapsed / 3600)
        weight = base_weight + random.gauss(0, 0.1)
        co2 = base_co2 + random.gauss(0, 100) + 50 * (elapsed / 3600)
        voc = 200 + random.gauss(0, 50)
        battery = 4.2 - 0.001 * elapsed + random.gauss(0, 0.05)
        motion = random.choice([0, 1, 1, 1, 2])
        lux = max(0, 500 + 300 * (elapsed % 86400) / 43200) + random.gauss(0, 50)
        pressure = 1013.25 + random.gauss(0, 5)
        sound = 45 + random.gauss(0, 10)
        
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        # Wyświetl w formacie key=value
        print(f"{timestamp} | "
              f"temp={temp:.1f}C | "
              f"hum={humidity:.1f}% | "
              f"weight={weight:.2f}kg | "
              f"co2={co2:.0f}ppm | "
              f"voc={voc:.0f}ppb | "
              f"bat={battery:.2f}V | "
              f"motion={motion} | "
              f"lux={lux:.0f} | "
              f"press={pressure:.1f}hPa | "
              f"sound={sound:.1f}dB")
        
        time.sleep(2)
        
except KeyboardInterrupt:
    print("\\nSimulation stopped.")
EOF
    }
    
    # ========================================================================
    # GŁÓWNE MENU OPCJI 6
    # ========================================================================
    
    while true; do
        clear_screen
        echo -e "${GREEN}>>> ${LANG[MENU_6]}${NC}"
        echo ""
        echo "Current selection: ${CYAN}${selected_port:-Not selected}${NC}"
        echo "Baud rate: ${CYAN}${baud_rate}${NC}"
        echo ""
        echo "  1) Auto-detect microcontroller"
        echo "  2) Select port manually"
        echo "  3) View raw data stream (10s sample)"
        echo "  4) Start live monitoring dashboard"
        echo "  5) Run simulation mode (no hardware required)"
        echo "  6) Configure serial settings"
        echo "  7) Save captured data to file"
        echo "  8) Check system serial permissions"
        echo ""
        echo "  0) Back to main menu"
        echo ""
        
        read -p "Select option: " choice
        
        case $choice in
            1)
                if auto_detect_microcontroller; then
                    echo ""
                    read -p "Start monitoring now? (y/N): " start_now
                    if [[ $start_now =~ ^[Yy]$ ]]; then
                        start_monitoring "$selected_port" "$baud_rate"
                    fi
                fi
                wait_for_key
                ;;
            2)
                if manual_select_port; then
                    echo ""
                    read -p "Test connection now? (y/N): " test_conn
                    if [[ $test_conn =~ ^[Yy]$ ]]; then
                        read_raw_data "$selected_port" "$baud_rate" 5
                    fi
                fi
                wait_for_key
                ;;
            3)
                if [[ -z "$selected_port" ]]; then
                    echo -e "${YELLOW}No port selected. Please select a port first.${NC}"
                    wait_for_key
                    continue
                fi
                read_raw_data "$selected_port" "$baud_rate" 10
                wait_for_key
                ;;
            4)
                if [[ -z "$selected_port" ]]; then
                    echo -e "${YELLOW}No port selected. Please select a port first.${NC}"
                    wait_for_key
                    continue
                fi
                start_monitoring "$selected_port" "$baud_rate"
                wait_for_key
                ;;
            5)
                simulate_data
                wait_for_key
                ;;
            6)
                echo "Serial Settings Configuration"
                echo "=============================="
                echo ""
                read -p "Enter new baud rate (current: $baud_rate): " new_baud
                if [[ -n "$new_baud" ]] && [[ "$new_baud" =~ ^[0-9]+$ ]]; then
                    baud_rate="$new_baud"
                    echo -e "${GREEN}Baud rate updated to $baud_rate${NC}"
                fi
                echo ""
                echo "Additional settings (flow control, parity, etc.)"
                echo "can be configured in the monitoring function."
                wait_for_key
                ;;
            7)
                local data_files=(/tmp/hive_monitor_live_*.csv)
                if [[ -e "${data_files[0]}" ]]; then
                    echo "Available data files:"
                    ls -lh "${data_files[@]}" 2>/dev/null
                    echo ""
                    read -p "Export latest file to current directory? (y/N): " export_choice
                    if [[ $export_choice =~ ^[Yy]$ ]]; then
                        cp "${data_files[-1]}" "./hive_data_$(date +%Y%m%d_%H%M%S).csv"
                        echo -e "${GREEN}File exported successfully${NC}"
                    fi
                else
                    echo -e "${YELLOW}No data files found${NC}"
                fi
                wait_for_key
                ;;
            8)
                echo "Checking serial permissions..."
                echo ""
                
                # Sprawdź czy użytkownik jest w grupie dialout/uucp
                if groups | grep -qE "(dialout|uucp)"; then
                    echo -e "${GREEN}✓ User has serial port permissions${NC}"
                else
                    echo -e "${YELLOW}✗ User lacks serial port permissions${NC}"
                    echo ""
                    echo "To fix this, run:"
                    echo "  sudo usermod -a -G dialout \$USER"
                    echo "Then log out and log back in."
                fi
                
                echo ""
                echo "Available serial devices:"
                ls -la /dev/ttyUSB* /dev/ttyACM* /dev/serial* 2>/dev/null || echo "  None found"
                
                echo ""
                echo "USB devices:"
                lsusb 2>/dev/null | grep -iE "(serial|arduino|pico|ftdi|ch340|cp210)" || echo "  No USB serial devices detected"
                
                wait_for_key
                ;;
            0)
                break
                ;;
            *)
                echo -e "${RED}Invalid option${NC}"
                wait_for_key
                ;;
        esac
    done
    
    log_message "INFO" "Live microcontroller data session ended"
}

# ============================================================================
# HISTORICAL DATA BROWSER FUNCTIONS
# ============================================================================

# Get database path from configuration
get_database_path() {
    local config_file="${CONFIG_DIR}/database.conf"
    local db_path
    db_path=$(get_config_value "$config_file" "DB_PATH" "${CONFIG_DIR}/data/hive_monitor.db")
    echo "$db_path"
}

# Get CSV directory from configuration
get_csv_directory() {
    local csv_dir="${CONFIG_DIR}/data/csv"
    mkdir -p "$csv_dir"
    echo "$csv_dir"
}

# Check if database exists and has data
check_database_exists() {
    local db_path="$1"
    if [[ -f "$db_path" ]]; then
        # Check if sqlite3 is available
        if command -v sqlite3 &> /dev/null; then
            local count
            count=$(sqlite3 "$db_path" "SELECT COUNT(*) FROM hive_data;" 2>/dev/null || echo "0")
            if [[ "$count" -gt 0 ]]; then
                return 0
            fi
        fi
    fi
    return 1
}

# Check for CSV files
check_csv_files() {
    local csv_dir="$1"
    if [[ -d "$csv_dir" ]] && [[ -n "$(ls -A "$csv_dir"/*.csv 2>/dev/null)" ]]; then
        return 0
    fi
    # Also check collector data file
    if [[ -f "/tmp/apiary_data.csv" ]]; then
        return 0
    fi
    return 1
}

# Display data from SQLite database by date range
view_data_by_date_sqlite() {
    local db_path="$1"
    
    echo ""
    echo "Enter date range (YYYY-MM-DD format):"
    read -p "Start date [default: 7 days ago]: " start_date
    read -p "End date [default: today]: " end_date
    
    # Set defaults
    if [[ -z "$start_date" ]]; then
        start_date=$(date -d "7 days ago" '+%Y-%m-%d' 2>/dev/null || date -v-7d '+%Y-%m-%d' 2>/dev/null || echo "")
        if [[ -z "$start_date" ]]; then
            start_date="2024-01-01"
        fi
    fi
    if [[ -z "$end_date" ]]; then
        end_date=$(date '+%Y-%m-%d')
    fi
    
    echo ""
    echo "Fetching data from $start_date to $end_date..."
    echo ""
    
    # Query database with formatted output
    sqlite3 -header -column "$db_path" <<EOF
SELECT 
    datetime(timestamp, 'localtime') as Time,
    temperature,
    humidity,
    pressure,
    light_intensity,
    sound_level,
    air_quality,
    battery_voltage
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date'
ORDER BY timestamp DESC
LIMIT 100;
EOF
    
    echo ""
    echo "Showing up to 100 most recent records in range."
}

# Display data from CSV files
view_data_from_csv() {
    local csv_dir="$1"
    
    echo ""
    echo "Available CSV files:"
    echo "--------------------"
    
    local csv_files=()
    while IFS= read -r -d '' file; do
        csv_files+=("$file")
    done < <(find "$csv_dir" -name "*.csv" -type f -print0 2>/dev/null)
    
    # Also include collector data file if exists
    if [[ -f "/tmp/apiary_data.csv" ]]; then
        csv_files+=("/tmp/apiary_data.csv")
    fi
    
    if [[ ${#csv_files[@]} -eq 0 ]]; then
        echo "No CSV files found."
        return 1
    fi
    
    # List files with numbers
    for i in "${!csv_files[@]}"; do
        echo "$((i+1))) ${csv_files[$i]}"
    done
    echo "0) Back"
    echo ""
    
    read -p "Select file to view (0-${#csv_files[@]}): " file_choice
    
    if [[ "$file_choice" -eq 0 ]]; then
        return 0
    fi
    
    if [[ "$file_choice" -ge 1 && "$file_choice" -le "${#csv_files[@]}" ]]; then
        local selected_file="${csv_files[$((file_choice-1))]}"
        echo ""
        echo "Contents of: $selected_file"
        echo "----------------------------------------"
        
        # Show first 50 lines
        head -n 50 "$selected_file"
        
        local line_count
        line_count=$(wc -l < "$selected_file")
        if [[ "$line_count" -gt 50 ]]; then
            echo ""
            echo "... (showing first 50 of $line_count lines)"
        fi
    else
        echo "Invalid selection."
    fi
}

# Search for specific metrics
search_metrics() {
    local db_path="$1"
    
    echo ""
    echo "Search Metrics"
    echo "=============="
    echo "Available metrics:"
    echo "1) Temperature"
    echo "2) Humidity"
    echo "3) Pressure"
    echo "4) Light Intensity"
    echo "5) Sound Level"
    echo "6) Air Quality"
    echo "7) Battery Voltage"
    echo "0) Back"
    echo ""
    
    read -p "Select metric to search (0-7): " metric_choice
    
    local column_name=""
    local unit=""
    case $metric_choice in
        1) column_name="temperature"; unit="°C" ;;
        2) column_name="humidity"; unit="%" ;;
        3) column_name="pressure"; unit="hPa" ;;
        4) column_name="light_intensity"; unit="lux" ;;
        5) column_name="sound_level"; unit="dB" ;;
        6) column_name="air_quality"; unit="AQI" ;;
        7) column_name="battery_voltage"; unit="V" ;;
        0) return 0 ;;
        *) echo "Invalid choice"; return 1 ;;
    esac
    
    echo ""
    read -p "Enter minimum value [$unit]: " min_val
    read -p "Enter maximum value [$unit]: " max_val
    
    if [[ -z "$min_val" ]]; then
        min_val="0"
    fi
    if [[ -z "$max_val" ]]; then
        max_val="9999"
    fi
    
    echo ""
    echo "Searching for $column_name between $min_val and $max_val $unit..."
    echo ""
    
    sqlite3 -header -column "$db_path" <<EOF
SELECT 
    datetime(timestamp, 'localtime') as Time,
    $column_name as Value
FROM hive_data 
WHERE $column_name BETWEEN $min_val AND $max_val
ORDER BY timestamp DESC
LIMIT 50;
EOF
    
    echo ""
    echo "Showing up to 50 matching records."
}

# Export data to CSV or JSON
export_data() {
    local db_path="$1"
    local export_dir="${CONFIG_DIR}/exports"
    mkdir -p "$export_dir"
    
    echo ""
    echo "Export Options"
    echo "=============="
    echo "1) Export to CSV"
    echo "2) Export to JSON"
    echo "0) Back"
    echo ""
    
    read -p "Select export format (0-2): " format_choice
    
    case $format_choice in
        1)
            local filename="hive_export_$(date '+%Y%m%d_%H%M%S').csv"
            local filepath="${export_dir}/${filename}"
            
            echo ""
            read -p "Enter start date (YYYY-MM-DD) [default: all data]: " start_date
            read -p "Enter end date (YYYY-MM-DD) [default: today]: " end_date
            
            if [[ -z "$end_date" ]]; then
                end_date=$(date '+%Y-%m-%d')
            fi
            
            sqlite3 -header -csv "$db_path" "
                SELECT * FROM hive_data 
                WHERE date(timestamp) BETWEEN '${start_date:-2000-01-01}' AND '$end_date'
                ORDER BY timestamp;
            " > "$filepath"
            
            if [[ $? -eq 0 ]]; then
                echo -e "${GREEN}Data exported successfully to: $filepath${NC}"
                echo "File size: $(du -h "$filepath" | cut -f1)"
            else
                echo -e "${RED}Export failed${NC}"
            fi
            ;;
        2)
            local filename="hive_export_$(date '+%Y%m%d_%H%M%S').json"
            local filepath="${export_dir}/${filename}"
            
            echo ""
            read -p "Enter start date (YYYY-MM-DD) [default: all data]: " start_date
            read -p "Enter end date (YYYY-MM-DD) [default: today]: " end_date
            
            if [[ -z "$end_date" ]]; then
                end_date=$(date '+%Y-%m-%d')
            fi
            
            # Create JSON manually since sqlite3 doesn't have native JSON output in older versions
            echo "[" > "$filepath"
            sqlite3 -separator ',' "$db_path" "
                SELECT 
                    json_object(
                        'timestamp', datetime(timestamp, 'localtime'),
                        'temperature', temperature,
                        'humidity', humidity,
                        'pressure', pressure,
                        'light_intensity', light_intensity,
                        'sound_level', sound_level,
                        'air_quality', air_quality,
                        'battery_voltage', battery_voltage
                    )
                FROM hive_data 
                WHERE date(timestamp) BETWEEN '${start_date:-2000-01-01}' AND '$end_date'
                ORDER BY timestamp;
            " | sed '$ ! s/$/,/' >> "$filepath"
            echo "]" >> "$filepath"
            
            if [[ $? -eq 0 ]]; then
                echo -e "${GREEN}Data exported successfully to: $filepath${NC}"
                echo "File size: $(du -h "$filepath" | cut -f1)"
            else
                echo -e "${RED}Export failed${NC}"
            fi
            ;;
        0)
            return 0
            ;;
        *)
            echo "Invalid choice"
            return 1
            ;;
    esac
}

# Generate summary report
generate_report() {
    local db_path="$1"
    local report_dir="${CONFIG_DIR}/reports"
    mkdir -p "$report_dir"
    
    echo ""
    echo "Generate Report"
    echo "==============="
    echo "1) Daily Summary"
    echo "2) Weekly Summary"
    echo "3) Monthly Summary"
    echo "4) Custom Range"
    echo "0) Back"
    echo ""
    
    read -p "Select report type (0-4): " report_type
    
    local start_date=""
    local end_date=""
    
    case $report_type in
        1)
            start_date=$(date '+%Y-%m-%d')
            end_date=$(date '+%Y-%m-%d')
            ;;
        2)
            start_date=$(date -d "7 days ago" '+%Y-%m-%d' 2>/dev/null || date -v-7d '+%Y-%m-%d' 2>/dev/null || echo "2024-01-01")
            end_date=$(date '+%Y-%m-%d')
            ;;
        3)
            start_date=$(date -d "30 days ago" '+%Y-%m-%d' 2>/dev/null || date -v-30d '+%Y-%m-%d' 2>/dev/null || echo "2024-01-01")
            end_date=$(date '+%Y-%m-%d')
            ;;
        4)
            read -p "Start date (YYYY-MM-DD): " start_date
            read -p "End date (YYYY-MM-DD): " end_date
            [[ -z "$end_date" ]] && end_date=$(date '+%Y-%m-%d')
            ;;
        0)
            return 0
            ;;
        *)
            echo "Invalid choice"
            return 1
            ;;
    esac
    
    local filename="report_$(date '+%Y%m%d_%H%M%S').txt"
    local filepath="${report_dir}/${filename}"
    
    {
        echo "========================================"
        echo "     HIVE MONITOR - DATA REPORT"
        echo "========================================"
        echo ""
        echo "Report Period: $start_date to $end_date"
        echo "Generated: $(date '+%Y-%m-%d %H:%M:%S')"
        echo ""
        echo "----------------------------------------"
        echo "SUMMARY STATISTICS"
        echo "----------------------------------------"
        
        sqlite3 "$db_path" <<EOF
SELECT 
    'Temperature (°C)' as Metric,
    printf('Min: %.2f | Max: %.2f | Avg: %.2f', MIN(temperature), MAX(temperature), AVG(temperature)) as Stats
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date'
UNION ALL
SELECT 
    'Humidity (%)',
    printf('Min: %.2f | Max: %.2f | Avg: %.2f', MIN(humidity), MAX(humidity), AVG(humidity))
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date'
UNION ALL
SELECT 
    'Pressure (hPa)',
    printf('Min: %.2f | Max: %.2f | Avg: %.2f', MIN(pressure), MAX(pressure), AVG(pressure))
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date'
UNION ALL
SELECT 
    'Light Intensity (lux)',
    printf('Min: %.2f | Max: %.2f | Avg: %.2f', MIN(light_intensity), MAX(light_intensity), AVG(light_intensity))
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date'
UNION ALL
SELECT 
    'Sound Level (dB)',
    printf('Min: %.2f | Max: %.2f | Avg: %.2f', MIN(sound_level), MAX(sound_level), AVG(sound_level))
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date'
UNION ALL
SELECT 
    'Battery Voltage (V)',
    printf('Min: %.2f | Max: %.2f | Avg: %.2f', MIN(battery_voltage), MAX(battery_voltage), AVG(battery_voltage))
FROM hive_data 
WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date';
EOF
        
        echo ""
        echo "----------------------------------------"
        echo "DATA POINTS"
        echo "----------------------------------------"
        local count
        count=$(sqlite3 "$db_path" "SELECT COUNT(*) FROM hive_data WHERE date(timestamp) BETWEEN '$start_date' AND '$end_date';")
        echo "Total records: $count"
        echo ""
        echo "========================================"
        
    } > "$filepath"
    
    echo -e "${GREEN}Report generated: $filepath${NC}"
    echo ""
    cat "$filepath"
}

# Text-based data visualization (simple ASCII chart)
visualize_data() {
    local db_path="$1"
    
    echo ""
    echo "Data Visualization"
    echo "=================="
    echo "Select metric to visualize:"
    echo "1) Temperature"
    echo "2) Humidity"
    echo "3) Pressure"
    echo "0) Back"
    echo ""
    
    read -p "Select metric (0-3): " viz_choice
    
    local column_name=""
    case $viz_choice in
        1) column_name="temperature" ;;
        2) column_name="humidity" ;;
        3) column_name="pressure" ;;
        0) return 0 ;;
        *) echo "Invalid choice"; return 1 ;;
    esac
    
    echo ""
    echo "Last 20 readings of $column_name:"
    echo ""
    
    # Get last 20 values and create simple bar chart
    local values
    values=$(sqlite3 "$db_path" "
        SELECT $column_name 
        FROM hive_data 
        ORDER BY timestamp DESC 
        LIMIT 20;
    ")
    
    if [[ -z "$values" ]]; then
        echo "No data available."
        return 1
    fi
    
    # Find min and max for scaling
    local min_val max_val
    min_val=$(echo "$values" | sort -n | head -1)
    max_val=$(echo "$values" | sort -n | tail -1)
    
    echo "Range: $min_val - $max_val"
    echo ""
    
    # Simple horizontal bar chart
    local count=0
    while IFS= read -r val; do
        if [[ -n "$val" ]]; then
            # Calculate bar length (scale to 0-40 characters)
            local range=$((max_val - min_val))
            if [[ "$range" -eq 0 ]]; then
                range=1
            fi
            local bar_len=$(( (val - min_val) * 40 / range ))
            local bar=""
            for ((i=0; i<bar_len; i++)); do
                bar+="█"
            done
            printf "%3d | %s %.2f\n" "$count" "$bar" "$val"
            ((count++))
        fi
    done <<< "$values"
    
    echo ""
    echo "Each █ represents relative value within range."
}

# Main historical data browser function
option_historical_data() {
    print_header "${LANG[MENU_7]}"
    
    local db_path
    db_path=$(get_database_path)
    local csv_dir
    csv_dir=$(get_csv_directory)
    
    echo "Historical Data Browser"
    echo "======================="
    echo ""
    echo "Database: $db_path"
    echo "CSV Directory: $csv_dir"
    echo ""
    
    # Check data sources
    local has_db=false
    local has_csv=false
    
    if check_database_exists "$db_path"; then
        has_db=true
        echo -e "${GREEN}✓ Database found with data${NC}"
    else
        echo -e "${YELLOW}✗ Database not found or empty${NC}"
    fi
    
    if check_csv_files "$csv_dir"; then
        has_csv=true
        echo -e "${GREEN}✓ CSV files found${NC}"
    else
        echo -e "${YELLOW}✗ No CSV files found${NC}"
    fi
    
    echo ""
    
    if ! $has_db && ! $has_csv; then
        echo -e "${RED}No historical data available.${NC}"
        echo ""
        echo "To collect data:"
        echo "1. Ensure the microcontroller data collector is running"
        echo "2. Check that sensors are properly connected"
        echo "3. Verify database configuration in option 5"
        log_message "WARN" "Historical data browser accessed but no data available"
        wait_for_key
        return 0
    fi
    
    echo "Main Menu:"
    echo "----------"
    echo "1) View data by date range"
    echo "2) Search specific metrics"
    echo "3) Export data (CSV/JSON)"
    echo "4) Generate reports"
    echo "5) Data visualization (text-based charts)"
    echo "6) View raw CSV files"
    echo "0) Back to main menu"
    echo ""
    
    read -p "Select option (0-6): " hist_choice
    
    case $hist_choice in
        1)
            if $has_db; then
                view_data_by_date_sqlite "$db_path"
            else
                view_data_from_csv "$csv_dir"
            fi
            ;;
        2)
            if $has_db; then
                search_metrics "$db_path"
            else
                echo -e "${RED}Search requires database. Please configure database first.${NC}"
            fi
            ;;
        3)
            if $has_db; then
                export_data "$db_path"
            else
                echo -e "${RED}Export requires database. Please configure database first.${NC}"
            fi
            ;;
        4)
            if $has_db; then
                generate_report "$db_path"
            else
                echo -e "${RED}Reports require database. Please configure database first.${NC}"
            fi
            ;;
        5)
            if $has_db; then
                visualize_data "$db_path"
            else
                echo -e "${RED}Visualization requires database. Please configure database first.${NC}"
            fi
            ;;
        6)
            view_data_from_csv "$csv_dir"
            ;;
        0)
            log_message "INFO" "Historical data browser exited"
            return 0
            ;;
        *)
            echo -e "${RED}Invalid option${NC}"
            ;;
    esac
    
    log_message "INFO" "Historical data browser operation completed"
    wait_for_key
}

option_system_status() {
    print_header "${LANG[MENU_8]}"
    echo "System Status Check"
    echo "==================="
    echo "[STUB] System status checking logic will be implemented here."
    echo ""
    echo "Checks to be implemented:"
    echo "- Service status"
    echo "- Disk space"
    echo "- Memory usage"
    echo "- Network connectivity"
    echo "- Configuration validity"
    log_message "INFO" "System status check requested"
    wait_for_key
}

option_reset_defaults() {
    print_header "${LANG[MENU_9]}"
    echo "Reset to Default Settings"
    echo "========================="
    echo -e "${RED}WARNING: This will reset all configurations to defaults!${NC}"
    echo ""
    read -p "Are you sure? (y/N): " confirm
    
    if [[ $confirm =~ ^[Yy]$ ]]; then
        echo "[STUB] Reset to defaults logic will be implemented here."
        log_message "WARN" "Reset to defaults initiated"
    else
        echo "Operation cancelled."
        log_message "INFO" "Reset to defaults cancelled"
    fi
    
    wait_for_key
}

# ============================================================================
# MAIN MENU
# ============================================================================

show_main_menu() {
    clear_screen
    echo -e "${GREEN}${LANG[WELCOME]}${NC}"
    echo ""
    echo "Main Menu:"
    echo "----------"
    echo "1) ${LANG[MENU_1]}"
    echo "2) ${LANG[MENU_2]}"
    echo "3) ${LANG[MENU_3]}"
    echo "4) ${LANG[MENU_4]}"
    echo "5) ${LANG[MENU_5]}"
    echo "6) ${LANG[MENU_6]}"
    echo "7) ${LANG[MENU_7]}"
    echo "8) ${LANG[MENU_8]}"
    echo "9) ${LANG[MENU_9]}"
    echo "0) ${LANG[MENU_0]}"
    echo ""
}

main() {
    # Initialize directories
    mkdir -p "$CONFIG_DIR" "$BACKUP_DIR" 2>/dev/null || true
    touch "$LOG_FILE" 2>/dev/null || true
    
    log_message "INFO" "Application started"
    
    while true; do
        show_main_menu
        read -p "Select option (0-9): " choice
        
        case $choice in
            1) option_language ;;
            2) option_install_dependencies ;;
            3) option_install_software ;;
            4) option_backup_config ;;
            5) option_configure_app ;;
            6) option_live_data ;;
            7) option_historical_data ;;
            8) option_system_status ;;
            9) option_reset_defaults ;;
            0)
                echo -e "${GREEN}Exiting... Goodbye!${NC}"
                log_message "INFO" "Application exited"
                exit 0
                ;;
            *)
                echo -e "${RED}${LANG[INVALID]}${NC}"
                wait_for_key
                ;;
        esac
    done
}

# ============================================================================
# ENTRY POINT
# ============================================================================

main "$@"
