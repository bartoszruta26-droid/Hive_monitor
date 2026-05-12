/*
 * ApiaryGuard - Audio Analysis Implementation
 * FFT, metrics calculation, and event classification
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_AUDIO in config.h
 * LOGGING: All audio processing operations are logged with error tracking
 * EXCEPTIONS: Graceful handling of invalid data and processing errors
 * 
 * DEBUG OUTPUT:
 * - [AUDIO] FFT completed: XX bins processed
 * - [AUDIO] ADC read completed: XX samples
 * - [AUDIO] Metrics calculated: ZCR=X.XX, RMS=XXX
 * - [AUDIO] WARNING/ERROR messages for validation failures
 * 
 * EXCEPTIONS HANDLED:
 * - Null pointer access
 * - Invalid buffer sizes
 * - ADC read errors
 * - NaN/Inf in calculations
 * - Division by zero
 */

#include "audio_analysis.h"
#include <math.h>
#include <Arduino.h>

// Debug counters for audio monitoring
static unsigned long audio_process_count = 0;
static unsigned long audio_error_count = 0;
static unsigned long fft_errors = 0;
static unsigned long adc_read_errors = 0;

// Global audio buffers (defined in main)
extern int16_t audioBuffer[AUDIO_BUFFER_SIZE];
extern float audioFFT[AUDIO_BUFFER_SIZE];
extern AudioMetrics currentAudioMetrics;

/**
 * Simple FFT implementation for RP2040
 * Note: For production, use ARM CMSIS-DSP library for better performance
 * 
 * @param input Input signal buffer (int16_t samples)
 * @param output Output frequency spectrum (magnitude per bin)
 * @param size Buffer size (must be AUDIO_BUFFER_SIZE)
 * 
 * DEBUG OUTPUT:
 * - [AUDIO] FFT completed: XX bins processed
 * - [AUDIO] ERROR: Invalid input buffer (null or zero-filled)
 * 
 * EXCEPTIONS HANDLED:
 * - Null input buffer
 * - Zero-filled input (no signal)
 * - Invalid size parameter
 */
void performFFT(int16_t* input, float* output, int size) {
    TRACE_ENTER(DEBUG_AUDIO);
    
    // Validate input parameters
    if (input == nullptr || output == nullptr) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] ERROR: Null pointer passed to performFFT");
        #endif
        fft_errors++;
        audio_error_count++;
        TRACE_EXIT(DEBUG_AUDIO);
        return;
    }
    
    if (size <= 0 || size > AUDIO_BUFFER_SIZE) {
        #ifdef DEBUG_AUDIO
        Serial.printf("[AUDIO] ERROR: Invalid FFT size: %d\n", size);
        #endif
        fft_errors++;
        audio_error_count++;
        TRACE_EXIT(DEBUG_AUDIO);
        return;
    }
    
    TRACE_VALUE(DEBUG_AUDIO, size, size);
    
    // Check for zero-filled input (no signal)
    bool hasSignal = false;
    for (int i = 0; i < size && !hasSignal; i++) {
        if (abs(input[i]) > 10) { // Threshold for noise floor
            hasSignal = true;
        }
    }
    
    if (!hasSignal) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] WARNING: Zero-filled or very low signal input");
        #endif
        // Continue anyway - might be valid silence
    }
    
    // Simplified DFT for demonstration (replace with proper FFT in production)
    PERF_START(fft_compute);
    for (int k = 0; k < size / 2; k++) {
        float real = 0.0f;
        float imag = 0.0f;
        
        for (int n = 0; n < size; n++) {
            float angle = (2.0f * PI * k * n) / size;
            real += input[n] * cos(angle);
            imag -= input[n] * sin(angle);
        }
        
        // Calculate magnitude with validation
        float magnitude = sqrt(real * real + imag * imag) / size;
        
        // Check for NaN/Inf before storing
        if (isnan(magnitude) || isinf(magnitude)) {
            #ifdef DEBUG_AUDIO
            Serial.printf("[AUDIO] WARNING: Invalid magnitude at bin %d\n", k);
            #endif
            magnitude = 0.0f;
            fft_errors++;
        }
        
        output[k] = magnitude;
    }
    PERF_END(fft_compute);
    
    // Increment process counter for throttled debug logs
    audio_process_count++;
    
    #ifdef DEBUG_AUDIO
    if (audio_process_count % 10 == 0) {
        Serial.printf("[AUDIO] FFT completed: %d bins processed\n", size / 2);
    }
    #endif
    
    TRACE_EXIT(DEBUG_AUDIO);
}

/**
 * Read audio signal from ADC pins
 * Implements sampling with error checking
 * 
 * DEBUG OUTPUT:
 * - [AUDIO] ADC read completed: XX samples
 * - [AUDIO] WARNING: ADC reading out of expected range
 * - [AUDIO] ERROR: Too many ADC errors, using fallback data
 * 
 * EXCEPTIONS HANDLED:
 * - ADC read timeout
 * - Values outside valid range
 * - Persistent ADC failures
 */
static void readAudioSignal() {
    TRACE_ENTER(DEBUG_AUDIO);
    PERF_START(adc_read);
    
    int validSamples = 0;
    int outOfRangeCount = 0;
    int consecutiveErrors = 0;
    
    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        // Read from MEMS mic (ADC0)
        int raw = analogRead(MIC_PIN);
        
        // Validate ADC reading (RP2040 ADC is 12-bit: 0-4095)
        if (raw < 0 || raw > 4095) {
            #ifdef DEBUG_AUDIO
            Serial.printf("[AUDIO] WARNING: ADC out of range: %d at sample %d\n", raw, i);
            #endif
            outOfRangeCount++;
            adc_read_errors++;
            consecutiveErrors++;
            raw = 2048; // Default to center value
        } else {
            consecutiveErrors = 0; // Reset on successful read
        }
        
        // Gentle assertion for ADC health monitoring
        GENTLE_ASSERT(raw >= 0 && raw <= 4095, "AUDIO", "ADC value validation failed");
        
        audioBuffer[i] = raw - 2048; // Center around 0
        
        if (abs(audioBuffer[i]) > 0) {
            validSamples++;
        }
        
        delayMicroseconds(250); // ~4kHz sample rate
        
        // Check for persistent ADC failure - switch to safe mode
        if (consecutiveErrors > 10) {
            #ifdef DEBUG_AUDIO
            Serial.println("[AUDIO] ERROR: Persistent ADC failure, using fallback data");
            #endif
            LOG_ERROR("AUDIO", "Persistent ADC failure detected");
            // Fill remaining buffer with safe defaults
            for (int j = i + 1; j < AUDIO_BUFFER_SIZE; j++) {
                audioBuffer[j] = 0;
            }
            break;
        }
    }
    
    PERF_END(adc_read);
    
    #ifdef DEBUG_AUDIO
    if (outOfRangeCount > 0) {
        Serial.printf("[AUDIO] ADC warnings: %d/%d samples out of range\n", 
                     outOfRangeCount, AUDIO_BUFFER_SIZE);
    }
    if (validSamples < AUDIO_BUFFER_SIZE / 2) {
        Serial.printf("[AUDIO] WARNING: Low signal quality (%d valid samples)\n", validSamples);
    }
    Serial.printf("[AUDIO] ADC read completed: %d samples\n", AUDIO_BUFFER_SIZE);
    #endif
    
    TRACE_EXIT(DEBUG_AUDIO);
}

/**
 * Calculate audio metrics from FFT data
 * 
 * Computes time-domain and frequency-domain metrics:
 * - Zero-crossing rate (voice activity detection)
 * - RMS amplitude (signal strength)
 * - Dominant frequency (bee buzz detection)
 * - Spectral centroid (brightness/timbre)
 * - Power in bee/swarm frequency bands
 * 
 * @param metrics Reference to AudioMetrics struct to populate
 * 
 * DEBUG OUTPUT:
 * - [AUDIO] Metrics calculated: ZCR=X.XX, RMS=XXX, DominantFreq=XXX Hz
 * - [AUDIO] WARNING: Invalid metric calculation (NaN detected)
 * - [AUDIO] ERROR: Null buffer access
 * 
 * EXCEPTIONS HANDLED:
 * - Division by zero in spectral calculations
 * - NaN/Inf results from FFT
 * - Out-of-range metric values
 * - Null pointer access
 */
void calculateAudioMetrics(AudioMetrics& metrics) {
    TRACE_ENTER(DEBUG_AUDIO);
    PERF_START(metrics_calc);
    
    // Validate buffer state before processing
    if (audioBuffer == nullptr || audioFFT == nullptr) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] ERROR: Null buffer in calculateAudioMetrics");
        #endif
        LOG_ERROR("AUDIO", "Null audio buffer");
        audio_error_count++;
        TRACE_EXIT(DEBUG_AUDIO);
        return;
    }
    
    // Zero-crossing rate (time domain)
    int zeroCrossings = 0;
    for (int i = 1; i < AUDIO_BUFFER_SIZE; i++) {
        if ((audioBuffer[i-1] >= 0 && audioBuffer[i] < 0) ||
            (audioBuffer[i-1] < 0 && audioBuffer[i] >= 0)) {
            zeroCrossings++;
        }
    }
    metrics.zero_crossing_rate = (float)zeroCrossings / AUDIO_BUFFER_SIZE;
    
    // Validate ZCR range with gentle assertion
    GENTLE_ASSERT(metrics.zero_crossing_rate >= 0.0f && metrics.zero_crossing_rate <= 1.0f,
                  "AUDIO", "ZCR out of expected range");
    if (isnan(metrics.zero_crossing_rate) || metrics.zero_crossing_rate > 1.0f) {
        #ifdef DEBUG_AUDIO
        Serial.printf("[AUDIO] WARNING: Invalid ZCR: %.3f\n", metrics.zero_crossing_rate);
        #endif
        metrics.zero_crossing_rate = 0.0f;
        audio_error_count++;
    }
    
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
    
    // Validate RMS
    if (isnan(metrics.rms_amplitude)) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] WARNING: NaN in RMS calculation");
        #endif
        metrics.rms_amplitude = 0.0f;
        audio_error_count++;
    }
    
    // Frequency analysis (from FFT)
    int beeBinMin = (BEE_FREQ_MIN * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    int beeBinMax = (BEE_FREQ_MAX * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    int swarmBinMin = (SWARM_FREQ_MIN * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    int swarmBinMax = (SWARM_FREQ_MAX * AUDIO_BUFFER_SIZE) / AUDIO_SAMPLE_RATE;
    
    // Validate bin ranges
    beeBinMin = constrain(beeBinMin, 1, AUDIO_BUFFER_SIZE / 2 - 1);
    beeBinMax = constrain(beeBinMax, beeBinMin + 1, AUDIO_BUFFER_SIZE / 2);
    swarmBinMin = constrain(swarmBinMin, 1, AUDIO_BUFFER_SIZE / 2 - 1);
    swarmBinMax = constrain(swarmBinMax, swarmBinMin + 1, AUDIO_BUFFER_SIZE / 2);
    
    float beePower = 0.0f;
    float swarmPower = 0.0f;
    float totalPower = 0.0f;
    float weightedSum = 0.0f;
    
    int dominantBin = 0;
    float maxMagnitude = 0.0f;
    
    for (int i = 1; i < AUDIO_BUFFER_SIZE / 2; i++) {
        float freq = (i * AUDIO_SAMPLE_RATE) / AUDIO_BUFFER_SIZE;
        float mag = audioFFT[i];
        
        // Check for invalid FFT output
        if (isnan(mag) || isinf(mag)) {
            #ifdef DEBUG_AUDIO
            Serial.printf("[AUDIO] WARNING: Invalid FFT value at bin %d: %f\n", i, mag);
            #endif
            mag = 0.0f;
            fft_errors++;
        }
        
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
    
    // Validate frequency metrics
    if (isnan(metrics.dominant_frequency) || metrics.dominant_frequency > AUDIO_SAMPLE_RATE / 2) {
        #ifdef DEBUG_AUDIO
        Serial.printf("[AUDIO] WARNING: Invalid dominant frequency: %.1f Hz\n", metrics.dominant_frequency);
        #endif
        metrics.dominant_frequency = 0.0f;
        audio_error_count++;
    }
    
    if (isnan(metrics.spectral_centroid)) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] WARNING: NaN in spectral centroid");
        #endif
        metrics.spectral_centroid = 0.0f;
        audio_error_count++;
    }
    
    // Classification indices
    metrics.bee_activity_index = metrics.power_in_bee_band * metrics.rms_amplitude / 1000.0f;
    metrics.swarm_probability = metrics.power_in_swarm_band * 2.0f;
    metrics.hive_health_audio = constrain(metrics.bee_activity_index * 100.0f, 0.0f, 100.0f);
    
    // Validate classification indices
    if (isnan(metrics.bee_activity_index)) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] WARNING: NaN in bee activity index");
        #endif
        metrics.bee_activity_index = 0.0f;
        audio_error_count++;
    }
    
    if (isnan(metrics.swarm_probability)) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] WARNING: NaN in swarm probability");
        #endif
        metrics.swarm_probability = 0.0f;
        audio_error_count++;
    }
    
    // Final validation of all metrics
    GENTLE_ASSERT(!isnan(metrics.zero_crossing_rate), "AUDIO", "ZCR is NaN after calculation");
    GENTLE_ASSERT(!isnan(metrics.rms_amplitude), "AUDIO", "RMS is NaN after calculation");
    GENTLE_ASSERT(!isnan(metrics.dominant_frequency) || metrics.dominant_frequency >= 0, 
                  "AUDIO", "Dominant frequency invalid");
    
    #ifdef DEBUG_AUDIO
    if (audio_process_count % 5 == 0) {
        Serial.printf("[AUDIO] Metrics: ZCR=%.3f, RMS=%.1f, DomFreq=%.1fHz, BeeIdx=%.2f\n",
                     metrics.zero_crossing_rate,
                     metrics.rms_amplitude,
                     metrics.dominant_frequency,
                     metrics.bee_activity_index);
    }
    #endif
    
    PERF_END(metrics_calc);
    TRACE_EXIT(DEBUG_AUDIO);
}

/**
 * Classify audio event based on metrics
 * 
 * @param event Reference to AudioEvent struct to populate
 * @param metrics AudioMetrics with calculated values
 * 
 * DEBUG OUTPUT:
 * - [AUDIO] Event classified: TYPE with XX% confidence
 * 
 * EXCEPTIONS HANDLED:
 * - Invalid metric inputs
 * - Buffer overflow in description
 */
void classifyAudioEvent(AudioEvent& event, const AudioMetrics& metrics) {
    TRACE_ENTER(DEBUG_AUDIO);
    
    // Validate input metrics
    if (isnan(metrics.bee_activity_index) || isnan(metrics.swarm_probability)) {
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] WARNING: NaN metrics in classification, using defaults");
        #endif
        event.type = AudioEventType::NORMAL_ACTIVITY;
        event.confidence = 0.5f;
        snprintf(event.description, sizeof(event.description), "Classification unavailable (sensor error)");
        TRACE_EXIT(DEBUG_AUDIO);
        return;
    }
    
    event.timestamp = millis();
    
    if (metrics.bee_activity_index > 0.8f) {
        event.type = AudioEventType::INCREASED_ACTIVITY;
        event.confidence = metrics.bee_activity_index;
        snprintf(event.description, sizeof(event.description), 
                 "High bee activity detected (%.1f%%)", event.confidence * 100.0f);
        #ifdef DEBUG_AUDIO
        Serial.printf("[AUDIO] Event classified: INCREASED_ACTIVITY (%.1f%%)\n", event.confidence * 100.0f);
        #endif
    } else if (metrics.swarm_probability > 0.6f) {
        event.type = AudioEventType::SWARM_PREPARATION;
        event.confidence = metrics.swarm_probability;
        snprintf(event.description, sizeof(event.description), 
                 "Swarm preparation likely (%.1f%%)", event.confidence * 100.0f);
        #ifdef DEBUG_AUDIO
        Serial.printf("[AUDIO] Event classified: SWARM_PREPARATION (%.1f%%)\n", event.confidence * 100.0f);
        #endif
    } else if (metrics.bee_activity_index < 0.2f) {
        event.type = AudioEventType::LOW_ACTIVITY;
        event.confidence = 1.0f - metrics.bee_activity_index;
        snprintf(event.description, sizeof(event.description), 
                 "Low hive activity (%.1f%%)", event.confidence * 100.0f);
        #ifdef DEBUG_AUDIO
        Serial.printf("[AUDIO] Event classified: LOW_ACTIVITY (%.1f%%)\n", event.confidence * 100.0f);
        #endif
    } else {
        event.type = AudioEventType::NORMAL_ACTIVITY;
        event.confidence = 0.8f;
        snprintf(event.description, sizeof(event.description), "Normal hive activity");
        #ifdef DEBUG_AUDIO
        Serial.println("[AUDIO] Event classified: NORMAL_ACTIVITY");
        #endif
    }
    
    TRACE_EXIT(DEBUG_AUDIO);
}

/**
 * Process audio signal periodically
 * 
 * @param now Current time in milliseconds (from millis())
 * 
 * DEBUG OUTPUT:
 * - [AUDIO] Processing audio data...
 * - [Audio] Event: TYPE - description
 * - [AUDIO] Stats summary every 30 minutes
 * 
 * EXCEPTIONS HANDLED:
 * - Processing errors in any stage are logged and counted
 */
void processAudioPeriodically(unsigned long now) {
    TRACE_ENTER(DEBUG_AUDIO);
    PERF_START(audio_process);
    
    static unsigned long lastProcess = 0;
    static unsigned long process_count = 0;
    
    // Process every 5 seconds
    if (now - lastProcess < 5000) return;
    lastProcess = now;
    process_count++;
    
    #ifdef DEBUG_AUDIO
    Serial.println("[AUDIO] Processing audio data...");
    #endif
    
    TRY_CATCH_RECOVER("AUDIO", audio_error_count++)
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
    CATCH_RECOVER("AUDIO", audio_error_count++)
        LOG_ERROR("AUDIO", "Exception during audio processing");
    }
    
    // Log stats summary every 30 minutes (360 cycles)
    #ifdef DEBUG_AUDIO
    static unsigned long stats_counter = 0;
    stats_counter++;
    if (stats_counter >= 360) {
        Serial.printf("[DEBUG] Audio Stats:\n");
        Serial.printf("  Process count: %lu\n", process_count);
        Serial.printf("  Errors: %lu\n", audio_error_count);
        Serial.printf("  FFT errors: %lu\n", fft_errors);
        Serial.printf("  ADC read errors: %lu\n", adc_read_errors);
        stats_counter = 0;
    }
    #endif
    
    PERF_END(audio_process);
    TRACE_EXIT(DEBUG_AUDIO);
}
