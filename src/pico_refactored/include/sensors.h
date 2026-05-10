/*
 * ApiaryGuard - Sensor Detection and Management
 * Handles detection, initialization, and reading of all sensors
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include "config.h"

// Sensor detection flags
struct SensorFlags {
    bool detected = false;
    bool active = false;
    uint8_t error_count = 0;
    uint32_t last_read_time = 0;
};

// Overall sensor state
struct SensorState {
    SensorFlags tempHum;    // DHT22/SHT40/BME280
    SensorFlags airQual;    // SGP41/BME688
    SensorFlags hx711;      // Weight sensor
    SensorFlags radar;      // LD2410B
    SensorFlags audio;      // MEMS/Piezo mic
};

// Global sensor state (extern to be defined in main file)
extern SensorState sensors;
extern DHT dht;

// Function declarations
void initI2C();
void initUART();
void initPWM();
void initGPIO();
void detectAllSensors();
void printSensorStatus();
void initSensors();
void readSensors(unsigned long now);

// Individual sensor detection
bool detectHX711();
bool detectDHT();
bool detectSGP41();
bool detectRadar();

#endif // SENSORS_H
