/*
 * ApiaryGuard - Flowing Hive Effector Control Header
 * Servo control for automatic frame emptying, second HX711 for superstructure weight,
 * and flow sensor for honey output monitoring
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_SERVO and DEBUG_FLOW in config.h
 * 
 * FEATURES:
 * - Servo control for Flowing Hive frame tilting
 * - Second HX711 weight sensor for superstructure monitoring
 * - Flow sensor integration for honey output measurement
 * - Safe mode support for critical error conditions
 * - Input validation for all functions
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
 * Sets up PWM for servo control on SERVO_EMPTY_PIN
 */
void initServoControl();

/**
 * @brief Set servo to specific angle
 * @param angle Angle in degrees (0-180)
 * 
 * Controls the servo motor that tilts the Flowing Hive frames
 * for automatic honey extraction
 */
void setServoAngle(uint16_t angle);

/**
 * @brief Move servo to rest position
 * Positions frames in normal bee-accessible configuration
 */
void setServoRestPosition();

/**
 * @brief Move servo to emptying position
 * Tilts frames to drain honey through the collection system
 */
void setServoEmptyPosition();

/**
 * @brief Execute automatic frame emptying sequence
 * @param duration_ms Duration to hold emptying position (default: 30 minutes)
 * 
 * Complete automated sequence:
 * 1. Move to emptying position
 * 2. Wait for honey to drain
 * 3. Return to rest position
 */
void executeAutoEmptySequence(unsigned long duration_ms = 1800000);

/**
 * @brief Update servo control loop (call periodically)
 */
void updateServoLoop();

// ============================================================================
// SECOND HX711 (SUPERSTRUCTURE WEIGHT) FUNCTIONS
// ============================================================================

/**
 * @brief Initialize second HX711 weight sensor
 * Reads superstructure/additional box weight
 */
void initHX711_2();

/**
 * @brief Read second HX711 sensor
 * @return long Weight value in ADC counts, 0 on error/timeout
 */
long readHX711_2();

/**
 * @brief Get superstructure weight in grams
 * @return float Weight in grams (calibrated)
 */
float getSuperstructureWeightGrams();

/**
 * @brief Calibrate/tare second HX711
 */
void tareHX711_2();

// ============================================================================
// FLOW SENSOR FUNCTIONS
// ============================================================================

/**
 * @brief Initialize flow sensor with interrupt
 * Sets up interrupt handler for pulse counting
 */
void initFlowSensor();

/**
 * @brief Get current flow rate
 * @return float Flow rate in liters per minute
 */
float getFlowRateLPM();

/**
 * @brief Get total volume flowed since last reset
 * @return float Total volume in liters
 */
float getTotalVolumeLiters();

/**
 * @brief Reset flow volume counter
 */
void resetFlowCounter();

/**
 * @brief Update flow sensor calculations (call periodically)
 */
void updateFlowSensor();

/**
 * @brief Check if honey is currently flowing
 * @return true if flow detected above minimum threshold
 */
bool isHoneyFlowing();

// ============================================================================
// COMBINED EFFECTOR STATUS
// ============================================================================

/**
 * @brief Print complete effector status including new devices
 */
void printFlowingHiveEffectorStatus();

/**
 * @brief Check if safe mode is active for flowing hive effectors
 * @return true if safe mode is active
 */
bool isFlowingHiveSafeModeActive();

/**
 * @brief Activate safe mode for flowing hive effectors
 * @param reason Description of why safe mode was activated
 */
void activateFlowingHiveSafeMode(const char* reason);

/**
 * @brief Deactivate safe mode for flowing hive effectors
 */
void deactivateFlowingHiveSafeMode();

#endif // EFFECTORS_FLOWING_HIVE_H
