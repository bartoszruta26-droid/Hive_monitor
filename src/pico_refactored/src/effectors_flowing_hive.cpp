/*
 * ============================================================================
 * ApiaryGuard - Flowing Hive Effector Implementation
 * ============================================================================
 * 
 * MODUŁ: Implementacja efektorów automatycznego opróżniania ramek Flowing Hive
 * 
 * KOMPONENTY:
 * 1. Serwo mechanizmu obracania ramek (GPIO 12 - PWM)
 * 2. Drugi czujnik HX711 do wagi nadstawki (GPIO 10, 11)
 * 3. Czujnik przepływu miodu YF-S201 (GPIO 25 - INT)
 * 
 * GENTLE CODE PRINCIPLES:
 * - Wszystkie funkcje sprawdzają czy hardware jest wykryty przed operacją
 * - Walidacja parametrów wejściowych z graceful degradation
 * - Brak exceptionów - return codes i flagi errorów
 * - Automatyczny safe mode przy wykryciu anomalii
 * - Comprehensive logging for diagnostics
 * 
 * ERROR HANDLING STRATEGY:
 * - Timeout protection dla wszystkich operacji I/O
 * - Validation ranges dla wszystkich sensor readings
 * - Error counters dla monitoringa zdrowia systemu
 * - Safe mode activation dla critical errors
 * 
 * DEBUGOWANIE:
 * - DEBUG_SERVO: Logowanie operacji serwa
 * - DEBUG_FLOW: Logowanie czujnika przepływu
 * - DEBUG_HX711: Logowanie wag
 * - DEBUG_VERBOSE: Szczegółowe trace enter/exit
 * - DEBUG_PERF: Monitoring czasu wykonania
 * 
 * @file effectors_flowing_hive.cpp
 * @version 1.0.0
 * @date 2024
 * @license MIT
 */

#include "effectors_flowing_hive.h"
#include <Arduino.h>
#ifdef ARDUINO_ARCH_RP2040
#include <RP2040Watchdog.h>
#endif

// ============================================================================
// GLOBAL VARIABLES - STATE & DIAGNOSTICS
// ============================================================================

// ----------------------------------------------------------------------------
// Servo State Variables
// ----------------------------------------------------------------------------
static int currentServoAngle = SERVO_REST_ANGLE;           ///< Current servo position in degrees
static unsigned long lastServoUpdate = 0;                   ///< Last update timestamp (millis)
static bool servoInitialized = false;                       ///< PWM initialized flag
static bool servoDetected = false;                          ///< Hardware detection flag
static bool autoEmptyActive = false;                        ///< Auto-empty sequence active flag
static unsigned long autoEmptyStartTime = 0;                ///< Sequence start timestamp
static unsigned long autoEmptyDuration = 0;                 ///< Sequence duration in ms

// ----------------------------------------------------------------------------
// HX711 #2 (Superstructure Weight) State Variables
// ----------------------------------------------------------------------------
static long hx711_2_value = 0;                              ///< Last raw ADC reading
static long hx711_2_offset = 0;                             ///< Tare offset for calibration
static float hx711_2_scale = 1.0f;                          ///< Scale factor for grams conversion
static bool hx711_2_initialized = false;                    ///< Initialization success flag
static bool hx711_2_detected = false;                       ///< Hardware detection flag

// ----------------------------------------------------------------------------
// Flow Sensor State Variables
// ----------------------------------------------------------------------------
static volatile unsigned long flowPulseCount = 0;           ///< Pulse counter (volatile - ISR updated)
static float currentFlowRate = 0.0f;                        ///< Current flow rate in L/min
static float totalVolume = 0.0f;                            ///< Accumulated volume in liters
static unsigned long lastFlowUpdate = 0;                    ///< Last calculation timestamp
static bool flowSensorInitialized = false;                  ///< Interrupt attached flag
static bool flowSensorDetected = false;                     ///< Hardware detection flag

// ----------------------------------------------------------------------------
// Safe Mode State
// ----------------------------------------------------------------------------
static bool flowingHiveSafeMode = false;                    ///< Safe mode active flag

// ----------------------------------------------------------------------------
// Diagnostic Counters - For Health Monitoring
// ----------------------------------------------------------------------------
static unsigned long servo_op_count = 0;                    ///< Total servo operations
static unsigned long servo_error_count = 0;                 ///< Servo operation errors
static unsigned long hx711_2_read_count = 0;                ///< Total HX711 #2 reads
static unsigned long hx711_2_error_count = 0;               ///< HX711 #2 read errors
static unsigned long flow_read_count = 0;                   ///< Total flow sensor updates
static unsigned long flow_error_count = 0;                  ///< Flow calculation errors

// ============================================================================
// FLOW SENSOR INTERRUPT SERVICE ROUTINE (ISR)
// ============================================================================

/**
 * @brief Interrupt handler for flow sensor pulses
 * 
 * ISR CHARACTERISTICS:
 * - Minimal execution time (counter increment only)
 * - Called on every rising edge of flow sensor output
 * - Updates volatile counter for thread-safe main loop access
 * 
 * THREAD SAFETY:
 * - flowPulseCount is volatile for atomic access
 * - No blocking operations in ISR
 * - No Serial printing in ISR
 * 
 * @note Attached to FLOW_SENSOR_PIN (GPIO 25) on RISING edge
 */
void IRAM_ATTR flowSensorISR() {
    flowPulseCount++;
}

// ============================================================================
// SERVO CONTROL IMPLEMENTATION
// ============================================================================

/**
 * @brief Initialize servo control system
 * 
 * INITIALIZATION STEPS:
 * 1. Configure GPIO pin as OUTPUT
 * 2. Setup PWM with 50Hz frequency (20ms period for standard servos)
 * 3. Initialize state variables
 * 4. Set detection flag (assumes present if PWM setup succeeds)
 * 
 * HARDWARE DETECTION:
 * - Cannot directly detect servo presence (open-loop control)
 * - Assumes connected if pin initialization succeeds
 * - Operator should verify mechanical operation
 * 
 * ERROR HANDLING:
 * - Logs initialization details
 * - Continues system operation even without servo
 * - Safe defaults applied (rest position)
 */
void initServoControl() {
    TRACE_ENTER(SERVO);
    
    // Configure PWM output pin
    pinMode(SERVO_EMPTY_PIN, OUTPUT);
    
    // Initialize PWM for servo control
    // Standard RC servos use 50Hz (20ms period)
    // Pulse width: 500us (0°) to 2500us (180°)
    analogWriteFrequency(SERVO_EMPTY_PIN, 50);
    analogWrite(SERVO_EMPTY_PIN, 0);  // Start at 0% duty cycle
    
    // Update state
    servoInitialized = true;
    servoDetected = true;  // Assume present - no feedback from servo
    currentServoAngle = SERVO_REST_ANGLE;
    
    // Log initialization
    DBG_SERVO("[SERVO] Initialized on GPIO %d\n", SERVO_EMPTY_PIN);
    DBG_SERVO("[SERVO] Rest position: %d degrees\n", SERVO_REST_ANGLE);
    DBG_SERVO("[SERVO] PWM: 50Hz, pulse width %d-%d us\n", 
              SERVO_PULSE_WIDTH_MIN, SERVO_PULSE_WIDTH_MAX);
    
    TRACE_EXIT(SERVO);
}

/**
 * @brief Convert angle to PWM duty cycle (0-255)
 * 
 * CONVERSION PROCESS:
 * 1. Constrain angle to valid range (0-180°)
 * 2. Map angle to pulse width (500-2500μs)
 * 3. Convert pulse width to duty cycle for 50Hz PWM
 * 
 * FORMULA:
 * duty_cycle = (pulse_width / 20000) * 255
 * where 20000μs = 20ms = 50Hz period
 * 
 * @param angle Servo angle in degrees (0-180)
 * @return uint8_t PWM duty cycle (0-255)
 * 
 * @note Internal helper function - not exposed in API
 */
static uint8_t angleToDuty(uint16_t angle) {
    // Gentle code: Constrain input to prevent hardware damage
    angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    
    // Map angle to pulse width using linear interpolation
    uint32_t pulseWidth = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, 
                              SERVO_PULSE_WIDTH_MIN, SERVO_PULSE_WIDTH_MAX);
    
    // Convert pulse width to duty cycle (8-bit resolution)
    // For 50Hz: period = 20ms = 20000μs
    // duty = (pulseWidth / 20000) * 255
    uint8_t duty = (pulseWidth * 255) / 20000;
    
    return duty;
}

/**
 * @brief Set servo to specific angle
 * 
 * OPERATION FLOW:
 * 1. Check if servo is detected and initialized
 * 2. Verify safe mode is not active
 * 3. Validate and constrain angle parameter
 * 4. Convert angle to PWM duty cycle
 * 5. Update PWM output
 * 6. Log operation for diagnostics
 * 
 * GENTLE CODE FEATURES:
 * - Returns early if hardware not detected (no crash)
 * - Respects safe mode (blocks movement)
 * - Constrains invalid angles gracefully
 * - Logs warnings instead of throwing exceptions
 * 
 * @param angle Target angle in degrees (0-180)
 *              Automatically constrained to valid range
 * 
 * @note Non-blocking operation - returns immediately
 */
void setServoAngle(uint16_t angle) {
    TRACE_ENTER(SERVO);
    servo_op_count++;
    
    // GENTLE CODE: Check hardware presence before operation
    if (!servoDetected || !servoInitialized) {
        DBG_SERVO("[SERVO] Command ignored - servo not detected/initialized\n");
        TRACE_EXIT(SERVO);
        return;  // Graceful exit - no error thrown
    }
    
    // GENTLE CODE: Respect safe mode
    if (flowingHiveSafeMode) {
        DBG_SERVO("[SERVO] Command ignored - safe mode active\n");
        TRACE_EXIT(SERVO);
        return;  // Blocked for safety
    }
    
    // Validate angle with graceful degradation
    if (angle > SERVO_MAX_ANGLE) {
        DBG_SERVO("[SERVO] WARNING: Invalid angle %d (max %d), constraining\n", 
                  angle, SERVO_MAX_ANGLE);
        servo_error_count++;
        angle = SERVO_MAX_ANGLE;  // Auto-correct instead of failing
    }
    
    // Update state and apply new position
    currentServoAngle = angle;
    uint8_t duty = angleToDuty(angle);
    
    analogWrite(SERVO_EMPTY_PIN, duty);
    
    DBG_SERVO("[SERVO] Angle set to %d degrees (duty=%d/255)\n", angle, duty);
    
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
        hx711_2_detected = true;  // Sensor detected and responding
        DBG_HX711("[HX711_2] Initialized successfully (test reading: %ld)\n", test);
    } else {
        hx711_2_initialized = false;
        hx711_2_detected = false;  // Sensor not detected
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
    flowSensorDetected = true;  // Assume present if interrupt attaches successfully
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
    // Check if flow sensor is detected/initialized
    if (!flowSensorDetected || !flowSensorInitialized) return;
    
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
    Serial.printf("    Detected: %s\n", servoDetected ? "YES" : "NO");
    Serial.printf("    Initialized: %s\n", servoInitialized ? "YES" : "NO");
    if (servoDetected) {
        Serial.printf("    Current angle: %d degrees\n", currentServoAngle);
        Serial.printf("    Auto-empty active: %s\n", autoEmptyActive ? "YES" : "NO");
    }
    Serial.printf("    Operations: %lu, Errors: %lu\n", servo_op_count, servo_error_count);
    
    // HX711_2 status
    Serial.printf("  HX711 #2 (Superstructure):\n");
    Serial.printf("    Detected: %s\n", hx711_2_detected ? "YES" : "NO");
    Serial.printf("    Initialized: %s\n", hx711_2_initialized ? "YES" : "NO");
    if (hx711_2_detected && hx711_2_initialized) {
        Serial.printf("    Weight: %.1f g\n", getSuperstructureWeightGrams());
    } else {
        Serial.println("    (Sensor not detected - skipping readings)");
    }
    Serial.printf("    Reads: %lu, Errors: %lu\n", hx711_2_read_count, hx711_2_error_count);
    
    // Flow sensor status
    Serial.printf("  Flow Sensor:\n");
    Serial.printf("    Detected: %s\n", flowSensorDetected ? "YES" : "NO");
    Serial.printf("    Initialized: %s\n", flowSensorInitialized ? "YES" : "NO");
    if (flowSensorDetected && flowSensorInitialized) {
        Serial.printf("    Flow rate: %.3f L/min\n", currentFlowRate);
        Serial.printf("    Total volume: %.3f L\n", totalVolume);
        Serial.printf("    Honey flowing: %s\n", isHoneyFlowing() ? "YES" : "NO");
    } else {
        Serial.println("    (Sensor not detected - skipping readings)");
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
