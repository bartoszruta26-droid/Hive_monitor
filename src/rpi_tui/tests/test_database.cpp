#include "apiary_database.h"
#include "csv_utils.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <ctime>
#include <unistd.h>
#include <sqlite3.h>

int main() {
    assert(apiary::csvEscape("UL-1") == "UL-1");
    assert(apiary::csvEscape("UL,1") == "\"UL,1\"");
    assert(apiary::csvEscape("UL\"1") == "\"UL\"\"1\"");
    assert(apiary::csvEscape("UL-1\r\nUL-2") == "\"UL-1\r\nUL-2\"");
    const std::string path = "/tmp/apiary_database_regression_" +
                             std::to_string(static_cast<long long>(getpid())) + ".db";

    auto& database = apiary::ApiaryDatabase::getInstance();
    apiary::DatabaseConfig config;
    config.db_path = path;
    config.aggregation_interval_sec = 3600;
    config.cleanup_interval_sec = 3600;
    config.auto_cleanup_enabled = false;
    database.initialize(config);

    // Seed one historical row through an independent SQLite connection.
    sqlite3* seed_db = nullptr;
    assert(sqlite3_open(path.c_str(), &seed_db) == SQLITE_OK);
    const char* seed_sql =
        "INSERT INTO aggregated_data (timestamp_start,timestamp_end,hive_id,agg_type,"
        "temperature_avg,record_count) VALUES (100,160,'UL-1',3,34.5,6)";
    assert(sqlite3_exec(seed_db, seed_sql, nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(seed_db);

    const auto legitimate = database.getHistoricalData(
        "UL-1", apiary::AggregationType::DAY, 0, 1000);
    assert(legitimate.size() == 1);
    assert(legitimate.front().hive_id == "UL-1");

    // An SQL-looking identifier must be treated as data, never as SQL syntax.
    const auto injection_attempt = database.getHistoricalData(
        "UL-1' OR 1=1 --", apiary::AggregationType::DAY, 0, 1000);
    assert(injection_attempt.empty());

    // Seed 30 completed minute aggregates in one old hour. The source level
    // filter and bucket idempotency must prevent cross-level/double aggregation.
    const long long now = static_cast<long long>(std::time(nullptr));
    const long long hour_start = ((now - 7200) / 3600) * 3600;
    sqlite3* aggregate_db = nullptr;
    assert(sqlite3_open(path.c_str(), &aggregate_db) == SQLITE_OK);
    sqlite3_stmt* seed_minute = nullptr;
    assert(sqlite3_prepare_v2(
        aggregate_db,
        "INSERT INTO aggregated_data (timestamp_start,timestamp_end,hive_id,agg_type,"
        "temperature_avg,humidity_avg,weight_avg,battery_avg,co2_avg,voc_avg,"
        "audio_rms_avg,audio_dominant_freq_avg,audio_swarm_prob_avg,audio_bee_activity_avg,"
        "radar_distance_avg,radar_energy_avg,radar_activity_avg,hx711_current_avg,"
        "hx711_slope_24h_avg,record_count) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        -1, &seed_minute, nullptr) == SQLITE_OK);
    for (int i = 0; i < 30; ++i) {
        const long long timestamp = hour_start + i * 60;
        sqlite3_bind_int64(seed_minute, 1, timestamp);
        sqlite3_bind_int64(seed_minute, 2, timestamp + 60);
        sqlite3_bind_text(seed_minute, 3, "UL-3", -1, SQLITE_STATIC);
        sqlite3_bind_int(seed_minute, 4, static_cast<int>(apiary::AggregationType::MINUTE));
        sqlite3_bind_double(seed_minute, 5, 33.0 + i);
        sqlite3_bind_double(seed_minute, 6, 60.0);
        sqlite3_bind_double(seed_minute, 7, 40.0);
        sqlite3_bind_int(seed_minute, 8, 90);
        sqlite3_bind_int(seed_minute, 9, 0);
        sqlite3_bind_int(seed_minute, 10, 0);
        for (int column = 11; column <= 19; ++column) sqlite3_bind_double(seed_minute, column, 1.0);
        sqlite3_bind_int(seed_minute, 20, 1);
        assert(sqlite3_step(seed_minute) == SQLITE_DONE);
        assert(sqlite3_reset(seed_minute) == SQLITE_OK);
        sqlite3_clear_bindings(seed_minute);
    }
    sqlite3_finalize(seed_minute);
    sqlite3_close(aggregate_db);

    assert(database.aggregateData(
        apiary::AggregationType::MINUTE, apiary::AggregationType::HOUR) == 1);
    assert(database.aggregateData(
        apiary::AggregationType::MINUTE, apiary::AggregationType::HOUR) == 0);
    const auto hourly = database.getHistoricalData(
        "UL-3", apiary::AggregationType::HOUR, hour_start, hour_start + 3600);
    assert(hourly.size() == 1);
    assert(hourly.front().record_count == 30);

    // A partial batch must be persisted on orderly shutdown.
    HiveData reading;
    reading.hive_id = "UL-2";
    reading.timestamp = 200;
    reading.temperature = 35.0f;
    reading.humidity = 60.0f;
    reading.weight = 40.0f;
    reading.battery_level = 90;
    database.storeRawData(reading);
    const auto pending_stats = database.getStats();
    assert(pending_stats.at("accepted_records") == 1);
    assert(pending_stats.at("buffered_records") == 1);
    assert(pending_stats.at("raw_records") == 0);
    assert(pending_stats.at("total_records") == 0);

    database.shutdown();
    const auto committed_stats = database.getStats();
    assert(committed_stats.at("accepted_records") == 1);
    assert(committed_stats.at("buffered_records") == 0);
    assert(committed_stats.at("raw_records") == 1);
    assert(committed_stats.at("total_records") == 1);

    sqlite3* verify_db = nullptr;
    assert(sqlite3_open(path.c_str(), &verify_db) == SQLITE_OK);
    sqlite3_stmt* stmt = nullptr;
    assert(sqlite3_prepare_v2(
        verify_db,
        "SELECT COUNT(*) FROM raw_data WHERE hive_id = 'UL-2'",
        -1, &stmt, nullptr) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    assert(sqlite3_column_int(stmt, 0) == 1);
    sqlite3_finalize(stmt);
    sqlite3_close(verify_db);

    std::remove(path.c_str());
    std::remove((path + "-wal").c_str());
    std::remove((path + "-shm").c_str());
    return 0;
}
