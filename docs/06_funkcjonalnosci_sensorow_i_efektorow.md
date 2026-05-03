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
| **SGP30/SGP41** | Jakość powietrza | eCO₂ 400-60000 ppm | ±10% | 1 Hz | TVOC, jakość powietrza w ulu |
| **Reed Switch** | Magnetyczny | Binary (ON/OFF) | N/A | Event-driven | Anti-theft, detekcja otwarcia |
| **NEO-6M GPS** | Lokalizacja | Globalny | ±2.5 m | 1 Hz | Tracking pasieki, anti-theft |
| **Capacitive Soil** | Wilgotność gleby | 0-100% | ±3% | 0.1 Hz | Warunki podłoża, drenaż |
| **Radar MMWave** | Wykrywanie ruchu | 0.1-10 m | ±2 cm | 1 Hz | Detekcja intruzów, monitoring aktywności |

### Szczegółowe Opisy Funkcjonalności Sensorów

#### 1. System Wagowy (HX711 + Strain Gauge)

**Zasada Działania:**
System wykorzystuje cztery tensometry rezystancyjne połączone w mostek Wheatstone'a, zamontowane pod podstawą ula. Sygnał analogiczny jest wzmacniany i konwertowany przez 24-bitowy ADC HX711 z rozdzielczością ±5g.

**Algorytmy Przetwarzania:**
- **Moving Average Filter**: Eliminacja szumu wysokoczęstotliwościowego
- **EMA (Exponential Moving Average)**: Wygładzanie z większym naciskiem na najnowsze dane
- **Temperature Compensation**: Korekta dryfu termicznego tensometrów
- **Auto-Tare**: Automatyczne zerowanie przy instalacji
- **Outlier Detection**: Odrzucanie anomaliowych odczytów metodą Z-score (np. wiatr, ptaki)
- **Linear Regression**: Analiza trendów w oknach czasowych (1h, 4h, 24h)

**105+ Parametrów HX711Metrics + Radar MMWave - Kompleksowa Analiza Danych:**

**A. Podstawowe Parametry Statystyczne (6):**
- `mean_weight` - Średnia waga [kg]
- `std_weight` - Odchylenie standardowe wagi [kg]
- `min_weight`, `max_weight` - Ekstrema wagi [kg]
- `range_weight` - Zakres zmian (max-min) [kg]
- `median_weight` - Mediana wagi [kg]

**B. Zaawansowane Parametry Statystyczne (6):**
- `weight_variance` - Wariancja wagi [kg²]
- `weight_cv` - Współczynnik zmienności (CV = std/mean)
- `weight_iqr` - Rozstęp międzykwartylowy [kg]
- `weight_skewness` - Współczynnik skośności rozkładu
- `weight_kurtosis` - Współczynnik kurtozy (nadmiarowej)
- `weight_gini` - Współczynnik Giniego (nierównomierność)

**C. Parametry Temporalne - Szybkości Zmian (8):**
- `current_rate` - Aktualna szybkość zmiany [kg/h]
- `mean_rate` - Średnia szybkość zmiany [kg/h]
- `max_rate_positive` - Maksymalny przyrost [kg/h]
- `max_rate_negative` - Maksymalny ubytek [kg/h]
- `acceleration` - Przyspieszenie zmiany wagi [kg/h²]
- `jerk` - Pochodna przyspieszenia (rate of change of acceleration) [kg/h³]
- `rate_variance` - Wariancja szybkości zmian
- `rate_entropy` - Entropia szybkości zmian (miara losowości)

**D. Parametry Kierunku Trendu (9):**
- `trend_slope_1h`, `trend_slope_4h`, `trend_slope_24h` - Nachylenia trendów [kg/h]
- `trend_correlation` - Współczynnik korelacji Pearsona (-1 do 1)
- `trend_direction` - Kierunek: -1 (spadek), 0 (stabilny), 1 (wzrost)
- `trend_strength` - Siła trendu (0-1, od |correlation|)
- `trend_persistence` - Persystencja trendu (czy utrzymuje się w czasie)
- `trend_change_points` - Liczba punktów zwrotnych trendu

**E. Parametry Pożytku i Zbiorów (10):**
- `nectar_inflow_rate` - Przepływ nektaru [kg/h]
- `nectar_accumulation` - Skumulowany nektar [kg]
- `foraging_efficiency` - Efektywność zbierania (0-100%)
- `honey_production_idx` - Indeks produkcji miodu (0-100%)
- `bloom_intensity` - Intensywność kwitnienia (0-100%)
- `nectar_quality_est` - Szacowana jakość nektaru (0-100%)
- `nectar_flow_duration` - Czas trwania przepływu [h]
- `foraging_window_start` - Godzina rozpoczęcia wylotów
- `foraging_window_end` - Godzina zakończenia wylotów

**F. Parametry Konsumpcji i Zużycia (7):**
- `consumption_rate` - Zużycie zapasów [kg/h]
- `daily_consumption` - Dzienne zużycie [kg/dzień]
- `food_reserve_days` - Zapas żywności na dni
- `winter_readiness` - Gotowość do zimowli (0-100%)
- `starvation_risk` - Ryzyko głodu (0-100%)
- `metabolic_rate` - Szacowane tempo metabolizmu [kg/h]
- `consumption_regularity` - Regularność zużycia (0-1)

**G. Parametry Cykliczności i Wzorców (9):**
- `daily_amplitude` - Amplituda dobowa [kg]
- `daily_phase` - Faza dobowa [godziny]
- `circadian_strength` - Siła rytmu dobowego (0-1)
- `weekly_pattern_match` - Dopasowanie wzorca tygodniowego (0-1)
- `seasonal_trend` - Trend sezonowy (-1 do 1)
- `harmonic_content` - Zawartość harmonicznych w sygnale
- `cycle_regularity` - Regularność cykli (0-1)
- `phase_coherence` - Koherencja fazowa (0-1)

**H. Parametry Jakości Sygnału (9):**
- `signal_quality` - Jakość sygnału wagi (0-100%)
- `noise_level` - Poziom szumu [kg]
- `drift_rate` - Dryft czujnika [kg/h]
- `stability_index` - Indeks stabilności (0-100%)
- `measurement_confidence` - Pewność pomiaru (0-1)
- `snr` - Stosunek sygnału do szumu [dB]
- `thd` - Total Harmonic Distortion (%)
- `baseline_stability` - Stabilność linii bazowej (0-1)

**I. Parametry Anomalii i Zdarzeń (8):**
- `anomaly_score` - Wynik anomalii (0-1, Z-score normalized)
- `sudden_change_mag` - Wielkość nagłej zmiany [kg]
- `oscillation_freq` - Częstotliwość oscylacji [cykle/dzień]
- `oscillation_damping` - Tłumienie oscylacji (0-1)
- `outlier_ratio` - Stosunek wartości odstających (%)
- `change_point_prob` - Prawdopodobieństwo punktu zwrotnego
- `volatility_index` - Indeks zmienności (0-100%)

**J. Wskaźniki Zdrowia Kolonii (9):**
- `colony_growth_rate` - Tempo wzrostu kolonii [%/dzień]
- `brood_activity_idx` - Indeks aktywności czerwiu (0-100%)
- `population_estimate` - Szacowana populacja [tysiące pszczół]
- `hive_health_weight` - Indeks zdrowia z wagi (0-100%)
- `productivity_score` - Ogólny wynik produktywności (0-100%)
- `stress_indicator` - Indikator stresu kolonii (0-1)
- `vitality_index` - Indeks vitalności (0-100%)
- `resilience_score` - Zdolność do regeneracji (0-1)

**K. Parametry Prognozy (5):**
- `predicted_weight_24h` - Prognoza wagi za 24h [kg]
- `forecast_confidence` - Pewność prognozy (0-1)
- `expected_honey_yield` - Oczekiwany zbiór miodu [kg]
- `prediction_interval` - Przedział ufności prognozy [kg]
- `forecast_trend` - Kierunek prognozy (-1, 0, 1)

**L. Dodatkowe Parametry Radar MMWave (27):**
- `mean_distance` - Średnia odległość obiektów [m]
- `std_distance` - Odchylenie standardowe odległości [m]
- `min_distance`, `max_distance` - Ekstrema odległości [m]
- `range_distance` - Zakres odległości (max-min) [m]
- `mean_energy` - Średnia energia sygnału [AU]
- `std_energy` - Odchylenie standardowe energii [AU]
- `min_energy`, `max_energy` - Ekstrema energii [AU]
- `energy_variance` - Wariancja energii [AU²]
- `energy_cv` - Współczynnik zmienności energii
- `mean_speed` - Średnia prędkość radialna [m/s]
- `std_speed` - Odchylenie standardowe prędkości [m/s]
- `max_speed_abs` - Maksymalna wartość bezwzględna prędkości [m/s]
- `activity_ratio` - Stosunek czasu aktywności do całkowitego [%]
- `idle_time_percent` - Procent czasu bezczynności [%]
- `motion_intensity` - Intensywność ruchu
- `target_rate` - Średnia liczba celów na pomiar
- `max_target_count` - Maksymalna liczba wykrytych celów
- `target_density` - Gęstość celów
- `trend_slope` - Nachylenie trendu energii [AU/s]
- `trend_correlation` - Współczynnik korelacji trendu [-1 do 1]
- `acceleration_rate` - Tempo zmiany aktywności
- `signal_quality` - Jakość sygnału (0-100%)
- `anomaly_score` - Ogólny wynik anomalii (0-1)
- `hive_health_index` - Indeks zdrowia ula (0-100%)
- `power_spectrum_peak` - Dominująca częstotliwość w spektrum mocy
- `zero_crossing_rate` - Częstotliwość przejść przez zero
- `entropy` - Entropia sygnału (miara losowości)

**Wykrywane Zdarzenia:**
- **Spadek wagi >2kg w 24h**: Potencjalne rojenie (alert)
- **Stopniowy wzrost wagi**: Intensywne zbieranie nektaru (pozytywne)
- **Nagły spadek >5kg**: Kradzież ula lub katastrofa (krytyczny alert)
- **Waga <5kg**: Krytycznie niskie zapasy (głód - alert)
- **Cykliczne zmiany dobowe**: Normalna aktywność zbieracka
- **Wysoki stress_indicator**: Stres kolonii (choroba, drapieżniki)
- **Niski vitality_index**: Spadkowa kondycja rodziny

**Kalibracja:**
```bash
# Procedura kalibracji
./calibrate_scale.sh --known-weight 10.0 --iterations 100
# Wynik: współczynnik kalibracyjny zapisany w EEPROM
```

**API Endpointy:**
- `GET /hx711/status` - Aktualny status i wszystkie 105+ parametrów (HX711 + Radar MMWave)
- `GET /hx711/metrics` - Historia metryk
- `GET /hx711/events` - Wykryte zdarzenia
- `GET /hx711/forecast` - Prognoza wagi i zbiorów
- `GET /radar/metrics` - Metryki radaru MMWave
- `GET /radar/status` - Status wykrywania ruchu i intruzów

#### 2. Analiza Akustyczna (Microphone + FFT)

**Przetwarzanie Sygnału:**
- **Pre-emphasis Filter**: Podbicie wysokich częstotliwości
- **Hamming Window**: Redukcja przecieków widmowych
- **256-point FFT**: Rozdzielczość częstotliwościowa ~31.25 Hz @ 8kHz (pełna implementacja Cooley-Tukey z liczbami zespolonymi)
- **Spectrogram Generation**: Wizualizacja czasowo-częstotliwościowa
- **97+ parametrów akustycznych**: Kompleksowa analiza metryk czasowych, częstotliwościowych, statystycznych, psychoakustycznych i zaawansowanych

**Parametry Akustyczne (AudioMetrics - 97+ metryk):**

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
- `irregularity` - Nieregularność widmowa

**I. NOWE Parametry Zaawansowane (dodatkowe 50+ metryk):**
- `spectral_flux` - Strumień spektralny (zmiana widma w czasie)
- `spectral_slope` - Nachylenie widma (balans tonalny)
- `spectral_kurtosis` - Kurtoza widma (ostrość pików)
- `spectral_skewness` - Asymetria widma
- `fundamental_salience` - Wyraźność tonu podstawowego (0-1)
- `partial_energy_ratio` - Stosunek energii harmonicznych do całości
- `odd_harmonic_energy` - Energia nieparzystych harmonicznych
- `even_harmonic_energy` - Energia parzystych harmonicznych
- `tritone_distance` - Dystans od trytonu (miara dysonansu)
- `inharmonicity_deviation` - Odchylenie nieharmoniczności
- `spectral_irregularity` - Nieregularność widmowa
- `log_attack_time` - Logarytmiczny czas ataku
- `temporal_log_attack` - Temporalny log attack
- `effective_duration` - Efektywny czas trwania sygnału
- `rise_time` - Czas narastania (10-90%)
- `decay_rate` - Szybkość zanikania [dB/s]
- `release_time` - Czas wybrzmiewania
- `vibrato_depth` - Głębokość wibrata (detekcja queen piping)
- `vibrato_rate` - Częstotliwość wibrata [Hz]
- `tremolo_depth` - Głębokość tremola
- `tremolo_rate` - Częstotliwość tremola [Hz]
- `spectral_valley_count` - Liczba dolin w widmie
- `peak_prominence` - Wyraźność dominującego piku
- `bandwidth_75` - Szerokość pasma 75% energii
- `bandwidth_95` - Szerokość pasma 95% energii
- `equivalent_sound_level` - Równoważny poziom dźwięku Leq [dB]
- `percentile_level_10` - Poziom L10 [dB]
- `percentile_level_90` - Poziom L90 [dB]
- `noise_floor_estimate` - Szacowane tło szumowe [dB]
- `signal_to_noise_ratio` - SNR [dB]
- `acoustic_complexity` - Złożoność akustyczna (ACI index)
- `bioacoustic_index` - Indeks bioakustyczny (BI)
- `normalized_difference` - Znormalizowany indeks różnicowy (NDI)
- `acoustic_diversity` - Różnorodność akustyczna (ADI)
- `acoustic_evenness` - Równość akustyczna (AEI)
- `power_band_1` do `power_band_8` - Moc w 8 pasmach częstotliwości [dB]
- `formant_freq_1`, `formant_freq_2` - Częstotliwości formantów [Hz]
- `formant_bandwidth_1`, `formant_bandwidth_2` - Szerokości formantów [Hz]
- `fundamental_frequency` - Częstotliwość podstawowa F0 [Hz]
- `pitch_strength` - Siła wysokości dźwięku (0-1)
- `inharmonicity` - Nieharmoniczność sygnału (0-1)
- `shimmer` - Fluktuacja amplitudy (%)
- `jitter` - Fluktuacja częstotliwości (%)
- `noise_to_harmonic_ratio` - NHR
- `harmonic_to_noise_ratio` - HNR
- `sustain_level` - Poziom podtrzymania (0-1)
- `colony_coherence` - Spójność kolonii (0-1)
- `foraging_efficiency_audio` - Efektywność zbierania z audio (0-100%)

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

