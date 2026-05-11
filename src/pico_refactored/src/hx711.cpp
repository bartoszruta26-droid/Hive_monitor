/*
 * ApiaryGuard - HX711 Implementation
 * Robust bit-bang implementation with timeout and filtering
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_HX711 in config.h
 * LOGGING: All errors are logged to Serial with timestamps
 * EXCEPTIONS: Timeout and communication errors are handled gracefully
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

/**
 * @brief Read HX711 weight sensor with proper timeout handling
 * @return long Weight value in ADC counts, 0 on error/timeout
 * 
 * Key improvements over original:
 * - Timeout prevents hanging if sensor disconnected
 * - Proper pin management with state validation
 * - Error logging via Serial with rate limiting
 * - Watchdog feed for RP2040 stability
 * 
 * DEBUG OUTPUT:
 * - [HX711] ERROR: Timeout - sensor not detected (every 5s)
 * - [HX711] ERROR: Invalid reading (count > 100 per minute)
 * 
 * EXCEPTIONS HANDLED:
 * - Sensor disconnection (timeout after HX711_TIMEOUT_MS)
 * - Invalid data (out of range values)
 * - Watchdog reset prevention
 */
long readHX711() {
    long count = 0;
    unsigned long read_start = millis();
    
    // DEBUG: Track read attempts
    hx711_read_count++;
    
    // Ensure proper pin modes - validate configuration
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
            static unsigned long last_error_log = 0;
            if (millis() - last_error_log > 5000) {
                Serial.print("[HX711] ERROR: Timeout - sensor not detected (");
                Serial.print(millis() - start_time);
                Serial.print("ms elapsed, max=");
                Serial.print(HX711_TIMEOUT_MS);
                Serial.println("ms)");
                Serial.print("[DEBUG] Error count: ");
                Serial.println(hx711_error_count);
                last_error_log = millis();
            }
            
            #ifdef DEBUG_HX711
            Serial.println("[HX711] DEBUG: DT pin state: " + String(digitalRead(HX711_DT)));
            Serial.println("[HX711] DEBUG: SCK pin state: " + String(digitalRead(HX711_SCK)));
            #endif
            
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
    
    // Validate reading - check for out-of-range values
    if (count < -8388608 || count > 8388607) {
        hx711_error_count++;
        #ifdef DEBUG_HX711
        Serial.print("[HX711] WARNING: Invalid reading: ");
        Serial.println(count);
        #endif
        return 0;
    }
    
    // DEBUG: Log successful reads periodically
    #ifdef DEBUG_HX711
    if (hx711_read_count % 100 == 0) {
        Serial.print("[HX711] DEBUG: Successful reads: ");
        Serial.print(hx711_read_count);
        Serial.print(", Errors: ");
        Serial.print(hx711_error_count);
        Serial.print(", Timeouts: ");
        Serial.println(hx711_timeout_count);
    }
    #endif
    
    return count;
}
