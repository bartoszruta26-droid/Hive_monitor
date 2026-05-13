/**
 * @file apiary_database.cpp
 * @brief Implementacja modułu bazy danych dla systemu APIARY Guard
 * 
 * Funkcjonalność:
 * - Zapis danych historycznych do SQLite
 * - Uśrednianie danych: sekundowe → minutowe → godzinne → dzienne → tygodniowe → miesięczne → kwartalne → roczne
 * - Automatyczne czyszczenie starych danych z zachowaniem agregatów
 */

#include "apiary_database.h"
#include "apiary_collector_types.h"
#include "apiary_logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>

namespace apiary {

// ============================================================================
// KONFIGURACJA BAZY DANYCH
// ============================================================================

DatabaseConfig::DatabaseConfig()
    : db_path("/var/lib/apiaryguard/apiary.db"),
      max_raw_records(3600),          // 1 godzina danych co sekundę
      max_minute_records(10080),      // 7 dni danych minutowych
      max_hour_records(720),          // 30 dni danych godzinnych
      max_day_records(365),           // 1 rok danych dziennych
      max_week_records(52),           // 1 rok danych tygodniowych
      max_month_records(60),          // 5 lat danych miesięcznych
      max_quarter_records(40),        // 10 lat danych kwartalnych
      max_year_records(100),          // 100 lat danych rocznych
      aggregation_interval_sec(60),   // Agreguj co minutę
      minute_to_hour_interval(60),
      hour_to_day_interval(24),
      day_to_week_interval(7),
      week_to_month_interval(4),
      month_to_quarter_interval(3),
      quarter_to_year_interval(4),
      auto_cleanup_enabled(true),
      cleanup_interval_sec(300)       // Czyść co 5 minut
{
}

bool DatabaseConfig::validate() const noexcept {
    return !db_path.empty() &&
           max_raw_records > 0 &&
           max_minute_records > 0 &&
           max_hour_records > 0 &&
           aggregation_interval_sec > 0 &&
           cleanup_interval_sec > 0;
}

// ============================================================================
// IMPLEMENTACJA APIARYDATABASE (SINGLETON)
// ============================================================================

ApiaryDatabase& ApiaryDatabase::getInstance() {
    static ApiaryDatabase instance;
    return instance;
}

ApiaryDatabase::ApiaryDatabase()
    : db_(nullptr), running_(false), initialized_(false) {
}

ApiaryDatabase::~ApiaryDatabase() {
    shutdown();
}

void ApiaryDatabase::initialize(const DatabaseConfig& config) {
    if (initialized_) {
        LOG_WARNING("Baza danych już zainicjalizowana");
        return;
    }
    
    config_ = config;
    
    // Tworzenie katalogu dla bazy danych
    std::string db_dir = config_.db_path;
    size_t last_slash = db_dir.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir = db_dir.substr(0, last_slash);
        mkdir(dir.c_str(), 0755);
    }
    
    // Otwarcie bazy SQLite
    int rc = sqlite3_open(config_.db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseInitException(err);
    }
    
    LOG_INFO("Otwarto bazę danych SQLite: " + config_.db_path);
    
    // Włączenie optymalizacji
    char* errMsg = nullptr;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "PRAGMA cache_size=-64000;", nullptr, nullptr, &errMsg); // 64MB cache
    sqlite3_exec(db_, "PRAGMA temp_store=MEMORY;", nullptr, nullptr, &errMsg);
    
    // Tworzenie tabel i indeksów
    createTables();
    createIndexes();
    
    // Uruchomienie wątków tła
    startBackgroundThreads();
    
    initialized_ = true;
    LOG_INFO("Baza danych zainicjalizowana pomyślnie");
}

void ApiaryDatabase::createTables() {
    const char* sql_tables = R"(
        -- Tabela dla danych surowych (sekundowych)
        CREATE TABLE IF NOT EXISTS raw_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            hive_id TEXT NOT NULL,
            
            -- Podstawowe parametry
            temperature REAL,
            humidity REAL,
            weight REAL,
            battery_level INTEGER,
            co2_eq INTEGER,
            voc_idx INTEGER,
            motion_detected INTEGER,
            
            -- Audio
            audio_rms REAL,
            audio_dominant_freq REAL,
            audio_swarm_prob REAL,
            audio_bee_activity REAL,
            
            -- Radar
            radar_distance REAL,
            radar_energy REAL,
            radar_activity REAL,
            
            -- HX711 Waga
            hx711_current REAL,
            hx711_slope_24h REAL,
            
            -- Temp/Humidity rozszerzone
            th_heat_index REAL,
            th_dew_point REAL,
            th_vpd REAL,
            
            -- Air Quality
            aq_iaq_index REAL,
            
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        );
        
        -- Tabela dla danych zagregowanych
        CREATE TABLE IF NOT EXISTS aggregated_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp_start INTEGER NOT NULL,
            timestamp_end INTEGER NOT NULL,
            hive_id TEXT NOT NULL,
            agg_type INTEGER NOT NULL,
            
            -- Temperatury
            temperature_avg REAL,
            temperature_min REAL,
            temperature_max REAL,
            
            -- Wilgotność
            humidity_avg REAL,
            humidity_min REAL,
            humidity_max REAL,
            
            -- Waga
            weight_avg REAL,
            weight_min REAL,
            weight_max REAL,
            
            -- Pozostałe średnie
            battery_avg INTEGER,
            co2_avg INTEGER,
            voc_avg INTEGER,
            
            -- Audio
            audio_rms_avg REAL,
            audio_dominant_freq_avg REAL,
            audio_swarm_prob_avg REAL,
            audio_bee_activity_avg REAL,
            
            -- Radar
            radar_distance_avg REAL,
            radar_energy_avg REAL,
            radar_activity_avg REAL,
            
            -- HX711
            hx711_current_avg REAL,
            hx711_slope_24h_avg REAL,
            
            -- Liczniki
            record_count INTEGER,
            
            created_at INTEGER DEFAULT (strftime('%s', 'now'))
        );
        
        -- Tabela metadanych agregacji
        CREATE TABLE IF NOT EXISTS aggregation_meta (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hive_id TEXT NOT NULL,
            agg_type INTEGER NOT NULL,
            last_timestamp INTEGER,
            last_aggregation INTEGER,
            record_count INTEGER DEFAULT 0,
            UNIQUE(hive_id, agg_type)
        );
    )";
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql_tables, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw DatabaseInitException(err);
    }
    
    LOG_DEBUG("Tabele bazy danych utworzone pomyślnie");
}

void ApiaryDatabase::createIndexes() {
    const char* sql_indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_raw_timestamp ON raw_data(timestamp);",
        "CREATE INDEX IF NOT EXISTS idx_raw_hive ON raw_data(hive_id);",
        "CREATE INDEX IF NOT EXISTS idx_raw_hive_timestamp ON raw_data(hive_id, timestamp);",
        
        "CREATE INDEX IF NOT EXISTS idx_agg_timestamp ON aggregated_data(timestamp_start);",
        "CREATE INDEX IF NOT EXISTS idx_agg_hive ON aggregated_data(hive_id);",
        "CREATE INDEX IF NOT EXISTS idx_agg_type ON aggregated_data(agg_type);",
        "CREATE INDEX IF NOT EXISTS idx_agg_hive_type ON aggregated_data(hive_id, agg_type);",
        "CREATE INDEX IF NOT EXISTS idx_agg_hive_type_ts ON aggregated_data(hive_id, agg_type, timestamp_start);"
    };
    
    for (const auto& sql : sql_indexes) {
        char* errMsg = nullptr;
        sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
        sqlite3_free(errMsg);
    }
    
    LOG_DEBUG("Indeksy bazy danych utworzone pomyślnie");
}

void ApiaryDatabase::startBackgroundThreads() {
    running_ = true;
    aggregation_thread_ = std::thread(&ApiaryDatabase::aggregationLoop, this);
    cleanup_thread_ = std::thread(&ApiaryDatabase::cleanupLoop, this);
    
    LOG_INFO("Wątki tła bazy danych uruchomione");
}

void ApiaryDatabase::storeRawData(const HiveData& hiveData) {
    if (!initialized_ || !db_) {
        write_errors_++;
        return;
    }
    
    // Dodaj do bufora
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        data_buffer_.push_back(hiveData);
        
        // Jeśli bufor pełny, zapisz do bazy
        if (data_buffer_.size() >= BUFFER_SIZE) {
            flushBuffer();
        }
    }
    
    total_records_++;
    raw_records_++;
}

void ApiaryDatabase::flushBuffer() {
    if (data_buffer_.empty() || !db_) return;
    
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    const char* sql = R"(
        INSERT INTO raw_data (
            timestamp, hive_id,
            temperature, humidity, weight, battery_level, co2_eq, voc_idx, motion_detected,
            audio_rms, audio_dominant_freq, audio_swarm_prob, audio_bee_activity,
            radar_distance, radar_energy, radar_activity,
            hx711_current, hx711_slope_24h,
            th_heat_index, th_dew_point, th_vpd,
            aq_iaq_index
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        write_errors_++;
        data_buffer_.clear();
        return;
    }
    
    sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    
    for (const auto& data : data_buffer_) {
        sqlite3_bind_int64(stmt, 1, data.timestamp);
        sqlite3_bind_text(stmt, 2, data.hive_id.c_str(), -1, SQLITE_STATIC);
        
        sqlite3_bind_double(stmt, 3, data.temperature);
        sqlite3_bind_double(stmt, 4, data.humidity);
        sqlite3_bind_double(stmt, 5, data.weight);
        sqlite3_bind_int(stmt, 6, data.battery_level);
        sqlite3_bind_int(stmt, 7, data.co2_eq);
        sqlite3_bind_int(stmt, 8, data.voc_idx);
        sqlite3_bind_int(stmt, 9, data.motion_detected);
        
        sqlite3_bind_double(stmt, 10, data.audio_rms);
        sqlite3_bind_double(stmt, 11, data.audio_dominant_freq);
        sqlite3_bind_double(stmt, 12, data.audio_swarm_prob);
        sqlite3_bind_double(stmt, 13, data.audio_bee_activity);
        
        sqlite3_bind_double(stmt, 14, data.radar_distance);
        sqlite3_bind_double(stmt, 15, data.radar_energy);
        sqlite3_bind_double(stmt, 16, data.radar_activity);
        
        sqlite3_bind_double(stmt, 17, data.hx711_current);
        sqlite3_bind_double(stmt, 18, data.hx711_slope_24h);
        
        sqlite3_bind_double(stmt, 19, data.th_heat_index);
        sqlite3_bind_double(stmt, 20, data.th_dew_point);
        sqlite3_bind_double(stmt, 21, data.th_vpd);
        
        sqlite3_bind_double(stmt, 22, data.aq_iaq_index);
        
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            write_errors_++;
        }
        
        sqlite3_reset(stmt);
    }
    
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);
    
    data_buffer_.clear();
}

void ApiaryDatabase::aggregationLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.aggregation_interval_sec));
        
        try {
            performAggregation();
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Błąd agregacji: ") + e.what());
        }
    }
}

void ApiaryDatabase::performAggregation() {
    // Kaskada agregacji: RAW → MINUTE → HOUR → DAY → WEEK → MONTH → QUARTER → YEAR
    
    // 1. Agregacja RAW → MINUTE
    aggregateData(AggregationType::RAW, AggregationType::MINUTE);
    
    // 2. Agregacja MINUTE → HOUR (co godzinę)
    static time_t last_hour_agg = 0;
    time_t now = std::time(nullptr);
    if (std::difftime(now, last_hour_agg) >= 3600) {
        aggregateData(AggregationType::MINUTE, AggregationType::HOUR);
        last_hour_agg = now;
    }
    
    // 3. Agregacja HOUR → DAY (co dobę)
    static time_t last_day_agg = 0;
    if (std::difftime(now, last_day_agg) >= 86400) {
        aggregateData(AggregationType::HOUR, AggregationType::DAY);
        last_day_agg = now;
    }
    
    // 4. Agregacja DAY → WEEK (co tydzień)
    static time_t last_week_agg = 0;
    if (std::difftime(now, last_week_agg) >= 604800) {
        aggregateData(AggregationType::DAY, AggregationType::WEEK);
        last_week_agg = now;
    }
    
    // 5. Agregacja WEEK → MONTH (co miesiąc ~30 dni)
    static time_t last_month_agg = 0;
    if (std::difftime(now, last_month_agg) >= 2592000) {
        aggregateData(AggregationType::WEEK, AggregationType::MONTH);
        last_month_agg = now;
    }
    
    // 6. Agregacja MONTH → QUARTER (co kwartał ~90 dni)
    static time_t last_quarter_agg = 0;
    if (std::difftime(now, last_quarter_agg) >= 7776000) {
        aggregateData(AggregationType::MONTH, AggregationType::QUARTER);
        last_quarter_agg = now;
    }
    
    // 7. Agregacja QUARTER → YEAR (co rok ~365 dni)
    static time_t last_year_agg = 0;
    if (std::difftime(now, last_year_agg) >= 31536000) {
        aggregateData(AggregationType::QUARTER, AggregationType::YEAR);
        last_year_agg = now;
    }
}

int ApiaryDatabase::aggregateData(AggregationType source_type, AggregationType target_type) {
    if (!db_) return 0;
    
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    std::string source_table = (source_type == AggregationType::RAW) ? "raw_data" : "aggregated_data";
    std::string source_filter = (source_type == AggregationType::RAW) ? 
        "timestamp" : "timestamp_start";
    
    // Określenie interwału czasowego w sekundach
    int interval_sec = 60; // domyślnie minuta
    switch (target_type) {
        case AggregationType::MINUTE: interval_sec = 60; break;
        case AggregationType::HOUR: interval_sec = 3600; break;
        case AggregationType::DAY: interval_sec = 86400; break;
        case AggregationType::WEEK: interval_sec = 604800; break;
        case AggregationType::MONTH: interval_sec = 2592000; break;
        case AggregationType::QUARTER: interval_sec = 7776000; break;
        case AggregationType::YEAR: interval_sec = 31536000; break;
        default: break;
    }
    
    // SQL do agregacji
    std::stringstream sql;
    sql << "INSERT INTO aggregated_data ("
           "timestamp_start, timestamp_end, hive_id, agg_type,"
           "temperature_avg, temperature_min, temperature_max,"
           "humidity_avg, humidity_min, humidity_max,"
           "weight_avg, weight_min, weight_max,"
           "battery_avg, co2_avg, voc_avg,"
           "audio_rms_avg, audio_dominant_freq_avg, audio_swarm_prob_avg, audio_bee_activity_avg,"
           "radar_distance_avg, radar_energy_avg, radar_activity_avg,"
           "hx711_current_avg, hx711_slope_24h_avg,"
           "record_count"
        ") SELECT "
        << source_filter << " / " << interval_sec << " * " << interval_sec << " AS ts_start,"
        << "(" << source_filter << " / " << interval_sec << " + 1) * " << interval_sec << " AS ts_end,"
        << "hive_id,"
        << static_cast<int>(target_type) << ","
        << "AVG(temperature) as temp_avg,"
           "MIN(temperature) as temp_min,"
           "MAX(temperature) as temp_max,"
           "AVG(humidity) as hum_avg,"
           "MIN(humidity) as hum_min,"
           "MAX(humidity) as hum_max,"
           "AVG(weight) as weight_avg,"
           "MIN(weight) as weight_min,"
           "MAX(weight) as weight_max,"
           "AVG(battery_level) as batt_avg,"
           "AVG(co2_eq) as co2_avg,"
           "AVG(voc_idx) as voc_avg,"
           "AVG(audio_rms) as audio_avg,"
           "AVG(audio_dominant_freq) as freq_avg,"
           "AVG(audio_swarm_prob) as swarm_avg,"
           "AVG(audio_bee_activity) as activity_avg,"
           "AVG(radar_distance) as dist_avg,"
           "AVG(radar_energy) as energy_avg,"
           "AVG(radar_activity) as rad_act_avg,"
           "AVG(hx711_current) as hx_avg,"
           "AVG(hx711_slope_24h) as hx_slope_avg,"
           "COUNT(*) as cnt "
        "FROM " << source_table
        << " WHERE " << source_filter << " < (strftime('%s', 'now') - " << interval_sec << ")"
        << " GROUP BY " << source_filter << " / " << interval_sec << ", hive_id"
        << " HAVING cnt >= " << (interval_sec / 10) << "  -- Minimalna liczba rekordów";
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.str().c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        LOG_WARNING("Agregacja nie powiodła się: " + err);
        return 0;
    }
    
    int changes = sqlite3_changes(db_);
    aggregated_records_ += changes;
    
    if (changes > 0) {
        LOG_DEBUG("Zagregowano " + std::to_string(changes) + " rekordów z " + 
                  aggregationTypeToString(source_type) + " do " + 
                  aggregationTypeToString(target_type));
    }
    
    return changes;
}

int ApiaryDatabase::cleanupOldData() {
    if (!db_) return 0;
    
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    int total_deleted = 0;
    
    // UWAGA: Usuwamy TYLKO najstarsze dane surowe (RAW), aby zwolnić miejsce.
    // Dane zagregowane (MINUTE, HOUR, DAY, WEEK, MONTH, QUARTER, YEAR) NIE SĄ usuwane,
    // aby umożliwić analizę trendów z kilkunastu lat.
    
    // Czyszczenie danych surowych - zachowaj ostatnie 7 dni (604800 sekund)
    std::string sql_raw = "DELETE FROM raw_data WHERE timestamp < (strftime('%s', 'now') - 604800)";
    
    sqlite3_exec(db_, sql_raw.c_str(), nullptr, nullptr, nullptr);
    int raw_deleted = sqlite3_changes(db_);
    total_deleted += raw_deleted;
    
    if (raw_deleted > 0) {
        LOG_INFO("Usunięto " + std::to_string(raw_deleted) + " starych rekordów surowych (>7 dni)");
    }
    
    // Nie czyścimy danych zagregowanych - zachowujemy pełną historię!
    // Dzięki temu użytkownik ma dostęp do:
    // - kilkunastu lat danych rocznych
    // - kilkudziesięciu kwartałów
    // - kilkudziesięciu miesięcy
    // - kilkudziesięciu tygodni
    // - kilkudziesięciu godzin
    // - kilkudziesięciu minut
    
    cleaned_records_ += total_deleted;
    
    // VACUUM tylko jeśli coś usunięto
    if (total_deleted > 0) {
        sqlite3_exec(db_, "VACUUM;", nullptr, nullptr, nullptr);
    }
    
    return total_deleted;
}

void ApiaryDatabase::cleanupLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.cleanup_interval_sec));
        
        if (config_.auto_cleanup_enabled) {
            try {
                cleanupOldData();
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Błąd czyszczenia: ") + e.what());
            }
        }
    }
}

std::vector<AggregatedRecord> ApiaryDatabase::getHistoricalData(
    const std::string& hive_id,
    AggregationType agg_type,
    long long start_time,
    long long end_time
) {
    std::vector<AggregatedRecord> result;
    
    if (!db_) return result;
    
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    std::stringstream sql;
    sql << "SELECT timestamp_start, timestamp_end, hive_id, agg_type,";
    sql << "temperature_avg, temperature_min, temperature_max,";
    sql << "humidity_avg, humidity_min, humidity_max,";
    sql << "weight_avg, weight_min, weight_max,";
    sql << "battery_avg, co2_avg, voc_avg,";
    sql << "audio_rms_avg, audio_dominant_freq_avg, audio_swarm_prob_avg, audio_bee_activity_avg,";
    sql << "radar_distance_avg, radar_energy_avg, radar_activity_avg,";
    sql << "hx711_current_avg, hx711_slope_24h_avg,";
    sql << "record_count ";
    sql << "FROM aggregated_data ";
    sql << "WHERE hive_id = '" << hive_id << "' ";
    sql << "AND agg_type = " << static_cast<int>(agg_type) << " ";
    sql << "AND timestamp_start >= " << start_time << " ";
    sql << "AND timestamp_start <= " << end_time << " ";
    sql << "ORDER BY timestamp_start ASC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AggregatedRecord record;
        record.timestamp_start = sqlite3_column_int64(stmt, 0);
        record.timestamp_end = sqlite3_column_int64(stmt, 1);
        record.hive_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.agg_type = static_cast<AggregationType>(sqlite3_column_int(stmt, 3));
        
        record.temperature_avg = sqlite3_column_double(stmt, 4);
        record.temperature_min = sqlite3_column_double(stmt, 5);
        record.temperature_max = sqlite3_column_double(stmt, 6);
        
        record.humidity_avg = sqlite3_column_double(stmt, 7);
        record.humidity_min = sqlite3_column_double(stmt, 8);
        record.humidity_max = sqlite3_column_double(stmt, 9);
        
        record.weight_avg = sqlite3_column_double(stmt, 10);
        record.weight_min = sqlite3_column_double(stmt, 11);
        record.weight_max = sqlite3_column_double(stmt, 12);
        
        record.battery_avg = sqlite3_column_int(stmt, 13);
        record.co2_avg = sqlite3_column_int(stmt, 14);
        record.voc_avg = sqlite3_column_int(stmt, 15);
        
        record.audio_rms_avg = sqlite3_column_double(stmt, 16);
        record.audio_dominant_freq_avg = sqlite3_column_double(stmt, 17);
        record.audio_swarm_prob_avg = sqlite3_column_double(stmt, 18);
        record.audio_bee_activity_avg = sqlite3_column_double(stmt, 19);
        
        record.radar_distance_avg = sqlite3_column_double(stmt, 20);
        record.radar_energy_avg = sqlite3_column_double(stmt, 21);
        record.radar_activity_avg = sqlite3_column_double(stmt, 22);
        
        record.hx711_current_avg = sqlite3_column_double(stmt, 23);
        record.hx711_slope_24h_avg = sqlite3_column_double(stmt, 24);
        
        record.record_count = sqlite3_column_int(stmt, 25);
        
        result.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

void ApiaryDatabase::exportToCSV(const std::string& filename, const std::string& hive_id, AggregationType agg_type) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Nie udało się otworzyć pliku CSV: " + filename);
        return;
    }
    
    // Nagłówek CSV
    file << "timestamp_start,timestamp_end,hive_id,agg_type,";
    file << "temp_avg,temp_min,temp_max,";
    file << "hum_avg,hum_min,hum_max,";
    file << "weight_avg,weight_min,weight_max,";
    file << "battery_avg,co2_avg,voc_avg,";
    file << "audio_rms_avg,audio_freq_avg,audio_swarm_avg,audio_activity_avg,";
    file << "radar_dist_avg,radar_energy_avg,radar_activity_avg,";
    file << "hx711_avg,hx711_slope_avg,record_count\n";
    
    // Pobierz dane
    auto records = getHistoricalData(hive_id, agg_type, 0, std::time(nullptr));
    
    for (const auto& rec : records) {
        file << rec.timestamp_start << ",";
        file << rec.timestamp_end << ",";
        file << rec.hive_id << ",";
        file << static_cast<int>(rec.agg_type) << ",";
        file << rec.temperature_avg << "," << rec.temperature_min << "," << rec.temperature_max << ",";
        file << rec.humidity_avg << "," << rec.humidity_min << "," << rec.humidity_max << ",";
        file << rec.weight_avg << "," << rec.weight_min << "," << rec.weight_max << ",";
        file << rec.battery_avg << "," << rec.co2_avg << "," << rec.voc_avg << ",";
        file << rec.audio_rms_avg << "," << rec.audio_dominant_freq_avg << "," << rec.audio_swarm_prob_avg << "," << rec.audio_bee_activity_avg << ",";
        file << rec.radar_distance_avg << "," << rec.radar_energy_avg << "," << rec.radar_activity_avg << ",";
        file << rec.hx711_current_avg << "," << rec.hx711_slope_24h_avg << ",";
        file << rec.record_count << "\n";
    }
    
    file.close();
    LOG_INFO("Eksportowano " + std::to_string(records.size()) + " rekordów do " + filename);
}

std::string ApiaryDatabase::aggregationTypeToString(AggregationType type) {
    switch (type) {
        case AggregationType::RAW: return "RAW";
        case AggregationType::MINUTE: return "MINUTE";
        case AggregationType::HOUR: return "HOUR";
        case AggregationType::DAY: return "DAY";
        case AggregationType::WEEK: return "WEEK";
        case AggregationType::MONTH: return "MONTH";
        case AggregationType::QUARTER: return "QUARTER";
        case AggregationType::YEAR: return "YEAR";
        default: return "UNKNOWN";
    }
}

AggregationType ApiaryDatabase::stringToAggregationType(const std::string& str) {
    if (str == "RAW") return AggregationType::RAW;
    if (str == "MINUTE") return AggregationType::MINUTE;
    if (str == "HOUR") return AggregationType::HOUR;
    if (str == "DAY") return AggregationType::DAY;
    if (str == "WEEK") return AggregationType::WEEK;
    if (str == "MONTH") return AggregationType::MONTH;
    if (str == "QUARTER") return AggregationType::QUARTER;
    if (str == "YEAR") return AggregationType::YEAR;
    return AggregationType::RAW;
}

std::map<std::string, size_t> ApiaryDatabase::getStats() const {
    std::map<std::string, size_t> stats;
    stats["total_records"] = total_records_;
    stats["raw_records"] = raw_records_;
    stats["aggregated_records"] = aggregated_records_;
    stats["cleaned_records"] = cleaned_records_;
    stats["write_errors"] = write_errors_;
    stats["initialized"] = initialized_ ? 1 : 0;
    return stats;
}

void ApiaryDatabase::shutdown() noexcept {
    running_ = false;
    
    // Flush bufora przed zamknięciem
    flushBuffer();
    
    // Poczekaj na wątki
    if (aggregation_thread_.joinable()) {
        aggregation_thread_.join();
    }
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    // Zamknij bazę
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    
    initialized_ = false;
    LOG_INFO("Baza danych zamknięta");
}

} // namespace apiary
