/*
 * ApiaryGuard - Configuration Header
 * Pin definitions, network settings, and global constants
 * 
 * DEBUG LEVELS:
 * - DEBUG_SENSORS: Enable sensor debugging
 * - DEBUG_NETWORK: Enable network debugging
 * - DEBUG_AUDIO: Enable audio analysis debugging
 * - DEBUG_HX711: Enable weight sensor debugging
 * - DEBUG_AIR: Enable air quality debugging
 * - DEBUG_RADAR: Enable radar debugging
 * - DEBUG_EFFECTORS: Enable effector control debugging
 * - DEBUG_ALL: Enable all debugging (use sparingly)
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

// Uncomment to enable specific debug modules
// #define DEBUG_SENSORS
// #define DEBUG_NETWORK
// #define DEBUG_AUDIO
// #define DEBUG_HX711
// #define DEBUG_AIR
// #define DEBUG_RADAR
// #define DEBUG_EFFECTORS
// #define DEBUG_ALL

// Master debug switch - enables all debug output when defined
#ifdef DEBUG_ALL
#define DEBUG_SENSORS
#define DEBUG_NETWORK
#define DEBUG_AUDIO
#define DEBUG_HX711
#define DEBUG_AIR
#define DEBUG_RADAR
#define DEBUG_EFFECTORS
#endif

// Debug level macros for conditional compilation
#ifdef DEBUG_SENSORS
#define DBG_SENSOR(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_SENSOR(...)
#endif

#ifdef DEBUG_NETWORK
#define DBG_NET(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_NET(...)
#endif

#ifdef DEBUG_AUDIO
#define DBG_AUDIO(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_AUDIO(...)
#endif

#ifdef DEBUG_HX711
#define DBG_HX711(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_HX711(...)
#endif

#ifdef DEBUG_AIR
#define DBG_AIR(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_AIR(...)
#endif

#ifdef DEBUG_RADAR
#define DBG_RADAR(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_RADAR(...)
#endif

#ifdef DEBUG_EFFECTORS
#define DBG_EFF(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_EFF(...)
#endif

// Error logging macro - always enabled for critical errors
#define LOG_ERROR(module, msg) do { \
    Serial.print("[ERROR] "); \
    Serial.print(module); \
    Serial.print(": "); \
    Serial.println(msg); \
} while(0)

// Warning logging macro - always enabled
#define LOG_WARN(module, msg) do { \
    Serial.print("[WARN] "); \
    Serial.print(module); \
    Serial.print(": "); \
    Serial.println(msg); \
} while(0)

// ============================================================================
// PIN DEFINITIONS (RP2040 PICO)
// ============================================================================

// W6100 Ethernet (SPI1)
#define W6100_CS      9
#define W6100_RST     8
#define W6100_INT     13

// Radar LD2410B (UART1)
#define RADAR_RX      4  // UART1 RX
#define RADAR_TX      5  // UART1 TX

// I2C Sensors (SGP41/BME688) (I2C0)
#define I2C_SDA       0
#define I2C_SCL       1

// DHT22 Temperature/Humidity
#define DHT_PIN       2

// HX711 Weight Sensor
#define HX711_DT      3
#define HX711_SCK     22

// Audio Sensors (ADC)
#define MIC_PIN       26  // ADC0
#define PIEZO_PIN     27  // ADC1

// Effectors (PWM/GPIO)
#define HEATER_PWM    6
#define FAN_PWM       7
#define PUMP_RELAY    14
#define VALVE_1       15
#define VALVE_2       16

// 8-Channel Relay
#define RELAY_CH1     17
#define RELAY_CH2     18
#define RELAY_CH3     19
#define RELAY_CH4     20
#define RELAY_CH5     21
#define RELAY_CH6     28  // Fixed: Changed from GPIO 26 (conflict with MIC_PIN)
#define RELAY_CH7     23
#define RELAY_CH8     24

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

#define MAC_ADDRESS   { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }
#define STATIC_IP     192, 168, 1, 177
#define GATEWAY       192, 168, 1, 1
#define SUBNET        255, 255, 255, 0
#define RPI_IP        192, 168, 1, 100
#define HTTP_PORT     8080
#define UDP_PORT      5005

// Network timeout configuration
#define NETWORK_TIMEOUT_MS        5000
#define NETWORK_RETRY_COUNT       3
#define NETWORK_RETRY_DELAY_MS    1000

// ============================================================================
// AUDIO ANALYSIS CONSTANTS
// ============================================================================

#define AUDIO_BUFFER_SIZE     256
#define AUDIO_SAMPLE_RATE     4000
#define BEE_FREQ_MIN          80
#define BEE_FREQ_MAX          800
#define SWARM_FREQ_MIN        150
#define SWARM_FREQ_MAX        350
#define QUEEN_PIP_FREQ_MIN    200
#define QUEEN_PIP_FREQ_MAX    500
#define AUDIO_HISTORY_SIZE    60

// Audio validation thresholds
#define AUDIO_RMS_MIN         0.0f
#define AUDIO_RMS_MAX         1.0f
#define AUDIO_ZCR_MAX         0.5f

// ============================================================================
// WEIGHT ANALYSIS CONSTANTS (HX711)
// ============================================================================

#define HX711_BUFFER_SIZE           288
#define HX711_TREND_WINDOW          12
#define HX711_DAILY_WINDOW          48
#define HX711_SHORT_WINDOW          6
#define HX711_WEIGHT_CHANGE_THRESH  0.05f
#define HX711_NECTAR_FLOW_MIN       0.02f
#define HX711_CONSUMPTION_MIN       0.01f
#define HX711_TIMEOUT_MS            50

// Weight validation limits
#define HX711_MIN_VALID_VALUE       -1000000L
#define HX711_MAX_VALID_VALUE       1000000L

// ============================================================================
// AIR QUALITY CONSTANTS (SGP41)
// ============================================================================

#define AIRQUAL_BUFFER_SIZE         144
#define CO2_WARNING_LEVEL           1000
#define CO2_ALERT_LEVEL             1500
#define VOC_ALERT_LEVEL             250

// Air quality validation
#define CO2_MIN_VALID               400
#define CO2_MAX_VALID               5000
#define VOC_MIN_VALID               0
#define VOC_MAX_VALID               500

// ============================================================================
// RADAR CONSTANTS (LD2410B)
// ============================================================================

#define RADAR_BUFFER_SIZE           120
#define RADAR_TREND_WINDOW          20
#define RADAR_ANOMALY_THRESHOLD     2.0f

// Radar validation
#define RADAR_MIN_DISTANCE          0.0f
#define RADAR_MAX_DISTANCE          500.0f  // cm
#define RADAR_MIN_ENERGY            0.0f
#define RADAR_MAX_ENERGY            100.0f

// ============================================================================
// SYSTEM CONSTANTS
// ============================================================================

#define WATCHDOG_TIMEOUT_MS         8000
#define SENSOR_DETECT_TIMEOUT_MS    100
#define DHT_READ_RETRY              3

// System health monitoring
#define MAX_SENSOR_ERRORS           10
#define ERROR_RESET_WINDOW_MS       60000  // Reset error count after 1 minute
#define CRITICAL_ERROR_THRESHOLD    50     // Trigger safe mode after this many errors

// Safe mode configuration
#define SAFE_MODE_HEATER_DUTY       0
#define SAFE_MODE_FAN_DUTY          128
#define SAFE_MODE_PUMP_STATE        false

#endif // CONFIG_H
