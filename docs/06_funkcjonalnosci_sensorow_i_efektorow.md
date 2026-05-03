## 🎛️ Funkcjonalności Sensorów i Efektorów

### Tabela Sensorów

| Sensor | Typ | Zakres Pomiarowy | Precyzja | Częstotliwość Samplingu | Zastosowanie |
|--------|-----|------------------|----------|------------------------|--------------|
| **HX711 + Strain Gauge** | Waga tensometryczna | 0-200 kg | ±5 g | 10 Hz | Monitoring zapasów miodu, detekcja rojenia, zdrowie rodziny |
| **MEMS Microphone** | Akustyka | 20 Hz - 20 kHz | ±2 dB | 8 kHz | Analiza brzmienia, detekcja rojenia, choroby |
| **DHT22/AM2302** | Temp + Wilgotność | -40÷80°C, 0-100% RH | ±0.5°C, ±2% | 0.5 Hz | Klimat gniazda, kondensacja, wentylacja |
| **Piezo Transducer** | Wibracje | 1 Hz - 10 kHz | ±5% | 100 Hz | Aktywność na wylotku, drapieżniki, ruch |
| **MH-Z19B** | CO₂ | 0-5000 ppm | ±50 ppm | 0.2 Hz | Wentylacja, metabolizm rodziny |
| **BH1750** | Natężenie światła | 1-65535 lux | ±20% | 1 Hz | Cykl dobowy, detekcja otwarcia |
| **BMP280** | Ciśnienie + Temp | 300-1100 hPa | ±1 hPa | 1 Hz | Prognoza pogody, korekta wysokościowa |
| **SGP30** | Jakość powietrza | eCO₂ 400-60000 ppm | ±10% | 1 Hz | TVOC, jakość powietrza w ulu |
| **Reed Switch** | Magnetyczny | Binary (ON/OFF) | N/A | Event-driven | Anti-theft, detekcja otwarcia |
| **NEO-6M GPS** | Lokalizacja | Globalny | ±2.5 m | 1 Hz | Tracking pasieki, anti-theft |
| **Capacitive Soil** | Wilgotność gleby | 0-100% | ±3% | 0.1 Hz | Warunki podłoża, drenaż |

### Szczegółowe Opisy Funkcjonalności Sensorów

#### 1. System Wagowy (HX711 + Strain Gauge)

**Zasada Działania:**
System wykorzystuje cztery tensometry rezystancyjne połączone w mostek Wheatstone'a, zamontowane pod podstawą ula. Sygnał analogiczny jest wzmacniany i konwertowany przez 24-bitowy ADC HX711.

**Algorytmy Przetwarzania:**
- **Moving Average Filter**: Eliminacja szumu wysokoczęstotliwościowego
- **Temperature Compensation**: Korekta dryfu termicznego tensometrów
- **Auto-Tare**: Automatyczne zerowanie przy instalacji
- **Outlier Detection**: Odrzucanie anomaliowych odczytów (np. wiatr, ptaki)

**Wykrywane Zdarzenia:**
- **Spadek wagi >2kg w 24h**: Potencjalne rojenie
- **Stopniowy wzrost wagi**: Intensywne zbieranie nektaru
- **Nagły spadek >5kg**: Kradzież ula lub katastrofa
- **Waga <5kg**: Krytycznie niskie zapasy (głód)
- **Cykliczne zmiany dobowe**: Normalna aktywność zbieracka

**Kalibracja:**
```bash
# Procedura kalibracji
./calibrate_scale.sh --known-weight 10.0 --iterations 100
# Wynik: współczynnik kalibracyjny zapisany w EEPROM
```

#### 2. Analiza Akustyczna (Microphone + FFT)

**Przetwarzanie Sygnału:**
- **Pre-emphasis Filter**: Podbicie wysokich częstotliwości
- **Hamming Window**: Redukcja przecieków widmowych
- **512-point FFT**: Rozdzielczość częstotliwościowa ~15.6 Hz @ 8kHz
- **Spectrogram Generation**: Wizualizacja czasowo-częstotliwościowa

**Signature Sounds:**
- **Piping (200-300 Hz)**: Queen pipes - sygnał przedrojowy
- **Quacking (150-250 Hz)**: Młode matki w matecznikach
- **Tooting (400-500 Hz)**: Nowa matka po wyjściu
- **Agitation Buzz (800-1200 Hz)**: Agresja, brak матки
- **Varroa Mite Sounds**: Specyficzne kliknięcia (badania w toku)

**Machine Learning Pipeline:**
1. Ekstrakcja cech: MFCC (Mel-Frequency Cepstral Coefficients)
2. Feature vector: 13 MFCC + delta + delta-delta
3. Klasyfikator: Random Forest / Neural Network
4. Output: Prawdopodobieństwo rojenia (0-100%)

#### 3. Monitoring Środowiskowy (Temp/Humidity/CO₂)

**Termoregulacja Ula:**
- **Idealna temperatura czerwiu**: 34-35°C
- **Alert overheating**: >36°C (ryzyko topnienia plastrów)
- **Alert chilling**: <32°C (zaburzenia rozwoju czerwiu)
- **Gradient temperatur**: Różnica centrum-peryferie >5°C = problem

**Wilgotność:**
- **Optymalna**: 50-60% RH
- **Zbyt wysoka >75%**: Ryzyko pleśni, grzybic (Ascosphaera apis)
- **Zbyt niska <30%**: Wysychanie nektaru, stres rodziny
- **Kondensacja**: Detekcja przez nagły wzrost RH przy spadku temp

**CO₂ Monitoring:**
- **Poziom bazowy**: 400-600 ppm
- **Podwyższony metabolizm**: 800-1500 ppm (intensywna praca)
- **Słaba wentylacja**: >2000 ppm (ryzyko zaczadzenia)
- **Trigger wentylatora**: >1500 ppm przez 30 min

### Tabela Efektorów

| Efektor | Typ | Moc/Zakres | Sterowanie | Zastosowanie |
|---------|-----|------------|------------|--------------|
| **Heater 10W** | Grzałka silikonowa | 10W @ 12V | PWM + PID | Termoterapia, wsparcie zimowania |
| **Axial Fan** | Wentylator 12V | 20 CFM | PWM 0-100% | Chłodzenie, wentylacja, dystrybucja |
| **Peristaltic Pump** | Dozownik leków | 0.1-50 ml | Stepper motor | Dawki kwasów, olejków, syropu |
| **Solenoid Valve** | Zawór NO/NC | 12V, 2L/min | Digital GPIO | Kontrola wylotka, izolacja |
| **Relay Module** | Przekaźniki 8x | 10A @ 250V | Digital GPIO | Przełączanie urządzeń high-power |
| **LED Strip** | Oświetlenie RGB | 5V, adresowalne | WS2812 | Sygnalizacja statusu, night service |

### Scenariusze Automatyzacji

#### Scenariusz 1: Wykrycie Rojenia i Interwencja

```
WARUNKI WYZWALAJĄCE:
├─ Spadek wagi >2kg w ciągu 4 godzin
├─ Wykrycie piping sounds (FFT analysis)
├─ Zwiększona aktywność na wylotku (piezo sensor)
└─ Wzrost temperatury powyżej 35.5°C

AKCJE AUTOMATYCZNE:
1. Natychmiastowe powiadomienie pszczelarza (SMS + Push)
2. Nagranie 60-sekundowego clipu audio do analizy
3. Zwiększenie częstotliwości samplingu do 1 Hz
4. Opcjonalne: Zamknięcie wylotka na 15 minut (zawór)
5. Uruchomienie kamery (jeśli dostępna)
6. Zapis zdarzenia do bazy z tagiem "SWARM_EVENT"

AKCJE PSZCZELARZA:
- Sprawdzenie powiadomienia w aplikacji
- Odsłuch nagrania audio
- Wizyta w pasiece w ciągu 24h
- Założenie nowej rójki lub połączenie rodzin
```

#### Scenariusz 2: Termoterapia Warrozy

```
WARUNKI WYZWALAJĄCE:
├─ Wykrycie vysokiego poziomu Varroa (analiza audio/wizyjna)
├─ Sezon: sierpień-wrzesień
└─ Temperatura zewnętrzna <15°C

PROTOKÓŁ TERMOTERAPII:
1. Podgrzewanie ula do 40°C (stopniowo, 0.5°C/min)
2. Utrzymanie 40°C przez 24 godziny
3. Monitorowanie temperatury co 10 sekund (PID control)
4. Awaryjne chłodzenie jeśli T > 42°C
5. Zapewnienie wentylacji (fan 30%)
6. Powiadomienie o zakończeniu terapii

BEZPIECZEŃSTWO:
- Thermal fuse fizyczny @ 45°C
- Software limit @ 43°C
- Watchdog timer resetujący sterowanie
- Backup battery podtrzymująca monitoring
```

#### Scenariusz 3: Automatyczne Dokarmianie Awaryjne

```
WARUNKI WYZWALAJĄCE:
├─ Waga ula <8kg (brak zapasów)
├─ Okres: późna jesień/zima/wczesna wiosna
└─ Brak lotów pszczół (temp zewnętrzna <10°C)

PROTOKÓŁ DOKARMIANIA:
1. Sprawdzenie poziomu syropu w zbiorniku
2. Uruchomienie pompy perystaltycznej
3. Dozowanie 200ml syropu 2:1 (cukier:woda)
4. Powtórzenie codziennie o świcie
5. Monitorowanie przyrostu wagi
6. Zatrzymanie gdy waga >15kg

MONITOROWANIE:
- Codzienny raport zużycia syropu
- Alert jeśli zbiornik pusty (<10%)
- Korelacja z temperaturą zewnętrzną
```

---

