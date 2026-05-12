/*
 * ApiaryGuard - Effectors Control Header
 * PWM, relays, and actuator control with error handling
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_EFFECTORS in config.h
 * 
 * FEATURES:
 * - Safe mode support for critical error conditions
 * - Debug macros for conditional logging
 * - Input validation for all functions
 */

#ifndef EFFECTORS_H
#define EFFECTORS_H

#include <Arduino.h>
#include "config.h"

// Function declarations - PWM control
void setHeaterPWM(uint8_t duty);
void setFanPWM(uint8_t duty);

// Function declarations - Relay control
void setPump(bool state);
void setValve1(bool state);
void setValve2(bool state);
void setRelay(uint8_t channel, bool state);

// Status and utility functions
void printEffectorStatus();
void updateOutputs();

// Safe mode functions
void activateSafeMode(const char* reason);
void deactivateSafeMode();
bool isSafeModeActive();

#endif // EFFECTORS_H
