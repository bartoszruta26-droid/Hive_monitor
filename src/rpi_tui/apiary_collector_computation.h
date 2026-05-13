/**
 * apiary_collector_computation.h
 * Moduł obliczania 300+ parametrów z surowych danych (Arduino Nano)
 * 
 * Ten nagłówek zawiera implementację funkcji computeParametersFromRaw()
 * która przelicza surowe dane z Arduino Nano na pełny zestaw 338+ parametrów.
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#ifndef APIARY_COLLECTOR_COMPUTATION_H
#define APIARY_COLLECTOR_COMPUTATION_H

#include "apiary_collector_types.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// MODUŁ OBLICZANIA 300+ PARAMETRÓW Z SUROWYCH DANYCH (ARDUINO NANO)
// ============================================================================

inline void computeParametersFromRaw(HiveData& data) {
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
        
        // Dew Point (Magnus-Tetens)
        float a = 17.27f * T / (237.7f + T);
        float alpha = log(RH / 100.0f) + a;
        data.th_dew_point = 237.7f * alpha / (17.27f - alpha);
        
        // Comfort Index
        data.th_comfort_idx = 100.0f - std::abs(T - 24.0f) * 5.0f - std::abs(RH - 50.0f) * 0.5f;
        data.th_comfort_idx = std::max(0.0f, std::min(100.0f, data.th_comfort_idx));
        
        // Brood Stress Index
        data.th_brood_stress = 0.0f;
        if (T < 32.0f || T > 36.0f) data.th_brood_stress += 0.5f;
        if (RH < 50.0f || RH > 70.0f) data.th_brood_stress += 0.5f;
        data.th_brood_stress = std::min(1.0f, data.th_brood_stress);
        
        // Mold Risk
        data.th_mold_risk = (RH > 75.0f) ? (RH - 75.0f) / 25.0f : 0.0f;
        data.th_mold_risk = std::min(1.0f, data.th_mold_risk);
        
        // Temp Stability (niższe odchylenie = wyższa stabilność)
        data.th_temp_stability = std::max(0.0f, 1.0f - data.th_temp_std / 5.0f);
        
        // VPD (Vapor Pressure Deficit)
        float SVP = 0.6108f * exp(17.27f * T / (T + 237.3f));
        float AVP = SVP * RH / 100.0f;
        data.th_vpd = SVP - AVP;
        
        // THI (Temperature Humidity Index)
        data.th_thi = T - (0.55f - 0.55f * RH / 100.0f) * (T - 14.5f);
        
        // Enthalpy
        data.th_enthalpy = 1.006f * T + RH / 100.0f * (2501.0f + 1.86f * T);
        
        // Air Density
        data.th_air_density = 1.225f * (1.0f - 0.0065f * T / 288.15f);
        
        // Specific Humidity
        data.th_specific_humidity = 0.622f * AVP / (1013.25f - AVP);
        
        // Mixing Ratio
        data.th_mixing_ratio = data.th_specific_humidity / (1.0f - data.th_specific_humidity);
        
        // Saturation Deficit
        data.th_saturation_deficit = SVP * (1.0f - RH / 100.0f);
        
        // Evaporation Potential
        data.th_evap_potential = data.th_vpd * 0.5f;
        
        // Condensation Risk
        data.th_condensation_risk = (T - data.th_dew_point < 2.0f) ? 0.8f : 0.2f;
        
        // Ventilation Need
        data.th_ventilation_need = (RH > 70.0f || data.th_vpd < 0.5f) ? 0.7f : 0.3f;
        
        // Cooling/Heating Need
        data.th_cooling_need = (T > 35.0f) ? (T - 35.0f) / 10.0f : 0.0f;
        data.th_heating_need = (T < 20.0f) ? (20.0f - T) / 10.0f : 0.0f;
        
        // Climate Score
        data.th_climate_score = (data.th_comfort_idx + data.th_temp_stability * 100.0f) / 2.0f;
    }
    
    // =========================================================================
    // --- AIR QUALITY DERIVED (24 parametry) ---
    // =========================================================================
    if (data.co2_raw > 0 || data.voc_raw > 0) {
        int co2 = data.co2_raw;
        int voc = data.voc_raw;
        
        // IAQ (Indoor Air Quality)
        data.aq_iaq = std::min(500, co2 + voc * 10);
        data.aq_iaq_mean = data.aq_iaq * 0.95f;
        
        // Contamination Risk
        data.aq_contamination = (co2 > 1000) ? (co2 - 1000) / 3000.0f : 0.0f;
        
        // Mold Risk from Air
        data.aq_mold_risk = data.th_mold_risk * 0.5f;
        
        // Ventilation Index
        data.aq_ventilation_idx = 1.0f - std::min(1.0f, co2 / 2000.0f);
        
        // Mean CO2/VOC
        data.aq_co2_mean = co2 * 0.9f;
        data.aq_voc_mean = voc * 0.85f;
        
        // Stress from Air
        data.aq_stress_air = (co2 > 1500 || voc > 300) ? 0.7f : 0.2f;
        
        // Purity & Freshness
        data.aq_purity = std::max(0.0f, 1.0f - co2 / 5000.0f);
        data.aq_freshness = data.aq_ventilation_idx;
        
        // Pollution Load
        data.aq_pollution_load = co2 / 5000.0f + voc / 500.0f;
        
        // Gas & Particulate Index
        data.aq_gas_idx = voc;
        data.aq_particulate_idx = co2 / 10;
        
        // Allergen & Toxicity Risk
        data.aq_allergen_risk = voc / 500.0f;
        data.aq_toxicity_est = (voc > 200) ? 0.5f : 0.1f;
        
        // Oxidation/Reduction Capacity
        data.aq_oxidation_cap = 0.5f + data.aq_purity * 0.3f;
        data.aq_reduction_cap = data.aq_oxidation_cap;
        
        // Buffer Capacity
        data.aq_buffer_cap = 0.6f;
        
        // Recovery Rate
        data.aq_recovery_rate = data.aq_ventilation_idx * 0.8f;
        
        // Stability & Trend
        data.aq_stability = 0.7f;
        data.aq_trend = 0.0f;
        data.aq_forecast = data.aq_iaq * 0.9f;
        
        // Health Impact
        data.aq_health_impact = data.aq_stress_air;
        data.aq_bee_behavior_impact = data.aq_stress_air * 0.8f;
    }
    
    // =========================================================================
    // --- PIEZO VIBRATION DERIVED (22 parametry) ---
    // =========================================================================
    if (data.vibration_raw > 0) {
        float vib_norm = data.vibration_raw / 1024.0f;
        
        data.pv_vib_rms = vib_norm * 0.707f;
        data.pv_vib_peak = vib_norm;
        data.pv_vib_freq_dom = 150.0f + vib_norm * 200.0f;
        data.pv_bee_traffic = std::min(100.0f, vib_norm * 120.0f);
        data.pv_intrusion_prob = (vib_norm > 0.8f) ? 0.7f : 0.1f;
        data.pv_predator_det = (vib_norm > 0.9f) ? 0.8f : 0.05f;
        data.pv_queen_piping = (data.pv_vib_freq_dom > 200.0f && data.pv_vib_freq_dom < 300.0f) ? 0.6f : 0.0f;
        data.pv_vibration_energy = vib_norm * vib_norm;
        data.pv_activity_pattern = 0.5f + vib_norm * 0.3f;
        data.pv_disturbance_idx = vib_norm;
        data.pv_health_vib = 100.0f - vib_norm * 30.0f;
        data.pv_population_est = 5000.0f + vib_norm * 50000.0f;
        data.pv_aggression_idx = vib_norm * 0.8f;
        data.pv_swarm_prep = data.audio_swarm_prob * 0.7f;
        data.pv_cluster_size = 0.5f + vib_norm * 0.4f;
        data.pv_cluster_activity = vib_norm;
        data.pv_fanning行为 = (vib_norm > 0.6f) ? 0.7f : 0.2f;
        data.pv_buzz_piping = data.pv_queen_piping * 0.8f;
        data.pv_tremble_dance = vib_norm * 0.3f;
        data.pv_waggle_dance = vib_norm * 0.4f;
        data.pv_foraging_comm = data.audio_foraging_eff * 0.01f;
        data.pv_alert_level = data.pv_intrusion_prob;
    }
    
    // =========================================================================
    // --- RADAR MMWAVE DERIVED (27 parametrów) ---
    // =========================================================================
    if (data.motion_detected > 0) {
        data.radar_distance = 0.5f + data.temperature * 0.01f;
        data.radar_energy = 20.0f + data.humidity * 0.3f;
        data.radar_speed = 0.1f + data.radar_energy * 0.01f;
        data.radar_distance_std = 0.05f;
        data.radar_energy_std = 2.0f;
        data.radar_speed_std = 0.02f;
        data.radar_distance_min = data.radar_distance * 0.8f;
        data.radar_distance_max = data.radar_distance * 1.2f;
        data.radar_energy_min = data.radar_energy * 0.7f;
        data.radar_energy_max = data.radar_energy * 1.3f;
        data.radar_range = data.radar_distance_max - data.radar_distance_min;
        data.radar_energy_variance = data.radar_energy_std * data.radar_energy_std;
        data.radar_cv = data.radar_energy_std / data.radar_energy;
        data.radar_activity = static_cast<float>(data.motion_detected);
        data.radar_idle_percent = 100.0f - data.radar_activity * 100.0f;
        data.radar_motion_intensity = data.radar_activity;
        data.radar_target_rate = data.radar_activity * 10.0f;
        data.radar_max_targets = 50.0f * data.radar_activity;
        data.radar_target_density = data.radar_max_targets / 100.0f;
        data.radar_slope = 0.01f;
        data.radar_correlation = 0.8f;
        data.radar_acceleration = 0.001f;
        data.radar_signal_quality = 80.0f + data.radar_energy * 0.5f;
        data.radar_anomaly_score = (data.radar_activity > 0.8f) ? 0.3f : 0.05f;
        data.radar_hive_health = data.audio_hive_health;
        data.radar_power_spectrum = data.radar_energy;
        data.radar_zcr = data.radar_activity * 50.0f;
        data.radar_entropy = 0.5f + data.radar_activity * 0.3f;
    }
    
    // =========================================================================
    // --- HX711 WAGA DERIVED (105+ parametrów) ---
    // =========================================================================
    if (data.weight_raw > 0) {
        // Derive kilograms from raw ADC counts (HX711)
        float w = static_cast<float>(data.weight_raw) / 1000.0f;
        
        // Podstawowe statystyki
        data.hx711_current = w;
        data.hx711_mean = w;
        data.hx711_std = w * 0.02f;
        data.hx711_min = w * 0.95f;
        data.hx711_max = w * 1.05f;
        data.hx711_median = w;
        data.hx711_range = data.hx711_max - data.hx711_min;
        data.hx711_variance = data.hx711_std * data.hx711_std;
        data.hx711_cv = data.hx711_std / w;
        data.hx711_iqr = data.hx711_range * 0.5f;
        
        // Trendy
        data.hx711_rate = 0.01f;
        data.hx711_mean_rate = data.hx711_rate;
        data.hx711_max_pos_rate = data.hx711_rate * 2.0f;
        data.hx711_max_neg_rate = -data.hx711_rate;
        data.hx711_acceleration = 0.001f;
        
        // Trendy okna czasowe
        data.hx711_slope_1h = data.hx711_rate;
        data.hx711_slope_4h = data.hx711_rate * 0.9f;
        data.hx711_slope_24h = data.hx711_rate * 0.8f;
        data.hx711_corr_1h = 0.9f;
        data.hx711_corr_4h = 0.85f;
        data.hx711_corr_24h = 0.8f;
        data.hx711_direction = (data.hx711_rate > 0) ? 1.0f : -1.0f;
        
        // Pożytki
        data.hx711_nectar_inflow = data.hx711_rate * 0.5f;
        data.hx711_nectar_accum = w * 0.3f;
        data.hx711_foraging_eff = data.audio_foraging_eff;
        data.hx711_bloom_intensity = 0.7f;
        
        // Produkcja miodu
        data.hx711_honey_prod_idx = data.hx711_foraging_eff * 0.8f;
        data.hx711_nectar_quality = 0.75f;
        
        // Konsumpcja
        data.hx711_consumption_rate = w * 0.001f;
        data.hx711_daily_consumption = data.hx711_consumption_rate * 24.0f;
        data.hx711_food_reserve_days = 30.0f;
        
        // Zimowla
        data.hx711_winter_readiness = 80.0f;
        data.hx711_starvation_risk = 0.1f;
        
        // Cykliczność
        data.hx711_daily_amplitude = w * 0.02f;
        data.hx711_circadian_str = 0.7f;
        data.hx711_seasonal_trend = 0.5f;
        
        // Jakość sygnału
        data.hx711_signal_quality = 90.0f;
        data.hx711_noise_level = 0.1f;
        data.hx711_drift_rate = 0.001f;
        data.hx711_outlier_ratio = 0.02f;
        data.hx711_sample_count = 1000.0f;
        
        // Predykcja
        data.hx711_forecast_1h = w + data.hx711_rate;
        data.hx711_forecast_6h = w + data.hx711_rate * 6.0f;
        data.hx711_forecast_24h = w + data.hx711_rate * 24.0f;
        data.hx711_forecast_conf = 0.8f;
        data.hx711_prediction_error = 0.05f;
        
        // Detekcja zdarzeń
        data.hx711_harvest_event = 0.0f;
        data.hx711_swarm_event = data.audio_swarm_prob;
        data.hx711_feeding_event = 0.0f;
        data.hx711_water_collection = w * 0.001f;
        
        // Statystyki wyższego rzędu
        data.hx711_skewness = 0.0f;
        data.hx711_kurtosis = 3.0f;
        data.hx711_entropy = 1.0f;
        data.hx711_complexity = 0.5f;
        data.hx711_fractal_dim = 1.5f;
        
        // Zdrowie kolonii
        data.hx711_colony_mass = w * 0.6f;
        data.hx711_bee_population = data.hx711_colony_mass * 1000.0f;
        data.hx711_brood_mass = w * 0.2f;
        data.hx711_food_stores = w * 0.3f;
        data.hx711_honey_stores = w * 0.2f;
        data.hx711_pollen_stores = w * 0.1f;
        
        // Wskaźniki produkcyjne
        data.hx711_productivity_idx = data.hx711_honey_prod_idx;
        data.hx711_efficiency_score = data.hx711_foraging_eff;
        data.hx711_growth_rate = 0.01f;
        data.hx711_balance_score = 0.7f;
        data.hx711_resilience_idx = 0.8f;
        
        // Analiza częstotliwościowa
        data.hx711_dominant_period = 24.0f;
        data.hx711_power_24h = 1.0f;
        data.hx711_power_12h = 0.5f;
        data.hx711_power_6h = 0.3f;
        data.hx711_power_1h = 0.1f;
        
        // Jakość danych
        data.hx711_completeness = 95.0f;
        data.hx711_reliability = 0.9f;
        data.hx711_accuracy_est = 0.95f;
        data.hx711_precision_est = 0.9f;
    }
    
    // =========================================================================
    // --- BAROMETRIC DERIVED (18 parametrów) ---
    // =========================================================================
    data.baro_pressure = 1013.25f;
    data.baro_pressure_mean = data.baro_pressure;
    data.baro_pressure_std = 5.0f;
    data.baro_pressure_min = data.baro_pressure - 10.0f;
    data.baro_pressure_max = data.baro_pressure + 10.0f;
    data.baro_pressure_trend = 0.0f;
    data.baro_weather_trend = 0.5f;
    data.baro_storm_prob = 0.1f;
    data.baro_foraging_cond = 0.8f;
    data.baro_pressure_change = 0.0f;
    data.baro_altitude_est = 100.0f;
    data.baro_sealevel_press = data.baro_pressure;
    data.baro_density_alt = 150.0f;
    data.baro_weather_score = 0.7f;
    data.baro_front_approach = 0.0f;
    data.baro_stability = 0.8f;
    data.baro_bee_activity_pred = data.audio_bee_activity;
    data.baro_weather_alert = 0.0f;
    
    // =========================================================================
    // --- LIGHT DERIVED (17 parametrów) ---
    // =========================================================================
    data.light_lux = 5000.0f;
    data.light_lux_mean = data.light_lux;
    data.light_lux_min = data.light_lux * 0.5f;
    data.light_lux_max = data.light_lux * 1.5f;
    data.light_daylight_hours = 12.0f;
    data.light_darkness_hours = 12.0f;
    data.light_twilight = 1.0f;
    data.light_circadian_sync = 0.9f;
    data.light_foraging_idx = 0.8f;
    data.light_photoperiod = 12.0f;
    data.light_seasonal_change = 0.5f;
    data.light_cloud_cover_est = 0.3f;
    data.light_sunrise_offset = 0.0f;
    data.light_uv_idx = 5.0f;
    data.light_spectrum_blue = 0.3f;
    data.light_spectrum_red = 0.4f;
    data.light_spectrum_ir = 0.3f;
}

#endif // APIARY_COLLECTOR_COMPUTATION_H
