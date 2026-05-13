/**
 * Flowing Hive - Automatyczny Efektor Opróżniania Ramek
 * 
 * System automatycznego wykrywania i opróżniania ramek Flow Hive
 * z wykorzystaniem serwomechanizmu i czujnika przepływu miodu.
 * 
 * Sprzęt:
 * - Mikrokontroler: Arduino Nano / ESP32 / Raspberry Pi Pico
 * - Serwomechanizm: SG90 lub MG996R (do obracania klucza ramki)
 * - Czujnik przepływu: optyczny czujnik kropli lub czujnik wagowy
 * - Wyświetlacz: OLED 0.96" I2C (opcjonalnie)
 * - Zasilanie: 5V USB lub bateria Li-Ion z przetwornicą
 * 
 * Autor: ApiaryGuard System
 * Wersja: 1.0.0
 * Data: 2024
 */

#include <Servo.h>

// Definicje pinów
#define SERVO_PIN 9           // Pin sterujący serwomechanizmem
#define FLOW_SENSOR_PIN 2     // Pin czujnika przepływu (interrupt)
#define LED_STATUS_PIN 13     // Dioda statusu
#define BUTTON_MANUAL_PIN 3   // Przycisk ręcznego uruchomienia

// Konfiguracja systemu
#define SERVO_CLOSED_POSITION 0     // Pozycja zamknięta (0 stopni)
#define SERVO_OPEN_POSITION 90      // Pozycja otwarta (90 stopni)
#define SERVO_DELAY_MS 500          // Opóźnienie ruchu serwa
#define FLOW_TIMEOUT_MS 30000       // Timeout braku przepływu (30s)
#define MIN_FLOW_INTERVAL_MS 2000   // Minimalny czas między wykryciami przepływu
#define DEBOUNCE_DELAY_MS 50        // Opóźnienie debouncingu przycisku

// Zmienne globalne
Servo hiveServo;
volatile unsigned long flowCount = 0;
unsigned long lastFlowTime = 0;
unsigned long lastInterruptTime = 0;
bool isDraining = false;
bool systemActive = true;
unsigned long drainStartTime = 0;

// Flaga przerwania dla czujnika przepływu
void flowInterrupt() {
  unsigned long currentTime = millis();
  
  // Debounce dla czujnika przepływu
  if (currentTime - lastInterruptTime > DEBOUNCE_DELAY_MS) {
    flowCount++;
    lastFlowTime = currentTime;
    lastInterruptTime = currentTime;
  }
}

// Inicjalizacja systemu
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Czekaj na połączenie szeregowe
  }
  
  Serial.println(F("=== Flowing Hive Auto Drain System ==="));
  Serial.println(F("Inicjalizacja systemu..."));
  
  // Konfiguracja pinów
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(BUTTON_MANUAL_PIN, INPUT_PULLUP);
  
  // Podłączenie przerwania dla czujnika przepływu
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowInterrupt, FALLING);
  
  // Inicjalizacja serwomechanizmu
  hiveServo.attach(SERVO_PIN);
  hiveServo.write(SERVO_CLOSED_POSITION);
  delay(1000);
  
  // Sygnalizacja startu
  blinkLED(3, 200);
  
  Serial.println(F("System gotowy. Nasłuchiwanie przepływu..."));
  Serial.println(F("Naciśnij przycisk aby uruchomić ręczne opróżnianie."));
}

// Główna pętla programu
void loop() {
  static unsigned long lastCheckTime = 0;
  unsigned long currentTime = millis();
  
  // Obsługa przycisku manualnego
  if (digitalRead(BUTTON_MANUAL_PIN) == LOW) {
    delay(DEBOUNCE_DELAY_MS);
    if (digitalRead(BUTTON_MANUAL_PIN) == LOW && !isDraining) {
      Serial.println(F(">>> Ręczne uruchomienie opróżniania"));
      startDraining();
    }
  }
  
  // Automatyczne wykrywanie przepływu miodu
  if (!isDraining && systemActive) {
    if (detectHoneyFlow()) {
      Serial.println(F(">>> Wykryto przepływ miodu!"));
      startDraining();
    }
  }
  
  // Monitorowanie procesu opróżniania
  if (isDraining) {
    monitorDrainingProcess(currentTime);
  }
  
  // Aktualizacja co 100ms
  if (currentTime - lastCheckTime >= 100) {
    lastCheckTime = currentTime;
    updateStatusLED();
  }
}

// Wykrywanie przepływu miodu
bool detectHoneyFlow() {
  static unsigned long lastDetectionTime = 0;
  unsigned long currentTime = millis();
  
  // Sprawdź czy minął wystarczający czas od ostatniego wykrycia
  if (currentTime - lastDetectionTime < MIN_FLOW_INTERVAL_MS) {
    return false;
  }
  
  // Sprawdź aktywność czujnika w oknie czasowym
  const unsigned long detectionWindow = 5000; // 5 sekund
  static unsigned long windowStart = 0;
  static unsigned int windowCount = 0;
  
  if (currentTime - windowStart >= detectionWindow) {
    // Nowe okno detekcji
    if (windowCount >= 3) { // Minimum 3 wykrycia w 5 sekund
      lastDetectionTime = currentTime;
      windowCount = 0;
      windowStart = currentTime;
      return true;
    }
    windowCount = 0;
    windowStart = currentTime;
  }
  
  // Zliczaj wykrycia w bieżącym oknie (uproszczone)
  noInterrupts();
  unsigned long currentCount = flowCount;
  interrupts();
  
  if (currentCount > windowCount) {
    windowCount = currentCount;
  }
  
  return false;
}

// Rozpoczęcie procesu opróżniania
void startDraining() {
  isDraining = true;
  drainStartTime = millis();
  
  Serial.println(F("--- ROZPOCZĘCIE OPRÓŻNIANIA ---"));
  Serial.print(F("Czas startu: "));
  Serial.println(drainStartTime);
  
  // Otwórz ramkę
  hiveServo.write(SERVO_OPEN_POSITION);
  delay(SERVO_DELAY_MS);
  
  digitalWrite(LED_STATUS_PIN, HIGH);
  Serial.println(F("Serwo: POZYCJA OTWARTA"));
}

// Monitorowanie procesu opróżniania
void monitorDrainingProcess(unsigned long currentTime) {
  // Sprawdź timeout braku przepływu
  if (currentTime - lastFlowTime > FLOW_TIMEOUT_MS) {
    Serial.println(F(">>> Brak przepływu przez 30s - kończenie opróżniania"));
    stopDraining();
    return;
  }
  
  // Sprawdź maksymalny czas opróżniania (zabezpieczenie)
  const unsigned long maxDrainTime = 300000; // 5 minut
  if (currentTime - drainStartTime > maxDrainTime) {
    Serial.println(F(">>> Maksymalny czas opróżniania osiągnięty"));
    stopDraining();
    return;
  }
  
  // Raportowanie postępu
  static unsigned long lastReportTime = 0;
  if (currentTime - lastReportTime >= 10000) { // Co 10 sekund
    lastReportTime = currentTime;
    
    noInterrupts();
    unsigned long currentCount = flowCount;
    interrupts();
    
    Serial.print(F("Postęp: "));
    Serial.print(currentTime - drainStartTime);
    Serial.print(F("ms, wykryć: "));
    Serial.println(currentCount);
  }
}

// Zakończenie procesu opróżniania
void stopDraining() {
  isDraining = false;
  
  Serial.println(F("--- KONIEC OPRÓŻNIANIA ---"));
  
  // Zamknij ramkę
  hiveServo.write(SERVO_CLOSED_POSITION);
  delay(SERVO_DELAY_MS);
  
  digitalWrite(LED_STATUS_PIN, LOW);
  Serial.println(F("Serwo: POZYCJA ZAMKNIĘTA"));
  
  // Reset licznika
  noInterrupts();
  flowCount = 0;
  interrupts();
  
  Serial.println(F("System w trybie nasłuchiwania..."));
}

// Aktualizacja diody statusu
void updateStatusLED() {
  static bool ledState = false;
  static unsigned long lastToggleTime = 0;
  
  if (isDraining) {
    // Szybkie miganie podczas opróżniania
    if (millis() - lastToggleTime >= 500) {
      ledState = !ledState;
      digitalWrite(LED_STATUS_PIN, ledState);
      lastToggleTime = millis();
    }
  } else {
    // Wolne miganie w trybie gotowości
    if (millis() - lastToggleTime >= 2000) {
      ledState = !ledState;
      digitalWrite(LED_STATUS_PIN, ledState);
      lastToggleTime = millis();
    }
  }
}

// Funkcja pomocnicza - miganie LED
void blinkLED(int count, int interval) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(interval);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(interval);
  }
}
