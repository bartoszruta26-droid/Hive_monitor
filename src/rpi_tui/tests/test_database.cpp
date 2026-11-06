#include "apiary_database.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <sqlite3.h>

int main() {
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

    // A partial batch must be persisted on orderly shutdown.
    HiveData reading;
    reading.hive_id = "UL-2";
    reading.timestamp = 200;
    reading.temperature = 35.0f;
    reading.humidity = 60.0f;
    reading.weight = 40.0f;
    reading.battery_level = 90;
    database.storeRawData(reading);
    database.shutdown();

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
