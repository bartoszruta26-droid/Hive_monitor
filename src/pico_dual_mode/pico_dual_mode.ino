/*
 * Raspberry Pi Pico - Dual Mode Communication (USB Priority + ENC28J60)
 * 
 * Opis:
 * Mikrokontroler automatycznie wykrywa aktywne połączenie.
 * 1. Priorytet: USB (Serial). Jeśli wykryto komunikację hosta, nasłuchuje komend tekstowych.
 * 2. Fallback: Ethernet (ENC28J60). Jeśli brak aktywności na USB, uruchamia serwer HTTP.
 * 
 * Wymagane biblioteki (Arduino IDE):
 * - UIPEthernet (dla modułu ENC28J60)
 * - DHT sensor library (Adafruit)
 * - HX711-Arduino (by Bogdan Necula)
 * - SparkFun SGP41 Arduino Library
 * 
 * Pinout (Konfiguracja):
 * --- Sensory ---
 * DHT22 (Temp/Hum) : GPIO 2 (Pin 4)
 * HX711 (Waga)     : DT -> GPIO 3 (Pin 5), SCK -> GPIO 4 (Pin 6)
 * SGP41 (NOx/VOC)  : I2C (SDA: GPIO 0/Pin 1, SCL: GPIO 1/Pin 2)
 * Mikrofon/Analog  : GPIO 26 (ADC0)
 * 
 * --- Aktuary ---
 * Grzałka (PWM)    : GPIO 5 (Pin 7)
 * Wentylator (PWM) : GPIO 6 (Pin 9)
 * Pompa (PWM)      : GPIO 7 (Pin 10)
 * Przekaźnik 1     : GPIO 8 (Pin 11)
 * Przekaźnik 2     : GPIO 9 (Pin 12)
 * Piezo/Buzzer     : GPIO 10 (Pin 14)
 * 
 * --- Ethernet (ENC28J60) ---
 * CS   : GPIO 17 (Pin 22) - Można zmienić w definicji
 * MOSI : GPIO 19 (Pin 25)
 * MISO : GPIO 16 (Pin 21)
 * SCK  : GPIO 18 (Pin 24)
 * RST  : GPIO 20 (Pin 26) - Opcjonalnie, często połączony z resetem Pico lub VCC przez rezystor
 */

#include <SPI.h>
#include <UIPEthernet.h> // Biblioteka dla ENC28J60
#include <DHT.h>
#include <HX711.h>
#include <Wire.h>
#include <SparkFun_SGP41_Arduino_Library.h>

// ================= KONFIGURACJA PINÓW =================
#define PIN_DHT 2
#define PIN_HX711_DT 3
#define PIN_HX711_SCK 4
#define PIN_MIC 26 // ADC0

#define PIN_HEATER 5
#define PIN_FAN 6
#define PIN_PUMP 7
#define PIN_RELAY1 8
#define PIN_RELAY2 9
#define PIN_BUZZER 10

#define PIN_ETH_CS 17 // Chip Select dla ENC28J60

// ================= KONFIGURACJA SIECIOWA =================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177); // Statyczne IP lub DHCP
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ================= OBIEKTY GLOBALNE =================
DHT dht(PIN_DHT, DHT22);
HX711 scale;
SGP41 sgp41;

EthernetServer server(80);
EthernetClient client;

// Zmienne stanu
bool usbActive = false;
bool ethActive = false;
unsigned long lastActivityTime = 0;
unsigned long lastSensorRead = 0;

// Dane z sensorów
float temperature = 0.0;
float humidity = 0.0;
float weight = 0.0;
uint16_t nox = 0;
uint16_t voc = 0;
int micLevel = 0;

// Ustawienia aktuatorów
int heaterDuty = 0;
int fanDuty = 0;
int pumpDuty = 0;
bool relay1State = false;
bool relay2State = false;

// Kalibracja wagi
#define SCALE_CALIBRATION_FACTOR 11000.0 // Do dostosowania
#define SCALE_TARE_OFFSET 0

void setup() {
  // Inicjalizacja Serial (USB) - zawsze startuje
  Serial.begin(115200);
  while (!Serial && millis() < 2000); // Czekaj na enumerację USB (do 2s)
  
  Serial.println("\n--- System Start: Pico Dual Mode ---");

  // Konfiguracja Pinów
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  
  // PWM Setup (Pico obsługuje analogWrite na odpowiednich pinach)
  analogWriteFreq(1000); // 1kHz PWM
  analogWriteRange(255); // 8-bit resolution

  // Inicjalizacja Sensorów
  dht.begin();
  
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  
  if (sgp41.begin(Wire) == false) {
    Serial.println("SGP41 nie wykryty! Sprawdź połączenia I2C.");
  } else {
    sgp41.measureRawSignal(); // Start pomiarów
  }

  scale.begin(PIN_HX711_DT, PIN_HX711_SCK);
  scale.set_scale(SCALE_CALIBRATION_FACTOR);
  scale.tare(SCALE_TARE_OFFSET);

  // Sprawdzenie trybu połączenia
  detectConnectionMode();
}

void loop() {
  // 1. Obsługa czujników (co 1 sekundę)
  if (millis() - lastSensorRead > 1000) {
    readSensors();
    lastSensorRead = millis();
  }

  // 2. Detekcja aktywności USB
  // Jeśli coś przyszło przez Serial, uznajemy USB za aktywne
  if (Serial.available()) {
    usbActive = true;
    lastActivityTime = millis();
    handleUSBCommand();
  }

  // 3. Logika przełączania trybów
  // Jeśli USB było aktywne w ciągu ostatnich 5 sekund, pracujemy w trybie USB
  // W przeciwnym razie próbujemy uruchomić Ethernet
  
  if (usbActive && (millis() - lastActivityTime < 5000)) {
    // Tryb USB: Ethernet może być wyłączony dla oszczędności energii lub zostawiony w tle
    if (ethActive) {
      // Opcjonalnie: server.stop(); ethActive = false; 
      // Na razie zostawiamy działające, ale priorytet ma obsługa Serial
    }
  } else {
    // Brak aktywności USB -> Przełącz w tryb Ethernet
    if (!ethActive) {
      Serial.println("Brak aktywności USB. Uruchamianie Ethernet...");
      initEthernet();
    }
    
    // Obsługa klienta Ethernet
    if (ethActive) {
      EthernetClient client = server.available();
      if (client) {
        handleEthernetClient(client);
      }
    }
  }

  // Aktualizacja wyjść PWM (można dodać miękkie starty itp.)
  updateOutputs();
}

void detectConnectionMode() {
  // Prosta detekcja: czekamy chwilę na sygnał z hosta USB
  delay(1000);
  if (Serial) {
    Serial.println("Wykryto połączenie USB. Tryb główny: SERIAL.");
    Serial.println("Wpisz 'HELP' aby zobaczyć listę komend.");
    usbActive = true;
    lastActivityTime = millis();
  } else {
    Serial.println("Nie wykryto hosta USB. Przejście w tryb Ethernet.");
    usbActive = false;
    initEthernet();
  }
}

void initEthernet() {
  Ethernet.init(PIN_ETH_CS);
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Nie udało się uzyskać adresu IP (DHCP). Próba statycznego...");
    Ethernet.begin(mac, ip, gateway, subnet);
  }
  
  server.begin();
  ethActive = true;
  
  IPAddress myIP = Ethernet.localIP();
  Serial.print("Serwer HTTP uruchomiony na: http://");
  Serial.println(myIP);
}

void readSensors() {
  // DHT
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  // Waga
  if (scale.is_ready()) {
    weight = scale.get_units(5); // Średnia z 5 pomiarów
  }

  // SGP41
  if (sgp41.isConnected()) {
    sgp41.measureRawSignal();
    nox = sgp41.rawNOx;
    voc = sgp41.rawVoc;
  }

  // Mikrofon (ADC)
  micLevel = analogRead(PIN_MIC);
}

void handleUSBCommand() {
  static String buffer = "";
  
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCommand(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }
}

void handleEthernetClient(EthernetClient client) {
  if (client.connected()) {
    if (client.available()) {
      String request = client.readStringUntil('\r');
      // Bardzo uproszczona obsługa HTTP GET
      if (request.indexOf("/api/status") >= 0) {
        sendJsonResponse(client);
      } else if (request.indexOf("/cmd?") >= 0) {
        // Parsowanie komendy z URL (np. /cmd?SET_HEATER=50)
        int eqIndex = request.indexOf('=');
        if (eqIndex != -1) {
          String cmdPart = request.substring(request.indexOf('?') + 1, eqIndex);
          String valPart = request.substring(eqIndex + 1);
          // Usunięcie ewentualnych znaków kończących HTTP
          valPart.trim(); 
          processCommand(cmdPart + " " + valPart);
        }
        sendHtmlDashboard(client);
      } else {
        sendHtmlDashboard(client);
      }
      delay(1);
      client.stop();
    }
  }
}

void processCommand(String cmd) {
  cmd.toUpperCase();
  cmd.trim();
  
  // Przykładowe komendy:
  // SET_HEATER 50
  // SET_FAN 100
  // SET_PUMP 0
  // SET_RELAY1 ON
  // CALIB_WEIGHT
  // STATUS
  
  if (cmd.startsWith("SET_HEATER")) {
    int val = extractValue(cmd);
    heaterDuty = constrain(val, 0, 255);
    Serial.println("OK: Grzałka ustawiona na " + String(heaterDuty));
  }
  else if (cmd.startsWith("SET_FAN")) {
    int val = extractValue(cmd);
    fanDuty = constrain(val, 0, 255);
    Serial.println("OK: Wentylator ustawiony na " + String(fanDuty));
  }
  else if (cmd.startsWith("SET_PUMP")) {
    int val = extractValue(cmd);
    pumpDuty = constrain(val, 0, 255);
    Serial.println("OK: Pompa ustawiona na " + String(pumpDuty));
  }
  else if (cmd.startsWith("SET_RELAY1")) {
    relay1State = (cmd.indexOf("ON") > 0 || cmd.indexOf("1") > 0);
    Serial.println("OK: Przekaźnik 1: " + String(relay1State ? "ON" : "OFF"));
  }
  else if (cmd.startsWith("SET_RELAY2")) {
    relay2State = (cmd.indexOf("ON") > 0 || cmd.indexOf("1") > 0);
    Serial.println("OK: Przekaźnik 2: " + String(relay2State ? "ON" : "OFF"));
  }
  else if (cmd.startsWith("CALIB_WEIGHT")) {
    scale.tare();
    Serial.println("OK: Waga wyzerowana (Tara).");
  }
  else if (cmd.startsWith("STATUS")) {
    printStatus(Serial);
  }
  else if (cmd.startsWith("HELP")) {
    Serial.println("Dostępne komendy:");
    Serial.println("  SET_HEATER [0-255]");
    Serial.println("  SET_FAN [0-255]");
    Serial.println("  SET_PUMP [0-255]");
    Serial.println("  SET_RELAY1 [ON/OFF]");
    Serial.println("  SET_RELAY2 [ON/OFF]");
    Serial.println("  CALIB_WEIGHT");
    Serial.println("  STATUS");
  }
  else {
    Serial.println("Nieznana komenda. Wpisz HELP.");
  }
}

int extractValue(String cmd) {
  int spaceIndex = cmd.indexOf(' ');
  if (spaceIndex != -1) {
    return cmd.substring(spaceIndex + 1).toInt();
  }
  return 0;
}

void updateOutputs() {
  analogWrite(PIN_HEATER, heaterDuty);
  analogWrite(PIN_FAN, fanDuty);
  analogWrite(PIN_PUMP, pumpDuty);
  digitalWrite(PIN_RELAY1, relay1State ? HIGH : LOW);
  digitalWrite(PIN_RELAY2, relay2State ? HIGH : LOW);
}

void printStatus(Print& out) {
  out.print("{\"temp\":"); out.print(temperature);
  out.print(", \"hum\":"); out.print(humidity);
  out.print(", \"weight\":"); out.print(weight);
  out.print(", \"nox\":"); out.print(nox);
  out.print(", \"voc\":"); out.print(voc);
  out.print(", \"mic\":"); out.print(micLevel);
  out.print(", \"heater\":"); out.print(heaterDuty);
  out.print(", \"fan\":"); out.print(fanDuty);
  out.print(", \"relay1\":"); out.print(relay1State ? 1 : 0);
  out.println("}");
}

void sendJsonResponse(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  printStatus(client);
}

void sendHtmlDashboard(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>Pico Controller</title>");
  client.println("<meta http-equiv='refresh' content='5'>");
  client.println("</head><body>");
  client.println("<h1>Panel Sterowania Pico</h1>");
  client.print("<p>Temperatura: "); client.print(temperature); client.println(" C</p>");
  client.print("<p>Wilgotność: "); client.print(humidity); client.println(" %</p>");
  client.print("<p>Waga: "); client.print(weight); client.println(" kg</p>");
  client.print("<p>NOx: "); client.print(nox); client.println("</p>");
  
  client.println("<h3>Sterowanie</h3>");
  client.println("<a href='/cmd?SET_HEATER=0'><button>Grzałka OFF</button></a>");
  client.println("<a href='/cmd?SET_HEATER=128'><button>Grzałka 50%</button></a>");
  client.println("<a href='/cmd?SET_HEATER=255'><button>Grzałka 100%</button></a><br>");
  
  client.println("<a href='/cmd?SET_RELAY1=ON'><button>Relay1 ON</button></a>");
  client.println("<a href='/cmd?SET_RELAY1=OFF'><button>Relay1 OFF</button></a><br>");
  
  client.println("</body></html>");
}
