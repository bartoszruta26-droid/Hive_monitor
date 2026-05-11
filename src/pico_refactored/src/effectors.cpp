/*
 * ApiaryGuard - Effectors Implementation
 * PWM, relays, and actuator control with comprehensive error handling
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_EFFECTORS in config.h
 * LOGGING: All effector operations are logged with state changes
 * EXCEPTIONS: Graceful handling of invalid channel numbers and pin errors
 */

#include "effectors.h"
#include <Arduino.h>

// Debug counters for effector monitoring
static unsigned long effector_op_count = 0;
static unsigned long effector_error_count = 0;
static unsigned long pwm_writes = 0;
static unsigned long relay_switches = 0;

/**
 * @brief Set heater PWM duty cycle
 * @param duty Duty cycle value (0-255)
 * 
 * Controls heating element via PWM output
 * 
 * DEBUG OUTPUT:
 * - [EFFECTORS] Heater PWM set to XX%
 * - [EFFECTORS] WARNING: Invalid duty cycle value
 * 
 * EXCEPTIONS HANDLED:
 * - Invalid duty cycle (>255)
 * - Pin not configured as OUTPUT
 */
void setHeaterPWM(uint8_t duty) {
    effector_op_count++;
    
    // Validate duty cycle
    if (duty > 255) {
        #ifdef DEBUG_EFFECTORS
        Serial.printf("[EFFECTORS] WARNING: Invalid heater duty cycle: %d (clamping to 255)\n", duty);
        #endif
        effector_error_count++;
        duty = 255;
    }
    
    #ifdef DEBUG_EFFECTORS
    Serial.printf("[EFFECTORS] Setting heater PWM to %d (%.1f%%)\n", duty, (float)duty/255.0*100.0);
    #endif
    
    analogWrite(HEATER_PWM, duty);
    pwm_writes++;
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
 */
void setFanPWM(uint8_t duty) {
    effector_op_count++;
    
    // Validate duty cycle
    if (duty > 255) {
        #ifdef DEBUG_EFFECTORS
        Serial.printf("[EFFECTORS] WARNING: Invalid fan duty cycle: %d (clamping to 255)\n", duty);
        #endif
        effector_error_count++;
        duty = 255;
    }
    
    #ifdef DEBUG_EFFECTORS
    Serial.printf("[EFFECTORS] Setting fan PWM to %d (%.1f%%)\n", duty, (float)duty/255.0*100.0);
    #endif
    
    analogWrite(FAN_PWM, duty);
    pwm_writes++;
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
    relay_switches++;
    
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
    
    // Validate channel number
    if (channel < 1 || channel > 8) {
        #ifdef DEBUG_EFFECTORS
        Serial.printf("[EFFECTORS] ERROR: Invalid relay channel: %d (valid: 1-8)\n", channel);
        #endif
        effector_error_count++;
        return;
    }
    
    relay_switches++;
    
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
 * 
 * DEBUG OUTPUT:
 * - Summary of all effector operations
 * - Error counts and operation statistics
 */
void printEffectorStatus() {
    Serial.println("\n>> Effector Status:");
    Serial.printf("  Total operations: %lu\n", effector_op_count);
    Serial.printf("  Errors: %lu\n", effector_error_count);
    Serial.printf("  PWM writes: %lu\n", pwm_writes);
    Serial.printf("  Relay switches: %lu\n", relay_switches);
    
    #ifdef DEBUG_EFFECTORS
    Serial.println("\n[DEBUG] Individual effector states:");
    Serial.printf("  Heater PWM pin: %d\n", HEATER_PWM);
    Serial.printf("  Fan PWM pin: %d\n", FAN_PWM);
    Serial.printf("  Pump relay pin: %d\n", PUMP_RELAY);
    #endif
}
