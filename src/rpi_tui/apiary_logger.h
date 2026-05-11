/**
 * @file apiary_logger.h
 * @brief Nagłówek modułu logowania dla systemu APIARY Guard
 * 
 * Deklaracje funkcji i struktur loggera.
 * Implementacja znajduje się w apiary_logger.cpp
 */

#ifndef APIARY_LOGGER_H
#define APIARY_LOGGER_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <chrono>

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
    std::string log_file;
    std::string debug_file;
    size_t max_file_size;
    size_t max_queue_size;
    bool console_output;
    bool file_output;
    LogLevel min_level;
    bool rotation_enabled;
    int rotation_count;
    
    LoggerConfig();
};

// Główna klasa Loggera
class Logger {
public:
    static Logger& getInstance();
    
    void initialize(const LoggerConfig& config);
    void log(LogLevel level, const std::string& message, const std::string& source = "");
    void setLogLevel(LogLevel level);
    void flush();
    void shutdown();
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void processQueue();
    void rotateFile(const std::string& filename);
    std::string formatTimestamp(std::chrono::system_clock::time_point tp);
    std::string levelToString(LogLevel level);
    
    LoggerConfig config_;
    std::queue<std::string> log_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_;
};

// Funkcje pomocnicze do logowania
void log_debug(const std::string& msg, const std::string& src = "");
void log_info(const std::string& msg, const std::string& src = "");
void log_warning(const std::string& msg, const std::string& src = "");
void log_error(const std::string& msg, const std::string& src = "");
void log_critical(const std::string& msg, const std::string& src = "");

} // namespace apiary

#endif // APIARY_LOGGER_H
