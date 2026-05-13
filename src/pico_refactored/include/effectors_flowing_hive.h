/*
 * ============================================================================
 * ApiaryGuard - Flowing Hive Effector Control Header
 * ============================================================================
 * 
 * MODUŁ: Sterowanie efektorami automatycznego opróżniania ramek Flowing Hive
 * 
 * KOMPONENTY:
 * 1. Serwo mechanizmu obracania ramek (GPIO 12 - PWM)
 * 2. Drugi czujnik HX711 do wagi nadstawki (GPIO 10, 11)
 * 3. Czujnik przepływu miodu YF-S201 (GPIO 25 - INT)
 * 
 * DEBUGOWANIE:
 * - Zdefiniuj DEBUG_SERVO w config.h dla logowania serwa
 * - Zdefiniuj DEBUG_FLOW w config.h dla logowania czujnika przepływu
 * - Zdefiniuj DEBUG_HX711 w config.h dla logowania wag
 * 
 * GENTLE CODE PRINCIPLES:
 * - Wszystkie funkcje sprawdzają czy hardware jest wykryty przed operacją
 * - Walidacja parametrów wejściowych z graceful degradation
 * - Brak exceptionów - return codes i flagi errorów
 * - Automatyczny safe mode przy wykryciu anomalii
 * 
 * THREAD SAFETY:
 * - Flow sensor ISR (interrupt service routine) aktualizuje volatile licznik
 * - Główne pętle bezpiecznie odczytują wartości przez atomic operations
 * 
 * @file effectors_flowing_hive.h
 * @version 1.0.0
 * @date 2024
 * @license MIT
 */

#ifndef EFFECTORS_FLOWING_HIVE_H
#define EFFECTORS_FLOWING_HIVE_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// SERVO CONTROL FUNCTIONS
// ============================================================================

/**
 * @brief Initialize servo control system
 * 
 * DETECTION LOGIC:
 * - Sets up PWM on SERVO_EMPTY_PIN (GPIO 12)
 * - Assumes servo is present if pin initialization succeeds
 * - Sets servoDetected flag for conditional operation
 * 
 * ERROR HANDLING:
 * - Graceful failure if PWM initialization fails
 * - Logs error but continues system operation
 * 
 * @note Call once during setup()
 */
void initServoControl();

/**
 * @brief Set servo to specific angle
 * 
 * GENTLE CODE FEATURES:
 * - Validates angle range (0-180 degrees)
 * - Checks if servo is detected before operation
 * - Respects safe mode - ignores command if active
 * - Logs warnings for invalid inputs without crashing
 * 
 * @param angle Angle in degrees (0-180)
 *              Constrained automatically to valid range
 * 
 * @return void - operation is fire-and-forget
 * 
 * @note Controls the servo motor that tilts the Flowing Hive frames
 *       for automatic honey extraction
 */
void setServoAngle(uint16_t angle);

/**
 * @brief Move servo to rest position
 * 
 * Rest position allows normal bee access to frames.
 * Safe default position used in error conditions.
 * 
 * @note Positions frames in normal bee-accessible configuration
 */
void setServoRestPosition();

/**
 * @brief Move servo to emptying position
 * 
 * Emptying position tilts frames to drain honey.
 * Used during automated harvest sequences.
 * 
 * @note Tilts frames to drain honey through the collection system
 */
void setServoEmptyPosition();

/**
 * @brief Execute automatic frame emptying sequence
 * 
 * COMPLETE AUTOMATED SEQUENCE:
 * 1. Validate safe mode status
 * 2. Move servo to emptying position (SERVO_EMPTY_ANGLE)
 * 3. Hold position for specified duration
 * 4. Automatically return to rest position
 * 
 * SAFETY FEATURES:
 * - Blocked if safe mode is active
 * - Non-blocking operation (returns immediately)
 * - Background execution via updateServoLoop()
 * 
 * @param duration_ms Duration to hold emptying position
 *                    Default: 1800000ms (30 minutes)
 *                    Range: 1000ms - unlimited
 * 
 * @note Call from main loop, then periodically call updateServoLoop()
 */
void executeAutoEmptySequence(unsigned long duration_ms = 1800000);

/**
 * @brief Update servo control loop (call periodically)
 * 
 * BACKGROUND TASKS:
 * - Monitors auto-empty sequence progress
 * - Returns servo to rest position when complete
 * - Non-blocking timing using millis()
 * 
 * @note Must be called regularly from main loop()
 *       Recommended: Every 100-500ms
 */
void updateServoLoop();

// ============================================================================
// SECOND HX711 (SUPERSTRUCTURE WEIGHT) FUNCTIONS
// ============================================================================

/**
 * @brief Initialize second HX711 weight sensor
 * 
 * DETECTION LOGIC:
 * - Configures GPIO pins (HX711_2_DT, HX711_2_SCK)
 * - Attempts communication with HX711 chip
 * - Sets hx711_2_detected flag based on response
 * - Only sets initialized=true if sensor responds
 * 
 * ERROR HANDLING:
 * - Timeout after HX711_2_TIMEOUT_MS (50ms)
 * - Logs error if sensor not detected
 * - System continues without superstructure weight monitoring
 * 
 * @note Reads superstructure/additional box weight
 *       Call once during setup()
 */
void initHX711_2();

/**
 * @brief Read second HX711 sensor
 * 
 * READ PROCESS:
 * 1. Wait for chip ready signal with timeout
 * 2. Shift out 24 bits of data (MSB first)
 * 3. Send 25th clock pulse to set channel/gain
 * 4. Validate reading against min/max limits
 * 
 * ERROR HANDLING:
 * - Returns 0 on timeout or communication failure
 * - Validates reading range (HX711_2_MIN/MAX_VALID_VALUE)
 * - Increments error counter for diagnostics
 * - Feeds watchdog during long reads
 * 
 * @return long Weight value in ADC counts
 *         0 on error/timeout/invalid reading
 * 
 * @note Thread-safe: Uses timeout to prevent blocking
 */
long readHX711_2();

/**
 * @brief Get superstructure weight in grams
 * 
 * CALCULATION:
 * weight_grams = (raw_value - offset) / scale_factor
 * 
 * CALIBRATION:
 * - Requires prior call to tareHX711_2() for offset
 * - Scale factor should be determined experimentally
 * 
 * ERROR HANDLING:
 * - Returns 0.0f if not initialized
 * - Returns 0.0f on read error
 * 
 * @return float Weight in grams (calibrated)
 *         0.0f on error or not calibrated
 */
float getSuperstructureWeightGrams();

/**
 * @brief Calibrate/tare second HX711
 * 
 * TARE PROCESS:
 * 1. Take 10 consecutive readings
 * 2. Average valid readings
 * 3. Store as zero offset (hx711_2_offset)
 * 
 * REQUIREMENTS:
 * - Ensure no load on superstructure during taring
 * - Call after warm-up period for stability
 * 
 * ERROR HANDLING:
 * - Skips invalid readings (value = 0)
 * - Logs warning if not initialized
 * 
 * @note Should be called with empty superstructure
 */
void tareHX711_2();

// ============================================================================
// FLOW SENSOR FUNCTIONS
// ============================================================================

/**
 * @brief Initialize flow sensor with interrupt
 * 
 * DETECTION LOGIC:
 * - Configures GPIO pin as INPUT_PULLUP (FLOW_SENSOR_PIN)
 * - Attaches interrupt on rising edge for pulse counting
 * - Assumes sensor present if interrupt attaches successfully
 * - Sets flowSensorDetected flag for conditional operation
 * 
 * INTERRUPT SERVICE ROUTINE:
 * - flowSensorISR() increments volatile pulse counter
 * - ISR is minimal - just counter increment for speed
 * - Main loop calculates flow rate from pulse frequency
 * 
 * ERROR HANDLING:
 * - Logs initialization details
 * - Continues system operation even if sensor absent
 * 
 * @note Sets up interrupt handler for pulse counting
 *       Call once during setup()
 */
void initFlowSensor();

/**
 * @brief Get current flow rate
 * 
 * CALCULATION:
 * flow_rate_LPM = (pulses / PULSES_PER_LITER) / time_minutes
 * 
 * UPDATE RATE:
 * - Calculated every FLOW_SAMPLE_INTERVAL_MS (1000ms)
 * - Smoothed over multiple samples for stability
 * 
 * VALIDATION:
 * - Constrained to 0.0 - FLOW_MAX_RATE L/min
 * - Returns 0.0 if below minimum threshold
 * 
 * @return float Flow rate in liters per minute
 *         0.0f if no flow or sensor not detected
 */
float getFlowRateLPM();

/**
 * @brief Get total volume flowed since last reset
 * 
 * ACCUMULATION:
 * - Integrates flow rate over time
 * - Persistent across sampling intervals
 * - Reset only by explicit resetFlowCounter() call
 * 
 * ACCURACY:
 * - Depends on regular updateFlowSensor() calls
 * - Typical accuracy: ±5% for honey viscosity
 * 
 * @return float Total volume in liters
 *         0.0f if not initialized or reset
 */
float getTotalVolumeLiters();

/**
 * @brief Reset flow volume counter
 * 
 * USE CASES:
 * - Start of new harvest session
 * - Calibration testing
 * - Manual reset via command interface
 * 
 * SIDE EFFECTS:
 * - Resets totalVolume to 0.0
 * - Resets flowPulseCount to 0
 * - Logs reset event for audit trail
 * 
 * @note Does not affect current flow rate reading
 */
void resetFlowCounter();

/**
 * @brief Update flow sensor calculations (call periodically)
 * 
 * PROCESSING STEPS:
 * 1. Check elapsed time since last update
 * 2. Read accumulated pulse count from ISR
 * 3. Calculate instantaneous flow rate
 * 4. Validate flow rate against limits
 * 5. Add volume to running total
 * 6. Reset pulse counter for next interval
 * 
 * TIMING REQUIREMENTS:
 * - Must be called at least every FLOW_SAMPLE_INTERVAL_MS
 * - Recommended: Every 500-1000ms
 * - Uses non-blocking millis() timing
 * 
 * THREAD SAFETY:
 * - Safely reads volatile flowPulseCount
 * - Atomic operations for counter reset
 * 
 * @note Critical: Must be called regularly from main loop()
 */
void updateFlowSensor();

/**
 * @brief Check if honey is currently flowing
 * 
 * DETECTION THRESHOLD:
 * - Returns true if flow >= FLOW_MIN_RATE (0.01 L/min)
 * - Hysteresis prevents false triggering
 * 
 * USE CASES:
 * - Trigger alerts when harvest begins
 * - Log harvest start/stop times
 * - Monitor for unexpected flow (potential leak)
 * 
 * @return true if flow detected above minimum threshold
 *         false otherwise or if sensor not detected
 */
bool isHoneyFlowing();

// ============================================================================
// COMBINED EFFECTOR STATUS
// ============================================================================

/**
 * @brief Print complete effector status including new devices
 * 
 * OUTPUT FORMAT:
 * - Servo: detected, initialized, angle, auto-empty state, error counts
 * - HX711 #2: detected, initialized, current weight, error counts
 * - Flow Sensor: detected, initialized, flow rate, total volume, error counts
 * - Safe Mode: active/inactive
 * 
 * DEBUG LEVELS:
 * - Always prints basic status
 * - With DEBUG_VERBOSE: includes detailed diagnostics
 * 
 * USE CASES:
 * - Debugging via Serial console
 * - STATUS_FLOWING command response
 * - System health monitoring
 * 
 * @note Outputs to Serial port - ensure Serial.begin() called
 */
void printFlowingHiveEffectorStatus();

/**
 * @brief Check if safe mode is active for flowing hive effectors
 * 
 * SAFE MODE BEHAVIOR:
 * - Blocks all servo movements
 * - Prevents auto-empty sequences
 * - Allows sensor readings (non-destructive)
 * - Logged when activated/deactivated
 * 
 * ACTIVATION TRIGGERS:
 * - Manual activation via command
 * - Anomalous sensor readings
 * - Communication timeouts
 * - Critical error conditions
 * 
 * @return true if safe mode is active
 *         false if normal operation permitted
 */
bool isFlowingHiveSafeModeActive();

/**
 * @brief Activate safe mode for flowing hive effectors
 * 
 * ACTIONS TAKEN:
 * 1. Set flowingHiveSafeMode flag = true
 * 2. Log error with provided reason message
 * 3. Move servo to rest position (if initialized)
 * 4. Cancel any active auto-empty sequence
 * 5. Continue sensor monitoring (read-only)
 * 
 * RECOVERY:
 * - Call deactivateFlowingHiveSafeMode() after resolving issue
 * - System resumes normal operation automatically
 * 
 * @param reason Description of why safe mode was activated
 *               Logged for diagnostic purposes
 * 
 * @note Safe mode persists across loop iterations until explicitly cleared
 */
void activateFlowingHiveSafeMode(const char* reason);

/**
 * @brief Deactivate safe mode for flowing hive effectors
 * 
 * RECOVERY PROCESS:
 * 1. Clear flowingHiveSafeMode flag
 * 2. Log deactivation event
 * 3. Resume normal effector operation
 * 
 * PREREQUISITES:
 * - Underlying issue must be resolved
 * - Operator confirmation recommended
 * 
 * @note Does not automatically restart interrupted sequences
 *       Call executeAutoEmptySequence() again if needed
 */
void deactivateFlowingHiveSafeMode();

#endif // EFFECTORS_FLOWING_HIVE_H
