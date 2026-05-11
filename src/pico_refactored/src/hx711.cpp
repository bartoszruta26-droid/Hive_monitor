/*
 * ApiaryGuard - HX711 Implementation
 * Robust bit-bang implementation with timeout and filtering
 */

#include "hx711.h"

// Global calibration variables
long hx711_value = 0;
const long hx711_offset = 0;    // Set during calibration (tare)
const float hx711_scale = 1.0f; // Set during calibration

/**
 * Read HX711 with proper timeout handling
 * Returns 0 on error/timeout
 * 
 * Key improvements over original:
 * - Timeout prevents hanging if sensor disconnected
 * - Proper pin management
 * - Error logging via Serial
 */
long readHX711() {
    long count = 0;
    
    // Ensure proper pin modes
    pinMode(HX711_DT, INPUT);
    pinMode(HX711_SCK, OUTPUT);
    digitalWrite(HX711_SCK, LOW);
    
    // Wait for chip to be ready with timeout
    unsigned long start_time = millis();
    while(digitalRead(HX711_DT)) {
        if (millis() - start_time > HX711_TIMEOUT_MS) {
            static unsigned long last_error_log = 0;
            if (millis() - last_error_log > 5000) {
                Serial.println("[HX711] ERROR: Timeout - sensor not detected");
                last_error_log = millis();
            }
            return 0;
        }
        // Prevent watchdog trigger during wait
        #ifdef ARDUINO_ARCH_RP2040
        watchdog_update();
        #endif
    }
    
    // Read 24 bits
    for(int i = 0; i < 24; i++) {
        digitalWrite(HX711_SCK, HIGH);
        count = count << 1;
        digitalWrite(HX711_SCK, LOW);
        
        if(digitalRead(HX711_DT)) {
            count++;
        }
        
        // Small delay for stability at high RP2040 frequency (133 MHz)
        delayMicroseconds(1);
    }
    
    // Send 25th clock pulse to set gain for next reading (gain=128)
    digitalWrite(HX711_SCK, HIGH);
    count = count ^ 0x800000; // Convert to signed (two's complement)
    digitalWrite(HX711_SCK, LOW);
    
    return count;
}
