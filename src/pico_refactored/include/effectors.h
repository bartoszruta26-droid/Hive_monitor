/*
 * ApiaryGuard - Effectors Control Header
 * PWM, relays, and actuator control with error handling
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_EFFECTORS in config.h
 */

#ifndef EFFECTORS_H
#define EFFECTORS_H

#include <Arduino.h>
#include "config.h"

// Function declarations
void setHeaterPWM(uint8_t duty);
void setFanPWM(uint8_t duty);
void setPump(bool state);
void setValve1(bool state);
void setValve2(bool state);
void setRelay(uint8_t channel, bool state);
void printEffectorStatus(); // New function for status reporting

#endif // EFFECTORS_H
