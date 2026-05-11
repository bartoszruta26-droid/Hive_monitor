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
    , rotation_count(5) {}

// ============================================================================
// IMPLEMENTACJA LogEntry
// ============================================================================

LogEntry::LogEntry(LogLevel lvl, const std::string& msg, const std::string& src)
    : level(lvl), message(msg), source(src), 
      timestamp(std::chrono::system_clock::now()) {}

// ============================================================================
// IMPLEMENTACJA Logger (Singleton)
// ============================================================================

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : running_(false) {}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(const LoggerConfig& config) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
        createDirectories(config_.log_file);
        createDirectories(config_.debug_file);
        running_ = true;
        worker_thread_ = std::thread(&Logger::workerFunction, this);
    }
    log(LogLevel::INFO, "System logowania uruchomiony", "Logger");
}

void Logger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        running_ = false;
    }
    cv_.notify_one();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.min_level = level;
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

void Logger::log(LogLevel level, const std::string& message, const std::string& source) {
    if (level < config_.min_level) return;
    
    LogEntry entry(level, message, source);
    
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
            processLogEntry(entry);
            lock.lock();
        }
    }
    while (!log_queue_.empty()) {
        LogEntry entry = log_queue_.front();
        log_queue_.pop();
        processLogEntry(entry);
    }
}

void Logger::processLogEntry(const LogEntry& entry) {
    std::string formatted = formatLogEntry(entry);
    if (config_.console_output) {
        outputToConsole(formatted, entry.level);
    }
    if (config_.file_output) {
        writeToFile(formatted, entry.level);
    }
}

std::string Logger::formatLogEntry(const LogEntry& entry) {
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
        case LogLevel::DEBUG: color = "\033[36m"; break;
        case LogLevel::INFO: color = "\033[32m"; break;
        case LogLevel::WARNING: color = "\033[33m"; break;
        case LogLevel::ERROR: color = "\033[31m"; break;
        case LogLevel::CRITICAL: color = "\033[35m"; break;
    }
    std::cout << color << message << "\033[0m" << std::endl;
}

void Logger::writeToFile(const std::string& message, LogLevel level) {
    std::string filename = (level == LogLevel::DEBUG) ? 
                           config_.debug_file : config_.log_file;
    if (config_.rotation_enabled) {
        checkRotation(filename);
    }
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        file << message << std::endl;
    } else {
        std::cerr << "[LOGGER ERROR] Cannot open file: " << filename << std::endl;
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
    std::string oldest = filename + "." + std::to_string(config_.rotation_count);
    std::remove(oldest.c_str());
    for (int i = config_.rotation_count - 1; i >= 1; --i) {
        std::string old_name = filename + "." + std::to_string(i);
        std::string new_name = filename + "." + std::to_string(i + 1);
        std::rename(old_name.c_str(), new_name.c_str());
    }
    std::rename(filename.c_str(), (filename + ".1").c_str());
    log(LogLevel::INFO, "Zrotowano plik: " + filename, "Logger");
}

void Logger::createDirectories(const std::string& filepath) {
    size_t pos = 0;
    std::string dir;
    while ((pos = filepath.find('/', pos + 1)) != std::string::npos) {
        dir = filepath.substr(0, pos);
        mkdir(dir.c_str(), 0755);
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
