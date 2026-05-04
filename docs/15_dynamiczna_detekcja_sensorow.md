# Dynamiczna Detekcja Sensorów - ApiaryGuard Pro v2.0

## Przegląd

Wersja 2.0 firmware wprowadza **dynamiczne wykrywanie sensorów** przy starcie systemu Raspberry Pi Pico. System automatycznie skanuje podłączone urządzenia i wylicza parametry **tylko dla aktywnych modułów**, co optymalizuje zużycie zasobów i eliminuje błędy odczytu z niepodłączonych sensorów.

## Jak to działa?

### 1. Faza Startu (setup())

```cpp
void setup() {
  // ... inicjalizacja podstawowa ...
  
  // DYNAMICZNE WYKRYWANIE SENSORÓW
  Serial.println(">> Skanowanie i wykrywanie sensorów...");
  detectAllSensors();
  printSensorStatus();
  
  // Inicjalizacja TYLKO wykrytych sensorów
  if (sensors.tempHum.detected) {
    dht.begin();
  }
  if (sensors.airQual.detected) {
    sgp.begin_I2C(0x59, &Wire);
  }
}
```

### 2. Struktury Danych

```cpp
struct SensorFlags {
    bool detected = false;   // Czy hardware został wykryty
    bool active = false;     // Czy moduł jest włączony do przetwarzania
    int errorCode = 0;       // Kod błędu inicjalizacji
    unsigned long lastRead = 0;
};

struct SystemSensors {
    SensorFlags hx711;       // Waga (HX711)
    SensorFlags audio;       // Mikrofon (MEMS)
    SensorFlags radar;       // Radar MMWave (LD2410B)
    SensorFlags tempHum;     // Temp/Wilg (DHT22/SHT40/BME280)
    SensorFlags airQual;     // Jakość powietrza (SGP41)
    SensorFlags piezo;       // Wibracje (Piezo)
    SensorFlags baro;        // Ciśnienie (BME280)
    SensorFlags light;       // Światło (BH1750/Analog)
} sensors;
```

### 3. Warunkowy Odczyt w loop()

```cpp
void loop() {
  // DHT22/SHT40 - tylko jeśli wykryty
  if (sensors.tempHum.active) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    // ...
  }
  
  // HX711 - tylko jeśli wykryty
  if (sensors.hx711.active) {
    long raw = readHX711();
    // ...
  }
  
  // Audio - tylko jeśli wykryty
  if (sensors.audio.active && (now - lastAudioProcess > 5000)) {
    processAudioSignal();
    // ...
  }
  
  // ... inne sensory ...
}
```

## Metody Detekcji dla Każdego Sensora

| Sensor | Metoda Detekcji | Kryterium Sukcesu |
|--------|----------------|-------------------|
| **HX711** | GPIO toggle test | Pin DT reaguje na zmiany |
| **Audio (MEMS)** | ADC noise floor | Zakres > 50 w 10 próbkach |
| **Radar MMWave** | UART handshake | Odpowiedź na komendę config |
| **Temp/Hum** | DHT22 read + I2C scan | isnan() check lub ACK I2C |
| **Air Quality** | I2C address scan | ACK na adresie 0x59 lub 0x58 |
| **Piezo** | ADC variance test | Zakres > 100 w 20 próbkach |
| **Barometric** | I2C address scan | ACK na adresie 0x76 lub 0x77 |
| **Light** | I2C + ADC fallback | ACK BH1750 lub rozsądny ADC |

## Przykładowy Output z Serial

```
=== ApiaryGuard Pico v2.0 Start ===
>> Skanowanie i wykrywanie sensorów...
>> Wykrywanie sensorów...

=== STATUS SENSORÓW ===
HX711 (Waga)         : OK [AKTYWNY]
Audio (MEMS Mic)     : OK [AKTYWNY]
Radar MMWave LD2410B : NIEWYKRYTY [WYŁĄCZONY]
Temp/Humidity        : OK [AKTYWNY]
Air Quality (SGP41)  : OK [AKTYWNY]
Piezo Vibration      : NIEWYKRYTY [WYŁĄCZONY]
Barometric (BME280)  : OK [AKTYWNY]
Light Sensor         : NIEWYKRYTY [WYŁĄCZONY]
=======================

>> DHT22/SHT40 zainicjalizowany
>> SGP41 zainicjalizowany
>> System gotowy.
```

## Korzyści

1. **Elastyczność sprzętowa** - Możesz podłączyć dowolną kombinację sensorów
2. **Oszczędność zasobów** - CPU nie traci czasu na odczyty z nieistniejących urządzeń
3. **Brak błędów** - System nie zgłasza błędów odczytu z niepodłączonych pinów
4. **Łatwa diagnostyka** - Clearny status sensorów przy starcie
5. **Skalowalność** - Łatwe dodawanie nowych typów sensorów

## Implementacja Funkcji Check

Każdy sensor ma swoją funkcję `checkXXX()` zwracającą `bool`:

```cpp
bool checkHX711() {
  pinMode(HX711_DT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  digitalWrite(HX711_SCK, LOW);
  delayMicroseconds(10);
  
  int val = digitalRead(HX711_DT);
  delayMicroseconds(10);
  int val2 = digitalRead(HX711_DT);
  
  return (val != val2) || (val >= 0);
}

bool checkAirQuality() {
  Wire.beginTransmission(0x59);
  if(Wire.endTransmission() == 0) return true;
  
  Wire.beginTransmission(0x58);
  if(Wire.endTransmission() == 0) return true;
  
  return false;
}
```

## Lokalizacja w Kodzie

- **Struktury danych**: linie ~592-608
- **Deklaracje funkcji**: linie ~611-620
- **Implementacja detectAllSensors()**: linie ~4168-4202
- **Implementacja printSensorStatus()**: linie ~4204-4225
- **Funkcje checkXXX()**: linie ~4227-4344
- **Wywołanie w setup()**: linie ~3758-3760
- **Warunkowy odczyt w loop()**: linie ~3819-3907

## Wymagane Zmiany w Istniejącym Kodzie

Jeśli aktualizujesz starszą wersję:

1. Dodaj struktury `SensorFlags` i `SystemSensors`
2. Dodaj deklaracje funkcji wykrywania
3. Zmień `setup()` aby wywoływało `detectAllSensors()` i `printSensorStatus()`
4. Otocz wszystkie odczyty sensorów w `loop()` warunkami `if (sensors.XXX.active)`
5. Dodaj implementacje funkcji `checkXXX()` na końcu pliku

## Kompatybilność

- **Wsteczna**: Tak - system działa z mniejszą liczbą sensorów
- **Sprzętowa**: Wszystkie sensory I2C, UART, ADC, GPIO
- **Platforma**: Raspberry Pi Pico (RP2040), Pico SDK

## Rozszerzenia w Przyszłości

Możliwe ulepszenia:
- Hot-plug detection (wykrywanie w trakcie pracy)
- Auto-kalibracja po wykryciu
- Konfigurowalne progi detekcji przez API
- Logging zdarzeń podłączenia/odłączenia
