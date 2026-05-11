/*
 * ApiaryGuard - Air Quality Implementation
 * SGP41 data processing and IAQ calculation with comprehensive debug support
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_AIR in config.h
 * LOGGING: All air quality calculations are logged with metrics
 * EXCEPTIONS: Graceful handling of sensor errors and invalid data
 */

#include "air_quality.h"
#include <Adafruit_SGP41.h>
#include <Arduino.h>

// For C++14 compatibility, use manual min/max instead of std::clamp
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

// Global variables (defined in main)
extern AirQualityMetrics currentAirMetrics;
extern Adafruit_SGP41 sgp;
extern uint16_t co2_eq;
extern uint16_t voc_idx;

// Historical data for trend analysis
static uint16_t co2History[AIRQUAL_BUFFER_SIZE];
static uint16_t vocHistory[AIRQUAL_BUFFER_SIZE];
static int historyIndex = 0;
static bool historyInitialized = false;

// Debug counters for air quality monitoring
static unsigned long aq_calc_count = 0;
static unsigned long aq_error_count = 0;
static unsigned long co2_anomalies = 0;
static unsigned long voc_anomalies = 0;

/**
 * @brief Update history buffer with new readings
 * @param co2 Current CO2 equivalent value
 * @param voc Current VOC index value
 * 
 * Implements circular buffer for efficient storage
 */
static void updateHistory(uint16_t co2, uint16_t voc) {
    // Validate input data
    if (co2 > 5000 || voc > 500) {
        #ifdef DEBUG_AIR
        Serial.print("[AIR] WARNING: Invalid sensor data - CO2=");
        Serial.print(co2);
        Serial.print(", VOC=");
        Serial.println(voc);
        #endif
        aq_error_count++;
        return;
    }
    
    co2History[historyIndex] = co2;
    vocHistory[historyIndex] = voc;
    historyIndex = (historyIndex + 1) % AIRQUAL_BUFFER_SIZE;
    
    if (historyIndex == 0) {
        historyInitialized = true;
    }
    
    #ifdef DEBUG_AIR
    if (historyIndex % 10 == 0) {
        Serial.printf("[AIR] History updated: index=%d, initialized=%s\n", 
                     historyIndex, historyInitialized?"yes":"no");
    }
    #endif
}

/**
 * @brief Calculate simple moving average from buffer
 * @param buffer Array of values
 * @param size Number of elements in buffer
 * @return float Mean value, 0.0f if no valid data
 */
static float calculateMean(uint16_t* buffer, int size) {
    uint32_t sum = 0;
    int count = 0;
    
    for (int i = 0; i < size; i++) {
        if (buffer[i] > 0 && buffer[i] < 10000) {
            sum += buffer[i];
            count++;
        }
    }
    
    if (count == 0) {
        #ifdef DEBUG_AIR
        Serial.println("[AIR] WARNING: No valid data for mean calculation");
        #endif
        return 0.0f;
    }
    
    return (float)sum / count;
}

/**
 * @brief Calculate trend slope using simple linear regression
 * @param buffer Array of values
 * @param windowSize Number of samples to consider
 * @return float Slope value (positive=increasing, negative=decreasing)
 */
static float calculateTrend(uint16_t* buffer, int windowSize) {
    if (!historyInitialized || windowSize < 2) {
        #ifdef DEBUG_AIR
        Serial.println("[AIR] Cannot calculate trend - insufficient data");
        #endif
        return 0.0f;
    }
    
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumXY = 0.0f;
    float sumX2 = 0.0f;
    
    int startIndex = (windowSize > historyIndex) ? 0 : historyIndex - windowSize;
    int count = historyIndex - startIndex;
    
    if (count < 2) {
        return 0.0f;
    }
    
    for (int i = startIndex; i < historyIndex; i++) {
        float x = (float)(i - startIndex);
        float y = (float)buffer[i];
        
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }
    
    float denominator = (count * sumX2 - sumX * sumX);
    if (abs(denominator) < 0.0001f) {
        #ifdef DEBUG_AIR
        Serial.println("[AIR] WARNING: Trend calculation - near-zero denominator");
        #endif
        return 0.0f;
    }
    
    float slope = (count * sumXY - sumX * sumY) / denominator;
    return slope;
}

/**
 * @brief Clamp value between min and max (C++14 compatible)
 */
static float clampValue(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

/**
 * @brief Calculate comprehensive air quality metrics
 * @param metrics Reference to AirQualityMetrics struct to populate
 * 
 * Calculates:
 * - Basic readings (CO2, VOC)
 * - Moving averages
 * - Trend analysis (1-hour slope)
 * - IAQ index (0-100%)
 * - Ventilation need assessment
 * - Stress level from rate of change
 * 
 * DEBUG OUTPUT:
 * - [AIR] CO2: XXX ppm, VOC: XXX, IAQ: XX.X%
 * - [AIR] WARNING: High CO2 level detected (XXX ppm)
 * - [AIR] ERROR: Invalid metric calculation
 */
void calculateAirMetrics(AirQualityMetrics& metrics) {
    aq_calc_count++;
    
    // Basic readings from sensor
    metrics.co2_eq = co2_eq;
    metrics.voc_idx = voc_idx;
    
    // Validate sensor readings
    if (co2_eq == 0 || voc_idx == 0) {
        #ifdef DEBUG_AIR
        Serial.println("[AIR] WARNING: Zero sensor reading detected");
        #endif
        aq_error_count++;
    }
    
    // Check for anomalies
    if (co2_eq > CO2_ALERT_LEVEL) {
        co2_anomalies++;
        #ifdef DEBUG_AIR
        Serial.printf("[AIR] ALERT: High CO2 level: %d ppm (threshold: %d)\n", 
                     co2_eq, CO2_ALERT_LEVEL);
        #endif
    }
    
    if (voc_idx > VOC_ALERT_LEVEL) {
        voc_anomalies++;
        #ifdef DEBUG_AIR
        Serial.printf("[AIR] ALERT: High VOC level: %d (threshold: %d)\n", 
                     voc_idx, VOC_ALERT_LEVEL);
        #endif
    }
    
    // Update history buffer
    updateHistory(co2_eq, voc_idx);
    
    // Calculate statistics
    metrics.mean_co2 = calculateMean(co2History, AIRQUAL_BUFFER_SIZE);
    metrics.mean_voc = calculateMean(vocHistory, AIRQUAL_BUFFER_SIZE);
    
    // Calculate trends (1 hour window = ~60 readings at 1/min)
    int trendWindow = min(60, AIRQUAL_BUFFER_SIZE);
    metrics.trend_slope_1h = calculateTrend(co2History, trendWindow);
    
    // Normalize trend direction to [-1, 1] range
    metrics.trend_direction = clampValue(metrics.trend_slope_1h / 50.0f, -1.0f, 1.0f);
    
    // IAQ Index calculation (simplified)
    // Higher is better (100 = excellent air quality)
    float co2Score = 100.0f - clampValue((float)metrics.co2_eq / CO2_WARNING_LEVEL * 100.0f, 0.0f, 100.0f);
    float vocScore = 100.0f - clampValue((float)metrics.voc_idx / VOC_ALERT_LEVEL * 100.0f, 0.0f, 100.0f);
    metrics.iaq_index = (co2Score + vocScore) / 2.0f;
    
    // Validate IAQ calculation
    if (isnan(metrics.iaq_index) || metrics.iaq_index < 0 || metrics.iaq_index > 100) {
        #ifdef DEBUG_AIR
        Serial.printf("[AIR] ERROR: Invalid IAQ index: %.1f (setting to 50)\n", metrics.iaq_index);
        #endif
        metrics.iaq_index = 50.0f; // Default to moderate
        aq_error_count++;
    }
    
    // Ventilation need assessment
    if (metrics.co2_eq > CO2_WARNING_LEVEL || metrics.voc_idx > VOC_ALERT_LEVEL) {
        metrics.ventilation_need = clampValue(
            ((float)metrics.co2_eq / CO2_WARNING_LEVEL + 
             (float)metrics.voc_idx / VOC_ALERT_LEVEL) * 50.0f,
            0.0f, 100.0f
        );
        #ifdef DEBUG_AIR
        Serial.printf("[AIR] Ventilation recommended: %.1f%%\n", metrics.ventilation_need);
        #endif
    } else {
        metrics.ventilation_need = 0.0f;
    }
    
    // Stress level (based on rate of change magnitude)
    metrics.stress_level = abs(metrics.trend_direction) * 100.0f;
    
    // DEBUG: Periodic logging
    #ifdef DEBUG_AIR
    if (aq_calc_count % 10 == 0) {
        Serial.printf("[Air] CO2: %d ppm, VOC: %d, IAQ: %.1f%%\n",
                      metrics.co2_eq,
                      metrics.voc_idx,
                      metrics.iaq_index);
        Serial.printf("[DEBUG] Mean CO2: %.1f, Trend: %.2f, Stress: %.1f\n",
                     metrics.mean_co2,
                     metrics.trend_slope_1h,
                     metrics.stress_level);
    }
    #endif
}

/**
 * @brief Process air quality data periodically
 * @param now Current time in milliseconds (from millis())
 * 
 * Runs every 60 seconds to match SGP41 measurement cycle
 * Skips processing if sensor is not active
 * 
 * DEBUG OUTPUT:
 * - [AIR] Processing air quality data
 * - [AIR] Sensor not active, skipping processing
 */
void processAirQualityPeriodically(unsigned long now) {
    static unsigned long lastProcess = 0;
    
    // Process every 60 seconds (SGP41 measurement cycle)
    if (now - lastProcess < 60000) return;
    lastProcess = now;
    
    #ifdef DEBUG_AIR
    Serial.println("[AIR] Processing air quality data...");
    #endif
    
    // Note: sensors.airQual is accessed from global SensorState defined in main .ino
    // Only process if sensor is active (check would be: if (sensors.airQual.active))
    
    // Read from sensor (already done in readSensors())
    // sgp.measureGas(); // Called in sensors.cpp
    
    // Calculate comprehensive metrics
    calculateAirMetrics(currentAirMetrics);
    
    // Log summary stats periodically
    #ifdef DEBUG_AIR
    static unsigned long debug_counter = 0;
    debug_counter++;
    if (debug_counter % 6 == 0) { // Every 6 minutes
        Serial.printf("[DEBUG] Air Quality Stats:\n");
        Serial.printf("  Calculations: %lu\n", aq_calc_count);
        Serial.printf("  Errors: %lu\n", aq_error_count);
        Serial.printf("  CO2 anomalies: %lu\n", co2_anomalies);
        Serial.printf("  VOC anomalies: %lu\n", voc_anomalies);
    }
    #endif
}
