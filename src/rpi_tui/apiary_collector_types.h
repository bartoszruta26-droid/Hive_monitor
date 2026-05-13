/**
 * apiary_collector_types.h
 * Struktury danych dla modułu zbierania danych z uli
 * 
 * OBSŁUGIWANE PARAMETRY (WSZYSTKIE Z .md i pico.ino - 338+ parametrów):
 * - Podstawowe (9): temp, humidity, weight, battery, co2, voc, motion, timestamp, online
 * - Audio (97+): rms_amplitude, dominant_frequency, swarm_probability, bee_activity_index, 
 *                spectral_centroid, mfcc_energy, bioacoustic_index, acoustic_diversity, etc.
 * - Radar MMWave (27): distance, energy, activity_ratio, hive_health_index, signal_quality,
 *                      target_rate, entropy, trend_slope, anomaly_score, etc.
 * - HX711 Waga (105+): mean_weight, std_weight, trend_slope_1h/4h/24h, nectar_inflow_rate,
 *                      consumption_rate, colony_growth_rate, productivity_score, forecast, etc.
 * - TempHumidity (28): heat_index, dew_point, comfort_index, brood_stress_index, 
 *                      mold_risk, temp_stability, vpd, etc.
 * - AirQuality (24): iaq_index, ventilation_need, contamination_risk, mold_risk,
 *                    co2_mean, voc_mean, stress_from_air, etc.
 * - PiezoVibration (22): vibration_rms, bee_traffic_score, intrusion_probability,
 *                        predator_detection, queen_piping_detected, etc.
 * - Barometric (18): pressure_mean, weather_trend, storm_probability, 
 *                    foraging_conditions, pressure_change_rate, etc.
 * - Light (17): lux_current, daylight_duration, circadian_sync, foraging_index,
 *               uv_index, photoperiod, etc.
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#ifndef APIARY_COLLECTOR_TYPES_H
#define APIARY_COLLECTOR_TYPES_H

#include <string>

// ============================================================================
// STRUKTURA DANYCH Z ULA - PEŁNA LISTA 338+ PARAMETRÓW Z .md i pico.ino
// ============================================================================
struct HiveData {
    // -------------------------------------------------------------------------
    // METADANE ŹRÓDŁA DANYCH
    // -------------------------------------------------------------------------
    std::string data_source;            // "pico" (precomputed) lub "nano" (raw only)
    bool is_precomputed = false;        // true = dane już wyliczone, false = surowe dane
    
    // -------------------------------------------------------------------------
    // PODSTAWOWE PARAMETRY (9 pól)
    // -------------------------------------------------------------------------
    std::string hive_id;
    float temperature = 0.0f;           // Temperatura [°C]
    float humidity = 0.0f;              // Wilgotność [%RH]
    float weight = 0.0f;                // Waga [kg]
    int battery_level = 0;              // Poziom baterii [%]
    int co2_eq = 0;                     // CO2 equivalent [ppm]
    int voc_idx = 0;                    // VOC index [index]
    int motion_detected = 0;            // Flaga ruchu z radaru (0/1)
    long long timestamp = 0;            // Timestamp [s]
    bool is_online = false;             // Status online/offline
    
    // -------------------------------------------------------------------------
    // SUROWE DANE Z ARDUINO NANO (tylko dla źródła "nano")
    // -------------------------------------------------------------------------
    float temp_raw = 0.0f;              // Surowa temperatura [°C]
    float hum_raw = 0.0f;               // Surowa wilgotność [%]
    long weight_raw = 0;                // Surowa waga [ADC counts]
    int audio_raw = 0;                  // Surowe audio [ADC]
    int vibration_raw = 0;              // Surowe wibracje [ADC]
    int co2_raw = 0;                    // Surowe CO2 [ppm]
    int voc_raw = 0;                    // Surowe VOC [index]
    
    // -------------------------------------------------------------------------
    // PARAMETRY AUDIO (97+ parametrów - pełna lista z audio.ino)
    // -------------------------------------------------------------------------
    // Podstawowe czasowe
    float audio_rms = 0.0f;             // RMS amplituda [V]
    float audio_peak = 0.0f;            // Wartość szczytowa [V]
    float audio_peak_to_peak = 0.0f;    // Wartość międzyszczytowa [V]
    float audio_zcr = 0.0f;             // Zero Crossing Rate [Hz]
    float audio_energy = 0.0f;          // Energia sygnału [V²]
    float audio_mean_amp = 0.0f;        // Średnia amplituda [V]
    float audio_std_amp = 0.0f;         // Odchylenie std amplitudy [V]
    float audio_cv_amp = 0.0f;          // Współczynnik zmienności amplitudy
    float audio_skewness = 0.0f;        // Asymetria rozkładu
    float audio_kurtosis = 0.0f;        // Kurtoza
    // Częstotliwościowe podstawowe
    float audio_dominant_freq = 0.0f;   // Dominująca częstotliwość [Hz]
    float audio_spectral_centroid = 0.0f; // Centrum widma [Hz]
    float audio_spectral_bandwidth = 0.0f; // Szerokość pasma [Hz]
    float audio_spectral_flatness = 0.0f; // Płaskość widma [0-1]
    float audio_spectral_rolloff = 0.0f; // Częstotliwość odcięcia [Hz]
    // Częstotliwościowe szczegółowe
    float audio_power_bee_band = 0.0f;  // Moc w paśmie pszczół 80-800Hz [dB]
    float audio_power_swarm_band = 0.0f;// Moc w paśmie rojenia 150-350Hz [dB]
    float audio_power_low_freq = 0.0f;  // Moc <80Hz [dB]
    float audio_power_high_freq = 0.0f; // Moc >800Hz [dB]
    float audio_harmonic_ratio = 0.0f;  // Stosunek harmonicznych
    // MFCC i zaawansowane
    float audio_mfcc_energy[4] = {0};   // Energia MFCC [4 współczynniki]
    float audio_spectral_entropy = 0.0f;// Entropia widmowa
    float audio_spectral_contrast = 0.0f;// Kontrast widmowy
    float audio_tonal_strength = 0.0f;  // Siła tonalna
    // Nowe parametry audio
    float audio_crest_factor = 0.0f;    // Współczynnik szczytu (peak/rms)
    float audio_formant_f1 = 0.0f;      // Pierwszy formant [Hz]
    float audio_formant_f2 = 0.0f;      // Drugi formant [Hz]
    float audio_fundamental_freq = 0.0f;// Częstotliwość podstawowa F0 [Hz]
    float audio_pitch_strength = 0.0f;  // Siła wysokości [0-1]
    float audio_inharmonicity = 0.0f;   // Nieharmoniczność [0-1]
    float audio_shimmer = 0.0f;         // Fluktuacja amplitudy [%]
    float audio_jitter = 0.0f;          // Fluktuacja częstotliwości [%]
    float audio_nhr = 0.0f;             // Noise to Harmonic Ratio
    float audio_hnr = 0.0f;             // Harmonic to Noise Ratio
    float audio_autocorr_peak = 0.0f;   // Szczyt autokorelacji [0-1]
    float audio_attack_time = 0.0f;     // Czas narastania [ms]
    float audio_decay_time = 0.0f;      // Czas zanikania [ms]
    float audio_sustain_level = 0.0f;   // Poziom podtrzymania [0-1]
    float audio_temporal_centroid = 0.0f;// Centrum czasowe [ms]
    float audio_loudness = 0.0f;        // Głośność psychoakustyczna [sones]
    // Spektralne dodatkowe
    float audio_spectral_flux = 0.0f;   // Strumień spektralny
    float audio_spectral_slope = 0.0f;  // Nachylenie widma
    float audio_spectral_kurtosis = 0.0f;// Kurtoza widma
    float audio_spectral_skewness = 0.0f;// Asymetria widma
    float audio_fund_salience = 0.0f;   // Wyraźność tonu podstawowego
    // Pasma mocy
    float audio_power_band[8] = {0};    // Moc w 8 pasmach [dB]
    float audio_leq = 0.0f;             // Równoważny poziom dźwięku [dB]
    float audio_l10 = 0.0f;             // Poziom L10 [dB]
    float audio_l90 = 0.0f;             // Poziom L90 [dB]
    float audio_noise_floor = 0.0f;     // Tło szumowe [dB]
    float audio_snr = 0.0f;             // Stosunek sygnału do szumu [dB]
    // Bioakustyczne indeksy
    float audio_aci = 0.0f;             // Acoustic Complexity Index
    float audio_bi = 0.0f;              // Bioacoustic Index
    float audio_ndi = 0.0f;             // Normalized Difference Index
    float audio_adi = 0.0f;             // Acoustic Diversity Index
    float audio_aei = 0.0f;             // Acoustic Evenness Index
    // Wskaźniki klasyfikacji
    float audio_bee_activity = 0.0f;    // Indeks aktywności pszczół [0-100%]
    float audio_swarm_prob = 0.0f;      // Prawdopodobieństwo rojenia [0-1]
    float audio_stress_indicator = 0.0f;// Wskaźnik stresu [0-1]
    float audio_hive_health = 0.0f;     // Indeks zdrowia z audio [0-100%]
    float audio_foraging_eff = 0.0f;    // Efektywność zbierania [0-100%]
    float audio_colony_coherence = 0.0f;// Spójność kolonii [0-1]
    
    // -------------------------------------------------------------------------
    // PARAMETRY RADAR MMWAVE (27 parametrów z radar.ino)
    // -------------------------------------------------------------------------
    float radar_distance = 0.0f;        // Odległość obiektu [m]
    float radar_energy = 0.0f;          // Energia sygnału [dB]
    float radar_speed = 0.0f;           // Prędkość obiektu [m/s]
    float radar_distance_std = 0.0f;    // Odchylenie odległości
    float radar_energy_std = 0.0f;      // Odchylenie energii
    float radar_speed_std = 0.0f;       // Odchylenie prędkości
    float radar_distance_min = 0.0f;    // Min odległość [m]
    float radar_distance_max = 0.0f;    // Max odległość [m]
    float radar_energy_min = 0.0f;      // Min energia [dB]
    float radar_energy_max = 0.0f;      // Max energia [dB]
    float radar_range = 0.0f;           // Zakres odległości
    float radar_energy_variance = 0.0f; // Wariancja energii
    float radar_cv = 0.0f;              // Współczynnik zmienności
    float radar_activity = 0.0f;        // Activity ratio [0-1]
    float radar_idle_percent = 0.0f;    // % czasu bezruchu
    float radar_motion_intensity = 0.0f;// Intensywność ruchu
    float radar_target_rate = 0.0f;     // Tempo celów [/min]
    float radar_max_targets = 0.0f;     // Max liczba celów
    float radar_target_density = 0.0f;  // Gęstość celów
    float radar_slope = 0.0f;           // Nachylenie trendu
    float radar_correlation = 0.0f;     // Korelacja trendu
    float radar_acceleration = 0.0f;    // Przyspieszenie
    float radar_signal_quality = 0.0f;  // Jakość sygnału [0-100%]
    float radar_anomaly_score = 0.0f;   // Wynik anomalii [0-1]
    float radar_hive_health = 0.0f;     // Indeks zdrowia ula [0-100%]
    float radar_power_spectrum = 0.0f;  // Szczyt widma mocy
    float radar_zcr = 0.0f;             // Zero crossing rate radaru
    float radar_entropy = 0.0f;         // Entropia sygnału
    
    // -------------------------------------------------------------------------
    // PARAMETRY HX711 WAGA (105+ parametrów z hx711.ino)
    // -------------------------------------------------------------------------
    float hx711_current = 0.0f;         // Aktualna waga [kg]
    float hx711_mean = 0.0f;            // Średnia waga [kg]
    float hx711_std = 0.0f;             // Odchylenie std [kg]
    float hx711_min = 0.0f;             // Min waga [kg]
    float hx711_max = 0.0f;             // Max waga [kg]
    float hx711_median = 0.0f;          // Mediana wagi [kg]
    float hx711_range = 0.0f;           // Zakres wagi [kg]
    float hx711_variance = 0.0f;        // Wariancja wagi
    float hx711_cv = 0.0f;              // Współczynnik zmienności
    float hx711_iqr = 0.0f;             // Rozstęp międzykwartylowy
    // Trendy temporalne
    float hx711_rate = 0.0f;            // Aktualna zmiana [kg/h]
    float hx711_mean_rate = 0.0f;       // Średnia zmiana [kg/h]
    float hx711_max_pos_rate = 0.0f;    // Max dodatnia zmiana [kg/h]
    float hx711_max_neg_rate = 0.0f;    // Max ujemna zmiana [kg/h]
    float hx711_acceleration = 0.0f;    // Przyspieszenie zmiany
    // Trendy okna czasowe
    float hx711_slope_1h = 0.0f;        // Trend 1h [kg/h]
    float hx711_slope_4h = 0.0f;        // Trend 4h [kg/h]
    float hx711_slope_24h = 0.0f;       // Trend 24h [kg/h]
    float hx711_corr_1h = 0.0f;         // Korelacja 1h
    float hx711_corr_4h = 0.0f;         // Korelacja 4h
    float hx711_corr_24h = 0.0f;        // Korelacja 24h
    float hx711_direction = 0.0f;       // Kierunek trendu [-1,1]
    // Pożytki i nektar
    float hx711_nectar_inflow = 0.0f;   // Przepływ nektaru [kg/h]
    float hx711_nectar_accum = 0.0f;    // Akumulacja nektaru [kg]
    float hx711_foraging_eff = 0.0f;    // Efektywność zbierania [%]
    float hx711_bloom_intensity = 0.0f; // Intensywność kwitnienia [0-1]
    // Produkcja miodu
    float hx711_honey_prod_idx = 0.0f;  // Indeks produkcji miodu
    float hx711_nectar_quality = 0.0f;  // Szacowana jakość nektaru
    // Konsumpcja
    float hx711_consumption_rate = 0.0f;// Zużycie [kg/h]
    float hx711_daily_consumption = 0.0f;// Dzienne zużycie [kg]
    float hx711_food_reserve_days = 0.0f;// Zapas dni jedzenia
    // Zimowla
    float hx711_winter_readiness = 0.0f;// Gotowość do zimowli [%]
    float hx711_starvation_risk = 0.0f; // Ryzyko głodu [0-1]
    // Cykliczność
    float hx711_daily_amplitude = 0.0f; // Dzienna amplituda [kg]
    float hx711_circadian_str = 0.0f;   // Siła cyrkadiana [0-1]
    float hx711_seasonal_trend = 0.0f;  // Trend sezonowy
    // Jakość sygnału
    float hx711_signal_quality = 0.0f;  // Jakość sygnału [0-100%]
    float hx711_noise_level = 0.0f;     // Poziom szumu
    float hx711_drift_rate = 0.0f;      // Dryft długoterminowy
    float hx711_outlier_ratio = 0.0f;   // % odrzutów
    float hx711_sample_count = 0.0f;    // Liczba prób
    // Predykcja
    float hx711_forecast_1h = 0.0f;     // Prognoza 1h [kg]
    float hx711_forecast_6h = 0.0f;     // Prognoza 6h [kg]
    float hx711_forecast_24h = 0.0f;    // Prognoza 24h [kg]
    float hx711_forecast_conf = 0.0f;   // Pewność prognozy [0-1]
    float hx711_prediction_error = 0.0f;// Błąd predykcji
    // Detekcja zdarzeń
    float hx711_harvest_event = 0.0f;   // Detekcja miodobrania [0-1]
    float hx711_swarm_event = 0.0f;     // Detekcja roju [0-1]
    float hx711_feeding_event = 0.0f;   // Dokarmianie [0-1]
    float hx711_water_collection = 0.0f;// Zbieranie wody [kg/h]
    // Statystyki wyższego rzędu
    float hx711_skewness = 0.0f;        // Asymetria rozkładu
    float hx711_kurtosis = 0.0f;        // Kurtoza
    float hx711_entropy = 0.0f;         // Entropia wagowa
    float hx711_complexity = 0.0f;      // Złożoność sygnału
    float hx711_fractal_dim = 0.0f;     // Wymiar fraktalny
    // Zdrowie kolonii z wagi
    float hx711_colony_mass = 0.0f;     // Masa kolonii [kg]
    float hx711_bee_population = 0.0f;  // Szacowana populacja
    float hx711_brood_mass = 0.0f;      // Masa czerwiu
    float hx711_food_stores = 0.0f;     // Zapasy jedzenia [kg]
    float hx711_honey_stores = 0.0f;    // Zapasy miodu [kg]
    float hx711_pollen_stores = 0.0f;   // Zapasy pyłku [kg]
    // Wskaźniki produkcyjne
    float hx711_productivity_idx = 0.0f;// Indeks produktywności
    float hx711_efficiency_score = 0.0f;// Wynik efektywności
    float hx711_growth_rate = 0.0f;     // Tempo rozwoju kolonii
    float hx711_balance_score = 0.0f;   // Bilans energetyczny
    float hx711_resilience_idx = 0.0f;  // Indeks odporności
    // Analiza częstotliwościowa
    float hx711_dominant_period = 0.0f; // Dominujący okres [h]
    float hx711_power_24h = 0.0f;       // Moc 24h
    float hx711_power_12h = 0.0f;       // Moc 12h
    float hx711_power_6h = 0.0f;        // Moc 6h
    float hx711_power_1h = 0.0f;        // Moc 1h
    // Jakość danych
    float hx711_completeness = 0.0f;    // Kompletność danych [%]
    float hx711_reliability = 0.0f;     // Wiarygodność [0-1]
    float hx711_accuracy_est = 0.0f;    // Szacowana dokładność
    float hx711_precision_est = 0.0f;   // Szacowana precyzja
    
    // -------------------------------------------------------------------------
    // PARAMETRY TEMP/HUMIDITY (28 parametrów z temp_humidity.ino)
    // -------------------------------------------------------------------------
    float th_temp_mean = 0.0f;          // Średnia temperatura
    float th_temp_std = 0.0f;           // Odchylenie std temperatury
    float th_temp_min = 0.0f;           // Min temperatura
    float th_temp_max = 0.0f;           // Max temperatura
    float th_temp_range = 0.0f;         // Zakres temperatury
    float th_hum_mean = 0.0f;           // Średnia wilgotność
    float th_hum_std = 0.0f;            // Odchylenie std wilgotności
    float th_hum_min = 0.0f;            // Min wilgotność
    float th_hum_max = 0.0f;            // Max wilgotność
    float th_hum_range = 0.0f;          // Zakres wilgotności
    float th_heat_index = 0.0f;         // Indeks upału
    float th_dew_point = 0.0f;          // Punkt rosy
    float th_comfort_idx = 0.0f;        // Indeks komfortu
    float th_brood_stress = 0.0f;       // Stres czerwiu
    float th_mold_risk = 0.0f;          // Ryzyko pleśni
    float th_temp_stability = 0.0f;     // Stabilność temperatury
    float th_vpd = 0.0f;                // VPD (Vapor Pressure Deficit)
    float th_thi = 0.0f;                // THI (Temperature Humidity Index)
    float th_enthalpy = 0.0f;           // Entalpia
    float th_air_density = 0.0f;        // Gęstość powietrza
    float th_specific_humidity = 0.0f;  // Wilgotność właściwa
    float th_mixing_ratio = 0.0f;       // Stosunek mieszania
    float th_saturation_deficit = 0.0f; // Deficyt nasycenia
    float th_evap_potential = 0.0f;     // Potencjał parowania
    float th_condensation_risk = 0.0f;  // Ryzyko kondensacji
    float th_ventilation_need = 0.0f;   // Potrzeba wentylacji
    float th_cooling_need = 0.0f;       // Potrzeba chłodzenia
    float th_heating_need = 0.0f;       // Potrzeba ogrzewania
    float th_climate_score = 0.0f;      // Ogólny wynik klimatu
    
    // -------------------------------------------------------------------------
    // PARAMETRY AIR QUALITY (24 parametry z air_quality.ino)
    // -------------------------------------------------------------------------
    float aq_iaq = 0.0f;                // Indoor Air Quality [0-500]
    float aq_iaq_mean = 0.0f;           // Średni IAQ
    float aq_contamination = 0.0f;      // Ryzyko kontaminacji
    float aq_mold_risk = 0.0f;          // Ryzyko pleśni z powietrza
    float aq_ventilation_idx = 0.0f;    // Indeks wentylacji
    float aq_co2_mean = 0.0f;           // Średnie CO2
    float aq_voc_mean = 0.0f;           // Średnie VOC
    float aq_stress_air = 0.0f;         // Stres od powietrza
    float aq_purity = 0.0f;             // Czystość powietrza
    float aq_freshness = 0.0f;          // Świeżość powietrza
    float aq_pollution_load = 0.0f;     // Obciążenie zanieczyszczeniami
    float aq_gas_idx = 0.0f;            // Indeks gazów
    float aq_particulate_idx = 0.0f;    // Indeks cząstek stałych
    float aq_allergen_risk = 0.0f;      // Ryzyko alergenów
    float aq_toxicity_est = 0.0f;       // Szacowana toksyczność
    float aq_oxidation_cap = 0.0f;      // Pojemność utleniania
    float aq_reduction_cap = 0.0f;      // Pojemność redukcji
    float aq_buffer_cap = 0.0f;         // Pojemność buforowa
    float aq_recovery_rate = 0.0f;      // Tempo regeneracji
    float aq_stability = 0.0f;          // Stabilność jakości
    float aq_trend = 0.0f;              // Trend jakości
    float aq_forecast = 0.0f;           // Prognoza jakości
    float aq_health_impact = 0.0f;      // Wpływ na zdrowie
    float aq_bee_behavior_impact = 0.0f;// Wpływ na zachowanie pszczół
    
    // -------------------------------------------------------------------------
    // PARAMETRY PIEZO VIBRATION (22 parametry z piezo_vibration.ino)
    // -------------------------------------------------------------------------
    float pv_vib_rms = 0.0f;            // RMS wibracji
    float pv_vib_peak = 0.0f;           // Szczyt wibracji
    float pv_vib_freq_dom = 0.0f;       // Dominująca częstotliwość wibracji
    float pv_bee_traffic = 0.0f;        // Ruch pszczół [0-100%]
    float pv_intrusion_prob = 0.0f;     // Prawdopodobieństwo intruza
    float pv_predator_det = 0.0f;       // Detekcja drapieżnika
    float pv_queen_piping = 0.0f;       // Detekcja queen piping
    float pv_vibration_energy = 0.0f;   // Energia wibracji
    float pv_activity_pattern = 0.0f;   // Wzorzec aktywności
    float pv_disturbance_idx = 0.0f;    // Indeks zakłóceń
    float pv_health_vib = 0.0f;         // Zdrowie z wibracji
    float pv_population_est = 0.0f;     // Szacunkowa populacja z wibracji
    float pv_aggression_idx = 0.0f;     // Indeks agresji
    float pv_swarm_prep = 0.0f;         // Przygotowanie do rojenia
    float pv_cluster_size = 0.0f;       // Rozmiar klastra
    float pv_cluster_activity = 0.0f;   // Aktywność klastra
    float pv_fanning行为 = 0.0f;        // Zachowanie wachlowania
    float pv_buzz_piping = 0.0f;        // Buzz piping detekcja
    float pv_tremble_dance = 0.0f;      // Tremble dance detekcja
    float pv_waggle_dance = 0.0f;       // Waggle dance detekcja
    float pv_foraging_comm = 0.0f;      // Komunikacja żerowa
    float pv_alert_level = 0.0f;        // Poziom alarmu
    
    // -------------------------------------------------------------------------
    // PARAMETRY BAROMETRIC (18 parametrów z barometric.ino)
    // -------------------------------------------------------------------------
    float baro_pressure = 0.0f;         // Ciśnienie [hPa]
    float baro_pressure_mean = 0.0f;    // Średnie ciśnienie
    float baro_pressure_std = 0.0f;     // Odchylenie ciśnienia
    float baro_pressure_min = 0.0f;     // Min ciśnienie
    float baro_pressure_max = 0.0f;     // Max ciśnienie
    float baro_pressure_trend = 0.0f;   // Trend ciśnienia
    float baro_weather_trend = 0.0f;    // Trend pogodowy
    float baro_storm_prob = 0.0f;       // Prawdopodobieństwo burzy
    float baro_foraging_cond = 0.0f;    // Warunki do wylotów
    float baro_pressure_change = 0.0f;  // Zmiana ciśnienia
    float baro_altitude_est = 0.0f;     // Szacowana wysokość
    float baro_sealevel_press = 0.0f;   // Ciśnienie na poziomie morza
    float baro_density_alt = 0.0f;      // Wysokość gęstościowa
    float baro_weather_score = 0.0f;    // Wynik pogody
    float baro_front_approach = 0.0f;   // Nadchodzący front
    float baro_stability = 0.0f;        // Stabilność atmosfery
    float baro_bee_activity_pred = 0.0f;// Przewidywana aktywność pszczół
    float baro_weather_alert = 0.0f;    // Alert pogodowy
    
    // -------------------------------------------------------------------------
    // PARAMETRY LIGHT (17 parametrów z light_sensor.ino)
    // -------------------------------------------------------------------------
    float light_lux = 0.0f;             // Natężenie światła [lux]
    float light_lux_mean = 0.0f;        // Średnie natężenie
    float light_lux_min = 0.0f;         // Min natężenie
    float light_lux_max = 0.0f;         // Max natężenie
    float light_daylight_hours = 0.0f;  // Czas dnia [godziny]
    float light_darkness_hours = 0.0f;  // Czas ciemności
    float light_twilight = 0.0f;        // Czas zmierzchu
    float light_circadian_sync = 0.0f;  // Synchronizacja cyrkadiana [0-1]
    float light_foraging_idx = 0.0f;    // Indeks światła do wylotów [0-100%]
    float light_photoperiod = 0.0f;     // Fotoperiod [godziny]
    float light_seasonal_change = 0.0f; // Zmiana sezonowa
    float light_cloud_cover_est = 0.0f; // Szacowane zachmurzenie
    float light_sunrise_offset = 0.0f;  // Offset wschodu słońca
    float light_uv_idx = 0.0f;          // Indeks UV
    float light_spectrum_blue = 0.0f;   // Niebieskie światło
    float light_spectrum_red = 0.0f;    // Czerwone światło
    float light_spectrum_ir = 0.0f;     // Podczerwień
    
            float aq_co2_alert = 0.0f;            // Alert CO2
        float aq_co2_base = 0.0f;             // Bazowe CO2
        float aq_co2_peak = 0.0f;             // Szczytowe CO2
        float aq_co2_trend = 0.0f;            // Trend CO2
        float aq_co2_variability = 0.0f;      // Zmienność CO2
        float aq_comfort_score = 0.0f;        // Wynik komfortu
        float aq_env_correlation = 0.0f;      // Korelacja środowiskowa
        float aq_stress_level = 0.0f;         // Poziom stresu
        float aq_variability_idx = 0.0f;      // Indeks zmienności
        float aq_voc_base = 0.0f;             // Bazowe VOC
        float aq_voc_peak = 0.0f;             // Szczytowe VOC
        float aq_voc_variability = 0.0f;      // Zmienność VOC
        float aq_voc_warning = 0.0f;          // Ostrzeżenie VOC
        float baro_activity_pred = 0.0f;      // Przewidywana aktywność
        int baro_alert_flag = 0;              // Flaga alertu
        float baro_foraging_idx = 0.0f;       // Indeks wylotów
        float baro_improving_flag = 0.0f;     // Flaga poprawy
        float baro_rain_risk = 0.0f;          // Ryzyko deszczu
        float baro_reliability_idx = 0.0f;    // Indeks wiarygodności
        float baro_severity = 0.0f;           // Wskaźnik dotkliwości
        float baro_storm_risk = 0.0f;         // Ryzyko burzy
        float baro_temperature = 0.0f;        // Temperatura z barometru
        float baro_trend_medium = 0.0f;       // Trend średnioterminowy
        float baro_trend_short = 0.0f;        // Trend krótkoterminowy
        float baro_weather_idx = 0.0f;        // Indeks pogodowy
        float hx711_anomaly_flag = 0.0f;      // Flaga anomalii
        float hx711_health_score = 0.0f;      // Wynik zdrowia
        float light_ir_ratio = 0.0f;          // Stosunek IR
        float light_lux_std = 0.0f;           // Odchylenie lux
        float light_spectrum_data = 0.0f;     // Dane spektralne
        float pv_alien_detected = 0.0f;       // Wykrycie obcego gatunku
        float pv_confidence = 0.0f;           // Pewność detekcji
        float pv_continuous_vib = 0.0f;       // Ciągłe wibracje
        float pv_event_count = 0.0f;          // Liczba zdarzeń
        float pv_impact_flag = 0.0f;          // Flaga uderzenia
        float pv_severity = 0.0f;             // Dotkliwość
        float pv_source_type = 0.0f;          // Typ źródła
        float pv_vib_mean = 0.0f;             // Średnia wibracji
        float pv_vib_std = 0.0f;              // Odchylenie wibracji
        float pv_wind_noise = 0.0f;           // Szum wiatru
        float pv_zcr = 0.0f;                  // Zero Crossing Rate
        float th_hum_stability_1h = 0.0f;     // Stabilność wilgotności 1h
        float th_hum_trend = 0.0f;            // Trend wilgotności
        float th_temp_trend = 0.0f;           // Trend temperatury
    
    // -------------------------------------------------------------------------
    // DODATKOWE POLA (backward compatibility)
    // -------------------------------------------------------------------------
    float weight_rate = 0.0f;           // Szybkość zmiany wagi [kg/h]
    float weight_trend = 0.0f;          // Trend wagi [-1..1]
    int air_iaq = 0;                    // Indeks jakości powietrza [0-100]
};

#endif // APIARY_COLLECTOR_TYPES_H
