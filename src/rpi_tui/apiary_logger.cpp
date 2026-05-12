/**
 * @file apiary_logger.cpp
 * @brief Moduł logowania i debugowania dla systemu APIARY Guard
 * 
 * Ten plik zawiera implementację systemu logowania z obsługą:
 * - Logów systemowych z poziomami (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * - Debugowania z timestampami i kolorami ANSI
 * - Rotacji plików logów
 * - Thread-safe operation z mutex i condition variable
 * - Specjalne metody dla zdarzeń uli i sieci
 * - Kolejka logów z worker thread
 * - Exception handling i gentle code principles
 * - Statystyki i callbacki
 * 
 * KOMPILACJA:
 * g++ -std=c++17 -pthread -o apiary_logger apiary_logger.cpp
 * 
 * UŻYCIE Z APIARY_COLLECTOR:
 * g++ -std=c++17 -pthread -DCOLLECTOR_BUILD apiary_collector.cpp apiary_logger.cpp -o apiary_collector
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#include "apiary_logger.h"
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
#include <map>
#include <stdexcept>

namespace apiary {

// ============================================================================
// IMPLEMENTACJA LoggerConfig
// ============================================================================

LoggerConfig::LoggerConfig() 
    : log_file("/var/log/apiaryguard/apiary.log")
    , debug_file("/var/log/apiaryguard/debug.log")
    , max_file_size(10 * 1024 * 1024)
    , max_queue_size(1000)
    , console_output(true)
    , file_output(true)
    , min_level(LogLevel::DEBUG)
    , rotation_enabled(true)
    , rotation_count(5)
    , include_timestamps(true)
    , include_source(true) {}

bool LoggerConfig::validate() const noexcept {
    if (log_file.empty() || debug_file.empty()) {
        return false;
    }
    if (max_file_size == 0 || max_queue_size == 0) {
        return false;
    }
    if (rotation_count < 0 || rotation_count > 100) {
        return false;
    }
    return true;
}

// ============================================================================
// IMPLEMENTACJA LogEntry
// ============================================================================

LogEntry::LogEntry(LogLevel lvl, const std::string& msg, const std::string& src,
                   const std::string& fl, int ln)
    : level(lvl), message(msg), source(src), 
      timestamp(std::chrono::system_clock::now()),
      file(fl), line(ln), thread_id(std::this_thread::get_id()) {}

// Static helper function for use in LogEntry (must be defined before format())
namespace {
    std::string levelToStringStatic(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default: return "UNKNOWN";
        }
    }
}

std::string LogEntry::format() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    oss << " [" << levelToStringStatic(level) << "]";
    if (!source.empty()) {
        oss << " [" << source << "]";
    }
    if (!file.empty() && line > 0) {
        oss << " (" << file << ":" << line << ")";
    }
    // Use captured thread_id from LogEntry instead of current thread
    oss << " {" << std::hex << thread_id << std::dec << "}";
    oss << " " << message;
    return oss.str();
}

// ============================================================================
// IMPLEMENTACJA Logger (Singleton)
// ============================================================================

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : running_(false), initialized_(false) {}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(const LoggerConfig& config) {
    if (!config.validate()) {
        throw LoggerInitException("Invalid configuration provided");
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) {
            return; // Already initialized
        }
        
        config_ = config;
        try {
            createDirectories(config_.log_file);
            createDirectories(config_.debug_file);
        } catch (const std::exception& e) {
            throw LoggerInitException(std::string("Failed to create directories: ") + e.what());
        }
        
        running_ = true;
        initialized_ = true;
        
        try {
            worker_thread_ = std::thread(&Logger::workerFunction, this);
        } catch (const std::exception& e) {
            running_ = false;
            initialized_ = false;
            throw LoggerInitException(std::string("Failed to start worker thread: ") + e.what());
        }
    }
    
    log(LogLevel::INFO, "System logowania uruchomiony", "Logger");
}

void Logger::shutdown() noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        running_ = false;
        // Reset initialization state to allow re-initialization after shutdown
        initialized_ = false;
    }
    cv_.notify_one();
    
    if (worker_thread_.joinable()) {
        try {
            worker_thread_.join();
        } catch (...) {
            // Gentle code: swallow exceptions during shutdown
        }
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.min_level = level;
}

LogLevel Logger::getLogLevel() const noexcept {
    return config_.min_level;
}

void Logger::flush() {
    cv_.notify_one();
}

void Logger::debug(const std::string& message, const std::string& source) {
    log(LogLevel::DEBUG, message, source);
}

void Logger::info(const std::string& message, const std::string& source) {
    log(LogLevel::INFO, message, source);
}

void Logger::warning(const std::string& message, const std::string& source) {
    log(LogLevel::WARNING, message, source);
}

void Logger::error(const std::string& message, const std::string& source) {
    log(LogLevel::ERROR, message, source);
}

void Logger::critical(const std::string& message, const std::string& source) {
    log(LogLevel::CRITICAL, message, source);
}

void Logger::exception(const std::exception& e, const std::string& source) {
    std::ostringstream oss;
    oss << "Exception caught: " << e.what() << " [type: " << typeid(e).name() << "]";
    log(LogLevel::ERROR, oss.str(), source);
}

void Logger::logHiveEvent(const std::string& hive_id, const std::string& event, 
                          double temperature, double humidity) {
    std::ostringstream oss;
    oss << "[" << hive_id << "] " << event;
    if (temperature > 0.0) {
        oss << " Temp: " << std::fixed << std::setprecision(1) << temperature << "C";
    }
    if (humidity > 0.0) {
        oss << " Humidity: " << std::fixed << std::setprecision(1) << humidity << "%";
    }
    info(oss.str(), "HIVE_MONITOR");
}

void Logger::logNetworkEvent(const std::string& device_ip, const std::string& event) {
    std::ostringstream oss;
    oss << "[" << device_ip << "] " << event;
    info(oss.str(), "NETWORK");
}

std::vector<LogEntry> Logger::getRecentLogs(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> result;
    auto start = recent_logs_.size() > count ? recent_logs_.size() - count : 0;
    for (size_t i = start; i < recent_logs_.size(); ++i) {
        result.push_back(recent_logs_[i]);
    }
    return result;
}

std::vector<LogEntry> Logger::getRecentDebug(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> result;
    for (const auto& entry : recent_logs_) {
        if (entry.level == LogLevel::DEBUG && result.size() < count) {
            result.push_back(entry);
        }
    }
    return result;
}

std::map<std::string, size_t> Logger::getStats() const {
    std::map<std::string, size_t> stats;
    stats["total"] = total_logs_.load();
    stats["debug"] = debug_count_.load();
    stats["info"] = info_count_.load();
    stats["warning"] = warning_count_.load();
    stats["error"] = error_count_.load();
    stats["critical"] = critical_count_.load();
    stats["write_errors"] = write_errors_.load();
    return stats;
}

void Logger::registerCallback(std::function<void(const LogEntry&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (callback) {
        callbacks_.push_back(callback);
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& source,
                 const std::string& file, int line) {
    if (level < config_.min_level) return;
    
    LogEntry entry(level, message, source, file, line);
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_queue_.size() >= config_.max_queue_size) {
            log_queue_.pop();
        }
        log_queue_.push(entry);
        if (recent_logs_.size() >= config_.max_queue_size) {
            recent_logs_.erase(recent_logs_.begin());
        }
        recent_logs_.push_back(entry);
    }
    cv_.notify_one();
}

void Logger::incrementCounter(LogLevel level) {
    total_logs_++;
    switch (level) {
        case LogLevel::DEBUG: debug_count_++; break;
        case LogLevel::INFO: info_count_++; break;
        case LogLevel::WARNING: warning_count_++; break;
        case LogLevel::ERROR: error_count_++; break;
        case LogLevel::CRITICAL: critical_count_++; break;
    }
}

void Logger::workerFunction() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return !log_queue_.empty() || !running_;
        });
        while (!log_queue_.empty() && running_) {
            LogEntry entry = log_queue_.front();
            log_queue_.pop();
            lock.unlock();
            
            try {
                processLogEntry(entry);
            } catch (const std::exception& e) {
                write_errors_++;
                std::cerr << "[LOGGER ERROR] Failed to process log entry: " << e.what() << std::endl;
            }
            
            lock.lock();
        }
    }
    // Drain remaining logs
    while (!log_queue_.empty()) {
        LogEntry entry = log_queue_.front();
        log_queue_.pop();
        try {
            processLogEntry(entry);
        } catch (...) {
            write_errors_++;
        }
    }
}

void Logger::processLogEntry(const LogEntry& entry) {
    incrementCounter(entry.level);
    
    std::string formatted = formatLogEntry(entry);
    
    if (config_.console_output) {
        outputToConsole(formatted, entry.level);
    }
    if (config_.file_output) {
        writeToFile(formatted, entry.level);
    }
    
    // Invoke callbacks
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : callbacks_) {
            try {
                callback(entry);
            } catch (...) {
                // Gentle code: ignore callback errors
            }
        }
    }
}

std::string Logger::formatLogEntry(const LogEntry& entry) {
    return entry.format();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}

void Logger::outputToConsole(const std::string& message, LogLevel level) {
    const char* color = "\033[0m";
    switch (level) {
        case LogLevel::DEBUG: color = "\033[36m"; break;  // Cyan
        case LogLevel::INFO: color = "\033[32m"; break;   // Green
        case LogLevel::WARNING: color = "\033[33m"; break; // Yellow
        case LogLevel::ERROR: color = "\033[31m"; break;  // Red
        case LogLevel::CRITICAL: color = "\033[35m"; break; // Magenta
    }
    std::cout << color << message << "\033[0m" << std::endl;
}

void Logger::writeToFile(const std::string& message, LogLevel level) {
    std::string filename = (level == LogLevel::DEBUG) ? 
                           config_.debug_file : config_.log_file;
    
    if (config_.rotation_enabled) {
        checkRotation(filename);
    }
    
    try {
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << message << std::endl;
            file.flush();
        } else {
            write_errors_++;
            std::cerr << "[LOGGER ERROR] Cannot open file: " << filename << std::endl;
        }
    } catch (const std::exception& e) {
        write_errors_++;
        std::cerr << "[LOGGER ERROR] Write failed: " << e.what() << std::endl;
    }
}

void Logger::checkRotation(const std::string& filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) != 0) return;
    if (static_cast<size_t>(st.st_size) >= config_.max_file_size) {
        rotateFile(filename);
    }
}

void Logger::rotateFile(const std::string& filename) {
    try {
        std::string oldest = filename + "." + std::to_string(config_.rotation_count);
        std::remove(oldest.c_str());
        
        for (int i = config_.rotation_count - 1; i >= 1; --i) {
            std::string old_name = filename + "." + std::to_string(i);
            std::string new_name = filename + "." + std::to_string(i + 1);
            std::rename(old_name.c_str(), new_name.c_str());
        }
        
        std::rename(filename.c_str(), (filename + ".1").c_str());
        log(LogLevel::INFO, "Zrotowano plik: " + filename, "Logger");
    } catch (const std::exception& e) {
        write_errors_++;
        std::cerr << "[LOGGER ERROR] Rotation failed: " << e.what() << std::endl;
    }
}

void Logger::createDirectories(const std::string& filepath) {
    size_t pos = 0;
    std::string dir;
    while ((pos = filepath.find('/', pos + 1)) != std::string::npos) {
        dir = filepath.substr(0, pos);
        if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
            throw std::runtime_error("Failed to create directory: " + dir);
        }
    }
}

// ============================================================================
// FUNKCJE POMOCNICZE (FREE FUNCTIONS)
// ============================================================================

void log_debug(const std::string& msg, const std::string& src) {
    Logger::getInstance().debug(msg, src);
}

void log_info(const std::string& msg, const std::string& src) {
    Logger::getInstance().info(msg, src);
}

void log_warning(const std::string& msg, const std::string& src) {
    Logger::getInstance().warning(msg, src);
}

void log_error(const std::string& msg, const std::string& src) {
    Logger::getInstance().error(msg, src);
}

void log_critical(const std::string& msg, const std::string& src) {
    Logger::getInstance().critical(msg, src);
}

} // namespace apiary
