#!/usr/bin/env python3
"""Regression test: TUI column indexes must match the collector CSV header."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
COLLECTOR = ROOT / "src/rpi_tui/apiary_collector.cpp"
TUI = ROOT / "src/rpi_tui/apiary_tui.sh"

source = COLLECTOR.read_text(encoding="utf-8")
start = source.index("std::string getStatusCSV()")
end = source.index("return csv.str();", start)
csv_body = source[start:end]
header_fragments = re.findall(r'csv\s*<<\s*"([^"]*)"\s*;', csv_body)
if not header_fragments:
    raise SystemExit("FAIL: collector CSV header was not found")
headers = [name.replace(r"\n", "").strip() for name in "".join(header_fragments).split(",")]

expected = {
    "id": "ID",
    "status": "STATUS",
    "timestamp": "TIMESTAMP",
    "temp": "TEMP",
    "hum": "HUM",
    "weight": "WEIGHT",
    "bat": "BAT",
    "co2": "CO2",
    "voc": "VOC",
    "motion": "MOTION",
    "online": "ONLINE",
    "audio_rms": "AUDIO_RMS",
    "audio_freq": "AUDIO_DOMINANT_FREQ",
    "swarm_prob": "AUDIO_SWARM_PROB",
    "radar_dist": "RADAR_DISTANCE",
    "radar_energy": "RADAR_ENERGY",
    "radar_act": "RADAR_ACTIVITY",
    "wag_rate": "HX711_RATE",
    "wag_trend": "HX711_DIRECTION",
    "air_iaq": "AIR_IAQ",
}

tui = TUI.read_text(encoding="utf-8")
for variable, column_name in expected.items():
    match = re.search(rf'\b{variable}="\$\{{columns\[(\d+)\]\}}"', tui)
    if not match:
        raise SystemExit(f"FAIL: TUI assignment for {variable} not found")
    actual_index = int(match.group(1))
    expected_index = headers.index(column_name)
    if actual_index != expected_index:
        raise SystemExit(
            f"FAIL: {variable} uses column {actual_index}, "
            f"but {column_name} is column {expected_index}"
        )

if len(headers) != 272:
    raise SystemExit(f"FAIL: expected 272 CSV columns, found {len(headers)}")
if "< 272" not in tui:
    raise SystemExit("FAIL: TUI does not reject truncated CSV rows")

print(f"PASS: TUI mapping matches all {len(expected)} checked columns in {len(headers)}-column CSV.")
