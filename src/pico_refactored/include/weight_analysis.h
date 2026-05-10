/*
 * ApiaryGuard - Weight Analysis Module Header  
 * HX711 data buffering, trend analysis, and event detection
 */

#ifndef WEIGHT_ANALYSIS_H
#define WEIGHT_ANALYSIS_H

#include <Arduino.h>
#include "config.h"

// Single weight data point
struct HX711DataPoint {
    uint32_t timestamp;
    float weight_raw;
    float weight_filtered;
    bool is_valid;
};

// Weight metrics (simplified from 60+ params)
struct HX711Metrics {
    // Basic statistics
    float mean_weight;
    float std_weight;
    float min_weight;
    float max_weight;
    
    // Trend analysis
    float current_rate;
    float trend_slope_1h;
    float trend_direction;
    
    // Hive health indicators
    float nectar_inflow_rate;
    float consumption_rate;
    float hive_health_weight;
    float anomaly_score;
};

// Weight event types
enum class HX711EventType {
    NONE = 0,
    NORMAL,
    NECTAR_FLOW,
    SWARM,
    PREDATOR,
    LOW_FOOD,
    SUDDEN_CHANGE
};

// External variables
extern HX711Metrics currentHX711Metrics;
extern HX711DataPoint hx711Buffer[HX711_BUFFER_SIZE];

// Function declarations
void processWeightPeriodically(unsigned long now);
void updateHX711Buffer(const HX711DataPoint& point);
void calculateHX711Metrics(HX711Metrics& metrics);
HX711EventType detectHX711Events();

#endif // WEIGHT_ANALYSIS_H
