/*
 * ApiaryGuard - Radar Analysis Implementation
 * LD2410B data processing and anomaly detection
 */

#include "radar_analysis.h"
#include <HardwareSerial.h>
#include <math.h>

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
 * Returns true if valid data received
 */
bool readRadarData() {
    static uint8_t buffer[24];
    static int bufIdx = 0;
    
    while (radarSerial.available()) {
        uint8_t b = radarSerial.read();
        
        // Simple frame synchronization
        if (bufIdx == 0 && b != 0xF4) {
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
                
                // Store in metrics
                currentRadarMetrics.distance = (float)dist / 100.0f; // Convert to meters
                currentRadarMetrics.energy = (float)energy / 100.0f;
                
                // Calculate speed from Doppler (simplified)
                currentRadarMetrics.speed = (energy > 50) ? 0.5f : 0.0f;
                
                bufIdx = 0;
                return true;
            }
            
            // Reset on invalid frame
            bufIdx = 0;
        }
    }
    
    return false;
}

/**
 * Update history buffer
 */
static void updateHistory(float distance, float energy) {
    distanceHistory[historyIndex] = distance;
    energyHistory[historyIndex] = energy;
    historyIndex = (historyIndex + 1) % RADAR_BUFFER_SIZE;
    
    if (historyIndex == 0) {
        historyInitialized = true;
    }
}

/**
 * Calculate mean of recent samples
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
    
    return (count > 0) ? sum / count : 0.0f;
}

/**
 * Calculate activity ratio (motion detection)
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
    
    return (total > 0) ? (float)motionCount / total : 0.0f;
}

/**
 * Calculate radar metrics
 */
void calculateRadarMetrics(RadarMetrics& metrics) {
    // Update history
    updateHistory(metrics.distance, metrics.energy);
    
    // Calculate statistics
    metrics.mean_distance = calculateMean(distanceHistory, RADAR_BUFFER_SIZE, RADAR_TREND_WINDOW);
    metrics.activity_ratio = calculateActivityRatio(RADAR_TREND_WINDOW);
    
    // Motion intensity (0-1 scale)
    metrics.motion_intensity = constrain(metrics.energy / 100.0f, 0.0f, 1.0f);
    
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
    
    // Anomaly score
    metrics.anomaly_score = 0.0f;
    if (metrics.activity_ratio > 0.8f) {
        metrics.anomaly_score += 0.3f;
    }
    if (abs(metrics.trend_slope) > 0.5f) {
        metrics.anomaly_score += 0.3f;
    }
    if (metrics.signal_quality < 50.0f) {
        metrics.anomaly_score += 0.4f;
    }
    metrics.anomaly_score = constrain(metrics.anomaly_score, 0.0f, 1.0f);
    
    // Hive health index
    float activityScore = metrics.activity_ratio * 100.0f;
    float stabilityScore = metrics.signal_quality;
    metrics.hive_health_index = (activityScore + stabilityScore) / 2.0f;
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
