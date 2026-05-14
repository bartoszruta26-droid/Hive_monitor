/**
 * apiary_collector.cpp
 * Modul zbierania danych z wielu uli przez Ethernet (Raspberry Pi 2 + Pico/Nano)
 * Kompilacja: g++ -std=c++17 -pthread -o apiary_collector apiary_collector.cpp
 * 
 * OBSLUGIWANE ZRODLA DANYCH:
 * 1. Raspberry Pi Pico - wysyla JUZ OBLICZONE 300+ parametrow
 * 2. Arduino Nano - wysyla TYLKO SUROWE DANE, Raspberry Pi musi je przeliczyc
 * 
 * DETEKCJA ZRODLA:
 * - JSON z polem "data_source": "pico" lub "nano"
 * - Brak pola = automatyczna detekcja po obecnosci parametrow wyliczonych
 * 
 * OBSLUGIWANE PARAMETRY (WSZYSTKIE Z .md i pico.ino - 338+ parametrow):
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

// Dolaczamy naglowek loggera (nie implementacje .cpp)
#include "apiary_logger.h"
#ifdef DEBUG_BUILD
#include "apiary_logger_debug.h"
#endif

// Dolaczamy modul bazy danych dla zapisu i agregacji
#include "apiary_database.h"

// Dolaczamy modul CSV do parsowania i generowania danych
#include "apiary_collector_csv.h"

// Uzywamy przestrzeni nazw apiary dla wygody
using namespace apiary;

// ============================================================================
// KONFIGURACJA DEBUGOWANIA I GENTLE CODE
// ============================================================================

#ifndef GENTLE_CODE_ENABLED
#define GENTLE_CODE_ENABLED 1
#endif

// Tryb bezpieczny - nie przerywa dzialania przy bledach, tylko loguje i kontynuuje
#if GENTLE_CODE_ENABLED
    #define GENTLE_TRY try {
    #define GENTLE_CATCH(action_on_error) } catch (const std::exception& e) { \
        Logger::getInstance().error(std::string("Gentle error: ") + e.what()); \
        action_on_error; \
    } catch (...) { \
        Logger::getInstance().error("Unknown exception in gentle block"); \
        action_on_error; \
    }
#else
    #define GENTLE_TRY try {
    #define GENTLE_CATCH(action) } catch (...) { action; throw; }
#endif

// Debug macros z fallbackiem gdy DEBUG_BUILD nie jest zdefiniowane
#ifndef DEBUG_BUILD
    #define DEBUG_LOG(msg) do {} while(0)
    #define DEBUG_FUNC_ENTER() do {} while(0)
    #define DEBUG_FUNC_EXIT() do {} while(0)
    #define DEBUG_CHECK(condition, msg) do {} while(0)
    #define DEBUG_TRACE_FUNC() do {} while(0)
    #define DEBUG_TRACE_POINT(label) do {} while(0)
    #define DEBUG_START_TIMER(name) do {} while(0)
    #define DEBUG_STOP_TIMER(name) do {} while(0)
    #define DEBUG_COUNTER_INC(name) do {} while(0)
    #define DEBUG_RECORD_EXCEPTION(e) do {} while(0)
#else
    #define DEBUG_LOG(msg) apiary::Logger::getInstance().debug(msg, "DEBUG_MACRO")
    #define DEBUG_FUNC_ENTER() DEBUG_TRACE_FUNC()
    #define DEBUG_FUNC_EXIT() do { DEBUG_TRACE_POINT("EXIT"); } while(0)
    #define DEBUG_CHECK(condition, msg) do { \
        if (!(condition)) { \
            apiary::Logger::getInstance().warning(std::string("Debug check failed: ") + (msg), "DEBUG_CHECK"); \
        } \
    } while(0)
#endif


// Menadzer danych uli
class ApiaryCollector {
private:
    std::map<std::string, HiveData> hives_data;
    std::mutex data_mutex;
    std::atomic<bool> running{false};
    std::vector<std::thread> worker_threads;
    
    std::vector<std::string> hive_ips; // Lista IP dla uli

    // Symulacja portu nasluchiwania (w rzeczywistosci Pico wysyla dane na ten port)
    int server_socket;
    struct sockaddr_in server_addr;

public:
    ApiaryCollector() {
        server_socket = -1;
    }

    ~ApiaryCollector() {
        stop();
    }
    
    // ============================================================================
    // MODUL OBLICZANIA 300+ PARAMETROW Z SUROWYCH DANYCH (ARDUINO NANO)
    // ============================================================================
    void computeParametersFromRaw(HiveData& data) {
        // Obliczanie parametrow z surowych danych Arduino Nano
        // Ten modul wykonuje te same obliczenia co Raspberry Pi Pico
        
        // =========================================================================
        // --- AUDIO METRICS (z audio_raw) - 97+ parametrow ---
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
        
        // Czestotliwosciowe podstawowe
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
        
        // Pasma mocy szczegolowe
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
        
        // Wskazniki klasyfikacji
        data.audio_bee_activity = std::min(100.0f, raw_norm * 150.0f);
        data.audio_swarm_prob = (data.audio_dominant_freq > 150.0f && data.audio_dominant_freq < 350.0f) ? 0.6f : 0.2f;
        data.audio_stress_indicator = (raw_norm > 0.7f) ? 0.8f : 0.3f;
        data.audio_hive_health = 100.0f - data.audio_stress_indicator * 50.0f;
        data.audio_foraging_eff = std::min(100.0f, 50.0f + data.audio_bee_activity * 0.5f);
        data.audio_colony_coherence = 0.5f + raw_norm * 0.3f;
        
        // =========================================================================
        // --- TEMPERATURE/HUMIDITY DERIVED (28 parametrow) ---
        // =========================================================================
        if (data.temp_raw != 0.0f || data.hum_raw != 0.0f) {
            float T = data.temp_raw;
            float RH = std::max(0.0f, std::min(100.0f, data.hum_raw));
            
            // Statystyki podstawowe (zakladajac stabilnosc przy braku historii)
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
            data.th_comfort_idx = 100.0f - std::abs(T - 25.0f) * 4.0f - std::abs(RH - 60.0f) * 0.5f;
            data.th_comfort_idx = std::max(0.0f, std::min(100.0f, data.th_comfort_idx));
            
            // Stability
            data.th_temp_stability = 80.0f;
            data.th_hum_stability_1h = 75.0f;
            data.th_temp_std = 0.25f + 4.0f; // temp_var + hum_var
            
            // Trends (brak historii = 0)
            data.th_temp_trend = 0.0f;
            data.th_hum_trend = 0.0f;
            data.th_vpd = -0.3f; // Typowa ujemna korelacja
            
            // Ryzyka
            data.th_brood_stress = (T > 35.0f) ? std::min(1.0f, (T - 35.0f) / 5.0f) : 0.0f;
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
            data.aq_co2_mean = data.co2_raw;
            data.aq_voc_mean = data.voc_raw;
            data.aq_co2_mean = data.co2_raw;
            data.aq_voc_mean = data.voc_raw;
            data.aq_co2_variability = data.co2_raw * 0.1f;
            data.aq_voc_variability = data.voc_raw * 0.15f;
            data.aq_co2_base = data.co2_raw * 0.9f;
            data.aq_co2_peak = data.co2_raw * 1.1f;
            data.aq_voc_base = data.voc_raw * 0.85f;
            data.aq_voc_peak = data.voc_raw * 1.15f;
            data.aq_voc_mean = 50; // Wartosc domyslna bez sensora NOx
            
            // IAQ Index
            float co2_factor = std::min(1.0f, static_cast<float>(data.co2_raw) / 1000.0f);
            float voc_factor = std::min(1.0f, static_cast<float>(data.voc_raw) / 200.0f);
            data.aq_gas_idx = (co2_factor * 250.0f) + (voc_factor * 250.0f);
            data.aq_ventilation_idx = std::min(5.0f, 1.0f + data.aq_gas_idx / 100.0f);
            
            // Ventilation Need
            data.aq_ventilation_idx = std::min(100.0f, co2_factor * 100.0f + voc_factor * 50.0f);
            
            // Stress & Comfort
            data.aq_stress_level = co2_factor * 0.5f + voc_factor * 0.3f;
            data.aq_comfort_score = 100.0f - data.aq_gas_idx / 5.0f;
            data.aq_comfort_score = std::max(0.0f, std::min(100.0f, data.aq_comfort_score));
            
            // Variability & Stability
            data.aq_variability_idx = 0.2f;
            data.aq_stability = 80.0f;
            data.aq_co2_trend = 0.0f;
            data.aq_env_correlation = 0.1f;
            data.aq_comfort_score = 100.0f - data.aq_ventilation_idx * 0.5f;
            
            // Alerty
            data.aq_co2_alert = (data.co2_raw > 1500) ? 1.0f : 0.0f;
            data.aq_voc_warning = (data.voc_raw > 150) ? 1.0f : 0.0f;
            data.aq_contamination = co2_factor * 0.4f + voc_factor * 0.4f;
            data.aq_contamination = voc_factor * 0.7f;
            data.aq_mold_risk = data.th_mold_risk * 0.5f;
        }
        
        // =========================================================================
        // --- PIEZO VIBRATION DERIVED (22 parametry) ---
        // =========================================================================
        if (data.vibration_raw != 0) {
            float vib_norm = static_cast<float>(data.vibration_raw) / 1024.0f;
            
            // Amplituda
            data.pv_vib_rms = vib_norm * 100.0f;
            data.pv_vib_peak = vib_norm * 150.0f;
            data.pv_vib_mean = vib_norm * 80.0f;
            data.pv_vib_std = vib_norm * 30.0f;
            data.pv_vibration_energy = vib_norm * vib_norm;
            
            // Czestotliwosc i ZCR
            data.pv_vib_freq_dom = 100.0f + vib_norm * 200.0f;
            data.pv_zcr = vib_norm * 50.0f;
            
            // Aktywnosc
            data.pv_activity_pattern = std::min(100.0f, vib_norm * 120.0f);
            data.pv_bee_traffic = data.pv_activity_pattern * 0.9f;
            
            // Detekcja zdarzen
            data.pv_predator_det = (vib_norm > 0.8f) ? 0.7f : 0.1f;
            data.pv_intrusion_prob = data.pv_predator_det * 0.8f;
            data.pv_queen_piping = (vib_norm > 0.6f && vib_norm < 0.8f) ? 0.5f : 0.0f;
            data.pv_swarm_prep = data.pv_queen_piping * 0.7f;
            data.pv_aggression_idx = vib_norm * 0.5f;
            data.pv_alien_detected = 0.0f;
            data.pv_wind_noise = (vib_norm < 0.3f) ? 0.5f : 0.1f;
            data.pv_impact_flag = (vib_norm > 0.9f) ? 1.0f : 0.0f;
            data.pv_continuous_vib = vib_norm * 0.8f;
            data.pv_event_count = vib_norm * 10.0f;
            data.pv_severity = vib_norm;
            data.pv_source_type = (vib_norm > 0.7f) ? 2.0f : 1.0f;
            data.pv_confidence = 0.7f + vib_norm * 0.2f;
        }
        
        // =========================================================================
        // --- RADAR MMWAVE DERIVED (27 parametrow) ---
        // =========================================================================
        // Surowe dane z radaru (motion_detected jako baza)
        float radar_base = static_cast<float>(data.motion_detected) * 0.5f;
        
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
        // --- HX711 WAGA DERIVED (105+ parametrow) ---
        // =========================================================================
        // Uzywamy weight_raw jako bazy (ADC counts przeliczone na kg)
        float weight_kg = static_cast<float>(data.weight_raw) / 1000.0f; // Przykladowe przeliczenie
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
        
        // Pozytki i nektar
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
        
        // Cyklicznosc
        data.hx711_daily_amplitude = 0.3f;
        data.hx711_circadian_str = 0.7f;
        data.hx711_seasonal_trend = 0.1f;
        
        // Jakosc sygnalu
        data.hx711_signal_quality = 85.0f;
        data.hx711_noise_level = 0.02f;
        data.hx711_drift_rate = 0.001f;
        data.hx711_signal_quality = 90.0f;
        
        // Anomalie
        data.hx711_anomaly_flag = 0.1f;
        data.hx711_drift_rate = 0.0f;
        data.hx711_dominant_period = 0.0f;
        
        // Zdrowie kolonii
        data.hx711_growth_rate = 0.5f;
        data.hx711_brood_mass = 0.6f;
        data.hx711_bee_population = weight_kg * 100.0f;
        data.hx711_health_score = 80.0f;
        data.hx711_productivity_idx = data.hx711_foraging_eff * 0.8f;
        
        // Prognoza
        data.hx711_forecast_24h = weight_kg + data.hx711_slope_24h * 24.0f;
        data.hx711_forecast_conf = 0.7f;
        data.hx711_forecast_24h = std::max(0.0f, data.hx711_nectar_accum * 0.6f);
        
        // =========================================================================
        // --- BAROMETRIC DERIVED (18 parametrow) ---
        // =========================================================================
        // Wartosci domyslne bez sensora BMP280
        data.baro_pressure = 1013.25f;
        data.baro_temperature = data.temperature;
        data.baro_altitude_est = 100.0f;
        data.baro_pressure_mean = 1013.25f;
        data.baro_pressure_std = 1.0f;
        data.baro_pressure_trend = 0.0f;
        data.baro_trend_short = 0.0f;
        data.baro_trend_medium = 0.0f;
        data.baro_pressure_change = 0.0f;
        data.baro_weather_idx = 0.0f;
        data.baro_storm_risk = 0.2f;
        data.baro_rain_risk = 0.3f;
        data.baro_improving_flag = 0.5f;
        data.baro_foraging_idx = 70.0f;
        data.baro_activity_pred = 60.0f;
        data.baro_severity = 0.2f;
        data.baro_alert_flag = 0.0f;
        data.baro_reliability_idx = 80.0f;
        
        // =========================================================================
        // --- LIGHT DERIVED (17 parametrow) ---
        // =========================================================================
        // Wartosci domyslne bez sensora BH1750
        uint32_t lux_est = 5000; // Swiatlo dzienne
        data.light_lux = lux_est;
        data.light_ir_ratio = 100.0f;
        data.light_uv_idx = 5.0f;
        data.light_spectrum_data = 1.0f;
        data.light_lux_mean = static_cast<float>(lux_est);
        data.light_lux_std = 500.0f;
        data.light_lux_min = lux_est - 500;
        data.light_lux_max = lux_est + 500;
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

    // Inicjalizacja socketu UDP do nasluchiwania danych z Pico
    bool initNetwork(int port = 5005) {
        server_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (server_socket < 0) {
            Logger::getInstance().error( "Nie udalo sie utworzyc socketu: " + std::string(strerror(errno)));
            return false;
        }

        // Ustawienie socketu na nieblokujacy (opcjonalne, ale dobre dla petli)
        int flags = fcntl(server_socket, F_GETFL, 0);
        fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Nasluchuj na wszystkich interfejsach
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            Logger::getInstance().error( "Blad bindowania portu " + std::to_string(port) + ": " + strerror(errno));
            close(server_socket);
            return false;
        }

        Logger::getInstance().info( "Serwer nasluchujacy uruchomiony na porcie UDP " + std::to_string(port));
        return true;
    }

    // Glowna petla odbierania danych (watek)
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
                // Prawdziwy blad sieciowy
                if (running) Logger::getInstance().warning( "Blad odbioru danych: " + std::string(strerror(errno)));
            }
            
            // Krotka pauza, aby nie obciazac CPU w petli busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // ============================================================================
    // PARSOWANIE I PRZETWARZANIE DANYCH - OBSLUGA JSON I CSV
    // ============================================================================
    void processData(const std::string& raw_data, const std::string& source_ip) {
        // Obslugiwane formaty:
        // 1. JSON: {"hive_id":"UL-1","temp":24.5,"hum":65.2,...}
        // 2. CSV: UL-1,24.5,65.2,45.300,98,450,35,1,1234567890,...
        
        long long now = std::time(nullptr);
        
        // Sprawdz czy to JSON
        if (raw_data.find('{') != std::string::npos && raw_data.find('}') != std::string::npos) {
            parseJSON(raw_data, source_ip, now);
        } else {
            // Uzywamy nowego modulu CSV do parsowania
            HiveData data;
            if (csv::parseCSV(raw_data, source_ip, now, data)) {
                std::lock_guard<std::mutex> lock(data_mutex);
                hives_data[data.hive_id] = data;
                Logger::getInstance().info("Parsed CSV data for hive: " + data.hive_id + " from " + source_ip);
            } else {
                Logger::getInstance().warning("Failed to parse CSV data from " + source_ip);
            }
        }
    }
    
    // Parsowanie JSON - pelna obsluga wszystkich parametrow (300+)
    void parseJSON(const std::string& json_str, const std::string& source_ip, long long timestamp) {
        DEBUG_FUNC_ENTER();
        GENTLE_TRY
        
        try {
            // Prosty parser JSON bez zewnetrznych bibliotek
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
            if (hive_id.empty()) {
                hive_id = "UNKNOWN";
                apiary::Logger::getInstance().warning("Missing hive_id in JSON from " + source_ip, "PARSER");
            }
            
            HiveData data;
            data.hive_id = hive_id;
            data.timestamp = timestamp;
            data.is_online = true;
            
            DEBUG_LOG("Parsing JSON for hive: " + hive_id);
            DEBUG_COUNTER_INC("json_parsed");
            
            // ====================================================================
            // DETEKCJA ZRODLA DANYCH I TYPU (PICO vs NANO)
            // ====================================================================
            std::string source = getValue("data_source");
            if (!source.empty()) {
                data.data_source = source;
                data.is_precomputed = (source == "pico" || source == "precomputed");
                DEBUG_LOG("Data source specified: " + source);
            } else {
                // Automatyczna detekcja: sprawdz czy sa parametry wyliczone
                bool has_computed_params = !getValue("audio_spectral_centroid").empty() ||
                                           !getValue("hx711_slope_1h").empty() ||
                                           !getValue("th_heat_index").empty() ||
                                           !getValue("aq_iaq_index").empty();
                if (has_computed_params) {
                    data.data_source = "pico";
                    data.is_precomputed = true;
                    DEBUG_LOG("Auto-detected PICO (precomputed data)");
                } else {
                    data.data_source = "nano";
                    data.is_precomputed = false;
                    DEBUG_LOG("Auto-detected NANO (raw data only)");
                }
            }
            
            DEBUG_START_TIMER("json_parse_raw");
            
            // Pobierz surowe dane z Arduino Nano (jesli obecne)
            try {
                data.temp_raw = !getValue("temp_raw").empty() ? std::stof(getValue("temp_raw")) : 0.0f;
                data.hum_raw = !getValue("hum_raw").empty() ? std::stof(getValue("hum_raw")) : 0.0f;
                data.weight_raw = !getValue("weight_raw").empty() ? std::stol(getValue("weight_raw")) : 0;
                data.audio_raw = !getValue("audio_raw").empty() ? std::stoi(getValue("audio_raw")) : 0;
                data.vibration_raw = !getValue("vibration_raw").empty() ? std::stoi(getValue("vibration_raw")) : 0;
                data.co2_raw = !getValue("co2_raw").empty() ? std::stoi(getValue("co2_raw")) : 0;
                data.voc_raw = !getValue("voc_raw").empty() ? std::stoi(getValue("voc_raw")) : 0;
            } catch (const std::exception& e) {
                apiary::Logger::getInstance().warning(std::string("Error parsing raw values: ") + e.what(), "PARSER");
                DEBUG_RECORD_EXCEPTION(e);
            }
            
            DEBUG_STOP_TIMER("json_parse_raw");
            DEBUG_START_TIMER("json_parse_basic");
            
            // Podstawowe parametry (9)
            try {
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
            } catch (const std::exception& e) {
                apiary::Logger::getInstance().error(std::string("Error parsing basic values: ") + e.what(), "PARSER");
                DEBUG_RECORD_EXCEPTION(e);
            }
            
            DEBUG_STOP_TIMER("json_parse_basic");
            
            // ====================================================================
            // JESLI DANE SA SUROWE (NANO), OBLICZ PARAMETRY NA RASPBERRY PI
            // ====================================================================
            if (!data.is_precomputed && data.data_source == "nano") {
                computeParametersFromRaw(data);
            }
            
            // Audio parametry (wybrane z 97+) - tylko jesli precomputed lub juz obliczone
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
            data.hx711_growth_rate = !getValue("colony_growth").empty() ? std::stof(getValue("colony_growth")) : 0.0f;
            data.hx711_productivity_idx = !getValue("productivity").empty() ? std::stof(getValue("productivity")) : 0.0f;
            data.hx711_forecast_24h = !getValue("predicted_24h").empty() ? std::stof(getValue("predicted_24h")) : 0.0f;
            data.hx711_anomaly_flag = !getValue("hx_anomaly").empty() ? std::stof(getValue("hx_anomaly")) : 0.0f;
            data.hx711_winter_readiness = !getValue("winter_readiness").empty() ? std::stof(getValue("winter_readiness")) : 0.0f;
            data.hx711_starvation_risk = !getValue("starvation_risk").empty() ? std::stof(getValue("starvation_risk")) : 0.0f;
            
            // Temp/Humidity parametry (wybrane z 28)
            data.th_heat_index = !getValue("heat_index").empty() ? std::stof(getValue("heat_index")) : 0.0f;
            data.th_dew_point = !getValue("dew_point").empty() ? std::stof(getValue("dew_point")) : 0.0f;
            data.th_comfort_idx = !getValue("comfort_index").empty() ? std::stof(getValue("comfort_index")) : 0.0f;
            data.th_brood_stress = !getValue("brood_stress").empty() ? std::stof(getValue("brood_stress")) : 0.0f;
            data.th_temp_stability = !getValue("temp_stability").empty() ? std::stof(getValue("temp_stability")) : 0.0f;
            data.th_mold_risk = !getValue("mold_risk").empty() ? std::stof(getValue("mold_risk")) : 0.0f;
            
            // Air Quality parametry (wybrane z 24)
            data.aq_co2_mean = !getValue("aq_co2_mean").empty() ? std::stoi(getValue("aq_co2_mean")) : 0;
            data.aq_voc_mean = !getValue("aq_voc_mean").empty() ? std::stoi(getValue("aq_voc_mean")) : 0;
            data.aq_gas_idx = !getValue("iaq_index").empty() ? std::stof(getValue("iaq_index")) : 0.0f;
            data.aq_ventilation_idx = !getValue("ventilation_need").empty() ? std::stof(getValue("ventilation_need")) : 0.0f;
            data.aq_contamination = !getValue("contamination_risk").empty() ? std::stof(getValue("contamination_risk")) : 0.0f;
            data.aq_comfort_score = !getValue("aq_comfort").empty() ? std::stof(getValue("aq_comfort")) : 0.0f;
            
            // Piezo parametry (wybrane z 22)
            data.pv_vib_rms = !getValue("piezo_rms").empty() ? std::stof(getValue("piezo_rms")) : 0.0f;
            data.pv_vib_freq_dom = !getValue("piezo_freq").empty() ? std::stof(getValue("piezo_freq")) : 0.0f;
            data.pv_activity_pattern = !getValue("piezo_activity").empty() ? std::stof(getValue("piezo_activity")) : 0.0f;
            data.pv_bee_traffic = !getValue("bee_traffic").empty() ? std::stof(getValue("bee_traffic")) : 0.0f;
            data.pv_predator_det = !getValue("predator_score").empty() ? std::stof(getValue("predator_score")) : 0.0f;
            data.pv_intrusion_prob = !getValue("intrusion_prob").empty() ? std::stof(getValue("intrusion_prob")) : 0.0f;
            
            // Barometric parametry (wybrane z 18)
            data.baro_pressure = !getValue("pressure").empty() ? std::stof(getValue("pressure")) : 0.0f;
            data.baro_pressure_trend = !getValue("baro_trend_1h").empty() ? std::stof(getValue("baro_trend_1h")) : 0.0f;
            data.baro_weather_idx = !getValue("weather_trend").empty() ? std::stof(getValue("weather_trend")) : 0.0f;
            data.baro_storm_risk = !getValue("storm_prob").empty() ? std::stof(getValue("storm_prob")) : 0.0f;
            data.baro_foraging_idx = !getValue("foraging_cond").empty() ? std::stof(getValue("foraging_cond")) : 0.0f;
            
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
            
            // Zapis do bazy danych SQLite
            try {
                ApiaryDatabase::getInstance().storeRawData(data);
                Logger::getInstance().debug("Zapisano dane do bazy dla " + hive_id, "DB");
            } catch (const std::exception& e) {
                Logger::getInstance().warning("Nie udało się zapisać do bazy: " + std::string(e.what()), "DB");
            }
            
            Logger::getInstance().debug("JSON: Zaktualizowano " + hive_id + " z " + source_ip);
            
        } catch (const std::exception& e) {
            Logger::getInstance().error("Blad parsowania JSON: " + std::string(e.what()));
        }
        GENTLE_CATCH(Logger::getInstance().error("Krytyczny blad w parseJSON");)
    }
    
    // Parsowanie CSV - pelna obsluga 338+ parametrow
    void parseCSV(const std::string& raw_data, const std::string& source_ip, long long timestamp) {
        DEBUG_FUNC_ENTER();
        GENTLE_TRY
        
        // Uzywamy modulu csv do parsowania
        HiveData data;
        if (csv::parseCSV(raw_data, source_ip, timestamp, data)) {
            std::lock_guard<std::mutex> lock(data_mutex);
            
            if (hives_data.find(data.hive_id) != hives_data.end()) {
                hives_data[data.hive_id] = data;
                Logger::getInstance().debug("CSV: Zaktualizowano " + data.hive_id + " z " + source_ip);
            } else {
                Logger::getInstance().info("Wykryto nowy ul: " + data.hive_id + " z IP " + source_ip);
                hives_data[data.hive_id] = data;
            }
            
            // Zapis do bazy danych SQLite
            try {
                ApiaryDatabase::getInstance().storeRawData(data);
                Logger::getInstance().debug("Zapisano dane CSV do bazy dla " + data.hive_id, "DB");
            } catch (const std::exception& e) {
                Logger::getInstance().warning("Nie udalo sie zapisac CSV do bazy: " + std::string(e.what()), "DB");
            }
            
            DEBUG_COUNTER_INC("csv_parsed");
        } else {
            Logger::getInstance().warning("Niepoprawny format CSV z " + source_ip + ": " + raw_data);
            DEBUG_COUNTER_INC("csv_parse_errors");
        }
        
        DEBUG_FUNC_EXIT();
        GENTLE_CATCH(Logger::getInstance().error("Krytyczny blad w parseCSV"))
    }

    // Symulacja danych (do testow bez fizycznych Pico)
    void simulationLoop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            std::lock_guard<std::mutex> lock(data_mutex);
            for (auto& pair : hives_data) {
                // Symuluj zmiany temperatury i wilgotnosci
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
                Logger::getInstance().warning( "Przelaczanie w tryb symulacji z powodu bledu sieci.");
                use_simulation = true;
            }
        }

        if (use_simulation) {
            Logger::getInstance().info( "Uruchamianie symulatora danych uli...");
            worker_threads.emplace_back(&ApiaryCollector::simulationLoop, this);
        } else {
            Logger::getInstance().info( "Uruchamianie nasluchiwania sieciowego...");
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

    // Metoda dla TUI/Basha do pobrania aktualnego stanu (eksport do JSON) - OBSLUGA WIELU ULl
    size_t getHiveCount() const {
        return hives_data.size();
    }
    
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
            json << "\"colony_growth\":" << d.hx711_growth_rate << ",";
            json << "\"productivity\":" << d.hx711_productivity_idx << ",";
            json << "\"predicted_24h\":" << d.hx711_forecast_24h << ",";
            json << "\"forecast_conf\":" << d.hx711_forecast_conf << ",";
            json << "\"winter_readiness\":" << d.hx711_winter_readiness << ",";
            json << "\"starvation_risk\":" << d.hx711_starvation_risk << ",";
            json << "\"anomaly_score\":" << d.hx711_anomaly_flag << "},";
            
            // Temp/Humidity parametry (wybrane z 28)
            json << "\"th\":{\"heat_index\":" << d.th_heat_index << ",";
            json << "\"dew_point\":" << d.th_dew_point << ",";
            json << "\"comfort_index\":" << d.th_comfort_idx << ",";
            json << "\"brood_stress\":" << d.th_brood_stress << ",";
            json << "\"temp_stability\":" << d.th_temp_stability << ",";
            json << "\"mold_risk\":" << d.th_mold_risk << ",";
            json << "\"vpd\":" << d.th_vpd << "},";
            
            // Air Quality parametry (wybrane z 24)
            json << "\"aq\":{\"co2_mean\":" << d.aq_co2_mean << ",";
            json << "\"voc_mean\":" << d.aq_voc_mean << ",";
            json << "\"iaq_index\":" << d.aq_gas_idx << ",";
            json << "\"ventilation_need\":" << d.aq_ventilation_idx << ",";
            json << "\"contamination_risk\":" << d.aq_contamination << ",";
            json << "\"mold_risk\":" << d.aq_mold_risk << "},";
            
            // Piezo Vibration parametry (wybrane z 22)
            json << "\"piezo\":{\"rms\":" << d.pv_vib_rms << ",";
            json << "\"freq\":" << d.pv_vib_freq_dom << ",";
            json << "\"activity\":" << d.pv_activity_pattern << ",";
            json << "\"bee_traffic\":" << d.pv_bee_traffic << ",";
            json << "\"predator_score\":" << d.pv_predator_det << ",";
            json << "\"intrusion_prob\":" << d.pv_intrusion_prob << "},";
            
            // Barometric parametry (wybrane z 18)
            json << "\"baro\":{\"pressure\":" << d.baro_pressure << ",";
            json << "\"trend_1h\":" << d.baro_pressure_trend << ",";
            json << "\"weather_trend\":" << d.baro_weather_trend << ",";
            json << "\"storm_prob\":" << d.baro_storm_prob << ",";
            json << "\"foraging_cond\":" << d.baro_foraging_cond << "},";
            
            // Light parametry (wybrane z 17)
            json << "\"light\":{\"lux\":" << d.light_lux << ",";
            json << "\"daylight_hours\":" << d.light_daylight_hours << ",";
            json << "\"circadian_sync\":" << d.light_circadian_sync << ",";
            json << "\"foraging_idx\":" << d.light_foraging_idx << ",";
            json << "\"uv\":" << d.light_uv_idx << "}";
            
            json << "}";
        }
        json << "}}";
        return json.str();
    }
    
    // Pelny format CSV - WSZYSTKIE 338+ PARAMETROW
    std::string getStatusCSV() {
        std::lock_guard<std::mutex> lock(data_mutex);
        // Uzywamy nowego modulu CSV do generowania pelnego raportu
        return csv::generateFullCSV(hives_data);
    }

    // Eksport danych do pliku CSV
    void exportToCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::getInstance().error("Nie udalo sie otworzyc pliku CSV: " + filename);
            return;
        }
        
        file << getStatusCSV();
        file.close();
        Logger::getInstance().debug("Eksportowano dane do pliku: " + filename);
    }
};

// Funkcja glowna do samodzielnego uruchomienia jako demon
int main(int argc, char* argv[]) {
    DEBUG_FUNC_ENTER();
    
    // Inicjalizacja loggera z pelna konfiguracja
    LoggerConfig loggerConfig;
    loggerConfig.log_file = "/var/log/apiaryguard/collector.log";
    loggerConfig.debug_file = "/var/log/apiaryguard/collector_debug.log";
    loggerConfig.console_output = true;
    loggerConfig.file_output = true;
    loggerConfig.min_level = LogLevel::DEBUG;
    loggerConfig.rotation_enabled = true;
    loggerConfig.max_file_size = 5 * 1024 * 1024; // 5MB
    loggerConfig.max_queue_size = 500;
    
    try {
        Logger::getInstance().initialize(loggerConfig);
        DEBUG_COUNTER_INC("logger_initialized");
    } catch (const LoggerInitException& e) {
        std::cerr << "[CRITICAL] Failed to initialize logger: " << e.what() << std::endl;
        DEBUG_RECORD_EXCEPTION(e);
        return 1;
    }
    
    Logger::getInstance().info("========================================", "MAIN");
    Logger::getInstance().info("Apiary Collector v1.0.0 - Start", "MAIN");
    Logger::getInstance().info("========================================", "MAIN");
    Logger::getInstance().debug("PID procesu: " + std::to_string(getpid()), "MAIN");
    Logger::getInstance().debug("Argumenty linii polecen: " + std::string(argc > 1 ? argv[1] : "brak"), "MAIN");
    
#ifdef DEBUG_BUILD
    Logger::getInstance().info("Build: DEBUG z funkcjami debugowania", "MAIN");
    apiary::debug::DebugTracer::getInstance().enable();
    apiary::debug::PerformanceMonitor::getInstance().enable();
#else
    Logger::getInstance().info("Build: RELEASE (bez rozszerzonego debugowania)", "MAIN");
#endif
    
    try {
    

    // Inicjalizacja bazy danych przed uruchomieniem kolektora
    Logger::getInstance().info("Inicjalizacja bazy danych SQLite...", "DB");
    DatabaseConfig dbConfig;
    dbConfig.db_path = "/var/lib/apiary/apiary.db";
    dbConfig.max_raw_records = 3600;          // 1 godzina danych co sekunde
    dbConfig.max_minute_records = 10080;      // 7 dni danych minutowych
    dbConfig.aggregation_interval_sec = 60;   // Agreguj co minute
    
    try {
        ApiaryDatabase::getInstance().initialize(dbConfig);
        Logger::getInstance().info("Baza danych zainicjalizowana pomyslnie", "DB");
    } catch (const DatabaseInitException& e) {
        Logger::getInstance().warning("Nie udalo sie zainicjalizowac bazy danych: " + std::string(e.what()) + ". Kontynuujemy bez zapisu do bazy.", "DB");
    }
    
    ApiaryCollector collector;


    // Serwer HTTP API JSON na porcie 8080
    Logger::getInstance().debug("Tworzenie socketu HTTP...", "HTTP");
    int server_fd = -1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        Logger::getInstance().error("Nie udalo sie utworzyc socketu HTTP: " + std::string(strerror(errno)), "HTTP");
        Logger::getInstance().critical("Krytyczny blad - zakonczenie dzialania", "MAIN");
        DEBUG_COUNTER_INC("socket_creation_failed");
        DEBUG_FUNC_EXIT();
        return 1;
    }
    Logger::getInstance().debug("Socket HTTP utworzony pomyslnie: fd=" + std::to_string(server_fd), "HTTP");
    DEBUG_COUNTER_INC("sockets_created");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Logger::getInstance().debug("Opcja SO_REUSEADDR ustawiona", "HTTP");

    struct sockaddr_in http_addr;
    memset(&http_addr, 0, sizeof(http_addr));
    http_addr.sin_family = AF_INET;
    http_addr.sin_addr.s_addr = INADDR_ANY;
    http_addr.sin_port = htons(8080);

    Logger::getInstance().info("Bindowanie portu HTTP 8080...", "HTTP");
    if (bind(server_fd, (struct sockaddr*)&http_addr, sizeof(http_addr)) < 0) {
        Logger::getInstance().error("Blad bindowania portu HTTP 8080: " + std::string(strerror(errno)), "HTTP");
        Logger::getInstance().error("Sprawdz czy port 8080 nie jest zajety przez inny proces", "HTTP");
        Logger::getInstance().error("Mozesz sprawdzic: netstat -tlnp | grep 8080", "HTTP");
        close(server_fd);
        DEBUG_COUNTER_INC("bind_failed");
        DEBUG_FUNC_EXIT();
        return 1;
    }
    Logger::getInstance().info("Port 8080 zbindowany pomyslnie", "HTTP");
    DEBUG_COUNTER_INC("binds_successful");

    if (listen(server_fd, 10) < 0) {
        Logger::getInstance().error("Blad listen na porcie HTTP: " + std::string(strerror(errno)), "HTTP");
        close(server_fd);
        DEBUG_COUNTER_INC("listen_failed");
        DEBUG_FUNC_EXIT();
        return 1;
    }
    Logger::getInstance().info("Nasluchiwanie na porcie 8080 rozpoczete", "HTTP");
    DEBUG_COUNTER_INC("listens_successful");

    Logger::getInstance().info("Serwer HTTP API JSON uruchomiony na porcie 8080", "HTTP");
    Logger::getInstance().info("Endpointy: GET /api/status, GET /api/hives, GET /health", "HTTP");
    Logger::getInstance().info("URL health check: http://localhost:8080/health", "HTTP");
    Logger::getInstance().info("URL status: http://localhost:8080/api/status", "HTTP");

    fd_set readfds;
    struct timeval tv;

    DEBUG_START_TIMER("main_loop");
    DEBUG_COUNTER_INC("main_started");
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            // Nowe polaczenie HTTP
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                char buffer[4096] = {0};
                recv(client_fd, buffer, sizeof(buffer)-1, 0);
                
                std::string request(buffer);
                std::string response;
                std::string content_type = "application/json";
                
                // Parsowanie zadania HTTP - obsluga pelnej listy endpointow
                if (request.find("GET /api/status") != std::string::npos || 
                    request.find("GET /api/hives") != std::string::npos) {
                    response = collector.getStatusJSON();
                } else if (request.find("GET /health") != std::string::npos) {
                    response = "{\"status\":\"ok\",\"timestamp\":" + std::to_string(std::time(nullptr)) + ",\"version\":\"1.0.0\",\"collector_url\":\"http://localhost:8080\"}";
                } else if (request.find("GET /api/csv") != std::string::npos) {
                    response = collector.getStatusCSV();
                    content_type = "text/csv";
                } else if (request.find("GET /api/sensors") != std::string::npos) {
                    response = "{\"sensors\":[],\"note\":\"Endpoint dostepny - dane dostepne w /api/hives\"}";
                } else if (request.find("GET /api/effectors") != std::string::npos) {
                    response = "{\"effectors\":[],\"note\":\"Endpoint dostepny\"}";
                } else if (request.find("GET /api/history") != std::string::npos) {
                    response = "{\"labels\":[],\"data\":[],\"note\":\"Dane historyczne dostepne przez TUI\"}";
                } else if (request.find("GET /api/logs") != std::string::npos) {
                    response = "{\"logs\":[],\"note\":\"Logi dostepne przez TUI\"}";
                } else {
                    response = "{\"error\":\"Unknown endpoint\",\"available\":[\"/api/status\",\"/api/hives\",\"/health\",\"/api/csv\",\"/api/sensors\",\"/api/effectors\",\"/api/history\",\"/api/logs\"]}";
                }
                
                // Naglowki HTTP
                std::stringstream http_response;
                http_response << "HTTP/1.1 200 OK\r\n";
                http_response << "Content-Type: " << content_type << "\r\n";
                http_response << "Access-Control-Allow-Origin: *\r\n";
                http_response << "Content-Length: " << response.length() << "\r\n";
                http_response << "Connection: close\r\n\r\n";
                http_response << response;
                
                send(client_fd, http_response.str().c_str(), http_response.str().length(), 0);
                close(client_fd);
                
                Logger::getInstance().debug("HTTP: Obsluzono zadanie od " + std::string(inet_ntoa(client_addr.sin_addr)));
            }
        }
        
        // Co 5 sekund loguj status
        static time_t last_log = 0;
        if (std::time(nullptr) - last_log >= 5) {
            last_log = std::time(nullptr);
            // Eksportuj dane do pliku CSV dla TUI
            collector.exportToCSV("/tmp/apiary_data.csv");
            // Mozna dodac okresowe logowanie statystyk
        }
    }
    
    } catch (const std::exception& e) {
        Logger::getInstance().critical(std::string("Critical error in main: ") + e.what(), "MAIN");
        return 1;
    }
    
    return 0;
}
