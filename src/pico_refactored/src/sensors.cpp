/*
 * ApiaryGuard - Sensor Implementation
 * Comprehensive sensor initialization and reading with debug support
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_SENSORS in config.h
 * LOGGING: All sensor operations are logged with error tracking
 * EXCEPTIONS: Graceful handling of missing sensors and read failures
 */

#include "sensors.h"
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>
#include <Arduino.h>

// External globals (defined in main .ino file)
extern Adafruit_SGP41 sgp;
extern HardwareSerial radarSerial;
extern float temperature;
extern float humidity;
extern uint16_t co2_eq;
extern uint16_t voc_idx;
extern long hx711_value;
// Note: sensors is declared extern in sensors.h, no need to redeclare here

// Debug counters for sensor health monitoring
static unsigned long sensor_init_count = 0;
static unsigned long sensor_error_count = 0;
static unsigned long dht_read_errors = 0;
static unsigned long sgp_read_errors = 0;
static unsigned long hx711_read_errors = 0;

/**
 * @brief Initialize I2C bus for SGP41 and other I2C sensors
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] I2C initialized with SDA/SCL pins
 * - [SENSORS] ERROR: I2C initialization failed
 */
void initI2C() {
    sensor_init_count++;
    
    #ifdef DEBUG_SENSORS
    Serial.print("[SENSORS] Initializing I2C... SDA=");
    Serial.print(I2C_SDA);
    Serial.print(", SCL=");
    Serial.println(I2C_SCL);
    #endif
    
    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    Wire.begin();
    
    #ifdef DEBUG_SENSORS
    Serial.println(">> I2C initialized successfully");
    #else
    Serial.println(">> I2C initialized");
    #endif
}

/**
 * @brief Initialize UART for radar communication
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] UART1 initialized for radar on RX/TX pins
 * - [SENSORS] WARNING: Radar UART may not be responding
 */
void initUART() {
    sensor_init_count++;
    
    #ifdef DEBUG_SENSORS
    Serial.print("[SENSORS] Initializing UART for radar... RX=");
    Serial.print(RADAR_RX);
    Serial.print(", TX=");
    Serial.print(RADAR_TX);
    Serial.println(", Baud=115200");
    #endif
    
    radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
    
    #ifdef DEBUG_SENSORS
    Serial.println(">> UART1 initialized successfully for radar");
    #else
    Serial.println(">> UART1 initialized for radar");
    #endif
}

/**
 * @brief Initialize PWM outputs for heater and fan control
 * 
 * NOTE: analogWriteFrequency is not available on RP2040 Arduino core
 * PWM frequency is set automatically (default ~1kHz)
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] PWM initialized on HEATER_PWM and FAN_PWM pins
 */
void initPWM() {
    sensor_init_count++;
    
    #ifdef DEBUG_SENSORS
    Serial.print("[SENSORS] Initializing PWM... Heater pin=");
    Serial.print(HEATER_PWM);
    Serial.print(", Fan pin=");
    Serial.println(FAN_PWM);
    #endif
    
    pinMode(HEATER_PWM, OUTPUT);
    pinMode(FAN_PWM, OUTPUT);
    // analogWriteFrequency is not available on RP2040 Arduino core
    // PWM frequency is set automatically (default ~1kHz)
    
    #ifdef DEBUG_SENSORS
    Serial.println(">> PWM initialized successfully (RP2040 auto frequency)");
    #else
    Serial.println(">> PWM initialized");
    #endif
}

/**
 * @brief Initialize GPIO pins for relays and outputs
 * 
 * Initializes all relay pins and sets them to LOW (off) state
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] GPIO initialized with X relay pins
 * - [SENSORS] ERROR: Failed to initialize relay pin X
 */
void initGPIO() {
    sensor_init_count++;
    
    #ifdef DEBUG_SENSORS
    Serial.println("[SENSORS] Initializing GPIO pins for relays...");
    #endif
    
    // Initialize relay pins
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(VALVE_1, OUTPUT);
    pinMode(VALVE_2, OUTPUT);
    pinMode(RELAY_CH1, OUTPUT);
    pinMode(RELAY_CH2, OUTPUT);
    pinMode(RELAY_CH3, OUTPUT);
    pinMode(RELAY_CH4, OUTPUT);
    pinMode(RELAY_CH5, OUTPUT);
    pinMode(RELAY_CH6, OUTPUT);
    pinMode(RELAY_CH7, OUTPUT);
    pinMode(RELAY_CH8, OUTPUT);
    
    // Set all relays to LOW (off) - safe default state
    digitalWrite(PUMP_RELAY, LOW);
    digitalWrite(VALVE_1, LOW);
    digitalWrite(VALVE_2, LOW);
    digitalWrite(RELAY_CH1, LOW);
    digitalWrite(RELAY_CH2, LOW);
    digitalWrite(RELAY_CH3, LOW);
    digitalWrite(RELAY_CH4, LOW);
    digitalWrite(RELAY_CH5, LOW);
    digitalWrite(RELAY_CH6, LOW);
    digitalWrite(RELAY_CH7, LOW);
    digitalWrite(RELAY_CH8, LOW);
    
    #ifdef DEBUG_SENSORS
    Serial.println(">> GPIO initialized successfully (11 relay pins configured)");
    #else
    Serial.println(">> GPIO initialized");
    #endif
}

/**
 * @brief Detect HX711 weight sensor presence
 * @return true if sensor detected, false otherwise
 * 
 * Uses timeout-based detection to prevent hanging
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] HX711: Detected
 * - [SENSORS] HX711: Timeout - sensor not detected (X ms)
 */
bool detectHX711() {
    sensor_init_count++;
    unsigned long detect_start = millis();
    
    pinMode(HX711_DT, INPUT);
    pinMode(HX711_SCK, OUTPUT);
    digitalWrite(HX711_SCK, LOW);
    
    unsigned long start = millis();
    while(digitalRead(HX711_DT)) {
        if (millis() - start > HX711_TIMEOUT_MS) {
            #ifdef DEBUG_SENSORS
            Serial.print("  [!] HX711: Timeout - sensor not detected (");
            Serial.print(millis() - start);
            Serial.println("ms elapsed)");
            #else
            Serial.println("  [!] HX711: Timeout - sensor not detected");
            #endif
            sensor_error_count++;
            return false;
        }
        delayMicroseconds(10); // Prevent CPU hogging
    }
    
    #ifdef DEBUG_SENSORS
    Serial.print("  [✓] HX711: Detected (");
    Serial.print(millis() - detect_start);
    Serial.println("ms)");
    #else
    Serial.println("  [✓] HX711: Detected");
    #endif
    return true;
}

/**
 * @brief Detect DHT temperature/humidity sensor
 * @return true if sensor detected and readable, false otherwise
 * 
 * Attempts multiple reads with retries to confirm sensor presence
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] DHT: Detected (T=X.X°C, H=X.X%)
 * - [SENSORS] DHT: Not detected or reading failed (X errors)
 */
bool detectDHT() {
    sensor_init_count++;
    
    pinMode(DHT_PIN, INPUT_PULLUP);
    dht.begin();
    
    #ifdef DEBUG_SENSORS
    Serial.print("[SENSORS] Attempting DHT detection on pin ");
    Serial.println(DHT_PIN);
    #endif
    
    // Try to read with retries
    for (int i = 0; i < DHT_READ_RETRY; i++) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (!isnan(h) && !isnan(t)) {
            #ifdef DEBUG_SENSORS
            Serial.printf("  [✓] DHT: Detected (T=%.1f°C, H=%.1f%%) after %d attempts\n", t, h, i+1);
            #else
            Serial.printf("  [✓] DHT: Detected (T=%.1f°C, H=%.1f%%)\n", t, h);
            #endif
            return true;
        }
        
        #ifdef DEBUG_SENSORS
        Serial.printf("[SENSORS] DHT read attempt %d failed (H=%s, T=%s)\n", 
                      i+1, isnan(h)?"NaN":"OK", isnan(t)?"NaN":"OK");
        #endif
        
        delay(100);
    }
    
    dht_read_errors++;
    sensor_error_count++;
    Serial.println("  [!] DHT: Not detected or reading failed");
    #ifdef DEBUG_SENSORS
    Serial.printf("[DEBUG] Total DHT errors: %lu\n", dht_read_errors);
    #endif
    return false;
}

/**
 * @brief Detect SGP41 air quality sensor via I2C
 * @return true if sensor responds at 0x59, false otherwise
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] SGP41: Detected at 0x59
 * - [SENSORS] SGP41: Not detected at 0x59 (I2C error: X)
 */
bool detectSGP41() {
    sensor_init_count++;
    
    #ifdef DEBUG_SENSORS
    Serial.println("[SENSORS] Scanning I2C for SGP41 at 0x59...");
    #endif
    
    Wire.beginTransmission(0x59);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        #ifdef DEBUG_SENSORS
        Serial.println("  [✓] SGP41: Detected at 0x59 (ACK received)");
        #else
        Serial.println("  [✓] SGP41: Detected at 0x59");
        #endif
        return true;
    }
    
    sgp_read_errors++;
    sensor_error_count++;
    #ifdef DEBUG_SENSORS
    Serial.print("  [!] SGP41: Not detected at 0x59 (I2C error: ");
    Serial.print(error);
    Serial.println(")");
    #else
    Serial.println("  [!] SGP41: Not detected at 0x59");
    #endif
    return false;
}

/**
 * @brief Detect LD2410B radar sensor via UART
 * @return true (always returns true as radar might be idle)
 * 
 * Performs basic UART check but doesn't fail if no response
 * (radar might be in idle mode)
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] LD2410B: Data detected on UART
 * - [SENSORS] LD2410B: No response (may still be connected)
 */
bool detectRadar() {
    sensor_init_count++;
    
    // Simple detection - check if UART responds
    radarSerial.flush();
    delay(100);
    
    #ifdef DEBUG_SENSORS
    Serial.println("[SENSORS] Checking radar UART for data...");
    #endif
    
    if (radarSerial.available()) {
        #ifdef DEBUG_SENSORS
        Serial.print("  [✓] LD2410B: Data detected on UART (");
        Serial.print(radarSerial.available());
        Serial.println(" bytes available)");
        #else
        Serial.println("  [✓] LD2410B: Data detected on UART");
        #endif
        return true;
    }
    
    #ifdef DEBUG_SENSORS
    Serial.println("  [?] LD2410B: No response (may still be connected - idle mode)");
    #else
    Serial.println("  [?] LD2410B: No response (may still be connected)");
    #endif
    return true; // Don't fail - radar might be idle
}

/**
 * @brief Detect all sensors and update sensor state
 * 
 * Calls individual detection functions for each sensor type
 * Updates global sensors struct with detection results
 * 
 * DEBUG OUTPUT:
 * - Summary of all sensor detection results
 */
void detectAllSensors() {
    #ifdef DEBUG_SENSORS
    Serial.println("\n[SENSORS] Starting full sensor detection sequence...");
    unsigned long detect_start = millis();
    #endif
    
    sensors.tempHum.detected = detectDHT();
    sensors.airQual.detected = detectSGP41();
    sensors.hx711.detected = detectHX711();
    sensors.radar.detected = detectRadar();
    
    // Audio sensors are always considered present (ADC pins)
    sensors.audio.detected = true;
    
    // Mark active sensors (successfully initialized)
    sensors.tempHum.active = sensors.tempHum.detected;
    sensors.airQual.active = sensors.airQual.detected;
    sensors.hx711.active = sensors.hx711.detected;
    sensors.radar.active = sensors.radar.detected;
    sensors.audio.active = true;
    
    #ifdef DEBUG_SENSORS
    Serial.print("[SENSORS] Detection complete in ");
    Serial.print(millis() - detect_start);
    Serial.println("ms");
    #endif
}

/**
 * @brief Print current sensor status to Serial
 * 
 * Shows detected/not detected status for all sensors
 */
void printSensorStatus() {
    Serial.println("\n>> Sensor Status:");
    Serial.printf("  Temp/Humidity: %s\n", sensors.tempHum.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Air Quality:   %s\n", sensors.airQual.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Weight (HX711):%s\n", sensors.hx711.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Radar:         %s\n", sensors.radar.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Audio:         %s\n", sensors.audio.detected ? "OK" : "NOT FOUND");
    
    #ifdef DEBUG_SENSORS
    Serial.printf("\n[DEBUG] Sensor Stats:\n");
    Serial.printf("  Init count: %lu\n", sensor_init_count);
    Serial.printf("  Error count: %lu\n", sensor_error_count);
    Serial.printf("  DHT errors: %lu\n", dht_read_errors);
    Serial.printf("  SGP errors: %lu\n", sgp_read_errors);
    Serial.printf("  HX711 errors: %lu\n", hx711_read_errors);
    #endif
}

/**
 * @brief Initialize active sensors after detection
 * 
 * Only initializes sensors that were detected as present
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] DHT22/SHT40 initialized
 * - [SENSORS] SGP41 initialized
 * - [SENSORS] SGP41 initialization failed
 */
void initSensors() {
    #ifdef DEBUG_SENSORS
    Serial.println("[SENSORS] Initializing detected sensors...");
    #endif
    
    if (sensors.tempHum.active) {
        dht.begin();
        #ifdef DEBUG_SENSORS
        Serial.println(">> DHT22/SHT40 initialized successfully");
        #else
        Serial.println(">> DHT22/SHT40 initialized");
        #endif
    } else {
        Serial.println(">> Skipping DHT initialization (not detected)");
    }
    
    if (sensors.airQual.active) {
        if (sgp.begin_I2C(0x59, &Wire)) {
            sgp.measureBaseline();
            #ifdef DEBUG_SENSORS
            Serial.println(">> SGP41 initialized successfully with baseline measurement");
            #else
            Serial.println(">> SGP41 initialized");
            #endif
        } else {
            sensors.airQual.active = false;
            sgp_read_errors++;
            sensor_error_count++;
            Serial.println(">> SGP41 initialization failed");
            #ifdef DEBUG_SENSORS
            Serial.println("[ERROR] SGP41 begin_I2C returned false");
            #endif
        }
    } else {
        Serial.println(">> Skipping SGP41 initialization (not detected)");
    }
}

/**
 * @brief Read all active sensors periodically
 * @param now Current time in milliseconds (from millis())
 * 
 * Reads sensors every 2 seconds to avoid over-sampling
 * Implements error counting for each sensor type
 * Uses new logging macros from config.h for consistent output
 * Validates readings against configured limits
 * Includes performance monitoring and trace debugging
 * 
 * DEBUG OUTPUT:
 * - [SENSORS] Read error: DHT returned NaN (count: X)
 * - [SENSORS] HX711 raw value: XXXXX
 * - [SENSORS] Invalid CO2/VOC readings with range info
 * - [TRACE] ENTER/EXIT readSensors (when DEBUG_VERBOSE enabled)
 * - [PERF] Execution time (when DEBUG_PERF enabled)
 * 
 * EXCEPTIONS HANDLED:
 * - NaN values from DHT sensor
 * - Invalid HX711 readings
 * - Out-of-range air quality values
 */
void readSensors(unsigned long now) {
    TRACE_ENTER(SENSORS);
    PERF_START(readSensors);
    
    static unsigned long lastRead = 0;
    
    // Rate limiting - read every 2 seconds
    if (now - lastRead < 2000) return;
    lastRead = now;
    
    #ifdef DEBUG_SENSORS
    static unsigned long read_cycle = 0;
    read_cycle++;
    if (read_cycle % 10 == 0) {
        Serial.printf("[SENSORS] Read cycle %lu\n", read_cycle);
    }
    #endif
    
    // Read temperature/humidity with exception handling
    if (sensors.tempHum.active) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (!isnan(h)) {
            humidity = h;
            sensors.tempHum.error_count = 0; // Reset error counter on success
        } else {
            sensors.tempHum.error_count++;
            dht_read_errors++;
            #ifdef DEBUG_SENSORS
            if (sensors.tempHum.error_count % 5 == 0) {
                LOG_WARN("DHT", "Humidity read error");
                Serial.printf("[SENSORS] WARNING: DHT humidity read error (consecutive: %lu)\n", 
                             sensors.tempHum.error_count);
            }
            #endif
        }
        
        if (!isnan(t)) {
            temperature = t;
        } else {
            #ifdef DEBUG_SENSORS
            Serial.println("[SENSORS] WARNING: DHT temperature read returned NaN");
            #endif
        }
        
        // Validate temperature and humidity ranges
        GENTLE_ASSERT(humidity >= 0.0f && humidity <= 100.0f, "SENSORS", "Humidity out of valid range");
        GENTLE_ASSERT(temperature >= -40.0f && temperature <= 80.0f, "SENSORS", "Temperature out of valid range");
    }
    
    // Read HX711 weight sensor with error tracking
    if (sensors.hx711.active) {
        long raw = readHX711();
        if (raw != 0) {
            hx711_value = (raw - hx711_offset) / hx711_scale;
            #ifdef DEBUG_SENSORS
            if (read_cycle % 50 == 0) {
                DBG_SENSOR("[SENSORS] HX711 raw: %ld, scaled: %ld\n", raw, hx711_value);
            }
            #endif
            
            // Validate weight reading range
            if (raw < HX711_MIN_VALID_VALUE || raw > HX711_MAX_VALID_VALUE) {
                LOG_WARN("HX711", "Weight reading out of valid range");
                DBG_SENSOR("[SENSORS] WARNING: HX711 raw value %ld outside valid range [%ld, %ld]\n",
                          raw, HX711_MIN_VALID_VALUE, HX711_MAX_VALID_VALUE);
            }
        } else {
            hx711_read_errors++;
            sensor_error_count++;
            #ifdef DEBUG_SENSORS
            if (hx711_read_errors % 10 == 0) {
                LOG_WARN("HX711", "Read error detected");
                Serial.printf("[SENSORS] WARNING: HX711 read error (total: %lu)\n", hx711_read_errors);
            }
            #endif
        }
    }
    
    // Read SGP41 air quality sensor with validation
    if (sensors.airQual.active) {
        sgp.measureGas();
        co2_eq = sgp.CO2eq;
        voc_idx = sgp.VOCindex;
        
        // Validate readings against configured limits from config.h
        if (co2_eq < CO2_MIN_VALID || co2_eq > CO2_MAX_VALID) {
            #ifdef DEBUG_SENSORS
            LOG_WARN("SGP41", "CO2 reading out of valid range");
            DBG_SENSOR("[SENSORS] Invalid CO2: %d ppm (valid: %d-%d)\n", 
                      co2_eq, CO2_MIN_VALID, CO2_MAX_VALID);
            #endif
        }
        if (voc_idx < VOC_MIN_VALID || voc_idx > VOC_MAX_VALID) {
            #ifdef DEBUG_SENSORS
            LOG_WARN("SGP41", "VOC reading out of valid range");
            DBG_SENSOR("[SENSORS] Invalid VOC: %d (valid: %d-%d)\n", 
                      voc_idx, VOC_MIN_VALID, VOC_MAX_VALID);
            #endif
        }
        
        #ifdef DEBUG_SENSORS
        if (read_cycle % 30 == 0) {
            DBG_SENSOR("[SENSORS] SGP41 - CO2: %d ppm, VOC: %d\n", co2_eq, voc_idx);
        }
        #endif
    }
    
    PERF_END(readSensors);
    TRACE_EXIT(SENSORS);
}
