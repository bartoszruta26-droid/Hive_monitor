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
#include <Servo.h>
#ifdef ARDUINO_ARCH_RP2040
#endif

// ============================================================================
// GLOBAL VARIABLES - STATE & DIAGNOSTICS
// ============================================================================

// ----------------------------------------------------------------------------
// Servo State Variables
// ----------------------------------------------------------------------------
static Servo flowingHiveServo;                         ///< RP2040 PIO-backed servo controller
static int currentServoAngle = SERVO_REST_ANGLE;           ///< Current servo position in degrees
static bool servoInitialized = false;                       ///< Servo PIO attachment succeeded
static bool servoControllerReady = false;                  ///< Controller attached; no physical feedback
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
static bool flowSensorInterruptReady = false;                     ///< Hardware detection flag

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
void flowSensorISR() {
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

    pinMode(SERVO_EMPTY_PIN, OUTPUT);
    digitalWrite(SERVO_EMPTY_PIN, LOW);

    // The Arduino-Pico core includes a PIO-backed Servo implementation.
    // It generates the 20ms servo frame independently of the main loop.
    const int servoAttachResult = flowingHiveServo.attach(
        SERVO_EMPTY_PIN,
        SERVO_PULSE_WIDTH_MIN,
        SERVO_PULSE_WIDTH_MAX
    );

    servoInitialized = (servoAttachResult >= 0);
    servoControllerReady = servoInitialized; // Physical servo presence is not sensed.
    currentServoAngle = SERVO_REST_ANGLE;
    autoEmptyActive = false;

    if (!servoInitialized) {
        LOG_ERROR("SERVO", "Servo PIO attachment failed; movement commands disabled");
        TRACE_EXIT(SERVO);
        return;
    }

    const uint32_t restPulse = map(
        SERVO_REST_ANGLE,
        SERVO_MIN_ANGLE,
        SERVO_MAX_ANGLE,
        SERVO_PULSE_WIDTH_MIN,
        SERVO_PULSE_WIDTH_MAX
    );
    flowingHiveServo.writeMicroseconds(static_cast<int>(restPulse));

    DBG_SERVO("[SERVO] Initialized on GPIO %d using RP2040 PIO servo driver\n",
              SERVO_EMPTY_PIN);
    DBG_SERVO("[SERVO] Pulse range: %d-%d us; refresh interval: 20 ms\n",
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
bool setServoAngle(uint16_t angle) {
    TRACE_ENTER(SERVO);
    servo_op_count++;
    
    // GENTLE CODE: Check hardware presence before operation
    if (!servoControllerReady || !servoInitialized) {
        DBG_SERVO("[SERVO] Command ignored - servo not detected/initialized\n");
        TRACE_EXIT(SERVO);
        return false;  // Graceful exit - no error thrown
    }
    
    // GENTLE CODE: Respect safe mode
    if (flowingHiveSafeMode) {
        DBG_SERVO("[SERVO] Command ignored - safe mode active\n");
        TRACE_EXIT(SERVO);
        return false;  // Blocked for safety
    }
    
    // Reject out-of-range angles rather than silently driving to a limit.
    if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE) {
        DBG_SERVO("[SERVO] ERROR: Invalid angle %d (allowed %d-%d)\n",
                  angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
        ++servo_error_count;
        TRACE_EXIT(SERVO);
        return false;
    }
    
    // Convert the requested angle to a pulse width in microseconds.
    // Servo library keeps generating stable 50Hz pulses in the RP2040 PIO.
    const uint32_t pulseWidth = map(
        angle,
        SERVO_MIN_ANGLE,
        SERVO_MAX_ANGLE,
        SERVO_PULSE_WIDTH_MIN,
        SERVO_PULSE_WIDTH_MAX
    );

    currentServoAngle = angle;
    flowingHiveServo.writeMicroseconds(static_cast<int>(pulseWidth));

    DBG_SERVO("[SERVO] Angle set to %d degrees (pulse=%lu us)\n",
              angle, static_cast<unsigned long>(pulseWidth));

    TRACE_EXIT(SERVO);
    return true;
}

/**
 * @brief Move servo to rest position
 */
bool setServoRestPosition() {
    DBG_SERVO("[SERVO] Moving to rest position (%d degrees)\n", SERVO_REST_ANGLE);
    return setServoAngle(SERVO_REST_ANGLE);
}

/**
 * @brief Move servo to emptying position
 */
bool setServoEmptyPosition() {
    DBG_SERVO("[SERVO] Moving to emptying position (%d degrees)\n", SERVO_EMPTY_ANGLE);
    return setServoAngle(SERVO_EMPTY_ANGLE);
}

/**
 * @brief Execute automatic frame emptying sequence
 */
bool executeAutoEmptySequence(unsigned long duration_ms) {
    TRACE_ENTER(SERVO);

    if (flowingHiveSafeMode) {
        DBG_SERVO("[SERVO] Auto-empty sequence blocked - safe mode active\n");
        TRACE_EXIT(SERVO);
        return false;
    }

    if (!servoInitialized || !servoControllerReady) {
        ++servo_error_count;
        LOG_WARN("SERVO", "Auto-empty request rejected: servo controller is unavailable");
        TRACE_EXIT(SERVO);
        return false;
    }

    if (duration_ms < SERVO_AUTO_EMPTY_MIN_MS ||
        duration_ms > SERVO_AUTO_EMPTY_MAX_MS) {
        ++servo_error_count;
        LOG_WARN("SERVO", "Auto-empty duration outside permitted 1-1800 second range");
        TRACE_EXIT(SERVO);
        return false;
    }

    DBG_SERVO("[SERVO] Starting auto-empty sequence (duration: %lu ms)\n", duration_ms);
    if (!setServoEmptyPosition()) {
        TRACE_EXIT(SERVO);
        return false;
    }

    autoEmptyActive = true;
    autoEmptyStartTime = millis();
    autoEmptyDuration = duration_ms;

    TRACE_EXIT(SERVO);
    return true;
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
    APIARY_PERF_START(readHX711_2);
    
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
            
            APIARY_PERF_END(readHX711_2);
            TRACE_EXIT(HX711_2);
            return 0;
        }
        
        #ifdef ARDUINO_ARCH_RP2040
        rp2040.wdt_reset();
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
        APIARY_PERF_END(readHX711_2);
        TRACE_EXIT(HX711_2);
        return 0;
    }
    
    hx711_2_value = count;
    
    APIARY_PERF_END(readHX711_2);
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

    // Initialize all ISR-shared state before enabling the interrupt.
    noInterrupts();
    flowPulseCount = 0;
    interrupts();
    currentFlowRate = 0.0f;
    totalVolume = 0.0f;
    lastFlowUpdate = millis();

    // Attach interrupt only after the pulse counter and timestamps are ready.
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowSensorISR, RISING);

    flowSensorInitialized = true;
    flowSensorInterruptReady = true;  // Interrupt setup succeeded; physical sensor presence is not verified.
    
    DBG_FLOW("[FLOW] Initialized on GPIO %d (interrupt-enabled)\n", FLOW_SENSOR_PIN);
    DBG_FLOW("[FLOW] Pulses per liter: %d\n", FLOW_SENSOR_PULSES_PER_LITER);
    
    TRACE_EXIT(FLOW);
}

/**
 * @brief Update flow sensor calculations
 */
void updateFlowSensor() {
    if (!flowSensorInterruptReady || !flowSensorInitialized) return;

    const unsigned long now = millis();
    const unsigned long elapsed = now - lastFlowUpdate;
    if (elapsed < FLOW_SAMPLE_INTERVAL_MS) return;

    ++flow_read_count;

    // Snapshot and clear the ISR-owned pulse counter atomically. Otherwise
    // a pulse arriving between read and reset could be lost.
    noInterrupts();
    const unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    interrupts();

    const float timeMinutes = elapsed / 60000.0f;
    if (timeMinutes > 0.0f) {
        const float liters = static_cast<float>(pulses) /
                             static_cast<float>(FLOW_SENSOR_PULSES_PER_LITER);
        const float measuredRate = liters / timeMinutes;

        if (measuredRate < 0.0f || measuredRate > FLOW_MAX_RATE) {
            DBG_FLOW("[FLOW] WARNING: Invalid flow rate %.3f L/min\n", measuredRate);
            ++flow_error_count;
            currentFlowRate = 0.0f;
        } else {
            currentFlowRate = measuredRate;
            totalVolume += liters;
        }
    }

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
    noInterrupts();
    flowPulseCount = 0;
    interrupts();
    currentFlowRate = 0.0f;
    lastFlowUpdate = millis();
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
    Serial.printf("    Controller ready: %s\n", servoControllerReady ? "YES" : "NO");
    Serial.printf("    Initialized: %s\n", servoInitialized ? "YES" : "NO");
    if (servoControllerReady) {
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
    Serial.printf("    Interrupt ready: %s\n", flowSensorInterruptReady ? "YES" : "NO");
    Serial.printf("    Initialized: %s\n", flowSensorInitialized ? "YES" : "NO");
    if (flowSensorInterruptReady && flowSensorInitialized) {
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
 * 
 * ACTIONS TAKEN:
 * 1. Move servo to rest position FIRST (before setting flag)
 * 2. Cancel any active auto-empty sequence
 * 3. Set flowingHiveSafeMode flag = true
 * 4. Log error with provided reason message
 * 
 * This ordering ensures the servo moves to safe position even if
 * called during an error condition, as setServoAngle() will be
 * blocked once the flag is set.
 */
void activateFlowingHiveSafeMode(const char* reason) {
    // CRITICAL: Move servo to rest position BEFORE setting safe mode flag
    // setServoAngle() checks the flag and will ignore commands if already set
    if (servoInitialized) {
        DBG_SERVO("[SERVO] Moving to rest position before safe mode activation\n");
        setServoRestPosition();
        autoEmptyActive = false;
    }
    
    // Now set safe mode flag
    flowingHiveSafeMode = true;
    
    LOG_ERROR("FLOWING_HIVE", reason);
    DBG_SERVO("[SERVO] Safe mode activated: %s\n", reason);
}

/**
 * @brief Deactivate safe mode
 */
void deactivateFlowingHiveSafeMode() {
    flowingHiveSafeMode = false;
    DBG_SERVO("[SERVO] Safe mode deactivated\n");
}
