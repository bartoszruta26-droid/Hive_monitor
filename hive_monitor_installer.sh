#!/bin/bash

# Hive Monitor Installer & Configurator
# Shell script with TUI menu system

# Strict mode for better error handling and safety
set -euo pipefail

# ============================================================================
# GLOBAL VARIABLES
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_DIR="${HOME}/.hive_monitor"
BACKUP_DIR="${CONFIG_DIR}/backups"
LOG_FILE="${CONFIG_DIR}/installer.log"
CURRENT_LANG="en"
INSTALL_FAILED=0

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
    [MENU_10]="Install Apiary Collector (Full)"
    [MENU_0]="Exit"
    [SELECTED]="Selected language: English"
    [PRESS_KEY]="Press any key to continue..."
    [INVALID]="Invalid option"
    [WELCOME]="Welcome to Hive Monitor Setup"
    [COMPILE_CPP]="Compile C++ Components"
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
    [MENU_10]="Zainstaluj Apiary Collector (Pełny)"
    [MENU_0]="Wyjście"
    [SELECTED]="Wybrany język: Polski"
    [PRESS_KEY]="Naciśnij dowolny klawisz, aby kontynuować..."
    [INVALID]="Nieprawidłowa opcja"
    [WELCOME]="Witaj w konfiguracji Hive Monitor"
    [COMPILE_CPP]="Kompiluj komponenty C++"
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
    echo "Dependencies: git, curl, wget, build-essential, sqlite3, etc."
    echo ""
    
    # Check if running as root or with sudo
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}Note: You may be prompted for sudo password${NC}"
    fi
    
    # Helper function to run commands with proper privilege handling and error checking
    run_cmd() {
        if [[ $EUID -eq 0 ]]; then
            "$@"
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
            INSTALL_FAILED=1
            return 1
        }
        run_cmd apt-get install -y git curl wget build-essential make g++ sqlite3 libsqlite3-dev || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via apt-get"
            INSTALL_FAILED=1
            return 1
        }
    elif command -v yum &> /dev/null; then
        echo "Detected RHEL/CentOS package manager (yum)"
        run_cmd yum install -y git curl wget gcc gcc-c++ make sqlite sqlite-devel || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via yum"
            INSTALL_FAILED=1
            return 1
        }
    elif command -v dnf &> /dev/null; then
        echo "Detected Fedora package manager (dnf)"
        run_cmd dnf install -y git curl wget gcc gcc-c++ make sqlite sqlite-devel || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via dnf"
            INSTALL_FAILED=1
            return 1
        }
    elif command -v pacman &> /dev/null; then
        echo "Detected Arch Linux package manager (pacman)"
        run_cmd pacman -Sy --noconfirm git curl wget base-devel sqlite || {
            echo -e "${RED}Failed to install dependencies${NC}"
            log_message "ERROR" "Failed to install dependencies via pacman"
            INSTALL_FAILED=1
            return 1
        }
    else
        echo -e "${RED}Error: Unsupported package manager. Please install dependencies manually.${NC}"
        echo "Required packages: git, curl, wget, build-essential/make/g++, sqlite3"
        log_message "ERROR" "Unsupported package manager"
        INSTALL_FAILED=1
        return 1
    fi
    
    echo ""
    echo -e "${GREEN}Dependencies installed successfully!${NC}"
    log_message "INFO" "Dependencies installation completed (including SQLite)"
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
                cd "$INSTALL_DIR" || { echo -e "${RED}Failed to change directory${NC}"; return 1; }
                
                # Check current branch and pull from correct branch
                current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null) || {
                    echo -e "${RED}Failed to determine git branch${NC}"
                    log_message "ERROR" "Failed to determine git branch"
                    wait_for_key
                    return 1
                }
                
                echo "Current branch: $current_branch"
                if ! git pull origin "$current_branch" 2>&1; then
                    echo -e "${RED}Failed to update repository from branch '$current_branch'${NC}"
                    log_message "ERROR" "Failed to update repository from branch '$current_branch'"
                    wait_for_key
                    return 1
                fi
                echo -e "${GREEN}Update completed!${NC}"
                log_message "INFO" "Software updated from GitHub (branch: $current_branch)"
                
                # Recompile C++ components after update if source exists
                if [[ -f "$INSTALL_DIR/src/rpi_tui/Makefile" ]]; then
                    echo ""
                    echo "C++ source files detected. Would you like to recompile after update?"
                    read -p "Recompile C++ components? (y/N): " recompile_choice
                    if [[ $recompile_choice =~ ^[Yy]$ ]]; then
                        echo ""
                        echo "Checking for required build tools..."
                        if ! command -v make &> /dev/null; then
                            echo -e "${RED}Error: 'make' is not installed.${NC}"
                            echo "Please run 'Install Dependencies' first."
                            log_message "ERROR" "make not found during recompilation"
                        elif ! command -v g++ &> /dev/null; then
                            echo -e "${RED}Error: 'g++' is not installed.${NC}"
                            echo "Please run 'Install Dependencies' first."
                            log_message "ERROR" "g++ not found during recompilation"
                        else
                            echo "Build tools found. Recompiling..."
                            cd "$INSTALL_DIR/src/rpi_tui" || { 
                                echo -e "${RED}Failed to change to rpi_tui directory${NC}"
                                log_message "ERROR" "Failed to cd to src/rpi_tui"
                                return 1
                            }
                            
                            echo "Cleaning previous builds..."
                            make clean 2>/dev/null || true
                            
                            echo "Compiling C++ components..."
                            if make all; then
                                echo ""
                                echo -e "${GREEN}Recompilation completed successfully!${NC}"
                                echo "Compiled binaries:"
                                ls -lh apiary_collector apiary_logger_test 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
                                
                                echo ""
                                echo "Would you like to reinstall the compiled binaries to system paths?"
                                read -p "Reinstall to system? (y/N): " reinstall_system
                                if [[ $reinstall_system =~ ^[Yy]$ ]]; then
                                    if [[ $EUID -eq 0 ]]; then
                                        make install || {
                                            echo -e "${RED}Failed to install binaries (running as root)${NC}"
                                            log_message "ERROR" "make install failed (root)"
                                        }
                                    else
                                        sudo make install || {
                                            echo -e "${RED}Failed to install binaries (sudo)${NC}"
                                            log_message "ERROR" "sudo make install failed"
                                        }
                                    fi
                                    if [[ $? -eq 0 ]]; then
                                        echo -e "${GREEN}Binaries reinstalled successfully!${NC}"
                                        log_message "INFO" "C++ binaries reinstalled to system paths after update"
                                    else
                                        echo -e "${YELLOW}Installation had issues but compilation succeeded${NC}"
                                    fi
                                fi
                            else
                                echo -e "${RED}Recompilation failed. Check error messages above.${NC}"
                                log_message "ERROR" "C++ recompilation failed after update"
                                cd - > /dev/null || true
                                return 1
                            fi
                            cd - > /dev/null
                        fi
                    fi
                fi
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
        
        # Compile C++ components if Makefile exists
        if [[ -f "$INSTALL_DIR/src/rpi_tui/Makefile" ]]; then
            echo ""
            echo "C++ source files detected. Would you like to compile the collector and logger?"
            read -p "Compile C++ components? (y/N): " compile_choice
            if [[ $compile_choice =~ ^[Yy]$ ]]; then
                echo ""
                echo "Checking for required build tools..."
                if ! command -v make &> /dev/null; then
                    echo -e "${RED}Error: 'make' is not installed.${NC}"
                    echo "Please run 'Install Dependencies' first to install build-essential."
                    log_message "ERROR" "make not found during C++ compilation"
                elif ! command -v g++ &> /dev/null; then
                    echo -e "${RED}Error: 'g++' is not installed.${NC}"
                    echo "Please run 'Install Dependencies' first to install build-essential."
                    log_message "ERROR" "g++ not found during C++ compilation"
                else
                    echo "Build tools found. Compiling..."
                    cd "$INSTALL_DIR/src/rpi_tui"
                    
                    # Clean previous builds
                    echo "Cleaning previous builds..."
                    make clean 2>/dev/null || true
                    
                    # Build all components
                    echo "Compiling C++ components (this may take a few minutes)..."
                    if make all; then
                        echo ""
                        echo -e "${GREEN}C++ components compiled successfully!${NC}"
                        echo "Compiled binaries:"
                        ls -lh apiary_collector apiary_logger_test 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
                        
                        # Optional: Install to system
                        echo ""
                        echo "Would you like to install the compiled binaries to system paths?"
                        echo "(This requires sudo and will copy binaries to /usr/local/bin)"
                        read -p "Install to system? (y/N): " install_system
                        if [[ $install_system =~ ^[Yy]$ ]]; then
                            if [[ $EUID -eq 0 ]]; then
                                make install
                            else
                                sudo make install
                            fi
                            if [[ $? -eq 0 ]]; then
                                echo -e "${GREEN}Binaries installed successfully!${NC}"
                                echo "You can now run:"
                                echo "  apiary-collector  - Main data collector"
                                echo "  apiary-logger     - Logger test utility"
                                echo "  apiary-tui        - TUI interface"
                                log_message "INFO" "C++ binaries installed to system paths"
                                
                                # Setup database after installation
                                echo ""
                                echo "Setting up SQLite database..."
                                setup_database
                            else
                                echo -e "${RED}Failed to install binaries${NC}"
                                log_message "ERROR" "Failed to install C++ binaries"
                            fi
                        else
                            echo "Binaries available in: $INSTALL_DIR/src/rpi_tui/"
                            echo "To run manually:"
                            echo "  cd $INSTALL_DIR/src/rpi_tui && ./apiary_collector"
                        fi
                    else
                        echo -e "${RED}Compilation failed. Check error messages above.${NC}"
                        log_message "ERROR" "C++ compilation failed"
                    fi
                    cd - > /dev/null
                fi
            else
                echo "Skipping C++ compilation. You can compile later by running:"
                echo "  cd $INSTALL_DIR/src/rpi_tui && make all"
                log_message "INFO" "User skipped C++ compilation"
            fi
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
        # Initialize database file with proper permissions
        setup_database
    fi
    
    log_message "INFO" "Database configured: $DB_TYPE"
    wait_for_key
}

# ============================================================================
# SETUP DATABASE FUNCTION - Creates and initializes SQLite database
# ============================================================================
setup_database() {
    local config_file="${CONFIG_DIR}/database.conf"
    local DB_PATH
    DB_PATH=$(get_config_value "$config_file" "DB_PATH" "/var/lib/apiaryguard/apiary.db")
    
    echo "Database path: $DB_PATH"
    echo ""
    
    # Create directory for database
    local db_dir
    db_dir=$(dirname "$DB_PATH")
    mkdir -p "$db_dir"
    
    if [[ $? -ne 0 ]]; then
        echo -e "${RED}Failed to create database directory: $db_dir${NC}"
        log_message "ERROR" "Failed to create database directory: $db_dir"
        return 1
    fi
    
    echo "Created database directory: $db_dir"
    
    # Set proper permissions
    chmod 755 "$db_dir"
    chmod 644 "$db_dir" 2>/dev/null || true
    
    # Check if sqlite3 is available
    if ! command -v sqlite3 &> /dev/null; then
        echo -e "${YELLOW}Warning: sqlite3 command not found. Database will be created by the application.${NC}"
        log_message "WARN" "sqlite3 not found, skipping manual database creation"
        return 0
    fi
    
    # Create database tables if database doesn't exist or is empty
    if [[ ! -f "$DB_PATH" ]] || [[ ! -s "$DB_PATH" ]]; then
        echo "Creating new SQLite database..."
        
        sqlite3 "$DB_PATH" <<EOF
-- Enable WAL mode for better concurrency
PRAGMA journal_mode=WAL;
PRAGMA synchronous=NORMAL;
PRAGMA cache_size=-64000;
PRAGMA temp_store=MEMORY;

-- Table for raw sensor data (second-level)
CREATE TABLE IF NOT EXISTS raw_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    hive_id TEXT NOT NULL,
    temperature REAL,
    humidity REAL,
    weight REAL,
    battery_level INTEGER,
    co2_eq INTEGER,
    voc_idx INTEGER,
    motion_detected INTEGER,
    audio_rms REAL,
    audio_dominant_freq REAL,
    audio_swarm_prob REAL,
    audio_bee_activity REAL,
    radar_distance REAL,
    radar_energy REAL,
    radar_activity REAL,
    hx711_current REAL,
    hx711_slope_24h REAL,
    th_heat_index REAL,
    th_dew_point REAL,
    th_vpd REAL,
    aq_iaq_index REAL
);

-- Table for aggregated data (minute, hour, day, week, month, quarter, year)
CREATE TABLE IF NOT EXISTS aggregated_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_start INTEGER NOT NULL,
    timestamp_end INTEGER NOT NULL,
    hive_id TEXT NOT NULL,
    aggregation_type TEXT NOT NULL,
    temperature_avg REAL,
    temperature_min REAL,
    temperature_max REAL,
    humidity_avg REAL,
    humidity_min REAL,
    humidity_max REAL,
    weight_avg REAL,
    weight_min REAL,
    weight_max REAL,
    battery_avg INTEGER,
    co2_avg INTEGER,
    voc_avg INTEGER,
    audio_rms_avg REAL,
    audio_dominant_freq_avg REAL,
    audio_swarm_prob_avg REAL,
    audio_bee_activity_avg REAL,
    radar_distance_avg REAL,
    radar_energy_avg REAL,
    radar_activity_avg REAL,
    hx711_current_avg REAL,
    hx711_slope_24h_avg REAL,
    record_count INTEGER
);

-- Table for aggregation metadata
CREATE TABLE IF NOT EXISTS aggregation_meta (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    aggregation_type TEXT NOT NULL,
    last_aggregation_time INTEGER,
    record_count INTEGER DEFAULT 0
);

-- Create indexes for faster queries
CREATE INDEX IF NOT EXISTS idx_raw_timestamp ON raw_data(timestamp);
CREATE INDEX IF NOT EXISTS idx_raw_hive ON raw_data(hive_id);
CREATE INDEX IF NOT EXISTS idx_raw_hive_timestamp ON raw_data(hive_id, timestamp);
CREATE INDEX IF NOT EXISTS idx_agg_type ON aggregated_data(aggregation_type);
CREATE INDEX IF NOT EXISTS idx_agg_timestamp ON aggregated_data(timestamp_start, timestamp_end);
CREATE INDEX IF NOT EXISTS idx_agg_hive ON aggregated_data(hive_id);
CREATE INDEX IF NOT EXISTS idx_agg_hive_type ON aggregated_data(hive_id, aggregation_type);

-- Insert initial metadata records
INSERT OR IGNORE INTO aggregation_meta (aggregation_type, last_aggregation_time, record_count)
VALUES 
    ('RAW', 0, 0),
    ('MINUTE', 0, 0),
    ('HOUR', 0, 0),
    ('DAY', 0, 0),
    ('WEEK', 0, 0),
    ('MONTH', 0, 0),
    ('QUARTER', 0, 0),
    ('YEAR', 0, 0);
EOF
        
        if [[ $? -eq 0 ]]; then
            echo -e "${GREEN}Database tables created successfully!${NC}"
            log_message "INFO" "SQLite database created and initialized at $DB_PATH"
        else
            echo -e "${RED}Failed to create database tables${NC}"
            log_message "ERROR" "Failed to create database tables"
            return 1
        fi
    else
        echo -e "${YELLOW}Database already exists at $DB_PATH${NC}"
        echo "Skipping table creation."
    fi
    
    # Set proper file permissions
    chmod 664 "$DB_PATH" 2>/dev/null || true
    chown root:root "$DB_PATH" 2>/dev/null || true
    
    echo ""
    echo "Database setup completed!"
    echo "Location: $DB_PATH"
    echo ""
    
    # Show database stats if sqlite3 is available
    if command -v sqlite3 &> /dev/null && [[ -f "$DB_PATH" ]]; then
        echo "Database statistics:"
        sqlite3 "$DB_PATH" "SELECT 'Tables: ' || COUNT(*) FROM sqlite_master WHERE type='table';" 2>/dev/null || true
        sqlite3 "$DB_PATH" "SELECT 'Indexes: ' || COUNT(*) FROM sqlite_master WHERE type='index';" 2>/dev/null || true
    fi
    
    log_message "INFO" "Database setup completed"
    return 0
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
            # Calculate bar length using awk for float-safe math (scale to 0-40 characters)
            local bar_len
            bar_len=$(awk -v val="$val" -v min="$min_val" -v max="$max_val" 'BEGIN {
                range = max - min
                if (range == 0) range = 1
                bar_len = int((val - min) * 40 / range + 0.5)
                if (bar_len < 0) bar_len = 0
                if (bar_len > 40) bar_len = 40
                print bar_len
            }')
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
    
    # Color codes for status
    STATUS_OK="${GREEN}[OK]${NC}"
    STATUS_WARN="${YELLOW}[WARNING]${NC}"
    STATUS_ERROR="${RED}[ERROR]${NC}"
    STATUS_INFO="${CYAN}[INFO]${NC}"
    
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}              SYSTEM STATUS REPORT - $(date '+%Y-%m-%d %H:%M:%S')${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    
    # ========================================================================
    # 1. SYSTEM INFORMATION
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  SYSTEM INFORMATION                                     │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    local hostname_val=$(hostname 2>/dev/null || echo "N/A")
    local kernel_ver=$(uname -r 2>/dev/null || echo "N/A")
    local os_info=""
    if [[ -f /etc/os-release ]]; then
        os_info=$(grep "PRETTY_NAME" /etc/os-release 2>/dev/null | cut -d'"' -f2 || echo "N/A")
    fi
    local uptime_str=$(uptime -p 2>/dev/null || uptime | awk -F'up ' '{print $2}' | awk -F',' '{print $1","$2}' || echo "N/A")
    local arch=$(uname -m 2>/dev/null || echo "N/A")
    
    printf "  %-20s : %s\n" "Hostname" "$hostname_val"
    printf "  %-20s : %s\n" "Operating System" "$os_info"
    printf "  %-20s : %s\n" "Kernel Version" "$kernel_ver"
    printf "  %-20s : %s\n" "Architecture" "$arch"
    printf "  %-20s : %s\n" "System Uptime" "$uptime_str"
    echo ""
    
    # ========================================================================
    # 2. CPU USAGE & LOAD
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  CPU STATUS                                             │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    # Load averages
    if [[ -f /proc/loadavg ]]; then
        read load1 load5 load15 rest < /proc/loadavg
        local cpu_count=$(nproc 2>/dev/null || grep -c processor /proc/cpuinfo 2>/dev/null || echo 1)
        # Convert load to integer for bash arithmetic (multiply by 100, divide by 100 later)
        local load1_int=$(echo "$load1" | awk '{printf "%.0f", $1 * 100}')
        local load_percent=$((load1_int / cpu_count))
        
        printf "  %-20s : %s\n" "Load Average (1m)" "$load1"
        printf "  %-20s : %s\n" "Load Average (5m)" "$load5"
        printf "  %-20s : %s\n" "Load Average (15m)" "$load15"
        printf "  %-20s : %s cores\n" "CPU Cores" "$cpu_count"
        
        # Load status indicator with fallback if bc is not available
        local load_status="UNKNOWN"
        if command -v bc &>/dev/null; then
            local load_status_check=$(echo "$load1 < $cpu_count * 0.7" | bc -l 2>/dev/null)
            local load_status_warn=$(echo "$load1 < $cpu_count" | bc -l 2>/dev/null)
            if [[ "$load_status_check" == "1" ]]; then
                load_status="$STATUS_OK"
            elif [[ "$load_status_warn" == "1" ]]; then
                load_status="$STATUS_WARN"
            else
                load_status="$STATUS_ERROR"
            fi
        else
            # Fallback: simple integer comparison (less accurate but works without bc)
            local load1_int=${load1%.*}
            if [[ $load1_int -lt $((cpu_count * 7 / 10)) ]]; then
                load_status="$STATUS_OK"
            elif [[ $load1_int -lt $cpu_count ]]; then
                load_status="$STATUS_WARN"
            else
                load_status="$STATUS_ERROR"
            fi
        fi
        printf "  %-20s : %s\n" "CPU Load Status" "$load_status"
    else
        echo "  CPU load information not available"
    fi
    
    # CPU temperature (if available)
    echo ""
    local cpu_temp="N/A"
    if [[ -f /sys/class/thermal/thermal_zone0/temp ]]; then
        cpu_temp=$(cat /sys/class/thermal/thermal_zone0/temp 2>/dev/null)
        # Convert millidegrees to degrees (e.g., 42000 -> 42°C)
        if [[ -n "$cpu_temp" && "$cpu_temp" =~ ^[0-9]+$ ]]; then
            cpu_temp="$((cpu_temp / 1000))°C"
        fi
    elif command -v vcgencmd &>/dev/null; then
        # Raspberry Pi specific
        cpu_temp=$(vcgencmd measure_temp 2>/dev/null | cut -d"'" -f2 || echo "N/A")
    fi
    printf "  %-20s : %s\n" "CPU Temperature" "$cpu_temp"
    echo ""
    
    # ========================================================================
    # 3. MEMORY USAGE
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  MEMORY STATUS                                          │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    if [[ -f /proc/meminfo ]]; then
        local mem_total=$(grep MemTotal /proc/meminfo | awk '{print $2}')
        local mem_free=$(grep MemFree /proc/meminfo | awk '{print $2}')
        local mem_available=$(grep MemAvailable /proc/meminfo | awk '{print $2}')
        local mem_buffers=$(grep Buffers /proc/meminfo | awk '{print $2}')
        local mem_cached=$(grep "^Cached:" /proc/meminfo | awk '{print $2}')
        
        local mem_used=$((mem_total - mem_free - mem_buffers - mem_cached))
        local mem_percent=$((mem_used * 100 / mem_total))
        
        # Convert to MB
        local mem_total_mb=$((mem_total / 1024))
        local mem_used_mb=$((mem_used / 1024))
        local mem_available_mb=$((mem_available / 1024))
        
        printf "  %-20s : %d MB\n" "Total Memory" "$mem_total_mb"
        printf "  %-20s : %d MB (%d%%)\n" "Used Memory" "$mem_used_mb" "$mem_percent"
        printf "  %-20s : %d MB\n" "Available Memory" "$mem_available_mb"
        
        # Memory status indicator
        if [[ $mem_percent -lt 70 ]]; then
            printf "  %-20s : %s\n" "Memory Status" "$STATUS_OK"
        elif [[ $mem_percent -lt 90 ]]; then
            printf "  %-20s : %s\n" "Memory Status" "$STATUS_WARN"
        else
            printf "  %-20s : %s\n" "Memory Status" "$STATUS_ERROR"
        fi
        
        # Swap information
        local swap_total=$(grep SwapTotal /proc/meminfo | awk '{print $2}')
        local swap_free=$(grep SwapFree /proc/meminfo | awk '{print $2}')
        if [[ $swap_total -gt 0 ]]; then
            local swap_used=$((swap_total - swap_free))
            local swap_percent=$((swap_used * 100 / swap_total))
            printf "  %-20s : %d MB (%d%% used)\n" "Swap" "$((swap_total / 1024))" "$swap_percent"
        fi
    else
        echo "  Memory information not available"
    fi
    echo ""
    
    # ========================================================================
    # 4. DISK USAGE
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  DISK STATUS                                            │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    if command -v df &>/dev/null; then
        printf "  %-15s %-10s %-10s %-10s %-8s %s\n" "Filesystem" "Size" "Used" "Avail" "Use%" "Mounted on"
        echo "  ───────────────────────────────────────────────────────────────"
        
        # Show root filesystem and any other mounted partitions
        df -h 2>/dev/null | grep -E '^/dev/' | while read -r line; do
            local fs=$(echo "$line" | awk '{print $1}')
            local size=$(echo "$line" | awk '{print $2}')
            local used=$(echo "$line" | awk '{print $3}')
            local avail=$(echo "$line" | awk '{print $4}')
            local percent=$(echo "$line" | awk '{print $5}' | tr -d '%')
            local mount=$(echo "$line" | awk '{print $6}')
            
            # Truncate long filesystem names
            [[ ${#fs} -gt 15 ]] && fs="${fs:0:12}..."
            [[ ${#mount} -gt 18 ]] && mount="${mount:0:15}..."
            
            local status="$STATUS_OK"
            [[ $percent -gt 80 ]] && status="$STATUS_WARN"
            [[ $percent -gt 95 ]] && status="$STATUS_ERROR"
            
            printf "  %-15s %-10s %-10s %-10s %s%% %s %s\n" "$fs" "$size" "$used" "$avail" "$percent" "$mount" "$status"
        done
        
        # Also show tmpfs if present
        df -h 2>/dev/null | grep -E '^tmpfs' | head -2 | while read -r line; do
            local fs=$(echo "$line" | awk '{print $1}')
            local size=$(echo "$line" | awk '{print $2}')
            local used=$(echo "$line" | awk '{print $3}')
            local avail=$(echo "$line" | awk '{print $4}')
            local percent=$(echo "$line" | awk '{print $5}' | tr -d '%')
            local mount=$(echo "$line" | awk '{print $6}')
            
            [[ ${#mount} -gt 18 ]] && mount="${mount:0:15}..."
            printf "  %-15s %-10s %-10s %-10s %s%% %s\n" "$fs" "$size" "$used" "$avail" "$percent" "$mount"
        done
    else
        echo "  Disk information not available"
    fi
    echo ""
    
    # ========================================================================
    # 5. NETWORK STATUS
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  NETWORK STATUS                                         │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    # Check network interfaces
    if command -v ip &>/dev/null; then
        local interfaces=$(ip -o link show 2>/dev/null | awk -F': ' '{print $2}' | grep -v lo)
        
        for iface in $interfaces; do
            # Strip @... suffix from interface names (e.g., eth0@if2 -> eth0)
            iface="${iface%%@*}"
            local state=$(ip link show "$iface" 2>/dev/null | grep -oE 'state [A-Z]+' | awk '{print $2}')
            local mac=$(ip link show "$iface" 2>/dev/null | grep -oE 'link/[a-z]+ [0-9a-f:]+' | awk '{print $2}')
            
            local state_color="$STATUS_ERROR"
            [[ "$state" == "UP" ]] && state_color="$STATUS_OK"
            [[ "$state" == "DOWN" ]] && state_color="$STATUS_WARN"
            
            printf "  Interface: %-12s State: %s %s\n" "$iface" "$state" "$state_color"
            printf "    MAC: %s\n" "$mac"
            
            # Get IP address if interface is up
            if [[ "$state" == "UP" ]]; then
                local ip_addr=$(ip -4 addr show "$iface" 2>/dev/null | grep -oP '(?<=inet\s)\d+(\.\d+){3}' | head -1)
                [[ -n "$ip_addr" ]] && printf "    IPv4: %s\n" "$ip_addr"
                
                local ip6_addr=$(ip -6 addr show "$iface" 2>/dev/null | grep -oP '(?<=inet6\s)[0-9a-f:]+' | grep -v '::1' | head -1)
                [[ -n "$ip6_addr" ]] && printf "    IPv6: %s\n" "$ip6_addr"
            fi
            echo ""
        done
    fi
    
    # Internet connectivity check
    echo "  Internet Connectivity:"
    if command -v ping &>/dev/null; then
        if ping -c 1 -W 2 8.8.8.8 &>/dev/null; then
            printf "    %-20s : %s\n" "Google DNS (8.8.8.8)" "$STATUS_OK"
        else
            printf "    %-20s : %s\n" "Google DNS (8.8.8.8)" "$STATUS_ERROR"
        fi
        
        if ping -c 1 -W 2 github.com &>/dev/null; then
            printf "    %-20s : %s\n" "GitHub.com" "$STATUS_OK"
        else
            printf "    %-20s : %s\n" "GitHub.com" "$STATUS_WARN"
        fi
    else
        echo "    Ping command not available"
    fi
    echo ""
    
    # ========================================================================
    # 6. HIVE MONITOR SERVICES STATUS
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  HIVE MONITOR SERVICES                                  │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    # Check for systemd services
    local hm_services=("hive-monitor" "hive_monitor" "hivemonitor")
    local services_found=0
    
    if command -v systemctl &>/dev/null; then
        for service in "${hm_services[@]}"; do
            if systemctl list-unit-files 2>/dev/null | grep -q "$service"; then
                services_found=1
                local status=$(systemctl is-active "$service" 2>/dev/null || echo "unknown")
                local status_color="$STATUS_ERROR"
                [[ "$status" == "active" ]] && status_color="$STATUS_OK"
                [[ "$status" == "inactive" ]] && status_color="$STATUS_WARN"
                printf "  %-25s : %-10s %s\n" "$service.service" "$status" "$status_color"
                
                # Show additional info
                if [[ "$status" == "active" ]]; then
                    local pid=$(systemctl show "$service" --property=MainPID 2>/dev/null | cut -d= -f2)
                    [[ -n "$pid" && "$pid" != "0" ]] && printf "    PID: %s\n" "$pid"
                fi
            fi
        done
    fi
    
    # Check for running processes
    echo ""
    echo "  Running Processes:"
    local hm_processes=$(pgrep -f "hive.*monitor\|hivemonitor" 2>/dev/null || echo "")
    if [[ -n "$hm_processes" ]]; then
        for pid in $hm_processes; do
            local proc_name=$(ps -p "$pid" -o comm= 2>/dev/null || echo "unknown")
            local proc_cpu=$(ps -p "$pid" -o %cpu= 2>/dev/null | tr -d ' ' || echo "N/A")
            local proc_mem=$(ps -p "$pid" -o %mem= 2>/dev/null | tr -d ' ' || echo "N/A")
            printf "    PID: %-6s Name: %-20s CPU: %s%% MEM: %s%%\n" "$pid" "$proc_name" "$proc_cpu" "$proc_mem"
        done
    else
        echo "    No Hive Monitor processes found running"
    fi
    
    # Check for Python scripts
    local py_scripts=$(pgrep -f "python.*hive\|python3.*hive" 2>/dev/null || echo "")
    if [[ -n "$py_scripts" ]]; then
        echo ""
        echo "  Python Hive Scripts:"
        for pid in $py_scripts; do
            local cmd=$(ps -p "$pid" -o args= 2>/dev/null | head -c 60 || echo "unknown")
            printf "    PID: %-6s %s...\n" "$pid" "$cmd"
        done
    fi
    
    if [[ $services_found -eq 0 && -z "$hm_processes" ]]; then
        echo -e "    ${YELLOW}No Hive Monitor services or processes detected${NC}"
        echo "    Application may not be installed or configured yet."
    fi
    echo ""
    
    # ========================================================================
    # 7. MICROCONTROLLER / SENSOR CONNECTIONS
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  CONNECTED DEVICES (Serial/USB)                         │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    # Check for serial devices
    local serial_devices=$(ls -la /dev/ttyUSB* /dev/ttyACM* /dev/ttyAMA* /dev/serial/by-id/* 2>/dev/null || echo "")
    if [[ -n "$serial_devices" ]]; then
        echo "  Detected Serial Devices:"
        ls -la /dev/ttyUSB* 2>/dev/null | while read -r line; do
            echo "    $line"
        done
        ls -la /dev/ttyACM* 2>/dev/null | while read -r line; do
            echo "    $line"
        done
        ls -la /dev/serial/by-id/* 2>/dev/null | while read -r line; do
            echo "    $line"
        done
        
        # Try to identify connected devices
        echo ""
        echo "  Device Details:"
        if command -v udevadm &>/dev/null; then
            for dev in /dev/ttyUSB* /dev/ttyACM*; do
                if [[ -e "$dev" ]]; then
                    local vendor=$(udevadm info -a -p "$(udevadm info -q path -n "$dev" 2>/dev/null)" 2>/dev/null | grep -m1 "ATTRS{idVendor}" | cut -d'"' -f2 || echo "N/A")
                    local product=$(udevadm info -a -p "$(udevadm info -q path -n "$dev" 2>/dev/null)" 2>/dev/null | grep -m1 "ATTRS{idProduct}" | cut -d'"' -f2 || echo "N/A")
                    printf "    %s: Vendor=%s Product=%s\n" "$dev" "$vendor" "$product"
                fi
            done
        fi
    else
        echo -e "  ${YELLOW}No serial devices detected${NC}"
        echo "  Microcontrollers (Arduino, Pico, etc.) may not be connected."
    fi
    echo ""
    
    # ========================================================================
    # 8. CONFIGURATION FILES STATUS
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  CONFIGURATION FILES                                    │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    local config_files=(
        "${CONFIG_DIR}/config.ini"
        "${CONFIG_DIR}/settings.json"
        "${HOME}/hive_monitor/config.ini"
        "${HOME}/hive_monitor/.env"
    )
    
    local configs_ok=0
    local configs_missing=0
    
    for cfg in "${config_files[@]}"; do
        if [[ -f "$cfg" ]]; then
            local size=$(du -h "$cfg" 2>/dev/null | cut -f1)
            local modified=$(stat -c %y "$cfg" 2>/dev/null | cut -d'.' -f1 || ls -la "$cfg" | awk '{print $6, $7, $8}')
            printf "  %s %s\n" "$STATUS_OK" "$cfg"
            printf "    Size: %s  Modified: %s\n" "$size" "$modified"
            configs_ok=$((configs_ok + 1))
        else
            printf "  %s %s\n" "$STATUS_WARN" "$cfg (not found)"
            configs_missing=$((configs_missing + 1))
        fi
    done
    
    echo ""
    printf "  Summary: %d found, %d missing\n" "$configs_ok" "$configs_missing"
    echo ""
    
    # ========================================================================
    # 9. RECENT LOG ENTRIES
    # ========================================================================
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│  RECENT LOG ENTRIES (Last 5)                            │${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────┘${NC}"
    
    if [[ -f "$LOG_FILE" ]]; then
        tail -5 "$LOG_FILE" 2>/dev/null | while read -r line; do
            if [[ "$line" == *"[ERROR]"* ]]; then
                echo -e "    ${RED}$line${NC}"
            elif [[ "$line" == *"[WARN]"* ]]; then
                echo -e "    ${YELLOW}$line${NC}"
            elif [[ "$line" == *"[INFO]"* ]]; then
                echo -e "    ${GREEN}$line${NC}"
            else
                echo "    $line"
            fi
        done
    else
        echo "    No installer log file found at: $LOG_FILE"
    fi
    
    # System logs (if accessible)
    echo ""
    if command -v journalctl &>/dev/null; then
        echo "  Recent System Messages:"
        journalctl -n 3 --no-pager 2>/dev/null | tail -3 | while read -r line; do
            echo "    $line"
        done
    fi
    echo ""
    
    # ========================================================================
    # SUMMARY
    # ========================================================================
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}  STATUS SUMMARY${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    
    # Calculate overall health
    local issues=0
    
    # Check critical items
    if [[ -f /proc/loadavg ]]; then
        if (( $(echo "$load1 > $cpu_count" | bc -l 2>/dev/null || echo 0) )); then
            issues=$((issues + 1))
        fi
    fi
    
    if [[ $mem_percent -gt 90 ]]; then
        issues=$((issues + 1))
    fi
    
    # Check disk space for root partition
    local root_usage=$(df / 2>/dev/null | tail -1 | awk '{print $5}' | tr -d '%')
    if [[ -n "$root_usage" && $root_usage -gt 90 ]]; then
        issues=$((issues + 1))
    fi
    
    if [[ $configs_missing -gt 0 ]]; then
        issues=$((issues + 1))
    fi
    
    echo ""
    if [[ $issues -eq 0 ]]; then
        echo -e "  Overall System Health: ${GREEN}HEALTHY${NC}"
        echo -e "  ${STATUS_OK} All critical systems operating normally"
    elif [[ $issues -le 2 ]]; then
        echo -e "  Overall System Health: ${YELLOW}ATTENTION NEEDED${NC}"
        echo -e "  ${STATUS_WARN} $issues minor issue(s) detected"
    else
        echo -e "  Overall System Health: ${RED}CRITICAL${NC}"
        echo -e "  ${STATUS_ERROR} $issues issue(s) require immediate attention"
    fi
    
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
    
    log_message "INFO" "System status check completed - Health: $([[ $issues -eq 0 ]] && echo 'HEALTHY' || echo "ISSUES: $issues")"
    
    wait_for_key
}

option_install_apirray() {
    print_header "${LANG[MENU_10]}"
    
    echo "🚀 Instalacja Apiary Collector (PEŁNA z WebUI i API)..."
    echo ""
    echo "Ta opcja uruchomi skrypt install_apiary.sh z katalogu src/rpi_tui"
    echo ""
    echo -e "${CYAN}Co zostanie zainstalowane:${NC}"
    echo "  • Zależności: git, build-essential, sqlite3, libsqlite3-dev"
    echo "  • Serwer WWW: Apache2, PHP, libapache2-mod-php"
    echo "  • Kompilacja projektu C++ (apiary_collector)"
    echo "  • Instalacja binarek w /usr/local/bin"
    echo "  • Baza danych SQLite z pełnym schematem agregacji"
    echo "  • WebUI API pod /var/www/html/apiary/index.php"
    echo "  • Usługa systemd: apiary-collector"
    echo ""
    echo -e "${YELLOW}UWAGA: To jest PEŁNA instalacja z interfejsem WWW${NC}"
    echo -e "${YELLOW}Jeśli chcesz tylko kolektor danych bez WebUI, użyj opcji 3${NC}"
    echo ""
    
    # Check if running as root or with sudo
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}Note: You may be prompted for sudo password${NC}"
    fi
    
    # Determine the path to install_apiary.sh
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    APIARY_SCRIPT="${SCRIPT_DIR}/src/rpi_tui/install_apiary.sh"
    
    # Check if the script exists
    if [[ ! -f "$APIARY_SCRIPT" ]]; then
        echo -e "${RED}Error: install_apiary.sh not found at $APIARY_SCRIPT${NC}"
        echo ""
        echo "Please ensure you have cloned the repository with all subdirectories."
        log_message "ERROR" "install_apiary.sh not found at $APIARY_SCRIPT"
        wait_for_key
        return 1
    fi
    
    echo "Found install script at: $APIARY_SCRIPT"
    echo ""
    read -p "Continue with FULL installation (WebUI + API)? (y/N): " confirm
    
    if [[ ! $confirm =~ ^[Yy]$ ]]; then
        echo "Installation cancelled."
        log_message "INFO" "Apiary Collector FULL installation cancelled by user"
        wait_for_key
        return 0
    fi
    
    # Make sure the script is executable
    chmod +x "$APIARY_SCRIPT"
    
    echo ""
    echo "Starting FULL installation..."
    echo "=================================================="
    
    # Execute the install script
    cd "$(dirname "$APIARY_SCRIPT")" || {
        echo -e "${RED}Failed to change directory${NC}"
        log_message "ERROR" "Failed to cd to $(dirname "$APIARY_SCRIPT")"
        wait_for_key
        return 1
    }
    
    if bash "$APIARY_SCRIPT"; then
        echo ""
        echo "=================================================="
        echo -e "${GREEN}✅ Apiary Collector FULL installation completed!${NC}"
        echo "=================================================="
        echo ""
        echo -e "${CYAN}Next steps:${NC}"
        echo "  1. Start the collector: sudo systemctl start apiary-collector"
        echo "  2. Check status: sudo systemctl status apiary-collector"
        echo "  3. Access WebUI (GUI): http://$(hostname -I | awk '{print $1}')/apiary/index.html"
        echo "  4. Access WebUI API: http://$(hostname -I | awk '{print $1}')/apiary/index.php?action=latest"
        echo ""
        log_message "INFO" "Apiary Collector FULL installation completed successfully"
    else
        echo ""
        echo "=================================================="
        echo -e "${RED}❌ Installation failed. Check error messages above.${NC}"
        echo "=================================================="
        log_message "ERROR" "Apiary Collector FULL installation failed"
    fi
    
    cd - > /dev/null || true
    wait_for_key
}

option_reset_defaults() {
    print_header "${LANG[MENU_9]}"
    
    echo -e "${RED}==================================================${NC}"
    echo -e "${RED}  WARNING: Reset to Factory Defaults${NC}"
    echo -e "${RED}==================================================${NC}"
    echo ""
    echo "This operation will:"
    echo "  • Remove all custom configuration files"
    echo "  • Reset all settings to factory defaults"
    echo "  • Clear user preferences"
    echo "  • Reset API endpoints, database settings, intervals"
    echo "  • Clear notification and network configurations"
    echo "  • Reset advanced options and watchdog settings"
    echo ""
    echo -e "${YELLOW}IMPORTANT: Your data files will NOT be deleted.${NC}"
    echo -e "${YELLOW}Only configuration files will be reset.${NC}"
    echo ""
    
    # First, offer to create a backup
    echo "It is recommended to create a backup before resetting."
    read -p "Create backup first? (Y/n): " backup_choice
    
    if [[ ! "$backup_choice" =~ ^[Nn]$ ]]; then
        echo ""
        echo "Creating backup..."
        mkdir -p "$BACKUP_DIR"
        TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
        BACKUP_FILE="${BACKUP_DIR}/hive_monitor_backup_${TIMESTAMP}.tar.gz"
        
        if [[ -d "$CONFIG_DIR" ]]; then
            tar -czf "$BACKUP_FILE" -C "$(dirname "$CONFIG_DIR")" "$(basename "$CONFIG_DIR")" 2>/dev/null
            if [[ $? -eq 0 ]]; then
                echo -e "${GREEN}Backup created: $BACKUP_FILE${NC}"
                log_message "INFO" "Backup created before reset: $BACKUP_FILE"
            else
                echo -e "${YELLOW}Warning: Backup creation failed${NC}"
                log_message "WARN" "Backup creation failed before reset"
            fi
        else
            echo -e "${YELLOW}No configuration directory found to backup${NC}"
        fi
        echo ""
    fi
    
    # Confirmation with extra safety
    echo -e "${RED}Are you ABSOLUTELY sure you want to reset all settings?${NC}"
    read -p "Type 'RESET' to confirm (or any other key to cancel): " confirm_reset
    
    if [[ "$confirm_reset" != "RESET" ]]; then
        echo ""
        echo "Operation cancelled. No changes made."
        log_message "INFO" "Reset to defaults cancelled by user"
        wait_for_key
        return 0
    fi
    
    echo ""
    echo "Starting reset process..."
    echo ""
    
    # Define all config files to reset
    declare -a CONFIG_FILES=(
        "${CONFIG_DIR}/api_endpoints.conf"
        "${CONFIG_DIR}/database.conf"
        "${CONFIG_DIR}/intervals.conf"
        "${CONFIG_DIR}/notifications.conf"
        "${CONFIG_DIR}/retention.conf"
        "${CONFIG_DIR}/network.conf"
        "${CONFIG_DIR}/user_prefs.conf"
        "${CONFIG_DIR}/advanced.conf"
        "${CONFIG_DIR}/config.ini"
        "${CONFIG_DIR}/settings.json"
    )
    
    # Remove old config files
    echo "Removing old configuration files..."
    for file in "${CONFIG_FILES[@]}"; do
        if [[ -f "$file" ]]; then
            rm -f "$file"
            echo "  Removed: $file"
            log_message "INFO" "Removed config file: $file"
        fi
    done
    
    # Also remove hive_monitor config if it exists in home directory
    if [[ -f "${HOME}/hive_monitor/config.ini" ]]; then
        rm -f "${HOME}/hive_monitor/config.ini"
        echo "  Removed: ${HOME}/hive_monitor/config.ini"
        log_message "INFO" "Removed config file: ${HOME}/hive_monitor/config.ini"
    fi
    
    if [[ -f "${HOME}/hive_monitor/.env" ]]; then
        rm -f "${HOME}/hive_monitor/.env"
        echo "  Removed: ${HOME}/hive_monitor/.env"
        log_message "INFO" "Removed config file: ${HOME}/hive_monitor/.env"
    fi
    
    echo ""
    echo "Creating fresh configuration files with default values..."
    echo ""
    
    # Recreate config directory
    mkdir -p "$CONFIG_DIR"
    
    # ==========================================================================
    # 1. API Endpoints - Default Values
    # ==========================================================================
    local api_file="${CONFIG_DIR}/api_endpoints.conf"
    set_config_value "$api_file" "LOCAL_API_URL" "http://localhost:8080/api"
    set_config_value "$api_file" "REMOTE_API_URL" "https://hive-monitor.example.com/api"
    set_config_value "$api_file" "MQTT_BROKER" "localhost"
    set_config_value "$api_file" "MQTT_PORT" "1883"
    echo -e "  ${GREEN}✓${NC} API endpoints reset to defaults"
    
    # ==========================================================================
    # 2. Database Settings - Default Values
    # ==========================================================================
    local db_file="${CONFIG_DIR}/database.conf"
    set_config_value "$db_file" "DB_TYPE" "sqlite"
    set_config_value "$db_file" "DB_HOST" "localhost"
    set_config_value "$db_file" "DB_PORT" "5432"
    set_config_value "$db_file" "DB_NAME" "hive_monitor"
    set_config_value "$db_file" "DB_USER" "hiveuser"
    set_config_value "$db_file" "DB_PASSWORD" ""
    set_config_value "$db_file" "DB_PATH" "${CONFIG_DIR}/data/hive_monitor.db"
    echo -e "  ${GREEN}✓${NC} Database settings reset to defaults"
    
    # ==========================================================================
    # 3. Update Intervals - Default Values (in seconds)
    # ==========================================================================
    local interval_file="${CONFIG_DIR}/intervals.conf"
    set_config_value "$interval_file" "SENSOR_INTERVAL" "30"
    set_config_value "$interval_file" "DISPLAY_INTERVAL" "5"
    set_config_value "$interval_file" "LOG_INTERVAL" "300"
    set_config_value "$interval_file" "SYNC_INTERVAL" "3600"
    set_config_value "$interval_file" "BACKUP_INTERVAL" "86400"
    echo -e "  ${GREEN}✓${NC} Update intervals reset to defaults"
    
    # ==========================================================================
    # 4. Notifications - Default Values
    # ==========================================================================
    local notify_file="${CONFIG_DIR}/notifications.conf"
    set_config_value "$notify_file" "EMAIL_ENABLED" "false"
    set_config_value "$notify_file" "EMAIL_ADDRESS" ""
    set_config_value "$notify_file" "SMTP_SERVER" "smtp.gmail.com"
    set_config_value "$notify_file" "SMTP_PORT" "587"
    set_config_value "$notify_file" "SMTP_USER" ""
    set_config_value "$notify_file" "SMTP_PASSWORD" ""
    set_config_value "$notify_file" "PUSH_ENABLED" "false"
    set_config_value "$notify_file" "TEMP_THRESHOLD_HIGH" "40"
    set_config_value "$notify_file" "TEMP_THRESHOLD_LOW" "10"
    set_config_value "$notify_file" "HUMIDITY_THRESHOLD" "80"
    set_config_value "$notify_file" "ALERT_WEIGHT_CRITICAL" "5"
    echo -e "  ${GREEN}✓${NC} Notification settings reset to defaults"
    
    # ==========================================================================
    # 5. Data Retention - Default Values (in days)
    # ==========================================================================
    local retention_file="${CONFIG_DIR}/retention.conf"
    set_config_value "$retention_file" "RAW_DATA_RETENTION" "30"
    set_config_value "$retention_file" "HOURLY_AVG_RETENTION" "365"
    set_config_value "$retention_file" "DAILY_AVG_RETENTION" "730"
    set_config_value "$retention_file" "MONTHLY_AVG_RETENTION" "3650"
    set_config_value "$retention_file" "AUTO_PURGE_ENABLED" "true"
    echo -e "  ${GREEN}✓${NC} Data retention policy reset to defaults"
    
    # ==========================================================================
    # 6. Network Settings - Default Values
    # ==========================================================================
    local network_file="${CONFIG_DIR}/network.conf"
    set_config_value "$network_file" "WIFI_SSID" ""
    set_config_value "$network_file" "WIFI_PASSWORD" ""
    set_config_value "$network_file" "STATIC_IP" ""
    set_config_value "$network_file" "STATIC_NETMASK" "255.255.255.0"
    set_config_value "$network_file" "STATIC_GATEWAY" ""
    set_config_value "$network_file" "DNS_PRIMARY" "8.8.8.8"
    set_config_value "$network_file" "DNS_SECONDARY" "8.8.4.4"
    set_config_value "$network_file" "TIMEZONE" "Europe/Warsaw"
    set_config_value "$network_file" "NTP_SERVER" "pool.ntp.org"
    echo -e "  ${GREEN}✓${NC} Network settings reset to defaults"
    
    # ==========================================================================
    # 7. User Preferences - Default Values
    # ==========================================================================
    local prefs_file="${CONFIG_DIR}/user_prefs.conf"
    set_config_value "$prefs_file" "LANGUAGE" "en"
    set_config_value "$prefs_file" "THEME" "dark"
    set_config_value "$prefs_file" "REFRESH_RATE" "5"
    set_config_value "$prefs_file" "DATE_FORMAT" "YYYY-MM-DD"
    set_config_value "$prefs_file" "TIME_FORMAT" "24h"
    set_config_value "$prefs_file" "TEMP_UNIT" "C"
    set_config_value "$prefs_file" "WEIGHT_UNIT" "kg"
    set_config_value "$prefs_file" "SHOW_GRAPHS" "true"
    set_config_value "$prefs_file" "SHOW_ALERTS" "true"
    echo -e "  ${GREEN}✓${NC} User preferences reset to defaults"
    
    # ==========================================================================
    # 8. Advanced Options - Default Values
    # ==========================================================================
    local advanced_file="${CONFIG_DIR}/advanced.conf"
    set_config_value "$advanced_file" "DEBUG_MODE" "false"
    set_config_value "$advanced_file" "VERBOSE_LOGGING" "false"
    set_config_value "$advanced_file" "MAX_LOG_SIZE" "10"
    set_config_value "$advanced_file" "LOG_ROTATION_COUNT" "5"
    set_config_value "$advanced_file" "WATCHDOG_ENABLED" "true"
    set_config_value "$advanced_file" "WATCHDOG_TIMEOUT" "30"
    set_config_value "$advanced_file" "SAFE_MODE" "false"
    echo -e "  ${GREEN}✓${NC} Advanced options reset to defaults"
    
    # ==========================================================================
    # 9. Main Config.ini - Default Values
    # ==========================================================================
    local main_config="${CONFIG_DIR}/config.ini"
    cat > "$main_config" << 'EOF'
# Hive Monitor Main Configuration
# Reset to factory defaults

[general]
app_name = Hive Monitor
version = 1.0.0
environment = production

[sensors]
auto_detect = true
polling_enabled = true

[logging]
level = INFO
path = /var/log/hive_monitor/

[security]
api_key_required = false
session_timeout = 3600
EOF
    echo -e "  ${GREEN}✓${NC} Main config.ini reset to defaults"
    
    # ==========================================================================
    # 10. Settings.json - Default Values
    # ==========================================================================
    local settings_json="${CONFIG_DIR}/settings.json"
    cat > "$settings_json" << 'EOF'
{
    "version": "1.0.0",
    "first_run": true,
    "wizard_completed": false,
    "features": {
        "graphs_enabled": true,
        "export_enabled": true,
        "alerts_enabled": true
    },
    "ui": {
        "sidebar_collapsed": false,
        "dashboard_layout": "default"
    }
}
EOF
    echo -e "  ${GREEN}✓${NC} Settings.json reset to defaults"
    
    # Also reset hive_monitor directory configs if they exist
    if [[ -d "${HOME}/hive_monitor" ]]; then
        mkdir -p "${HOME}/hive_monitor"
        
        # Create default .env file
        cat > "${HOME}/hive_monitor/.env" << 'EOF'
# Hive Monitor Environment Variables
# Reset to factory defaults

APP_ENV=production
APP_DEBUG=false
APP_URL=http://localhost:8080

DB_CONNECTION=sqlite
DB_DATABASE=${HOME}/.hive_monitor/data/hive_monitor.db

API_KEY=
SECRET_KEY=
EOF
        echo -e "  ${GREEN}✓${NC} Hive Monitor .env reset to defaults"
    fi
    
    echo ""
    echo "=================================================="
    echo -e "${GREEN}  Reset completed successfully!${NC}"
    echo "=================================================="
    echo ""
    echo "All configuration files have been reset to factory defaults."
    echo ""
    echo "Default values applied:"
    echo "  • API: localhost:8080, MQTT on port 1883"
    echo "  • Database: SQLite in ${CONFIG_DIR}/data/"
    echo "  • Sensor interval: 30 seconds"
    echo "  • Data retention: 30 days (raw), 365 days (hourly avg)"
    echo "  • Timezone: Europe/Warsaw"
    echo "  • Language: English"
    echo "  • Debug mode: Disabled"
    echo "  • Watchdog: Enabled (30s timeout)"
    echo ""
    echo -e "${YELLOW}Note: You may need to restart the application for changes to take effect.${NC}"
    echo ""
    
    log_message "INFO" "Reset to defaults completed successfully"
    
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
    echo "10) ${LANG[MENU_10]}"
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
        read -p "Select option (0-10): " choice
        
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
            10) option_install_apirray ;;
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
