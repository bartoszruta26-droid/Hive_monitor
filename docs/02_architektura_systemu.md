## 🏗️ Architektura Systemu

### Diagram Architektury

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         POZIOM PASIEKI (CLOUD/EDGE)                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐               │
│  │   Dashboard  │    │  Analityka   │    │   Powiadom.  │               │
│  │   Web/Mobile │    │   Danych     │    │   SMS/Email  │               │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘               │
│         └───────────────────┼───────────────────┘                        │
│                             │ API REST                                   │
└─────────────────────────────┼────────────────────────────────────────────┘
                              │
                    ┌─────────▼─────────┐
                    │   Apache Server   │
                    │  (Raspberry Pi 2) │
                    │  - HTTP/HTTPS     │
                    │  - MQTT Broker    │
                    │  - Local Database │
                    └─────────┬─────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
┌───────▼───────┐   ┌────────▼────────┐   ┌───────▼───────┐
│   Moduł LTE   │   │   Ethernet CAP  │   │   Zasilanie   │
│   (Aero2 SIM) │   │   (PoE Splitter)│   │     PoE       │
│   USB Dongle  │   │   5V/2.4A       │   │   802.3af     │
└───────────────┘   └─────────────────┘   └───────────────┘
                              │
                    ┌─────────▼─────────┐
                    │   Arduino Nano    │
                    │   (Slave Device)  │
                    │   - Sensor Hub    │
                    │   - Actuator Ctrl │
                    └─────────┬─────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
┌───────▼───────┐   ┌────────▼────────┐   ┌───────▼───────┐
│   Sensory     │   │   Efektory      │   │   Komunikacja │
│   - HX711     │   │   - Heater 10W  │   │   - I2C       │
│   - Mic       │   │   - Fan         │   │   - SPI       │
│   - DHT22     │   │   - Dispenser   │   │   - UART      │
│   - Piezo     │   │   - Valves      │   │   - GPIO      │
│   - Strain    │   │   - Relays      │   │               │
└───────────────┘   └─────────────────┘   └───────────────┘
```

### Warstwy Systemu

#### Warstwa 1: Sensoryczna (Field Layer)
- Bezpośredni kontakt z environmentem ula
- Analogowe i cyfrowe czujniki
- Konwersja sygnałów (HX711 dla tensometrów)
- Odporność na wilgotność, temperaturę, wibracje

#### Warstwa 2: Sterowania (Control Layer)
- Arduino Nano jako lokalny kontroler
- Real-time processing sygnałów
- PWM sterowanie efektorem
- Watchdog i safe-mode

#### Warstwa 3: Agregacji (Gateway Layer)
- Raspberry Pi 2 jako bramka
- Apache2 server z bazą danych
- Komunikacja LTE z chmurą
- Local caching i offline operation

#### Warstwa 4: Aplikacyjna (Application Layer)
- Web dashboard
- Mobile applications
- API dla integracji zewnętrznych
- Machine Learning analytics

---

