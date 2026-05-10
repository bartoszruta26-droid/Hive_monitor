/*
 * ApiaryGuard - Audio Analysis Implementation
 * FFT, metrics calculation, and event classification
 */

#include "audio_analysis.h"
#include <math.h>

// Global audio buffers (defined in main)
extern int16_t audioBuffer[AUDIO_BUFFER_SIZE];
extern float audioFFT[AUDIO_BUFFER_SIZE];
extern AudioMetrics currentAudioMetrics;

/**
 * Simple FFT implementation for RP2040
 * Note: For production, use ARM CMSIS-DSP library for better performance
 */
void performFFT(int16_t* input, float* output, int size) {
    // Simplified DFT for demonstration (replace with proper FFT in production)
    for (int k = 0; k < size / 2; k++) {
        float real = 0.0f;
        float imag = 0.0f;
        
        for (int n = 0; n < size; n++) {
            float angle = (2.0f * PI * k * n) / size;
            real += input[n] * cos(angle);
            imag -= input[n] * sin(angle);
        }
        
        // Calculate magnitude
        output[k] = sqrt(real * real + imag * imag) / size;
    }
}

/**
 * Read audio signal from ADC pins
 */
static void readAudioSignal() {
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        // Read from MEMS mic (ADC0)
        audioBuffer[i] = analogRead(MIC_PIN) - 2048; // Center around 0
        delayMicroseconds(250); // ~4kHz sample rate
    }
}

/**
 * Calculate audio metrics from FFT data
 */
void calculateAudioMetrics(AudioMetrics& metrics) {
    // Zero-crossing rate (time domain)
    int zeroCrossings = 0;
    for (int i = 1; i < AUDIO_BUFFER_SIZE; i++) {
        if ((audioBuffer[i-1] >= 0 && audioBuffer[i] < 0) ||
            (audioBuffer[i-1] < 0 && audioBuffer[i] >= 0)) {
            zeroCrossings++;
        }
    }
    metrics.zero_crossing_rate = (float)zeroCrossings / AUDIO_BUFFER_SIZE;
    
    // RMS amplitude
    float sum = 0.0f;
    metrics.peak_amplitude = 0.0f;
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        float val = abs(audioBuffer[i]);
        sum += val * val;
        if (val > metrics.peak_amplitude) {
            metrics.peak_amplitude = val;
        }
    }
    metrics.rms_amplitude = sqrt(sum / AUDIO_BUFFER_SIZE);
    
    // Frequency analysis (from FFT)
    int beeBinMin = (BEE_FREQ_MIN * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    int beeBinMax = (BEE_FREQ_MAX * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    int swarmBinMin = (SWARM_FREQ_MIN * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    int swarmBinMax = (SWARM_FREQ_MAX * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    
    float beePower = 0.0f;
    float swarmPower = 0.0f;
    float totalPower = 0.0f;
    float weightedSum = 0.0f;
    
    int dominantBin = 0;
    float maxMagnitude = 0.0f;
    
    for (int i = 1; i < AUDIO_BUFFER_SIZE / 2; i++) {
        float freq = (i * AUDIO_SAMPLE_RATE) / AUDIO_BUFFER_SIZE;
        float mag = audioFFT[i];
        
        totalPower += mag * mag;
        weightedSum += freq * mag;
        
        if (mag > maxMagnitude) {
            maxMagnitude = mag;
            dominantBin = i;
        }
        
        if (i >= beeBinMin && i < beeBinMax) {
            beePower += mag * mag;
        }
        if (i >= swarmBinMin && i < swarmBinMax) {
            swarmPower += mag * mag;
        }
    }
    
    metrics.dominant_frequency = (dominantBin * AUDIO_SAMPLE_RATE) / AUDIO_BUFFER_SIZE;
    metrics.spectral_centroid = (totalPower > 0) ? weightedSum / totalPower : 0.0f;
    metrics.power_in_bee_band = (totalPower > 0) ? beePower / totalPower : 0.0f;
    metrics.power_in_swarm_band = (totalPower > 0) ? swarmPower / totalPower : 0.0f;
    
    // Classification indices
    metrics.bee_activity_index = metrics.power_in_bee_band * metrics.rms_amplitude / 1000.0f;
    metrics.swarm_probability = metrics.power_in_swarm_band * 2.0f;
    metrics.hive_health_audio = constrain(metrics.bee_activity_index * 100.0f, 0.0f, 100.0f);
}

/**
 * Classify audio event based on metrics
 */
void classifyAudioEvent(AudioEvent& event, const AudioMetrics& metrics) {
    event.timestamp = millis();
    
    if (metrics.bee_activity_index > 0.8f) {
        event.type = AudioEventType::INCREASED_ACTIVITY;
        event.confidence = metrics.bee_activity_index;
        snprintf(event.description, sizeof(event.description), 
                 "High bee activity detected (%.1f%%)", event.confidence * 100.0f);
    } else if (metrics.swarm_probability > 0.6f) {
        event.type = AudioEventType::SWARM_PREPARATION;
        event.confidence = metrics.swarm_probability;
        snprintf(event.description, sizeof(event.description), 
                 "Swarm preparation likely (%.1f%%)", event.confidence * 100.0f);
    } else if (metrics.bee_activity_index < 0.2f) {
        event.type = AudioEventType::LOW_ACTIVITY;
        event.confidence = 1.0f - metrics.bee_activity_index;
        snprintf(event.description, sizeof(event.description), 
                 "Low hive activity (%.1f%%)", event.confidence * 100.0f);
    } else {
        event.type = AudioEventType::NORMAL_ACTIVITY;
        event.confidence = 0.8f;
        snprintf(event.description, sizeof(event.description), "Normal hive activity");
    }
}

/**
 * Process audio signal periodically
 */
void processAudioPeriodically(unsigned long now) {
    static unsigned long lastProcess = 0;
    
    // Process every 5 seconds
    if (now - lastProcess < 5000) return;
    lastProcess = now;
    
    // Read audio signal
    readAudioSignal();
    
    // Perform FFT
    performFFT(audioBuffer, audioFFT, AUDIO_BUFFER_SIZE);
    
    // Calculate metrics
    calculateAudioMetrics(currentAudioMetrics);
    
    // Classify event (optional logging)
    AudioEvent event;
    classifyAudioEvent(event, currentAudioMetrics);
    
    #ifdef DEBUG_AUDIO
    Serial.printf("[Audio] Event: %s - %s\n", 
                  event.type == AudioEventType::NORMAL_ACTIVITY ? "NORMAL" : "ALERT",
                  event.description);
    #endif
}
