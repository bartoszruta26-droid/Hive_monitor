/**
 * @file apiary_logger.cpp
 * @brief Moduł logowania i debugowania dla systemu APIARY Guard
 * 
 * Ten plik zawiera implementację systemu logowania z obsługą:
 * - Logów systemowych z poziomami (INFO, WARN, ERROR, DEBUG)
 * - Debugowania z timestampami
 * - Rotacji plików logów
 * - Thread-safe operation
 * 
 * Kompilacja na Raspberry Pi:
 * g++ -std=c++17 -pthread -o apiary_logger apiary_logger.cpp
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <condition_variable>
#include <atomic>

namespace apiary {

// Poziomy logowania
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

// Konfiguracja loggera
struct LoggerConfig {
    std::string log_file = "/var/log/apiaryguard/apiary.log";
    std::string debug_file = "/var/log/apiaryguard/debug.log";
    size_t max_file_size = 10 * 1024 * 1024; // 10MB
    size_t max_queue_size = 1000;
    bool console_output = true;
    bool file_output = true;
    LogLevel min_level = LogLevel::DEBUG;
    bool rotation_enabled = true;
    int rotation_count = 5;
};

// Wpis logu
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string source;
    std::chrono::system_clock::time_point timestamp;
    
    LogEntry(LogLevel lvl, const std::string& msg, const std::string& src = "")
        : level(lvl), message(msg), source(src), 
          timestamp(std::chrono::system_clock::now()) {}
};

// Główna klasa Loggera
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void initialize(const LoggerConfig& config) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            config_ = config;

            // Utwórz katalogi jeśli nie istnieją
            createDirectories(config_.log_file);
            createDirectories(config_.debug_file);

            // Start worker thread
            running_ = true;
            worker_thread_ = std::thread(&Logger::workerFunction, this);
        }

        log(LogLevel::INFO, "System logowania uruchomiony", "Logger");
    }
    
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_one();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        
        log(LogLevel::INFO, "Logger", "System logowania zatrzymany");
    }
    
    // Metody logowania
    void debug(const std::string& message, const std::string& source = "") {
        log(LogLevel::DEBUG, message, source);
    }
    
    void info(const std::string& message, const std::string& source = "") {
        log(LogLevel::INFO, message, source);
    }
    
    void warning(const std::string& message, const std::string& source = "") {
        log(LogLevel::WARNING, message, source);
    }
    
    void error(const std::string& message, const std::string& source = "") {
        log(LogLevel::ERROR, message, source);
    }
    
    void critical(const std::string& message, const std::string& source = "") {
        log(LogLevel::CRITICAL, message, source);
    }
    
    // Specjalne logi dla uli
    void logHiveEvent(const std::string& hive_id, const std::string& event, 
                      double temperature = 0.0, double humidity = 0.0) {
        std::ostringstream oss;
        oss << "[" << hive_id << "] " << event;
        if (temperature > 0.0) {
            oss << " Temp: " << std::fixed << std::setprecision(1) << temperature << "°C";
        }
        if (humidity > 0.0) {
            oss << " Humidity: " << std::fixed << std::setprecision(1) << humidity << "%";
        }
        info(oss.str(), "HIVE_MONITOR");
    }
    
    void logNetworkEvent(const std::string& device_ip, const std::string& event) {
        std::ostringstream oss;
        oss << "[" << device_ip << "] " << event;
        info(oss.str(), "NETWORK");
    }
    
    // Pobierz ostatnie logi
    std::vector<LogEntry> getRecentLogs(size_t count = 100) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LogEntry> result;
        
        // Skopiuj ostatnie wpisy z kolejki
        auto start = recent_logs_.size() > count ? recent_logs_.size() - count : 0;
        for (size_t i = start; i < recent_logs_.size(); ++i) {
            result.push_back(recent_logs_[i]);
        }
        
        return result;
    }
    
    std::vector<LogEntry> getRecentDebug(size_t count = 100) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LogEntry> result;
        
        for (const auto& entry : recent_logs_) {
            if (entry.level == LogLevel::DEBUG && result.size() < count) {
                result.push_back(entry);
            }
        }
        
        return result;
    }

private:
    Logger() : running_(false) {}
    ~Logger() { shutdown(); }
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void log(LogLevel level, const std::string& message, const std::string& source) {
        if (level < config_.min_level) return;
        
        LogEntry entry(level, message, source);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Dodaj do kolejki
            if (log_queue_.size() >= config_.max_queue_size) {
                log_queue_.pop();
            }
            log_queue_.push(entry);
            
            // Przechowuj w pamięci dla szybkiego dostępu
            if (recent_logs_.size() >= config_.max_queue_size) {
                recent_logs_.erase(recent_logs_.begin());
            }
            recent_logs_.push_back(entry);
        }
        
        cv_.notify_one();
    }
    
    void workerFunction() {
        while (running_) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            cv_.wait(lock, [this] {
                return !log_queue_.empty() || !running_;
            });
            
            while (!log_queue_.empty() && running_) {
                LogEntry entry = log_queue_.front();
                log_queue_.pop();
                lock.unlock();
                
                processLogEntry(entry);
                
                lock.lock();
            }
        }
        
        // Opróżnij kolejkę przed zamknięciem
        while (!log_queue_.empty()) {
            LogEntry entry = log_queue_.front();
            log_queue_.pop();
            processLogEntry(entry);
        }
    }
    
    void processLogEntry(const LogEntry& entry) {
        std::string formatted = formatLogEntry(entry);
        
        // Wyjście na konsolę
        if (config_.console_output) {
            outputToConsole(formatted, entry.level);
        }
        
        // Zapis do pliku
        if (config_.file_output) {
            writeToFile(formatted, entry.level);
        }
    }
    
    std::string formatLogEntry(const LogEntry& entry) {
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        oss << " [" << levelToString(entry.level) << "]";
        
        if (!entry.source.empty()) {
            oss << " [" << entry.source << "]";
        }
        
        oss << " " << entry.message;
        
        return oss.str();
    }
    
    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default: return "UNKNOWN";
        }
    }
    
    void outputToConsole(const std::string& message, LogLevel level) {
        // Kolory ANSI
        const char* color = "\033[0m";
        switch (level) {
            case LogLevel::DEBUG: color = "\033[36m"; break;   // Cyan
            case LogLevel::INFO: color = "\033[32m"; break;    // Green
            case LogLevel::WARNING: color = "\033[33m"; break; // Yellow
            case LogLevel::ERROR: color = "\033[31m"; break;   // Red
            case LogLevel::CRITICAL: color = "\033[35m"; break; // Magenta
        }
        
        std::cout << color << message << "\033[0m" << std::endl;
    }
    
    void writeToFile(const std::string& message, LogLevel level) {
        std::string filename = (level == LogLevel::DEBUG) ? 
                               config_.debug_file : config_.log_file;
        
        // Sprawdź rotację
        if (config_.rotation_enabled) {
            checkRotation(filename);
        }
        
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << message << std::endl;
            file.close();
        }
    }
    
    void checkRotation(const std::string& filename) {
        struct stat st;
        if (stat(filename.c_str(), &st) != 0) {
            return;
        }
        
        if (static_cast<size_t>(st.st_size) >= config_.max_file_size) {
            rotateFile(filename);
        }
    }
    
    void rotateFile(const std::string& filename) {
        // Usuń najstarszy plik
        std::string oldest = filename + "." + std::to_string(config_.rotation_count);
        std::remove(oldest.c_str());
        
        // Przesuń pozostałe
        for (int i = config_.rotation_count - 1; i >= 1; --i) {
            std::string old_name = filename + "." + std::to_string(i);
            std::string new_name = filename + "." + std::to_string(i + 1);
            std::rename(old_name.c_str(), new_name.c_str());
        }
        
        // Przenieś obecny plik
        std::rename(filename.c_str(), (filename + ".1").c_str());
        
        log(LogLevel::INFO, "Logger", "Zrotowano plik: " + filename);
    }
    
    void createDirectories(const std::string& filepath) {
        size_t pos = 0;
        std::string dir;
        
        while ((pos = filepath.find('/', pos + 1)) != std::string::npos) {
            dir = filepath.substr(0, pos);
            mkdir(dir.c_str(), 0755);
        }
    }
    
    // Członkowskie dane
    LoggerConfig config_;
    std::queue<LogEntry> log_queue_;
    std::vector<LogEntry> recent_logs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_;
};

// Helper functions dla łatwego użycia
inline void log_debug(const std::string& msg, const std::string& src = "") {
    Logger::getInstance().debug(msg, src);
}

inline void log_info(const std::string& msg, const std::string& src = "") {
    Logger::getInstance().info(msg, src);
}

inline void log_warning(const std::string& msg, const std::string& src = "") {
    Logger::getInstance().warning(msg, src);
}

inline void log_error(const std::string& msg, const std::string& src = "") {
    Logger::getInstance().error(msg, src);
}

inline void log_critical(const std::string& msg, const std::string& src = "") {
    Logger::getInstance().critical(msg, src);
}

} // namespace apiary

// Przykładowe użycie jako standalone program
#ifdef STANDALONE_TEST
int main() {
    using namespace apiary;
    
    // Konfiguracja
    LoggerConfig config;
    config.console_output = true;
    config.file_output = true;
    config.min_level = LogLevel::DEBUG;
    
    // Inicjalizacja
    Logger::getInstance().initialize(config);
    
    std::cout << "=== Test Loggera APIARY Guard ===" << std::endl;
    std::cout << "Naciśnij Ctrl+C aby zakończyć" << std::endl << std::endl;
    
    // Przykładowe logi
    log_info("System uruchomiony", "MAIN");
    log_debug("Inicjalizacja modułu sieciowego", "NETWORK");
    log_info("Połączono z brokerem MQTT", "MQTT");
    
    // Symulacja zdarzeń z uli
    Logger::getInstance().logHiveEvent("UL-001", "Pomiar temperatury", 35.5, 65.0);
    Logger::getInstance().logHiveEvent("UL-002", "Alarm wysokiej temperatury", 39.2, 45.0);
    Logger::getInstance().logNetworkEvent("192.168.1.100", "Połączenie nawiązane");
    
    log_warning("Niski poziom baterii w UL-003", "POWER");
    log_error("Utracono połączenie z UL-004", "NETWORK");
    
    // Symulacja ciągłego logowania
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        log_debug("Iteracja " + std::to_string(i), "LOOP");
    }
    
    // Pokaż ostatnie logi
    std::cout << "\n=== Ostatnie logi ===" << std::endl;
    auto logs = Logger::getInstance().getRecentLogs(5);
    for (const auto& log : logs) {
        std::cout << "[" << apiary::Logger::getInstance().getRecentLogs(1).size() << "] " 
                  << log.message << std::endl;
    }
    
    // Shutdown
    Logger::getInstance().shutdown();
    
    return 0;
}
#endif

// Eksportowane funkcje C-style dla integracji z innymi językami
// Tylko w trybie standalone - nie gdy includowane w innym pliku cpp
#ifndef COLLECTOR_BUILD
extern "C" {
    void apiary_log_init(const char* log_file, const char* debug_file) {
        apiary::LoggerConfig config;
        if (log_file) config.log_file = log_file;
        if (debug_file) config.debug_file = debug_file;
        apiary::Logger::getInstance().initialize(config);
    }
    
    void apiary_log_shutdown() {
        apiary::Logger::getInstance().shutdown();
    }
    
    void apiary_log_info(const char* message) {
        apiary::Logger::getInstance().info(message);
    }
    
    void apiary_log_error(const char* message) {
        apiary::Logger::getInstance().error(message);
    }
    
    void apiary_log_debug(const char* message) {
        apiary::Logger::getInstance().debug(message);
    }
    
    void apiary_log_hive_event(const char* hive_id, const char* event, 
                                double temp, double humidity) {
        apiary::Logger::getInstance().logHiveEvent(hive_id, event, temp, humidity);
    }
}
#endif
