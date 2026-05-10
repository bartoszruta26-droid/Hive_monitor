/*
 * ApiaryGuard - Audio Analysis Module Header
 * FFT, metrics calculation, and event classification
 */

#ifndef AUDIO_ANALYSIS_H
#define AUDIO_ANALYSIS_H

#include <Arduino.h>
#include "config.h"

// Audio metrics structure (simplified from original 100+ params)
struct AudioMetrics {
    // Basic time-domain
    float rms_amplitude;
    float peak_amplitude;
    float zero_crossing_rate;
    
    // Frequency-domain
    float dominant_frequency;
    float spectral_centroid;
    float power_in_bee_band;
    float power_in_swarm_band;
    
    // Classification indices
    float bee_activity_index;
    float swarm_probability;
    float hive_health_audio;
};

// Audio event types
enum class AudioEventType {
    NONE = 0,
    NORMAL_ACTIVITY,
    INCREASED_ACTIVITY,
    SWARM_PREPARATION,
    SWARM_ACTIVE,
    QUEEN_PIPING,
    PREDATOR_THREAT,
    DISTRESS,
    LOW_ACTIVITY
};

// Audio event structure
struct AudioEvent {
    AudioEventType type;
    float confidence;
    uint32_t timestamp;
    char description[64];
};

// External variables
extern AudioMetrics currentAudioMetrics;
extern int16_t audioBuffer[AUDIO_BUFFER_SIZE];
extern float audioFFT[AUDIO_BUFFER_SIZE];

// Function declarations
void processAudioSignal();
void processAudioPeriodically(unsigned long now);
void calculateAudioMetrics(AudioMetrics& metrics);
void performFFT(int16_t* input, float* output, int size);
void classifyAudioEvent(AudioEvent& event, const AudioMetrics& metrics);

#endif // AUDIO_ANALYSIS_H
