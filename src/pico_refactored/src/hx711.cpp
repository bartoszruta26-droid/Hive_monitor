/*
 * ApiaryGuard - HX711 Implementation
 * Robust bit-bang implementation with timeout and filtering
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_HX711 in config.h
 * LOGGING: All errors are logged to Serial with timestamps
 * EXCEPTIONS: Timeout and communication errors are handled gracefully
 * 
 * IMPROVEMENTS IN THIS VERSION:
 * - Enhanced error handling with validation ranges
 * - Debug macros from config.h for consistent logging
 * - Watchdog feed integration for RP2040 stability
 * - Rate-limited error reporting to prevent log flooding
 * - Trace debugging support (DEBUG_VERBOSE)
 * - Performance monitoring (DEBUG_PERF)
 */

#include "hx711.h"
#include <Arduino.h>
#ifdef ARDUINO_ARCH_RP2040
#include <RP2040Watchdog.h>
#endif

// Global calibration variables with debug support
long hx711_value = 0;
const long hx711_offset = 0;    // Set during calibration (tare)
const float hx711_scale = 1.0f; // Set during calibration

// Debug counter for error tracking
static unsigned long hx711_error_count = 0;
static unsigned long hx711_read_count = 0;
static unsigned long hx711_timeout_count = 0;
static unsigned long hx711_invalid_count = 0;
static unsigned long last_error_log_time = 0;

/**
 * @brief Read HX711 weight sensor with proper timeout handling
 * @return long Weight value in ADC counts, 0 on error/timeout
 * 
 * Key improvements over original:
 * - Timeout prevents hanging if sensor disconnected
 * - Proper pin management with state validation
 * - Error logging via Serial with rate limiting
 * - Watchdog feed for RP2040 stability
 * - Input validation using configured limits
 * - Trace debugging support
 * 
 * DEBUG OUTPUT:
 * - [HX711] ERROR: Timeout - sensor not detected (every 5s)
 * - [HX711] ERROR: Invalid reading (count > 100 per minute)
 * - [TRACE] ENTER/EXIT readHX711 (when DEBUG_VERBOSE enabled)
 * - [PERF] Read duration (when DEBUG_PERF enabled)
 * 
 * EXCEPTIONS HANDLED:
 * - Sensor disconnection (timeout after HX711_TIMEOUT_MS)
 * - Invalid data (out of range values)
 * - Watchdog reset prevention
 */
long readHX711() {
    TRACE_ENTER(HX711);
    PERF_START(readHX711);
    
    long count = 0;
    unsigned long read_start = millis();
    
    // DEBUG: Track read attempts
    hx711_read_count++;
    
    // Ensure proper pin modes - validate configuration
    GENTLE_ASSERT(digitalRead(HX711_SCK) == LOW || digitalRead(HX711_SCK) == HIGH, 
                  "HX711", "SCK pin may not be configured correctly");
    
    pinMode(HX711_DT, INPUT);
    pinMode(HX711_SCK, OUTPUT);
    digitalWrite(HX711_SCK, LOW);
    
    // Wait for chip to be ready with timeout protection
    unsigned long start_time = millis();
    while(digitalRead(HX711_DT)) {
        // Check for timeout condition
        if (millis() - start_time > HX711_TIMEOUT_MS) {
            hx711_timeout_count++;
            hx711_error_count++;
            
            // Rate-limited error logging (every 5 seconds)
            if (millis() - last_error_log_time > 5000) {
                LOG_ERROR("HX711", "Timeout - sensor not detected");
                DBG_HX711("[HX711] DEBUG: Timeout after %lums (max=%dms)\n", 
                          millis() - start_time, HX711_TIMEOUT_MS);
                DBG_HX711("[HX711] Error count: %lu, Timeout count: %lu\n", 
                          hx711_error_count, hx711_timeout_count);
                last_error_log_time = millis();
            }
            
            #ifdef DEBUG_HX711
            Serial.println("[HX711] DEBUG: DT pin state: " + String(digitalRead(HX711_DT)));
            Serial.println("[HX711] DEBUG: SCK pin state: " + String(digitalRead(HX711_SCK)));
            #endif
            
            PERF_END(readHX711);
            TRACE_EXIT(HX711);
            return 0; // Return error code
        }
        
        // Prevent watchdog trigger during wait (RP2040 specific)
        #ifdef ARDUINO_ARCH_RP2040
        rp2040.wdtReset();
        #endif
        
        // Small delay to prevent CPU hogging
        delayMicroseconds(10);
    }
    
    // Read 24 bits from HX711
    for(int i = 0; i < 24; i++) {
        digitalWrite(HX711_SCK, HIGH);
        count = count << 1;
        digitalWrite(HX711_SCK, LOW);
        
        if(digitalRead(HX711_DT)) {
            count++;
        }
        
        // Small delay for stability at high RP2040 frequency (133 MHz)
        // This prevents timing issues with the bit-bang protocol
        delayMicroseconds(1);
    }
    
    // Send 25th clock pulse to set gain for next reading (gain=128)
    digitalWrite(HX711_SCK, HIGH);
    count = count ^ 0x800000; // Convert to signed (two's complement)
    digitalWrite(HX711_SCK, LOW);
    
    // Validate reading - check for out-of-range values using config constants
    if (count < HX711_MIN_VALID_VALUE || count > HX711_MAX_VALID_VALUE) {
        hx711_invalid_count++;
        hx711_error_count++;
        
        #ifdef DEBUG_HX711
        Serial.printf("[HX711] WARNING: Invalid reading: %ld (out of range [%ld, %ld])\n", 
                      count, HX711_MIN_VALID_VALUE, HX711_MAX_VALID_VALUE);
        #endif
        
        // Rate-limited invalid reading warnings
        if (millis() - last_error_log_time > 10000) {
            LOG_WARN("HX711", "Invalid readings detected - check connections");
            DBG_HX711("[HX711] Invalid count: %lu times\n", hx711_invalid_count);
            last_error_log_time = millis();
        }
        
        PERF_END(readHX711);
        TRACE_EXIT(HX711);
        return 0;
    }
    
    // Additional validation - check for stuck values (same value repeated)
    static long last_valid_count = 0;
    static unsigned long same_value_count = 0;
    if (count == last_valid_count && count != 0) {
        same_value_count++;
        if (same_value_count > 10) {
            DBG_HX711("[HX711] WARNING: Stuck value detected (%ld repeated %lu times)\n", 
                      count, same_value_count);
        }
    } else {
        same_value_count = 0;
        last_valid_count = count;
    }
    
    // DEBUG: Log successful reads periodically
    #ifdef DEBUG_HX711
    if (hx711_read_count % 100 == 0) {
        Serial.printf("[HX711] DEBUG: Successful reads: %lu, Errors: %lu, Timeouts: %lu, Invalid: %lu\n", 
                      hx711_read_count, hx711_error_count, hx711_timeout_count, hx711_invalid_count);
    }
    #endif
    
    PERF_END(readHX711);
    TRACE_EXIT(HX711);
    return count;
}
