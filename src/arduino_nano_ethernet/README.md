# ApiaryGuard - Arduino Nano + Ethernet Shield

## Wersja: RAW DATA ONLY (Tylko Surowe Dane)

**WSZYSTKIE OBLICZENIA PARAMETRÓW WYKONYWANE SĄ W RASPBERRY PI**  
Arduino Nano wysyła wyłącznie surowe odczyty z sensorów.

### Obsługiwane sensory (RAW data):
- **DHT22**: temperatura, wilgotność
- **HX711**: waga (surowe ADC)
- **MEMS Microphone**: poziom dźwięku (RMS)
- **Piezo**: wibracje
- **SGP30/SGP41** (opcjonalnie): CO2, VOC przez I2C

### Efektory:
- 3x PWM: grzałka (D5), wentylator (D6), pompa (D9)
- 2x przekaźnik: zawór 1 (D7), zawór 2 (D8)

### Połączenia pinów Arduino Nano:
| Sensor/Efektor | Pin Arduino |
|----------------|-------------|
| Ethernet W5100 CS | D10 |
| Ethernet MOSI | D11 |
| Ethernet MISO | D12 |
| Ethernet SCK | D13 |
| HX711 DT | D2 |
| HX711 SCK | D3 |
| DHT22 DATA | D4 |
| HEATER PWM | D5 |
| FAN PWM | D6 |
| RELAY 1 | D7 |
| RELAY 2 | D8 |
| PUMP PWM | D9 |
| MIC (Audio) | A0 |
| PIEZO | A1 |
| SGP30 SDA | A4 |
| SGP30 SCL | A5 |

### Endpointy HTTP:
- `GET /api/raw` - surowe dane ze wszystkich sensorów (JSON)
- `GET /status` - alias do /api/raw
- `GET /heater/on`, `/heater/off` - sterowanie grzałką
- `GET /fan/on`, `/fan/off` - sterowanie wentylatorem
- `GET /pump/on`, `/pump/off` - sterowanie pompą
- `GET /api/cmd?SET_HEATER:128` - ustaw PWM grzałki (0-255)
- `GET /api/cmd?SET_FAN:200` - ustaw PWM wentylatora
- `GET /api/cmd?SET_PUMP:100` - ustaw PWM pompy
- `GET /api/cmd?SET_RELAY1:1` - włącz przekaźnik 1
- `GET /api/cmd?SET_RELAY2:0` - wyłącz przekaźnik 2
- `GET /api/cmd?CALIB_WEIGHT` - kalibracja wagi (zerowanie)

### Przykładowy JSON z /api/raw:
```json
{
  "temp_raw": 23.5,
  "hum_raw": 55.2,
  "weight_raw": 12345,
  "audio_raw": 45,
  "vibration_raw": 512,
  "co2_raw": 450,
  "voc_raw": 85,
  "heater_pwm": 0,
  "fan_pwm": 128,
  "pump_pwm": 0,
  "relay1": 0,
  "relay2": 1,
  "timestamp": 12345678,
  "free_ram": 856
}
```

### Wymagane biblioteki Arduino:
- Ethernet (wbudowana)
- DHT sensor library by Adafruit
- ArduinoJson by Benoit Blanchon (v6 lub v7)

### Uwagi:
- Kod zoptymalizowany pod kątem 2KB RAM Arduino Nano
- Brak obliczeń jakości powietrza - tylko surowe dane
- Raspberry Pi odpowiada za wszystkie obliczenia i analizy
- Częstotliwość odczytu sensorów: 1 sekunda
