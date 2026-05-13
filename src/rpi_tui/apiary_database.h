/**
 * @file apiary_database.h
 * @brief Moduł bazy danych dla systemu APIARY Guard
 * 
 * Funkcjonalność:
 * - Zapis danych historycznych do SQLite
 * - Uśrednianie danych: sekundowe → minutowe → godzinne → dzienne → tygodniowe → miesięczne → kwartalne → roczne
 * - Automatyczne czyszczenie starych danych z zachowaniem agregatów
 * - Thread-safe operation
 */

#ifndef APIARY_DATABASE_H
#define APIARY_DATABASE_H

#include "apiary_collector_types.h"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <map>
#include <memory>
#include <stdexcept>
#include <sqlite3.h>

// Struktura HiveDataMinimal - alias do pełnej struktury HiveData z apiary_collector_types.h
// Używamy aliasu dla kompatybilności wstecznej
using HiveDataMinimal = HiveData;

namespace apiary {

// Typy agregacji
enum class AggregationType {
    RAW,        // Dane surowe (sekundowe)
    MINUTE,     // Uśrednione minutowe
    HOUR,       // Uśrednione godzinne
    DAY,        // Uśrednione dzienne
    WEEK,       // Uśrednione tygodniowe
    MONTH,      // Uśrednione miesięczne
    QUARTER,    // Uśrednione kwartalne
    YEAR        // Uśrednione roczne
};

// Konfiguracja przechowywania danych
struct DatabaseConfig {
    std::string db_path;              // Ścieżka do bazy SQLite
    size_t max_raw_records;           // Max liczba rekordów surowych (domyślnie 1 godzina = 3600)
    size_t max_minute_records;        // Max minutówek (domyślnie 7 dni = 10080)
    size_t max_hour_records;          // Max godzinówek (domyślnie 30 dni = 720)
    size_t max_day_records;           // Max dziennych (domyślnie 365 dni = 365)
    size_t max_week_records;          // Max tygodniowych (domyślnie 52 tygodnie = 52)
    size_t max_month_records;         // Max miesięcznych (domyślnie 60 miesięcy = 60)
    size_t max_quarter_records;       // Max kwartalnych (domyślnie 40 kwartałów = 40)
    size_t max_year_records;          // Max rocznych (domyślnie 100 lat = 100)
    
    int aggregation_interval_sec;     // Interwał agregacji surowe→minuty (60s)
    int minute_to_hour_interval;      // Interwał minuty→godziny (60 minut)
    int hour_to_day_interval;         // Interwał godziny→dni (24 godziny)
    int day_to_week_interval;         // Interwał dni→tygodnie (7 dni)
    int week_to_month_interval;       // Interwał tygodnie→miesiące (~4 tygodnie)
    int month_to_quarter_interval;    // Interwał miesiące→kwartały (3 miesiące)
    int quarter_to_year_interval;     // Interwał kwartały→lata (4 kwartały)
    
    bool auto_cleanup_enabled;        // Czy włączyć automatyczne czyszczenie
    int cleanup_interval_sec;         // Interwał czyszczenia (300s)
    
    DatabaseConfig();
    bool validate() const noexcept;
};

// Statystyki agregacji dla pojedynczego rekordu
struct AggregatedRecord {
    long long timestamp_start;        // Początek okresu agregacji
    long long timestamp_end;          // Koniec okresu agregacji
    std::string hive_id;              // ID ula
    AggregationType agg_type;         // Typ agregacji
    
    // Podstawowe parametry - średnie
    float temperature_avg = 0.0f;
    float temperature_min = 0.0f;
    float temperature_max = 0.0f;
    float humidity_avg = 0.0f;
    float humidity_min = 0.0f;
    float humidity_max = 0.0f;
    float weight_avg = 0.0f;
    float weight_min = 0.0f;
    float weight_max = 0.0f;
    int battery_avg = 0;
    int co2_avg = 0;
    int voc_avg = 0;
    
    // Audio - średnie
    float audio_rms_avg = 0.0f;
    float audio_dominant_freq_avg = 0.0f;
    float audio_swarm_prob_avg = 0.0f;
    float audio_bee_activity_avg = 0.0f;
    
    // Radar - średnie
    float radar_distance_avg = 0.0f;
    float radar_energy_avg = 0.0f;
    float radar_activity_avg = 0.0f;
    
    // Waga HX711 - średnie
    float hx711_current_avg = 0.0f;
    float hx711_slope_24h_avg = 0.0f;
    
    // Liczniki
    int record_count = 0;             // Liczba rekordów zagregowanych
    
    AggregatedRecord() : timestamp_start(0), timestamp_end(0), agg_type(AggregationType::RAW) {}
};

// Wyjątki bazy danych
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& msg) : std::runtime_error(msg) {}
};

class DatabaseInitException : public DatabaseException {
public:
    explicit DatabaseInitException(const std::string& msg) : DatabaseException("Database initialization failed: " + msg) {}
};

class DatabaseWriteException : public DatabaseException {
public:
    explicit DatabaseWriteException(const std::string& msg) : DatabaseException("Database write failed: " + msg) {}
};

// Główna klasa zarządzania bazą danych (Singleton)
class ApiaryDatabase {
public:
    /**
     * @brief Pobiera instancję singletona ApiaryDatabase
     * @return Referencja do globalnej instancji
     */
    static ApiaryDatabase& getInstance();
    
    /**
     * @brief Inicjalizuje bazę danych
     * @param config Konfiguracja bazy
     * @throws DatabaseInitException jeśli inicjalizacja nie powiedzie się
     */
    void initialize(const DatabaseConfig& config);
    
    /**
     * @brief Zapisuje pojedynczy rekord danych surowych
     * @param hiveData Dane z ula
     * @throws DatabaseWriteException jeśli zapis nie powiedzie się
     */
    void storeRawData(const HiveData& hiveData);
    
    /**
     * @brief Wykonuje agregację danych
     * @param source_type Źródłowy typ agregacji (np. RAW)
     * @param target_type Docelowy typ agregacji (np. MINUTE)
     * @return Liczba utworzonych rekordów zagregowanych
     */
    int aggregateData(AggregationType source_type, AggregationType target_type);
    
    /**
     * @brief Czyści stare dane zgodnie z konfiguracją
     * @return Liczba usuniętych rekordów
     */
    int cleanupOldData();
    
    /**
     * @brief Pobiera dane historyczne dla ula
     * @param hive_id ID ula
     * @param agg_type Typ agregacji
     * @param start_time Czas początkowy (timestamp)
     * @param end_time Czas końcowy (timestamp)
     * @return Wektor rekordów zagregowanych
     */
    std::vector<AggregatedRecord> getHistoricalData(
        const std::string& hive_id,
        AggregationType agg_type,
        long long start_time,
        long long end_time
    );
    
    /**
     * @brief Eksportuje dane do CSV
     * @param filename Nazwa pliku wyjściowego
     * @param hive_id ID ula (pusty = wszystkie ule)
     * @param agg_type Typ agregacji
     */
    void exportToCSV(const std::string& filename, const std::string& hive_id = "", AggregationType agg_type = AggregationType::DAY);
    
    /**
     * @brief Zatrzymuje wątki tła i zwalnia zasoby
     */
    void shutdown() noexcept;
    
    /**
     * @brief Sprawdza czy baza jest zainicjalizowana
     */
    bool isInitialized() const noexcept { return initialized_; }
    
    /**
     * @brief Pobiera statystyki bazy danych
     */
    std::map<std::string, size_t> getStats() const;
    
private:
    ApiaryDatabase();
    ~ApiaryDatabase();
    ApiaryDatabase(const ApiaryDatabase&) = delete;
    ApiaryDatabase& operator=(const ApiaryDatabase&) = delete;
    
    void createTables();                          // Tworzy tabele w bazie
    void createIndexes();                         // Tworzy indeksy
    void startBackgroundThreads();                // Uruchamia wątki tła
    void aggregationLoop();                       // Pętla agregacji danych
    void cleanupLoop();                           // Pętla czyszczenia danych
    void performAggregation();                    // Wykonuje pełny cykl agregacji
    void insertAggregatedRecord(const AggregatedRecord& record); // Wstawia rekord zagregowany
    std::string aggregationTypeToString(AggregationType type); // Konwertuje typ na string
    AggregationType stringToAggregationType(const std::string& str); // Konwertuje string na typ
    void workerFunction();                        // Funkcja wątku roboczego
    void flushBuffer();                           // Zapisuje zawartość bufora do bazy
    
    DatabaseConfig config_;
    sqlite3* db_;
    std::mutex db_mutex_;
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    std::thread aggregation_thread_;
    std::thread cleanup_thread_;
    
    // Buforowanie danych do wsadowego zapisu
    std::vector<HiveData> data_buffer_;
    std::mutex buffer_mutex_;
    static const size_t BUFFER_SIZE = 100;
    
    // Statystyki
    std::atomic<size_t> total_records_{0};
    std::atomic<size_t> raw_records_{0};
    std::atomic<size_t> aggregated_records_{0};
    std::atomic<size_t> cleaned_records_{0};
    std::atomic<size_t> write_errors_{0};
};

// Makra pomocnicze
#define DB_STORE_RAW(data) apiary::ApiaryDatabase::getInstance().storeRawData(data)
#define DB_GET_HISTORY(hive, type, start, end) apiary::ApiaryDatabase::getInstance().getHistoricalData(hive, type, start, end)

} // namespace apiary

#endif // APIARY_DATABASE_H
