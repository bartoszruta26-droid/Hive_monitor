/*
 * ApiaryGuard - Main Firmware File for Raspberry Pi Pico (RP2040)
 * Refactored version with modular architecture
 * 
 * This is the main entry point that includes all modules
 */

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>
#include <RP2040Watchdog.h>

// Include module headers
#include "config.h"
#include "sensors.h"
#include "audio_analysis.h"
#include "weight_analysis.h"
#include "air_quality.h"
#include "radar_analysis.h"
#include "network.h"
#include "effectors.h"

// Global objects
EthernetServer server(8080);
EthernetUDP udp;
DHT dht(DHT_PIN, DHT22);
Adafruit_SGP41 sgp;
HardwareSerial radarSerial(uart1);

// Global sensor state
SensorState sensors;
AudioMetrics currentAudioMetrics;
HX711Metrics currentHX711Metrics;
AirQualityMetrics currentAirMetrics;
RadarMetrics currentRadarMetrics;

// Timing variables
unsigned long lastMillis = 0;

void setup() {
    Serial.begin(115200);
    while(!Serial && (millis() < 3000));
    
    Serial.println("=== ApiaryGuard Pico v3.0 (Refactored) ===");
    
    // Enable Watchdog (8 second timeout)
    rp2040.wdtEnable(8000);
    Serial.println(">> Watchdog enabled (timeout: 8s)");
    
    // Initialize subsystems
    initI2C();
    initUART();
    initPWM();
    initGPIO();
    
    // Detect and initialize sensors
    Serial.println(">> Scanning and detecting sensors...");
    detectAllSensors();
    printSensorStatus();
    initSensors();
    
    // Initialize network
    initW6100();
    
    Serial.println(">> System ready.");
}

void loop() {
    unsigned long now = millis();
    
    // Feed watchdog
    rp2040.wdtReset();
    
    // Maintain Ethernet connection
    Ethernet.maintain();
    
    // Read sensors conditionally (only if detected)
    readSensors(now);
    
    // Process audio analysis periodically
    processAudioPeriodically(now);
    
    // Process weight analysis
    processWeightPeriodically(now);
    
    // Process air quality
    processAirQualityPeriodically(now);
    
    // Process radar data
    processRadarPeriodically(now);
    
    // Handle HTTP clients
    handleClients();
    
    // Send UDP data to RPi
    sendUDPData();
}
