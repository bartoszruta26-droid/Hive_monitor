#!/usr/bin/env python3
"""Regression guard: Nano HTTP must not expose unauthenticated actuator writes."""
from pathlib import Path

source = Path(__file__).resolve().parents[1] / "apiaryguard_nano_dual.ino"
text = source.read_text(encoding="utf-8")
start = text.index("void handleEthernetServer()")
end = text.index("/**\n * Wysyłanie heartbeat przez Ethernet", start)
handler = text[start:end]

assert "legacy_control_disabled" in handler
assert 'request.startsWith("GET /heater/")' in handler
assert 'request.startsWith("GET /fan/")' in handler
assert 'request.startsWith("GET /pump/")' in handler
assert 'request.startsWith("GET /api/cmd?")' in handler
for forbidden in ("setPWM(", "setRelay(", "handleHTTPCommand("):
    assert forbidden not in handler, f"unauthenticated HTTP handler calls {forbidden}"

print("Arduino Nano HTTP actuator guard: PASS")
