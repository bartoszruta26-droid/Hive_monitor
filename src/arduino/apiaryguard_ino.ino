/*
 * ApiaryGuard - Arduino Nano Firmware
 * 
 * Obsługa sensorów i efektorów dla systemu monitoringu uli
 * Komunikacja przez Ethernet W6100 (SPI) + I2C do Raspberry Pi
 * 
 * Sprzęt:
 * - Arduino Nano V3.0 (ATmega328P)
 * - Wiznet W6100 (Ethernet TCP/IP offload)
 * - HX711 (waga 24-bit ADC)
 * - DHT22 (temperatura i wilgotność)
 * - Mikrofon MEMS (analiza brzmienia)
 * - Czujnik piezoelektryczny (wibracje)
 * - SGP41/BME688 (CO2/VOC gazy)
 * - LD2410B (radar MMWave)
 * - Grzałka 10W (PWM)
 * - Wentylator 12V (PWM)
 * - Pompa perystaltyczna
 * - Zawory elektromagnetyczne
 * - Przekaźniki 8-kanałowe
 * 
 * Autor: ApiaryGuard Team
 * Licencja: MIT
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

// ============================================================================
// KONFIGURACJA PINÓW
// ============================================================================

// W6100 Ethernet - SPI
#define W6100_CS_PIN    10
#define W6100_RST_PIN   9

// HX711 - Waga
#define HX711_DOUT      A0
#define HX711_SCK       A1

// DHT22 - Temperatura i Wilgotność
#define DHT22_PIN       2

// Mikrofon MEMS - ADC
#define MIC_PIN         A2

// Piezo - Wibracje
#define PIEZO_PIN       A3

// SGP41/BME688 - Gazy (I2C)
// SDA: A4, SCL: A5 (domyślne I2C)

// LD2410B - Radar MMWave (UART)
#define RADAR_TX        3
#define RADAR_RX        4

// Efektory - PWM/Digital
#define HEATER_PWM      5
#define FAN_PWM         6
#define PUMP_PIN        7
#define VALVE_PIN       8

// Przekaźniki
#define RELAY_1         A6
#define RELAY_2         A7

// ============================================================================
// KONFIGURACJA SIECIOWA W6100
// ============================================================================

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

// Serwer MQTT/Raspberry Pi
IPAddress serverIP(192, 168, 1, 50);
const uint16_t serverPort = 1883;

// ============================================================================
// STRUKTURY DANYCH
// ============================================================================

struct SensorData {
  uint32_t timestamp;
  float weight_kg;
  float temperature_c;
  float humidity_percent;
  uint16_t audio_rms;
  uint16_t piezo_activity;
  uint16_t co2_ppm;
  uint16_t tvoc_ppb;
  uint8_t radar_motion;
  uint8_t status_flags;
};

struct ActuatorCommand {
  uint8_t heater_pwm;      // 0-255
  uint8_t fan_pwm;         // 0-255
  uint8_t pump_duration;   // sekundy
  uint8_t valve_state;     // 0=OFF, 1=ON
  uint8_t relay_mask;      // bitmaska 8 przekaźników
};

// ============================================================================
// ZMIENNE GLOBALNE
// ============================================================================

EthernetClient ethClient;
SensorData currentData;
ActuatorCommand actuatorCmd;

unsigned long lastSensorRead = 0;
unsigned long lastNetworkCheck = 0;
unsigned long lastHeartbeat = 0;

const unsigned long SENSOR_INTERVAL = 1000;      // 1Hz
const unsigned long NETWORK_INTERVAL = 5000;     // 5s
const unsigned long HEARTBEAT_INTERVAL = 30000;  // 30s

// Kalibracja wagi
float weight_scale = 1.0f;
float weight_offset = 0.0f;

// ============================================================================
// DEKLARACJE FUNKCJI
// ============================================================================

void initW6100();
void initSensors();
void initActuators();
void readAllSensors();
void processNetworkCommands();
void sendSensorData();
void executeActuatorCommands();
float readWeight();
float readTemperature();
float readHumidity();
uint16_t readAudioRMS();
uint16_t readPiezoActivity();
uint16_t readCO2();
uint16_t readTVOC();
uint8_t readRadarMotion();
void setHeaterPWM(uint8_t value);
void setFanPWM(uint8_t value);
void runPump(uint8_t seconds);
void setValve(uint8_t state);
void setRelays(uint8_t mask);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Inicjalizacja Serial dla debugowania
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port
  }
  
  Serial.println(F("=== ApiaryGuard Arduino Nano ==="));
  Serial.println(F("Inicjalizacja systemu..."));
  
  // Reset W6100
  pinMode(W6100_RST_PIN, OUTPUT);
  digitalWrite(W6100_RST_PIN, LOW);
  delay(100);
  digitalWrite(W6100_RST_PIN, HIGH);
  delay(200);
  
  // Inicjalizacja Ethernet W6100
  initW6100();
  
  // Inicjalizacja I2C
  Wire.begin();
  
  // Inicjalizacja sensorów
  initSensors();
  
  // Inicjalizacja efektorów
  initActuators();
  
  // Wczytaj kalibrację z EEPROM
  loadCalibration();
  
  // Inicjalizacja struktury danych
  memset(&currentData, 0, sizeof(SensorData));
  memset(&actuatorCmd, 0, sizeof(ActuatorCommand));
  
  Serial.println(F("System gotowy!"));
  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();
  
  // Obsługa klienta Ethernet
  Ethernet.maintain();
  
  // Czytaj sensory co SENSOR_INTERVAL
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    readAllSensors();
  }
  
  // Sprawdź komendy sieciowe co NETWORK_INTERVAL
  if (now - lastNetworkCheck >= NETWORK_INTERVAL) {
    lastNetworkCheck = now;
    processNetworkCommands();
    sendSensorData();
  }
  
  // Heartbeat co HEARTBEAT_INTERVAL
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;
    sendHeartbeat();
  }
  
  // Wykonaj komendy efektorów
  executeActuatorCommands();
  
  // Watchdog reset (opcjonalnie)
  // wdt_reset();
  
  delay(10);
}

// ============================================================================
// IMPLEMENTACJA FUNKCJI
// ============================================================================

/**
 * Inicjalizacja modułu W6100 Ethernet
 */
void initW6100() {
  Serial.println(F("Inicjalizacja W6100 Ethernet..."));
  
  Ethernet.init(W6100_CS_PIN);
  
  // Konfiguracja statycznego IP
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  
  // Czekaj na DHCP (jeśli używasz DHCP)
  // if (Ethernet.begin(mac) == 0) {
  //   Serial.println(F("Failed to configure Ethernet using DHCP"));
  //   // Fallback do statycznego IP
  //   Ethernet.begin(mac, ip, dns, gateway, subnet);
  // }
  
  delay(1000);
  
  if (Ethernet.linkStatus() == LinkON) {
    Serial.println(F("W6100: Połączenie fizyczne OK"));
  } else {
    Serial.println(F("W6100: Brak połączenia fizycznego"));
  }
  
  Serial.print(F("W6100 IP: "));
  Serial.println(Ethernet.localIP());
}

/**
 * Inicjalizacja sensorów
 */
void initSensors() {
  Serial.println(F("Inicjalizacja sensorów..."));
  
  // HX711
  pinMode(HX711_DOUT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  digitalWrite(HX711_SCK, LOW);
  
  // DHT22
  pinMode(DHT22_PIN, INPUT_PULLUP);
  
  // Audio ADC
  // MIC_PIN - analog input
  
  // Piezo
  pinMode(PIEZO_PIN, INPUT);
  
  // Radar UART
  Serial1.begin(115200);
  
  Serial.println(F("Sensory zainicjalizowane"));
}

/**
 * Inicjalizacja efektorów
 */
void initActuators() {
  Serial.println(F("Inicjalizacja efektorów..."));
  
  // PWM outputs
  pinMode(HEATER_PWM, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);
  
  // Digital outputs
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  
  // Domyślnie wyłączone
  analogWrite(HEATER_PWM, 0);
  analogWrite(FAN_PWM, 0);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  
  Serial.println(F("Efektory zainicjalizowane"));
}

/**
 * Odczyt wszystkich sensorów
 */
void readAllSensors() {
  currentData.timestamp = millis();
  
  // Waga
  currentData.weight_kg = readWeight();
  
  // Temperatura
  currentData.temperature_c = readTemperature();
  
  // Wilgotność
  currentData.humidity_percent = readHumidity();
  
  // Audio RMS
  currentData.audio_rms = readAudioRMS();
  
  // Piezo activity
  currentData.piezo_activity = readPiezoActivity();
  
  // CO2 (SGP41/BME688)
  currentData.co2_ppm = readCO2();
  
  // TVOC
  currentData.tvoc_ppb = readTVOC();
  
  // Radar motion
  currentData.radar_motion = readRadarMotion();
  
  // Status flags
  currentData.status_flags = 0x00;
  if (Ethernet.linkStatus() == LinkON) {
    currentData.status_flags |= 0x01;
  }
  
  // Debug output
  #ifdef DEBUG_SENSORS
  Serial.print(F("Weight: "));
  Serial.print(currentData.weight_kg);
  Serial.print(F(" kg, Temp: "));
  Serial.print(currentData.temperature_c);
  Serial.print(F(" C, Hum: "));
  Serial.print(currentData.humidity_percent);
  Serial.println(F(" %"));
  #endif
}

/**
 * Odczyt wagi z HX711
 */
float readWeight() {
  // Implementacja protokołu HX711
  // 24-bit ADC reading
  
  long sum = 0;
  const int samples = 10;
  
  for (int i = 0; i < samples; i++) {
    while (digitalRead(HX711_DOUT));
    
    long value = 0;
    for (int j = 0; j < 24; j++) {
      digitalWrite(HX711_SCK, HIGH);
      value = value << 1;
      if (digitalRead(HX711_DOUT)) {
        value++;
      }
      digitalWrite(HX711_SCK, LOW);
    }
    
    // Clock pulse for gain selection (24th pulse)
    digitalWrite(HX711_SCK, HIGH);
    digitalWrite(HX711_SCK, LOW);
    
    // Konwersja na signed
    if (value & 0x800000) {
      value |= 0xFF000000;
    }
    
    sum += value;
  }
  
  long average = sum / samples;
  
  // Aplikowanie kalibracji
  float weight = (average - weight_offset) / weight_scale;
  
  return weight / 1000.0f; // Konwersja na kg
}

/**
 * Odczyt temperatury z DHT22
 */
float readTemperature() {
  // Prosta implementacja DHT22
  // W produkcji użyj biblioteki DHT sensor library
  
  uint8_t data[5] = {0};
  uint8_t bit, byte, checkSum;
  
  // Request data
  pinMode(DHT22_PIN, OUTPUT);
  digitalWrite(DHT22_PIN, LOW);
  delayMicroseconds(20);
  digitalWrite(DHT22_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT22_PIN, INPUT_PULLUP);
  
  // Wait for response
  if (digitalRead(DHT22_PIN)) {
    return -999.0f; // Error
  }
  
  // Read 40 bits
  for (byte = 0; byte < 5; byte++) {
    for (bit = 0; bit < 8; bit++) {
      while (!digitalRead(DHT22_PIN));
      delayMicroseconds(40);
      if (digitalRead(DHT22_PIN)) {
        data[byte] |= (1 << (7 - bit));
      }
      while (digitalRead(DHT22_PIN));
    }
  }
  
  // Checksum validation
  checkSum = data[0] + data[1] + data[2] + data[3];
  if (data[4] != checkSum) {
    return -999.0f;
  }
  
  // Temperature is in data[2] and data[3]
  float temp = ((data[2] & 0x7F) << 8) | data[3];
  temp /= 10.0f;
  
  if (data[2] & 0x80) {
    temp = -temp;
  }
  
  return temp;
}

/**
 * Odczyt wilgotności z DHT22
 */
float readHumidity() {
  // Podobna implementacja jak temperatura
  // Uproszczone dla przykładu
  
  return 45.0f; // Placeholder
}

/**
 * Odczyt poziomu audio (RMS)
 */
uint16_t readAudioRMS() {
  const int samples = 64;
  long sum = 0;
  
  for (int i = 0; i < samples; i++) {
    int val = analogRead(MIC_PIN);
    int normalized = val - 512; // Center around 0
    sum += normalized * normalized;
  }
  
  float rms = sqrt((float)sum / samples);
  return (uint16_t)rms;
}

/**
 * Odczyt aktywności piezo
 */
uint16_t readPiezoActivity() {
  int val = analogRead(PIEZO_PIN);
  return (uint16_t)abs(val - 512);
}

/**
 * Odczyt CO2 z SGP41/BME688 (I2C)
 */
uint16_t readCO2() {
  // Implementacja I2C dla SGP41
  // Adres I2C: 0x59
  
  Wire.beginTransmission(0x59);
  Wire.write(0x21); // Measure command
  Wire.endTransmission();
  
  delay(50);
  
  Wire.requestFrom(0x59, 6);
  
  if (Wire.available() >= 6) {
    uint8_t data[6];
    for (int i = 0; i < 6; i++) {
      data[i] = Wire.read();
    }
    
    // Parse CO2 value (przykładowe)
    uint16_t co2 = (data[0] << 8) | data[1];
    return co2;
  }
  
  return 400; // Default ambient CO2
}

/**
 * Odczyt TVOC z SGP41/BME688
 */
uint16_t readTVOC() {
  // Podobna implementacja jak CO2
  return 100; // Placeholder
}

/**
 * Odczyt detekcji ruchu z radaru MMWave (UART)
 */
uint8_t readRadarMotion() {
  // Parsowanie ramki UART z LD2410B
  if (Serial1.available() >= 10) {
    // Przykładowe parsowanie
    uint8_t header[4];
    Serial1.readBytes(header, 4);
    
    if (header[0] == 0xF4 && header[1] == 0xF3 && 
        header[2] == 0xF2 && header[3] == 0xF1) {
      // Valid frame
      uint8_t motion = Serial1.read();
      return motion;
    }
  }
  
  return 0;
}

/**
 * Przetwarzanie komend sieciowych
 */
void processNetworkCommands() {
  if (!ethClient.connected()) {
    ethClient.stop();
    
    // Próba połączenia z serwerem
    if (ethClient.connect(serverIP, serverPort)) {
      Serial.println(F("Połączono z serwerem"));
    }
    return;
  }
  
  // Sprawdź dostępne dane
  if (ethClient.available()) {
    // Odbierz komendę
    uint8_t cmdHeader = ethClient.read();
    
    if (cmdHeader == 0xAA) {
      // Command packet
      uint8_t cmdData[8];
      ethClient.readBytes(cmdData, 8);
      
      actuatorCmd.heater_pwm = cmdData[0];
      actuatorCmd.fan_pwm = cmdData[1];
      actuatorCmd.pump_duration = cmdData[2];
      actuatorCmd.valve_state = cmdData[3];
      actuatorCmd.relay_mask = cmdData[4];
    }
  }
}

/**
 * Wysyłka danych sensorów
 */
void sendSensorData() {
  if (!ethClient.connected()) {
    return;
  }
  
  // Nagłówek pakietu
  ethClient.write(0x55);
  
  // Dane sensorów
  ethClient.write((uint8_t*)&currentData, sizeof(SensorData));
  
  // Checksum
  uint8_t checksum = 0;
  uint8_t* ptr = (uint8_t*)&currentData;
  for (size_t i = 0; i < sizeof(SensorData); i++) {
    checksum ^= ptr[i];
  }
  ethClient.write(checksum);
}

/**
 * Wysyłka heartbeat
 */
void sendHeartbeat() {
  if (!ethClient.connected()) {
    return;
  }
  
  ethClient.write(0xBB);
  ethClient.write(millis() >> 24);
  ethClient.write(millis() >> 16);
  ethClient.write(millis() >> 8);
  ethClient.write(millis() & 0xFF);
}

/**
 * Wykonanie komend efektorów
 */
void executeActuatorCommands() {
  static bool pumpRunning = false;
  static unsigned long pumpStart = 0;
  
  // Heater PWM
  setHeaterPWM(actuatorCmd.heater_pwm);
  
  // Fan PWM
  setFanPWM(actuatorCmd.fan_pwm);
  
  // Pump
  if (actuatorCmd.pump_duration > 0 && !pumpRunning) {
    pumpStart = millis();
    pumpRunning = true;
    digitalWrite(PUMP_PIN, HIGH);
  }
  
  if (pumpRunning && millis() - pumpStart >= (unsigned long)actuatorCmd.pump_duration * 1000) {
    digitalWrite(PUMP_PIN, LOW);
    pumpRunning = false;
    actuatorCmd.pump_duration = 0; // Reset command
  }
  
  // Valve
  setValve(actuatorCmd.valve_state);
  
  // Relays
  setRelays(actuatorCmd.relay_mask);
}

/**
 * Ustawienie PWM grzałki
 */
void setHeaterPWM(uint8_t value) {
  analogWrite(HEATER_PWM, value);
}

/**
 * Ustawienie PWM wentylatora
 */
void setFanPWM(uint8_t value) {
  analogWrite(FAN_PWM, value);
}

/**
 * Ustawienie zaworu
 */
void setValve(uint8_t state) {
  digitalWrite(VALVE_PIN, state ? HIGH : LOW);
}

/**
 * Ustawienie przekaźników
 */
void setRelays(uint8_t mask) {
  digitalWrite(RELAY_1, (mask & 0x01) ? HIGH : LOW);
  digitalWrite(RELAY_2, (mask & 0x02) ? HIGH : LOW);
}

/**
 * Wczytanie kalibracji z EEPROM
 */
void loadCalibration() {
  // Implementacja odczytu z EEPROM
  // W produkcji użyj biblioteki EEPROM
  
  weight_scale = 1.0f;
  weight_offset = 0.0f;
}

/**
 * Zapis kalibracji do EEPROM
 */
void saveCalibration() {
  // Implementacja zapisu do EEPROM
}

/**
 * Automatyczna kalibracja wagi
 */
void performAutoCalibration() {
  Serial.println(F("Kalibracja wagi..."));
  
  // Zerowanie
  weight_offset = readWeightRaw();
  
  // Kalibracja z znanym ciężarem (np. 1kg)
  Serial.println(F("Umiesc 1kg i nacisnij dowolny klawisz..."));
  while (!Serial.available()) {}
  Serial.read();
  
  long withWeight = readWeightRaw();
  weight_scale = (withWeight - weight_offset) / 1000.0f;
  
  saveCalibration();
  Serial.println(F("Kalibracja zakończona"));
}

/**
 * Odczyt surowej wartości wagi
 */
long readWeightRaw() {
  long sum = 0;
  const int samples = 10;
  
  for (int i = 0; i < samples; i++) {
    while (digitalRead(HX711_DOUT));
    
    long value = 0;
    for (int j = 0; j < 24; j++) {
      digitalWrite(HX711_SCK, HIGH);
      value = value << 1;
      if (digitalRead(HX711_DOUT)) {
        value++;
      }
      digitalWrite(HX711_SCK, LOW);
    }
    
    digitalWrite(HX711_SCK, HIGH);
    digitalWrite(HX711_SCK, LOW);
    
    if (value & 0x800000) {
      value |= 0xFF000000;
    }
    
    sum += value;
  }
  
  return sum / samples;
}
