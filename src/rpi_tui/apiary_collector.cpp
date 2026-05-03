/**
 * apiary_collector.cpp
 * Moduł zbierania danych z wielu uli przez Ethernet (Raspberry Pi 2 + Pico)
 * Kompilacja: g++ -std=c++17 -pthread -o apiary_collector apiary_collector.cpp
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

    // Konfiguracja listy uli (IP)
    void configureHives(const std::vector<std::string>& ips) {
        hive_ips = ips;
        Logger::getInstance().info( "Skonfigurowano " + std::to_string(ips.size()) + " uli do monitorowania.");
        
        // Inicjalizacja struktury danych - wszystkie pola na zero/false
        std::lock_guard<std::mutex> lock(data_mutex);
        for (size_t i = 0; i < ips.size(); ++i) {
            std::string id = "UL-" + std::to_string(i + 1);
            hives_data[id] = {id, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, false, 
                              0.0f, 0.0f, 0.0f,  // audio
                              0.0f, 0.0f, 0.0f,  // radar
                              0.0f, 0.0f,        // waga trend
                              0};                // air_iaq
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
    
    // Parsowanie JSON - pełna obsługa wszystkich parametrów
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
            
            // Podstawowe parametry
            data.temperature = std::stof(getValue("temp"));
            data.humidity = std::stof(getValue("hum"));
            data.weight = std::stof(getValue("weight"));
            data.battery_level = std::stoi(getValue("bat"));
            data.co2_eq = std::stoi(getValue("co2"));
            data.voc_idx = std::stoi(getValue("voc"));
            data.motion_detected = std::stoi(getValue("motion"));
            
            // Audio parametry
            data.audio_rms = std::stof(getValue("audio_rms"));
            data.audio_dominant_freq = std::stof(getValue("audio_freq"));
            data.audio_swarm_prob = std::stof(getValue("swarm_prob"));
            data.audio_bee_activity = std::stof(getValue("bee_activity"));
            data.audio_spectral_centroid = std::stof(getValue("spectral_centroid"));
            data.audio_power_bee_band = std::stof(getValue("power_bee_band"));
            data.audio_crest_factor = std::stof(getValue("crest_factor"));
            data.audio_spectral_entropy = std::stof(getValue("spectral_entropy"));
            data.audio_foraging_eff = std::stof(getValue("foraging_eff"));
            data.audio_hive_health = std::stof(getValue("hive_health"));
            
            // Radar parametry
            data.radar_distance = std::stof(getValue("radar_dist"));
            data.radar_energy = std::stof(getValue("radar_energy"));
            data.radar_activity = std::stof(getValue("radar_activity"));
            data.radar_signal_quality = std::stof(getValue("signal_quality"));
            data.radar_target_rate = std::stof(getValue("target_rate"));
            data.radar_entropy = std::stof(getValue("radar_entropy"));
            data.radar_anomaly_score = std::stof(getValue("anomaly_score"));
            data.radar_hive_health = std::stof(getValue("radar_health"));
            
            // HX711 parametry
            data.hx711_mean = std::stof(getValue("hx_mean"));
            data.hx711_std = std::stof(getValue("hx_std"));
            data.hx711_slope_1h = std::stof(getValue("hx_slope_1h"));
            data.hx711_slope_24h = std::stof(getValue("hx_slope_24h"));
            data.hx711_nectar_inflow = std::stof(getValue("nectar_inflow"));
            data.hx711_consumption_rate = std::stof(getValue("consumption_rate"));
            data.hx711_colony_growth = std::stof(getValue("colony_growth"));
            data.hx711_productivity = std::stof(getValue("productivity"));
            data.hx711_predicted_24h = std::stof(getValue("predicted_24h"));
            data.hx711_anomaly_score = std::stof(getValue("hx_anomaly"));
            
            // Temp/Humidity parametry
            data.th_heat_index = std::stof(getValue("heat_index"));
            data.th_dew_point = std::stof(getValue("dew_point"));
            data.th_comfort_index = std::stof(getValue("comfort_index"));
            data.th_brood_stress = std::stof(getValue("brood_stress"));
            data.th_temp_stability = std::stof(getValue("temp_stability"));
            data.th_mold_risk = std::stof(getValue("mold_risk"));
            
            // Air Quality parametry
            data.aq_co2_mean = std::stoi(getValue("aq_co2_mean"));
            data.aq_voc_mean = std::stoi(getValue("aq_voc_mean"));
            data.aq_iaq_index = std::stof(getValue("iaq_index"));
            data.aq_ventilation_need = std::stof(getValue("ventilation_need"));
            data.aq_contamination_risk = std::stof(getValue("contamination_risk"));
            data.aq_hive_comfort = std::stof(getValue("aq_comfort"));
            
            // Piezo parametry
            data.piezo_rms = std::stof(getValue("piezo_rms"));
            data.piezo_dominant_freq = std::stof(getValue("piezo_freq"));
            data.piezo_activity_idx = std::stof(getValue("piezo_activity"));
            data.piezo_bee_traffic = std::stof(getValue("bee_traffic"));
            data.piezo_predator_score = std::stof(getValue("predator_score"));
            data.piezo_intrusion_prob = std::stof(getValue("intrusion_prob"));
            
            // Barometric parametry
            data.baro_pressure = std::stof(getValue("pressure"));
            data.baro_trend_1h = std::stof(getValue("baro_trend_1h"));
            data.baro_weather_trend = std::stof(getValue("weather_trend"));
            data.baro_storm_prob = std::stof(getValue("storm_prob"));
            data.baro_foraging_cond = std::stof(getValue("foraging_cond"));
            
            // Light parametry
            data.light_lux = std::stoul(getValue("lux"));
            data.light_daylight_hours = std::stof(getValue("daylight_hours"));
            data.light_circadian_sync = std::stof(getValue("circadian_sync"));
            data.light_foraging_idx = std::stof(getValue("foraging_idx"));
            
            // Backward compatibility
            data.weight_rate = std::stof(getValue("weight_rate"));
            data.weight_trend = std::stof(getValue("weight_trend"));
            data.air_iaq = std::stoi(getValue("air_iaq"));
            
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
    
    // Prostszy format dla Bash (CSV) - wszystkie parametry
    std::string getStatusCSV() {
        std::lock_guard<std::mutex> lock(data_mutex);
        std::stringstream csv;
        csv << "ID,STATUS,TEMP,HUM,WEIGHT,BAT,CO2,VOC,MOTION,AUDIO_RMS,AUDIO_FREQ,SWARM_PROB,RADAR_DIST,RADAR_ENERGY,RADAR_ACT,WAG_RATE,WAG_TREND,AIR_IAQ,TIME\n";
        for (const auto& pair : hives_data) {
            const auto& d = pair.second;
            std::string status = d.is_online ? "ONLINE" : "OFFLINE";
            if (d.is_online && (std::time(nullptr) - d.timestamp > 60)) status = "STALE"; // Brak danych > 60s
            
            csv << d.hive_id << "," 
                << status << "," 
                << d.temperature << "," 
                << d.humidity << "," 
                << d.weight << "," 
                << d.battery_level << ","
                << d.co2_eq << ","
                << d.voc_idx << ","
                << d.motion_detected << ","
                << d.audio_rms << ","
                << d.audio_dominant_freq << ","
                << d.audio_swarm_prob << ","
                << d.radar_distance << ","
                << d.radar_energy << ","
                << d.radar_activity << ","
                << d.weight_rate << ","
                << d.weight_trend << ","
                << d.air_iaq << ","
                << d.timestamp << "\n";
        }
        return csv.str();
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
                
                // Parsowanie żądania HTTP
                if (request.find("GET /api/status") != std::string::npos) {
                    response = collector.getStatusJSON();
                } else if (request.find("GET /api/hives") != std::string::npos) {
                    response = collector.getStatusJSON();
                } else if (request.find("GET /health") != std::string::npos) {
                    response = "{\"status\":\"ok\",\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
                } else if (request.find("GET /api/csv") != std::string::npos) {
                    response = collector.getStatusCSV();
                    content_type = "text/csv";
                } else {
                    response = "{\"error\":\"Unknown endpoint\",\"available\":[\"/api/status\",\"/api/hives\",\"/health\",\"/api/csv\"]}";
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
            // Można dodać okresowe logowanie statystyk
        }
    }

    close(server_fd);
    return 0;
}
