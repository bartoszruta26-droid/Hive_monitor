/*
 * ApiaryGuard - Arduino Nano Firmware z Web Server GUI
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
 * Funkcje dodatkowe:
 * - Web Server z GUI HTML/CSS
 * - JSON API dla wszystkich parametrów
 * - Panel sterowania efektorami
 * - Auto-refresh danych co 5 sekund
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

// Web Server port
EthernetServer webServer(80);

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
EthernetClient webClient;
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
void handleWebClients();
void sendWebPage(EthernetClient &client);
void sendJSONData(EthernetClient &client);
void handleControlCommand(String params);
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
  
  // Start Web Server
  webServer.begin();
  
  Serial.println(F("System gotowy!"));
  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
  Serial.println(F("Web Server: http://"));
  Serial.println(Ethernet.localIP());
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();
  
  // Obsługa klienta Ethernet (MQTT)
  Ethernet.maintain();
  
  // Obsługa Web Server GUI
  handleWebClients();
  
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

// ============================================================================
// WEB SERVER - GUI I API
// ============================================================================

/**
 * Obsługa klientów web servera
 */
void handleWebClients() {
  EthernetClient client = webServer.available();
  
  if (client) {
    boolean currentLineIsBlank = true;
    String httpRequest = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        httpRequest += c;
        
        if (c == '\n' && currentLineIsBlank) {
          // Koniec nagłówka HTTP
          
          // Sprawdź typ żądania
          if (httpRequest.indexOf("GET /api ") > 0) {
            // Żądanie JSON API
            sendJSONData(client);
          } else if (httpRequest.indexOf("GET /control?") > 0) {
            // Żądanie sterowania
            int startIndex = httpRequest.indexOf("/control?");
            int endIndex = httpRequest.indexOf(" ", startIndex);
            String params = httpRequest.substring(startIndex + 9, endIndex);
            handleControlCommand(params);
            sendWebPage(client);
          } else {
            // Żądanie strony głównej
            sendWebPage(client);
          }
          break;
        }
        
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    
    delay(1);
    client.stop();
  }
}

/**
 * Wysyłka strony HTML z GUI
 */
void sendWebPage(EthernetClient &client) {
  // Nagłówek HTTP
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println("Refresh: 5");  // Auto-refresh co 5 sekund
  client.println();
  
  // HTML开始
  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  client.println("<title>ApiaryGuard - Monitor Ula</title>");
  client.println("<style>");
  client.println("*{box-sizing:border-box;margin:0;padding:0}");
  client.println("body{font-family:Arial,sans-serif;background:#f0f4f8;padding:20px}");
  client.println(".container{max-width:1200px;margin:0 auto}");
  client.println("h1{color:#2c3e50;text-align:center;margin-bottom:20px}");
  client.println(".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px}");
  client.println(".card{background:white;border-radius:10px;padding:20px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}");
  client.println(".card h2{color:#3498db;font-size:18px;margin-bottom:15px;border-bottom:2px solid #3498db;padding-bottom:10px}");
  client.println(".metric{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #eee}");
  client.println(".metric:last-child{border-bottom:none}");
  client.println(".metric-label{color:#7f8c8d}");
  client.println(".metric-value{color:#2c3e50;font-weight:bold}");
  client.println(".unit{color:#95a5a6;font-size:12px}");
  client.println(".status-ok{color:#27ae60}");
  client.println(".status-warn{color:#f39c12}");
  client.println(".status-error{color:#e74c3c}");
  client.println(".control-panel{margin-top:20px}");
  client.println(".control-group{background:white;border-radius:10px;padding:20px;margin-bottom:20px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}");
  client.println(".control-group h2{color:#e67e22;margin-bottom:15px}");
  client.println("input[type=range]{width:100%;margin:10px 0}");
  client.println("button{background:#3498db;color:white;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;margin:5px}");
  client.println("button:hover{background:#2980b9}");
  client.println("button.active{background:#27ae60}");
  client.println(".footer{text-align:center;margin-top:30px;color:#7f8c8d}");
  client.println("</style></head><body>");
  client.println("<div class='container'>");
  client.println("<h1>🐝 ApiaryGuard - Monitor Ula</h1>");
  
  // Sekcja Sensorów
  client.println("<div class='grid'>");
  
  // Karta: Waga
  client.println("<div class='card'><h2>⚖️ Waga Ula</h2>");
  client.print("<div class='metric'><span class='metric-label'>Waga:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.weight_kg, 2);
  client.println(" <span class='unit'>kg</span></span></div>");
  client.println("</div>");
  
  // Karta: Klimat
  client.println("<div class='card'><h2>🌡️ Klimat</h2>");
  client.print("<div class='metric'><span class='metric-label'>Temperatura:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.temperature_c, 1);
  client.println(" <span class='unit'>°C</span></span></div>");
  client.print("<div class='metric'><span class='metric-label'>Wilgotność:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.humidity_percent, 1);
  client.println(" <span class='unit'>%</span></span></div>");
  client.println("</div>");
  
  // Karta: Jakość Powietrza
  client.println("<div class='card'><h2>💨 Jakość Powietrza</h2>");
  client.print("<div class='metric'><span class='metric-label'>CO2:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.co2_ppm);
  client.println(" <span class='unit'>ppm</span></span></div>");
  client.print("<div class='metric'><span class='metric-label'>TVOC:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.tvoc_ppb);
  client.println(" <span class='unit'>ppb</span></span></div>");
  client.println("</div>");
  
  // Karta: Dźwięk i Wibracje
  client.println("<div class='card'><h2>🔊 Dźwięk i Wibracje</h2>");
  client.print("<div class='metric'><span class='metric-label'>Poziom Audio:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.audio_rms);
  client.println(" <span class='unit'>RMS</span></span></div>");
  client.print("<div class='metric'><span class='metric-label'>Wibracje:</span>");
  client.print("<span class='metric-value'>");
  client.print(currentData.piezo_activity);
  client.println(" <span class='unit'>ADC</span></span></div>");
  client.println("</div>");
  
  // Karta: Radar i Status
  client.println("<div class='card'><h2>📡 Radar i Status</h2>");
  client.print("<div class='metric'><span class='metric-label'>Ruch:</span>");
  client.print("<span class='metric-value ");
  if (currentData.radar_motion > 0) {
    client.println("status-ok'>Wykryto</span></div>");
  } else {
    client.println("'>Brak</span></div>");
  }
  client.print("<div class='metric'><span class='metric-label'>Sieć:</span>");
  client.print("<span class='metric-value ");
  if (Ethernet.linkStatus() == LinkON) {
    client.println("status-ok'>Połączono</span></div>");
  } else {
    client.println("status-error'>Rozłączono</span></div>");
  }
  client.print("<div class='metric'><span class='metric-label'>Uptime:</span>");
  client.print("<span class='metric-value'>");
  client.print(millis() / 1000);
  client.println(" <span class='unit'>s</span></span></div>");
  client.println("</div>");
  
  client.println("</div>");  // End grid
  
  // Panel Sterowania
  client.println("<div class='control-panel'>");
  client.println("<div class='control-group'><h2>🎛️ Panel Sterowania</h2>");
  
  // Grzałka
  client.println("<div style='margin-bottom:15px'>");
  client.println("<label>Grzałka PWM: </label>");
  client.print("<form method='GET' action='/control' style='display:inline'>");
  client.println("<input type='range' name='heater' min='0' max='255' value='");
  client.print(actuatorCmd.heater_pwm);
  client.println("' onchange='this.form.submit()'>");
  client.print("<span id='heaterVal'>");
  client.print(actuatorCmd.heater_pwm);
  client.println("</span></form></div>");
  
  // Wentylator
  client.println("<div style='margin-bottom:15px'>");
  client.println("<label>Wentylator PWM: </label>");
  client.print("<form method='GET' action='/control' style='display:inline'>");
  client.println("<input type='range' name='fan' min='0' max='255' value='");
  client.print(actuatorCmd.fan_pwm);
  client.println("' onchange='this.form.submit()'>");
  client.print("<span id='fanVal'>");
  client.print(actuatorCmd.fan_pwm);
  client.println("</span></form></div>");
  
  // Pompa
  client.println("<div style='margin-bottom:15px'>");
  client.println("<label>Pompa: </label>");
  client.print("<form method='GET' action='/control' style='display:inline'>");
  client.println("<button type='submit' name='pump' value='5'>5s</button>");
  client.println("<button type='submit' name='pump' value='10'>10s</button>");
  client.println("<button type='submit' name='pump' value='30'>30s</button>");
  client.println("</form></div>");
  
  // Zawór
  client.println("<div style='margin-bottom:15px'>");
  client.println("<label>Zawór: </label>");
  client.print("<form method='GET' action='/control' style='display:inline'>");
  if (actuatorCmd.valve_state == 0) {
    client.println("<button type='submit' name='valve' value='1' style='background:#e74c3c'>WŁĄCZ</button>");
  } else {
    client.println("<button type='submit' name='valve' value='0' class='active'>WYŁĄCZ</button>");
  }
  client.println("</form></div>");
  
  // Przekaźniki
  client.println("<div style='margin-bottom:15px'>");
  client.println("<label>Przekaźniki: </label>");
  client.print("<form method='GET' action='/control' style='display:inline'>");
  client.print("<button type='submit' name='relay1' value='");
  client.println((actuatorCmd.relay_mask & 0x01) ? "0" : "1");
  client.print("'>Relay 1: ");
  client.println((actuatorCmd.relay_mask & 0x01) ? "ON" : "OFF");
  client.print("</button><button type='submit' name='relay2' value='");
  client.println((actuatorCmd.relay_mask & 0x02) ? "0" : "2");
  client.print("'>Relay 2: ");
  client.println((actuatorCmd.relay_mask & 0x02) ? "ON" : "OFF");
  client.println("</button></form></div>");
  
  client.println("</div></div>");  // End control-panel and control-group
  
  // Footer
  client.println("<div class='footer'>");
  client.println("<p>ApiaryGuard v1.0 | Odświeżanie: 5s | <a href='/api' target='_blank'>API JSON</a></p>");
  client.println("</div>");
  
  client.println("</div></body></html>");
}

/**
 * Wysyłka danych w formacie JSON (API)
 */
void sendJSONData(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  
  client.println("{");
  client.print("  \"timestamp\": ");
  client.println(currentData.timestamp);
  client.print("  \"weight_kg\": ");
  client.println(currentData.weight_kg, 2);
  client.print("  \"temperature_c\": ");
  client.println(currentData.temperature_c, 1);
  client.print("  \"humidity_percent\": ");
  client.println(currentData.humidity_percent, 1);
  client.print("  \"audio_rms\": ");
  client.println(currentData.audio_rms);
  client.print("  \"piezo_activity\": ");
  client.println(currentData.piezo_activity);
  client.print("  \"co2_ppm\": ");
  client.println(currentData.co2_ppm);
  client.print("  \"tvoc_ppb\": ");
  client.println(currentData.tvoc_ppb);
  client.print("  \"radar_motion\": ");
  client.println(currentData.radar_motion);
  client.print("  \"network_connected\": ");
  client.println((Ethernet.linkStatus() == LinkON) ? "true" : "false");
  client.print("  \"uptime_seconds\": ");
  client.println(millis() / 1000);
  client.print("  \"heater_pwm\": ");
  client.println(actuatorCmd.heater_pwm);
  client.print("  \"fan_pwm\": ");
  client.println(actuatorCmd.fan_pwm);
  client.print("  \"valve_state\": ");
  client.println(actuatorCmd.valve_state);
  client.print("  \"relay_mask\": ");
  client.println(actuatorCmd.relay_mask);
  client.println("}");
}

/**
 * Obsługa komend sterujących z GUI
 */
void handleControlCommand(String params) {
  // Parsowanie parametrów URL
  if (params.indexOf("heater=") >= 0) {
    int start = params.indexOf("heater=") + 7;
    int end = params.indexOf("&", start);
    if (end < 0) end = params.length();
    String val = params.substring(start, end);
    actuatorCmd.heater_pwm = constrain(val.toInt(), 0, 255);
    Serial.print("Heater PWM: ");
    Serial.println(actuatorCmd.heater_pwm);
  }
  
  if (params.indexOf("fan=") >= 0) {
    int start = params.indexOf("fan=") + 4;
    int end = params.indexOf("&", start);
    if (end < 0) end = params.length();
    String val = params.substring(start, end);
    actuatorCmd.fan_pwm = constrain(val.toInt(), 0, 255);
    Serial.print("Fan PWM: ");
    Serial.println(actuatorCmd.fan_pwm);
  }
  
  if (params.indexOf("pump=") >= 0) {
    int start = params.indexOf("pump=") + 5;
    int end = params.indexOf("&", start);
    if (end < 0) end = params.length();
    String val = params.substring(start, end);
    actuatorCmd.pump_duration = constrain(val.toInt(), 0, 60);
    Serial.print("Pump duration: ");
    Serial.print(actuatorCmd.pump_duration);
    Serial.println("s");
  }
  
  if (params.indexOf("valve=") >= 0) {
    int start = params.indexOf("valve=") + 6;
    int end = params.indexOf("&", start);
    if (end < 0) end = params.length();
    String val = params.substring(start, end);
    actuatorCmd.valve_state = val.toInt();
    Serial.print("Valve: ");
    Serial.println(actuatorCmd.valve_state ? "ON" : "OFF");
  }
  
  if (params.indexOf("relay1=") >= 0) {
    int start = params.indexOf("relay1=") + 7;
    int end = params.indexOf("&", start);
    if (end < 0) end = params.length();
    String val = params.substring(start, end);
    if (val.toInt() == 1) {
      actuatorCmd.relay_mask |= 0x01;
    } else {
      actuatorCmd.relay_mask &= ~0x01;
    }
    Serial.print("Relay 1: ");
    Serial.println((actuatorCmd.relay_mask & 0x01) ? "ON" : "OFF");
  }
  
  if (params.indexOf("relay2=") >= 0) {
    int start = params.indexOf("relay2=") + 7;
    int end = params.indexOf("&", start);
    if (end < 0) end = params.length();
    String val = params.substring(start, end);
    if (val.toInt() == 2) {
      actuatorCmd.relay_mask |= 0x02;
    } else {
      actuatorCmd.relay_mask &= ~0x02;
    }
    Serial.print("Relay 2: ");
    Serial.println((actuatorCmd.relay_mask & 0x02) ? "ON" : "OFF");
  }
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
