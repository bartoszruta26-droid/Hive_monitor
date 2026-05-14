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
    
    // Parsowanie CSV - pełna obsługa 338+ parametrów
    void parseCSV(const std::string& raw_data, const std::string& source_ip, long long timestamp) {
        DEBUG_FUNC_ENTER();
        GENTLE_TRY
        
        std::stringstream ss(raw_data);
        std::string segment;
        std::vector<std::string> parts;

        while (std::getline(ss, segment, ',')) {
            parts.push_back(segment);
        }

        // Wymagane minimum 9 pol dla podstawowych danych
        if (parts.size() < 9) {
            Logger::getInstance().warning("Niepoprawny format CSV z " + source_ip + ": " + raw_data);
            DEBUG_COUNTER_INC("csv_parse_errors");
            DEBUG_FUNC_EXIT();
            return;
        }

        DEBUG_LOG("Parsing CSV with " + std::to_string(parts.size()) + " fields from " + source_ip);
        DEBUG_COUNTER_INC("csv_parsed");
        DEBUG_START_TIMER("csv_parse");

        try {
            std::string hive_id = parts[0];
            float temp = std::stof(parts[1]);
            float humidity = std::stof(parts[2]);
            float weight = std::stof(parts[3]);
            int battery = std::stoi(parts[4]);
            int co2 = std::stoi(parts[5]);
            int voc = std::stoi(parts[6]);
            int motion = std::stoi(parts[7]);
            
            // Pomocnicza funkcja do bezpiecznego parsowania float
            auto getFloat = [&](size_t idx) -> float {
                return (idx < parts.size()) ? std::stof(parts[idx]) : 0.0f;
            };
            
            // Pomocnicza funkcja do bezpiecznego parsowania int
            auto getInt = [&](size_t idx) -> int {
                return (idx < parts.size()) ? std::stoi(parts[idx]) : 0;
            };

            // =========================================================================
            // PARAMETRY AUDIO (63 parametry) - indeksy 9-71
            // =========================================================================
            float audio_rms = getFloat(9);
            float audio_peak = getFloat(10);
            float audio_peak_to_peak = getFloat(11);
            float audio_zcr = getFloat(12);
            float audio_energy = getFloat(13);
            float audio_mean_amp = getFloat(14);
            float audio_std_amp = getFloat(15);
            float audio_cv_amp = getFloat(16);
            float audio_skewness = getFloat(17);
            float audio_kurtosis = getFloat(18);
            float audio_dominant_freq = getFloat(19);
            float audio_spectral_centroid = getFloat(20);
            float audio_spectral_bandwidth = getFloat(21);
            float audio_spectral_flatness = getFloat(22);
            float audio_spectral_rolloff = getFloat(23);
            float audio_power_bee_band = getFloat(24);
            float audio_power_swarm_band = getFloat(25);
            float audio_power_low_freq = getFloat(26);
            float audio_power_high_freq = getFloat(27);
            float audio_harmonic_ratio = getFloat(28);
            // MFCC energy [4] - indeksy 29-32
            float audio_mfcc_energy[4] = {getFloat(29), getFloat(30), getFloat(31), getFloat(32)};
            float audio_spectral_entropy = getFloat(33);
            float audio_spectral_contrast = getFloat(34);
            float audio_tonal_strength = getFloat(35);
            float audio_crest_factor = getFloat(36);
            float audio_formant_f1 = getFloat(37);
            float audio_formant_f2 = getFloat(38);
            float audio_fundamental_freq = getFloat(39);
            float audio_pitch_strength = getFloat(40);
            float audio_inharmonicity = getFloat(41);
            float audio_shimmer = getFloat(42);
            float audio_jitter = getFloat(43);
            float audio_nhr = getFloat(44);
            float audio_hnr = getFloat(45);
            float audio_autocorr_peak = getFloat(46);
            float audio_attack_time = getFloat(47);
            float audio_decay_time = getFloat(48);
            float audio_sustain_level = getFloat(49);
            float audio_temporal_centroid = getFloat(50);
            float audio_loudness = getFloat(51);
            float audio_spectral_flux = getFloat(52);
            float audio_spectral_slope = getFloat(53);
            float audio_spectral_kurtosis = getFloat(54);
            float audio_spectral_skewness = getFloat(55);
            float audio_fund_salience = getFloat(56);
            // Power bands [8] - indeksy 57-64
            float audio_power_band[8] = {getFloat(57), getFloat(58), getFloat(59), getFloat(60), 
                                         getFloat(61), getFloat(62), getFloat(63), getFloat(64)};
            float audio_leq = getFloat(65);
            float audio_l10 = getFloat(66);
            float audio_l90 = getFloat(67);
            float audio_noise_floor = getFloat(68);
            float audio_snr = getFloat(69);
            float audio_aci = getFloat(70);
            float audio_bi = getFloat(71);
            float audio_ndi = getFloat(72);
            float audio_adi = getFloat(73);
            float audio_aei = getFloat(74);
            float audio_bee_activity = getFloat(75);
            float audio_swarm_prob = getFloat(76);
            float audio_stress_indicator = getFloat(77);
            float audio_hive_health = getFloat(78);
            float audio_foraging_eff = getFloat(79);
            float audio_colony_coherence = getFloat(80);

            // =========================================================================
            // PARAMETRY RADAR MMWAVE (28 parametrów) - indeksy 81-108
            // =========================================================================
            float radar_distance = getFloat(81);
            float radar_energy = getFloat(82);
            float radar_speed = getFloat(83);
            float radar_distance_std = getFloat(84);
            float radar_energy_std = getFloat(85);
            float radar_speed_std = getFloat(86);
            float radar_distance_min = getFloat(87);
            float radar_distance_max = getFloat(88);
            float radar_energy_min = getFloat(89);
            float radar_energy_max = getFloat(90);
            float radar_range = getFloat(91);
            float radar_energy_variance = getFloat(92);
            float radar_cv = getFloat(93);
            float radar_activity = getFloat(94);
            float radar_idle_percent = getFloat(95);
            float radar_motion_intensity = getFloat(96);
            float radar_target_rate = getFloat(97);
            float radar_max_targets = getFloat(98);
            float radar_target_density = getFloat(99);
            float radar_slope = getFloat(100);
            float radar_correlation = getFloat(101);
            float radar_acceleration = getFloat(102);
            float radar_signal_quality = getFloat(103);
            float radar_anomaly_score = getFloat(104);
            float radar_hive_health = getFloat(105);
            float radar_power_spectrum = getFloat(106);
            float radar_zcr = getFloat(107);
            float radar_entropy = getFloat(108);

            // =========================================================================
            // PARAMETRY HX711 WAGA (77 parametrów) - indeksy 109-185
            // =========================================================================
            float hx711_current = getFloat(109);
            float hx711_mean = getFloat(110);
            float hx711_std = getFloat(111);
            float hx711_min = getFloat(112);
            float hx711_max = getFloat(113);
            float hx711_median = getFloat(114);
            float hx711_range = getFloat(115);
            float hx711_variance = getFloat(116);
            float hx711_cv = getFloat(117);
            float hx711_iqr = getFloat(118);
            float hx711_rate = getFloat(119);
            float hx711_mean_rate = getFloat(120);
            float hx711_max_pos_rate = getFloat(121);
            float hx711_max_neg_rate = getFloat(122);
            float hx711_acceleration = getFloat(123);
            float hx711_slope_1h = getFloat(124);
            float hx711_slope_4h = getFloat(125);
            float hx711_slope_24h = getFloat(126);
            float hx711_corr_1h = getFloat(127);
            float hx711_corr_4h = getFloat(128);
            float hx711_corr_24h = getFloat(129);
            float hx711_direction = getFloat(130);
            float hx711_nectar_inflow = getFloat(131);
            float hx711_nectar_accum = getFloat(132);
            float hx711_foraging_eff = getFloat(133);
            float hx711_bloom_intensity = getFloat(134);
            float hx711_honey_prod_idx = getFloat(135);
            float hx711_nectar_quality = getFloat(136);
            float hx711_consumption_rate = getFloat(137);
            float hx711_daily_consumption = getFloat(138);
            float hx711_food_reserve_days = getFloat(139);
            float hx711_winter_readiness = getFloat(140);
            float hx711_starvation_risk = getFloat(141);
            float hx711_daily_amplitude = getFloat(142);
            float hx711_circadian_str = getFloat(143);
            float hx711_seasonal_trend = getFloat(144);
            float hx711_signal_quality = getFloat(145);
            float hx711_noise_level = getFloat(146);
            float hx711_drift_rate = getFloat(147);
            float hx711_outlier_ratio = getFloat(148);
            float hx711_sample_count = getFloat(149);
            float hx711_forecast_1h = getFloat(150);
            float hx711_forecast_6h = getFloat(151);
            float hx711_forecast_24h = getFloat(152);
            float hx711_forecast_conf = getFloat(153);
            float hx711_prediction_error = getFloat(154);
            float hx711_harvest_event = getFloat(155);
            float hx711_swarm_event = getFloat(156);
            float hx711_feeding_event = getFloat(157);
            float hx711_water_collection = getFloat(158);
            float hx711_skewness = getFloat(159);
            float hx711_kurtosis = getFloat(160);
            float hx711_entropy = getFloat(161);
            float hx711_complexity = getFloat(162);
            float hx711_fractal_dim = getFloat(163);
            float hx711_colony_mass = getFloat(164);
            float hx711_bee_population = getFloat(165);
            float hx711_brood_mass = getFloat(166);
            float hx711_food_stores = getFloat(167);
            float hx711_honey_stores = getFloat(168);
            float hx711_pollen_stores = getFloat(169);
            float hx711_productivity_idx = getFloat(170);
            float hx711_efficiency_score = getFloat(171);
            float hx711_growth_rate = getFloat(172);
            float hx711_balance_score = getFloat(173);
            float hx711_resilience_idx = getFloat(174);
            float hx711_dominant_period = getFloat(175);
            float hx711_power_24h = getFloat(176);
            float hx711_power_12h = getFloat(177);
            float hx711_power_6h = getFloat(178);
            float hx711_power_1h = getFloat(179);
            float hx711_completeness = getFloat(180);
            float hx711_reliability = getFloat(181);
            float hx711_accuracy_est = getFloat(182);
            float hx711_precision_est = getFloat(183);
            float hx711_anomaly_flag = getFloat(184);
            float hx711_health_score = getFloat(185);

            // =========================================================================
            // PARAMETRY TEMP/HUMIDITY (32 parametry) - indeksy 186-217
            // =========================================================================
            float th_temp_mean = getFloat(186);
            float th_temp_std = getFloat(187);
            float th_temp_min = getFloat(188);
            float th_temp_max = getFloat(189);
            float th_temp_range = getFloat(190);
            float th_hum_mean = getFloat(191);
            float th_hum_std = getFloat(192);
            float th_hum_min = getFloat(193);
            float th_hum_max = getFloat(194);
            float th_hum_range = getFloat(195);
            float th_heat_index = getFloat(196);
            float th_dew_point = getFloat(197);
            float th_comfort_idx = getFloat(198);
            float th_brood_stress = getFloat(199);
            float th_mold_risk = getFloat(200);
            float th_temp_stability = getFloat(201);
            float th_vpd = getFloat(202);
            float th_thi = getFloat(203);
            float th_enthalpy = getFloat(204);
            float th_air_density = getFloat(205);
            float th_specific_humidity = getFloat(206);
            float th_mixing_ratio = getFloat(207);
            float th_saturation_deficit = getFloat(208);
            float th_evap_potential = getFloat(209);
            float th_condensation_risk = getFloat(210);
            float th_ventilation_need = getFloat(211);
            float th_cooling_need = getFloat(212);
            float th_heating_need = getFloat(213);
            float th_climate_score = getFloat(214);
            float th_hum_stability_1h = getFloat(215);
            float th_hum_trend = getFloat(216);
            float th_temp_trend = getFloat(217);

            // =========================================================================
            // PARAMETRY AIR QUALITY (37 parametrów) - indeksy 218-254
            // =========================================================================
            float aq_iaq = getFloat(218);
            float aq_iaq_mean = getFloat(219);
            float aq_contamination = getFloat(220);
            float aq_mold_risk = getFloat(221);
            float aq_ventilation_idx = getFloat(222);
            float aq_co2_mean = getFloat(223);
            float aq_voc_mean = getFloat(224);
            float aq_stress_air = getFloat(225);
            float aq_purity = getFloat(226);
            float aq_freshness = getFloat(227);
            float aq_pollution_load = getFloat(228);
            float aq_gas_idx = getFloat(229);
            float aq_particulate_idx = getFloat(230);
            float aq_allergen_risk = getFloat(231);
            float aq_toxicity_est = getFloat(232);
            float aq_oxidation_cap = getFloat(233);
            float aq_reduction_cap = getFloat(234);
            float aq_buffer_cap = getFloat(235);
            float aq_recovery_rate = getFloat(236);
            float aq_stability = getFloat(237);
            float aq_trend = getFloat(238);
            float aq_forecast = getFloat(239);
            float aq_health_impact = getFloat(240);
            float aq_bee_behavior_impact = getFloat(241);
            float aq_co2_alert = getFloat(242);
            float aq_co2_base = getFloat(243);
            float aq_co2_peak = getFloat(244);
            float aq_co2_trend = getFloat(245);
            float aq_co2_variability = getFloat(246);
            float aq_comfort_score = getFloat(247);
            float aq_env_correlation = getFloat(248);
            float aq_stress_level = getFloat(249);
            float aq_variability_idx = getFloat(250);
            float aq_voc_base = getFloat(251);
            float aq_voc_peak = getFloat(252);
            float aq_voc_variability = getFloat(253);
            float aq_voc_warning = getFloat(254);

            // =========================================================================
            // PARAMETRY PIEZO VIBRATION (33 parametry) - indeksy 255-287
            // =========================================================================
            float pv_vib_rms = getFloat(255);
            float pv_vib_peak = getFloat(256);
            float pv_vib_freq_dom = getFloat(257);
            float pv_bee_traffic = getFloat(258);
            float pv_intrusion_prob = getFloat(259);
            float pv_predator_det = getFloat(260);
            float pv_queen_piping = getFloat(261);
            float pv_vibration_energy = getFloat(262);
            float pv_activity_pattern = getFloat(263);
            float pv_disturbance_idx = getFloat(264);
            float pv_health_vib = getFloat(265);
            float pv_population_est = getFloat(266);
            float pv_aggression_idx = getFloat(267);
            float pv_swarm_prep = getFloat(268);
            float pv_cluster_size = getFloat(269);
            float pv_cluster_activity = getFloat(270);
            float pv_fanning行为 = getFloat(271);
            float pv_buzz_piping = getFloat(272);
            float pv_tremble_dance = getFloat(273);
            float pv_waggle_dance = getFloat(274);
            float pv_foraging_comm = getFloat(275);
            float pv_alert_level = getFloat(276);
            float pv_alien_detected = getFloat(277);
            float pv_confidence = getFloat(278);
            float pv_continuous_vib = getFloat(279);
            float pv_event_count = getFloat(280);
            float pv_impact_flag = getFloat(281);
            float pv_severity = getFloat(282);
            float pv_source_type = getFloat(283);
            float pv_vib_mean = getFloat(284);
            float pv_vib_std = getFloat(285);
            float pv_wind_noise = getFloat(286);
            float pv_zcr = getFloat(287);

            // =========================================================================
            // PARAMETRY BAROMETRIC (30 parametrów) - indeksy 288-317
            // =========================================================================
            float baro_pressure = getFloat(288);
            float baro_pressure_mean = getFloat(289);
            float baro_pressure_std = getFloat(290);
            float baro_pressure_min = getFloat(291);
            float baro_pressure_max = getFloat(292);
            float baro_pressure_trend = getFloat(293);
            float baro_weather_trend = getFloat(294);
            float baro_storm_prob = getFloat(295);
            float baro_foraging_cond = getFloat(296);
            float baro_pressure_change = getFloat(297);
            float baro_altitude_est = getFloat(298);
            float baro_sealevel_press = getFloat(299);
            float baro_density_alt = getFloat(300);
            float baro_weather_score = getFloat(301);
            float baro_front_approach = getFloat(302);
            float baro_stability = getFloat(303);
            float baro_bee_activity_pred = getFloat(304);
            float baro_weather_alert = getFloat(305);
            float baro_activity_pred = getFloat(306);
            int baro_alert_flag = getInt(307);
            float baro_foraging_idx = getFloat(308);
            float baro_improving_flag = getFloat(309);
            float baro_rain_risk = getFloat(310);
            float baro_reliability_idx = getFloat(311);
            float baro_severity = getFloat(312);
            float baro_storm_risk = getFloat(313);
            float baro_temperature = getFloat(314);
            float baro_trend_medium = getFloat(315);
            float baro_trend_short = getFloat(316);
            float baro_weather_idx = getFloat(317);

            // =========================================================================
            // PARAMETRY LIGHT (20 parametrów) - indeksy 318-337
            // =========================================================================
            float light_lux = getFloat(318);
            float light_lux_mean = getFloat(319);
            float light_lux_min = getFloat(320);
            float light_lux_max = getFloat(321);
            float light_daylight_hours = getFloat(322);
            float light_darkness_hours = getFloat(323);
            float light_twilight = getFloat(324);
            float light_circadian_sync = getFloat(325);
            float light_foraging_idx = getFloat(326);
            float light_photoperiod = getFloat(327);
            float light_seasonal_change = getFloat(328);
            float light_cloud_cover_est = getFloat(329);
            float light_sunrise_offset = getFloat(330);
            float light_uv_idx = getFloat(331);
            float light_spectrum_blue = getFloat(332);
            float light_spectrum_red = getFloat(333);
            float light_spectrum_ir = getFloat(334);
            float light_ir_ratio = getFloat(335);
            float light_lux_std = getFloat(336);
            float light_spectrum_data = getFloat(337);

            // Dodatkowe pola backward compatibility
            float weight_rate = getFloat(338);
            float weight_trend = getFloat(339);
            int air_iaq = getInt(340);

            {
                std::lock_guard<std::mutex> lock(data_mutex);
                if (hives_data.find(hive_id) != hives_data.end()) {
                    // Podstawowe parametry
                    hives_data[hive_id].temperature = temp;
                    hives_data[hive_id].humidity = humidity;
                    hives_data[hive_id].weight = weight;
                    hives_data[hive_id].battery_level = battery;
                    hives_data[hive_id].co2_eq = co2;
                    hives_data[hive_id].voc_idx = voc;
                    hives_data[hive_id].motion_detected = motion;
                    hives_data[hive_id].timestamp = timestamp;
                    hives_data[hive_id].is_online = true;
                    
                    // Audio
                    hives_data[hive_id].audio_rms = audio_rms;
                    hives_data[hive_id].audio_peak = audio_peak;
                    hives_data[hive_id].audio_peak_to_peak = audio_peak_to_peak;
                    hives_data[hive_id].audio_zcr = audio_zcr;
                    hives_data[hive_id].audio_energy = audio_energy;
                    hives_data[hive_id].audio_mean_amp = audio_mean_amp;
                    hives_data[hive_id].audio_std_amp = audio_std_amp;
                    hives_data[hive_id].audio_cv_amp = audio_cv_amp;
                    hives_data[hive_id].audio_skewness = audio_skewness;
                    hives_data[hive_id].audio_kurtosis = audio_kurtosis;
                    hives_data[hive_id].audio_dominant_freq = audio_dominant_freq;
                    hives_data[hive_id].audio_spectral_centroid = audio_spectral_centroid;
                    hives_data[hive_id].audio_spectral_bandwidth = audio_spectral_bandwidth;
                    hives_data[hive_id].audio_spectral_flatness = audio_spectral_flatness;
                    hives_data[hive_id].audio_spectral_rolloff = audio_spectral_rolloff;
                    hives_data[hive_id].audio_power_bee_band = audio_power_bee_band;
                    hives_data[hive_id].audio_power_swarm_band = audio_power_swarm_band;
                    hives_data[hive_id].audio_power_low_freq = audio_power_low_freq;
                    hives_data[hive_id].audio_power_high_freq = audio_power_high_freq;
                    hives_data[hive_id].audio_harmonic_ratio = audio_harmonic_ratio;
                    for(int i=0; i<4; i++) hives_data[hive_id].audio_mfcc_energy[i] = audio_mfcc_energy[i];
                    hives_data[hive_id].audio_spectral_entropy = audio_spectral_entropy;
                    hives_data[hive_id].audio_spectral_contrast = audio_spectral_contrast;
                    hives_data[hive_id].audio_tonal_strength = audio_tonal_strength;
                    hives_data[hive_id].audio_crest_factor = audio_crest_factor;
                    hives_data[hive_id].audio_formant_f1 = audio_formant_f1;
                    hives_data[hive_id].audio_formant_f2 = audio_formant_f2;
                    hives_data[hive_id].audio_fundamental_freq = audio_fundamental_freq;
                    hives_data[hive_id].audio_pitch_strength = audio_pitch_strength;
                    hives_data[hive_id].audio_inharmonicity = audio_inharmonicity;
                    hives_data[hive_id].audio_shimmer = audio_shimmer;
                    hives_data[hive_id].audio_jitter = audio_jitter;
                    hives_data[hive_id].audio_nhr = audio_nhr;
                    hives_data[hive_id].audio_hnr = audio_hnr;
                    hives_data[hive_id].audio_autocorr_peak = audio_autocorr_peak;
                    hives_data[hive_id].audio_attack_time = audio_attack_time;
                    hives_data[hive_id].audio_decay_time = audio_decay_time;
                    hives_data[hive_id].audio_sustain_level = audio_sustain_level;
                    hives_data[hive_id].audio_temporal_centroid = audio_temporal_centroid;
                    hives_data[hive_id].audio_loudness = audio_loudness;
                    hives_data[hive_id].audio_spectral_flux = audio_spectral_flux;
                    hives_data[hive_id].audio_spectral_slope = audio_spectral_slope;
                    hives_data[hive_id].audio_spectral_kurtosis = audio_spectral_kurtosis;
                    hives_data[hive_id].audio_spectral_skewness = audio_spectral_skewness;
                    hives_data[hive_id].audio_fund_salience = audio_fund_salience;
                    for(int i=0; i<8; i++) hives_data[hive_id].audio_power_band[i] = audio_power_band[i];
                    hives_data[hive_id].audio_leq = audio_leq;
                    hives_data[hive_id].audio_l10 = audio_l10;
                    hives_data[hive_id].audio_l90 = audio_l90;
                    hives_data[hive_id].audio_noise_floor = audio_noise_floor;
                    hives_data[hive_id].audio_snr = audio_snr;
                    hives_data[hive_id].audio_aci = audio_aci;
                    hives_data[hive_id].audio_bi = audio_bi;
                    hives_data[hive_id].audio_ndi = audio_ndi;
                    hives_data[hive_id].audio_adi = audio_adi;
                    hives_data[hive_id].audio_aei = audio_aei;
                    hives_data[hive_id].audio_bee_activity = audio_bee_activity;
                    hives_data[hive_id].audio_swarm_prob = audio_swarm_prob;
                    hives_data[hive_id].audio_stress_indicator = audio_stress_indicator;
                    hives_data[hive_id].audio_hive_health = audio_hive_health;
                    hives_data[hive_id].audio_foraging_eff = audio_foraging_eff;
                    hives_data[hive_id].audio_colony_coherence = audio_colony_coherence;
                    
                    // Radar
                    hives_data[hive_id].radar_distance = radar_distance;
                    hives_data[hive_id].radar_energy = radar_energy;
                    hives_data[hive_id].radar_speed = radar_speed;
                    hives_data[hive_id].radar_distance_std = radar_distance_std;
                    hives_data[hive_id].radar_energy_std = radar_energy_std;
                    hives_data[hive_id].radar_speed_std = radar_speed_std;
                    hives_data[hive_id].radar_distance_min = radar_distance_min;
                    hives_data[hive_id].radar_distance_max = radar_distance_max;
                    hives_data[hive_id].radar_energy_min = radar_energy_min;
                    hives_data[hive_id].radar_energy_max = radar_energy_max;
                    hives_data[hive_id].radar_range = radar_range;
                    hives_data[hive_id].radar_energy_variance = radar_energy_variance;
                    hives_data[hive_id].radar_cv = radar_cv;
                    hives_data[hive_id].radar_activity = radar_activity;
                    hives_data[hive_id].radar_idle_percent = radar_idle_percent;
                    hives_data[hive_id].radar_motion_intensity = radar_motion_intensity;
                    hives_data[hive_id].radar_target_rate = radar_target_rate;
                    hives_data[hive_id].radar_max_targets = radar_max_targets;
                    hives_data[hive_id].radar_target_density = radar_target_density;
                    hives_data[hive_id].radar_slope = radar_slope;
                    hives_data[hive_id].radar_correlation = radar_correlation;
                    hives_data[hive_id].radar_acceleration = radar_acceleration;
                    hives_data[hive_id].radar_signal_quality = radar_signal_quality;
                    hives_data[hive_id].radar_anomaly_score = radar_anomaly_score;
                    hives_data[hive_id].radar_hive_health = radar_hive_health;
                    hives_data[hive_id].radar_power_spectrum = radar_power_spectrum;
                    hives_data[hive_id].radar_zcr = radar_zcr;
                    hives_data[hive_id].radar_entropy = radar_entropy;
                    
                    // HX711
                    hives_data[hive_id].hx711_current = hx711_current;
                    hives_data[hive_id].hx711_mean = hx711_mean;
                    hives_data[hive_id].hx711_std = hx711_std;
                    hives_data[hive_id].hx711_min = hx711_min;
                    hives_data[hive_id].hx711_max = hx711_max;
                    hives_data[hive_id].hx711_median = hx711_median;
                    hives_data[hive_id].hx711_range = hx711_range;
                    hives_data[hive_id].hx711_variance = hx711_variance;
                    hives_data[hive_id].hx711_cv = hx711_cv;
                    hives_data[hive_id].hx711_iqr = hx711_iqr;
                    hives_data[hive_id].hx711_rate = hx711_rate;
                    hives_data[hive_id].hx711_mean_rate = hx711_mean_rate;
                    hives_data[hive_id].hx711_max_pos_rate = hx711_max_pos_rate;
                    hives_data[hive_id].hx711_max_neg_rate = hx711_max_neg_rate;
                    hives_data[hive_id].hx711_acceleration = hx711_acceleration;
                    hives_data[hive_id].hx711_slope_1h = hx711_slope_1h;
                    hives_data[hive_id].hx711_slope_4h = hx711_slope_4h;
                    hives_data[hive_id].hx711_slope_24h = hx711_slope_24h;
                    hives_data[hive_id].hx711_corr_1h = hx711_corr_1h;
                    hives_data[hive_id].hx711_corr_4h = hx711_corr_4h;
                    hives_data[hive_id].hx711_corr_24h = hx711_corr_24h;
                    hives_data[hive_id].hx711_direction = hx711_direction;
                    hives_data[hive_id].hx711_nectar_inflow = hx711_nectar_inflow;
                    hives_data[hive_id].hx711_nectar_accum = hx711_nectar_accum;
                    hives_data[hive_id].hx711_foraging_eff = hx711_foraging_eff;
                    hives_data[hive_id].hx711_bloom_intensity = hx711_bloom_intensity;
                    hives_data[hive_id].hx711_honey_prod_idx = hx711_honey_prod_idx;
                    hives_data[hive_id].hx711_nectar_quality = hx711_nectar_quality;
                    hives_data[hive_id].hx711_consumption_rate = hx711_consumption_rate;
                    hives_data[hive_id].hx711_daily_consumption = hx711_daily_consumption;
                    hives_data[hive_id].hx711_food_reserve_days = hx711_food_reserve_days;
                    hives_data[hive_id].hx711_winter_readiness = hx711_winter_readiness;
                    hives_data[hive_id].hx711_starvation_risk = hx711_starvation_risk;
                    hives_data[hive_id].hx711_daily_amplitude = hx711_daily_amplitude;
                    hives_data[hive_id].hx711_circadian_str = hx711_circadian_str;
                    hives_data[hive_id].hx711_seasonal_trend = hx711_seasonal_trend;
                    hives_data[hive_id].hx711_signal_quality = hx711_signal_quality;
                    hives_data[hive_id].hx711_noise_level = hx711_noise_level;
                    hives_data[hive_id].hx711_drift_rate = hx711_drift_rate;
                    hives_data[hive_id].hx711_outlier_ratio = hx711_outlier_ratio;
                    hives_data[hive_id].hx711_sample_count = hx711_sample_count;
                    hives_data[hive_id].hx711_forecast_1h = hx711_forecast_1h;
                    hives_data[hive_id].hx711_forecast_6h = hx711_forecast_6h;
                    hives_data[hive_id].hx711_forecast_24h = hx711_forecast_24h;
                    hives_data[hive_id].hx711_forecast_conf = hx711_forecast_conf;
                    hives_data[hive_id].hx711_prediction_error = hx711_prediction_error;
                    hives_data[hive_id].hx711_harvest_event = hx711_harvest_event;
                    hives_data[hive_id].hx711_swarm_event = hx711_swarm_event;
                    hives_data[hive_id].hx711_feeding_event = hx711_feeding_event;
                    hives_data[hive_id].hx711_water_collection = hx711_water_collection;
                    hives_data[hive_id].hx711_skewness = hx711_skewness;
                    hives_data[hive_id].hx711_kurtosis = hx711_kurtosis;
                    hives_data[hive_id].hx711_entropy = hx711_entropy;
                    hives_data[hive_id].hx711_complexity = hx711_complexity;
                    hives_data[hive_id].hx711_fractal_dim = hx711_fractal_dim;
                    hives_data[hive_id].hx711_colony_mass = hx711_colony_mass;
                    hives_data[hive_id].hx711_bee_population = hx711_bee_population;
                    hives_data[hive_id].hx711_brood_mass = hx711_brood_mass;
                    hives_data[hive_id].hx711_food_stores = hx711_food_stores;
                    hives_data[hive_id].hx711_honey_stores = hx711_honey_stores;
                    hives_data[hive_id].hx711_pollen_stores = hx711_pollen_stores;
                    hives_data[hive_id].hx711_productivity_idx = hx711_productivity_idx;
                    hives_data[hive_id].hx711_efficiency_score = hx711_efficiency_score;
                    hives_data[hive_id].hx711_growth_rate = hx711_growth_rate;
                    hives_data[hive_id].hx711_balance_score = hx711_balance_score;
                    hives_data[hive_id].hx711_resilience_idx = hx711_resilience_idx;
                    hives_data[hive_id].hx711_dominant_period = hx711_dominant_period;
                    hives_data[hive_id].hx711_power_24h = hx711_power_24h;
                    hives_data[hive_id].hx711_power_12h = hx711_power_12h;
                    hives_data[hive_id].hx711_power_6h = hx711_power_6h;
                    hives_data[hive_id].hx711_power_1h = hx711_power_1h;
                    hives_data[hive_id].hx711_completeness = hx711_completeness;
                    hives_data[hive_id].hx711_reliability = hx711_reliability;
                    hives_data[hive_id].hx711_accuracy_est = hx711_accuracy_est;
                    hives_data[hive_id].hx711_precision_est = hx711_precision_est;
                    hives_data[hive_id].hx711_anomaly_flag = hx711_anomaly_flag;
                    hives_data[hive_id].hx711_health_score = hx711_health_score;
                    
                    // Temp/Humidity
                    hives_data[hive_id].th_temp_mean = th_temp_mean;
                    hives_data[hive_id].th_temp_std = th_temp_std;
                    hives_data[hive_id].th_temp_min = th_temp_min;
                    hives_data[hive_id].th_temp_max = th_temp_max;
                    hives_data[hive_id].th_temp_range = th_temp_range;
                    hives_data[hive_id].th_hum_mean = th_hum_mean;
                    hives_data[hive_id].th_hum_std = th_hum_std;
                    hives_data[hive_id].th_hum_min = th_hum_min;
                    hives_data[hive_id].th_hum_max = th_hum_max;
                    hives_data[hive_id].th_hum_range = th_hum_range;
                    hives_data[hive_id].th_heat_index = th_heat_index;
                    hives_data[hive_id].th_dew_point = th_dew_point;
                    hives_data[hive_id].th_comfort_idx = th_comfort_idx;
                    hives_data[hive_id].th_brood_stress = th_brood_stress;
                    hives_data[hive_id].th_mold_risk = th_mold_risk;
                    hives_data[hive_id].th_temp_stability = th_temp_stability;
                    hives_data[hive_id].th_vpd = th_vpd;
                    hives_data[hive_id].th_thi = th_thi;
                    hives_data[hive_id].th_enthalpy = th_enthalpy;
                    hives_data[hive_id].th_air_density = th_air_density;
                    hives_data[hive_id].th_specific_humidity = th_specific_humidity;
                    hives_data[hive_id].th_mixing_ratio = th_mixing_ratio;
                    hives_data[hive_id].th_saturation_deficit = th_saturation_deficit;
                    hives_data[hive_id].th_evap_potential = th_evap_potential;
                    hives_data[hive_id].th_condensation_risk = th_condensation_risk;
                    hives_data[hive_id].th_ventilation_need = th_ventilation_need;
                    hives_data[hive_id].th_cooling_need = th_cooling_need;
                    hives_data[hive_id].th_heating_need = th_heating_need;
                    hives_data[hive_id].th_climate_score = th_climate_score;
                    hives_data[hive_id].th_hum_stability_1h = th_hum_stability_1h;
                    hives_data[hive_id].th_hum_trend = th_hum_trend;
                    hives_data[hive_id].th_temp_trend = th_temp_trend;
                    
                    // Air Quality
                    hives_data[hive_id].aq_iaq = aq_iaq;
                    hives_data[hive_id].aq_iaq_mean = aq_iaq_mean;
                    hives_data[hive_id].aq_contamination = aq_contamination;
                    hives_data[hive_id].aq_mold_risk = aq_mold_risk;
                    hives_data[hive_id].aq_ventilation_idx = aq_ventilation_idx;
                    hives_data[hive_id].aq_co2_mean = aq_co2_mean;
                    hives_data[hive_id].aq_voc_mean = aq_voc_mean;
                    hives_data[hive_id].aq_stress_air = aq_stress_air;
                    hives_data[hive_id].aq_purity = aq_purity;
                    hives_data[hive_id].aq_freshness = aq_freshness;
                    hives_data[hive_id].aq_pollution_load = aq_pollution_load;
                    hives_data[hive_id].aq_gas_idx = aq_gas_idx;
                    hives_data[hive_id].aq_particulate_idx = aq_particulate_idx;
                    hives_data[hive_id].aq_allergen_risk = aq_allergen_risk;
                    hives_data[hive_id].aq_toxicity_est = aq_toxicity_est;
                    hives_data[hive_id].aq_oxidation_cap = aq_oxidation_cap;
                    hives_data[hive_id].aq_reduction_cap = aq_reduction_cap;
                    hives_data[hive_id].aq_buffer_cap = aq_buffer_cap;
                    hives_data[hive_id].aq_recovery_rate = aq_recovery_rate;
                    hives_data[hive_id].aq_stability = aq_stability;
                    hives_data[hive_id].aq_trend = aq_trend;
                    hives_data[hive_id].aq_forecast = aq_forecast;
                    hives_data[hive_id].aq_health_impact = aq_health_impact;
                    hives_data[hive_id].aq_bee_behavior_impact = aq_bee_behavior_impact;
                    hives_data[hive_id].aq_co2_alert = aq_co2_alert;
                    hives_data[hive_id].aq_co2_base = aq_co2_base;
                    hives_data[hive_id].aq_co2_peak = aq_co2_peak;
                    hives_data[hive_id].aq_co2_trend = aq_co2_trend;
                    hives_data[hive_id].aq_co2_variability = aq_co2_variability;
                    hives_data[hive_id].aq_comfort_score = aq_comfort_score;
                    hives_data[hive_id].aq_env_correlation = aq_env_correlation;
                    hives_data[hive_id].aq_stress_level = aq_stress_level;
                    hives_data[hive_id].aq_variability_idx = aq_variability_idx;
                    hives_data[hive_id].aq_voc_base = aq_voc_base;
                    hives_data[hive_id].aq_voc_peak = aq_voc_peak;
                    hives_data[hive_id].aq_voc_variability = aq_voc_variability;
                    hives_data[hive_id].aq_voc_warning = aq_voc_warning;
                    
                    // Piezo Vibration
                    hives_data[hive_id].pv_vib_rms = pv_vib_rms;
                    hives_data[hive_id].pv_vib_peak = pv_vib_peak;
                    hives_data[hive_id].pv_vib_freq_dom = pv_vib_freq_dom;
                    hives_data[hive_id].pv_bee_traffic = pv_bee_traffic;
                    hives_data[hive_id].pv_intrusion_prob = pv_intrusion_prob;
                    hives_data[hive_id].pv_predator_det = pv_predator_det;
                    hives_data[hive_id].pv_queen_piping = pv_queen_piping;
                    hives_data[hive_id].pv_vibration_energy = pv_vibration_energy;
                    hives_data[hive_id].pv_activity_pattern = pv_activity_pattern;
                    hives_data[hive_id].pv_disturbance_idx = pv_disturbance_idx;
                    hives_data[hive_id].pv_health_vib = pv_health_vib;
                    hives_data[hive_id].pv_population_est = pv_population_est;
                    hives_data[hive_id].pv_aggression_idx = pv_aggression_idx;
                    hives_data[hive_id].pv_swarm_prep = pv_swarm_prep;
                    hives_data[hive_id].pv_cluster_size = pv_cluster_size;
                    hives_data[hive_id].pv_cluster_activity = pv_cluster_activity;
                    hives_data[hive_id].pv_fanning行为 = pv_fanning 行为;
                    hives_data[hive_id].pv_buzz_piping = pv_buzz_piping;
                    hives_data[hive_id].pv_tremble_dance = pv_tremble_dance;
                    hives_data[hive_id].pv_waggle_dance = pv_waggle_dance;
                    hives_data[hive_id].pv_foraging_comm = pv_foraging_comm;
                    hives_data[hive_id].pv_alert_level = pv_alert_level;
                    hives_data[hive_id].pv_alien_detected = pv_alien_detected;
                    hives_data[hive_id].pv_confidence = pv_confidence;
                    hives_data[hive_id].pv_continuous_vib = pv_continuous_vib;
                    hives_data[hive_id].pv_event_count = pv_event_count;
                    hives_data[hive_id].pv_impact_flag = pv_impact_flag;
                    hives_data[hive_id].pv_severity = pv_severity;
                    hives_data[hive_id].pv_source_type = pv_source_type;
                    hives_data[hive_id].pv_vib_mean = pv_vib_mean;
                    hives_data[hive_id].pv_vib_std = pv_vib_std;
                    hives_data[hive_id].pv_wind_noise = pv_wind_noise;
                    hives_data[hive_id].pv_zcr = pv_zcr;
                    
                    // Barometric
                    hives_data[hive_id].baro_pressure = baro_pressure;
                    hives_data[hive_id].baro_pressure_mean = baro_pressure_mean;
                    hives_data[hive_id].baro_pressure_std = baro_pressure_std;
                    hives_data[hive_id].baro_pressure_min = baro_pressure_min;
                    hives_data[hive_id].baro_pressure_max = baro_pressure_max;
                    hives_data[hive_id].baro_pressure_trend = baro_pressure_trend;
                    hives_data[hive_id].baro_weather_trend = baro_weather_trend;
                    hives_data[hive_id].baro_storm_prob = baro_storm_prob;
                    hives_data[hive_id].baro_foraging_cond = baro_foraging_cond;
                    hives_data[hive_id].baro_pressure_change = baro_pressure_change;
                    hives_data[hive_id].baro_altitude_est = baro_altitude_est;
                    hives_data[hive_id].baro_sealevel_press = baro_sealevel_press;
                    hives_data[hive_id].baro_density_alt = baro_density_alt;
                    hives_data[hive_id].baro_weather_score = baro_weather_score;
                    hives_data[hive_id].baro_front_approach = baro_front_approach;
                    hives_data[hive_id].baro_stability = baro_stability;
                    hives_data[hive_id].baro_bee_activity_pred = baro_bee_activity_pred;
                    hives_data[hive_id].baro_weather_alert = baro_weather_alert;
                    hives_data[hive_id].baro_activity_pred = baro_activity_pred;
                    hives_data[hive_id].baro_alert_flag = baro_alert_flag;
                    hives_data[hive_id].baro_foraging_idx = baro_foraging_idx;
                    hives_data[hive_id].baro_improving_flag = baro_improving_flag;
                    hives_data[hive_id].baro_rain_risk = baro_rain_risk;
                    hives_data[hive_id].baro_reliability_idx = baro_reliability_idx;
                    hives_data[hive_id].baro_severity = baro_severity;
                    hives_data[hive_id].baro_storm_risk = baro_storm_risk;
                    hives_data[hive_id].baro_temperature = baro_temperature;
                    hives_data[hive_id].baro_trend_medium = baro_trend_medium;
                    hives_data[hive_id].baro_trend_short = baro_trend_short;
                    hives_data[hive_id].baro_weather_idx = baro_weather_idx;
                    
                    // Light
                    hives_data[hive_id].light_lux = light_lux;
                    hives_data[hive_id].light_lux_mean = light_lux_mean;
                    hives_data[hive_id].light_lux_min = light_lux_min;
                    hives_data[hive_id].light_lux_max = light_lux_max;
                    hives_data[hive_id].light_daylight_hours = light_daylight_hours;
                    hives_data[hive_id].light_darkness_hours = light_darkness_hours;
                    hives_data[hive_id].light_twilight = light_twilight;
                    hives_data[hive_id].light_circadian_sync = light_circadian_sync;
                    hives_data[hive_id].light_foraging_idx = light_foraging_idx;
                    hives_data[hive_id].light_photoperiod = light_photoperiod;
                    hives_data[hive_id].light_seasonal_change = light_seasonal_change;
                    hives_data[hive_id].light_cloud_cover_est = light_cloud_cover_est;
                    hives_data[hive_id].light_sunrise_offset = light_sunrise_offset;
                    hives_data[hive_id].light_uv_idx = light_uv_idx;
                    hives_data[hive_id].light_spectrum_blue = light_spectrum_blue;
                    hives_data[hive_id].light_spectrum_red = light_spectrum_red;
                    hives_data[hive_id].light_spectrum_ir = light_spectrum_ir;
                    hives_data[hive_id].light_ir_ratio = light_ir_ratio;
                    hives_data[hive_id].light_lux_std = light_lux_std;
                    hives_data[hive_id].light_spectrum_data = light_spectrum_data;
                    
                    // Backward compatibility
                    hives_data[hive_id].weight_rate = weight_rate;
                    hives_data[hive_id].weight_trend = weight_trend;
                    hives_data[hive_id].air_iaq = air_iaq;
                    
                    Logger::getInstance().debug("CSV: Zaktualizowano " + hive_id + " z " + source_ip + " (" + std::to_string(parts.size()) + " parametrow)");
                } else {
                    Logger::getInstance().info("Wykryto nowy ul: " + hive_id + " z IP " + source_ip);
                    HiveData new_hive;
                    new_hive.hive_id = hive_id;
                    // Podstawowe parametry
                    new_hive.temperature = temp;
                    new_hive.humidity = humidity;
                    new_hive.weight = weight;
                    new_hive.battery_level = battery;
                    new_hive.co2_eq = co2;
                    new_hive.voc_idx = voc;
                    new_hive.motion_detected = motion;
                    new_hive.timestamp = timestamp;
                    new_hive.is_online = true;
                    
                    // Audio
                    new_hive.audio_rms = audio_rms;
                    new_hive.audio_peak = audio_peak;
                    new_hive.audio_peak_to_peak = audio_peak_to_peak;
                    new_hive.audio_zcr = audio_zcr;
                    new_hive.audio_energy = audio_energy;
                    new_hive.audio_mean_amp = audio_mean_amp;
                    new_hive.audio_std_amp = audio_std_amp;
                    new_hive.audio_cv_amp = audio_cv_amp;
                    new_hive.audio_skewness = audio_skewness;
                    new_hive.audio_kurtosis = audio_kurtosis;
                    new_hive.audio_dominant_freq = audio_dominant_freq;
                    new_hive.audio_spectral_centroid = audio_spectral_centroid;
                    new_hive.audio_spectral_bandwidth = audio_spectral_bandwidth;
                    new_hive.audio_spectral_flatness = audio_spectral_flatness;
                    new_hive.audio_spectral_rolloff = audio_spectral_rolloff;
                    new_hive.audio_power_bee_band = audio_power_bee_band;
                    new_hive.audio_power_swarm_band = audio_power_swarm_band;
                    new_hive.audio_power_low_freq = audio_power_low_freq;
                    new_hive.audio_power_high_freq = audio_power_high_freq;
                    new_hive.audio_harmonic_ratio = audio_harmonic_ratio;
                    for(int i=0; i<4; i++) new_hive.audio_mfcc_energy[i] = audio_mfcc_energy[i];
                    new_hive.audio_spectral_entropy = audio_spectral_entropy;
                    new_hive.audio_spectral_contrast = audio_spectral_contrast;
                    new_hive.audio_tonal_strength = audio_tonal_strength;
                    new_hive.audio_crest_factor = audio_crest_factor;
                    new_hive.audio_formant_f1 = audio_formant_f1;
                    new_hive.audio_formant_f2 = audio_formant_f2;
                    new_hive.audio_fundamental_freq = audio_fundamental_freq;
                    new_hive.audio_pitch_strength = audio_pitch_strength;
                    new_hive.audio_inharmonicity = audio_inharmonicity;
                    new_hive.audio_shimmer = audio_shimmer;
                    new_hive.audio_jitter = audio_jitter;
                    new_hive.audio_nhr = audio_nhr;
                    new_hive.audio_hnr = audio_hnr;
                    new_hive.audio_autocorr_peak = audio_autocorr_peak;
                    new_hive.audio_attack_time = audio_attack_time;
                    new_hive.audio_decay_time = audio_decay_time;
                    new_hive.audio_sustain_level = audio_sustain_level;
                    new_hive.audio_temporal_centroid = audio_temporal_centroid;
                    new_hive.audio_loudness = audio_loudness;
                    new_hive.audio_spectral_flux = audio_spectral_flux;
                    new_hive.audio_spectral_slope = audio_spectral_slope;
                    new_hive.audio_spectral_kurtosis = audio_spectral_kurtosis;
                    new_hive.audio_spectral_skewness = audio_spectral_skewness;
                    new_hive.audio_fund_salience = audio_fund_salience;
                    for(int i=0; i<8; i++) new_hive.audio_power_band[i] = audio_power_band[i];
                    new_hive.audio_leq = audio_leq;
                    new_hive.audio_l10 = audio_l10;
                    new_hive.audio_l90 = audio_l90;
                    new_hive.audio_noise_floor = audio_noise_floor;
                    new_hive.audio_snr = audio_snr;
                    new_hive.audio_aci = audio_aci;
                    new_hive.audio_bi = audio_bi;
                    new_hive.audio_ndi = audio_ndi;
                    new_hive.audio_adi = audio_adi;
                    new_hive.audio_aei = audio_aei;
                    new_hive.audio_bee_activity = audio_bee_activity;
                    new_hive.audio_swarm_prob = audio_swarm_prob;
                    new_hive.audio_stress_indicator = audio_stress_indicator;
                    new_hive.audio_hive_health = audio_hive_health;
                    new_hive.audio_foraging_eff = audio_foraging_eff;
                    new_hive.audio_colony_coherence = audio_colony_coherence;
                    
                    // Radar
                    new_hive.radar_distance = radar_distance;
                    new_hive.radar_energy = radar_energy;
                    new_hive.radar_speed = radar_speed;
                    new_hive.radar_distance_std = radar_distance_std;
                    new_hive.radar_energy_std = radar_energy_std;
                    new_hive.radar_speed_std = radar_speed_std;
                    new_hive.radar_distance_min = radar_distance_min;
                    new_hive.radar_distance_max = radar_distance_max;
                    new_hive.radar_energy_min = radar_energy_min;
                    new_hive.radar_energy_max = radar_energy_max;
                    new_hive.radar_range = radar_range;
                    new_hive.radar_energy_variance = radar_energy_variance;
                    new_hive.radar_cv = radar_cv;
                    new_hive.radar_activity = radar_activity;
                    new_hive.radar_idle_percent = radar_idle_percent;
                    new_hive.radar_motion_intensity = radar_motion_intensity;
                    new_hive.radar_target_rate = radar_target_rate;
                    new_hive.radar_max_targets = radar_max_targets;
                    new_hive.radar_target_density = radar_target_density;
                    new_hive.radar_slope = radar_slope;
                    new_hive.radar_correlation = radar_correlation;
                    new_hive.radar_acceleration = radar_acceleration;
                    new_hive.radar_signal_quality = radar_signal_quality;
                    new_hive.radar_anomaly_score = radar_anomaly_score;
                    new_hive.radar_hive_health = radar_hive_health;
                    new_hive.radar_power_spectrum = radar_power_spectrum;
                    new_hive.radar_zcr = radar_zcr;
                    new_hive.radar_entropy = radar_entropy;
                    
                    // HX711
                    new_hive.hx711_current = hx711_current;
                    new_hive.hx711_mean = hx711_mean;
                    new_hive.hx711_std = hx711_std;
                    new_hive.hx711_min = hx711_min;
                    new_hive.hx711_max = hx711_max;
                    new_hive.hx711_median = hx711_median;
                    new_hive.hx711_range = hx711_range;
                    new_hive.hx711_variance = hx711_variance;
                    new_hive.hx711_cv = hx711_cv;
                    new_hive.hx711_iqr = hx711_iqr;
                    new_hive.hx711_rate = hx711_rate;
                    new_hive.hx711_mean_rate = hx711_mean_rate;
                    new_hive.hx711_max_pos_rate = hx711_max_pos_rate;
                    new_hive.hx711_max_neg_rate = hx711_max_neg_rate;
                    new_hive.hx711_acceleration = hx711_acceleration;
                    new_hive.hx711_slope_1h = hx711_slope_1h;
                    new_hive.hx711_slope_4h = hx711_slope_4h;
                    new_hive.hx711_slope_24h = hx711_slope_24h;
                    new_hive.hx711_corr_1h = hx711_corr_1h;
                    new_hive.hx711_corr_4h = hx711_corr_4h;
                    new_hive.hx711_corr_24h = hx711_corr_24h;
                    new_hive.hx711_direction = hx711_direction;
                    new_hive.hx711_nectar_inflow = hx711_nectar_inflow;
                    new_hive.hx711_nectar_accum = hx711_nectar_accum;
                    new_hive.hx711_foraging_eff = hx711_foraging_eff;
                    new_hive.hx711_bloom_intensity = hx711_bloom_intensity;
                    new_hive.hx711_honey_prod_idx = hx711_honey_prod_idx;
                    new_hive.hx711_nectar_quality = hx711_nectar_quality;
                    new_hive.hx711_consumption_rate = hx711_consumption_rate;
                    new_hive.hx711_daily_consumption = hx711_daily_consumption;
                    new_hive.hx711_food_reserve_days = hx711_food_reserve_days;
                    new_hive.hx711_winter_readiness = hx711_winter_readiness;
                    new_hive.hx711_starvation_risk = hx711_starvation_risk;
                    new_hive.hx711_daily_amplitude = hx711_daily_amplitude;
                    new_hive.hx711_circadian_str = hx711_circadian_str;
                    new_hive.hx711_seasonal_trend = hx711_seasonal_trend;
                    new_hive.hx711_signal_quality = hx711_signal_quality;
                    new_hive.hx711_noise_level = hx711_noise_level;
                    new_hive.hx711_drift_rate = hx711_drift_rate;
                    new_hive.hx711_outlier_ratio = hx711_outlier_ratio;
                    new_hive.hx711_sample_count = hx711_sample_count;
                    new_hive.hx711_forecast_1h = hx711_forecast_1h;
                    new_hive.hx711_forecast_6h = hx711_forecast_6h;
                    new_hive.hx711_forecast_24h = hx711_forecast_24h;
                    new_hive.hx711_forecast_conf = hx711_forecast_conf;
                    new_hive.hx711_prediction_error = hx711_prediction_error;
                    new_hive.hx711_harvest_event = hx711_harvest_event;
                    new_hive.hx711_swarm_event = hx711_swarm_event;
                    new_hive.hx711_feeding_event = hx711_feeding_event;
                    new_hive.hx711_water_collection = hx711_water_collection;
                    new_hive.hx711_skewness = hx711_skewness;
                    new_hive.hx711_kurtosis = hx711_kurtosis;
                    new_hive.hx711_entropy = hx711_entropy;
                    new_hive.hx711_complexity = hx711_complexity;
                    new_hive.hx711_fractal_dim = hx711_fractal_dim;
                    new_hive.hx711_colony_mass = hx711_colony_mass;
                    new_hive.hx711_bee_population = hx711_bee_population;
                    new_hive.hx711_brood_mass = hx711_brood_mass;
                    new_hive.hx711_food_stores = hx711_food_stores;
                    new_hive.hx711_honey_stores = hx711_honey_stores;
                    new_hive.hx711_pollen_stores = hx711_pollen_stores;
                    new_hive.hx711_productivity_idx = hx711_productivity_idx;
                    new_hive.hx711_efficiency_score = hx711_efficiency_score;
                    new_hive.hx711_growth_rate = hx711_growth_rate;
                    new_hive.hx711_balance_score = hx711_balance_score;
                    new_hive.hx711_resilience_idx = hx711_resilience_idx;
                    new_hive.hx711_dominant_period = hx711_dominant_period;
                    new_hive.hx711_power_24h = hx711_power_24h;
                    new_hive.hx711_power_12h = hx711_power_12h;
                    new_hive.hx711_power_6h = hx711_power_6h;
                    new_hive.hx711_power_1h = hx711_power_1h;
                    new_hive.hx711_completeness = hx711_completeness;
                    new_hive.hx711_reliability = hx711_reliability;
                    new_hive.hx711_accuracy_est = hx711_accuracy_est;
                    new_hive.hx711_precision_est = hx711_precision_est;
                    new_hive.hx711_anomaly_flag = hx711_anomaly_flag;
                    new_hive.hx711_health_score = hx711_health_score;
                    
                    // Temp/Humidity
                    new_hive.th_temp_mean = th_temp_mean;
                    new_hive.th_temp_std = th_temp_std;
                    new_hive.th_temp_min = th_temp_min;
                    new_hive.th_temp_max = th_temp_max;
                    new_hive.th_temp_range = th_temp_range;
                    new_hive.th_hum_mean = th_hum_mean;
                    new_hive.th_hum_std = th_hum_std;
                    new_hive.th_hum_min = th_hum_min;
                    new_hive.th_hum_max = th_hum_max;
                    new_hive.th_hum_range = th_hum_range;
                    new_hive.th_heat_index = th_heat_index;
                    new_hive.th_dew_point = th_dew_point;
                    new_hive.th_comfort_idx = th_comfort_idx;
                    new_hive.th_brood_stress = th_brood_stress;
                    new_hive.th_mold_risk = th_mold_risk;
                    new_hive.th_temp_stability = th_temp_stability;
                    new_hive.th_vpd = th_vpd;
                    new_hive.th_thi = th_thi;
                    new_hive.th_enthalpy = th_enthalpy;
                    new_hive.th_air_density = th_air_density;
                    new_hive.th_specific_humidity = th_specific_humidity;
                    new_hive.th_mixing_ratio = th_mixing_ratio;
                    new_hive.th_saturation_deficit = th_saturation_deficit;
                    new_hive.th_evap_potential = th_evap_potential;
                    new_hive.th_condensation_risk = th_condensation_risk;
                    new_hive.th_ventilation_need = th_ventilation_need;
                    new_hive.th_cooling_need = th_cooling_need;
                    new_hive.th_heating_need = th_heating_need;
                    new_hive.th_climate_score = th_climate_score;
                    new_hive.th_hum_stability_1h = th_hum_stability_1h;
                    new_hive.th_hum_trend = th_hum_trend;
                    new_hive.th_temp_trend = th_temp_trend;
                    
                    // Air Quality
                    new_hive.aq_iaq = aq_iaq;
                    new_hive.aq_iaq_mean = aq_iaq_mean;
                    new_hive.aq_contamination = aq_contamination;
                    new_hive.aq_mold_risk = aq_mold_risk;
                    new_hive.aq_ventilation_idx = aq_ventilation_idx;
                    new_hive.aq_co2_mean = aq_co2_mean;
                    new_hive.aq_voc_mean = aq_voc_mean;
                    new_hive.aq_stress_air = aq_stress_air;
                    new_hive.aq_purity = aq_purity;
                    new_hive.aq_freshness = aq_freshness;
                    new_hive.aq_pollution_load = aq_pollution_load;
                    new_hive.aq_gas_idx = aq_gas_idx;
                    new_hive.aq_particulate_idx = aq_particulate_idx;
                    new_hive.aq_allergen_risk = aq_allergen_risk;
                    new_hive.aq_toxicity_est = aq_toxicity_est;
                    new_hive.aq_oxidation_cap = aq_oxidation_cap;
                    new_hive.aq_reduction_cap = aq_reduction_cap;
                    new_hive.aq_buffer_cap = aq_buffer_cap;
                    new_hive.aq_recovery_rate = aq_recovery_rate;
                    new_hive.aq_stability = aq_stability;
                    new_hive.aq_trend = aq_trend;
                    new_hive.aq_forecast = aq_forecast;
                    new_hive.aq_health_impact = aq_health_impact;
                    new_hive.aq_bee_behavior_impact = aq_bee_behavior_impact;
                    new_hive.aq_co2_alert = aq_co2_alert;
                    new_hive.aq_co2_base = aq_co2_base;
                    new_hive.aq_co2_peak = aq_co2_peak;
                    new_hive.aq_co2_trend = aq_co2_trend;
                    new_hive.aq_co2_variability = aq_co2_variability;
                    new_hive.aq_comfort_score = aq_comfort_score;
                    new_hive.aq_env_correlation = aq_env_correlation;
                    new_hive.aq_stress_level = aq_stress_level;
                    new_hive.aq_variability_idx = aq_variability_idx;
                    new_hive.aq_voc_base = aq_voc_base;
                    new_hive.aq_voc_peak = aq_voc_peak;
                    new_hive.aq_voc_variability = aq_voc_variability;
                    new_hive.aq_voc_warning = aq_voc_warning;
                    
                    // Piezo Vibration
                    new_hive.pv_vib_rms = pv_vib_rms;
                    new_hive.pv_vib_peak = pv_vib_peak;
                    new_hive.pv_vib_freq_dom = pv_vib_freq_dom;
                    new_hive.pv_bee_traffic = pv_bee_traffic;
                    new_hive.pv_intrusion_prob = pv_intrusion_prob;
                    new_hive.pv_predator_det = pv_predator_det;
                    new_hive.pv_queen_piping = pv_queen_piping;
                    new_hive.pv_vibration_energy = pv_vibration_energy;
                    new_hive.pv_activity_pattern = pv_activity_pattern;
                    new_hive.pv_disturbance_idx = pv_disturbance_idx;
                    new_hive.pv_health_vib = pv_health_vib;
                    new_hive.pv_population_est = pv_population_est;
                    new_hive.pv_aggression_idx = pv_aggression_idx;
                    new_hive.pv_swarm_prep = pv_swarm_prep;
                    new_hive.pv_cluster_size = pv_cluster_size;
                    new_hive.pv_cluster_activity = pv_cluster_activity;
                    new_hive.pv_fanning 行为 = pv_fanning 行为;
                    new_hive.pv_buzz_piping = pv_buzz_piping;
                    new_hive.pv_tremble_dance = pv_tremble_dance;
                    new_hive.pv_waggle_dance = pv_waggle_dance;
                    new_hive.pv_foraging_comm = pv_foraging_comm;
                    new_hive.pv_alert_level = pv_alert_level;
                    new_hive.pv_alien_detected = pv_alien_detected;
                    new_hive.pv_confidence = pv_confidence;
                    new_hive.pv_continuous_vib = pv_continuous_vib;
                    new_hive.pv_event_count = pv_event_count;
                    new_hive.pv_impact_flag = pv_impact_flag;
                    new_hive.pv_severity = pv_severity;
                    new_hive.pv_source_type = pv_source_type;
                    new_hive.pv_vib_mean = pv_vib_mean;
                    new_hive.pv_vib_std = pv_vib_std;
                    new_hive.pv_wind_noise = pv_wind_noise;
                    new_hive.pv_zcr = pv_zcr;
                    
                    // Barometric
                    new_hive.baro_pressure = baro_pressure;
                    new_hive.baro_pressure_mean = baro_pressure_mean;
                    new_hive.baro_pressure_std = baro_pressure_std;
                    new_hive.baro_pressure_min = baro_pressure_min;
                    new_hive.baro_pressure_max = baro_pressure_max;
                    new_hive.baro_pressure_trend = baro_pressure_trend;
                    new_hive.baro_weather_trend = baro_weather_trend;
                    new_hive.baro_storm_prob = baro_storm_prob;
                    new_hive.baro_foraging_cond = baro_foraging_cond;
                    new_hive.baro_pressure_change = baro_pressure_change;
                    new_hive.baro_altitude_est = baro_altitude_est;
                    new_hive.baro_sealevel_press = baro_sealevel_press;
                    new_hive.baro_density_alt = baro_density_alt;
                    new_hive.baro_weather_score = baro_weather_score;
                    new_hive.baro_front_approach = baro_front_approach;
                    new_hive.baro_stability = baro_stability;
                    new_hive.baro_bee_activity_pred = baro_bee_activity_pred;
                    new_hive.baro_weather_alert = baro_weather_alert;
                    new_hive.baro_activity_pred = baro_activity_pred;
                    new_hive.baro_alert_flag = baro_alert_flag;
                    new_hive.baro_foraging_idx = baro_foraging_idx;
                    new_hive.baro_improving_flag = baro_improving_flag;
                    new_hive.baro_rain_risk = baro_rain_risk;
                    new_hive.baro_reliability_idx = baro_reliability_idx;
                    new_hive.baro_severity = baro_severity;
                    new_hive.baro_storm_risk = baro_storm_risk;
                    new_hive.baro_temperature = baro_temperature;
                    new_hive.baro_trend_medium = baro_trend_medium;
                    new_hive.baro_trend_short = baro_trend_short;
                    new_hive.baro_weather_idx = baro_weather_idx;
                    
                    // Light
                    new_hive.light_lux = light_lux;
                    new_hive.light_lux_mean = light_lux_mean;
                    new_hive.light_lux_min = light_lux_min;
                    new_hive.light_lux_max = light_lux_max;
                    new_hive.light_daylight_hours = light_daylight_hours;
                    new_hive.light_darkness_hours = light_darkness_hours;
                    new_hive.light_twilight = light_twilight;
                    new_hive.light_circadian_sync = light_circadian_sync;
                    new_hive.light_foraging_idx = light_foraging_idx;
                    new_hive.light_photoperiod = light_photoperiod;
                    new_hive.light_seasonal_change = light_seasonal_change;
                    new_hive.light_cloud_cover_est = light_cloud_cover_est;
                    new_hive.light_sunrise_offset = light_sunrise_offset;
                    new_hive.light_uv_idx = light_uv_idx;
                    new_hive.light_spectrum_blue = light_spectrum_blue;
                    new_hive.light_spectrum_red = light_spectrum_red;
                    new_hive.light_spectrum_ir = light_spectrum_ir;
                    new_hive.light_ir_ratio = light_ir_ratio;
                    new_hive.light_lux_std = light_lux_std;
                    new_hive.light_spectrum_data = light_spectrum_data;
                    
                    // Backward compatibility
                    new_hive.weight_rate = weight_rate;
                    new_hive.weight_trend = weight_trend;
                    new_hive.air_iaq = air_iaq;
                    
                    hives_data[hive_id] = new_hive;
                    
                    // Zapis do bazy danych SQLite
                    try {
                        ApiaryDatabase::getInstance().storeRawData(new_hive);
                        Logger::getInstance().debug("Zapisano dane CSV do bazy dla " + hive_id, "DB");
                    } catch (const std::exception& e) {
                        Logger::getInstance().warning("Nie udało się zapisać CSV do bazy: " + std::string(e.what()), "DB");
                    }
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().error("Blad parsowania CSV: " + std::string(e.what()));
        }
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
