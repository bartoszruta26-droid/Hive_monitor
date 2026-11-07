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
#include "csv_utils.h"
#include "apiary_collector_types.h"
#include "apiary_logger.h"
#include "json_object_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <sys/stat.h>
#include <cstdlib>
#include <cmath>

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
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    if (!config.validate()) throw DatabaseInitException("Invalid database configuration");
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
    
    // Włączenie optymalizacji. Nie pozostawiaj otwartego, częściowo skonfigurowanego
    // uchwytu SQLite po błędzie inicjalizacji.
    try {
    sqlite3_busy_timeout(db_, 3000);
    const auto executePragma = [this](const char* sql, const char* label) {
        char* error_message = nullptr;
        const int pragma_rc = sqlite3_exec(db_, sql, nullptr, nullptr, &error_message);
        if (pragma_rc != SQLITE_OK) {
            const std::string detail = error_message ? error_message : sqlite3_errmsg(db_);
            sqlite3_free(error_message);
            throw DatabaseInitException(std::string(label) + ": " + detail);
        }
        sqlite3_free(error_message);
    };
    executePragma("PRAGMA journal_mode=WAL;", "Cannot enable SQLite WAL");
    executePragma("PRAGMA synchronous=NORMAL;", "Cannot configure SQLite synchronous mode");
    executePragma("PRAGMA cache_size=-64000;", "Cannot configure SQLite cache"); // 64MB cache
    executePragma("PRAGMA temp_store=MEMORY;", "Cannot configure SQLite temporary storage");

    // Tworzenie tabel i indeksów
    createTables();
    createIndexes();
    
    // Wątki muszą wystartować zanim przyjmiemy rekordy od producentów.
    initialized_ = true;
    startBackgroundThreads();
    accepting_ = true;
    LOG_INFO("Baza danych zainicjalizowana pomyślnie");
    } catch (...) {
        accepting_ = false;
        running_ = false;
        wake_cv_.notify_all();
        if (aggregation_thread_.joinable()) aggregation_thread_.join();
        if (cleanup_thread_.joinable()) cleanup_thread_.join();
        if (db_) {
            sqlite3_close_v2(db_);
            db_ = nullptr;
        }
        initialized_ = false;
        throw;
    }
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
            aq_gas_idx REAL,
            payload_json TEXT,
            
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
    
    // Migracja wstecznie zgodna: starsze bazy otrzymują kolumnę oryginalnego payloadu.
    bool has_payload_column = false;
    sqlite3_stmt* columns_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "PRAGMA table_info(raw_data);", -1, &columns_stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(columns_stmt) == SQLITE_ROW) {
            const auto* column_name = reinterpret_cast<const char*>(sqlite3_column_text(columns_stmt, 1));
            if (column_name && std::string(column_name) == "payload_json") {
                has_payload_column = true;
                break;
            }
        }
    }
    sqlite3_finalize(columns_stmt);
    if (!has_payload_column) {
        char* migration_error = nullptr;
        if (sqlite3_exec(db_, "ALTER TABLE raw_data ADD COLUMN payload_json TEXT;", nullptr, nullptr, &migration_error) != SQLITE_OK) {
            const std::string detail = migration_error ? migration_error : sqlite3_errmsg(db_);
            sqlite3_free(migration_error);
            throw DatabaseInitException("Cannot add raw payload column: " + detail);
        }
        sqlite3_free(migration_error);
    }

    const char* metric_tables = R"(
        CREATE TABLE IF NOT EXISTS raw_metric_values (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            hive_id TEXT NOT NULL,
            metric_name TEXT NOT NULL,
            metric_value REAL NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_raw_metric_hive_time
            ON raw_metric_values(hive_id, timestamp, metric_name);
        CREATE TABLE IF NOT EXISTS aggregated_metric_values (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp_start INTEGER NOT NULL,
            timestamp_end INTEGER NOT NULL,
            hive_id TEXT NOT NULL,
            agg_type INTEGER NOT NULL,
            metric_name TEXT NOT NULL,
            value_avg REAL NOT NULL,
            value_min REAL NOT NULL,
            value_max REAL NOT NULL,
            sample_count INTEGER NOT NULL,
            UNIQUE(hive_id, agg_type, timestamp_start, metric_name)
        );
        CREATE INDEX IF NOT EXISTS idx_agg_metric_hive_type_time
            ON aggregated_metric_values(hive_id, agg_type, timestamp_start, metric_name);
    )";
    char* metric_error = nullptr;
    if (sqlite3_exec(db_, metric_tables, nullptr, nullptr, &metric_error) != SQLITE_OK) {
        const std::string detail = metric_error ? metric_error : sqlite3_errmsg(db_);
        sqlite3_free(metric_error);
        throw DatabaseInitException("Cannot create dynamic metric archive: " + detail);
    }
    sqlite3_free(metric_error);

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
        char* error_message = nullptr;
        const int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK) {
            const std::string detail = error_message ? error_message : sqlite3_errmsg(db_);
            sqlite3_free(error_message);
            throw DatabaseInitException("Cannot create SQLite index: " + detail);
        }
        sqlite3_free(error_message);
    }

    LOG_DEBUG("Indeksy bazy danych utworzone pomyślnie");
}

void ApiaryDatabase::startBackgroundThreads() {
    running_ = true;
    try {
        aggregation_thread_ = std::thread(&ApiaryDatabase::aggregationLoop, this);
        cleanup_thread_ = std::thread(&ApiaryDatabase::cleanupLoop, this);
    } catch (...) {
        running_ = false;
        wake_cv_.notify_all();
        if (aggregation_thread_.joinable()) aggregation_thread_.join();
        if (cleanup_thread_.joinable()) cleanup_thread_.join();
        throw;
    }
    LOG_INFO("Wątki tła bazy danych uruchomione");
}

void ApiaryDatabase::storeRawData(const HiveData& hiveData) {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    if (!accepting_ || !initialized_ || !db_) {
        ++write_errors_;
        throw DatabaseWriteException("Database is not initialized; record was not accepted");
    }

    bool flush_required = false;
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        data_buffer_.push_back(hiveData);
        accepted_records_++;
        flush_required = data_buffer_.size() >= BUFFER_SIZE;
    }

    if (flush_required && !flushBuffer()) {
        LOG_WARNING("Bufor SQLite zachowany po nieudanym zapisie; ponowna próba nastąpi w kolejnym cyklu");
    }
}

bool ApiaryDatabase::flushBuffer() {
    std::lock_guard<std::mutex> buffer_lock(buffer_mutex_);
    if (data_buffer_.empty()) return true;

    std::lock_guard<std::mutex> db_lock(db_mutex_);
    if (!db_) return false;
    const char* sql = R"(
        INSERT INTO raw_data (
            timestamp, hive_id,
            temperature, humidity, weight, battery_level, co2_eq, voc_idx, motion_detected,
            audio_rms, audio_dominant_freq, audio_swarm_prob, audio_bee_activity,
            radar_distance, radar_energy, radar_activity,
            hx711_current, hx711_slope_24h,
            th_heat_index, th_dew_point, th_vpd,
            aq_gas_idx, payload_json
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        ++write_errors_;
        LOG_ERROR(std::string("Nie można przygotować zapisu SQLite: ") + sqlite3_errmsg(db_));
        return false; // Zachowaj bufor do kolejnej próby.
    }

    sqlite3_stmt* metric_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "INSERT INTO raw_metric_values(timestamp,hive_id,metric_name,metric_value) VALUES(?,?,?,?)", -1, &metric_stmt, nullptr) != SQLITE_OK) {
        ++write_errors_;
        LOG_ERROR(std::string("Nie można przygotować zapisu metryk dynamicznych: ") + sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }

    char* error_message = nullptr;
    if (sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &error_message) != SQLITE_OK) {
        ++write_errors_;
        LOG_ERROR(std::string("Nie można rozpocząć transakcji SQLite: ") +
                  (error_message ? error_message : sqlite3_errmsg(db_)));
        sqlite3_free(error_message);
        sqlite3_finalize(metric_stmt);
        sqlite3_finalize(stmt);
        return false;
    }

    const size_t batch_size = data_buffer_.size();
    bool batch_ok = true;
    for (const auto& data : data_buffer_) {
        int rc = sqlite3_bind_int64(stmt, 1, data.timestamp);
        if (rc == SQLITE_OK) rc = sqlite3_bind_text(stmt, 2, data.hive_id.c_str(), -1, SQLITE_TRANSIENT);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 3, data.temperature);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 4, data.humidity);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 5, data.weight);
        if (rc == SQLITE_OK) rc = sqlite3_bind_int(stmt, 6, data.battery_level);
        if (rc == SQLITE_OK) rc = sqlite3_bind_int(stmt, 7, data.co2_eq);
        if (rc == SQLITE_OK) rc = sqlite3_bind_int(stmt, 8, data.voc_idx);
        if (rc == SQLITE_OK) rc = sqlite3_bind_int(stmt, 9, data.motion_detected);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 10, data.audio_rms);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 11, data.audio_dominant_freq);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 12, data.audio_swarm_prob);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 13, data.audio_bee_activity);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 14, data.radar_distance);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 15, data.radar_energy);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 16, data.radar_activity);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 17, data.hx711_current);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 18, data.hx711_slope_24h);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 19, data.th_heat_index);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 20, data.th_dew_point);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 21, data.th_vpd);
        if (rc == SQLITE_OK) rc = sqlite3_bind_double(stmt, 22, data.aq_gas_idx);
        if (rc == SQLITE_OK) rc = sqlite3_bind_text(stmt, 23, data.raw_payload_json.c_str(), -1, SQLITE_TRANSIENT);

        if (rc == SQLITE_OK) rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            batch_ok = false;
            LOG_ERROR(std::string("Zapis rekordu SQLite nie powiódł się: ") + sqlite3_errmsg(db_));
            break;
        }

        if (!data.raw_payload_json.empty()) {
            std::map<std::string, std::string> values;
            if (parseJsonObjectScalars(data.raw_payload_json, values)) {
                for (const auto& item : values) {
                    if (item.first.empty() || item.first.size() > 128 || item.second.empty() ||
                        item.second == "true" || item.second == "false" || item.second == "null") continue;
                    char* number_end = nullptr;
                    const double value = std::strtod(item.second.c_str(), &number_end);
                    if (number_end == item.second.c_str() || *number_end != '\\0' || !std::isfinite(value)) continue;
                    int metric_rc = sqlite3_bind_int64(metric_stmt, 1, data.timestamp);
                    if (metric_rc == SQLITE_OK) metric_rc = sqlite3_bind_text(metric_stmt, 2, data.hive_id.c_str(), -1, SQLITE_TRANSIENT);
                    if (metric_rc == SQLITE_OK) metric_rc = sqlite3_bind_text(metric_stmt, 3, item.first.c_str(), -1, SQLITE_TRANSIENT);
                    if (metric_rc == SQLITE_OK) metric_rc = sqlite3_bind_double(metric_stmt, 4, value);
                    if (metric_rc == SQLITE_OK) metric_rc = sqlite3_step(metric_stmt);
                    if (metric_rc != SQLITE_DONE) {
                        batch_ok = false;
                        LOG_ERROR(std::string("Zapis metryki dynamicznej nie powiódł się: ") + sqlite3_errmsg(db_));
                        break;
                    }
                    sqlite3_reset(metric_stmt);
                    sqlite3_clear_bindings(metric_stmt);
                }
            }
        }
        if (!batch_ok) break;

        rc = sqlite3_reset(stmt);
        if (rc != SQLITE_OK) {
            batch_ok = false;
            LOG_ERROR(std::string("Reset instrukcji SQLite nie powiódł się: ") + sqlite3_errmsg(db_));
            break;
        }
        rc = sqlite3_clear_bindings(stmt);
        if (rc != SQLITE_OK) {
            batch_ok = false;
            LOG_ERROR(std::string("Nie można wyczyścić parametrów instrukcji SQLite: ") + sqlite3_errmsg(db_));
            break;
        }
    }

    if (batch_ok) {
        if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &error_message) != SQLITE_OK) {
            batch_ok = false;
            LOG_ERROR(std::string("Commit SQLite nie powiódł się: ") +
                      (error_message ? error_message : sqlite3_errmsg(db_)));
            sqlite3_free(error_message);
        }
    }

    if (!batch_ok) {
        char* rollback_error = nullptr;
        const int rollback_rc = sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &rollback_error);
        if (rollback_rc != SQLITE_OK) {
            const std::string detail = rollback_error ? rollback_error : sqlite3_errmsg(db_);
            LOG_ERROR("Nie udało się wycofać transakcji SQLite: " + detail);
        }
        sqlite3_free(rollback_error);
        ++write_errors_;
    } else {
        data_buffer_.clear();
        raw_records_ += batch_size;
        total_records_ += batch_size;
    }

    sqlite3_finalize(metric_stmt);
    sqlite3_finalize(stmt);
    return batch_ok;
}

void ApiaryDatabase::aggregationLoop() {
    while (running_) {
        std::unique_lock<std::mutex> wait_lock(wake_mutex_);
        const bool stopping = wake_cv_.wait_for(
            wait_lock,
            std::chrono::seconds(config_.aggregation_interval_sec),
            [this] { return !running_.load(); }
        );
        wait_lock.unlock();
        if (stopping || !running_) break;

        try {
            // Najpierw utrwal dane surowe, dopiero potem uruchom agregację.
            flushBuffer();
            performAggregation();
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Błąd agregacji: ") + e.what());
        }
    }
}

void ApiaryDatabase::performAggregation() {
    // Agreguj wyłącznie zamknięte przedziały. Każdy poziom ma własne,
    // kalendarzowe granice; miesiące, kwartały i lata nie są stałą liczbą dni.
    aggregateData(AggregationType::RAW, AggregationType::MINUTE);
    aggregateData(AggregationType::MINUTE, AggregationType::HOUR);
    aggregateData(AggregationType::HOUR, AggregationType::DAY);
    aggregateData(AggregationType::DAY, AggregationType::WEEK);
    aggregateData(AggregationType::DAY, AggregationType::MONTH);
    aggregateData(AggregationType::MONTH, AggregationType::QUARTER);
    aggregateData(AggregationType::MONTH, AggregationType::YEAR);
}

int ApiaryDatabase::aggregateData(AggregationType source_type, AggregationType target_type) {
    if (source_type == target_type || target_type == AggregationType::RAW) return 0;

    const bool source_is_raw = source_type == AggregationType::RAW;
    if (!source_is_raw && source_type == AggregationType::RAW) return 0;
    const std::string source_table = source_is_raw ? "raw_data" : "aggregated_data";
    const std::string source_time_column = source_is_raw ? "timestamp" : "timestamp_start";
    const std::string source_time = "src." + source_time_column;

    // Granice minut i godzin są stałe. Doba, tydzień, miesiąc, kwartał i rok
    // są wyznaczane według kalendarza UTC, z uwzględnieniem długości miesięcy
    // i lat przestępnych.
    std::string bucket_start;
    std::string bucket_end;
    int minimum_records = 1;
    switch (target_type) {
        case AggregationType::MINUTE:
            bucket_start = "(" + source_time + " / 60 * 60)";
            bucket_end = "(" + bucket_start + " + 60)";
            minimum_records = 6;
            break;
        case AggregationType::HOUR:
            bucket_start = "(" + source_time + " / 3600 * 3600)";
            bucket_end = "(" + bucket_start + " + 3600)";
            minimum_records = 30;
            break;
        case AggregationType::DAY:
            bucket_start = "CAST(strftime('%s', date(" + source_time + ", 'unixepoch')) AS INTEGER)";
            bucket_end = "(" + bucket_start + " + 86400)";
            minimum_records = 12;
            break;
        case AggregationType::WEEK:
            bucket_start = "CAST(strftime('%s', date(" + source_time +
                ", 'unixepoch', '-' || ((CAST(strftime('%w', " + source_time +
                ", 'unixepoch') AS INTEGER) + 6) % 7) || ' days')) AS INTEGER)";
            bucket_end = "(" + bucket_start + " + 604800)";
            minimum_records = 4;
            break;
        case AggregationType::MONTH:
            bucket_start = "CAST(strftime('%s', date(" + source_time +
                ", 'unixepoch', 'start of month')) AS INTEGER)";
            bucket_end = "CAST(strftime('%s', date(" + bucket_start +
                ", 'unixepoch', '+1 month')) AS INTEGER)";
            minimum_records = 15;
            break;
        case AggregationType::QUARTER: {
            const std::string month_index = "(CAST(strftime('%m', " + source_time +
                ", 'unixepoch') AS INTEGER) - 1)";
            const std::string quarter_month = "(CAST(" + month_index + " / 3 AS INTEGER) * 3)";
            bucket_start = "CAST(strftime('%s', date(" + source_time +
                ", 'unixepoch', 'start of year', '+' || " + quarter_month +
                " || ' months')) AS INTEGER)";
            bucket_end = "CAST(strftime('%s', date(" + bucket_start +
                ", 'unixepoch', '+3 months')) AS INTEGER)";
            minimum_records = 2;
            break;
        }
        case AggregationType::YEAR:
            bucket_start = "CAST(strftime('%s', date(" + source_time +
                ", 'unixepoch', 'start of year')) AS INTEGER)";
            bucket_end = "CAST(strftime('%s', date(" + bucket_start +
                ", 'unixepoch', '+1 year')) AS INTEGER)";
            minimum_records = 6;
            break;
        default:
            return 0;
    }

    const auto averageExpression = [source_is_raw](const std::string& raw, const std::string& average) {
        return source_is_raw ? "AVG(src." + raw + ")" :
            "SUM(src." + average + " * src.record_count) / NULLIF(SUM(CASE WHEN src." +
            average + " IS NOT NULL THEN src.record_count ELSE 0 END), 0)";
    };
    const auto minimumExpression = [source_is_raw](const std::string& raw, const std::string& minimum) {
        return source_is_raw ? "MIN(src." + raw + ")" : "MIN(src." + minimum + ")";
    };
    const auto maximumExpression = [source_is_raw](const std::string& raw, const std::string& maximum) {
        return source_is_raw ? "MAX(src." + raw + ")" : "MAX(src." + maximum + ")";
    };
    const std::string temperature_avg = averageExpression("temperature", "temperature_avg");
    const std::string temperature_min = minimumExpression("temperature", "temperature_min");
    const std::string temperature_max = maximumExpression("temperature", "temperature_max");
    const std::string humidity_avg = averageExpression("humidity", "humidity_avg");
    const std::string humidity_min = minimumExpression("humidity", "humidity_min");
    const std::string humidity_max = maximumExpression("humidity", "humidity_max");
    const std::string weight_avg = averageExpression("weight", "weight_avg");
    const std::string weight_min = minimumExpression("weight", "weight_min");
    const std::string weight_max = maximumExpression("weight", "weight_max");
    const std::string battery_avg = averageExpression("battery_level", "battery_avg");
    const std::string co2_avg = averageExpression("co2_eq", "co2_avg");
    const std::string voc_avg = averageExpression("voc_idx", "voc_avg");
    const std::string audio_rms_avg = averageExpression("audio_rms", "audio_rms_avg");
    const std::string audio_frequency_avg = averageExpression("audio_dominant_freq", "audio_dominant_freq_avg");
    const std::string audio_swarm_avg = averageExpression("audio_swarm_prob", "audio_swarm_prob_avg");
    const std::string audio_activity_avg = averageExpression("audio_bee_activity", "audio_bee_activity_avg");
    const std::string radar_distance_avg = averageExpression("radar_distance", "radar_distance_avg");
    const std::string radar_energy_avg = averageExpression("radar_energy", "radar_energy_avg");
    const std::string radar_activity_avg = averageExpression("radar_activity", "radar_activity_avg");
    const std::string hx711_current_avg = averageExpression("hx711_current", "hx711_current_avg");
    const std::string hx711_slope_avg = averageExpression("hx711_slope_24h", "hx711_slope_24h_avg");
    const std::string source_record_count = source_is_raw ? "COUNT(*)" : "SUM(src.record_count)";

    const long long target_type_value = static_cast<int>(target_type);
    const long long source_type_value = static_cast<int>(source_type);
    std::stringstream sql;
    sql << "INSERT INTO aggregated_data ("
           "timestamp_start, timestamp_end, hive_id, agg_type,"
           "temperature_avg, temperature_min, temperature_max,"
           "humidity_avg, humidity_min, humidity_max,"
           "weight_avg, weight_min, weight_max,"
           "battery_avg, co2_avg, voc_avg,"
           "audio_rms_avg, audio_dominant_freq_avg, audio_swarm_prob_avg, audio_bee_activity_avg,"
           "radar_distance_avg, radar_energy_avg, radar_activity_avg,"
           "hx711_current_avg, hx711_slope_24h_avg, record_count"
        ") SELECT "
        << bucket_start << ", " << bucket_end << ", src.hive_id, " << target_type_value << ","
        << temperature_avg << ", " << temperature_min << ", " << temperature_max << ","
        << humidity_avg << ", " << humidity_min << ", " << humidity_max << ","
        << weight_avg << ", " << weight_min << ", " << weight_max << ","
        << battery_avg << ", " << co2_avg << ", " << voc_avg << ","
        << audio_rms_avg << ", " << audio_frequency_avg << ","
        << audio_swarm_avg << ", " << audio_activity_avg << ","
        << radar_distance_avg << ", " << radar_energy_avg << ", " << radar_activity_avg << ","
        << hx711_current_avg << ", " << hx711_slope_avg << ", " << source_record_count << " "
        << "FROM " << source_table << " AS src "
        << "WHERE " << bucket_end << " <= CAST(strftime('%s', 'now') AS INTEGER) ";

    if (!source_is_raw) {
        sql << "AND src.agg_type = " << source_type_value << " ";
    }

    // Klucz idempotencji jest początkiem rzeczywistego przedziału kalendarzowego.
    sql << "AND NOT EXISTS ("
        << "SELECT 1 FROM aggregated_data AS existing "
        << "WHERE existing.hive_id = src.hive_id "
        << "AND existing.agg_type = " << target_type_value << " "
        << "AND existing.timestamp_start = " << bucket_start << ") "
        << "GROUP BY " << bucket_start << ", src.hive_id "
        << "HAVING COUNT(*) >= " << minimum_records;

    char* error_message = nullptr;
    const int rc = sqlite3_exec(db_, sql.str().c_str(), nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK) {
        const std::string error = error_message ? error_message : sqlite3_errmsg(db_);
        sqlite3_free(error_message);
        LOG_WARNING("Agregacja nie powiodła się: " + error);
        return 0;
    }

    const int fixed_changes = sqlite3_changes(db_);

    const std::string metric_source = source_is_raw ? "raw_metric_values" : "aggregated_metric_values";
    const std::string metric_time = "src." + std::string(source_is_raw ? "timestamp" : "timestamp_start");
    const std::string metric_avg = source_is_raw ? "AVG(src.metric_value)" :
        "SUM(src.value_avg * src.sample_count) / NULLIF(SUM(src.sample_count),0)";
    const std::string metric_min = source_is_raw ? "MIN(src.metric_value)" : "MIN(src.value_min)";
    const std::string metric_max = source_is_raw ? "MAX(src.metric_value)" : "MAX(src.value_max)";
    const std::string metric_count = source_is_raw ? "COUNT(*)" : "SUM(src.sample_count)";
    std::stringstream metric_sql;
    metric_sql << "INSERT OR IGNORE INTO aggregated_metric_values("
        << "timestamp_start,timestamp_end,hive_id,agg_type,metric_name,value_avg,value_min,value_max,sample_count) "
        << "SELECT " << bucket_start << "," << bucket_end << ",src.hive_id," << target_type_value
        << ",src.metric_name," << metric_avg << "," << metric_min << "," << metric_max << "," << metric_count
        << " FROM " << metric_source << " AS src WHERE " << bucket_end
        << " <= CAST(strftime('%s','now') AS INTEGER) ";
    if (!source_is_raw) metric_sql << "AND src.agg_type = " << source_type_value << " ";
    metric_sql << "AND NOT EXISTS (SELECT 1 FROM aggregated_metric_values e WHERE e.hive_id=src.hive_id "
        << "AND e.agg_type=" << target_type_value << " AND e.timestamp_start=" << bucket_start
        << " AND e.metric_name=src.metric_name) GROUP BY " << bucket_start
        << ",src.hive_id,src.metric_name HAVING COUNT(*) >= " << minimum_records;
    char* metric_error = nullptr;
    const int metric_rc = sqlite3_exec(db_, metric_sql.str().c_str(), nullptr, nullptr, &metric_error);
    int metric_changes = 0;
    if (metric_rc == SQLITE_OK) metric_changes = sqlite3_changes(db_);
    else {
        const std::string detail = metric_error ? metric_error : sqlite3_errmsg(db_);
        sqlite3_free(metric_error);
        LOG_WARNING("Agregacja metryk dynamicznych nie powiodła się: " + detail);
        return fixed_changes;
    }
    sqlite3_free(metric_error);
    const int changes = fixed_changes + metric_changes;
    aggregated_records_ += static_cast<size_t>(changes);
    if (changes > 0) {
        LOG_DEBUG("Zagregowano " + std::to_string(changes) + " rekordów z " +
                  aggregationTypeToString(source_type) + " do " +
                  aggregationTypeToString(target_type));
    }
    return changes;
}

int ApiaryDatabase::cleanupOldData() {
    // Retencja danych źródłowych jest bezterminowa. Dane RAW są podstawą
    // audytu pomiarów i umożliwiają ponowne przeliczenie agregatów po zmianie
    // algorytmu. Nie wolno ich usuwać automatycznie po 7 dniach.
    //
    // Agregaty pozostają przechowywane równolegle; ograniczenia rozmiaru
    // bazy należy rozwiązywać archiwizacją/backupem, a nie utratą historii.
    return 0;
}

void ApiaryDatabase::cleanupLoop() {
    while (running_) {
        std::unique_lock<std::mutex> wait_lock(wake_mutex_);
        const bool stopping = wake_cv_.wait_for(
            wait_lock,
            std::chrono::seconds(config_.cleanup_interval_sec),
            [this] { return !running_.load(); }
        );
        wait_lock.unlock();
        if (stopping || !running_) break;

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
    if (start_time > end_time) return result;

    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) return result;

    const char* sql =
        "SELECT timestamp_start, timestamp_end, hive_id, agg_type,"
        "temperature_avg, temperature_min, temperature_max,"
        "humidity_avg, humidity_min, humidity_max,"
        "weight_avg, weight_min, weight_max,"
        "battery_avg, co2_avg, voc_avg,"
        "audio_rms_avg, audio_dominant_freq_avg, audio_swarm_prob_avg, audio_bee_activity_avg,"
        "radar_distance_avg, radar_energy_avg, radar_activity_avg,"
        "hx711_current_avg, hx711_slope_24h_avg, record_count "
        "FROM aggregated_data "
        "WHERE hive_id = ? AND agg_type = ? "
        "AND timestamp_start >= ? AND timestamp_start <= ? "
        "ORDER BY timestamp_start ASC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR(std::string("Nie można przygotować odczytu historii SQLite: ") + sqlite3_errmsg(db_));
        return result;
    }

    int rc = sqlite3_bind_text(stmt, 1, hive_id.c_str(), -1, SQLITE_TRANSIENT);
    if (rc == SQLITE_OK) rc = sqlite3_bind_int(stmt, 2, static_cast<int>(agg_type));
    if (rc == SQLITE_OK) rc = sqlite3_bind_int64(stmt, 3, start_time);
    if (rc == SQLITE_OK) rc = sqlite3_bind_int64(stmt, 4, end_time);

    if (rc != SQLITE_OK) {
        LOG_ERROR(std::string("Nie można przypisać parametrów odczytu SQLite: ") + sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return result;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        AggregatedRecord record;
        record.timestamp_start = sqlite3_column_int64(stmt, 0);
        record.timestamp_end = sqlite3_column_int64(stmt, 1);
        const auto* stored_hive_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.hive_id = stored_hive_id ? stored_hive_id : "";
        record.agg_type = static_cast<AggregationType>(sqlite3_column_int(stmt, 3));

        record.temperature_avg = static_cast<float>(sqlite3_column_double(stmt, 4));
        record.temperature_min = static_cast<float>(sqlite3_column_double(stmt, 5));
        record.temperature_max = static_cast<float>(sqlite3_column_double(stmt, 6));
        record.humidity_avg = static_cast<float>(sqlite3_column_double(stmt, 7));
        record.humidity_min = static_cast<float>(sqlite3_column_double(stmt, 8));
        record.humidity_max = static_cast<float>(sqlite3_column_double(stmt, 9));
        record.weight_avg = static_cast<float>(sqlite3_column_double(stmt, 10));
        record.weight_min = static_cast<float>(sqlite3_column_double(stmt, 11));
        record.weight_max = static_cast<float>(sqlite3_column_double(stmt, 12));
        record.battery_avg = sqlite3_column_int(stmt, 13);
        record.co2_avg = sqlite3_column_int(stmt, 14);
        record.voc_avg = sqlite3_column_int(stmt, 15);
        record.audio_rms_avg = static_cast<float>(sqlite3_column_double(stmt, 16));
        record.audio_dominant_freq_avg = static_cast<float>(sqlite3_column_double(stmt, 17));
        record.audio_swarm_prob_avg = static_cast<float>(sqlite3_column_double(stmt, 18));
        record.audio_bee_activity_avg = static_cast<float>(sqlite3_column_double(stmt, 19));
        record.radar_distance_avg = static_cast<float>(sqlite3_column_double(stmt, 20));
        record.radar_energy_avg = static_cast<float>(sqlite3_column_double(stmt, 21));
        record.radar_activity_avg = static_cast<float>(sqlite3_column_double(stmt, 22));
        record.hx711_current_avg = static_cast<float>(sqlite3_column_double(stmt, 23));
        record.hx711_slope_24h_avg = static_cast<float>(sqlite3_column_double(stmt, 24));
        record.record_count = sqlite3_column_int(stmt, 25);
        result.push_back(record);
    }

    if (rc != SQLITE_DONE) {
        LOG_ERROR(std::string("Odczyt historii SQLite przerwany: ") + sqlite3_errmsg(db_));
        result.clear();
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<MetricAggregateRecord> ApiaryDatabase::getHistoricalMetrics(
    const std::string& hive_id, AggregationType agg_type,
    long long start_time, long long end_time
) {
    std::vector<MetricAggregateRecord> result;
    if (start_time > end_time) return result;
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) return result;
    const char* sql = "SELECT timestamp_start,timestamp_end,metric_name,value_avg,value_min,value_max,sample_count "
        "FROM aggregated_metric_values WHERE hive_id=? AND agg_type=? "
        "AND timestamp_start>=? AND timestamp_start<=? ORDER BY timestamp_start,metric_name";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    int rc = sqlite3_bind_text(stmt,1,hive_id.c_str(),-1,SQLITE_TRANSIENT);
    if (rc == SQLITE_OK) rc = sqlite3_bind_int(stmt,2,static_cast<int>(agg_type));
    if (rc == SQLITE_OK) rc = sqlite3_bind_int64(stmt,3,start_time);
    if (rc == SQLITE_OK) rc = sqlite3_bind_int64(stmt,4,end_time);
    while (rc == SQLITE_OK && (rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        MetricAggregateRecord record;
        record.timestamp_start = sqlite3_column_int64(stmt,0);
        record.timestamp_end = sqlite3_column_int64(stmt,1);
        const auto* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt,2));
        record.metric_name = name ? name : "";
        record.average = sqlite3_column_double(stmt,3);
        record.minimum = sqlite3_column_double(stmt,4);
        record.maximum = sqlite3_column_double(stmt,5);
        record.sample_count = sqlite3_column_int64(stmt,6);
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
        file << csvEscape(rec.hive_id) << ",";
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
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        stats["buffered_records"] = data_buffer_.size();
    }
    stats["accepted_records"] = accepted_records_;
    stats["total_records"] = total_records_;
    stats["raw_records"] = raw_records_;
    stats["aggregated_records"] = aggregated_records_;
    stats["cleaned_records"] = cleaned_records_;
    stats["write_errors"] = write_errors_;
    stats["initialized"] = initialized_ ? 1 : 0;
    return stats;
}

void ApiaryDatabase::shutdown() noexcept {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    accepting_ = false;
    running_ = false;
    wake_cv_.notify_all();

    // Najpierw zatrzymaj wszystkie wątki korzystające z połączenia.
    if (aggregation_thread_.joinable()) aggregation_thread_.join();
    if (cleanup_thread_.joinable()) cleanup_thread_.join();

    // Nie zamykaj SQLite, jeśli wsad nie został zatwierdzony. Bufor i uchwyt
    // pozostają dostępne do ponownego shutdown() po usunięciu przyczyny awarii.
    if (!flushBuffer()) {
        LOG_ERROR("Shutdown przerwany: nie udało się utrwalić bufora SQLite; dane pozostają w pamięci procesu");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(db_mutex_);
        if (db_) {
            const int rc = sqlite3_close(db_);
            if (rc == SQLITE_OK) {
                db_ = nullptr;
            } else {
                LOG_ERROR(std::string("Nie można zamknąć bazy SQLite: ") + sqlite3_errmsg(db_));
                return;
            }
        }
    }

    initialized_ = false;
    LOG_INFO("Baza danych zamknięta");
}

} // namespace apiary
