/*
 * ApiaryGuard - Air Quality Implementation
 * SGP41 data processing and IAQ calculation
 */

#include "air_quality.h"
#include <Adafruit_SGP41.h>

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

/**
 * Update history buffer
 */
static void updateHistory(uint16_t co2, uint16_t voc) {
    co2History[historyIndex] = co2;
    vocHistory[historyIndex] = voc;
    historyIndex = (historyIndex + 1) % AIRQUAL_BUFFER_SIZE;
    
    if (historyIndex == 0) {
        historyInitialized = true;
    }
}

/**
 * Calculate simple moving average
 */
static float calculateMean(uint16_t* buffer, int size) {
    uint32_t sum = 0;
    int count = 0;
    
    for (int i = 0; i < size; i++) {
        if (buffer[i] > 0) {
            sum += buffer[i];
            count++;
        }
    }
    
    return (count > 0) ? (float)sum / count : 0.0f;
}

/**
 * Calculate trend slope (simple linear regression)
 */
static float calculateTrend(uint16_t* buffer, int windowSize) {
    if (!historyInitialized || windowSize < 2) {
        return 0.0f;
    }
    
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumXY = 0.0f;
    float sumX2 = 0.0f;
    
    int startIndex = (windowSize > historyIndex) ? 0 : historyIndex - windowSize;
    int count = historyIndex - startIndex;
    
    if (count < 2) return 0.0f;
    
    for (int i = startIndex; i < historyIndex; i++) {
        float x = (float)(i - startIndex);
        float y = (float)buffer[i];
        
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }
    
    float slope = (count * sumXY - sumX * sumY) / (count * sumX2 - sumX * sumX);
    return slope;
}

/**
 * Calculate air quality metrics
 */
void calculateAirMetrics(AirQualityMetrics& metrics) {
    // Basic readings from sensor
    metrics.co2_eq = co2_eq;
    metrics.voc_idx = voc_idx;
    
    // Update history
    updateHistory(co2_eq, voc_idx);
    
    // Calculate statistics
    metrics.mean_co2 = calculateMean(co2History, AIRQUAL_BUFFER_SIZE);
    metrics.mean_voc = calculateMean(vocHistory, AIRQUAL_BUFFER_SIZE);
    
    // Calculate trends (1 hour window = ~60 readings at 1/min)
    int trendWindow = min(60, AIRQUAL_BUFFER_SIZE);
    metrics.trend_slope_1h = calculateTrend(co2History, trendWindow);
    metrics.trend_direction = constrain(metrics.trend_slope_1h / 50.0f, -1.0f, 1.0f);
    
    // IAQ Index (simplified calculation)
    float co2Score = 100.0f - constrain((float)metrics.co2_eq / CO2_WARNING_LEVEL * 100.0f, 0.0f, 100.0f);
    float vocScore = 100.0f - constrain((float)metrics.voc_idx / VOC_ALERT_LEVEL * 100.0f, 0.0f, 100.0f);
    metrics.iaq_index = (co2Score + vocScore) / 2.0f;
    
    // Ventilation need
    if (metrics.co2_eq > CO2_WARNING_LEVEL || metrics.voc_idx > VOC_ALERT_LEVEL) {
        metrics.ventilation_need = constrain(
            ((float)metrics.co2_eq / CO2_WARNING_LEVEL + 
             (float)metrics.voc_idx / VOC_ALERT_LEVEL) * 50.0f,
            0.0f, 100.0f
        );
    } else {
        metrics.ventilation_need = 0.0f;
    }
    
    // Stress level (based on rate of change)
    metrics.stress_level = abs(metrics.trend_direction) * 100.0f;
}

/**
 * Process air quality data periodically
 */
void processAirQualityPeriodically(unsigned long now) {
    static unsigned long lastProcess = 0;
    
    // Process every 60 seconds (SGP41 measurement cycle)
    if (now - lastProcess < 60000) return;
    lastProcess = now;
    
    // Only process if sensor is active
    // Note: sensors.airQual is accessed from global SensorState defined in main .ino
    
    // Read from sensor (already done in readSensors())
    // sgp.measureGas(); // Called in sensors.cpp
    
    // Calculate metrics
    calculateAirMetrics(currentAirMetrics);
    
    #ifdef DEBUG_AIR
    Serial.printf("[Air] CO2: %d ppm, VOC: %d, IAQ: %.1f%%\n",
                  currentAirMetrics.co2_eq,
                  currentAirMetrics.voc_idx,
                  currentAirMetrics.iaq_index);
    #endif
}
