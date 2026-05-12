/**
 * @file apiary_logger.h
 * @brief Nagłówek modułu logowania dla systemu APIARY Guard
 * 
 * Deklaracje funkcji i struktur loggera.
 * Implementacja znajduje się w apiary_logger.cpp
 * 
 * FUNKCJONALNOŚĆ:
 * - Wielowątkowe logowanie z kolejką zadań
 * - Rotacja plików logów
 * - Wyjście na konsolę z kolorami ANSI
 * - Specjalne metody dla uli i sieci
 * - Thread-safe operation z mutex i condition variable
 * - Exception-safe design
 * - Gentle code principles
 */

#ifndef APIARY_LOGGER_H
#define APIARY_LOGGER_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <chrono>
#include <thread>
#include <vector>
#include <stdexcept>
#include <functional>
#include <memory>
#include <map>

namespace apiary {

// Poziomy logowania
enum class LogLevel {
    DEBUG = 0,      // Szczegółowe informacje debugowania
    INFO = 1,       // Informacje o normalnym działaniu
    WARNING = 2,    // Ostrzeżenia o potencjalnych problemach
    ERROR = 3,      // Błędy wymagające uwagi
    CRITICAL = 4    // Krytyczne błędy mogące zatrzymać system
};

// Konfiguracja loggera
struct LoggerConfig {
    std::string log_file;           // Ścieżka do głównego pliku logów
    std::string debug_file;         // Ścieżka do pliku logów debug
    size_t max_file_size;           // Maksymalny rozmiar pliku przed rotacją
    size_t max_queue_size;          // Maksymalna wielkość kolejki logów
    bool console_output;            // Czy wypisywać na konsolę
    bool file_output;               // Czy zapisywać do pliku
    LogLevel min_level;             // Minimalny poziom logowania
    bool rotation_enabled;          // Czy włączyć rotację plików
    int rotation_count;             // Liczba przechowywanych rotated files
    bool include_timestamps;        // Czy dołączać timestampy
    bool include_source;            // Czy dołączać źródło wiadomości
    
    LoggerConfig();
    
    // Walidacja konfiguracji
    bool validate() const noexcept;
};

// Wpis logu (struktura wewnętrzna)
struct LogEntry {
    LogLevel level;                 // Poziom logu
    std::string message;            // Treść wiadomości
    std::string source;             // Źródło logu (moduł/funkcja)
    std::chrono::system_clock::time_point timestamp; // Czas zdarzenia
    std::string file;               // Plik źródłowy (__FILE__)
    int line;                       // Linia kodu (__LINE__)
    std::thread::id thread_id;      // ID wątku
    
    LogEntry(LogLevel lvl, const std::string& msg, const std::string& src = "",
             const std::string& fl = "", int ln = 0);
    
    // Formatowanie wpisu
    std::string format() const;
};

// Wyjątki specyficzne dla loggera
class LoggerException : public std::runtime_error {
public:
    explicit LoggerException(const std::string& msg) : std::runtime_error(msg) {}
};

class LoggerInitException : public LoggerException {
public:
    explicit LoggerInitException(const std::string& msg) : LoggerException("Logger initialization failed: " + msg) {}
};

class LoggerWriteException : public LoggerException {
public:
    explicit LoggerWriteException(const std::string& msg) : LoggerException("Logger write failed: " + msg) {}
};

// Główna klasa Loggera (Singleton)
class Logger {
public:
    /**
     * @brief Pobiera instancję singletona Logger
     * @return Referencja do globalnej instancji Logger
     * @throws LoggerInitException jeśli logger nie został zainicjalizowany
     */
    static Logger& getInstance();
    
    /**
     * @brief Sprawdza czy logger jest zainicjalizowany
     * @return true jeśli zainicjalizowany
     */
    bool isInitialized() const noexcept { return initialized_; }
    
    /**
     * @brief Inicjalizuje logger z podaną konfiguracją
     * @param config Konfiguracja loggera
     * @throws LoggerInitException jeśli inicjalizacja nie powiedzie się
     * 
     * Tworzy katalogi dla plików logów, uruchamia wątek worker
     */
    void initialize(const LoggerConfig& config);
    
    /**
     * @brief Dodaje wpis do logu
     * @param level Poziom logowania
     * @param message Treść wiadomości
     * @param source Źródło wiadomości (domyślnie puste)
     * @param file Plik źródłowy (automatycznie z __FILE__)
     * @param line Linia kodu (automatycznie z __LINE__)
     * @throws LoggerWriteException jeśli zapis nie powiedzie się
     */
    void log(LogLevel level, const std::string& message, const std::string& source = "",
             const std::string& file = "", int line = 0);
    
    /**
     * @brief Ustawia minimalny poziom logowania
     * @param level Nowy minimalny poziom
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief Pobiera aktualny poziom logowania
     * @return Aktualny minimalny poziom
     */
    LogLevel getLogLevel() const noexcept;
    
    /**
     * @brief Wymusza zapisanie wszystkich oczekujących logów
     * @throws LoggerWriteException jeśli flush nie powiedzie się
     */
    void flush();
    
    /**
     * @brief Zatrzymuje logger i zwalnia zasoby
     * 
     * Czeka na przetworzenie kolejki, zamyka wątek worker
     */
    void shutdown() noexcept;
    
    // Metody wygodne dla poszczególnych poziomów logowania
    
    /**
     * @brief Loguje wiadomość na poziomie DEBUG
     * @param message Treść wiadomości
     * @param source Źródło wiadomości
     */
    void debug(const std::string& message, const std::string& source = "");
    
    /**
     * @brief Loguje wiadomość na poziomie INFO
     * @param message Treść wiadomości
     * @param source Źródło wiadomości
     */
    void info(const std::string& message, const std::string& source = "");
    
    /**
     * @brief Loguje wiadomość na poziomie WARNING
     * @param message Treść wiadomości
     * @param source Źródło wiadomości
     */
    void warning(const std::string& message, const std::string& source = "");
    
    /**
     * @brief Loguje wiadomość na poziomie ERROR
     * @param message Treść wiadomości
     * @param source Źródło wiadomości
     */
    void error(const std::string& message, const std::string& source = "");
    
    /**
     * @brief Loguje wiadomość na poziomie CRITICAL
     * @param message Treść wiadomość
     * @param source Źródło wiadomości
     */
    void critical(const std::string& message, const std::string& source = "");
    
    /**
     * @brief Loguje wyjątek
     * @param e Wyjątek do zalogowania
     * @param source Źródło wyjątku
     */
    void exception(const std::exception& e, const std::string& source = "EXCEPTION");
    
    /**
     * @brief Loguje zdarzenie związane z ulem
     * @param hive_id Identyfikator ula
     * @param event Opis zdarzenia
     * @param temperature Temperatura (opcjonalnie)
     * @param humidity Wilgotność (opcjonalnie)
     */
    void logHiveEvent(const std::string& hive_id, const std::string& event, 
                      double temperature = 0.0, double humidity = 0.0);
    
    /**
     * @brief Loguje zdarzenie sieciowe
     * @param device_ip Adres IP urządzenia
     * @param event Opis zdarzenia
     */
    void logNetworkEvent(const std::string& device_ip, const std::string& event);
    
    /**
     * @brief Pobiera ostatnie logi z pamięci
     * @param count Liczba logów do pobrania
     * @return Wektor ostatnich logów
     */
    std::vector<LogEntry> getRecentLogs(size_t count = 100);
    
    /**
     * @brief Pobiera ostatnie logi DEBUG z pamięci
     * @param count Liczba logów do pobrania
     * @return Wektor logów DEBUG
     */
    std::vector<LogEntry> getRecentDebug(size_t count = 100);
    
    /**
     * @brief Pobiera statystyki loggera
     * @return Mapa ze statystykami
     */
    std::map<std::string, size_t> getStats() const;
    
    /**
     * @brief Rejestruje callback dla nowych logów
     * @param callback Funkcja wywoływana przy każdym logu
     */
    void registerCallback(std::function<void(const LogEntry&)> callback);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void processQueue();              // Przetwarza kolejkę logów
    void rotateFile(const std::string& filename); // Rotuje plik logów
    std::string formatTimestamp(std::chrono::system_clock::time_point tp); // Formatuje timestamp
    std::string levelToString(LogLevel level); // Konwertuje poziom na string
    void createDirectories(const std::string& filepath); // Tworzy katalogi
    void checkRotation(const std::string& filename); // Sprawdza potrzebę rotacji
    void processLogEntry(const LogEntry& entry); // Przetwarza pojedynczy wpis
    std::string formatLogEntry(const LogEntry& entry); // Formatuje wpis
    void outputToConsole(const std::string& message, LogLevel level); // Wyjście na konsolę
    void writeToFile(const std::string& message, LogLevel level); // Zapis do pliku
    void workerFunction(); // Funkcja wątku worker
    void incrementCounter(LogLevel level); // Inkrementuje liczniki
    
    LoggerConfig config_;
    std::queue<LogEntry> log_queue_;
    std::vector<LogEntry> recent_logs_;  // Pamięć podręczna dla szybkiego dostępu
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    
    // Statystyki
    std::atomic<size_t> total_logs_{0};
    std::atomic<size_t> debug_count_{0};
    std::atomic<size_t> info_count_{0};
    std::atomic<size_t> warning_count_{0};
    std::atomic<size_t> error_count_{0};
    std::atomic<size_t> critical_count_{0};
    std::atomic<size_t> write_errors_{0};
    
    // Callbacks
    std::vector<std::function<void(const LogEntry&)>> callbacks_;
    std::mutex callback_mutex_;
};

// Makra pomocnicze do automatycznego dodawania informacji o pliku i linii
#define LOG_DEBUG(msg) apiary::Logger::getInstance().debug(msg, __func__)
#define LOG_INFO(msg) apiary::Logger::getInstance().info(msg, __func__)
#define LOG_WARNING(msg) apiary::Logger::getInstance().warning(msg, __func__)
#define LOG_ERROR(msg) apiary::Logger::getInstance().error(msg, __func__)
#define LOG_CRITICAL(msg) apiary::Logger::getInstance().critical(msg, __func__)
#define LOG_EXCEPTION(e) apiary::Logger::getInstance().exception(e, __func__)

// Funkcje pomocnicze do logowania (free functions)
void log_debug(const std::string& msg, const std::string& src = "");
void log_info(const std::string& msg, const std::string& src = "");
void log_warning(const std::string& msg, const std::string& src = "");
void log_error(const std::string& msg, const std::string& src = "");
void log_critical(const std::string& msg, const std::string& src = "");

} // namespace apiary

#endif // APIARY_LOGGER_H
