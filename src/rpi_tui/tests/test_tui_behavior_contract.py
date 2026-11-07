#!/usr/bin/env python3
"""Regression checks for the Bash TUI's safety-critical interaction contracts."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
TUI = ROOT / "src/rpi_tui/apiary_tui.sh"
source = TUI.read_text(encoding="utf-8")

# set -e is intentional: arithmetic commands must not return a failure status
# during ordinary navigation.
assert "set -e" in source
for expression in (
    "((SCROLL_OFFSET_LOG++))",
    "((SCROLL_OFFSET_DEBUG++))",
    "((SCROLL_OFFSET_LOG--))",
    "((SCROLL_OFFSET_DEBUG--))",
    "((CURRENT_TAB = (CURRENT_TAB + 1) % TAB_COUNT))",
    "((CURRENT_TAB = (CURRENT_TAB - 1 + TAB_COUNT) % TAB_COUNT))",
):
    assert expression not in source, f"unsafe arithmetic remains: {expression}"

# The right-arrow wrap from the final tab to tab zero must survive errexit.
subprocess.run(
    [
        "bash",
        "-e",
        "-c",
        "tab=3; count=4; tab=$(((tab + 1) % count)); "
        "offset=0; offset=$((offset + 1)); "
        "offset=$((offset - 1)); test \"$tab\" -eq 0; "
        "test \"$offset\" -eq 0",
    ],
    check=True,
)

# Rendering must not poll the collector on every terminal repaint.
assert 'if [ "$AUTO_REFRESH" = true ]; then' in source
assert "now - LAST_REFRESH >= REFRESH_INTERVAL" in source
assert 'MANUAL_REFRESH_REQUESTED=true' in source
assert "fetch_hive_data" in source

# Missing measurements and empty logs must never be fabricated as live data.
assert "BRAK DANYCH — oczekiwanie" in source
assert "BRAK LOGÓW — nie zarejestrowano zdarzeń" in source
assert "BRAK DANYCH DEBUG — brak zarejestrowanych komunikatów" in source
assert "Wysoka temperatura w UL-002: 38°C" not in source
assert 'local temp="${HIVE_TEMP[$i]:-—}"' in source
assert 'local status="${HIVE_STATUS[$i]:-UNKNOWN}"' in source

# The auto-refresh toggle must assign a real boolean value.
assert "AUTO_REFRESH=!$AUTO_REFRESH" not in source
assert "AUTO_REFRESH=false" in source
assert "AUTO_REFRESH=true" in source


# Unsupported telemetry fields must remain visibly unavailable instead of
# looking like legitimate zero-valued measurements.
for field in (
    "HIVE_BEE_ACTIVITY",
    "HIVE_SPECTRAL_CENTROID",
    "HIVE_AUDIO_HEALTH",
    "HIVE_RADAR_HEALTH",
    "HIVE_RADAR_ANOMALY",
    "HIVE_NECTAR_INFLOW",
    "HIVE_COLONY_GROWTH",
    "HIVE_PRODUCTIVITY",
    "HIVE_HEAT_INDEX",
    "HIVE_COMFORT_INDEX",
    "HIVE_BROOD_STRESS",
    "HIVE_VENTILATION_NEED",
    "HIVE_PIEZO_ACTIVITY",
    "HIVE_PREDATOR_SCORE",
    "HIVE_PRESSURE",
    "HIVE_WEATHER_TREND",
    "HIVE_FORAGING_COND",
    "HIVE_LUX",
    "HIVE_CIRCADIAN_SYNC",
):
    assert f'{field}+=("—")' in source, f"{field} must show unavailable"
    assert f'{field}+=("0")' not in source, f"{field} must not fabricate zero"

print("TUI behavior contract: PASS")
