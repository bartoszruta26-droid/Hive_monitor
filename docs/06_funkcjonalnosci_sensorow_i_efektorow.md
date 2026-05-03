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
- **256-point FFT**: Rozdzielczość częstotliwościowa ~31.25 Hz @ 8kHz (pełna implementacja Cooley-Tukey z liczbami zespolonymi)
- **Spectrogram Generation**: Wizualizacja czasowo-częstotliwościowa
- **47 parametrów akustycznych**: Kompleksowa analiza metryk czasowych, częstotliwościowych, statystycznych i psychoakustycznych

**Parametry Akustyczne (AudioMetrics - 47 metryk):**

**A. Parametry Czasowe i Amplitudowe:**
- `rms_amplitude` - Root Mean Square (energia sygnału)
- `peak_amplitude` - Maksymalna amplituda szczytowa
- `peak_to_peak` - Różnica między max a min
- `zero_crossing_rate` - Liczba przejść przez zero (aktywność)
- `signal_energy` - Całkowita energia sygnału
- `crest_factor` - Stosunek peak/RMS (wskaźnik impulsowości)
- `average_amplitude` - Średnia amplituda bezwzględna

**B. Parametry Statystyczne:**
- `mean_value` - Średnia wartość sygnału
- `std_deviation` - Odchylenie standardowe
- `skewness` - Miara asymetrii rozkładu
- `kurtosis` - Miara "spiczastości" rozkładu
- `coefficient_of_variation` - CV (std_dev/mean)
- `dynamic_range` - Zakres dynamiczny w dB

**C. Parametry Częstotliwościowe:**
- `dominant_frequency` - Dominująca częstotliwość (Hz)
- `spectral_centroid` - Centrum ciężkości widma (Hz)
- `spectral_bandwidth` - Szerokość pasma (Hz)
- `spectral_flatness` - Płaskość widma (0-tonalny, 1-szum)
- `spectral_rolloff` - Częstotliwość odcięcia 85% energii
- `spectral_entropy` - Entropia widmowa (miara chaosu)
- `harmonic_to_noise_ratio` - Stosunek harmonicznych do szumu
- `autocorrelation_peak` - Szczyt autokorelacji (periodyczność)

**D. Moc w Pasmach Częstotliwości:**
- `power_low_freq` - Moc w paśmie 0-80 Hz (niskie wibracje)
- `power_bee_band` - Moc w paśmie pszczelim 80-800 Hz
- `power_swarm_band` - Moc w paśmie rojowym 150-350 Hz
- `power_mid_freq` - Moc w paśmie 800-2000 Hz
- `power_high_freq` - Moc w paśmie >2000 Hz

**E. Wskaźniki Klasyfikacji:**
- `bee_activity_index` - Indeks aktywności pszczół (0-100%)
- `swarm_probability` - Prawdopodobieństwo rojenia (0-100%)
- `stress_indicator` - Wskaźnik stresu rodziny (0-100%)
- `hive_health_audio` - Indeks zdrowia na podstawie audio (0-100%)

**F. Parametry Formantów i Jakości Dźwięku:**
- `formant_f1`, `formant_f2`, `formant_f3` - Trzy pierwsze formanty
- `brightness` - Jasność dźwięku (energia wysokich tonów)
- `roughness` - Chropowatość dźwięku
- `sharpness` - Ostrość dźwięku
- `tonality` - Tonacyjność (0-szum, 1-czysty ton)
- `prominence_ratio` - Stosunek wyróżnienia tonalnego

**G. Cechy Temporalne:**
- `attack_time` - Czas narastania sygnału (ms)
- `decay_time` - Czas wybrzmiewania (ms)
- `temporal_centroid` - Centrum czasowe energii (ms)
- `silence_ratio` - Udział ciszy w sygnale (0-1)
- `modulation_index` - Indeks modulacji amplitudy

**H. Parametry Psychoakustyczne:**
- `loudness` - Głośność perceived (sones)
- `roughness_fast` - Szybkie fluktuacje (<70Hz)
- `spectral_decrease` - Spadek spektralny (ciemność dźwięku)
- `irregularity` - Niereegularność widmowa

**Signature Sounds:**
- **Piping (200-300 Hz)**: Queen pipes - sygnał przedrojowy
- **Quacking (150-250 Hz)**: Młode matki w matecznikach
- **Tooting (400-500 Hz)**: Nowa matka po wyjściu
- **Agitation Buzz (800-1200 Hz)**: Agresja, brak матки
- **Varroa Mite Sounds**: Specyficzne kliknięcia (badania w toku)
- **Normal Activity (80-400 Hz)**: Spokojna praca pszczół
- **Foraging Return (200-600 Hz)**: Powrót z pożytku
- **Distress Signals (>1000 Hz)**: Sygnały zagrożenia

**Klasyfikacja Zdarzeń Akustycznych (9 typów):**
1. **NORMAL_ACTIVITY** - Normalna aktywność (pozytywny wpływ)
2. **INCREASED_ACTIVITY** - Zwiększona aktywność (pozytywny - dobry pożytek)
3. **PRE_SWARMING** - Przygotowania do rojenia (negatywny)
4. **ACTIVE_SWARMING** - Aktywne rojenie (krytyczny)
5. **QUEEN_PIPING** - Piszczenie królowej (pozytywny)
6. **PREDATOR_THREAT** - Zagrożenie drapieżnikiem (krytyczny)
7. **DISTRESS_SIGNALS** - Sygnały distress (krytyczny)
8. **LOW_ACTIVITY** - Niska aktywność (negatywny)
9. **UNKNOWN** - Nieznany wzorzec (neutralny)

**Machine Learning Pipeline:**
1. Ekstrakcja cech: 47 parametrów AudioMetrics + MFCC (Mel-Frequency Cepstral Coefficients)
2. Feature vector: 47 metryk + 13 MFCC + delta + delta-delta = 65+ cech
3. Klasyfikator: Random Forest / Neural Network (planowane)
4. Output: Prawdopodobieństwo rojenia (0-100%) + klasyfikacja zdarzenia

**Bufor Historii:**
- 60-punktowa historia danych audio (1 minuta przy 1Hz sampling)
- Trendy temporalne dla wszystkich kluczowych metryk
- Detekcja zmian i anomalii w czasie rzeczywistym

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

