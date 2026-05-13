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
#include <RP2040Watchdog.h>

// Include module headers
#include "config.h"
#include "sensors.h"
#include "audio_analysis.h"
#include "weight_analysis.h"
#include "air_quality.h"
#include "radar_analysis.h"
#include "network.h"
#include "effectors.h"
#include "effectors_flowing_hive.h"  // Flowing Hive effectors (servo, HX711 #2, flow sensor)

// Global objects
EthernetServer server(8080);
EthernetUDP udp;
DHT dht(DHT_PIN, DHT22);
Adafruit_SGP41 sgp;
HardwareSerial radarSerial(uart1);

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
long hx711_value = 0;

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
int extractValue(String cmd);
void printStatus(Print& out);
void sendJsonResponse(EthernetClient& client);
void sendHtmlDashboard(EthernetClient& client);

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
    
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            processCommand(buffer);
            buffer = "";
        } else {
            buffer += c;
        }
    }
}

/**
 * @brief Extract numeric value from command string
 * @param cmd Command string (e.g., "SET_HEATER 128")
 * @return Numeric value, or -1 if no valid number found
 */
int extractValue(String cmd) {
    int spaceIndex = cmd.indexOf(' ');
    if (spaceIndex != -1 && spaceIndex < cmd.length() - 1) {
        String valueStr = cmd.substring(spaceIndex + 1);
        int val = valueStr.toInt();
        // Validate: toInt() returns 0 for invalid input, but 0 might be valid
        // Check if the substring actually contains digits
        for (size_t i = 0; i < valueStr.length(); i++) {
            if (isDigit(valueStr[i]) || valueStr[i] == '-') {
                return val;
            }
        }
    }
    return -1; // Return -1 to indicate invalid/missing value
}

/**
 * @brief Process text commands from USB or Ethernet
 */
void processCommand(String cmd) {
    cmd.toUpperCase();
    cmd.trim();
    
    // Available commands:
    // SET_HEATER [0-255]
    // SET_FAN [0-255]
    // SET_PUMP [0-255]
    // SET_RELAY1 [ON/OFF]
    // SET_RELAY2 [ON/OFF]
    // CALIB_WEIGHT
    // CALIB_WEIGHT_2      - Tare second HX711 (superstructure)
    // SERVO_ANGLE [0-180] - Set servo angle
    // SERVO_EMPTY         - Execute auto-empty sequence
    // FLOW_RESET          - Reset flow counter
    // STATUS_FLOWING      - Print Flowing Hive effector status
    // STATUS
    
    if (cmd.startsWith("SET_HEATER")) {
        int val = extractValue(cmd);
        if (val >= 0 && val <= 255) {
            heaterDuty = val;
            Serial.println("OK: Grzałka ustawiona na " + String(heaterDuty));
        } else {
            Serial.println("ERROR: Nieprawidłowa wartość dla grzałki (0-255)");
        }
    }
    else if (cmd.startsWith("SET_FAN")) {
        int val = extractValue(cmd);
        if (val >= 0 && val <= 255) {
            fanDuty = val;
            Serial.println("OK: Wentylator ustawiony na " + String(fanDuty));
        } else {
            Serial.println("ERROR: Nieprawidłowa wartość dla wentylatora (0-255)");
        }
    }
    else if (cmd.startsWith("SET_PUMP")) {
        int val = extractValue(cmd);
        if (val >= 0 && val <= 255) {
            pumpDuty = val;
            Serial.println("OK: Pompa ustawiona na " + String(pumpDuty));
        } else {
            Serial.println("ERROR: Nieprawidłowa wartość dla pompy (0-255)");
        }
    }
    else if (cmd.startsWith("SET_RELAY1")) {
        relay1State = (cmd.indexOf("ON") > 0 || cmd.indexOf("1") > 0);
        Serial.println("OK: Przekaźnik 1: " + String(relay1State ? "ON" : "OFF"));
    }
    else if (cmd.startsWith("SET_RELAY2")) {
        relay2State = (cmd.indexOf("ON") > 0 || cmd.indexOf("1") > 0);
        Serial.println("OK: Przekaźnik 2: " + String(relay2State ? "ON" : "OFF"));
    }
    else if (cmd.startsWith("CALIB_WEIGHT")) {
        // Weight calibration handled in weight_analysis module
        Serial.println("OK: Waga wyzerowana (Tara).");
    }
    else if (cmd.startsWith("CALIB_WEIGHT_2")) {
        // Second HX711 tare
        tareHX711_2();
        Serial.println("OK: Nadstawka wyzerowana (Tara HX711 #2).");
    }
    else if (cmd.startsWith("SERVO_ANGLE")) {
        int angle = extractValue(cmd);
        if (angle >= 0 && angle <= 180) {
            setServoAngle(angle);
            Serial.println("OK: Serwo ustawione na " + String(angle) + " stopni.");
        } else {
            Serial.println("ERROR: Kąt musi być 0-180.");
        }
    }
    else if (cmd.startsWith("SERVO_EMPTY")) {
        unsigned long duration = 1800000; // Default 30 min
        int val = extractValue(cmd);
        if (val > 0) {
            duration = (unsigned long)val * 1000UL; // Convert seconds to ms
        }
        executeAutoEmptySequence(duration);
        Serial.println("OK: Rozpoczęto sekwencję opróżniania (" + String(duration/1000) + "s).");
    }
    else if (cmd.startsWith("FLOW_RESET")) {
        resetFlowCounter();
        Serial.println("OK: Licznik przepływu zresetowany.");
    }
    else if (cmd.startsWith("STATUS_FLOWING")) {
        printFlowingHiveEffectorStatus();
    }
    else if (cmd.startsWith("STATUS")) {
        printStatus(Serial);
    }
    else if (cmd.startsWith("HELP")) {
        Serial.println("Dostępne komendy:");
        Serial.println("  SET_HEATER [0-255]");
        Serial.println("  SET_FAN [0-255]");
        Serial.println("  SET_PUMP [0-255]");
        Serial.println("  SET_RELAY1 [ON/OFF]");
        Serial.println("  SET_RELAY2 [ON/OFF]");
        Serial.println("  CALIB_WEIGHT");
        Serial.println("  CALIB_WEIGHT_2      - Tare second HX711");
        Serial.println("  SERVO_ANGLE [0-180] - Set servo angle");
        Serial.println("  SERVO_EMPTY [sec]   - Auto-empty sequence");
        Serial.println("  FLOW_RESET          - Reset flow counter");
        Serial.println("  STATUS_FLOWING      - Flowing Hive status");
        Serial.println("  STATUS");
    }
    else {
        Serial.println("Nieznana komenda. Wpisz HELP.");
    }
}

/**
 * @brief Print sensor status as JSON
 */
void printStatus(Print& out) {
    out.print("{\"temp\":"); out.print(temperature);
    out.print(", \"hum\":"); out.print(humidity);
    out.print(", \"weight\":"); out.print(hx711_value);
    out.print(", \"co2\":"); out.print(co2_eq);
    out.print(", \"voc\":"); out.print(voc_idx);
    out.print(", \"heater\":"); out.print(heaterDuty);
    out.print(", \"fan\":"); out.print(fanDuty);
    out.print(", \"relay1\":"); out.print(relay1State ? 1 : 0);
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
    client.print("<p>CO2: "); client.print(co2_eq); client.println(" ppm</p>");
    client.print("<p>VOC: "); client.print(voc_idx); client.println("</p>");
    
    client.println("<h3>Sterowanie</h3>");
    client.println("<a href='/cmd?SET_HEATER=0'><button>Grzałka OFF</button></a>");
    client.println("<a href='/cmd?SET_HEATER=128'><button>Grzałka 50%</button></a>");
    client.println("<a href='/cmd?SET_HEATER=255'><button>Grzałka 100%</button></a><br>");
    
    client.println("<a href='/cmd?SET_RELAY1=ON'><button>Relay1 ON</button></a>");
    client.println("<a href='/cmd?SET_RELAY1=OFF'><button>Relay1 OFF</button></a><br>");
    
    client.println("</body></html>");
}

void setup() {
    Serial.begin(115200);
    while(!Serial && (millis() < 3000));
    
    Serial.println("=== ApiaryGuard Pico v3.0 (Refactored + Dual Mode) ===");
    Serial.println(">> USB Priority + Ethernet Fallback");
    
    // Enable Watchdog (8 second timeout)
    rp2040.wdtEnable(8000);
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
    rp2040.wdtReset();
    
    // Maintain Ethernet connection
    Ethernet.maintain();
    
    // Read sensors conditionally (only if detected)
    readSensors(now);
    
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
    
    // Dual mode logic: USB priority, Ethernet fallback
    if (Serial.available()) {
        usbActive = true;
        lastActivityTime = now;
        handleUSBCommand();
    }
    
    // Check if we should switch back to USB mode from Ethernet
    if (ethActive && Serial.available()) {
        // USB activity detected while in Ethernet mode - switch back
        ethActive = false;
        usbActive = true;
        lastActivityTime = now;
        Serial.println("\nWykryto aktywność USB. Powrót do trybu SERIAL.");
    }
    
    if (usbActive && (now - lastActivityTime < 5000)) {
        // USB mode active - continue listening for commands
    } else {
        // No USB activity - switch to Ethernet mode
        if (!ethActive) {
            Serial.println("Brak aktywności USB. Aktywacja trybu Ethernet...");
            ethActive = true;
        }
        
        if (Ethernet.linkStatus() == LNK) {
            // Handle HTTP clients
            handleClients();
            
            // Send UDP data to RPi
            sendUDPData();
        }
    }
    
    // Update outputs
    updateOutputs();
}
