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

option_configure_app() {
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
    echo "[STUB] Configuration submenu logic will be implemented here."
    log_message "INFO" "Application configuration requested"
    wait_for_key
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
