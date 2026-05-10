/*
 * ApiaryGuard - Air Quality Module Header
 * SGP41 data processing and IAQ calculation
 */

#ifndef AIR_QUALITY_H
#define AIR_QUALITY_H

#include <Arduino.h>
#include "config.h"

// Air quality metrics (simplified from 24+ params)
struct AirQualityMetrics {
    // Basic readings
    uint16_t co2_eq;
    uint16_t voc_idx;
    
    // Statistics
    float mean_co2;
    float mean_voc;
    
    // Trend analysis
    float trend_slope_1h;
    float trend_direction;
    
    // Health indices
    float iaq_index;
    float ventilation_need;
    float stress_level;
};

// External variables
extern AirQualityMetrics currentAirMetrics;

// Function declarations
void processAirQualityPeriodically(unsigned long now);
void calculateAirMetrics(AirQualityMetrics& metrics);

#endif // AIR_QUALITY_H
