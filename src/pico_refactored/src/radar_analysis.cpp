/*
 * ApiaryGuard - Radar Analysis Implementation
 * LD2410B data processing and anomaly detection
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_RADAR in config.h
 * LOGGING: All radar operations are logged with metrics
 * EXCEPTIONS: Graceful handling of invalid data and communication errors
 */

#include "radar_analysis.h"
#include <HardwareSerial.h>
#include <math.h>
#include <Arduino.h>

// Debug counters for radar monitoring
static unsigned long radar_calc_count = 0;
static unsigned long radar_error_count = 0;
static unsigned long parse_errors = 0;
static unsigned long no_data_cycles = 0;

// Global variables (defined in main)
extern RadarMetrics currentRadarMetrics;
extern HardwareSerial radarSerial;

// Historical data for trend analysis
static float distanceHistory[RADAR_BUFFER_SIZE];
static float energyHistory[RADAR_BUFFER_SIZE];
static int historyIndex = 0;
static bool historyInitialized = false;

// LD2410B packet constants
#define PACKET_HEADER 0xF4F3F2F1
#define PACKET_FOOTER 0xF5F6F7F8

/**
 * Parse LD2410B UART data
 * @return true if valid data received, false otherwise
 * 
 * Implements frame synchronization and validation for LD2410B protocol
 * 
 * DEBUG OUTPUT:
 * - [RADAR] Valid frame received: dist=X.XXm, energy=X.X
 * - [RADAR] WARNING: Invalid frame header
 * - [RADAR] ERROR: Parse error at byte X
 * 
 * EXCEPTIONS HANDLED:
 * - Invalid frame header
 * - Incomplete frames
 * - UART read errors
 */
bool readRadarData() {
    static uint8_t buffer[24];
    static int bufIdx = 0;
    
    while (radarSerial.available()) {
        uint8_t b = radarSerial.read();
        
        // Simple frame synchronization
        if (bufIdx == 0 && b != 0xF4) {
            // Skip bytes until we find header start
            continue;
        }
        
        buffer[bufIdx++] = b;
        
        // Check for complete frame (simplified)
        if (bufIdx >= 24) {
            // Validate header
            if (buffer[0] == 0xF4 && buffer[1] == 0xF3 && 
                buffer[2] == 0xF2 && buffer[3] == 0xF1) {
                
                // Extract distance (bytes 8-9, in cm)
                uint16_t dist = buffer[8] | (buffer[9] << 8);
                
                // Extract energy (bytes 12-13)
                uint16_t energy = buffer[12] | (buffer[13] << 8);
                
                // Validate extracted values
                if (dist > 5000) { // Max 50 meters
                    #ifdef DEBUG_RADAR
                    Serial.printf("[RADAR] WARNING: Distance out of range: %d cm\n", dist);
                    #endif
                    parse_errors++;
                    bufIdx = 0;
                    return false;
                }
                
                // Store in metrics
                currentRadarMetrics.distance = (float)dist / 100.0f; // Convert to meters
                currentRadarMetrics.energy = (float)energy / 100.0f;
                
                // Calculate speed from Doppler (simplified)
                currentRadarMetrics.speed = (energy > 50) ? 0.5f : 0.0f;
                
                bufIdx = 0;
                
                #ifdef DEBUG_RADAR
                Serial.printf("[RADAR] Valid frame: dist=%.2fm, energy=%.1f\n", 
                             currentRadarMetrics.distance, currentRadarMetrics.energy);
                #endif
                
                return true;
            }
            
            // Invalid frame - log error and reset
            #ifdef DEBUG_RADAR
            Serial.printf("[RADAR] WARNING: Invalid frame header (got: %02X %02X %02X %02X)\n",
                         buffer[0], buffer[1], buffer[2], buffer[3]);
            #endif
            parse_errors++;
            
            // Reset on invalid frame
            bufIdx = 0;
        }
    }
    
    return false;
}

/**
 * Update history buffer
 * @param distance Current distance reading in meters
 * @param energy Current energy reading
 * 
 * Implements circular buffer for efficient storage of radar readings
 * 
 * DEBUG OUTPUT:
 * - [RADAR] History updated: index=X, initialized=Y
 */
static void updateHistory(float distance, float energy) {
    distanceHistory[historyIndex] = distance;
    energyHistory[historyIndex] = energy;
    historyIndex = (historyIndex + 1) % RADAR_BUFFER_SIZE;
    
    if (historyIndex == 0) {
        historyInitialized = true;
    }
    
    #ifdef DEBUG_RADAR
    if (historyIndex % 20 == 0) {
        Serial.printf("[RADAR] History updated: index=%d, initialized=%s\n", 
                     historyIndex, historyInitialized?"yes":"no");
    }
    #endif
}

/**
 * Calculate mean of recent samples
 * @param buffer Array of values
 * @param size Total size of buffer
 * @param window Number of samples to consider
 * @return float Mean value, 0.0f if no valid data
 * 
 * DEBUG OUTPUT:
 * - [RADAR] Mean calculated: X.XX (window=Y)
 * - [RADAR] WARNING: No valid data for mean calculation
 */
static float calculateMean(float* buffer, int size, int window) {
    float sum = 0.0f;
    int count = 0;
    int start = (window > historyIndex) ? 0 : historyIndex - window;
    
    for (int i = start; i < historyIndex && count < window; i++) {
        if (buffer[i] >= 0) {
            sum += buffer[i];
            count++;
        }
    }
    
    if (count == 0) {
        #ifdef DEBUG_RADAR
        Serial.println("[RADAR] WARNING: No valid data for mean calculation");
        #endif
        radar_error_count++;
        return 0.0f;
    }
    
    return (count > 0) ? sum / count : 0.0f;
}

/**
 * Calculate activity ratio (motion detection)
 * @param window Number of samples to consider
 * @return float Ratio of motion detections (0.0-1.0)
 * 
 * DEBUG OUTPUT:
 * - [RADAR] Activity ratio: X.X%
 */
static float calculateActivityRatio(int window) {
    int motionCount = 0;
    int total = 0;
    int start = (window > historyIndex) ? 0 : historyIndex - window;
    
    for (int i = start; i < historyIndex && total < window; i++) {
        if (energyHistory[i] > 30.0f) {
            motionCount++;
        }
        total++;
    }
    
    float ratio = (total > 0) ? (float)motionCount / total : 0.0f;
    
    #ifdef DEBUG_RADAR
    if (radar_calc_count % 10 == 0) {
        Serial.printf("[RADAR] Activity ratio: %.1f%% (%d/%d)\n", ratio*100, motionCount, total);
    }
    #endif
    
    return ratio;
}

/**
 * Calculate radar metrics from buffered data
 * @param metrics Reference to RadarMetrics struct to populate
 * 
 * Computes comprehensive radar statistics:
 * - Distance and energy statistics
 * - Motion detection (activity ratio)
 * - Trend analysis
 * - Signal quality assessment
 * - Anomaly detection
 * - Hive health index
 * 
 * DEBUG OUTPUT:
 * - [RADAR] Metrics calculated: dist=X.XXm, activity=X.X%
 * - [RADAR] ALERT: High anomaly score detected
 * - [RADAR] WARNING: Low signal quality
 * 
 * EXCEPTIONS HANDLED:
 * - Empty history buffer
 * - Invalid metric calculations (NaN/Inf)
 * - Out-of-range values
 */
void calculateRadarMetrics(RadarMetrics& metrics) {
    radar_calc_count++;
    
    // Update history
    updateHistory(metrics.distance, metrics.energy);
    
    // Calculate statistics
    metrics.mean_distance = calculateMean(distanceHistory, RADAR_BUFFER_SIZE, RADAR_TREND_WINDOW);
    metrics.activity_ratio = calculateActivityRatio(RADAR_TREND_WINDOW);
    
    // Motion intensity (0-1 scale)
    metrics.motion_intensity = constrain(metrics.energy / 100.0f, 0.0f, 1.0f);
    
    // Validate motion intensity
    if (isnan(metrics.motion_intensity)) {
        #ifdef DEBUG_RADAR
        Serial.println("[RADAR] WARNING: NaN in motion intensity");
        #endif
        metrics.motion_intensity = 0.0f;
        radar_error_count++;
    }
    
    // Trend slope (simple difference)
    if (historyInitialized && historyIndex > 1) {
        int prevIdx = (historyIndex - 2 + RADAR_BUFFER_SIZE) % RADAR_BUFFER_SIZE;
        metrics.trend_slope = distanceHistory[historyIndex-1] - distanceHistory[prevIdx];
    } else {
        metrics.trend_slope = 0.0f;
    }
    
    // Signal quality (based on consistency)
    float variance = 0.0f;
    int count = 0;
    for (int i = 0; i < RADAR_BUFFER_SIZE && count < RADAR_TREND_WINDOW; i++) {
        if (distanceHistory[i] >= 0) {
            float diff = distanceHistory[i] - metrics.mean_distance;
            variance += diff * diff;
            count++;
        }
    }
    
    float stdDev = (count > 0) ? sqrt(variance / count) : 0.0f;
    metrics.signal_quality = constrain(100.0f - stdDev * 10.0f, 0.0f, 100.0f);
    
    // Validate signal quality
    if (isnan(metrics.signal_quality)) {
        #ifdef DEBUG_RADAR
        Serial.println("[RADAR] WARNING: NaN in signal quality");
        #endif
        metrics.signal_quality = 50.0f; // Default to moderate
        radar_error_count++;
    }
    
    // Warn about low signal quality
    if (metrics.signal_quality < 50.0f) {
        #ifdef DEBUG_RADAR
        Serial.printf("[RADAR] WARNING: Low signal quality (%.1f%%)\n", metrics.signal_quality);
        #endif
    }
    
    // Anomaly score
    metrics.anomaly_score = 0.0f;
    if (metrics.activity_ratio > 0.8f) {
        metrics.anomaly_score += 0.3f;
        #ifdef DEBUG_RADAR
        Serial.println("[RADAR] ALERT: High activity ratio detected");
        #endif
    }
    if (abs(metrics.trend_slope) > 0.5f) {
        metrics.anomaly_score += 0.3f;
        #ifdef DEBUG_RADAR
        Serial.printf("[RADAR] ALERT: Significant trend change (%.2f)\n", metrics.trend_slope);
        #endif
    }
    if (metrics.signal_quality < 50.0f) {
        metrics.anomaly_score += 0.4f;
    }
    metrics.anomaly_score = constrain(metrics.anomaly_score, 0.0f, 1.0f);
    
    // Log elevated anomaly scores
    if (metrics.anomaly_score > 0.5f) {
        #ifdef DEBUG_RADAR
        Serial.printf("[RADAR] ALERT: Anomaly score elevated (%.2f)\n", metrics.anomaly_score);
        #endif
        radar_error_count++;
    }
    
    // Hive health index
    float activityScore = metrics.activity_ratio * 100.0f;
    float stabilityScore = metrics.signal_quality;
    metrics.hive_health_index = (activityScore + stabilityScore) / 2.0f;
    
    // Validate health index
    if (isnan(metrics.hive_health_index)) {
        #ifdef DEBUG_RADAR
        Serial.println("[RADAR] WARNING: NaN in hive health index");
        #endif
        metrics.hive_health_index = 50.0f;
        radar_error_count++;
    }
    
    #ifdef DEBUG_RADAR
    if (radar_calc_count % 10 == 0) {
        Serial.printf("[RADAR] Stats - Calc: %lu, Errors: %lu, Parse errors: %lu\n",
                     radar_calc_count, radar_error_count, parse_errors);
    }
    #endif
}

/**
 * Process radar data periodically
 */
void processRadarPeriodically(unsigned long now) {
    static unsigned long lastProcess = 0;
    
    // Process every 2 seconds (radar update rate)
    if (now - lastProcess < 2000) return;
    lastProcess = now;
    
    // Try to read new data from radar
    bool newData = readRadarData();
    
    if (newData) {
        // Recalculate metrics with new data
        calculateRadarMetrics(currentRadarMetrics);
        
        #ifdef DEBUG_RADAR
        Serial.printf("[Radar] Dist: %.2fm, Energy: %.1f, Activity: %.1f%%\n",
                      currentRadarMetrics.distance,
                      currentRadarMetrics.energy,
                      currentRadarMetrics.activity_ratio * 100.0f);
        #endif
    } else {
        // No new data - could indicate radar disconnected
        static int noDataCount = 0;
        noDataCount++;
        
        if (noDataCount > 10) {
            #ifdef DEBUG_RADAR
            Serial.println("[Radar] No data received - check connection");
            #endif
            noDataCount = 0;
        }
    }
}
