/*
 * ApiaryGuard - HX711 Weight Sensor with Robust Implementation
 * Features: Timeout handling, filtering, error detection
 */

#ifndef HX711_H
#define HX711_H

#include <Arduino.h>
#include "config.h"

// HX711 reading with timeout and error handling
long readHX711();

// Calibration variables (to be set during calibration)
extern long hx711_value;
extern const long hx711_offset;
extern const float hx711_scale;

#endif // HX711_H
