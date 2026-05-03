/*
 * ApiaryGuard - Firmware dla Raspberry Pi Pico (RP2040)
 * Obsługa: W6100 (Ethernet), Sensory (HX711, DHT22, SGP41, LD2410B, MEMS, Piezo), Efektory
 * Środowisko: Arduino IDE (Core: Raspberry Pi RP2040 Boards)
 * Język: C++
 * 
 * Połączenia sprzętowe (PINOUT PICO):
 * ---------------------------------------------------------
 * MODUŁ W6100 (SPI1):
 *   CS   -> GPIO 9  (SS)
 *   MOSI -> GPIO 11 (MOSI)
 *   MISO -> GPIO 12 (MISO)
 *   SCK  -> GPIO 10 (SCK)
 *   RST  -> GPIO 8
 *   INT  -> GPIO 13 (opcjonalny, do przerwania)
 * 
 * RADAR LD2410B (UART1):
 *   TX   -> GPIO 4 (RX1)
 *   RX   -> GPIO 5 (TX1)
 *   VCC  -> 5V
 *   GND  -> GND
 * 
 * CZUJNIKI I2C (SGP41/BME688) (I2C0):
 *   SDA  -> GPIO 0
 *   SCL  -> GPIO 1
 * 
 * DHT22 (Temp/Wilg):
 *   DATA -> GPIO 2
 * 
 * HX711 (Waga):
 *   DT   -> GPIO 3
 *   SCK  -> GPIO 22
 * 
 * MEMS Mic & Piezo (ADC):
 *   MIC  -> GPIO 26 (ADC0)
 *   PIEZO-> GPIO 27 (ADC1)
 * 
 * EFEKTORY (PWM/GPIO):
 *   Grzałka PWM    -> GPIO 6
 *   Wentylator PWM -> GPIO 7
 *   Pompa perystaltyczna (Relay) -> GPIO 14
 *   Zawór elektromagnetyczny 1   -> GPIO 15
 *   Zawór elektromagnetyczny 2   -> GPIO 16
 *   Przekaźnik 8-kanałowy (przykładowe piny) -> GPIO 17-24
 *   LED Status     -> GPIO 25 (WBUDOWANY)
 * ---------------------------------------------------------
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP41.h>
#include <HardwareSerial.h>

// --- KONFIGURACJA SIECI W6100 ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177); // Statyczne IP dla Pico
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(8080); // Port nasłuchiwania

// --- DEFINICJE PINÓW PICO ---
#define W6100_CS   9
#define W6100_RST  8
#define W6100_INT  13

#define RADAR_RX   4
#define RADAR_TX   5

#define I2C_SDA    0
#define I2C_SCL    1

#define DHT_PIN    2
#define HX711_DT   3
#define HX711_SCK  22

#define MIC_PIN    26 // ADC0
#define PIEZO_PIN  27 // ADC1

#define HEATER_PWM 6
#define FAN_PWM    7
#define PUMP_RELAY 14
#define VALVE_1    15
#define VALVE_2    16

// Dodatkowe piny dla przekaźnika 8-kanałowego
#define RELAY_CH1  17
#define RELAY_CH2  18
#define RELAY_CH3  19
#define RELAY_CH4  20
#define RELAY_CH5  21
#define RELAY_CH6  22
#define RELAY_CH7  23
#define RELAY_CH8  24

// --- OBIEKTY ---
EthernetClient client;
DHT dht(DHT_PIN, DHT22);
Adafruit_SGP41 sgp;
HardwareSerial radarSerial(uart1); // UART1 dla radaru

// Zmienne dla HX711 (prosta implementacja bit-bang)
long hx711_value = 0;
long hx711_offset = 0;
float hx711_scale = 1.0;

// Zmienne sensorów
float temperature = 0.0;
float humidity = 0.0;
uint16_t co2_eq = 0;
uint16_t voc_idx = 0;
float audio_rms = 0.0;
float vibration_level = 0.0;
bool motion_detected = false;

// Timer'y
unsigned long lastMillis = 0;
unsigned long networkHeartbeat = 0;

// --- FUNKCJE POMOCNICZE ---

// Inicjalizacja W6100
void initW6100() {
  pinMode(W6100_RST, OUTPUT);
  pinMode(W6100_CS, OUTPUT);
  
  digitalWrite(W6100_RST, HIGH);
  delay(100);
  digitalWrite(W6100_RST, LOW);
  delay(200);
  digitalWrite(W6100_RST, HIGH);
  delay(200);
  
  digitalWrite(W6100_CS, HIGH);
  
  Serial.println("Inicjalizacja W6100...");
  
  // Konfiguracja SPI1 dla W6100
  // Uwaga: Biblioteka Ethernet standardowo używa SPI. 
  // Na Pico trzeba upewnić się, że piny są przypisane do SPI1.
  // Jeśli biblioteka nie wykrywa automatycznie, może być wymagana modyfikacja pins_arduino.h lub użycie setCS().
  
  if (Ethernet.begin(mac, ip, gateway, subnet)) {
    Serial.println("W6100 połączono. IP: " + Ethernet.localIP().toString());
    server.begin();
  } else {
    Serial.println("Błąd konfiguracji W6100!");
    while(true); // Zatrzymaj jeśli brak łącza
  }
}

// Obsługa HX711 (Bit-bang dla Pico)
long readHX711() {
  long count = 0;
  pinMode(HX711_DT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  
  digitalWrite(HX711_SCK, LOW);
  while(digitalRead(HX711_DT)); // Czekaj na gotowość
  
  for(int i = 0; i < 24; i++) {
    digitalWrite(HX711_SCK, HIGH);
    count = count << 1;
    digitalWrite(HX711_SCK, LOW);
    if(digitalRead(HX711_DT)) count++;
  }
  
  // 25 impuls dla następnego odczytu (gain 128)
  digitalWrite(HX711_SCK, HIGH);
  count = count ^ 0x800000; // Konwersja na signed
  digitalWrite(HX711_SCK, LOW);
  
  return count;
}

// Obliczanie RMS dźwięku
float calculateAudioRMS() {
  int samples = 100;
  float sum = 0;
  for(int i=0; i<samples; i++) {
    int val = analogRead(MIC_PIN);
    float voltage = (val / 4095.0) * 3.3; // Pico ADC 12-bit
    sum += (voltage * voltage);
    delay(1);
  }
  return sqrt(sum / samples);
}

// Parsowanie ramki LD2410B (uproszczone)
void processRadar() {
  static uint8_t buffer[10];
  static int idx = 0;
  
  while(radarSerial.available()) {
    uint8_t b = radarSerial.read();
    
    // Prosta detekcja nagłówka 0xF4 0xF3 0x01 ...
    if(idx == 0 && b != 0xF4) continue;
    if(idx == 1 && b != 0xF3) { idx = 0; continue; }
    
    buffer[idx++] = b;
    if(idx >= 10) {
      // Przykładowa analiza: bajt 8 zawiera informację o ruchu
      if(buffer[8] > 0) motion_detected = true;
      else motion_detected = false;
      
      idx = 0; // Reset bufora
      break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial && (millis() < 3000)); // Czekaj na USB CDC
  
  Serial.println("=== ApiaryGuard Pico Start ===");
  
  // Inicjalizacja I2C
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  
  // Inicjalizacja UART dla Radaru
  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX, RADAR_TX);
  
  // Inicjalizacja Sensorów
  dht.begin();
  
  if (!sgp.begin_I2C(0x59, &Wire)) { // Domyślny adres SGP41
    Serial.println("Nie znaleziono SGP41!");
  } else {
    sgp.measureBaseline();
  }
  
  // Inicjalizacja PWM
  pinMode(HEATER_PWM, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);
  analogWriteFrequency(HEATER_PWM, 1000);
  analogWriteFrequency(FAN_PWM, 1000);
  
  // Inicjalizacja GPIO
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
  
  // Start Ethernet W6100
  initW6100();
  
  Serial.println("System gotowy.");
}

void loop() {
  unsigned long now = millis();
  
  // 1. Odczyt Sensorów (co 2 sekundy)
  if(now - lastMillis > 2000) {
    lastMillis = now;
    
    // DHT22
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if(!isnan(h)) humidity = h;
    if(!isnan(t)) temperature = t;
    
    // HX711
    long raw = readHX711();
    hx711_value = (raw - hx711_offset) / hx711_scale;
    
    // SGP41 (CO2/VOC)
    sgp.measureGas();
    co2_eq = sgp.CO2eq;
    voc_idx = sgp.VOCindex;
    
    // Audio RMS
    audio_rms = calculateAudioRMS();
    
    // Wibracje (Piezo)
    vibration_level = analogRead(PIEZO_PIN);
    
    // Radar
    processRadar();
    
    // Debug Serial
    Serial.printf("T:%.1f H:%.1f Waga:%ld CO2:%d VOC:%d Audio:%.2f Ruch:%d\n", 
                  temperature, humidity, hx711_value, co2_eq, voc_idx, audio_rms, motion_detected);
  }
  
  // 2. Obsługa Sieci W6100
  EthernetClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();
    
    // Prosty protokół HTTP/GET lub binarny
    // Przykład: GET /status
    if(request.indexOf("/status") > 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"temp\":"); client.print(temperature);
      client.print(",\"hum\":"); client.print(humidity);
      client.print(",\"weight\":"); client.print(hx711_value);
      client.print(",\"co2\":"); client.print(co2_eq);
      client.print(",\"voc\":"); client.print(voc_idx);
      client.print(",\"audio\":"); client.print(audio_rms);
      client.print(",\"motion\":"); client.print(motion_detected ? 1 : 0);
      client.println("}");
    } else if (request.indexOf("/heater/on") > 0) {
      analogWrite(HEATER_PWM, 200); // PWM 0-255
      client.println("HTTP/1.1 200 OK\n\nHeater ON");
    } else if (request.indexOf("/heater/off") > 0) {
      analogWrite(HEATER_PWM, 0);
      client.println("HTTP/1.1 200 OK\n\nHeater OFF");
    } else if (request.indexOf("/fan/on") > 0) {
      analogWrite(FAN_PWM, 255);
      client.println("HTTP/1.1 200 OK\n\nFan ON");
    } else if (request.indexOf("/fan/off") > 0) {
      analogWrite(FAN_PWM, 0);
      client.println("HTTP/1.1 200 OK\n\nFan OFF");
    } else if (request.indexOf("/pump/on") > 0) {
      digitalWrite(PUMP_RELAY, HIGH);
      client.println("HTTP/1.1 200 OK\n\nPump ON");
    } else if (request.indexOf("/pump/off") > 0) {
      digitalWrite(PUMP_RELAY, LOW);
      client.println("HTTP/1.1 200 OK\n\nPump OFF");
    }
    
    delay(1);
    client.stop();
  }
  
  // 3. Heartbeat sieciowy (opcjonalne pingi)
  if(now - networkHeartbeat > 10000) {
    networkHeartbeat = now;
    // Tu można dodać wysyłanie danych do serwera centralnego przez TCP/UDP
  }
  
  // 4. Logika sterowania (przykład)
  if(temperature < 15.0) {
    analogWrite(HEATER_PWM, 150); // Włącz grzanie
  } else {
    analogWrite(HEATER_PWM, 0);
  }
  
  if(motion_detected && humidity > 80) {
    digitalWrite(VALVE_1, HIGH); // Otwórz wentylację
  } else {
    digitalWrite(VALVE_1, LOW);
  }
}
