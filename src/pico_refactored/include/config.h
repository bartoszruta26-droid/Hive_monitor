/*
 * ApiaryGuard - Configuration Header
 * Pin definitions, network settings, and global constants
 */

#ifndef CONFIG_H
#define CONFIG_H

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

// ============================================================================
// AIR QUALITY CONSTANTS (SGP41)
// ============================================================================

#define AIRQUAL_BUFFER_SIZE         144
#define CO2_WARNING_LEVEL           1000
#define CO2_ALERT_LEVEL             1500
#define VOC_ALERT_LEVEL             250

// ============================================================================
// RADAR CONSTANTS (LD2410B)
// ============================================================================

#define RADAR_BUFFER_SIZE           120
#define RADAR_TREND_WINDOW          20
#define RADAR_ANOMALY_THRESHOLD     2.0f

// ============================================================================
// SYSTEM CONSTANTS
// ============================================================================

#define WATCHDOG_TIMEOUT_MS         8000
#define SENSOR_DETECT_TIMEOUT_MS    100
#define DHT_READ_RETRY              3

#endif // CONFIG_H
