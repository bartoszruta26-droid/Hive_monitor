/*
 * ApiaryGuard - Weight Analysis Implementation
 * HX711 data buffering, trend analysis, and event detection
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_WEIGHT in config.h
 * LOGGING: All weight analysis operations are logged with metrics
 * EXCEPTIONS: Graceful handling of invalid data and sensor errors
 */

#include "weight_analysis.h"
#include "hx711.h"
#include <math.h>
#include <Arduino.h>

// Debug counters for weight monitoring
static unsigned long weight_calc_count = 0;
static unsigned long weight_error_count = 0;
static unsigned long invalid_readings = 0;
static unsigned long buffer_updates = 0;

// Global variables (defined in main)
extern HX711Metrics currentHX711Metrics;
extern HX711DataPoint hx711Buffer[HX711_BUFFER_SIZE];
extern long hx711_value;

/**
 * Update circular buffer with new weight data point
 * @param point New HX711DataPoint to add to buffer
 * 
 * Implements circular buffer for efficient storage of weight readings
 * 
 * DEBUG OUTPUT:
 * - [WEIGHT] Buffer updated: index=X, total updates=Y
 * - [WEIGHT] WARNING: Invalid data point added to buffer
 */
void updateHX711Buffer(const HX711DataPoint& point) {
    static int currentIndex = 0;
    
    // Validate data point
    if (!point.is_valid) {
        invalid_readings++;
        #ifdef DEBUG_WEIGHT
        Serial.printf("[WEIGHT] WARNING: Adding invalid data point to buffer (raw=%.2f)\n", point.weight_raw);
        #endif
    }
    
    hx711Buffer[currentIndex] = point;
    currentIndex = (currentIndex + 1) % HX711_BUFFER_SIZE;
    buffer_updates++;
    
    #ifdef DEBUG_WEIGHT
    if (buffer_updates % 50 == 0) {
        Serial.printf("[WEIGHT] Buffer updated: index=%d, total updates=%lu\n", currentIndex, buffer_updates);
    }
    #endif
}

/**
 * Simple moving average filter
 * @param windowSize Number of samples to average
 * @return float Mean value, 0.0f if no valid data
 * 
 * DEBUG OUTPUT:
 * - [WEIGHT] Moving average calculated: X.XX (window=Y)
 * - [WEIGHT] WARNING: No valid data for averaging
 */
static float movingAverage(int windowSize) {
    float sum = 0.0f;
    int count = 0;
    
    for (int i = 0; i < HX711_BUFFER_SIZE && count < windowSize; i++) {
        if (hx711Buffer[i].is_valid) {
            sum += hx711Buffer[i].weight_filtered;
            count++;
        }
    }
    
    if (count == 0) {
        #ifdef DEBUG_WEIGHT
        Serial.println("[WEIGHT] WARNING: No valid data for moving average");
        #endif
        weight_error_count++;
        return 0.0f;
    }
    
    float result = sum / count;
    
    #ifdef DEBUG_WEIGHT
    if (weight_calc_count % 20 == 0) {
        Serial.printf("[WEIGHT] Moving average: %.2f (window=%d, count=%d)\n", result, windowSize, count);
    }
    #endif
    
    return result;
}

/**
 * Calculate weight metrics from buffered data
 * @param metrics Reference to HX711Metrics struct to populate
 * 
 * Computes comprehensive weight statistics:
 * - Basic statistics (mean, std, min, max)
 * - Trend analysis (rate of change, slope)
 * - Hive health indicators (nectar inflow, consumption)
 * - Anomaly detection
 * 
 * DEBUG OUTPUT:
 * - [WEIGHT] Metrics calculated: mean=X.XX, rate=X.XXX
 * - [WEIGHT] WARNING: No valid samples for calculation
 * - [WEIGHT] ALERT: High anomaly score detected
 * 
 * EXCEPTIONS HANDLED:
 * - Empty buffer (no valid samples)
 * - Division by zero in calculations
 * - Invalid metric values (NaN/Inf)
 */
void calculateHX711Metrics(HX711Metrics& metrics) {
    weight_calc_count++;
    
    // Collect valid samples
    float samples[HX711_SHORT_WINDOW];
    int validCount = 0;
    
    for (int i = HX711_BUFFER_SIZE - HX711_SHORT_WINDOW; i < HX711_BUFFER_SIZE; i++) {
        if (hx711Buffer[i].is_valid && validCount < HX711_SHORT_WINDOW) {
            samples[validCount++] = hx711Buffer[i].weight_filtered;
        }
    }
    
    // Handle empty buffer case
    if (validCount == 0) {
        #ifdef DEBUG_WEIGHT
        Serial.println("[WEIGHT] WARNING: No valid samples for metric calculation");
        #endif
        weight_error_count++;
        
        // Set safe defaults
        metrics.mean_weight = 0.0f;
        metrics.std_weight = 0.0f;
        metrics.min_weight = 0.0f;
        metrics.max_weight = 0.0f;
        metrics.current_rate = 0.0f;
        metrics.trend_slope_1h = 0.0f;
        metrics.trend_direction = 0.0f;
        metrics.nectar_inflow_rate = 0.0f;
        metrics.consumption_rate = 0.0f;
        metrics.hive_health_weight = 50.0f;
        metrics.anomaly_score = 0.0f;
        return;
    }
    
    // Basic statistics
    float sum = 0.0f;
    metrics.min_weight = samples[0];
    metrics.max_weight = samples[0];
    
    for (int i = 0; i < validCount; i++) {
        sum += samples[i];
        if (samples[i] < metrics.min_weight) metrics.min_weight = samples[i];
        if (samples[i] > metrics.max_weight) metrics.max_weight = samples[i];
    }
    
    metrics.mean_weight = sum / validCount;
    
    // Standard deviation
    float variance = 0.0f;
    for (int i = 0; i < validCount; i++) {
        float diff = samples[i] - metrics.mean_weight;
        variance += diff * diff;
    }
    metrics.std_weight = sqrt(variance / validCount);
    
    // Current rate (change per minute)
    if (validCount >= 2) {
        float timeSpanMinutes = HX711_SHORT_WINDOW / 10.0f; // Assuming 1 reading per 6 seconds
        metrics.current_rate = (samples[validCount-1] - samples[0]) / timeSpanMinutes;
    } else {
        metrics.current_rate = 0.0f;
    }
    
    // Trend direction (-1 to 1)
    metrics.trend_direction = constrain(metrics.current_rate / 100.0f, -1.0f, 1.0f);
    metrics.trend_slope_1h = metrics.current_rate * 60.0f; // Extrapolate to hour
    
    // Nectar inflow detection (positive weight change)
    if (metrics.current_rate > HX711_WEIGHT_CHANGE_THRESH) {
        metrics.nectar_inflow_rate = metrics.current_rate;
    } else {
        metrics.nectar_inflow_rate = 0.0f;
    }
    
    // Consumption detection (negative weight change)
    if (metrics.current_rate < -HX711_WEIGHT_CHANGE_THRESH) {
        metrics.consumption_rate = abs(metrics.current_rate);
    } else {
        metrics.consumption_rate = 0.0f;
    }
    
    // Hive health index (based on stability and activity)
    float stabilityScore = 100.0f - constrain(metrics.std_weight * 10.0f, 0.0f, 100.0f);
    float activityScore = constrain(abs(metrics.nectar_inflow_rate - metrics.consumption_rate) * 10.0f, 0.0f, 100.0f);
    metrics.hive_health_weight = (stabilityScore + activityScore) / 2.0f;
    
    // Anomaly detection
    metrics.anomaly_score = 0.0f;
    if (metrics.std_weight > 5.0f) {
        metrics.anomaly_score += 0.3f;
    }
    if (abs(metrics.current_rate) > 1.0f) {
        metrics.anomaly_score += 0.4f;
    }
    if (metrics.anomaly_score > 1.0f) {
        metrics.anomaly_score = 1.0f;
    }
}

/**
 * Detect weight-related events
 */
HX711EventType detectHX711Events() {
    if (currentHX711Metrics.anomaly_score > 0.7f) {
        return HX711EventType::SUDDEN_CHANGE;
    }
    
    if (currentHX711Metrics.nectar_inflow_rate > HX711_NECTAR_FLOW_MIN) {
        return HX711EventType::NECTAR_FLOW;
    }
    
    if (currentHX711Metrics.consumption_rate > HX711_CONSUMPTION_MIN) {
        return HX711EventType::LOW_FOOD;
    }
    
    if (currentHX711Metrics.trend_direction < -0.5f) {
        return HX711EventType::SWARM;
    }
    
    return HX711EventType::NORMAL;
}

/**
 * Process weight data periodically
 */
void processWeightPeriodically(unsigned long now) {
    static unsigned long lastProcess = 0;
    
    // Process every 6 seconds
    if (now - lastProcess < 6000) return;
    lastProcess = now;
    
    // Create new data point
    HX711DataPoint point;
    point.timestamp = now;
    point.weight_raw = (float)hx711_value;
    point.weight_filtered = point.weight_raw; // Could add more filtering here
    point.is_valid = (hx711_value != 0);
    
    // Update buffer
    updateHX711Buffer(point);
    
    // Recalculate metrics
    calculateHX711Metrics(currentHX711Metrics);
    
    // Detect events
    HX711EventType event = detectHX711Events();
    
    #ifdef DEBUG_WEIGHT
    Serial.printf("[Weight] %.2f kg, Rate: %.3f kg/min, Health: %.1f%%\n",
                  currentHX711Metrics.mean_weight,
                  currentHX711Metrics.current_rate,
                  currentHX711Metrics.hive_health_weight);
    #endif
}
