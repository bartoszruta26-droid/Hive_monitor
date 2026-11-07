/*
 * ApiaryGuard - Main Firmware File for Raspberry Pi Pico (RP2040)
 * Refactored version with modular architecture + Dual Mode (USB Priority + Ethernet Fallback)
 * 
 * This is the main entry point that includes all modules
 * 
 * Dual Mode Operation:
 * 1. Priority: USB (Serial). If host communication detected, listens for text commands.
 * 2. Fallback: Ethernet (W6100). If no USB activity, runs HTTP server.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>
#include <string.h>

// Include module headers
#include "config.h"
#include "sensors.h"
#include "hx711.h"
#include "audio_analysis.h"
#include "weight_analysis.h"
#include "air_quality.h"
#include "radar_analysis.h"
#include "network.h"
#include "effectors.h"
#include "effectors_flowing_hive.h"  // Flowing Hive effectors (servo, HX711 #2, flow sensor)
#include "safe_mode_policy.h"

// Global objects
EthernetServer server(8080);
EthernetUDP udp;
DHT dht(DHT_PIN, DHT22);
Adafruit_SGP41 sgp;
SerialUART& radarSerial = Serial2; // RP2040 UART1 on GPIO4 RX / GPIO5 TX

// Global sensor state
SensorState sensors;
AudioMetrics currentAudioMetrics;
HX711Metrics currentHX711Metrics;
AirQualityMetrics currentAirMetrics;
RadarMetrics currentRadarMetrics;

// Global sensor data variables (shared across modules)
float temperature = 0.0f;
float humidity = 0.0f;
uint16_t co2_eq = 0;
uint16_t voc_idx = 0;
uint16_t nox_idx = 0;
bool co2_available = false;

// Audio buffers
int16_t audioBuffer[AUDIO_BUFFER_SIZE];
float audioFFT[AUDIO_BUFFER_SIZE];

// Weight buffer
HX711DataPoint hx711Buffer[HX711_BUFFER_SIZE];

// Timing variables
unsigned long lastMillis = 0;

// Dual mode state
bool usbActive = false;
bool ethActive = false;
unsigned long lastActivityTime = 0;

// Forward declarations
void detectConnectionMode();
void initEthernet();
void handleUSBCommand();
void processCommand(String cmd);
void printStatus(Print& out);
void sendJsonResponse(EthernetClient& client);
void sendHtmlDashboard(EthernetClient& client);
void checkActuatorSafety(unsigned long now);

// Actuator settings
int heaterDuty = 0;
int fanDuty = 0;
int pumpDuty = 0;
bool relay1State = false;
bool relay2State = false;

/**
 * @brief Detect connection mode - USB priority with Ethernet fallback
 * 
 * Waits for USB host activity. If detected, enables USB command mode.
 * Otherwise, activates Ethernet mode automatically.
 */
void detectConnectionMode() {
    // Wait for potential USB host communication
    delay(1000);
    
    // Check if Serial data is available (indicates USB host)
    if (Serial.available() > 0 || usbActive) {
        Serial.println("Wykryto połączenie USB. Tryb główny: SERIAL.");
        Serial.println("Wpisz 'HELP' aby zobaczyć listę komend.");
        usbActive = true;
        lastActivityTime = millis();
    } else {
        Serial.println("Nie wykryto aktywności USB. Przejście w tryb Ethernet.");
        usbActive = false;
        ethActive = true; // Enable Ethernet mode
    }
}

/**
 * @brief Initialize Ethernet (placeholder - network already initialized in initW6100)
 */
void initEthernet() {
    // Network is already initialized in initW6100()
    // This function is kept for compatibility
    ethActive = true;
    Serial.print("Serwer HTTP uruchomiony na: http://");
    Serial.println(Ethernet.localIP());
}

/**
 * @brief Handle USB serial commands
 */
void handleUSBCommand() {
    static String buffer = "";
    static bool discardingOverlongCommand = false;
    const size_t maxCommandLength = 96;

    while (Serial.available()) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n') {
            if (discardingOverlongCommand) {
                Serial.println("ERROR: Komenda przekracza limit 96 znakow; odrzucono.");
            } else if (buffer.length() > 0) {
                processCommand(buffer);
            }
            buffer = "";
            discardingOverlongCommand = false;
        } else if (c != '\r' && !discardingOverlongCommand) {
            if (buffer.length() < maxCommandLength) {
                buffer += c;
            } else {
                buffer = "";
                discardingOverlongCommand = true;
            }
        }
    }
}

// Accept only a complete command name followed by one unsigned integer.
// Reject trailing characters, signs, empty arguments and out-of-range values.
static bool parseUnsignedArgument(const String& command, const char* name,
                                  unsigned long minValue, unsigned long maxValue,
                                  unsigned long& value) {
    const size_t nameLength = strlen(name);
    if (command.length() <= nameLength ||
        command.substring(0, nameLength) != name ||
        command.charAt(nameLength) != ' ') {
        return false;
    }

    String argument = command.substring(nameLength + 1);
    argument.trim();
    if (argument.length() == 0) return false;

    unsigned long parsed = 0;
    for (size_t i = 0; i < argument.length(); ++i) {
        const char ch = argument.charAt(i);
        if (ch < '0' || ch > '9') return false;
        const unsigned long digit = static_cast<unsigned long>(ch - '0');
        if (parsed > (maxValue - digit) / 10UL) return false;
        parsed = parsed * 10UL + digit;
    }

    if (parsed < minValue || parsed > maxValue) return false;
    value = parsed;
    return true;
}

static bool isExactCommand(const String& command, const char* name) {
    return command == name;
}

static bool isHeaterSensorReadyForRecovery(unsigned long now) {
    return safeModePolicy::heaterSensorReady(
        sensors.tempHum.detected, sensors.tempHum.active,
        sensors.tempHum.critical_error_count, sensors.tempHum.last_read_time,
        now, HEATER_SENSOR_MAX_AGE_MS, HEATER_SENSOR_MAX_ERRORS);
}

// Public guard used by the actuator layer before it writes non-zero heater PWM.
bool isHeaterSensorReadyForActuation() {
    return isHeaterSensorReadyForRecovery(millis());
}

/**
 * @brief Process strictly validated text commands from USB.
 */
void processCommand(String cmd) {
    cmd.trim();
    cmd.toUpperCase();

    unsigned long value = 0;

    if (parseUnsignedArgument(cmd, "SET_HEATER", 0, 255, value)) {
        if (isSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny aktywny; polecenie grzania odrzucone.");
            return;
        }
        if (!setHeaterPWM(static_cast<int>(value))) {
            Serial.println("ERROR: Polecenie grzania odrzucone przez zabezpieczenie wykonawcze.");
            return;
        }
        Serial.println("OK: Grzałka ustawiona na " + String(heaterDuty));
    }
    else if (parseUnsignedArgument(cmd, "SET_FAN", 0, 255, value)) {
        if (isSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny aktywny; polecenie wentylatora odrzucone.");
            return;
        }
        setFanPWM(static_cast<int>(value));
        Serial.println("OK: Wentylator ustawiony na " + String(fanDuty));
    }
    else if (parseUnsignedArgument(cmd, "SET_PUMP", 0, 255, value)) {
        if (isSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny aktywny; polecenie pompy odrzucone.");
            return;
        }
        pumpDuty = static_cast<int>(value);
        Serial.println("OK: Pompa ustawiona na " + String(pumpDuty));
    }
    else if (cmd == "SET_RELAY1 ON" || cmd == "SET_RELAY1 OFF") {
        if (isSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny aktywny; polecenie przekaźnika odrzucone.");
            return;
        }
        relay1State = (cmd == "SET_RELAY1 ON");
        Serial.println("OK: Przekaźnik 1: " + String(relay1State ? "ON" : "OFF"));
    }
    else if (cmd == "SET_RELAY2 ON" || cmd == "SET_RELAY2 OFF") {
        if (isSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny aktywny; polecenie przekaźnika odrzucone.");
            return;
        }
        relay2State = (cmd == "SET_RELAY2 ON");
        Serial.println("OK: Przekaźnik 2: " + String(relay2State ? "ON" : "OFF"));
    }
    // The longer command must be checked before CALIB_WEIGHT.
    else if (isExactCommand(cmd, "CALIB_WEIGHT_2")) {
        tareHX711_2();
        Serial.println("OK: Nadstawka wyzerowana (Tara HX711 #2).");
    }
    else if (isExactCommand(cmd, "CALIB_WEIGHT")) {
        long sum = 0;
        const int samples = 10;
        int validSamples = 0;
        Serial.println("Trwanie kalibracji...");

        for (int i = 0; i < samples; ++i) {
            const long reading = readHX711();
            if (reading != 0) {
                sum += reading;
                ++validSamples;
            }
            delay(10);
        }

        if (validSamples == 0) {
            Serial.println("ERROR: Brak prawidlowych probek HX711; kalibracja przerwana.");
            return;
        }

        hx711_offset = sum / validSamples;
        Serial.println("OK: Waga wyzerowana. Offset: " + String(hx711_offset));
    }
    else if (parseUnsignedArgument(cmd, "SERVO_ANGLE", SERVO_MIN_ANGLE,
                                   SERVO_MAX_ANGLE, value)) {
        if (isFlowingHiveSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny Flowing Hive aktywny.");
            return;
        }
        if (!setServoAngle(static_cast<uint16_t>(value))) {
            Serial.println("ERROR: Sterownik serwa nie jest gotowy; polecenie odrzucone.");
            return;
        }
        Serial.println("OK: Serwo ustawione na " + String(value) + " stopni.");
    }
    else if (cmd == "SERVO_EMPTY" ||
             parseUnsignedArgument(cmd, "SERVO_EMPTY", 1, 1800, value)) {
        if (isFlowingHiveSafeModeActive()) {
            Serial.println("ERROR: Tryb bezpieczny Flowing Hive aktywny.");
            return;
        }
        const unsigned long duration = (cmd == "SERVO_EMPTY")
            ? 1800000UL : value * 1000UL;
        if (!executeAutoEmptySequence(duration)) {
            Serial.println("ERROR: Nie uruchomiono sekwencji; sprawdź sterownik, tryb bezpieczny i czas.");
            return;
        }
        Serial.println("OK: Rozpoczęto sekwencję opróżniania (" +
                       String(duration / 1000UL) + "s).");
    }
    else if (isExactCommand(cmd, "FLOW_RESET")) {
        resetFlowCounter();
        Serial.println("OK: Licznik przepływu zresetowany.");
    }
    else if (isExactCommand(cmd, "SAFE_MODE_STATUS")) {
        Serial.println(String("MAIN_SAFE_MODE=") + (isSafeModeActive() ? "ACTIVE" : "INACTIVE"));
        Serial.println(String("MAIN_FAULT_REASON=") + getSafeModeReason());
        Serial.println(String("FLOWING_HIVE_SAFE_MODE=") + (isFlowingHiveSafeModeActive() ? "ACTIVE" : "INACTIVE"));
        Serial.println(String("FLOWING_HIVE_FAULT_REASON=") + getFlowingHiveSafeModeReason());
    }
    else if (isExactCommand(cmd, "SAFE_MODE_RELEASE CONFIRM")) {
        if (!releaseSafeMode(true)) {
            Serial.println("ERROR: Zwolnienie blokady glownej odrzucone; wymagana jest aktywna blokada, potwierdzenie operatora i aktualny, poprawny pomiar czujnika temperatury.");
        } else {
            Serial.println("OK: Glowna blokada zwolniona. Wszystkie nastawy pozostaja wyzerowane; Flowing Hive nadal zablokowany.");
        }
    }
    else if (isExactCommand(cmd, "FLOW_SAFE_MODE_RELEASE INSPECTED CONFIRM")) {
        if (!deactivateFlowingHiveSafeMode(true)) {
            Serial.println("ERROR: Zwolnienie Flowing Hive odrzucone; wymagane sa aktywna blokada, fizyczna kontrola pozycji spoczynkowej i zwolniona blokada glowna.");
        } else {
            Serial.println("OK: Blokada Flowing Hive zwolniona po potwierdzonej kontroli fizycznej. Przerwana sekwencja nie zostala wznowiona.");
        }
    }
    else if (isExactCommand(cmd, "STATUS_FLOWING")) {
        printFlowingHiveEffectorStatus();
    }
    else if (isExactCommand(cmd, "STATUS")) {
        printStatus(Serial);
    }
    else if (isExactCommand(cmd, "HELP")) {
        Serial.println("Dostępne komendy:");
        Serial.println("  SET_HEATER [0-255]");
        Serial.println("  SET_FAN [0-255]");
        Serial.println("  SET_PUMP [0-255]");
        Serial.println("  SET_RELAY1 [ON/OFF]");
        Serial.println("  SET_RELAY2 [ON/OFF]");
        Serial.println("  CALIB_WEIGHT");
        Serial.println("  CALIB_WEIGHT_2");
        Serial.println("  SERVO_ANGLE [0-180]");
        Serial.println("  SERVO_EMPTY [1-1800 sekund; domyślnie 1800]");
        Serial.println("  FLOW_RESET");
        Serial.println("  STATUS_FLOWING");
        Serial.println("  SAFE_MODE_STATUS");
        Serial.println("  SAFE_MODE_RELEASE CONFIRM");
        Serial.println("  FLOW_SAFE_MODE_RELEASE INSPECTED CONFIRM  (po fizycznej kontroli pozycji spoczynkowej)");
        Serial.println("  STATUS");
    }
    else {
        Serial.println("ERROR: Nieznana komenda lub nieprawidlowy argument. Wpisz HELP.");
    }
}

/**
 * @brief Print sensor status as JSON
 */
void printStatus(Print& out) {
    out.print("{\"temp\":"); out.print(temperature);
    out.print(", \"hum\":"); out.print(humidity);
    out.print(", \"weight\":"); out.print(hx711_value);
    out.print(", \"co2_available\":"); out.print(co2_available ? "true" : "false");
    out.print(", \"co2\":"); if (co2_available) out.print(co2_eq); else out.print("null");
    out.print(", \"voc\":"); out.print(voc_idx);
    out.print(", \"nox\":"); out.print(nox_idx);
    out.print(", \"heater\":"); out.print(heaterDuty);
    out.print(", \"fan\":"); out.print(fanDuty);
    out.print(", \"pump\":"); out.print(pumpDuty > 0 ? 1 : 0);
    out.print(", \"relay1\":"); out.print(relay1State ? 1 : 0);
    out.print(", \"relay2\":"); out.print(relay2State ? 1 : 0);
    out.print(", \"safe_mode\":"); out.print(isSafeModeActive() ? "true" : "false");
    out.print(", \"flowing_hive_safe_mode\":"); out.print(isFlowingHiveSafeModeActive() ? "true" : "false");
    // Additional metrics from analysis modules
    out.print(", \"audio_bee_idx\":"); out.print(currentAudioMetrics.bee_activity_index, 2);
    out.print(", \"audio_swarm_prob\":"); out.print(currentAudioMetrics.swarm_probability, 2);
    out.print(", \"weight_rate\":"); out.print(currentHX711Metrics.current_rate, 3);
    out.print(", \"weight_health\":"); out.print(currentHX711Metrics.hive_health_weight, 1);
    out.print(", \"air_iaq\":"); out.print(currentAirMetrics.iaq_index, 1);
    out.print(", \"radar_activity\":"); out.print(currentRadarMetrics.activity_ratio, 2);
    out.println("}");
}

/**
 * @brief Send JSON response to HTTP client
 */
void sendJsonResponse(EthernetClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    printStatus(client);
}

/**
 * @brief Send HTML dashboard to HTTP client
 */
void sendHtmlDashboard(EthernetClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><title>Pico Controller</title>");
    client.println("<meta http-equiv='refresh' content='5'>");
    client.println("</head><body>");
    client.println("<h1>Panel Sterowania Pico</h1>");
    client.print("<p>Temperatura: "); client.print(temperature); client.println(" C</p>");
    client.print("<p>Wilgotność: "); client.print(humidity); client.println(" %</p>");
    client.print("<p>Waga: "); client.print(hx711_value); client.println(" kg</p>");
    client.print("<p>CO2: ");
    if (co2_available) {
        client.print(co2_eq);
        client.println(" ppm</p>");
    } else {
        client.println("niedostępne — SGP41 nie mierzy CO2</p>");
    }
    client.print("<p>VOC index: "); client.print(voc_idx); client.println("</p>");
    client.print("<p>NOx index: "); client.print(nox_idx); client.println("</p>");
    
    client.println("<p>Panel tylko do odczytu. Sterowanie wykonuj przez lokalny port USB.</p>");
    client.println("<p><a href='/status'>Otworz dane JSON</a></p>");
    
    client.println("</body></html>");
}

// Heater interlock: a heater request is permitted only while the
// temperature sensor is present and producing fresh, valid measurements.
// This is a software fault interlock, not an independent thermal cutoff.
void checkActuatorSafety(unsigned long now) {
    if (isSafeModeActive() || heaterDuty <= 0) return;

    if (!isHeaterSensorReadyForRecovery(now)) {
        activateSafeMode("Heater interlock: temperature sensor unavailable, invalid or stale");
    }
}

void setup() {
    Serial.begin(115200);
    while(!Serial && (millis() < 3000));
    
    Serial.println("=== ApiaryGuard Pico v3.0 (Refactored + Dual Mode) ===");
    Serial.println(">> USB Priority + Ethernet Fallback");
    
    // Enable Watchdog (8 second timeout)
    rp2040.wdt_begin(8000);
    Serial.println(">> Watchdog enabled (timeout: 8s)");
    
    // Initialize subsystems
    initI2C();
    initUART();
    initPWM();
    initGPIO();
    
    // Detect and initialize sensors
    Serial.println(">> Scanning and detecting sensors...");
    detectAllSensors();
    printSensorStatus();
    initSensors();
    
    // Initialize Flowing Hive effectors (conditionally - only if hardware detected)
    Serial.println(">> Initializing Flowing Hive effectors...");
    initServoControl();      // Servo for frame emptying
    initHX711_2();           // Second HX711 for superstructure weight
    initFlowSensor();        // Flow sensor for honey output
    
    // Initialize network (will be activated if no USB host detected)
    initW6100();
    
    // Detect connection mode (USB priority or Ethernet fallback)
    detectConnectionMode();
    
    Serial.println(">> System ready.");
}

void loop() {
    unsigned long now = millis();
    
    // Feed watchdog
    rp2040.wdt_reset();
    
    // Maintain Ethernet connection
    Ethernet.maintain();
    
    // Read sensors conditionally (only if detected)
    readSensors(now);

    // Sensor measurements are refreshed before control commands are processed.
    // The heater interlock is evaluated immediately before outputs are applied,
    // so a command received later in this loop cannot bypass the check.

    // Process audio analysis periodically
    processAudioPeriodically(now);
    
    // Process weight analysis
    processWeightPeriodically(now);
    
    // Process air quality
    processAirQualityPeriodically(now);
    
    // Process radar data
    processRadarPeriodically(now);
    
    // Update Flowing Hive effectors (servo loop and flow sensor)
    updateServoLoop();       // Check auto-empty sequence completion
    updateFlowSensor();      // Calculate flow rate from pulse count
    
    // Check for USB activity
    if (Serial.available()) {
        usbActive = true;
        lastActivityTime = now;
        handleUSBCommand();
    }
    
    // Dual mode logic: USB priority, Ethernet fallback
    if (usbActive && (now - lastActivityTime < 5000)) {
        // USB mode active - continue listening for commands
    } else {
        // No USB activity - switch to Ethernet mode
        if (!ethActive) {
            Serial.println("Brak aktywności USB. Aktywacja trybu Ethernet...");
            ethActive = true;
        }
        
        if (Ethernet.linkStatus() == LinkON) {
            // Handle HTTP clients
            handleClients();
            
            // Send UDP data to RPi
            sendUDPData();
        }
    }
    
    // Final actuator interlock: evaluate after all command paths in this loop
    // and immediately before applying physical outputs.
    checkActuatorSafety(millis());

    // Update outputs
    updateOutputs();
}
