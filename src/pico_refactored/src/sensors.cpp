/*
 * ApiaryGuard - Sensor Implementation
 */

#include "sensors.h"
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>

// External globals (defined in main .ino file)
extern Adafruit_SGP41 sgp;
extern HardwareSerial radarSerial;
extern float temperature;
extern float humidity;
extern uint16_t co2_eq;
extern uint16_t voc_idx;
extern long hx711_value;
// Note: sensors is declared extern in sensors.h, no need to redeclare here

void initI2C() {
    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    Wire.begin();
    Serial.println(">> I2C initialized");
}

void initUART() {
    radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
    Serial.println(">> UART1 initialized for radar");
}

void initPWM() {
    pinMode(HEATER_PWM, OUTPUT);
    pinMode(FAN_PWM, OUTPUT);
    analogWriteFrequency(HEATER_PWM, 1000);
    analogWriteFrequency(FAN_PWM, 1000);
    Serial.println(">> PWM initialized");
}

void initGPIO() {
    // Initialize relay pins
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(VALVE_1, OUTPUT);
    pinMode(VALVE_2, OUTPUT);
    pinMode(RELAY_CH1, OUTPUT);
    pinMode(RELAY_CH2, OUTPUT);
    pinMode(RELAY_CH3, OUTPUT);
    pinMode(RELAY_CH4, OUTPUT);
    pinMode(RELAY_CH5, OUTPUT);
    pinMode(RELAY_CH6, OUTPUT);
    pinMode(RELAY_CH7, OUTPUT);
    pinMode(RELAY_CH8, OUTPUT);
    
    // Set all relays to LOW (off)
    digitalWrite(PUMP_RELAY, LOW);
    digitalWrite(VALVE_1, LOW);
    digitalWrite(VALVE_2, LOW);
    digitalWrite(RELAY_CH1, LOW);
    digitalWrite(RELAY_CH2, LOW);
    digitalWrite(RELAY_CH3, LOW);
    digitalWrite(RELAY_CH4, LOW);
    digitalWrite(RELAY_CH5, LOW);
    digitalWrite(RELAY_CH6, LOW);
    digitalWrite(RELAY_CH7, LOW);
    digitalWrite(RELAY_CH8, LOW);
    
    Serial.println(">> GPIO initialized");
}

bool detectHX711() {
    pinMode(HX711_DT, INPUT);
    pinMode(HX711_SCK, OUTPUT);
    digitalWrite(HX711_SCK, LOW);
    
    unsigned long start = millis();
    while(digitalRead(HX711_DT)) {
        if (millis() - start > HX711_TIMEOUT_MS) {
            Serial.println("  [!] HX711: Timeout - sensor not detected");
            return false;
        }
    }
    
    Serial.println("  [✓] HX711: Detected");
    return true;
}

bool detectDHT() {
    pinMode(DHT_PIN, INPUT_PULLUP);
    dht.begin();
    
    // Try to read with retries
    for (int i = 0; i < DHT_READ_RETRY; i++) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (!isnan(h) && !isnan(t)) {
            Serial.printf("  [✓] DHT: Detected (T=%.1f°C, H=%.1f%%)\n", t, h);
            return true;
        }
        delay(100);
    }
    
    Serial.println("  [!] DHT: Not detected or reading failed");
    return false;
}

bool detectSGP41() {
    Wire.beginTransmission(0x59);
    if (Wire.endTransmission() == 0) {
        Serial.println("  [✓] SGP41: Detected at 0x59");
        return true;
    }
    
    Serial.println("  [!] SGP41: Not detected at 0x59");
    return false;
}

bool detectRadar() {
    // Simple detection - check if UART responds
    radarSerial.flush();
    delay(100);
    
    if (radarSerial.available()) {
        Serial.println("  [✓] LD2410B: Data detected on UART");
        return true;
    }
    
    Serial.println("  [?] LD2410B: No response (may still be connected)");
    return true; // Don't fail - radar might be idle
}

void detectAllSensors() {
    sensors.tempHum.detected = detectDHT();
    sensors.airQual.detected = detectSGP41();
    sensors.hx711.detected = detectHX711();
    sensors.radar.detected = detectRadar();
    
    // Audio sensors are always considered present (ADC pins)
    sensors.audio.detected = true;
    
    // Mark active sensors (successfully initialized)
    sensors.tempHum.active = sensors.tempHum.detected;
    sensors.airQual.active = sensors.airQual.detected;
    sensors.hx711.active = sensors.hx711.detected;
    sensors.radar.active = sensors.radar.detected;
    sensors.audio.active = true;
}

void printSensorStatus() {
    Serial.println("\n>> Sensor Status:");
    Serial.printf("  Temp/Humidity: %s\n", sensors.tempHum.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Air Quality:   %s\n", sensors.airQual.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Weight (HX711):%s\n", sensors.hx711.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Radar:         %s\n", sensors.radar.detected ? "OK" : "NOT FOUND");
    Serial.printf("  Audio:         %s\n", sensors.audio.detected ? "OK" : "NOT FOUND");
}

void initSensors() {
    if (sensors.tempHum.active) {
        dht.begin();
        Serial.println(">> DHT22/SHT40 initialized");
    }
    
    if (sensors.airQual.active) {
        if (sgp.begin_I2C(0x59, &Wire)) {
            sgp.measureBaseline();
            Serial.println(">> SGP41 initialized");
        } else {
            sensors.airQual.active = false;
            Serial.println(">> SGP41 initialization failed");
        }
    }
}

void readSensors(unsigned long now) {
    static unsigned long lastRead = 0;
    
    if (now - lastRead < 2000) return;
    lastRead = now;
    
    // Read temperature/humidity
    if (sensors.tempHum.active) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (!isnan(h)) {
            humidity = h;
            sensors.tempHum.error_count = 0;
        } else {
            sensors.tempHum.error_count++;
        }
        
        if (!isnan(t)) {
            temperature = t;
        }
    }
    
    // Read HX711 weight
    if (sensors.hx711.active) {
        long raw = readHX711();
        hx711_value = (raw - hx711_offset) / hx711_scale;
    }
    
    // Read SGP41 air quality
    if (sensors.airQual.active) {
        sgp.measureGas();
        co2_eq = sgp.CO2eq;
        voc_idx = sgp.VOCindex;
    }
}
