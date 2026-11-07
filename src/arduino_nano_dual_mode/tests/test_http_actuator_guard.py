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


# The heater must remain de-energized at boot and reject positive duty unless
# a valid DHT22 sample is no older than the configured freshness window.
assert "bool dhtTemperatureValid = false;" in text
assert "const unsigned long DHT_MAX_AGE_MS = 5000;" in text
assert "dhtTemperatureValid = false;" in text
assert "dhtTemperatureValid = true;" in text
assert "millis() - lastDhtGoodAt > DHT_MAX_AGE_MS" in text
assert 'logWarning("HEATER", "Command rejected: DHT22 sample invalid or stale")' in text
setup = text[text.index("void setup() {"):]
assert setup.index("analogWrite(HEATER_PWM, 0);") < setup.index("// DHT22")

assert "void enforceHeaterSafety()" in text
assert 'logWarning("HEATER", "Output forced OFF: DHT22 invalid or stale")' in text
loop = text[text.index("void loop() {"):]
assert "enforceHeaterSafety();" in loop
assert loop.index("enforceHeaterSafety();") < loop.index("handleEthernetServer();")


print("Arduino Nano HTTP actuator guard and heater interlock: PASS")
