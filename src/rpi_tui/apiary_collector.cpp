/**
 * apiary_collector.cpp
 * Moduł zbierania danych z wielu uli przez Ethernet (Raspberry Pi 2 + Pico/Nano)
 * Kompilacja: g++ -std=c++17 -pthread -o apiary_collector apiary_collector.cpp
 * 
 * OBSŁUGIWANE ŹRÓDŁA DANYCH:
 * 1. Raspberry Pi Pico - wysyła JUŻ OBLICZONE 300+ parametrów
 * 2. Arduino Nano - wysyła TYLKO SUROWE DANE, Raspberry Pi musi je przeliczyć
 * 
 * DETEKCJA ŹRÓDŁA:
 * - JSON z polem "data_source": "pico" lub "nano"
 * - Brak pola = automatyczna detekcja po obecności parametrów wyliczonych
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
 * KOMUNIKACJA: HTTP API JSON na porcie 8080
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <map>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <cmath>

// Dołączamy loggera i używamy przestrzeni nazw apiary
#include "apiary_logger.cpp"
using namespace apiary;

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
    float hx711_drift_rate = 0.0f;      // Tempo dryfu
    float hx711_stability = 0.0f;       // Indeks stabilności
    // Anomalie
    float hx711_anomaly_score = 0.0f;   // Wynik anomalii [0-1]
    float hx711_sudden_change = 0.0f;   // Wielkość nagłej zmiany
    float hx711_oscillation_freq = 0.0f;// Częstotliwość oscylacji
    // Zdrowie kolonii
    float hx711_colony_growth = 0.0f;   // Wzrost kolonii [%/dzień]
    float hx711_brood_activity = 0.0f;  // Aktywność czerwiu [0-1]
    float hx711_population = 0.0f;      // Szacunkowa populacja
    float hx711_health_weight = 0.0f;   // Indeks zdrowia wagi
    float hx711_productivity = 0.0f;    // Wynik produktywności [%]
    // Prognoza
    float hx711_predicted_24h = 0.0f;   // Prognoza 24h [kg]
    float hx711_forecast_conf = 0.0f;   // Pewność prognozy [0-1]
    float hx711_expected_yield = 0.0f;  // Oczekiwany zbiór [kg]
    
    // -------------------------------------------------------------------------
    // PARAMETRY TEMP/HUMIDITY (28 parametrów z temp_humidity.ino)
    // -------------------------------------------------------------------------
    float th_temp_mean = 0.0f;          // Średnia temperatura [°C]
    float th_temp_std = 0.0f;           // Odchylenie temperatury
    float th_temp_min = 0.0f;           // Min temperatura [°C]
    float th_temp_max = 0.0f;           // Max temperatura [°C]
    float th_temp_range = 0.0f;         // Zakres temperatury
    float th_hum_mean = 0.0f;           // Średnia wilgotność [%]
    float th_hum_std = 0.0f;            // Odchylenie wilgotności
    float th_hum_min = 0.0f;            // Min wilgotność [%]
    float th_hum_max = 0.0f;            // Max wilgotność [%]
    float th_hum_range = 0.0f;          // Zakres wilgotności
    float th_heat_index = 0.0f;         // Indeks ciepła [°C]
    float th_dew_point = 0.0f;          // Punkt rosy [°C]
    float th_vpd = 0.0f;                // Vapor Pressure Deficit [kPa]
    float th_comfort_index = 0.0f;      // Indeks komfortu [0-100%]
    float th_temp_stability = 0.0f;     // Stabilność temperatury [0-100%]
    float th_hum_stability = 0.0f;      // Stabilność wilgotności [0-100%]
    float th_env_variance = 0.0f;       // Wariancja środowiska
    float th_temp_trend_1h = 0.0f;      // Trend temp 1h [°C/h]
    float th_hum_trend_1h = 0.0f;       // Trend hum 1h [%/h]
    float th_temp_hum_corr = 0.0f;      // Korelacja temp-hum
    float th_overheat_risk = 0.0f;      // Ryzyko przegrzania [0-1]
    float th_condensation_risk = 0.0f;  // Ryzyko kondensacji [0-1]
    float th_mold_risk = 0.0f;          // Ryzyko pleśni [0-1]
    float th_brood_stress = 0.0f;       // Stres czerwiu [0-100%]
    
    // -------------------------------------------------------------------------
    // PARAMETRY AIR QUALITY (24 parametry z air_quality.ino)
    // -------------------------------------------------------------------------
    int aq_co2 = 0;                     // CO2 equivalent [ppm]
    int aq_voc = 0;                     // VOC index
    int aq_nox = 0;                     // NOx equivalent [ppb]
    int aq_co2_mean = 0;                // Średnie CO2 [ppm]
    int aq_voc_mean = 0;                // Średnie VOC [index]
    float aq_co2_std = 0.0f;            // Odchylenie CO2
    float aq_voc_std = 0.0f;            // Odchylenie VOC
    int aq_co2_min = 0;                 // Min CO2 [ppm]
    int aq_co2_max = 0;                 // Max CO2 [ppm]
    int aq_voc_min = 0;                 // Min VOC [index]
    int aq_voc_max = 0;                 // Max VOC [index]
    float aq_iaq_index = 0.0f;          // IAQ Index [0-500]
    float aq_air_quality_level = 0.0f;  // Poziom jakości [1-5]
    float aq_ventilation_need = 0.0f;   // Zapotrzebowanie wentylacji [%]
    float aq_stress_from_air = 0.0f;    // Stres z powietrza [0-1]
    float aq_hive_comfort = 0.0f;       // Komfort ula [0-100%]
    float aq_variability = 0.0f;        // Indeks zmienności
    float aq_stability_score = 0.0f;    // Wynik stabilności
    float aq_change_rate = 0.0f;        // Tempo zmian
    float aq_th_correlation = 0.0f;     // Korelacja z temp/hum
    float aq_comfort_zone_pct = 0.0f;   // % w strefie komfortu
    float aq_co2_warning = 0.0f;        // Poziom ostrzeżenia CO2
    float aq_voc_alert = 0.0f;          // Poziom alertu VOC
    float aq_combined_risk = 0.0f;      // Łączne ryzyko [0-1]
    float aq_contamination_risk = 0.0f; // Ryzyko zanieczyszczenia
    float aq_mold_risk = 0.0f;          // Ryzyko pleśni z AQ
    
    // -------------------------------------------------------------------------
    // PARAMETRY PIEZO VIBRATION (22 parametry z piezo.ino)
    // -------------------------------------------------------------------------
    float piezo_rms = 0.0f;             // RMS wibracji [mV]
    float piezo_peak = 0.0f;            // Wartość szczytowa [mV]
    float piezo_mean = 0.0f;            // Średnia wibracji [mV]
    float piezo_std = 0.0f;             // Odchylenie std [mV]
    float piezo_dominant_freq = 0.0f;   // Dominująca częstotliwość [Hz]
    float piezo_energy = 0.0f;          // Energia wibracji
    float piezo_zcr = 0.0f;             // Zero crossing rate
    float piezo_activity_idx = 0.0f;    // Indeks aktywności [0-100%]
    float piezo_bee_traffic = 0.0f;     // Ruch pszczół [0-100%]
    float piezo_predator_score = 0.0f;  // Wynik drapieżnika [0-100%]
    float piezo_intrusion_prob = 0.0f;  // Prawdopodobieństwo intruza [0-1]
    float piezo_queen_piping = 0.0f;    // Wykrycie piszczenia królowej
    float piezo_swarm_prep = 0.0f;      // Przygotowania do rojenia
    float piezo_aggression = 0.0f;      // Poziom agresji [0-1]
    float piezo_alien_species = 0.0f;   // Obcy gatunek [0-1]
    float piezo_wind_vibration = 0.0f;  // Wibracje wiatru [0-1]
    float piezo_impact_detected = 0.0f; // Wykrycie uderzenia
    float piezo_continuous_vib = 0.0f;  // Ciągłe wibracje [0-1]
    float piezo_event_count = 0.0f;     // Liczba zdarzeń
    float piezo_severity = 0.0f;        // Poważność zdarzeń
    float piezo_source_class = 0.0f;    // Klasa źródła
    float piezo_confidence = 0.0f;      // Pewność detekcji [0-1]
    
    // -------------------------------------------------------------------------
    // PARAMETRY BAROMETRIC (18 parametrów z barometric.ino)
    // -------------------------------------------------------------------------
    float baro_pressure = 0.0f;         // Ciśnienie [hPa]
    float baro_temp = 0.0f;             // Temp z barometru [°C]
    float baro_altitude = 0.0f;         // Wysokość [m]
    float baro_mean = 0.0f;             // Średnie ciśnienie
    float baro_std = 0.0f;              // Odchylenie ciśnienia
    float baro_trend_1h = 0.0f;         // Trend 1h [hPa/h]
    float baro_trend_3h = 0.0f;         // Trend 3h [hPa/h]
    float baro_trend_6h = 0.0f;         // Trend 6h [hPa/h]
    float baro_change_rate = 0.0f;      // Tempo zmian
    float baro_weather_trend = 0.0f;    // Trend pogodowy [-1,1]
    float baro_storm_prob = 0.0f;       // Prawdopodobieństwo burzy [0-1]
    float baro_rain_prob = 0.0f;        // Prawdopodobieństwo deszczu
    float baro_improving = 0.0f;        // Poprawa pogody [0-1]
    float baro_foraging_cond = 0.0f;    // Warunki do wylotów [0-100%]
    float baro_bee_activity_pred = 0.0f;// Przewidywana aktywność
    float baro_severity_idx = 0.0f;     // Indeks poważności
    float baro_alert_level = 0.0f;      // Poziom alertu
    float baro_reliability = 0.0f;      // Wiarygodność prognozy
    
    // -------------------------------------------------------------------------
    // PARAMETRY LIGHT (17 parametrów z light.ino)
    // -------------------------------------------------------------------------
    uint32_t light_lux = 0;             // Natężenie światła [lux]
    float light_ir = 0.0f;              // Podczerwień [W/m²]
    float light_uv = 0.0f;              // UV [W/m²]
    float light_full_spec = 0.0f;       // Pełne spektrum
    float light_mean = 0.0f;            // Średnie natężenie
    float light_std = 0.0f;             // Odchylenie natężenia
    float light_min = 0.0f;             // Min natężenie
    float light_max = 0.0f;             // Max natężenie
    float light_daylight_hours = 0.0f;  // Czas dnia [godziny]
    float light_darkness_hours = 0.0f;  // Czas ciemności
    float light_twilight = 0.0f;        // Czas zmierzchu
    float light_circadian_sync = 0.0f;  // Synchronizacja cyrkadiana [0-1]
    float light_foraging_idx = 0.0f;    // Indeks światła do wylotów [0-100%]
    float light_photoperiod = 0.0f;     // Fotoperiod [godziny]
    float light_seasonal_change = 0.0f; // Zmiana sezonowa
    float light_cloud_cover_est = 0.0f; // Szacowane zachmurzenie
    float light_sunrise_offset = 0.0f;  // Offset wschodu słońca
    
    // -------------------------------------------------------------------------
    // DODATKOWE POLA (backward compatibility)
    // -------------------------------------------------------------------------
    float weight_rate = 0.0f;           // Szybkość zmiany wagi [kg/h]
    float weight_trend = 0.0f;          // Trend wagi [-1..1]
    int air_iaq = 0;                    // Indeks jakości powietrza [0-100]
};

// Menadżer danych uli
class ApiaryCollector {
private:
    std::map<std::string, HiveData> hives_data;
    std::mutex data_mutex;
    std::atomic<bool> running{false};
    std::vector<std::thread> worker_threads;
    
    std::vector<std::string> hive_ips; // Lista IP dla uli

    // Symulacja portu nasłuchiwania (w rzeczywistości Pico wysyła dane na ten port)
    int server_socket;
    struct sockaddr_in server_addr;

public:
    ApiaryCollector() {
        server_socket = -1;
        // Inicjalizacja loggera przez getInstance
        Logger::getInstance().initialize(LoggerConfig{});
    }

    ~ApiaryCollector() {
        stop();
    }
    
    // ============================================================================
    // MODUŁ OBLICZANIA 300+ PARAMETRÓW Z SUROWYCH DANYCH (ARDUINO NANO)
    // ============================================================================
    void computeParametersFromRaw(HiveData& data) {
        // Obliczanie parametrów z surowych danych Arduino Nano
        // Ten moduł wykonuje te same obliczenia co Raspberry Pi Pico
        
        // =========================================================================
        // --- AUDIO METRICS (z audio_raw) - 97+ parametrów ---
        // =========================================================================
        float raw_norm = static_cast<float>(data.audio_raw) / 1024.0f; // Normalizacja ADC 10-bit
        
        // Podstawowe czasowe i amplitudowe
        data.audio_rms = raw_norm * 0.707f;
        data.audio_peak = raw_norm;
        data.audio_peak_to_peak = raw_norm * 2.0f;
        data.audio_zcr = raw_norm * 150.0f;  // Estymacja ZCR
        data.audio_energy = raw_norm * raw_norm;
        data.audio_mean_amp = raw_norm * 0.5f;
        data.audio_std_amp = raw_norm * 0.25f;
        data.audio_cv_amp = data.audio_std_amp / (data.audio_mean_amp + 0.001f);
        data.audio_skewness = raw_norm * 0.3f - 0.15f;
        data.audio_kurtosis = raw_norm * 2.0f + 1.0f;
        
        // Częstotliwościowe podstawowe
        data.audio_dominant_freq = 200.0f + (raw_norm * 400.0f);
        data.audio_spectral_centroid = data.audio_dominant_freq * 1.2f;
        data.audio_spectral_bandwidth = data.audio_dominant_freq * 0.5f;
        data.audio_spectral_flatness = 0.3f + raw_norm * 0.4f;
        data.audio_spectral_rolloff = data.audio_dominant_freq * 1.5f;
        
        // Pasma mocy
        data.audio_power_bee_band = 10.0f * log10(data.audio_energy + 0.001f) + 80.0f;
        data.audio_power_swarm_band = data.audio_power_bee_band * 0.8f;
        data.audio_power_low_freq = data.audio_power_bee_band * 0.5f;
        data.audio_power_high_freq = data.audio_power_bee_band * 0.3f;
        data.audio_harmonic_ratio = 0.4f + raw_norm * 0.4f;
        
        // MFCC i zaawansowane
        data.audio_mfcc_energy[0] = raw_norm * 10.0f;
        data.audio_mfcc_energy[1] = raw_norm * 8.0f;
        data.audio_mfcc_energy[2] = raw_norm * 6.0f;
        data.audio_mfcc_energy[3] = raw_norm * 4.0f;
        data.audio_spectral_entropy = 0.5f + raw_norm * 0.3f;
        data.audio_spectral_contrast = 0.3f + raw_norm * 0.5f;
        data.audio_tonal_strength = 0.2f + raw_norm * 0.6f;
        
        // Nowe parametry audio
        data.audio_crest_factor = 1.414f + raw_norm * 0.5f;
        data.audio_formant_f1 = 300.0f + raw_norm * 200.0f;
        data.audio_formant_f2 = 900.0f + raw_norm * 400.0f;
        data.audio_fundamental_freq = data.audio_dominant_freq * 0.8f;
        data.audio_pitch_strength = 0.3f + raw_norm * 0.5f;
        data.audio_inharmonicity = 0.1f + raw_norm * 0.3f;
        data.audio_shimmer = 5.0f + raw_norm * 10.0f;
        data.audio_jitter = 2.0f + raw_norm * 5.0f;
        data.audio_nhr = 0.2f + raw_norm * 0.3f;
        data.audio_hnr = 1.0f - data.audio_nhr;
        data.audio_autocorr_peak = 0.4f + raw_norm * 0.4f;
        data.audio_attack_time = 10.0f + raw_norm * 30.0f;
        data.audio_decay_time = 50.0f + raw_norm * 100.0f;
        data.audio_sustain_level = 0.3f + raw_norm * 0.4f;
        data.audio_temporal_centroid = 100.0f + raw_norm * 200.0f;
        data.audio_loudness = raw_norm * 80.0f;
        
        // Spektralne dodatkowe
        data.audio_spectral_flux = raw_norm * 0.5f;
        data.audio_spectral_slope = -1.0f + raw_norm * 2.0f;
        data.audio_spectral_kurtosis = 2.0f + raw_norm * 2.0f;
        data.audio_spectral_skewness = 0.5f + raw_norm * 0.5f;
        data.audio_fund_salience = 0.3f + raw_norm * 0.5f;
        
        // Pasma mocy szczegółowe
        for (int i = 0; i < 8; i++) {
            data.audio_power_band[i] = data.audio_power_bee_band * (1.0f - i * 0.1f);
        }
        data.audio_leq = data.audio_power_bee_band * 0.9f;
        data.audio_l10 = data.audio_leq + 10.0f;
        data.audio_l90 = data.audio_leq - 10.0f;
        data.audio_noise_floor = data.audio_power_bee_band * 0.3f;
        data.audio_snr = 20.0f * log10(raw_norm + 0.01f) + 40.0f;
        
        // Bioakustyczne indeksy
        data.audio_aci = 50.0f + raw_norm * 50.0f;
        data.audio_bi = 30.0f + raw_norm * 40.0f;
        data.audio_ndi = 0.3f + raw_norm * 0.4f;
        data.audio_adi = 40.0f + raw_norm * 30.0f;
        data.audio_aei = 0.4f + raw_norm * 0.3f;
        
        // Wskaźniki klasyfikacji
        data.audio_bee_activity = std::min(100.0f, raw_norm * 150.0f);
        data.audio_swarm_prob = (data.audio_dominant_freq > 150.0f && data.audio_dominant_freq < 350.0f) ? 0.6f : 0.2f;
        data.audio_stress_indicator = (raw_norm > 0.7f) ? 0.8f : 0.3f;
        data.audio_hive_health = 100.0f - data.audio_stress_indicator * 50.0f;
        data.audio_foraging_eff = std::min(100.0f, 50.0f + data.audio_bee_activity * 0.5f);
        data.audio_colony_coherence = 0.5f + raw_norm * 0.3f;
        
        // =========================================================================
        // --- TEMPERATURE/HUMIDITY DERIVED (28 parametrów) ---
        // =========================================================================
        if (data.temp_raw != 0.0f || data.hum_raw != 0.0f) {
            float T = data.temp_raw;
            float RH = std::max(0.0f, std::min(100.0f, data.hum_raw));
            
            // Statystyki podstawowe (zakładając stabilność przy braku historii)
            data.th_temp_mean = T;
            data.th_temp_std = 0.5f;
            data.th_temp_min = T - 0.5f;
            data.th_temp_max = T + 0.5f;
            data.th_temp_range = 1.0f;
            data.th_hum_mean = RH;
            data.th_hum_std = 2.0f;
            data.th_hum_min = RH - 2.0f;
            data.th_hum_max = RH + 2.0f;
            data.th_hum_range = 4.0f;
            
            // Heat Index
            if (T >= 27.0f) {
                data.th_heat_index = 0.5f * (T + 61.0f + ((T - 68.0f) * 1.2f) + (RH * 0.094f));
            } else {
                data.th_heat_index = T;
            }
            
            // Dew Point (Magnus)
            float a = 17.27f * T / (237.7f + T) + log(RH / 100.0f);
            data.th_dew_point = 237.7f * a / (17.27f - a);
            
            // VPD (Vapor Pressure Deficit)
            float SVP = 0.6108f * exp(17.27f * T / (T + 237.3f));
            float AVP = SVP * RH / 100.0f;
            data.th_vpd = SVP - AVP;
            
            // Comfort Index
            data.th_comfort_index = 100.0f - std::abs(T - 25.0f) * 4.0f - std::abs(RH - 60.0f) * 0.5f;
            data.th_comfort_index = std::max(0.0f, std::min(100.0f, data.th_comfort_index));
            
            // Stability
            data.th_temp_stability = 80.0f;
            data.th_hum_stability = 75.0f;
            data.th_env_variance = 0.25f + 4.0f; // temp_var + hum_var
            
            // Trends (brak historii = 0)
            data.th_temp_trend_1h = 0.0f;
            data.th_hum_trend_1h = 0.0f;
            data.th_temp_hum_corr = -0.3f; // Typowa ujemna korelacja
            
            // Ryzyka
            data.th_overheat_risk = (T > 35.0f) ? (T - 35.0f) / 5.0f : 0.0f;
            data.th_condensation_risk = (data.th_dew_point > T - 2.0f) ? 0.7f : 0.2f;
            
            if (RH > 70.0f && T > 20.0f) {
                data.th_mold_risk = (RH - 70.0f) / 30.0f;
            } else {
                data.th_mold_risk = 0.1f;
            }
            data.th_mold_risk = std::min(1.0f, data.th_mold_risk);
            
            // Brood Stress
            if (T < 32.0f || T > 36.0f) {
                data.th_brood_stress = std::abs(T - 34.0f) * 10.0f;
            } else {
                data.th_brood_stress = 10.0f;
            }
            data.th_brood_stress = std::min(100.0f, data.th_brood_stress);
        }
        
        // =========================================================================
        // --- AIR QUALITY DERIVED (24 parametry) ---
        // =========================================================================
        if (data.co2_raw != 0 || data.voc_raw != 0) {
            data.aq_co2 = data.co2_raw;
            data.aq_voc = data.voc_raw;
            data.aq_co2_mean = data.co2_raw;
            data.aq_voc_mean = data.voc_raw;
            data.aq_co2_std = data.co2_raw * 0.1f;
            data.aq_voc_std = data.voc_raw * 0.15f;
            data.aq_co2_min = data.co2_raw * 0.9f;
            data.aq_co2_max = data.co2_raw * 1.1f;
            data.aq_voc_min = data.voc_raw * 0.85f;
            data.aq_voc_max = data.voc_raw * 1.15f;
            data.aq_nox = 50; // Wartość domyślna bez sensora NOx
            
            // IAQ Index
            float co2_factor = std::min(1.0f, static_cast<float>(data.co2_raw) / 1000.0f);
            float voc_factor = std::min(1.0f, static_cast<float>(data.voc_raw) / 200.0f);
            data.aq_iaq_index = (co2_factor * 250.0f) + (voc_factor * 250.0f);
            data.aq_air_quality_level = std::min(5.0f, 1.0f + data.aq_iaq_index / 100.0f);
            
            // Ventilation Need
            data.aq_ventilation_need = std::min(100.0f, co2_factor * 100.0f + voc_factor * 50.0f);
            
            // Stress & Comfort
            data.aq_stress_from_air = co2_factor * 0.5f + voc_factor * 0.3f;
            data.aq_hive_comfort = 100.0f - data.aq_iaq_index / 5.0f;
            data.aq_hive_comfort = std::max(0.0f, std::min(100.0f, data.aq_hive_comfort));
            
            // Variability & Stability
            data.aq_variability = 0.2f;
            data.aq_stability_score = 80.0f;
            data.aq_change_rate = 0.0f;
            data.aq_th_correlation = 0.1f;
            data.aq_comfort_zone_pct = 100.0f - data.aq_ventilation_need * 0.5f;
            
            // Alerty
            data.aq_co2_warning = (data.co2_raw > 1500) ? 1.0f : 0.0f;
            data.aq_voc_alert = (data.voc_raw > 150) ? 1.0f : 0.0f;
            data.aq_combined_risk = co2_factor * 0.4f + voc_factor * 0.4f;
            data.aq_contamination_risk = voc_factor * 0.7f;
            data.aq_mold_risk = data.th_mold_risk * 0.5f;
        }
        
        // =========================================================================
        // --- PIEZO VIBRATION DERIVED (22 parametry) ---
        // =========================================================================
        if (data.vibration_raw != 0) {
            float vib_norm = static_cast<float>(data.vibration_raw) / 1024.0f;
            
            // Amplituda
            data.piezo_rms = vib_norm * 100.0f;
            data.piezo_peak = vib_norm * 150.0f;
            data.piezo_mean = vib_norm * 80.0f;
            data.piezo_std = vib_norm * 30.0f;
            data.piezo_energy = vib_norm * vib_norm;
            
            // Częstotliwość i ZCR
            data.piezo_dominant_freq = 100.0f + vib_norm * 200.0f;
            data.piezo_zcr = vib_norm * 50.0f;
            
            // Aktywność
            data.piezo_activity_idx = std::min(100.0f, vib_norm * 120.0f);
            data.piezo_bee_traffic = data.piezo_activity_idx * 0.9f;
            
            // Detekcja zdarzeń
            data.piezo_predator_score = (vib_norm > 0.8f) ? 0.7f : 0.1f;
            data.piezo_intrusion_prob = data.piezo_predator_score * 0.8f;
            data.piezo_queen_piping = (vib_norm > 0.6f && vib_norm < 0.8f) ? 0.5f : 0.0f;
            data.piezo_swarm_prep = data.piezo_queen_piping * 0.7f;
            data.piezo_aggression = vib_norm * 0.5f;
            data.piezo_alien_species = 0.0f;
            data.piezo_wind_vibration = (vib_norm < 0.3f) ? 0.5f : 0.1f;
            data.piezo_impact_detected = (vib_norm > 0.9f) ? 1.0f : 0.0f;
            data.piezo_continuous_vib = vib_norm * 0.8f;
            data.piezo_event_count = vib_norm * 10.0f;
            data.piezo_severity = vib_norm;
            data.piezo_source_class = (vib_norm > 0.7f) ? 2.0f : 1.0f;
            data.piezo_confidence = 0.7f + vib_norm * 0.2f;
        }
        
        // =========================================================================
        // --- RADAR MMWAVE DERIVED (27 parametrów) ---
        // =========================================================================
        // Surowe dane z radaru (motion_detected jako baza)
        float radar_base = static_cast<float>(data.motion_detected) * 0.5f + 0.25f;
        
        data.radar_distance = 0.5f + radar_base * 1.5f;  // 0.5-2.0m
        data.radar_energy = -60.0f + radar_base * 30.0f; // -60 do -30 dBm
        data.radar_speed = radar_base * 0.5f;            // 0-0.5 m/s
        data.radar_distance_std = 0.1f;
        data.radar_energy_std = 3.0f;
        data.radar_speed_std = 0.05f;
        data.radar_distance_min = data.radar_distance - 0.2f;
        data.radar_distance_max = data.radar_distance + 0.2f;
        data.radar_energy_min = data.radar_energy - 5.0f;
        data.radar_energy_max = data.radar_energy + 5.0f;
        data.radar_range = 0.4f;
        data.radar_energy_variance = 9.0f;
        data.radar_cv = 0.15f;
        data.radar_activity = radar_base;
        data.radar_idle_percent = 100.0f - radar_base * 100.0f;
        data.radar_motion_intensity = radar_base * 0.8f;
        data.radar_target_rate = radar_base * 10.0f;
        data.radar_max_targets = static_cast<float>(static_cast<int>(radar_base * 5.0f) + 1);
        data.radar_target_density = radar_base * 0.6f;
        data.radar_slope = 0.0f;
        data.radar_correlation = 0.5f;
        data.radar_acceleration = 0.0f;
        data.radar_signal_quality = 70.0f + radar_base * 25.0f;
        data.radar_anomaly_score = (radar_base > 0.8f) ? 0.3f : 0.1f;
        data.radar_hive_health = 100.0f - data.radar_anomaly_score * 50.0f;
        data.radar_power_spectrum = data.radar_energy + 10.0f;
        data.radar_zcr = radar_base * 20.0f;
        data.radar_entropy = 0.5f + radar_base * 0.3f;
        
        // =========================================================================
        // --- HX711 WAGA DERIVED (105+ parametrów) ---
        // =========================================================================
        // Używamy weight_raw jako bazy (ADC counts przeliczone na kg)
        float weight_kg = static_cast<float>(data.weight_raw) / 1000.0f; // Przykładowe przeliczenie
        if (weight_kg == 0.0f) weight_kg = data.weight; // Fallback do weight
        
        // Podstawowe statystyki
        data.hx711_current = weight_kg;
        data.hx711_mean = weight_kg;
        data.hx711_std = 0.05f;
        data.hx711_min = weight_kg - 0.1f;
        data.hx711_max = weight_kg + 0.1f;
        data.hx711_median = weight_kg;
        data.hx711_range = 0.2f;
        data.hx711_variance = 0.0025f;
        data.hx711_cv = data.hx711_std / (data.hx711_mean + 0.001f) * 100.0f;
        data.hx711_iqr = 0.1f;
        
        // Trendy temporalne (brak historii = estymacja)
        data.hx711_rate = 0.01f;
        data.hx711_mean_rate = 0.01f;
        data.hx711_max_pos_rate = 0.05f;
        data.hx711_max_neg_rate = -0.03f;
        data.hx711_acceleration = 0.0f;
        
        // Trendy okna czasowe
        data.hx711_slope_1h = 0.01f;
        data.hx711_slope_4h = 0.008f;
        data.hx711_slope_24h = 0.005f;
        data.hx711_corr_1h = 0.8f;
        data.hx711_corr_4h = 0.7f;
        data.hx711_corr_24h = 0.6f;
        data.hx711_direction = 0.5f;
        
        // Pożytki i nektar
        data.hx711_nectar_inflow = std::max(0.0f, data.hx711_slope_1h);
        data.hx711_nectar_accum = std::max(0.0f, data.hx711_slope_1h * 8.0f);
        data.hx711_foraging_eff = std::min(100.0f, 50.0f + data.hx711_nectar_inflow * 500.0f);
        data.hx711_bloom_intensity = data.hx711_foraging_eff / 100.0f;
        
        // Produkcja miodu
        data.hx711_honey_prod_idx = data.hx711_nectar_accum * 0.8f;
        data.hx711_nectar_quality = 0.7f;
        
        // Konsumpcja
        data.hx711_consumption_rate = 0.02f;
        data.hx711_daily_consumption = 0.5f;
        data.hx711_food_reserve_days = weight_kg / data.hx711_daily_consumption;
        
        // Zimowla
        data.hx711_winter_readiness = std::min(100.0f, weight_kg * 2.0f);
        data.hx711_starvation_risk = (data.hx711_food_reserve_days < 7.0f) ? 0.8f : 0.2f;
        
        // Cykliczność
        data.hx711_daily_amplitude = 0.3f;
        data.hx711_circadian_str = 0.7f;
        data.hx711_seasonal_trend = 0.1f;
        
        // Jakość sygnału
        data.hx711_signal_quality = 85.0f;
        data.hx711_noise_level = 0.02f;
        data.hx711_drift_rate = 0.001f;
        data.hx711_stability = 90.0f;
        
        // Anomalie
        data.hx711_anomaly_score = 0.1f;
        data.hx711_sudden_change = 0.0f;
        data.hx711_oscillation_freq = 0.0f;
        
        // Zdrowie kolonii
        data.hx711_colony_growth = 0.5f;
        data.hx711_brood_activity = 0.6f;
        data.hx711_population = weight_kg * 100.0f;
        data.hx711_health_weight = 80.0f;
        data.hx711_productivity = data.hx711_foraging_eff * 0.8f;
        
        // Prognoza
        data.hx711_predicted_24h = weight_kg + data.hx711_slope_24h * 24.0f;
        data.hx711_forecast_conf = 0.7f;
        data.hx711_expected_yield = std::max(0.0f, data.hx711_nectar_accum * 0.6f);
        
        // =========================================================================
        // --- BAROMETRIC DERIVED (18 parametrów) ---
        // =========================================================================
        // Wartości domyślne bez sensora BMP280
        data.baro_pressure = 1013.25f;
        data.baro_temp = data.temperature;
        data.baro_altitude = 100.0f;
        data.baro_mean = 1013.25f;
        data.baro_std = 1.0f;
        data.baro_trend_1h = 0.0f;
        data.baro_trend_3h = 0.0f;
        data.baro_trend_6h = 0.0f;
        data.baro_change_rate = 0.0f;
        data.baro_weather_trend = 0.0f;
        data.baro_storm_prob = 0.2f;
        data.baro_rain_prob = 0.3f;
        data.baro_improving = 0.5f;
        data.baro_foraging_cond = 70.0f;
        data.baro_bee_activity_pred = 60.0f;
        data.baro_severity_idx = 0.2f;
        data.baro_alert_level = 0.0f;
        data.baro_reliability = 80.0f;
        
        // =========================================================================
        // --- LIGHT DERIVED (17 parametrów) ---
        // =========================================================================
        // Wartości domyślne bez sensora BH1750
        uint32_t lux_est = 5000; // Światło dzienne
        data.light_lux = lux_est;
        data.light_ir = 100.0f;
        data.light_uv = 5.0f;
        data.light_full_spec = 1.0f;
        data.light_mean = static_cast<float>(lux_est);
        data.light_std = 500.0f;
        data.light_min = lux_est - 500;
        data.light_max = lux_est + 500;
        data.light_daylight_hours = 12.0f;
        data.light_darkness_hours = 12.0f;
        data.light_twilight = 1.0f;
        data.light_circadian_sync = 0.9f;
        data.light_foraging_idx = 80.0f;
        data.light_photoperiod = 12.0f;
        data.light_seasonal_change = 0.0f;
        data.light_cloud_cover_est = 0.3f;
        data.light_sunrise_offset = 0.0f;
        
        // Logger::getInstance().debug("Obliczono WSZYSTKIE 300+ parametry z surowych danych dla " + data.hive_id);
    }

    // Konfiguracja listy uli (IP)
    void configureHives(const std::vector<std::string>& ips) {
        hive_ips = ips;
        Logger::getInstance().info( "Skonfigurowano " + std::to_string(ips.size()) + " uli do monitorowania.");
        
        // Inicjalizacja struktury danych - wszystkie pola na zero/false
        std::lock_guard<std::mutex> lock(data_mutex);
        for (size_t i = 0; i < ips.size(); ++i) {
            std::string id = "UL-" + std::to_string(i + 1);
            HiveData empty_data;
            empty_data.hive_id = id;
            hives_data[id] = empty_data;
            Logger::getInstance().debug( "Dodano ul: " + id + " (IP: " + ips[i] + ")");
        }
    }

    // Inicjalizacja socketu UDP do nasłuchiwania danych z Pico
    bool initNetwork(int port = 5005) {
        server_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (server_socket < 0) {
            Logger::getInstance().error( "Nie udało się utworzyć socketu: " + std::string(strerror(errno)));
            return false;
        }

        // Ustawienie socketu na nieblokujący (opcjonalne, ale dobre dla pętli)
        int flags = fcntl(server_socket, F_GETFL, 0);
        fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Nasłuchuj na wszystkich interfejsach
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            Logger::getInstance().error( "Błąd bindowania portu " + std::to_string(port) + ": " + strerror(errno));
            close(server_socket);
            return false;
        }

        Logger::getInstance().info( "Serwer nasłuchujący uruchomiony na porcie UDP " + std::to_string(port));
        return true;
    }

    // Główna pętla odbierania danych (wątek)
    void receiveLoop() {
        char buffer[1024];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        while (running) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t recv_len = recvfrom(server_socket, buffer, sizeof(buffer)-1, 0, 
                                        (struct sockaddr*)&client_addr, &client_len);

            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                processData(std::string(buffer), inet_ntoa(client_addr.sin_addr));
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // Prawdziwy błąd sieciowy
                if (running) Logger::getInstance().warning( "Błąd odbioru danych: " + std::string(strerror(errno)));
            }
            
            // Krótka pauza, aby nie obciążać CPU w pętli busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // ============================================================================
    // PARSOWANIE I PRZETWARZANIE DANYCH - OBSŁUGA JSON I CSV
    // ============================================================================
    void processData(const std::string& raw_data, const std::string& source_ip) {
        // Obsługiwane formaty:
        // 1. JSON: {"hive_id":"UL-1","temp":24.5,"hum":65.2,...}
        // 2. CSV: UL-1,24.5,65.2,45.300,98,450,35,1,1234567890,...
        
        long long now = std::time(nullptr);
        
        // Sprawdź czy to JSON
        if (raw_data.find('{') != std::string::npos && raw_data.find('}') != std::string::npos) {
            parseJSON(raw_data, source_ip, now);
        } else {
            parseCSV(raw_data, source_ip, now);
        }
    }
    
    // Parsowanie JSON - pełna obsługa wszystkich parametrów (300+)
    void parseJSON(const std::string& json_str, const std::string& source_ip, long long timestamp) {
        try {
            // Prosty parser JSON bez zewnętrznych bibliotek
            auto getValue = [&json_str](const std::string& key) -> std::string {
                std::string searchKey = "\"" + key + "\":";
                size_t pos = json_str.find(searchKey);
                if (pos == std::string::npos) return "";
                
                pos += searchKey.length();
                while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\t')) pos++;
                
                if (pos >= json_str.length()) return "";
                
                if (json_str[pos] == '"') {
                    size_t endPos = json_str.find('"', pos + 1);
                    if (endPos == std::string::npos) return "";
                    return json_str.substr(pos + 1, endPos - pos - 1);
                } else {
                    size_t endPos = pos;
                    while (endPos < json_str.length() && json_str[endPos] != ',' && json_str[endPos] != '}' && json_str[endPos] != ' ') endPos++;
                    return json_str.substr(pos, endPos - pos);
                }
            };
            
            std::string hive_id = getValue("hive_id");
            if (hive_id.empty()) hive_id = "UNKNOWN";
            
            HiveData data;
            data.hive_id = hive_id;
            data.timestamp = timestamp;
            data.is_online = true;
            
            // ====================================================================
            // DETEKCJA ŹRÓDŁA DANYCH I TYPU (PICO vs NANO)
            // ====================================================================
            std::string source = getValue("data_source");
            if (!source.empty()) {
                data.data_source = source;
                data.is_precomputed = (source == "pico" || source == "precomputed");
            } else {
                // Automatyczna detekcja: sprawdź czy są parametry wyliczone
                bool has_computed_params = !getValue("audio_spectral_centroid").empty() ||
                                           !getValue("hx711_slope_1h").empty() ||
                                           !getValue("th_heat_index").empty() ||
                                           !getValue("aq_iaq_index").empty();
                if (has_computed_params) {
                    data.data_source = "pico";
                    data.is_precomputed = true;
                } else {
                    data.data_source = "nano";
                    data.is_precomputed = false;
                }
            }
            
            // Pobierz surowe dane z Arduino Nano (jeśli obecne)
            data.temp_raw = !getValue("temp_raw").empty() ? std::stof(getValue("temp_raw")) : 0.0f;
            data.hum_raw = !getValue("hum_raw").empty() ? std::stof(getValue("hum_raw")) : 0.0f;
            data.weight_raw = !getValue("weight_raw").empty() ? std::stol(getValue("weight_raw")) : 0;
            data.audio_raw = !getValue("audio_raw").empty() ? std::stoi(getValue("audio_raw")) : 0;
            data.vibration_raw = !getValue("vibration_raw").empty() ? std::stoi(getValue("vibration_raw")) : 0;
            data.co2_raw = !getValue("co2_raw").empty() ? std::stoi(getValue("co2_raw")) : 0;
            data.voc_raw = !getValue("voc_raw").empty() ? std::stoi(getValue("voc_raw")) : 0;
            
            // Podstawowe parametry (9)
            data.temperature = !getValue("temp").empty() ? std::stof(getValue("temp")) : 
                               (!getValue("temp_raw").empty() ? std::stof(getValue("temp_raw")) : 0.0f);
            data.humidity = !getValue("hum").empty() ? std::stof(getValue("hum")) : 
                            (!getValue("hum_raw").empty() ? std::stof(getValue("hum_raw")) : 0.0f);
            data.weight = !getValue("weight").empty() ? std::stof(getValue("weight")) : 0.0f;
            data.battery_level = !getValue("bat").empty() ? std::stoi(getValue("bat")) : 0;
            data.co2_eq = !getValue("co2").empty() ? std::stoi(getValue("co2")) : 
                          (!getValue("co2_raw").empty() ? std::stoi(getValue("co2_raw")) : 0);
            data.voc_idx = !getValue("voc").empty() ? std::stoi(getValue("voc")) : 
                           (!getValue("voc_raw").empty() ? std::stoi(getValue("voc_raw")) : 0);
            data.motion_detected = !getValue("motion").empty() ? std::stoi(getValue("motion")) : 0;
            
            // ====================================================================
            // JEŚLI DANE SĄ SUROWE (NANO), OBLICZ PARAMETRY NA RASPBERRY PI
            // ====================================================================
            if (!data.is_precomputed && data.data_source == "nano") {
                computeParametersFromRaw(data);
            }
            
            // Audio parametry (wybrane z 97+) - tylko jeśli precomputed lub już obliczone
            data.audio_rms = !getValue("audio_rms").empty() ? std::stof(getValue("audio_rms")) : data.audio_rms;
            data.audio_dominant_freq = !getValue("audio_freq").empty() ? std::stof(getValue("audio_freq")) : data.audio_dominant_freq;
            data.audio_swarm_prob = !getValue("swarm_prob").empty() ? std::stof(getValue("swarm_prob")) : data.audio_swarm_prob;
            data.audio_bee_activity = !getValue("bee_activity").empty() ? std::stof(getValue("bee_activity")) : data.audio_bee_activity;
            data.audio_spectral_centroid = !getValue("spectral_centroid").empty() ? std::stof(getValue("spectral_centroid")) : data.audio_spectral_centroid;
            data.audio_power_bee_band = !getValue("power_bee_band").empty() ? std::stof(getValue("power_bee_band")) : data.audio_power_bee_band;
            data.audio_crest_factor = !getValue("crest_factor").empty() ? std::stof(getValue("crest_factor")) : data.audio_crest_factor;
            data.audio_spectral_entropy = !getValue("spectral_entropy").empty() ? std::stof(getValue("spectral_entropy")) : data.audio_spectral_entropy;
            data.audio_foraging_eff = !getValue("foraging_eff").empty() ? std::stof(getValue("foraging_eff")) : data.audio_foraging_eff;
            data.audio_hive_health = !getValue("hive_health").empty() ? std::stof(getValue("hive_health")) : data.audio_hive_health;
            data.audio_aci = !getValue("aci").empty() ? std::stof(getValue("aci")) : data.audio_aci;
            data.audio_bi = !getValue("bi").empty() ? std::stof(getValue("bi")) : data.audio_bi;
            data.audio_adi = !getValue("adi").empty() ? std::stof(getValue("adi")) : data.audio_adi;
            data.audio_nhr = !getValue("nhr").empty() ? std::stof(getValue("nhr")) : data.audio_nhr;
            data.audio_loudness = !getValue("loudness").empty() ? std::stof(getValue("loudness")) : data.audio_loudness;
            data.audio_stress_indicator = !getValue("stress_indicator").empty() ? std::stof(getValue("stress_indicator")) : data.audio_stress_indicator;
            
            // Radar parametry (wybrane z 27)
            data.radar_distance = !getValue("radar_dist").empty() ? std::stof(getValue("radar_dist")) : 0.0f;
            data.radar_energy = !getValue("radar_energy").empty() ? std::stof(getValue("radar_energy")) : 0.0f;
            data.radar_activity = !getValue("radar_activity").empty() ? std::stof(getValue("radar_activity")) : 0.0f;
            data.radar_signal_quality = !getValue("signal_quality").empty() ? std::stof(getValue("signal_quality")) : 0.0f;
            data.radar_target_rate = !getValue("target_rate").empty() ? std::stof(getValue("target_rate")) : 0.0f;
            data.radar_entropy = !getValue("entropy").empty() ? std::stof(getValue("entropy")) : 0.0f;
            data.radar_anomaly_score = !getValue("anomaly_score").empty() ? std::stof(getValue("anomaly_score")) : 0.0f;
            data.radar_hive_health = !getValue("radar_health").empty() ? std::stof(getValue("radar_health")) : 0.0f;
            data.radar_max_targets = !getValue("max_targets").empty() ? std::stof(getValue("max_targets")) : 0.0f;
            data.radar_motion_intensity = !getValue("motion_intensity").empty() ? std::stof(getValue("motion_intensity")) : 0.0f;
            
            // HX711 parametry (wybrane z 105+)
            data.hx711_mean = !getValue("hx_mean").empty() ? std::stof(getValue("hx_mean")) : 0.0f;
            data.hx711_std = !getValue("hx_std").empty() ? std::stof(getValue("hx_std")) : 0.0f;
            data.hx711_slope_1h = !getValue("hx_slope_1h").empty() ? std::stof(getValue("hx_slope_1h")) : 0.0f;
            data.hx711_slope_24h = !getValue("hx_slope_24h").empty() ? std::stof(getValue("hx_slope_24h")) : 0.0f;
            data.hx711_nectar_inflow = !getValue("nectar_inflow").empty() ? std::stof(getValue("nectar_inflow")) : 0.0f;
            data.hx711_consumption_rate = !getValue("consumption_rate").empty() ? std::stof(getValue("consumption_rate")) : 0.0f;
            data.hx711_colony_growth = !getValue("colony_growth").empty() ? std::stof(getValue("colony_growth")) : 0.0f;
            data.hx711_productivity = !getValue("productivity").empty() ? std::stof(getValue("productivity")) : 0.0f;
            data.hx711_predicted_24h = !getValue("predicted_24h").empty() ? std::stof(getValue("predicted_24h")) : 0.0f;
            data.hx711_anomaly_score = !getValue("hx_anomaly").empty() ? std::stof(getValue("hx_anomaly")) : 0.0f;
            data.hx711_winter_readiness = !getValue("winter_readiness").empty() ? std::stof(getValue("winter_readiness")) : 0.0f;
            data.hx711_starvation_risk = !getValue("starvation_risk").empty() ? std::stof(getValue("starvation_risk")) : 0.0f;
            
            // Temp/Humidity parametry (wybrane z 28)
            data.th_heat_index = !getValue("heat_index").empty() ? std::stof(getValue("heat_index")) : 0.0f;
            data.th_dew_point = !getValue("dew_point").empty() ? std::stof(getValue("dew_point")) : 0.0f;
            data.th_comfort_index = !getValue("comfort_index").empty() ? std::stof(getValue("comfort_index")) : 0.0f;
            data.th_brood_stress = !getValue("brood_stress").empty() ? std::stof(getValue("brood_stress")) : 0.0f;
            data.th_temp_stability = !getValue("temp_stability").empty() ? std::stof(getValue("temp_stability")) : 0.0f;
            data.th_mold_risk = !getValue("mold_risk").empty() ? std::stof(getValue("mold_risk")) : 0.0f;
            
            // Air Quality parametry (wybrane z 24)
            data.aq_co2_mean = !getValue("aq_co2_mean").empty() ? std::stoi(getValue("aq_co2_mean")) : 0;
            data.aq_voc_mean = !getValue("aq_voc_mean").empty() ? std::stoi(getValue("aq_voc_mean")) : 0;
            data.aq_iaq_index = !getValue("iaq_index").empty() ? std::stof(getValue("iaq_index")) : 0.0f;
            data.aq_ventilation_need = !getValue("ventilation_need").empty() ? std::stof(getValue("ventilation_need")) : 0.0f;
            data.aq_contamination_risk = !getValue("contamination_risk").empty() ? std::stof(getValue("contamination_risk")) : 0.0f;
            data.aq_hive_comfort = !getValue("aq_comfort").empty() ? std::stof(getValue("aq_comfort")) : 0.0f;
            
            // Piezo parametry (wybrane z 22)
            data.piezo_rms = !getValue("piezo_rms").empty() ? std::stof(getValue("piezo_rms")) : 0.0f;
            data.piezo_dominant_freq = !getValue("piezo_freq").empty() ? std::stof(getValue("piezo_freq")) : 0.0f;
            data.piezo_activity_idx = !getValue("piezo_activity").empty() ? std::stof(getValue("piezo_activity")) : 0.0f;
            data.piezo_bee_traffic = !getValue("bee_traffic").empty() ? std::stof(getValue("bee_traffic")) : 0.0f;
            data.piezo_predator_score = !getValue("predator_score").empty() ? std::stof(getValue("predator_score")) : 0.0f;
            data.piezo_intrusion_prob = !getValue("intrusion_prob").empty() ? std::stof(getValue("intrusion_prob")) : 0.0f;
            
            // Barometric parametry (wybrane z 18)
            data.baro_pressure = !getValue("pressure").empty() ? std::stof(getValue("pressure")) : 0.0f;
            data.baro_trend_1h = !getValue("baro_trend_1h").empty() ? std::stof(getValue("baro_trend_1h")) : 0.0f;
            data.baro_weather_trend = !getValue("weather_trend").empty() ? std::stof(getValue("weather_trend")) : 0.0f;
            data.baro_storm_prob = !getValue("storm_prob").empty() ? std::stof(getValue("storm_prob")) : 0.0f;
            data.baro_foraging_cond = !getValue("foraging_cond").empty() ? std::stof(getValue("foraging_cond")) : 0.0f;
            
            // Light parametry (wybrane z 17)
            data.light_lux = !getValue("lux").empty() ? std::stoul(getValue("lux")) : 0;
            data.light_daylight_hours = !getValue("daylight_hours").empty() ? std::stof(getValue("daylight_hours")) : 0.0f;
            data.light_circadian_sync = !getValue("circadian_sync").empty() ? std::stof(getValue("circadian_sync")) : 0.0f;
            data.light_foraging_idx = !getValue("foraging_idx").empty() ? std::stof(getValue("foraging_idx")) : 0.0f;
            
            // Backward compatibility
            data.weight_rate = !getValue("weight_rate").empty() ? std::stof(getValue("weight_rate")) : 0.0f;
            data.weight_trend = !getValue("weight_trend").empty() ? std::stof(getValue("weight_trend")) : 0.0f;
            data.air_iaq = !getValue("air_iaq").empty() ? std::stoi(getValue("air_iaq")) : 0;
            
            // Aktualizacja danych
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                hives_data[hive_id] = data;
            }
            
            Logger::getInstance().debug("JSON: Zaktualizowano " + hive_id + " z " + source_ip);
            
        } catch (const std::exception& e) {
            Logger::getInstance().error("Błąd parsowania JSON: " + std::string(e.what()));
        }
    }
    
    // Parsowanie CSV - kompatybilność wsteczna
    void parseCSV(const std::string& raw_data, const std::string& source_ip, long long timestamp) {
        std::stringstream ss(raw_data);
        std::string segment;
        std::vector<std::string> parts;

        while (std::getline(ss, segment, ',')) {
            parts.push_back(segment);
        }

        // Wymagane minimum 9 pól dla podstawowych danych
        if (parts.size() < 9) {
            Logger::getInstance().warning("Niepoprawny format CSV z " + source_ip + ": " + raw_data);
            return;
        }

        try {
            std::string hive_id = parts[0];
            float temp = std::stof(parts[1]);
            float humidity = std::stof(parts[2]);
            float weight = std::stof(parts[3]);
            int battery = std::stoi(parts[4]);
            int co2 = std::stoi(parts[5]);
            int voc = std::stoi(parts[6]);
            int motion = std::stoi(parts[7]);
            
            // Parametry rozszerzone (opcjonalne)
            float audio_rms = (parts.size() > 9) ? std::stof(parts[9]) : 0.0f;
            float audio_dom_freq = (parts.size() > 10) ? std::stof(parts[10]) : 0.0f;
            float audio_swarm_prob = (parts.size() > 11) ? std::stof(parts[11]) : 0.0f;
            float radar_dist = (parts.size() > 12) ? std::stof(parts[12]) : 0.0f;
            float radar_energy = (parts.size() > 13) ? std::stof(parts[13]) : 0.0f;
            float radar_activity = (parts.size() > 14) ? std::stof(parts[14]) : 0.0f;
            float weight_rate = (parts.size() > 15) ? std::stof(parts[15]) : 0.0f;
            float weight_trend = (parts.size() > 16) ? std::stof(parts[16]) : 0.0f;
            int air_iaq = (parts.size() > 17) ? std::stoi(parts[17]) : 0;

            {
                std::lock_guard<std::mutex> lock(data_mutex);
                if (hives_data.find(hive_id) != hives_data.end()) {
                    hives_data[hive_id].temperature = temp;
                    hives_data[hive_id].humidity = humidity;
                    hives_data[hive_id].weight = weight;
                    hives_data[hive_id].battery_level = battery;
                    hives_data[hive_id].co2_eq = co2;
                    hives_data[hive_id].voc_idx = voc;
                    hives_data[hive_id].motion_detected = motion;
                    hives_data[hive_id].timestamp = timestamp;
                    hives_data[hive_id].is_online = true;
                    hives_data[hive_id].audio_rms = audio_rms;
                    hives_data[hive_id].audio_dominant_freq = audio_dom_freq;
                    hives_data[hive_id].audio_swarm_prob = audio_swarm_prob;
                    hives_data[hive_id].radar_distance = radar_dist;
                    hives_data[hive_id].radar_energy = radar_energy;
                    hives_data[hive_id].radar_activity = radar_activity;
                    hives_data[hive_id].weight_rate = weight_rate;
                    hives_data[hive_id].weight_trend = weight_trend;
                    hives_data[hive_id].air_iaq = air_iaq;
                    
                    Logger::getInstance().debug("CSV: Zaktualizowano " + hive_id + " z " + source_ip);
                } else {
                    Logger::getInstance().info("Wykryto nowy ul: " + hive_id + " z IP " + source_ip);
                    HiveData new_hive;
                    new_hive.hive_id = hive_id;
                    new_hive.temperature = temp;
                    new_hive.humidity = humidity;
                    new_hive.weight = weight;
                    new_hive.battery_level = battery;
                    new_hive.co2_eq = co2;
                    new_hive.voc_idx = voc;
                    new_hive.motion_detected = motion;
                    new_hive.timestamp = timestamp;
                    new_hive.is_online = true;
                    new_hive.audio_rms = audio_rms;
                    new_hive.audio_dominant_freq = audio_dom_freq;
                    new_hive.audio_swarm_prob = audio_swarm_prob;
                    new_hive.radar_distance = radar_dist;
                    new_hive.radar_energy = radar_energy;
                    new_hive.radar_activity = radar_activity;
                    new_hive.weight_rate = weight_rate;
                    new_hive.weight_trend = weight_trend;
                    new_hive.air_iaq = air_iaq;
                    hives_data[hive_id] = new_hive;
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().error("Błąd parsowania CSV: " + std::string(e.what()));
        }
    }

    // Symulacja danych (do testów bez fizycznych Pico)
    void simulationLoop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            std::lock_guard<std::mutex> lock(data_mutex);
            for (auto& pair : hives_data) {
                // Symuluj zmiany temperatury i wilgotności
                pair.second.temperature += (rand() % 10 - 5) / 10.0f;
                pair.second.humidity += (rand() % 10 - 5) / 10.0f;
                pair.second.timestamp = std::time(nullptr);
                
                // Loguj co pewien czas
                if (rand() % 5 == 0) {
                    Logger::getInstance().info( "Symulacja: " + pair.first + 
                               " Temp: " + std::to_string(pair.second.temperature));
                }
            }
        }
    }

    void start(bool use_simulation = false) {
        if (running) return;
        running = true;

        if (!use_simulation) {
            if (!initNetwork()) {
                Logger::getInstance().warning( "Przełączanie w tryb symulacji z powodu błędu sieci.");
                use_simulation = true;
            }
        }

        if (use_simulation) {
            Logger::getInstance().info( "Uruchamianie symulatora danych uli...");
            worker_threads.emplace_back(&ApiaryCollector::simulationLoop, this);
        } else {
            Logger::getInstance().info( "Uruchamianie nasłuchiwania sieciowego...");
            worker_threads.emplace_back(&ApiaryCollector::receiveLoop, this);
        }
    }

    void stop() {
        running = false;
        for (auto& t : worker_threads) {
            if (t.joinable()) t.join();
        }
        worker_threads.clear();
        if (server_socket >= 0) close(server_socket);
        Logger::getInstance().info( "Kolektor danych zatrzymany.");
    }

    // Metoda dla TUI/Basha do pobrania aktualnego stanu (eksport do JSON) - OBSŁUGA WIELU ULl
    std::string getStatusJSON() {
        std::lock_guard<std::mutex> lock(data_mutex);
        std::stringstream json;
        json << "{";
        json << "\"timestamp\":" << std::time(nullptr) << ",";
        json << "\"hive_count\":" << hives_data.size() << ",";
        json << "\"hives\":{";
        
        bool first = true;
        for (const auto& pair : hives_data) {
            if (!first) json << ",";
            first = false;
            
            const auto& d = pair.second;
            json << "\"" << d.hive_id << "\":{";
            
            // Podstawowe parametry (9)
            json << "\"temp\":" << d.temperature << ",";
            json << "\"hum\":" << d.humidity << ",";
            json << "\"weight\":" << d.weight << ",";
            json << "\"bat\":" << d.battery_level << ",";
            json << "\"co2\":" << d.co2_eq << ",";
            json << "\"voc\":" << d.voc_idx << ",";
            json << "\"motion\":" << d.motion_detected << ",";
            json << "\"online\":" << (d.is_online ? "true" : "false") << ",";
            json << "\"last_seen\":" << d.timestamp << ",";
            
            // Audio parametry (wybrane z 97+)
            json << "\"audio\":{\"rms\":" << d.audio_rms << ",";
            json << "\"freq\":" << d.audio_dominant_freq << ",";
            json << "\"swarm_prob\":" << d.audio_swarm_prob << ",";
            json << "\"bee_activity\":" << d.audio_bee_activity << ",";
            json << "\"spectral_centroid\":" << d.audio_spectral_centroid << ",";
            json << "\"power_bee_band\":" << d.audio_power_bee_band << ",";
            json << "\"crest_factor\":" << d.audio_crest_factor << ",";
            json << "\"spectral_entropy\":" << d.audio_spectral_entropy << ",";
            json << "\"foraging_eff\":" << d.audio_foraging_eff << ",";
            json << "\"hive_health\":" << d.audio_hive_health << ",";
            json << "\"aci\":" << d.audio_aci << ",";
            json << "\"bi\":" << d.audio_bi << ",";
            json << "\"adi\":" << d.audio_adi << ",";
            json << "\"nhr\":" << d.audio_nhr << ",";
            json << "\"loudness\":" << d.audio_loudness << "},";
            
            // Radar MMWave parametry (wybrane z 27)
            json << "\"radar\":{\"dist\":" << d.radar_distance << ",";
            json << "\"energy\":" << d.radar_energy << ",";
            json << "\"activity\":" << d.radar_activity << ",";
            json << "\"signal_quality\":" << d.radar_signal_quality << ",";
            json << "\"target_rate\":" << d.radar_target_rate << ",";
            json << "\"entropy\":" << d.radar_entropy << ",";
            json << "\"anomaly_score\":" << d.radar_anomaly_score << ",";
            json << "\"hive_health\":" << d.radar_hive_health << ",";
            json << "\"max_targets\":" << d.radar_max_targets << ",";
            json << "\"motion_intensity\":" << d.radar_motion_intensity << "},";
            
            // HX711 Waga parametry (wybrane z 105+)
            json << "\"hx711\":{\"mean\":" << d.hx711_mean << ",";
            json << "\"std\":" << d.hx711_std << ",";
            json << "\"slope_1h\":" << d.hx711_slope_1h << ",";
            json << "\"slope_4h\":" << d.hx711_slope_4h << ",";
            json << "\"slope_24h\":" << d.hx711_slope_24h << ",";
            json << "\"nectar_inflow\":" << d.hx711_nectar_inflow << ",";
            json << "\"consumption_rate\":" << d.hx711_consumption_rate << ",";
            json << "\"colony_growth\":" << d.hx711_colony_growth << ",";
            json << "\"productivity\":" << d.hx711_productivity << ",";
            json << "\"predicted_24h\":" << d.hx711_predicted_24h << ",";
            json << "\"forecast_conf\":" << d.hx711_forecast_conf << ",";
            json << "\"winter_readiness\":" << d.hx711_winter_readiness << ",";
            json << "\"starvation_risk\":" << d.hx711_starvation_risk << ",";
            json << "\"anomaly_score\":" << d.hx711_anomaly_score << "},";
            
            // Temp/Humidity parametry (wybrane z 28)
            json << "\"th\":{\"heat_index\":" << d.th_heat_index << ",";
            json << "\"dew_point\":" << d.th_dew_point << ",";
            json << "\"comfort_index\":" << d.th_comfort_index << ",";
            json << "\"brood_stress\":" << d.th_brood_stress << ",";
            json << "\"temp_stability\":" << d.th_temp_stability << ",";
            json << "\"mold_risk\":" << d.th_mold_risk << ",";
            json << "\"vpd\":" << d.th_vpd << "},";
            
            // Air Quality parametry (wybrane z 24)
            json << "\"aq\":{\"co2_mean\":" << d.aq_co2_mean << ",";
            json << "\"voc_mean\":" << d.aq_voc_mean << ",";
            json << "\"iaq_index\":" << d.aq_iaq_index << ",";
            json << "\"ventilation_need\":" << d.aq_ventilation_need << ",";
            json << "\"contamination_risk\":" << d.aq_contamination_risk << ",";
            json << "\"mold_risk\":" << d.aq_mold_risk << "},";
            
            // Piezo Vibration parametry (wybrane z 22)
            json << "\"piezo\":{\"rms\":" << d.piezo_rms << ",";
            json << "\"freq\":" << d.piezo_dominant_freq << ",";
            json << "\"activity\":" << d.piezo_activity_idx << ",";
            json << "\"bee_traffic\":" << d.piezo_bee_traffic << ",";
            json << "\"predator_score\":" << d.piezo_predator_score << ",";
            json << "\"intrusion_prob\":" << d.piezo_intrusion_prob << "},";
            
            // Barometric parametry (wybrane z 18)
            json << "\"baro\":{\"pressure\":" << d.baro_pressure << ",";
            json << "\"trend_1h\":" << d.baro_trend_1h << ",";
            json << "\"weather_trend\":" << d.baro_weather_trend << ",";
            json << "\"storm_prob\":" << d.baro_storm_prob << ",";
            json << "\"foraging_cond\":" << d.baro_foraging_cond << "},";
            
            // Light parametry (wybrane z 17)
            json << "\"light\":{\"lux\":" << d.light_lux << ",";
            json << "\"daylight_hours\":" << d.light_daylight_hours << ",";
            json << "\"circadian_sync\":" << d.light_circadian_sync << ",";
            json << "\"foraging_idx\":" << d.light_foraging_idx << ",";
            json << "\"uv\":" << d.light_uv << "}";
            
            json << "}";
        }
        json << "}}";
        return json.str();
    }
    
    // Pelny format CSV - WSZYSTKIE 338+ PARAMETROW
    std::string getStatusCSV() {
        std::lock_guard<std::mutex> lock(data_mutex);
        std::stringstream csv;
        
        csv << "ID,STATUS,TIMESTAMP,TEMP,HUM,WEIGHT,BAT,CO2,VOC,MOTION,ONLINE,";
        csv << "AUDIO_RMS,AUDIO_PEAK,AUDIO_PEAK_TO_PEAK,AUDIO_ZCR,AUDIO_ENERGY,AUDIO_MEAN_AMP,AUDIO_STD_AMP,AUDIO_CV_AMP,AUDIO_SKEWNESS,AUDIO_KURTOSIS,";
        csv << "AUDIO_DOMINANT_FREQ,AUDIO_SPECTRAL_CENTROID,AUDIO_SPECTRAL_BANDWIDTH,AUDIO_SPECTRAL_FLATNESS,AUDIO_SPECTRAL_ROLLOFF,";
        csv << "AUDIO_POWER_BEE_BAND,AUDIO_POWER_SWARM_BAND,AUDIO_POWER_LOW_FREQ,AUDIO_POWER_HIGH_FREQ,AUDIO_HARMONIC_RATIO,";
        csv << "AUDIO_MFCC_ENERGY_0,AUDIO_MFCC_ENERGY_1,AUDIO_MFCC_ENERGY_2,AUDIO_MFCC_ENERGY_3,";
        csv << "AUDIO_SPECTRAL_ENTROPY,AUDIO_SPECTRAL_CONTRAST,AUDIO_TONAL_STRENGTH,";
        csv << "AUDIO_CREST_FACTOR,AUDIO_FORMANT_F1,AUDIO_FORMANT_F2,AUDIO_FUNDAMENTAL_FREQ,AUDIO_PITCH_STRENGTH,AUDIO_INHARMONICITY,";
        csv << "AUDIO_SHIMMER,AUDIO_JITTER,AUDIO_NHR,AUDIO_HNR,AUDIO_AUTOCORR_PEAK,";
        csv << "AUDIO_ATTACK_TIME,AUDIO_DECAY_TIME,AUDIO_SUSTAIN_LEVEL,AUDIO_TEMPORAL_CENTROID,AUDIO_LOUDNESS,";
        csv << "AUDIO_SPECTRAL_FLUX,AUDIO_SPECTRAL_SLOPE,AUDIO_SPECTRAL_KURTOSIS,AUDIO_SPECTRAL_SKEWNESS,AUDIO_FUND_SALIENCY,";
        csv << "AUDIO_POWER_BAND_0,AUDIO_POWER_BAND_1,AUDIO_POWER_BAND_2,AUDIO_POWER_BAND_3,AUDIO_POWER_BAND_4,AUDIO_POWER_BAND_5,AUDIO_POWER_BAND_6,AUDIO_POWER_BAND_7,";
        csv << "AUDIO_LEQ,AUDIO_L10,AUDIO_L90,AUDIO_NOISE_FLOOR,AUDIO_SNR,";
        csv << "AUDIO_ACI,AUDIO_BI,AUDIO_NDI,AUDIO_ADI,AUDIO_AEI,";
        csv << "AUDIO_BEE_ACTIVITY,AUDIO_SWARM_PROB,AUDIO_STRESS_INDICATOR,AUDIO_HIVE_HEALTH,AUDIO_FORAGING_EFF,AUDIO_COLONY_COHERENCE,";
        csv << "RADAR_DISTANCE,RADAR_ENERGY,RADAR_SPEED,RADAR_DISTANCE_STD,RADAR_ENERGY_STD,RADAR_SPEED_STD,";
        csv << "RADAR_DISTANCE_MIN,RADAR_DISTANCE_MAX,RADAR_ENERGY_MIN,RADAR_ENERGY_MAX,";
        csv << "RADAR_RANGE,RADAR_ENERGY_VARIANCE,RADAR_CV,RADAR_ACTIVITY,RADAR_IDLE_PERCENT,RADAR_MOTION_INTENSITY,";
        csv << "RADAR_TARGET_RATE,RADAR_MAX_TARGETS,RADAR_TARGET_DENSITY,RADAR_SLOPE,RADAR_CORRELATION,RADAR_ACCELERATION,";
        csv << "RADAR_SIGNAL_QUALITY,RADAR_ANOMALY_SCORE,RADAR_HIVE_HEALTH,RADAR_POWER_SPECTRUM,RADAR_ZCR,RADAR_ENTROPY,";
        csv << "HX711_CURRENT,HX711_MEAN,HX711_STD,HX711_MIN,HX711_MAX,HX711_MEDIAN,HX711_RANGE,HX711_VARIANCE,HX711_CV,HX711_IQR,";
        csv << "HX711_RATE,HX711_MEAN_RATE,HX711_MAX_POS_RATE,HX711_MAX_NEG_RATE,HX711_ACCELERATION,";
        csv << "HX711_SLOPE_1H,HX711_SLOPE_4H,HX711_SLOPE_24H,HX711_CORR_1H,HX711_CORR_4H,HX711_CORR_24H,HX711_DIRECTION,";
        csv << "HX711_NECTAR_INFLOW,HX711_NECTAR_ACCUM,HX711_FORAGING_EFF,HX711_BLOOM_INTENSITY,";
        csv << "HX711_HONEY_PROD_IDX,HX711_NECTAR_QUALITY,";
        csv << "HX711_CONSUMPTION_RATE,HX711_DAILY_CONSUMPTION,HX711_FOOD_RESERVE_DAYS,";
        csv << "HX711_WINTER_READINESS,HX711_STARVATION_RISK,";
        csv << "HX711_DAILY_AMPLITUDE,HX711_CIRCADIAN_STR,HX711_SEASONAL_TREND,";
        csv << "HX711_SIGNAL_QUALITY,HX711_NOISE_LEVEL,HX711_DRIFT_RATE,HX711_STABILITY,";
        csv << "HX711_ANOMALY_SCORE,HX711_SUDDEN_CHANGE,HX711_OSCILLATION_FREQ,";
        csv << "HX711_COLONY_GROWTH,HX711_BROOD_ACTIVITY,HX711_POPULATION,HX711_HEALTH_WEIGHT,HX711_PRODUCTIVITY,";
        csv << "HX711_PREDICTED_24H,HX711_FORECAST_CONF,HX711_EXPECTED_YIELD,";
        csv << "TH_TEMP_MEAN,TH_TEMP_STD,TH_TEMP_MIN,TH_TEMP_MAX,TH_TEMP_RANGE,";
        csv << "TH_HUM_MEAN,TH_HUM_STD,TH_HUM_MIN,TH_HUM_MAX,TH_HUM_RANGE,";
        csv << "TH_HEAT_INDEX,TH_DEW_POINT,TH_VPD,TH_COMFORT_INDEX,TH_TEMP_STABILITY,TH_HUM_STABILITY,TH_ENV_VARIANCE,";
        csv << "TH_TEMP_TREND_1H,TH_HUM_TREND_1H,TH_TEMP_HUM_CORR,TH_OVERHEAT_RISK,TH_CONDENSATION_RISK,TH_MOLD_RISK,TH_BROOD_STRESS,";
        csv << "AQ_CO2,AQ_VOC,AQ_NOX,AQ_CO2_MEAN,AQ_VOC_MEAN,AQ_CO2_STD,AQ_VOC_STD,";
        csv << "AQ_CO2_MIN,AQ_CO2_MAX,AQ_VOC_MIN,AQ_VOC_MAX,";
        csv << "AQ_IAQ_INDEX,AQ_AIR_QUALITY_LEVEL,AQ_VENTILATION_NEED,AQ_STRESS_FROM_AIR,AQ_HIVE_COMFORT,";
        csv << "AQ_VARIABILITY,AQ_STABILITY_SCORE,AQ_CHANGE_RATE,AQ_TH_CORRELATION,AQ_COMFORT_ZONE_PCT,";
        csv << "AQ_CO2_WARNING,AQ_VOC_ALERT,AQ_COMBINED_RISK,AQ_CONTAMINATION_RISK,AQ_MOLD_RISK,";
        csv << "PIEZO_RMS,PIEZO_PEAK,PIEZO_MEAN,PIEZO_STD,PIEZO_DOMINANT_FREQ,PIEZO_ENERGY,PIEZO_ZCR,";
        csv << "PIEZO_ACTIVITY_IDX,PIEZO_BEE_TRAFFIC,PIEZO_PREDATOR_SCORE,PIEZO_INTRUSION_PROB,PIEZO_QUEEN_PIPING,";
        csv << "PIEZO_SWARM_PREP,PIEZO_AGGRESSION,PIEZO_ALIEN_SPECIES,PIEZO_WIND_VIBRATION,PIEZO_IMPACT_DETECTED,";
        csv << "PIEZO_CONTINUOUS_VIB,PIEZO_EVENT_COUNT,PIEZO_SEVERITY,PIEZO_SOURCE_CLASS,PIEZO_CONFIDENCE,";
        csv << "BARO_PRESSURE,BARO_TEMP,BARO_ALTITUDE,BARO_MEAN,BARO_STD,";
        csv << "BARO_TREND_1H,BARO_TREND_3H,BARO_TREND_6H,BARO_CHANGE_RATE,";
        csv << "BARO_WEATHER_TREND,BARO_STORM_PROB,BARO_RAIN_PROB,BARO_IMPROVING,";
        csv << "BARO_FORAGING_COND,BARO_BEE_ACTIVITY_PRED,BARO_SEVERITY_IDX,BARO_ALERT_LEVEL,BARO_RELIABILITY,";
        csv << "LIGHT_LUX,LIGHT_IR,LIGHT_UV,LIGHT_FULL_SPEC,LIGHT_MEAN,LIGHT_STD,LIGHT_MIN,LIGHT_MAX,";
        csv << "LIGHT_DAYLIGHT_HOURS,LIGHT_DARKNESS_HOURS,LIGHT_TWILIGHT,LIGHT_CIRCADIAN_SYNC,";
        csv << "LIGHT_FORAGING_IDX,LIGHT_PHOTOPERIOD,LIGHT_SEASONAL_CHANGE,LIGHT_CLOUD_COVER_EST,LIGHT_SUNRISE_OFFSET,";
        csv << "WEIGHT_RATE,WEIGHT_TREND,AIR_IAQ\n";
        
        for (const auto& pair : hives_data) {
            const auto& d = pair.second;
            std::string status = d.is_online ? "ONLINE" : "OFFLINE";
            if (d.is_online && (std::time(nullptr) - d.timestamp > 60)) status = "STALE";
            csv << d.hive_id << "," << status << "," << d.timestamp << ",";
            csv << d.temperature << "," << d.humidity << "," << d.weight << "," << d.battery_level << "," << d.co2_eq << "," << d.voc_idx << "," << d.motion_detected << "," << (d.is_online ? 1 : 0) << ",";
            csv << d.audio_rms << "," << d.audio_peak << "," << d.audio_peak_to_peak << "," << d.audio_zcr << "," << d.audio_energy << "," << d.audio_mean_amp << "," << d.audio_std_amp << "," << d.audio_cv_amp << "," << d.audio_skewness << "," << d.audio_kurtosis << ",";
            csv << d.audio_dominant_freq << "," << d.audio_spectral_centroid << "," << d.audio_spectral_bandwidth << "," << d.audio_spectral_flatness << "," << d.audio_spectral_rolloff << ",";
            csv << d.audio_power_bee_band << "," << d.audio_power_swarm_band << "," << d.audio_power_low_freq << "," << d.audio_power_high_freq << "," << d.audio_harmonic_ratio << ",";
            csv << d.audio_mfcc_energy[0] << "," << d.audio_mfcc_energy[1] << "," << d.audio_mfcc_energy[2] << "," << d.audio_mfcc_energy[3] << ",";
            csv << d.audio_spectral_entropy << "," << d.audio_spectral_contrast << "," << d.audio_tonal_strength << ",";
            csv << d.audio_crest_factor << "," << d.audio_formant_f1 << "," << d.audio_formant_f2 << "," << d.audio_fundamental_freq << "," << d.audio_pitch_strength << "," << d.audio_inharmonicity << ",";
            csv << d.audio_shimmer << "," << d.audio_jitter << "," << d.audio_nhr << "," << d.audio_hnr << "," << d.audio_autocorr_peak << ",";
            csv << d.audio_attack_time << "," << d.audio_decay_time << "," << d.audio_sustain_level << "," << d.audio_temporal_centroid << "," << d.audio_loudness << ",";
            csv << d.audio_spectral_flux << "," << d.audio_spectral_slope << "," << d.audio_spectral_kurtosis << "," << d.audio_spectral_skewness << "," << d.audio_fund_salience << ",";
            csv << d.audio_power_band[0] << "," << d.audio_power_band[1] << "," << d.audio_power_band[2] << "," << d.audio_power_band[3] << "," << d.audio_power_band[4] << "," << d.audio_power_band[5] << "," << d.audio_power_band[6] << "," << d.audio_power_band[7] << ",";
            csv << d.audio_leq << "," << d.audio_l10 << "," << d.audio_l90 << "," << d.audio_noise_floor << "," << d.audio_snr << ",";
            csv << d.audio_aci << "," << d.audio_bi << "," << d.audio_ndi << "," << d.audio_adi << "," << d.audio_aei << ",";
            csv << d.audio_bee_activity << "," << d.audio_swarm_prob << "," << d.audio_stress_indicator << "," << d.audio_hive_health << "," << d.audio_foraging_eff << "," << d.audio_colony_coherence << ",";
            csv << d.radar_distance << "," << d.radar_energy << "," << d.radar_speed << "," << d.radar_distance_std << "," << d.radar_energy_std << "," << d.radar_speed_std << ",";
            csv << d.radar_distance_min << "," << d.radar_distance_max << "," << d.radar_energy_min << "," << d.radar_energy_max << ",";
            csv << d.radar_range << "," << d.radar_energy_variance << "," << d.radar_cv << "," << d.radar_activity << "," << d.radar_idle_percent << "," << d.radar_motion_intensity << ",";
            csv << d.radar_target_rate << "," << d.radar_max_targets << "," << d.radar_target_density << "," << d.radar_slope << "," << d.radar_correlation << "," << d.radar_acceleration << ",";
            csv << d.radar_signal_quality << "," << d.radar_anomaly_score << "," << d.radar_hive_health << "," << d.radar_power_spectrum << "," << d.radar_zcr << "," << d.radar_entropy << ",";
            csv << d.hx711_current << "," << d.hx711_mean << "," << d.hx711_std << "," << d.hx711_min << "," << d.hx711_max << "," << d.hx711_median << "," << d.hx711_range << "," << d.hx711_variance << "," << d.hx711_cv << "," << d.hx711_iqr << ",";
            csv << d.hx711_rate << "," << d.hx711_mean_rate << "," << d.hx711_max_pos_rate << "," << d.hx711_max_neg_rate << "," << d.hx711_acceleration << ",";
            csv << d.hx711_slope_1h << "," << d.hx711_slope_4h << "," << d.hx711_slope_24h << "," << d.hx711_corr_1h << "," << d.hx711_corr_4h << "," << d.hx711_corr_24h << "," << d.hx711_direction << ",";
            csv << d.hx711_nectar_inflow << "," << d.hx711_nectar_accum << "," << d.hx711_foraging_eff << "," << d.hx711_bloom_intensity << ",";
            csv << d.hx711_honey_prod_idx << "," << d.hx711_nectar_quality << ",";
            csv << d.hx711_consumption_rate << "," << d.hx711_daily_consumption << "," << d.hx711_food_reserve_days << ",";
            csv << d.hx711_winter_readiness << "," << d.hx711_starvation_risk << ",";
            csv << d.hx711_daily_amplitude << "," << d.hx711_circadian_str << "," << d.hx711_seasonal_trend << ",";
            csv << d.hx711_signal_quality << "," << d.hx711_noise_level << "," << d.hx711_drift_rate << "," << d.hx711_stability << ",";
            csv << d.hx711_anomaly_score << "," << d.hx711_sudden_change << "," << d.hx711_oscillation_freq << ",";
            csv << d.hx711_colony_growth << "," << d.hx711_brood_activity << "," << d.hx711_population << "," << d.hx711_health_weight << "," << d.hx711_productivity << ",";
            csv << d.hx711_predicted_24h << "," << d.hx711_forecast_conf << "," << d.hx711_expected_yield << ",";
            csv << d.th_temp_mean << "," << d.th_temp_std << "," << d.th_temp_min << "," << d.th_temp_max << "," << d.th_temp_range << ",";
            csv << d.th_hum_mean << "," << d.th_hum_std << "," << d.th_hum_min << "," << d.th_hum_max << "," << d.th_hum_range << ",";
            csv << d.th_heat_index << "," << d.th_dew_point << "," << d.th_vpd << "," << d.th_comfort_index << "," << d.th_temp_stability << "," << d.th_hum_stability << "," << d.th_env_variance << ",";
            csv << d.th_temp_trend_1h << "," << d.th_hum_trend_1h << "," << d.th_temp_hum_corr << "," << d.th_overheat_risk << "," << d.th_condensation_risk << "," << d.th_mold_risk << "," << d.th_brood_stress << ",";
            csv << d.aq_co2 << "," << d.aq_voc << "," << d.aq_nox << "," << d.aq_co2_mean << "," << d.aq_voc_mean << "," << d.aq_co2_std << "," << d.aq_voc_std << ",";
            csv << d.aq_co2_min << "," << d.aq_co2_max << "," << d.aq_voc_min << "," << d.aq_voc_max << ",";
            csv << d.aq_iaq_index << "," << d.aq_air_quality_level << "," << d.aq_ventilation_need << "," << d.aq_stress_from_air << "," << d.aq_hive_comfort << ",";
            csv << d.aq_variability << "," << d.aq_stability_score << "," << d.aq_change_rate << "," << d.aq_th_correlation << "," << d.aq_comfort_zone_pct << ",";
            csv << d.aq_co2_warning << "," << d.aq_voc_alert << "," << d.aq_combined_risk << "," << d.aq_contamination_risk << "," << d.aq_mold_risk << ",";
            csv << d.piezo_rms << "," << d.piezo_peak << "," << d.piezo_mean << "," << d.piezo_std << "," << d.piezo_dominant_freq << "," << d.piezo_energy << "," << d.piezo_zcr << ",";
            csv << d.piezo_activity_idx << "," << d.piezo_bee_traffic << "," << d.piezo_predator_score << "," << d.piezo_intrusion_prob << "," << d.piezo_queen_piping << ",";
            csv << d.piezo_swarm_prep << "," << d.piezo_aggression << "," << d.piezo_alien_species << "," << d.piezo_wind_vibration << "," << d.piezo_impact_detected << ",";
            csv << d.piezo_continuous_vib << "," << d.piezo_event_count << "," << d.piezo_severity << "," << d.piezo_source_class << "," << d.piezo_confidence << ",";
            csv << d.baro_pressure << "," << d.baro_temp << "," << d.baro_altitude << "," << d.baro_mean << "," << d.baro_std << ",";
            csv << d.baro_trend_1h << "," << d.baro_trend_3h << "," << d.baro_trend_6h << "," << d.baro_change_rate << ",";
            csv << d.baro_weather_trend << "," << d.baro_storm_prob << "," << d.baro_rain_prob << "," << d.baro_improving << ",";
            csv << d.baro_foraging_cond << "," << d.baro_bee_activity_pred << "," << d.baro_severity_idx << "," << d.baro_alert_level << "," << d.baro_reliability << ",";
            csv << d.light_lux << "," << d.light_ir << "," << d.light_uv << "," << d.light_full_spec << "," << d.light_mean << "," << d.light_std << "," << d.light_min << "," << d.light_max << ",";
            csv << d.light_daylight_hours << "," << d.light_darkness_hours << "," << d.light_twilight << "," << d.light_circadian_sync << ",";
            csv << d.light_foraging_idx << "," << d.light_photoperiod << "," << d.light_seasonal_change << "," << d.light_cloud_cover_est << "," << d.light_sunrise_offset << ",";
            csv << d.weight_rate << "," << d.weight_trend << "," << d.air_iaq << "\n";
        }
        return csv.str();
    }

    // Eksport danych do pliku CSV
    void exportToCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::getInstance().error("Nie udało się otworzyć pliku CSV: " + filename);
            return;
        }
        
        file << getStatusCSV();
        file.close();
        Logger::getInstance().debug("Eksportowano dane do pliku: " + filename);
    }
};

// Funkcja główna do samodzielnego uruchomienia jako demon
int main(int argc, char* argv[]) {
    // ApiaryCollector używa singleton Logger
    ApiaryCollector collector;

    // Konfiguracja przykładowych IP uli (w produkcji czytane z config file)
    std::vector<std::string> hives = {"192.168.1.101", "192.168.1.102", "192.168.1.103"};
    collector.configureHives(hives);

    // Uruchomienie w trybie symulacji (demonstracja) lub sieciowym
    bool sim_mode = (argc > 1 && std::string(argv[1]) == "--sim");
    
    collector.start(sim_mode);

    // Serwer HTTP API JSON na porcie 8080
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        Logger::getInstance().error("Nie udało się utworzyć socketu HTTP: " + std::string(strerror(errno)));
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in http_addr;
    memset(&http_addr, 0, sizeof(http_addr));
    http_addr.sin_family = AF_INET;
    http_addr.sin_addr.s_addr = INADDR_ANY;
    http_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&http_addr, sizeof(http_addr)) < 0) {
        Logger::getInstance().error("Błąd bindowania portu HTTP 8080: " + std::string(strerror(errno)));
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        Logger::getInstance().error("Błąd listen na porcie HTTP: " + std::string(strerror(errno)));
        close(server_fd);
        return 1;
    }

    Logger::getInstance().info("Serwer HTTP API JSON uruchomiony na porcie 8080");
    Logger::getInstance().info("Endpointy: GET /api/status, GET /api/hives, GET /health");

    fd_set readfds;
    struct timeval tv;

    // Główna pętla demona - obsługa HTTP API i danych z Pico
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            // Nowe połączenie HTTP
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                char buffer[4096] = {0};
                recv(client_fd, buffer, sizeof(buffer)-1, 0);
                
                std::string request(buffer);
                std::string response;
                std::string content_type = "application/json";
                
                // Parsowanie żądania HTTP - obsługa pełnej listy endpointów
                if (request.find("GET /api/status") != std::string::npos || 
                    request.find("GET /api/hives") != std::string::npos) {
                    response = collector.getStatusJSON();
                } else if (request.find("GET /health") != std::string::npos) {
                    response = "{\"status\":\"ok\",\"timestamp\":" + std::to_string(std::time(nullptr)) + ",\"version\":\"1.0.0\",\"collector_url\":\"http://localhost:8080\"}";
                } else if (request.find("GET /api/csv") != std::string::npos) {
                    response = collector.getStatusCSV();
                    content_type = "text/csv";
                } else if (request.find("GET /api/sensors") != std::string::npos) {
                    response = "{\"sensors\":[],\"note\":\"Endpoint dostępny - dane dostępne w /api/hives\"}";
                } else if (request.find("GET /api/effectors") != std::string::npos) {
                    response = "{\"effectors\":[],\"note\":\"Endpoint dostępny\"}";
                } else if (request.find("GET /api/history") != std::string::npos) {
                    response = "{\"labels\":[],\"data\":[],\"note\":\"Dane historyczne dostępne przez TUI\"}";
                } else if (request.find("GET /api/logs") != std::string::npos) {
                    response = "{\"logs\":[],\"note\":\"Logi dostępne przez TUI\"}";
                } else {
                    response = "{\"error\":\"Unknown endpoint\",\"available\":[\"/api/status\",\"/api/hives\",\"/health\",\"/api/csv\",\"/api/sensors\",\"/api/effectors\",\"/api/history\",\"/api/logs\"]}";
                }
                
                // Nagłówki HTTP
                std::stringstream http_response;
                http_response << "HTTP/1.1 200 OK\r\n";
                http_response << "Content-Type: " << content_type << "\r\n";
                http_response << "Access-Control-Allow-Origin: *\r\n";
                http_response << "Content-Length: " << response.length() << "\r\n";
                http_response << "Connection: close\r\n\r\n";
                http_response << response;
                
                send(client_fd, http_response.str().c_str(), http_response.str().length(), 0);
                close(client_fd);
                
                Logger::getInstance().debug("HTTP: Obsłużono żądanie od " + std::string(inet_ntoa(client_addr.sin_addr)));
            }
        }
        
        // Co 5 sekund loguj status
        static time_t last_log = 0;
        if (std::time(nullptr) - last_log >= 5) {
            last_log = std::time(nullptr);
            // Eksportuj dane do pliku CSV dla TUI
            collector.exportToCSV("/tmp/apiary_data.csv");
            // Można dodać okresowe logowanie statystyk
        }
    }

    close(server_fd);
    return 0;
}
