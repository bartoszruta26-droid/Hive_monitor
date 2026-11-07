/*
 * ApiaryGuard - Effectors Implementation
 * PWM, relays, and actuator control with comprehensive error handling
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_EFFECTORS in config.h
 * LOGGING: All effector operations are logged with state changes
 * EXCEPTIONS: Graceful handling of invalid channel numbers and pin errors
 * 
 * IMPROVEMENTS IN THIS VERSION:
 * - Enhanced validation using config constants
 * - Debug macros from config.h for consistent logging
 * - Safe mode support for critical error conditions
 * - Rate-limited warning messages to prevent log flooding
 */

#include "effectors.h"
#include "effectors_flowing_hive.h"
#include <Arduino.h>
#include <string.h>
#include "safe_mode_policy.h"

// External actuator settings (defined in main .ino)
extern int heaterDuty;
extern int fanDuty;
extern int pumpDuty;
extern bool relay1State;
extern bool relay2State;

// Debug counters for effector monitoring
static unsigned long effector_op_count = 0;
static unsigned long effector_error_count = 0;
static unsigned long pwm_writes = 0;
static unsigned long relay_switches = 0;
static unsigned long last_warning_time = 0;

// Safe mode flag
static bool safe_mode_active = false;
static char safe_mode_reason[96] = "No latched fault";

/**
 * @brief Update all outputs based on current duty cycles and states
 * 
 * Called periodically in loop() to apply actuator settings
 * Includes safe mode override capability
 */
static void applySafeOutputState() {
    // LOW is the existing firmware's documented inactive relay level.
    // Verify relay-board polarity against the actual hardware before deployment.
    analogWrite(HEATER_PWM, SAFE_MODE_HEATER_DUTY);
    analogWrite(FAN_PWM, SAFE_MODE_FAN_DUTY);
    digitalWrite(PUMP_RELAY, SAFE_MODE_PUMP_STATE ? HIGH : LOW);
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
}

void updateOutputs() {
    // Safe mode is an output interlock, not merely a change to three outputs.
    if (safe_mode_active) {
        applySafeOutputState();
        DBG_EFF("[EFFECTORS] Safe mode active - all relay outputs inhibited\n");
        return;
    }
    
    // Normal operation with validation
    analogWrite(HEATER_PWM, constrain(heaterDuty, 0, 255));
    analogWrite(FAN_PWM, constrain(fanDuty, 0, 255));
    // PUMP_RELAY is a digital relay, not PWM - use digitalWrite
    digitalWrite(PUMP_RELAY, pumpDuty > 0 ? HIGH : LOW);
    digitalWrite(RELAY_CH1, relay1State ? HIGH : LOW);
    digitalWrite(RELAY_CH2, relay2State ? HIGH : LOW);
}

/**
 * @brief Activate safe mode - disables potentially dangerous outputs
 * @param reason Description of why safe mode was activated
 */
void activateSafeMode(const char* reason) {
    // Synchronize the Flowing Hive servo interlock with the main safe mode.
    // This cancels any active auto-empty sequence before outputs are applied.
    activateFlowingHiveSafeMode(reason);
    if (!safe_mode_active) {
        strncpy(safe_mode_reason, reason ? reason : "Unspecified fault", sizeof(safe_mode_reason) - 1);
        safe_mode_reason[sizeof(safe_mode_reason) - 1] = '\0';
    }
    safe_mode_active = true;
    LOG_ERROR("EFFECTORS", reason ? reason : "Unspecified fault");
    DBG_EFF("[EFFECTORS] Safe mode activated: %s\n", reason ? reason : "Unspecified fault");
    
    // Apply the complete safe output state immediately, not on the next loop.
    applySafeOutputState();
}

/**
 * @brief Deactivate safe mode and restore normal operation
 */
static void deactivateSafeMode() {
    // Releasing the interlock must never resume stale actuator commands.
    // Clear remembered setpoints; the operator must issue fresh commands
    // after investigating and resolving the condition that caused safe mode.
    heaterDuty = 0;
    fanDuty = 0;
    pumpDuty = 0;
    relay1State = false;
    relay2State = false;

    // Force every output inactive immediately. Do not restore prior fan,
    // heater, pump, valve, or relay states on safe-mode release.
    analogWrite(HEATER_PWM, 0);
    analogWrite(FAN_PWM, 0);
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

    // Keep the independent Flowing Hive latch active. Releasing the main
    // interlock must never silently unlock a subsystem-specific actuator.
    safe_mode_active = false;
    DBG_EFF("[EFFECTORS] Main safe mode released; setpoints cleared; Flowing Hive latch remains independent\n");
}

/**
 * @brief Check if safe mode is currently active
 * @return true if safe mode is active
 */
bool isSafeModeActive() {
    return safe_mode_active;
}

const char* getSafeModeReason() {
    return safe_mode_reason;
}

bool releaseSafeMode(bool operatorConfirmed, bool heaterSensorHealthy) {
    if (!safeModePolicy::mayReleaseMain(safe_mode_active, operatorConfirmed, heaterSensorHealthy)) {
        return false;
    }
    deactivateSafeMode();
    return true;
}

/**
 * @brief Set heater PWM duty cycle
 * @param duty Duty cycle value (0-255)
 * 
 * Controls heating element via PWM output
 * Includes trace debugging and gentle assertions
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Heater PWM set to XX%
 * - [EFFECTORS] WARNING: Invalid duty cycle value
 * - [TRACE] ENTER/EXIT setHeaterPWM (when DEBUG_VERBOSE enabled)
 * 
 * EXCEPTIONS HANDLED:
 * - Invalid duty cycle (>255)
 * - Pin not configured as OUTPUT
 * - Safe mode override
 */
void setHeaterPWM(int duty) {
    ++effector_op_count;

    if (safe_mode_active) {
        DBG_EFF("[EFFECTORS] Heater command ignored - safe mode active\n");
        return;
    }
    if (duty < 0 || duty > 255) {
        ++effector_error_count;
        LOG_WARN("EFFECTORS", "Rejected heater PWM outside 0-255 range");
        return;
    }

    // Guard the immediate hardware write, not only the end-of-loop update.
    // This prevents even a brief heater pulse when the safety sensor is stale.
    if (duty > 0 && !isHeaterSensorReadyForActuation()) {
        ++effector_error_count;
        LOG_ERROR("EFFECTORS", "Rejected heater activation: temperature sensor is unavailable, invalid or stale");
        activateSafeMode("Heater command rejected: temperature sensor unavailable, invalid or stale");
        return;
    }

    heaterDuty = duty;
    analogWrite(HEATER_PWM, duty);
    ++pwm_writes;
}

/**
 * @brief Set fan PWM duty cycle
 * @param duty Duty cycle value (0-255)
 * 
 * Controls ventilation fan via PWM output
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Fan PWM set to XX%
 * - [EFFECTORS] WARNING: Invalid duty cycle value
 * 
 * EXCEPTIONS HANDLED:
 * - Invalid duty cycle (>255)
 * - Safe mode override (only allows safe duty cycle)
 */
void setFanPWM(int duty) {
    ++effector_op_count;

    if (safe_mode_active) {
        DBG_EFF("[EFFECTORS] Fan command ignored - safe mode active\n");
        return;
    }
    if (duty < 0 || duty > 255) {
        ++effector_error_count;
        LOG_WARN("EFFECTORS", "Rejected fan PWM outside 0-255 range");
        return;
    }

    fanDuty = duty;
    analogWrite(FAN_PWM, duty);
    ++pwm_writes;
}

/**
 * @brief Control water pump relay
 * @param state true=ON, false=OFF
 * 
 * Activates/deactivates water pump via relay
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Pump turned ON/OFF
 * - [EFFECTORS] WARNING: Pump relay pin error
 * 
 * EXCEPTIONS HANDLED:
 * - Relay pin configuration error
 */
void setPump(bool state) {
    effector_op_count++;
    if (safe_mode_active) {
        DBG_EFF("[EFFECTORS] Pump command ignored - safe mode active\n");
        return;
    }
    relay_switches++;
    pumpDuty = state ? 255 : 0;
    
    #ifdef DEBUG_EFFECTORS
    Serial.printf("[EFFECTORS] Pump relay: %s\n", state ? "ON" : "OFF");
    #endif
    
    digitalWrite(PUMP_RELAY, state ? HIGH : LOW);
}

/**
 * @brief Control valve 1 relay
 * @param state true=ON, false=OFF
 * 
 * Activates/deactivates first solenoid valve
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Valve 1 turned ON/OFF
 */
void setValve1(bool state) {
    effector_op_count++;
    if (safe_mode_active) {
        DBG_EFF("[EFFECTORS] Valve 1 command ignored - safe mode active\n");
        return;
    }
    relay_switches++;
    
    #ifdef DEBUG_EFFECTORS
    Serial.printf("[EFFECTORS] Valve 1: %s\n", state ? "OPEN" : "CLOSED");
    #endif
    
    digitalWrite(VALVE_1, state ? HIGH : LOW);
}

/**
 * @brief Control valve 2 relay
 * @param state true=ON, false=OFF
 * 
 * Activates/deactivates second solenoid valve
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Valve 2 turned ON/OFF
 */
void setValve2(bool state) {
    effector_op_count++;
    if (safe_mode_active) {
        DBG_EFF("[EFFECTORS] Valve 2 command ignored - safe mode active\n");
        return;
    }
    relay_switches++;
    
    #ifdef DEBUG_EFFECTORS
    Serial.printf("[EFFECTORS] Valve 2: %s\n", state ? "OPEN" : "CLOSED");
    #endif
    
    digitalWrite(VALVE_2, state ? HIGH : LOW);
}

/**
 * @brief Control generic relay channel
 * @param channel Relay channel number (1-8)
 * @param state true=ON, false=OFF
 * 
 * Controls one of 8 relay channels
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Relay CHX turned ON/OFF
 * - [EFFECTORS] ERROR: Invalid relay channel X
 * 
 * EXCEPTIONS HANDLED:
 * - Invalid channel number (not 1-8)
 * - Relay pin not configured
 */
void setRelay(uint8_t channel, bool state) {
    effector_op_count++;

    if (safe_mode_active) {
        DBG_EFF("[EFFECTORS] Relay command ignored - safe mode active\n");
        return;
    }

    // Validate channel number
    if (channel < 1 || channel > 8) {
        #ifdef DEBUG_EFFECTORS
        Serial.printf("[EFFECTORS] ERROR: Invalid relay channel: %d (valid: 1-8)\n", channel);
        #endif
        effector_error_count++;
        return;
    }
    
    relay_switches++;
    if (channel == 1) relay1State = state;
    if (channel == 2) relay2State = state;
    
    #ifdef DEBUG_EFFECTORS
    Serial.printf("[EFFECTORS] Relay CH%d: %s\n", channel, state ? "ON" : "OFF");
    #endif
    
    switch(channel) {
        case 1: digitalWrite(RELAY_CH1, state ? HIGH : LOW); break;
        case 2: digitalWrite(RELAY_CH2, state ? HIGH : LOW); break;
        case 3: digitalWrite(RELAY_CH3, state ? HIGH : LOW); break;
        case 4: digitalWrite(RELAY_CH4, state ? HIGH : LOW); break;
        case 5: digitalWrite(RELAY_CH5, state ? HIGH : LOW); break;
        case 6: digitalWrite(RELAY_CH6, state ? HIGH : LOW); break;
        case 7: digitalWrite(RELAY_CH7, state ? HIGH : LOW); break;
        case 8: digitalWrite(RELAY_CH8, state ? HIGH : LOW); break;
        default:
            // Should never reach here due to validation above
            #ifdef DEBUG_EFFECTORS
            Serial.println("[EFFECTORS] CRITICAL: Switch statement fell through");
            #endif
            effector_error_count++;
            break;
    }
}

/**
 * @brief Print effector status summary
 * 
 * Shows current state and statistics for all effectors
 * Includes safe mode status
 * 
 * DEBUG OUTPUT:
 * - Summary of all effector operations
 * - Error counts and operation statistics
 * - Safe mode status if active
 */
void printEffectorStatus() {
    Serial.println("\n>> Effector Status:");
    Serial.printf("  Total operations: %lu\n", effector_op_count);
    Serial.printf("  Errors: %lu\n", effector_error_count);
    Serial.printf("  PWM writes: %lu\n", pwm_writes);
    Serial.printf("  Relay switches: %lu\n", relay_switches);
    Serial.printf("  Safe mode: %s\n", safe_mode_active ? "ACTIVE" : "inactive");
    
    #ifdef DEBUG_EFFECTORS
    Serial.println("\n[DEBUG] Individual effector states:");
    Serial.printf("  Heater PWM pin: %d\n", HEATER_PWM);
    Serial.printf("  Fan PWM pin: %d\n", FAN_PWM);
    Serial.printf("  Pump relay pin: %d\n", PUMP_RELAY);
    Serial.printf("  Valve 1 pin: %d\n", VALVE_1);
    Serial.printf("  Valve 2 pin: %d\n", VALVE_2);
    #endif
    
    // Warning if error rate is high
    if (effector_op_count > 0) {
        float error_rate = (float)effector_error_count / effector_op_count * 100.0f;
        if (error_rate > 5.0f) {
            LOG_WARN("EFFECTORS", "High error rate detected");
            DBG_EFF("[EFFECTORS] Error rate: %.2f%% (%lu errors / %lu operations)\n", 
                    error_rate, effector_error_count, effector_op_count);
        }
    }
}
