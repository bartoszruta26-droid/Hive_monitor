#!/usr/bin/env python3
"""Static regression checks for security properties that do not require hardware."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


collector = (ROOT / "src/rpi_tui/apiary_collector.cpp").read_text(encoding="utf-8")
network = (ROOT / "src/rpi_tui/network_security.h").read_text(encoding="utf-8")
protocol = (ROOT / "src/rpi_tui/http_protocol.h").read_text(encoding="utf-8")
database = (ROOT / "src/rpi_tui/apiary_database.cpp").read_text(encoding="utf-8")
database_header = (ROOT / "src/rpi_tui/apiary_database.h").read_text(encoding="utf-8")
measurement_validation = (ROOT / "src/rpi_tui/measurement_validation.h").read_text(encoding="utf-8")\ncollector_types = (ROOT / "src/rpi_tui/apiary_collector_types.h").read_text(encoding="utf-8")
firmware_sensors = (ROOT / "src/pico_refactored/src/sensors.cpp").read_text(encoding="utf-8")
firmware_flow = (ROOT / "src/pico_refactored/src/effectors_flowing_hive.cpp").read_text(encoding="utf-8")
php_api = (ROOT / "src/webui/api.php").read_text(encoding="utf-8")

require("recvmsg(server_socket" in collector and "datagramWasTruncated" in collector,
        "UDP receive path must detect truncated datagrams")
require("INADDR_ANY" not in collector and "inet_ntoa(" not in collector,
        "collector must not bind wildcard interfaces or use inet_ntoa")
require("Access-Control-Allow-Origin: *" not in collector,
        "collector must not emit permissive wildcard CORS")
require("parseIPv4BindAddress" in collector and "127.0.0.1" in collector,
        "collector must use validated loopback-by-default bind addresses")
require("MAX_TRACKED_IPS = 4096" in network and "cleanupLocked" in network,
        "rate limiter must remain bounded and periodically cleaned")
require("inet_ntop" in network and "inet_pton" in network,
        "network address conversion and parsing must use reentrant APIs")
for route in ("sensors", "effectors", "history", "logs"):
    require(f'path == "/api/{route}") return {{HttpRoute::' in protocol,
            f"route /api/{route} must remain explicitly registered")
require("return {HttpRoute::Sensors, 501}" in protocol and
        "return {HttpRoute::Effectors, 501}" in protocol and
        "return {HttpRoute::History, 501}" in protocol and
        "return {HttpRoute::Logs, 501}" in protocol,
        "unimplemented collector routes must return HTTP 501")

dht_start = firmware_sensors.index("bool detectDHT() {")
dht_end = firmware_sensors.index("\n}\n", dht_start)
dht_function = firmware_sensors[dht_start:dht_end]
require("delay(2000)" in dht_function and "delay(100)" not in dht_function,
        "DHT detection must respect the minimum sampling interval")

require("flowSensorInterruptConfigured" in firmware_flow and
        "flowSensorInterruptReady" not in firmware_flow,
        "flow diagnostics must distinguish interrupt setup from physical detection")
require("csvEscape(rec.hive_id)" in database,
        "CSV export must escape hive identifiers")
require("raw_records_ += batch_size" in database and
        "accepted_records_" in database and "buffered_records" in database,
        "database statistics must distinguish accepted, buffered and committed records")
require("CURLOPT_CONNECTTIMEOUT" in php_api and
        "callCollector('/health', 'GET', null, 3)" in php_api,
        "PHP collector health checks must use a bounded connection and total timeout")

require("validateHivePayloadFields" in collector and "json_validation_errors" in collector,
        "collector must validate measurement payloads before conversion and persistence")
require("measurementRanges()" in measurement_validation and "std::isfinite(value)" in measurement_validation,
        "measurement schema must enforce finite, field-specific numeric bounds")

parse_start = collector.index("void parseJSON")
parse_end = collector.index("void parseCSV", parse_start)
parser_keys = set(re.findall(r'getValue\("([^"]+)"\)', collector[parse_start:parse_end]))
range_keys = set(re.findall(r'\{"([^"]+)",\s*\{', measurement_validation))
unvalidated_keys = parser_keys - {"hive_id", "data_source"} - range_keys
require(not unvalidated_keys,
        "every numeric key consumed by parseJSON must have a domain rule: " + ", ".join(sorted(unvalidated_keys)))

struct_fields = re.findall(
    r'^\s*(std::string|bool|int|long long|unsigned(?:\s+\w+)?|float|double|size_t)\s+(\w+)\s*(?:=\s*([^;]+))?;\s*(?://\s*(.*))?
require("lifecycle_mutex_" in database_header and "accepting_" in database_header,
        "SQLite lifecycle must serialize initialization, producers and shutdown")
require("if (!flushBuffer())" in database and "ROLLBACK;" in database and
        "sqlite3_busy_timeout" in database,
        "SQLite shutdown must preserve failed batches and use bounded busy waits")


print("Software security contract checks passed")
,
    collector_types, re.MULTILINE
)
numeric_struct_fields = {name for kind, name, _, _ in struct_fields if kind not in {"std::string", "bool"}}
unvalidated_struct_fields = numeric_struct_fields - range_keys
require(not unvalidated_struct_fields,
        "every numeric HiveData field must have an explicit domain rule: " +
        ", ".join(sorted(unvalidated_struct_fields)))

require("lifecycle_mutex_" in database_header and "accepting_" in database_header,
        "SQLite lifecycle must serialize initialization, producers and shutdown")
require("if (!flushBuffer())" in database and "ROLLBACK;" in database and
        "sqlite3_busy_timeout" in database,
        "SQLite shutdown must preserve failed batches and use bounded busy waits")


print("Software security contract checks passed")
