/*
 * ApiaryGuard - Radar Analysis Module Header
 * LD2410B data processing and anomaly detection
 */

#ifndef RADAR_ANALYSIS_H
#define RADAR_ANALYSIS_H

#include <Arduino.h>
#include "config.h"

// Radar metrics (simplified from 27+ params)
struct RadarMetrics {
    // Basic readings
    float distance;
    float energy;
    float speed;
    
    // Statistics
    float mean_distance;
    float activity_ratio;
    
    // Trend analysis
    float trend_slope;
    float motion_intensity;
    
    // Health indices
    float signal_quality;
    float anomaly_score;
    float hive_health_index;
};

// External variables
extern RadarMetrics currentRadarMetrics;

// Function declarations
void processRadarPeriodically(unsigned long now);
void readRadarData();
void calculateRadarMetrics(RadarMetrics& metrics);

#endif // RADAR_ANALYSIS_H
