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
    echo "Live Microcontroller Data Reader"
    echo "================================="
    echo "Available ports: /dev/ttyUSB*, /dev/ttyACM*"
    echo ""
    echo "1) Auto-detect microcontroller"
    echo "2) Select port manually"
    echo "3) View raw data stream"
    echo "4) Start/Stop monitoring"
    echo "0) Back to main menu"
    echo ""
    echo "[STUB] Live data reading logic will be implemented here."
    log_message "INFO" "Live microcontroller data access requested"
    wait_for_key
}

option_historical_data() {
    print_header "${LANG[MENU_7]}"
    echo "Historical Data Browser"
    echo "======================="
    echo "Data location: ${CONFIG_DIR}/data"
    echo ""
    echo "1) View by date range"
    echo "2) Search specific metrics"
    echo "3) Export data (CSV/JSON)"
    echo "4) Generate reports"
    echo "5) Data visualization (text-based)"
    echo "0) Back to main menu"
    echo ""
    echo "[STUB] Historical data browsing logic will be implemented here."
    log_message "INFO" "Historical data browser requested"
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
