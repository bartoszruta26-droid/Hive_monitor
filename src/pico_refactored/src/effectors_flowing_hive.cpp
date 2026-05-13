/*
 * ApiaryGuard - Flowing Hive Effector Implementation
 * Servo control for automatic frame emptying, second HX711 for superstructure weight,
 * and flow sensor for honey output monitoring
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_SERVO and DEBUG_FLOW in config.h
 * LOGGING: All operations are logged with state changes
 * EXCEPTIONS: Graceful handling of invalid values and hardware errors
 */

#include "effectors_flowing_hive.h"
#include <Arduino.h>
#ifdef ARDUINO_ARCH_RP2040
#include <RP2040Watchdog.h>
#endif

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Servo state
static int currentServoAngle = SERVO_REST_ANGLE;
static unsigned long lastServoUpdate = 0;
static bool servoInitialized = false;
static bool autoEmptyActive = false;
static unsigned long autoEmptyStartTime = 0;
static unsigned long autoEmptyDuration = 0;

// Second HX711 state
static long hx711_2_value = 0;
static long hx711_2_offset = 0;
static float hx711_2_scale = 1.0f;
static bool hx711_2_initialized = false;

// Flow sensor state
static volatile unsigned long flowPulseCount = 0;
static float currentFlowRate = 0.0f;
static float totalVolume = 0.0f;
static unsigned long lastFlowUpdate = 0;
static bool flowSensorInitialized = false;

// Safe mode state
static bool flowingHiveSafeMode = false;

// Debug counters
static unsigned long servo_op_count = 0;
static unsigned long servo_error_count = 0;
static unsigned long hx711_2_read_count = 0;
static unsigned long hx711_2_error_count = 0;
static unsigned long flow_read_count = 0;
static unsigned long flow_error_count = 0;

// ============================================================================
// FLOW SENSOR INTERRUPT HANDLER
// ============================================================================

void IRAM_ATTR flowSensorISR() {
    flowPulseCount++;
}

// ============================================================================
// SERVO CONTROL IMPLEMENTATION
// ============================================================================

/**
 * @brief Initialize servo control system
 */
void initServoControl() {
    TRACE_ENTER(SERVO);
    
    pinMode(SERVO_EMPTY_PIN, OUTPUT);
    
    // Initialize PWM for servo (50Hz = 20ms period)
    // RP2040 uses analogWrite for PWM
    analogWriteFrequency(SERVO_EMPTY_PIN, 50);
    analogWrite(SERVO_EMPTY_PIN, 0);
    
    servoInitialized = true;
    currentServoAngle = SERVO_REST_ANGLE;
    
    DBG_SERVO("[SERVO] Initialized on GPIO %d\n", SERVO_EMPTY_PIN);
    DBG_SERVO("[SERVO] Rest position: %d degrees\n", SERVO_REST_ANGLE);
    
    TRACE_EXIT(SERVO);
}

/**
 * @brief Convert angle to PWM duty cycle (0-255)
 */
static uint8_t angleToDuty(uint16_t angle) {
    // Constrain angle to valid range
    angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    
    // Map angle to pulse width (500-2500us)
    uint32_t pulseWidth = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, 
                              SERVO_PULSE_WIDTH_MIN, SERVO_PULSE_WIDTH_MAX);
    
    // Convert pulse width to duty cycle (for 50Hz = 20ms period)
    // Duty = (pulseWidth / 20000) * 255
    uint8_t duty = (pulseWidth * 255) / 20000;
    
    return duty;
}

/**
 * @brief Set servo to specific angle
 */
void setServoAngle(uint16_t angle) {
    TRACE_ENTER(SERVO);
    servo_op_count++;
    
    // Check safe mode
    if (flowingHiveSafeMode) {
        DBG_SERVO("[SERVO] Command ignored - safe mode active\n");
        TRACE_EXIT(SERVO);
        return;
    }
    
    // Validate angle
    if (angle > SERVO_MAX_ANGLE) {
        DBG_SERVO("[SERVO] WARNING: Invalid angle %d (max %d)\n", angle, SERVO_MAX_ANGLE);
        servo_error_count++;
        angle = SERVO_MAX_ANGLE;
    }
    
    currentServoAngle = angle;
    uint8_t duty = angleToDuty(angle);
    
    analogWrite(SERVO_EMPTY_PIN, duty);
    
    DBG_SERVO("[SERVO] Angle set to %d degrees (duty=%d)\n", angle, duty);
    
    TRACE_EXIT(SERVO);
}

/**
 * @brief Move servo to rest position
 */
void setServoRestPosition() {
    DBG_SERVO("[SERVO] Moving to rest position (%d degrees)\n", SERVO_REST_ANGLE);
    setServoAngle(SERVO_REST_ANGLE);
}

/**
 * @brief Move servo to emptying position
 */
void setServoEmptyPosition() {
    DBG_SERVO("[SERVO] Moving to emptying position (%d degrees)\n", SERVO_EMPTY_ANGLE);
    setServoAngle(SERVO_EMPTY_ANGLE);
}

/**
 * @brief Execute automatic frame emptying sequence
 */
void executeAutoEmptySequence(unsigned long duration_ms) {
    TRACE_ENTER(SERVO);
    
    if (flowingHiveSafeMode) {
        DBG_SERVO("[SERVO] Auto-empty sequence blocked - safe mode active\n");
        TRACE_EXIT(SERVO);
        return;
    }
    
    DBG_SERVO("[SERVO] Starting auto-empty sequence (duration: %lu ms)\n", duration_ms);
    
    // Move to emptying position
    setServoEmptyPosition();
    
    autoEmptyActive = true;
    autoEmptyStartTime = millis();
    autoEmptyDuration = duration_ms;
    
    TRACE_EXIT(SERVO);
}

/**
 * @brief Update servo control loop
 */
void updateServoLoop() {
    if (!servoInitialized || !autoEmptyActive) return;
    
    unsigned long elapsed = millis() - autoEmptyStartTime;
    
    if (elapsed >= autoEmptyDuration) {
        DBG_SERVO("[SERVO] Auto-empty sequence complete - returning to rest\n");
        setServoRestPosition();
        autoEmptyActive = false;
    }
}

// ============================================================================
// SECOND HX711 IMPLEMENTATION
// ============================================================================

/**
 * @brief Read second HX711 with timeout
 */
long readHX711_2() {
    TRACE_ENTER(HX711_2);
    PERF_START(readHX711_2);
    
    long count = 0;
    unsigned long read_start = millis();
    
    hx711_2_read_count++;
    
    // Ensure proper pin modes
    pinMode(HX711_2_DT, INPUT);
    pinMode(HX711_2_SCK, OUTPUT);
    digitalWrite(HX711_2_SCK, LOW);
    
    // Wait for chip to be ready with timeout
    unsigned long start_time = millis();
    while(digitalRead(HX711_2_DT)) {
        if (millis() - start_time > HX711_2_TIMEOUT_MS) {
            hx711_2_error_count++;
            
            #ifdef DEBUG_HX711
            Serial.printf("[HX711_2] Timeout after %lums\n", millis() - start_time);
            #endif
            
            PERF_END(readHX711_2);
            TRACE_EXIT(HX711_2);
            return 0;
        }
        
        #ifdef ARDUINO_ARCH_RP2040
        rp2040.wdtReset();
        #endif
        
        delayMicroseconds(10);
    }
    
    // Read 24 bits
    for(int i = 0; i < 24; i++) {
        digitalWrite(HX711_2_SCK, HIGH);
        count = count << 1;
        digitalWrite(HX711_2_SCK, LOW);
        
        if(digitalRead(HX711_2_DT)) {
            count++;
        }
        
        delayMicroseconds(1);
    }
    
    // Send 25th clock pulse
    digitalWrite(HX711_2_SCK, HIGH);
    count = count ^ 0x800000;
    digitalWrite(HX711_2_SCK, LOW);
    
    // Validate reading
    if (count < HX711_2_MIN_VALID_VALUE || count > HX711_2_MAX_VALID_VALUE) {
        hx711_2_error_count++;
        DBG_HX711("[HX711_2] Invalid reading: %ld\n", count);
        PERF_END(readHX711_2);
        TRACE_EXIT(HX711_2);
        return 0;
    }
    
    hx711_2_value = count;
    
    PERF_END(readHX711_2);
    TRACE_EXIT(HX711_2);
    return count;
}

/**
 * @brief Initialize second HX711
 */
void initHX711_2() {
    TRACE_ENTER(HX711_2);
    
    pinMode(HX711_2_DT, INPUT);
    pinMode(HX711_2_SCK, OUTPUT);
    digitalWrite(HX711_2_SCK, LOW);
    
    // Test communication
    delay(100);
    long test = readHX711_2();
    
    if (test != 0) {
        hx711_2_initialized = true;
        DBG_HX711("[HX711_2] Initialized successfully (test reading: %ld)\n", test);
    } else {
        hx711_2_initialized = false;
        LOG_ERROR("HX711_2", "Initialization failed - sensor not detected");
    }
    
    TRACE_EXIT(HX711_2);
}

/**
 * @brief Get superstructure weight in grams
 */
float getSuperstructureWeightGrams() {
    if (!hx711_2_initialized) return 0.0f;
    
    long raw = readHX711_2();
    if (raw == 0) return 0.0f;
    
    // Apply calibration
    float weight = (float)(raw - hx711_2_offset) / hx711_2_scale;
    
    return weight;
}

/**
 * @brief Calibrate/tare second HX711
 */
void tareHX711_2() {
    TRACE_ENTER(HX711_2);
    
    if (!hx711_2_initialized) {
        LOG_ERROR("HX711_2", "Cannot tare - not initialized");
        TRACE_EXIT(HX711_2);
        return;
    }
    
    DBG_HX711("[HX711_2] Taring...\n");
    
    // Take multiple readings and average
    long sum = 0;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        long reading = readHX711_2();
        if (reading != 0) {
            sum += reading;
        }
        delay(10);
    }
    
    hx711_2_offset = sum / samples;
    
    DBG_HX711("[HX711_2] Tare complete (offset: %ld)\n", hx711_2_offset);
    
    TRACE_EXIT(HX711_2);
}

// ============================================================================
// FLOW SENSOR IMPLEMENTATION
// ============================================================================

/**
 * @brief Initialize flow sensor with interrupt
 */
void initFlowSensor() {
    TRACE_ENTER(FLOW);
    
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    
    // Attach interrupt on rising edge
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorISR, RISING);
    
    flowSensorInitialized = true;
    flowPulseCount = 0;
    currentFlowRate = 0.0f;
    totalVolume = 0.0f;
    lastFlowUpdate = millis();
    
    DBG_FLOW("[FLOW] Initialized on GPIO %d (interrupt-enabled)\n", FLOW_SENSOR_PIN);
    DBG_FLOW("[FLOW] Pulses per liter: %d\n", FLOW_SENSOR_PULSES_PER_LITER);
    
    TRACE_EXIT(FLOW);
}

/**
 * @brief Update flow sensor calculations
 */
void updateFlowSensor() {
    if (!flowSensorInitialized) return;
    
    unsigned long now = millis();
    unsigned long elapsed = now - lastFlowUpdate;
    
    if (elapsed < FLOW_SAMPLE_INTERVAL_MS) return;
    
    flow_read_count++;
    
    // Calculate flow rate from pulse count
    // Flow rate (L/min) = (pulses / pulses_per_liter) / (time_minutes)
    float timeMinutes = elapsed / 60000.0f;
    
    if (timeMinutes > 0) {
        float liters = (float)flowPulseCount / FLOW_SENSOR_PULSES_PER_LITER;
        currentFlowRate = liters / timeMinutes;
        
        // Validate flow rate
        if (currentFlowRate < 0 || currentFlowRate > FLOW_MAX_RATE) {
            DBG_FLOW("[FLOW] WARNING: Invalid flow rate %.3f L/min\n", currentFlowRate);
            flow_error_count++;
            currentFlowRate = 0.0f;
        }
        
        // Add to total volume
        totalVolume += liters;
    }
    
    // Reset pulse counter for next interval
    flowPulseCount = 0;
    lastFlowUpdate = now;
    
    #ifdef DEBUG_FLOW
    if (flow_read_count % 10 == 0) {
        DBG_FLOW("[FLOW] Rate: %.3f L/min, Total: %.3f L\n", currentFlowRate, totalVolume);
    }
    #endif
}

/**
 * @brief Get current flow rate
 */
float getFlowRateLPM() {
    return currentFlowRate;
}

/**
 * @brief Get total volume flowed
 */
float getTotalVolumeLiters() {
    return totalVolume;
}

/**
 * @brief Reset flow volume counter
 */
void resetFlowCounter() {
    totalVolume = 0.0f;
    flowPulseCount = 0;
    DBG_FLOW("[FLOW] Volume counter reset\n");
}

/**
 * @brief Check if honey is flowing
 */
bool isHoneyFlowing() {
    return currentFlowRate >= FLOW_MIN_RATE;
}

// ============================================================================
// SAFE MODE AND STATUS FUNCTIONS
// ============================================================================

/**
 * @brief Print complete effector status
 */
void printFlowingHiveEffectorStatus() {
    Serial.println("\n>> Flowing Hive Effector Status:");
    
    // Servo status
    Serial.printf("  Servo:\n");
    Serial.printf("    Initialized: %s\n", servoInitialized ? "YES" : "NO");
    Serial.printf("    Current angle: %d degrees\n", currentServoAngle);
    Serial.printf("    Auto-empty active: %s\n", autoEmptyActive ? "YES" : "NO");
    Serial.printf("    Operations: %lu, Errors: %lu\n", servo_op_count, servo_error_count);
    
    // HX711_2 status
    Serial.printf("  HX711 #2 (Superstructure):\n");
    Serial.printf("    Initialized: %s\n", hx711_2_initialized ? "YES" : "NO");
    if (hx711_2_initialized) {
        Serial.printf("    Weight: %.1f g\n", getSuperstructureWeightGrams());
    }
    Serial.printf("    Reads: %lu, Errors: %lu\n", hx711_2_read_count, hx711_2_error_count);
    
    // Flow sensor status
    Serial.printf("  Flow Sensor:\n");
    Serial.printf("    Initialized: %s\n", flowSensorInitialized ? "YES" : "NO");
    if (flowSensorInitialized) {
        Serial.printf("    Flow rate: %.3f L/min\n", currentFlowRate);
        Serial.printf("    Total volume: %.3f L\n", totalVolume);
        Serial.printf("    Honey flowing: %s\n", isHoneyFlowing() ? "YES" : "NO");
    }
    Serial.printf("    Updates: %lu, Errors: %lu\n", flow_read_count, flow_error_count);
    
    // Safe mode status
    Serial.printf("  Safe mode: %s\n", flowingHiveSafeMode ? "ACTIVE" : "inactive");
}

/**
 * @brief Check if safe mode is active
 */
bool isFlowingHiveSafeModeActive() {
    return flowingHiveSafeMode;
}

/**
 * @brief Activate safe mode
 */
void activateFlowingHiveSafeMode(const char* reason) {
    flowingHiveSafeMode = true;
    
    LOG_ERROR("FLOWING_HIVE", reason);
    DBG_SERVO("[SERVO] Safe mode activated: %s\n", reason);
    
    // Move servo to safe rest position
    if (servoInitialized) {
        setServoRestPosition();
        autoEmptyActive = false;
    }
}

/**
 * @brief Deactivate safe mode
 */
void deactivateFlowingHiveSafeMode() {
    flowingHiveSafeMode = false;
    DBG_SERVO("[SERVO] Safe mode deactivated\n");
}
