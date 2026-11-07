#!/usr/bin/env python3
"""Guard the WebUI's same-origin API routing contract."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
APP = (ROOT / "src/webui/app.js").read_text(encoding="utf-8")
PROXY = (ROOT / "src/webui/api.php").read_text(encoding="utf-8")

assert "api.php?endpoint=health" in APP
assert "api.php?endpoint=hives" in APP
assert "api.php?endpoint=history" in APP
assert "api.php?endpoint=logs" in APP
assert ":8080" not in APP, "browser must not connect directly to collector port"
assert "window.location.protocol}//${window.location.hostname}" not in APP
assert "'hives' => '/api/hives'" in PROXY
assert "'health' => '/health'" in PROXY
assert "function callCollector(" in PROXY
assert "data.collector === 'connected' && data.mode === 'live'" in APP
assert "API działa, kolektor jest odłączony" in APP
assert "Usługa API jest niedostępna" in APP
assert "function saveLocalPreferences()" in APP
assert "function saveConfigToBackend()" not in APP

print("WebUI same-origin API proxy contract: PASS")
