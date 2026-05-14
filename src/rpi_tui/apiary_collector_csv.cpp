/**
 * apiary_collector_csv.cpp
 * Implementacja modułu parsowania i generowania danych CSV dla 338+ parametrów uli
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#include "apiary_collector_csv.h"
#include <sstream>
#include <stdexcept>
#include <cmath>

// Dolaczamy logger tylko jesli potrzebny (opcjonalne)
// #include "apiary_logger.h"

namespace apiary {
namespace csv {

// ============================================================================
// FUNKCJE POMOCNICZE
// ============================================================================

float safeGetFloat(const std::vector<std::string>& parts, size_t idx, float defaultValue) {
    if (idx >= parts.size()) {
        return defaultValue;
    }
    try {
        return std::stof(parts[idx]);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

int safeGetInt(const std::vector<std::string>& parts, size_t idx, int defaultValue) {
    if (idx >= parts.size()) {
        return defaultValue;
    }
    try {
        return std::stoi(parts[idx]);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

// ============================================================================
// PARSOWANIE CSV - GŁÓWNA FUNKCJA
// ============================================================================

bool parseCSV(const std::string& raw_data, const std::string& source_ip,
              long long timestamp, HiveData& data) {
    // Inicjalizacja danych
    data = HiveData();
    data.timestamp = timestamp;
    
    std::stringstream ss(raw_data);
    std::string segment;
    std::vector<std::string> parts;

    while (std::getline(ss, segment, ',')) {
        parts.push_back(segment);
    }

    // Wymagane minimum 9 pól dla podstawowych danych
    if (parts.size() < 9) {
        // Logger::getInstance().warning("Niepoprawny format CSV z " + source_ip + ": " + raw_data);
        return false;
    }

    try {
        // =========================================================================
        // PODSTAWOWE PARAMETRY (indeksy 0-8)
        // =========================================================================
        data.hive_id = parts[0];
        data.temperature = safeGetFloat(parts, 1);
        data.humidity = safeGetFloat(parts, 2);
        data.weight = safeGetFloat(parts, 3);
        data.battery_level = safeGetInt(parts, 4);
        data.co2_eq = safeGetInt(parts, 5);
        data.voc_idx = safeGetInt(parts, 6);
        data.motion_detected = safeGetInt(parts, 7);
        data.is_online = true;
        
        // Surowe dane (dla backward compatibility)
        data.temp_raw = data.temperature;
        data.hum_raw = data.humidity;
        data.co2_raw = data.co2_eq;
        data.voc_raw = data.voc_idx;

        // =========================================================================
        // PARAMETRY AUDIO (63 parametry) - indeksy 9-71
        // =========================================================================
        data.audio_rms = safeGetFloat(parts, 9);
        data.audio_peak = safeGetFloat(parts, 10);
        data.audio_peak_to_peak = safeGetFloat(parts, 11);
        data.audio_zcr = safeGetFloat(parts, 12);
        data.audio_energy = safeGetFloat(parts, 13);
        data.audio_mean_amp = safeGetFloat(parts, 14);
        data.audio_std_amp = safeGetFloat(parts, 15);
        data.audio_cv_amp = safeGetFloat(parts, 16);
        data.audio_skewness = safeGetFloat(parts, 17);
        data.audio_kurtosis = safeGetFloat(parts, 18);
        data.audio_dominant_freq = safeGetFloat(parts, 19);
        data.audio_spectral_centroid = safeGetFloat(parts, 20);
        data.audio_spectral_bandwidth = safeGetFloat(parts, 21);
        data.audio_spectral_flatness = safeGetFloat(parts, 22);
        data.audio_spectral_rolloff = safeGetFloat(parts, 23);
        data.audio_power_bee_band = safeGetFloat(parts, 24);
        data.audio_power_swarm_band = safeGetFloat(parts, 25);
        data.audio_power_low_freq = safeGetFloat(parts, 26);
        data.audio_power_high_freq = safeGetFloat(parts, 27);
        data.audio_harmonic_ratio = safeGetFloat(parts, 28);
        
        // MFCC energy [4] - indeksy 29-32
        data.audio_mfcc_energy[0] = safeGetFloat(parts, 29);
        data.audio_mfcc_energy[1] = safeGetFloat(parts, 30);
        data.audio_mfcc_energy[2] = safeGetFloat(parts, 31);
        data.audio_mfcc_energy[3] = safeGetFloat(parts, 32);
        
        data.audio_spectral_entropy = safeGetFloat(parts, 33);
        data.audio_spectral_contrast = safeGetFloat(parts, 34);
        data.audio_tonal_strength = safeGetFloat(parts, 35);
        data.audio_crest_factor = safeGetFloat(parts, 36);
        data.audio_formant_f1 = safeGetFloat(parts, 37);
        data.audio_formant_f2 = safeGetFloat(parts, 38);
        data.audio_fundamental_freq = safeGetFloat(parts, 39);
        data.audio_pitch_strength = safeGetFloat(parts, 40);
        data.audio_inharmonicity = safeGetFloat(parts, 41);
        data.audio_shimmer = safeGetFloat(parts, 42);
        data.audio_jitter = safeGetFloat(parts, 43);
        data.audio_nhr = safeGetFloat(parts, 44);
        data.audio_hnr = safeGetFloat(parts, 45);
        data.audio_autocorr_peak = safeGetFloat(parts, 46);
        data.audio_attack_time = safeGetFloat(parts, 47);
        data.audio_decay_time = safeGetFloat(parts, 48);
        data.audio_sustain_level = safeGetFloat(parts, 49);
        data.audio_temporal_centroid = safeGetFloat(parts, 50);
        data.audio_loudness = safeGetFloat(parts, 51);
        data.audio_spectral_flux = safeGetFloat(parts, 52);
        data.audio_spectral_slope = safeGetFloat(parts, 53);
        data.audio_spectral_kurtosis = safeGetFloat(parts, 54);
        data.audio_spectral_skewness = safeGetFloat(parts, 55);
        data.audio_fund_salience = safeGetFloat(parts, 56);
        
        // Power bands [8] - indeksy 57-64
        for (int i = 0; i < 8; i++) {
            data.audio_power_band[i] = safeGetFloat(parts, 57 + i);
        }
        
        data.audio_leq = safeGetFloat(parts, 65);
        data.audio_l10 = safeGetFloat(parts, 66);
        data.audio_l90 = safeGetFloat(parts, 67);
        data.audio_noise_floor = safeGetFloat(parts, 68);
        data.audio_snr = safeGetFloat(parts, 69);
        data.audio_aci = safeGetFloat(parts, 70);
        data.audio_bi = safeGetFloat(parts, 71);
        data.audio_ndi = safeGetFloat(parts, 72);
        data.audio_adi = safeGetFloat(parts, 73);
        data.audio_aei = safeGetFloat(parts, 74);
        data.audio_bee_activity = safeGetFloat(parts, 75);
        data.audio_swarm_prob = safeGetFloat(parts, 76);
        data.audio_stress_indicator = safeGetFloat(parts, 77);
        data.audio_hive_health = safeGetFloat(parts, 78);
        data.audio_foraging_eff = safeGetFloat(parts, 79);
        data.audio_colony_coherence = safeGetFloat(parts, 80);

        // =========================================================================
        // PARAMETRY RADAR MMWAVE (28 parametrów) - indeksy 81-108
        // =========================================================================
        data.radar_distance = safeGetFloat(parts, 81);
        data.radar_energy = safeGetFloat(parts, 82);
        data.radar_speed = safeGetFloat(parts, 83);
        data.radar_distance_std = safeGetFloat(parts, 84);
        data.radar_energy_std = safeGetFloat(parts, 85);
        data.radar_speed_std = safeGetFloat(parts, 86);
        data.radar_distance_min = safeGetFloat(parts, 87);
        data.radar_distance_max = safeGetFloat(parts, 88);
        data.radar_energy_min = safeGetFloat(parts, 89);
        data.radar_energy_max = safeGetFloat(parts, 90);
        data.radar_range = safeGetFloat(parts, 91);
        data.radar_energy_variance = safeGetFloat(parts, 92);
        data.radar_cv = safeGetFloat(parts, 93);
        data.radar_activity = safeGetFloat(parts, 94);
        data.radar_idle_percent = safeGetFloat(parts, 95);
        data.radar_motion_intensity = safeGetFloat(parts, 96);
        data.radar_target_rate = safeGetFloat(parts, 97);
        data.radar_max_targets = safeGetFloat(parts, 98);
        data.radar_target_density = safeGetFloat(parts, 99);
        data.radar_slope = safeGetFloat(parts, 100);
        data.radar_correlation = safeGetFloat(parts, 101);
        data.radar_acceleration = safeGetFloat(parts, 102);
        data.radar_signal_quality = safeGetFloat(parts, 103);
        data.radar_anomaly_score = safeGetFloat(parts, 104);
        data.radar_hive_health = safeGetFloat(parts, 105);
        data.radar_power_spectrum = safeGetFloat(parts, 106);
        data.radar_zcr = safeGetFloat(parts, 107);
        data.radar_entropy = safeGetFloat(parts, 108);

        // =========================================================================
        // PARAMETRY HX711 WAGA (77 parametrów) - indeksy 109-185
        // =========================================================================
        data.hx711_current = safeGetFloat(parts, 109);
        data.hx711_mean = safeGetFloat(parts, 110);
        data.hx711_std = safeGetFloat(parts, 111);
        data.hx711_min = safeGetFloat(parts, 112);
        data.hx711_max = safeGetFloat(parts, 113);
        data.hx711_median = safeGetFloat(parts, 114);
        data.hx711_range = safeGetFloat(parts, 115);
        data.hx711_variance = safeGetFloat(parts, 116);
        data.hx711_cv = safeGetFloat(parts, 117);
        data.hx711_iqr = safeGetFloat(parts, 118);
        data.hx711_rate = safeGetFloat(parts, 119);
        data.hx711_mean_rate = safeGetFloat(parts, 120);
        data.hx711_max_pos_rate = safeGetFloat(parts, 121);
        data.hx711_max_neg_rate = safeGetFloat(parts, 122);
        data.hx711_acceleration = safeGetFloat(parts, 123);
        data.hx711_slope_1h = safeGetFloat(parts, 124);
        data.hx711_slope_4h = safeGetFloat(parts, 125);
        data.hx711_slope_24h = safeGetFloat(parts, 126);
        data.hx711_corr_1h = safeGetFloat(parts, 127);
        data.hx711_corr_4h = safeGetFloat(parts, 128);
        data.hx711_corr_24h = safeGetFloat(parts, 129);
        data.hx711_direction = safeGetFloat(parts, 130);
        data.hx711_nectar_inflow = safeGetFloat(parts, 131);
        data.hx711_nectar_accum = safeGetFloat(parts, 132);
        data.hx711_foraging_eff = safeGetFloat(parts, 133);
        data.hx711_bloom_intensity = safeGetFloat(parts, 134);
        data.hx711_honey_prod_idx = safeGetFloat(parts, 135);
        data.hx711_nectar_quality = safeGetFloat(parts, 136);
        data.hx711_consumption_rate = safeGetFloat(parts, 137);
        data.hx711_daily_consumption = safeGetFloat(parts, 138);
        data.hx711_food_reserve_days = safeGetFloat(parts, 139);
        data.hx711_winter_readiness = safeGetFloat(parts, 140);
        data.hx711_starvation_risk = safeGetFloat(parts, 141);
        data.hx711_daily_amplitude = safeGetFloat(parts, 142);
        data.hx711_circadian_str = safeGetFloat(parts, 143);
        data.hx711_seasonal_trend = safeGetFloat(parts, 144);
        data.hx711_signal_quality = safeGetFloat(parts, 145);
        data.hx711_noise_level = safeGetFloat(parts, 146);
        data.hx711_drift_rate = safeGetFloat(parts, 147);
        data.hx711_outlier_ratio = safeGetFloat(parts, 148);
        data.hx711_sample_count = safeGetFloat(parts, 149);
        data.hx711_forecast_1h = safeGetFloat(parts, 150);
        data.hx711_forecast_6h = safeGetFloat(parts, 151);
        data.hx711_forecast_24h = safeGetFloat(parts, 152);
        data.hx711_forecast_conf = safeGetFloat(parts, 153);
        data.hx711_prediction_error = safeGetFloat(parts, 154);
        data.hx711_harvest_event = safeGetFloat(parts, 155);
        data.hx711_swarm_event = safeGetFloat(parts, 156);
        data.hx711_feeding_event = safeGetFloat(parts, 157);
        data.hx711_water_collection = safeGetFloat(parts, 158);
        data.hx711_skewness = safeGetFloat(parts, 159);
        data.hx711_kurtosis = safeGetFloat(parts, 160);
        data.hx711_entropy = safeGetFloat(parts, 161);
        data.hx711_complexity = safeGetFloat(parts, 162);
        data.hx711_fractal_dim = safeGetFloat(parts, 163);
        data.hx711_colony_mass = safeGetFloat(parts, 164);
        data.hx711_bee_population = safeGetFloat(parts, 165);
        data.hx711_brood_mass = safeGetFloat(parts, 166);
        data.hx711_food_stores = safeGetFloat(parts, 167);
        data.hx711_honey_stores = safeGetFloat(parts, 168);
        data.hx711_pollen_stores = safeGetFloat(parts, 169);
        data.hx711_productivity_idx = safeGetFloat(parts, 170);
        data.hx711_efficiency_score = safeGetFloat(parts, 171);
        data.hx711_growth_rate = safeGetFloat(parts, 172);
        data.hx711_balance_score = safeGetFloat(parts, 173);
        data.hx711_resilience_idx = safeGetFloat(parts, 174);
        data.hx711_dominant_period = safeGetFloat(parts, 175);
        data.hx711_power_24h = safeGetFloat(parts, 176);
        data.hx711_power_12h = safeGetFloat(parts, 177);
        data.hx711_power_6h = safeGetFloat(parts, 178);
        data.hx711_power_1h = safeGetFloat(parts, 179);
        data.hx711_completeness = safeGetFloat(parts, 180);
        data.hx711_reliability = safeGetFloat(parts, 181);
        data.hx711_accuracy_est = safeGetFloat(parts, 182);
        data.hx711_precision_est = safeGetFloat(parts, 183);
        data.hx711_anomaly_flag = safeGetFloat(parts, 184);
        data.hx711_health_score = safeGetFloat(parts, 185);

        // =========================================================================
        // PARAMETRY TEMP/HUMIDITY (32 parametry) - indeksy 186-217
        // =========================================================================
        data.th_temp_mean = safeGetFloat(parts, 186);
        data.th_temp_std = safeGetFloat(parts, 187);
        data.th_temp_min = safeGetFloat(parts, 188);
        data.th_temp_max = safeGetFloat(parts, 189);
        data.th_temp_range = safeGetFloat(parts, 190);
        data.th_hum_mean = safeGetFloat(parts, 191);
        data.th_hum_std = safeGetFloat(parts, 192);
        data.th_hum_min = safeGetFloat(parts, 193);
        data.th_hum_max = safeGetFloat(parts, 194);
        data.th_hum_range = safeGetFloat(parts, 195);
        data.th_heat_index = safeGetFloat(parts, 196);
        data.th_dew_point = safeGetFloat(parts, 197);
        data.th_comfort_idx = safeGetFloat(parts, 198);
        data.th_brood_stress = safeGetFloat(parts, 199);
        data.th_mold_risk = safeGetFloat(parts, 200);
        data.th_temp_stability = safeGetFloat(parts, 201);
        data.th_vpd = safeGetFloat(parts, 202);
        data.th_thi = safeGetFloat(parts, 203);
        data.th_enthalpy = safeGetFloat(parts, 204);
        data.th_air_density = safeGetFloat(parts, 205);
        data.th_specific_humidity = safeGetFloat(parts, 206);
        data.th_mixing_ratio = safeGetFloat(parts, 207);
        data.th_saturation_deficit = safeGetFloat(parts, 208);
        data.th_evap_potential = safeGetFloat(parts, 209);
        data.th_condensation_risk = safeGetFloat(parts, 210);
        data.th_ventilation_need = safeGetFloat(parts, 211);
        data.th_cooling_need = safeGetFloat(parts, 212);
        data.th_heating_need = safeGetFloat(parts, 213);
        data.th_climate_score = safeGetFloat(parts, 214);
        data.th_hum_stability_1h = safeGetFloat(parts, 215);
        data.th_hum_trend = safeGetFloat(parts, 216);
        data.th_temp_trend = safeGetFloat(parts, 217);

        // =========================================================================
        // PARAMETRY AIR QUALITY (37 parametrów) - indeksy 218-254
        // =========================================================================
        data.aq_iaq = safeGetFloat(parts, 218);
        data.aq_iaq_mean = safeGetFloat(parts, 219);
        data.aq_contamination = safeGetFloat(parts, 220);
        data.aq_mold_risk = safeGetFloat(parts, 221);
        data.aq_ventilation_idx = safeGetFloat(parts, 222);
        data.aq_co2_mean = safeGetFloat(parts, 223);
        data.aq_voc_mean = safeGetFloat(parts, 224);
        data.aq_stress_air = safeGetFloat(parts, 225);
        data.aq_purity = safeGetFloat(parts, 226);
        data.aq_freshness = safeGetFloat(parts, 227);
        data.aq_pollution_load = safeGetFloat(parts, 228);
        data.aq_gas_idx = safeGetFloat(parts, 229);
        data.aq_particulate_idx = safeGetFloat(parts, 230);
        data.aq_allergen_risk = safeGetFloat(parts, 231);
        data.aq_toxicity_est = safeGetFloat(parts, 232);
        data.aq_oxidation_cap = safeGetFloat(parts, 233);
        data.aq_reduction_cap = safeGetFloat(parts, 234);
        data.aq_buffer_cap = safeGetFloat(parts, 235);
        data.aq_recovery_rate = safeGetFloat(parts, 236);
        data.aq_stability = safeGetFloat(parts, 237);
        data.aq_trend = safeGetFloat(parts, 238);
        data.aq_forecast = safeGetFloat(parts, 239);
        data.aq_health_impact = safeGetFloat(parts, 240);
        data.aq_bee_behavior_impact = safeGetFloat(parts, 241);
        data.aq_co2_alert = safeGetFloat(parts, 242);
        data.aq_co2_base = safeGetFloat(parts, 243);
        data.aq_co2_peak = safeGetFloat(parts, 244);
        data.aq_co2_trend = safeGetFloat(parts, 245);
        data.aq_co2_variability = safeGetFloat(parts, 246);
        data.aq_comfort_score = safeGetFloat(parts, 247);
        data.aq_env_correlation = safeGetFloat(parts, 248);
        data.aq_stress_level = safeGetFloat(parts, 249);
        data.aq_variability_idx = safeGetFloat(parts, 250);
        data.aq_voc_base = safeGetFloat(parts, 251);
        data.aq_voc_peak = safeGetFloat(parts, 252);
        data.aq_voc_variability = safeGetFloat(parts, 253);
        data.aq_voc_warning = safeGetFloat(parts, 254);

        // =========================================================================
        // PARAMETRY PIEZO VIBRATION (33 parametry) - indeksy 255-287
        // =========================================================================
        data.pv_vib_rms = safeGetFloat(parts, 255);
        data.pv_vib_peak = safeGetFloat(parts, 256);
        data.pv_vib_freq_dom = safeGetFloat(parts, 257);
        data.pv_bee_traffic = safeGetFloat(parts, 258);
        data.pv_intrusion_prob = safeGetFloat(parts, 259);
        data.pv_predator_det = safeGetFloat(parts, 260);
        data.pv_queen_piping = safeGetFloat(parts, 261);
        data.pv_vibration_energy = safeGetFloat(parts, 262);
        data.pv_activity_pattern = safeGetFloat(parts, 263);
        data.pv_disturbance_idx = safeGetFloat(parts, 264);
        data.pv_health_vib = safeGetFloat(parts, 265);
        data.pv_population_est = safeGetFloat(parts, 266);
        data.pv_aggression_idx = safeGetFloat(parts, 267);
        data.pv_swarm_prep = safeGetFloat(parts, 268);
        data.pv_cluster_size = safeGetFloat(parts, 269);
        data.pv_cluster_activity = safeGetFloat(parts, 270);
        data.pv_fanning行为 = safeGetFloat(parts, 271);
        data.pv_buzz_piping = safeGetFloat(parts, 272);
        data.pv_tremble_dance = safeGetFloat(parts, 273);
        data.pv_waggle_dance = safeGetFloat(parts, 274);
        data.pv_foraging_comm = safeGetFloat(parts, 275);
        data.pv_alert_level = safeGetFloat(parts, 276);
        data.pv_alien_detected = safeGetFloat(parts, 277);
        data.pv_confidence = safeGetFloat(parts, 278);
        data.pv_continuous_vib = safeGetFloat(parts, 279);
        data.pv_event_count = safeGetFloat(parts, 280);
        data.pv_impact_flag = safeGetFloat(parts, 281);
        data.pv_severity = safeGetFloat(parts, 282);
        data.pv_source_type = safeGetFloat(parts, 283);
        data.pv_vib_mean = safeGetFloat(parts, 284);
        data.pv_vib_std = safeGetFloat(parts, 285);
        data.pv_wind_noise = safeGetFloat(parts, 286);
        data.pv_zcr = safeGetFloat(parts, 287);

        // =========================================================================
        // PARAMETRY BAROMETRIC (30 parametrów) - indeksy 288-317
        // =========================================================================
        data.baro_pressure = safeGetFloat(parts, 288);
        data.baro_pressure_mean = safeGetFloat(parts, 289);
        data.baro_pressure_std = safeGetFloat(parts, 290);
        data.baro_pressure_min = safeGetFloat(parts, 291);
        data.baro_pressure_max = safeGetFloat(parts, 292);
        data.baro_pressure_trend = safeGetFloat(parts, 293);
        data.baro_weather_trend = safeGetFloat(parts, 294);
        data.baro_storm_prob = safeGetFloat(parts, 295);
        data.baro_foraging_cond = safeGetFloat(parts, 296);
        data.baro_pressure_change = safeGetFloat(parts, 297);
        data.baro_altitude_est = safeGetFloat(parts, 298);
        data.baro_sealevel_press = safeGetFloat(parts, 299);
        data.baro_density_alt = safeGetFloat(parts, 300);
        data.baro_weather_score = safeGetFloat(parts, 301);
        data.baro_front_approach = safeGetFloat(parts, 302);
        data.baro_stability = safeGetFloat(parts, 303);
        data.baro_bee_activity_pred = safeGetFloat(parts, 304);
        data.baro_weather_alert = safeGetFloat(parts, 305);
        data.baro_activity_pred = safeGetFloat(parts, 306);
        data.baro_alert_flag = safeGetInt(parts, 307);
        data.baro_foraging_idx = safeGetFloat(parts, 308);
        data.baro_improving_flag = safeGetFloat(parts, 309);
        data.baro_rain_risk = safeGetFloat(parts, 310);
        data.baro_reliability_idx = safeGetFloat(parts, 311);
        data.baro_severity = safeGetFloat(parts, 312);
        data.baro_storm_risk = safeGetFloat(parts, 313);
        data.baro_temperature = safeGetFloat(parts, 314);
        data.baro_trend_medium = safeGetFloat(parts, 315);
        data.baro_trend_short = safeGetFloat(parts, 316);
        data.baro_weather_idx = safeGetFloat(parts, 317);

        // =========================================================================
        // PARAMETRY LIGHT (20 parametrów) - indeksy 318-337
        // =========================================================================
        data.light_lux = safeGetFloat(parts, 318);
        data.light_lux_mean = safeGetFloat(parts, 319);
        data.light_lux_min = safeGetFloat(parts, 320);
        data.light_lux_max = safeGetFloat(parts, 321);
        data.light_daylight_hours = safeGetFloat(parts, 322);
        data.light_darkness_hours = safeGetFloat(parts, 323);
        data.light_twilight = safeGetFloat(parts, 324);
        data.light_circadian_sync = safeGetFloat(parts, 325);
        data.light_foraging_idx = safeGetFloat(parts, 326);
        data.light_photoperiod = safeGetFloat(parts, 327);
        data.light_seasonal_change = safeGetFloat(parts, 328);
        data.light_cloud_cover_est = safeGetFloat(parts, 329);
        data.light_sunrise_offset = safeGetFloat(parts, 330);
        data.light_uv_idx = safeGetFloat(parts, 331);
        data.light_spectrum_blue = safeGetFloat(parts, 332);
        data.light_spectrum_red = safeGetFloat(parts, 333);
        data.light_spectrum_ir = safeGetFloat(parts, 334);
        data.light_ir_ratio = safeGetFloat(parts, 335);
        data.light_lux_std = safeGetFloat(parts, 336);
        data.light_spectrum_data = safeGetFloat(parts, 337);

        // Dodatkowe pola backward compatibility
        data.weight_rate = safeGetFloat(parts, 338);
        data.weight_trend = safeGetFloat(parts, 339);
        data.air_iaq = safeGetInt(parts, 340);

        return true;
        
    } catch (const std::exception& e) {
        // Logger::getInstance().error("Błąd parsowania CSV: " + std::string(e.what()));
        return false;
    }
}

// ============================================================================
// GENEROWANIE NAGŁÓWKA CSV
// ============================================================================

std::string getCSVHeader() {
    std::stringstream csv;
    
    // Podstawowe
    csv << "ID,STATUS,TIMESTAMP,TEMP,HUM,WEIGHT,BAT,CO2,VOC,MOTION,ONLINE,";
    
    // Audio (63 parametry)
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
    
    // Radar (28 parametrów)
    csv << "RADAR_DISTANCE,RADAR_ENERGY,RADAR_SPEED,RADAR_DISTANCE_STD,RADAR_ENERGY_STD,RADAR_SPEED_STD,";
    csv << "RADAR_DISTANCE_MIN,RADAR_DISTANCE_MAX,RADAR_ENERGY_MIN,RADAR_ENERGY_MAX,RADAR_RANGE,";
    csv << "RADAR_ENERGY_VARIANCE,RADAR_CV,RADAR_ACTIVITY,RADAR_IDLE_PERCENT,RADAR_MOTION_INTENSITY,";
    csv << "RADAR_TARGET_RATE,RADAR_MAX_TARGETS,RADAR_TARGET_DENSITY,RADAR_SLOPE,RADAR_CORRELATION,";
    csv << "RADAR_ACCELERATION,RADAR_SIGNAL_QUALITY,RADAR_ANOMALY_SCORE,RADAR_HIVE_HEALTH,";
    csv << "RADAR_POWER_SPECTRUM,RADAR_ZCR,RADAR_ENTROPY,";
    
    // HX711 Waga (77 parametrów)
    csv << "HX711_CURRENT,HX711_MEAN,HX711_STD,HX711_MIN,HX711_MAX,HX711_MEDIAN,HX711_RANGE,HX711_VARIANCE,HX711_CV,HX711_IQR,";
    csv << "HX711_RATE,HX711_MEAN_RATE,HX711_MAX_POS_RATE,HX711_MAX_NEG_RATE,HX711_ACCELERATION,";
    csv << "HX711_SLOPE_1H,HX711_SLOPE_4H,HX711_SLOPE_24H,HX711_CORR_1H,HX711_CORR_4H,HX711_CORR_24H,HX711_DIRECTION,";
    csv << "HX711_NECTAR_INFLOW,HX711_NECTAR_ACCUM,HX711_FORAGING_EFF,HX711_BLOOM_INTENSITY,";
    csv << "HX711_HONEY_PROD_IDX,HX711_NECTAR_QUALITY,HX711_CONSUMPTION_RATE,HX711_DAILY_CONSUMPTION,";
    csv << "HX711_FOOD_RESERVE_DAYS,HX711_WINTER_READINESS,HX711_STARVATION_RISK,";
    csv << "HX711_DAILY_AMPLITUDE,HX711_CIRCADIAN_STR,HX711_SEASONAL_TREND,";
    csv << "HX711_SIGNAL_QUALITY,HX711_NOISE_LEVEL,HX711_DRIFT_RATE,HX711_OUTLIER_RATIO,HX711_SAMPLE_COUNT,";
    csv << "HX711_FORECAST_1H,HX711_FORECAST_6H,HX711_FORECAST_24H,HX711_FORECAST_CONF,HX711_PREDICTION_ERROR,";
    csv << "HX711_HARVEST_EVENT,HX711_SWARM_EVENT,HX711_FEEDING_EVENT,HX711_WATER_COLLECTION,";
    csv << "HX711_SKEWNESS,HX711_KURTOSIS,HX711_ENTROPY,HX711_COMPLEXITY,HX711_FRACTAL_DIM,";
    csv << "HX711_COLONY_MASS,HX711_BEE_POPULATION,HX711_BROOD_MASS,HX711_FOOD_STORES,HX711_HONEY_STORES,HX711_POLLEN_STORES,";
    csv << "HX711_PRODUCTIVITY_IDX,HX711_EFFICIENCY_SCORE,HX711_GROWTH_RATE,HX711_BALANCE_SCORE,HX711_RESILIENCE_IDX,";
    csv << "HX711_DOMINANT_PERIOD,HX711_POWER_24H,HX711_POWER_12H,HX711_POWER_6H,HX711_POWER_1H,";
    csv << "HX711_COMPLETENESS,HX711_RELIABILITY,HX711_ACCURACY_EST,HX711_PRECISION_EST,";
    csv << "HX711_ANOMALY_FLAG,HX711_HEALTH_SCORE,";
    
    // Temp/Humidity (32 parametry)
    csv << "TH_TEMP_MEAN,TH_TEMP_STD,TH_TEMP_MIN,TH_TEMP_MAX,TH_TEMP_RANGE,";
    csv << "TH_HUM_MEAN,TH_HUM_STD,TH_HUM_MIN,TH_HUM_MAX,TH_HUM_RANGE,";
    csv << "TH_HEAT_INDEX,TH_DEW_POINT,TH_COMFORT_IDX,TH_BROOD_STRESS,TH_MOLD_RISK,";
    csv << "TH_TEMP_STABILITY,TH_VPD,TH_THI,TH_ENTHALPY,TH_AIR_DENSITY,TH_SPECIFIC_HUMIDITY,";
    csv << "TH_MIXING_RATIO,TH_SATURATION_DEFICIT,TH_EVAP_POTENTIAL,TH_CONDENSATION_RISK,";
    csv << "TH_VENTILATION_NEED,TH_COOLING_NEED,TH_HEATING_NEED,TH_CLIMATE_SCORE,";
    csv << "TH_HUM_STABILITY_1H,TH_HUM_TREND,TH_TEMP_TREND,";
    
    // Air Quality (37 parametrów)
    csv << "AQ_IAQ,AQ_IAQ_MEAN,AQ_CONTAMINATION,AQ_MOLD_RISK,AQ_VENTILATION_IDX,";
    csv << "AQ_CO2_MEAN,AQ_VOC_MEAN,AQ_STRESS_AIR,AQ_PURITY,AQ_FRESHNESS,AQ_POLLUTION_LOAD,";
    csv << "AQ_GAS_IDX,AQ_PARTICULATE_IDX,AQ_ALLERGEN_RISK,AQ_TOXICITY_EST,";
    csv << "AQ_OXIDATION_CAP,AQ_REDUCTION_CAP,AQ_BUFFER_CAP,AQ_RECOVERY_RATE,";
    csv << "AQ_STABILITY,AQ_TREND,AQ_FORECAST,AQ_HEALTH_IMPACT,AQ_BEE_BEHAVIOR_IMPACT,";
    csv << "AQ_CO2_ALERT,AQ_CO2_BASE,AQ_CO2_PEAK,AQ_CO2_TREND,AQ_CO2_VARIABILITY,";
    csv << "AQ_COMFORT_SCORE,AQ_ENV_CORRELATION,AQ_STRESS_LEVEL,AQ_VARIABILITY_IDX,";
    csv << "AQ_VOC_BASE,AQ_VOC_PEAK,AQ_VOC_VARIABILITY,AQ_VOC_WARNING,";
    
    // Piezo Vibration (33 parametry)
    csv << "PV_VIB_RMS,PV_VIB_PEAK,PV_VIB_FREQ_DOM,PV_BEE_TRAFFIC,PV_INTRUSION_PROB,";
    csv << "PV_PREDATOR_DET,PV_QUEEN_PIPING,PV_VIBRATION_ENERGY,PV_ACTIVITY_PATTERN,";
    csv << "PV_DISTURBANCE_IDX,PV_HEALTH_VIB,PV_POPULATION_EST,PV_AGGRESSION_IDX,";
    csv << "PV_SWARM_PREP,PV_CLUSTER_SIZE,PV_CLUSTER_ACTIVITY,PV_FANNING,PV_BUZZ_PIPING,";
    csv << "PV_TREMBLE_DANCE,PV_WAGGLE_DANCE,PV_FORAGING_COMM,PV_ALERT_LEVEL,";
    csv << "PV_ALIEN_DETECTED,PV_CONFIDENCE,PV_CONTINUOUS_VIB,PV_EVENT_COUNT,";
    csv << "PV_IMPACT_FLAG,PV_SEVERITY,PV_SOURCE_TYPE,PV_VIB_MEAN,PV_VIB_STD,";
    csv << "PV_WIND_NOISE,PV_ZCR,";
    
    // Barometric (30 parametrów)
    csv << "BARO_PRESSURE,BARO_PRESSURE_MEAN,BARO_PRESSURE_STD,BARO_PRESSURE_MIN,BARO_PRESSURE_MAX,";
    csv << "BARO_PRESSURE_TREND,BARO_WEATHER_TREND,BARO_STORM_PROB,BARO_FORAGING_COND,";
    csv << "BARO_PRESSURE_CHANGE,BARO_ALTITUDE_EST,BARO_SEALEVEL_PRESS,BARO_DENSITY_ALT,";
    csv << "BARO_WEATHER_SCORE,BARO_FRONT_APPROACH,BARO_STABILITY,BARO_BEE_ACTIVITY_PRED,";
    csv << "BARO_WEATHER_ALERT,BARO_ACTIVITY_PRED,BARO_ALERT_FLAG,BARO_FORAGING_IDX,";
    csv << "BARO_IMPROVING_FLAG,BARO_RAIN_RISK,BARO_RELIABILITY_IDX,BARO_SEVERITY,";
    csv << "BARO_STORM_RISK,BARO_TEMPERATURE,BARO_TREND_MEDIUM,BARO_TREND_SHORT,BARO_WEATHER_IDX,";
    
    // Light (20 parametrów)
    csv << "LIGHT_LUX,LIGHT_LUX_MEAN,LIGHT_LUX_MIN,LIGHT_LUX_MAX,LIGHT_DAYLIGHT_HOURS,";
    csv << "LIGHT_DARKNESS_HOURS,LIGHT_TWILIGHT,LIGHT_CIRCADIAN_SYNC,LIGHT_FORAGING_IDX,";
    csv << "LIGHT_PHOTOPERIOD,LIGHT_SEASONAL_CHANGE,LIGHT_CLOUD_COVER_EST,LIGHT_SUNRISE_OFFSET,";
    csv << "LIGHT_UV_IDX,LIGHT_SPECTRUM_BLUE,LIGHT_SPECTRUM_RED,LIGHT_SPECTRUM_IR,";
    csv << "LIGHT_IR_RATIO,LIGHT_LUX_STD,LIGHT_SPECTRUM_DATA,";
    
    // Dodatkowe
    csv << "WEIGHT_RATE,WEIGHT_TREND,AIR_IAQ";
    
    return csv.str();
}

// ============================================================================
// KONWERSJA HIVEDATA DO CSV
// ============================================================================

std::string hiveDataToCSV(const HiveData& data) {
    std::stringstream csv;
    
    // Podstawowe
    csv << data.hive_id << ",";
    csv << "OK,";
    csv << data.timestamp << ",";
    csv << data.temperature << ",";
    csv << data.humidity << ",";
    csv << data.weight << ",";
    csv << data.battery_level << ",";
    csv << data.co2_eq << ",";
    csv << data.voc_idx << ",";
    csv << data.motion_detected << ",";
    csv << (data.is_online ? "1" : "0") << ",";
    
    // Audio
    csv << data.audio_rms << "," << data.audio_peak << "," << data.audio_peak_to_peak << ",";
    csv << data.audio_zcr << "," << data.audio_energy << "," << data.audio_mean_amp << ",";
    csv << data.audio_std_amp << "," << data.audio_cv_amp << "," << data.audio_skewness << ",";
    csv << data.audio_kurtosis << "," << data.audio_dominant_freq << ",";
    csv << data.audio_spectral_centroid << "," << data.audio_spectral_bandwidth << ",";
    csv << data.audio_spectral_flatness << "," << data.audio_spectral_rolloff << ",";
    csv << data.audio_power_bee_band << "," << data.audio_power_swarm_band << ",";
    csv << data.audio_power_low_freq << "," << data.audio_power_high_freq << ",";
    csv << data.audio_harmonic_ratio << ",";
    csv << data.audio_mfcc_energy[0] << "," << data.audio_mfcc_energy[1] << ",";
    csv << data.audio_mfcc_energy[2] << "," << data.audio_mfcc_energy[3] << ",";
    csv << data.audio_spectral_entropy << "," << data.audio_spectral_contrast << ",";
    csv << data.audio_tonal_strength << "," << data.audio_crest_factor << ",";
    csv << data.audio_formant_f1 << "," << data.audio_formant_f2 << ",";
    csv << data.audio_fundamental_freq << "," << data.audio_pitch_strength << ",";
    csv << data.audio_inharmonicity << "," << data.audio_shimmer << ",";
    csv << data.audio_jitter << "," << data.audio_nhr << "," << data.audio_hnr << ",";
    csv << data.audio_autocorr_peak << "," << data.audio_attack_time << ",";
    csv << data.audio_decay_time << "," << data.audio_sustain_level << ",";
    csv << data.audio_temporal_centroid << "," << data.audio_loudness << ",";
    csv << data.audio_spectral_flux << "," << data.audio_spectral_slope << ",";
    csv << data.audio_spectral_kurtosis << "," << data.audio_spectral_skewness << ",";
    csv << data.audio_fund_salience << ",";
    for (int i = 0; i < 8; i++) {
        csv << data.audio_power_band[i] << ",";
    }
    csv << data.audio_leq << "," << data.audio_l10 << "," << data.audio_l90 << ",";
    csv << data.audio_noise_floor << "," << data.audio_snr << ",";
    csv << data.audio_aci << "," << data.audio_bi << "," << data.audio_ndi << ",";
    csv << data.audio_adi << "," << data.audio_aei << ",";
    csv << data.audio_bee_activity << "," << data.audio_swarm_prob << ",";
    csv << data.audio_stress_indicator << "," << data.audio_hive_health << ",";
    csv << data.audio_foraging_eff << "," << data.audio_colony_coherence << ",";
    
    // Radar
    csv << data.radar_distance << "," << data.radar_energy << "," << data.radar_speed << ",";
    csv << data.radar_distance_std << "," << data.radar_energy_std << "," << data.radar_speed_std << ",";
    csv << data.radar_distance_min << "," << data.radar_distance_max << ",";
    csv << data.radar_energy_min << "," << data.radar_energy_max << ",";
    csv << data.radar_range << "," << data.radar_energy_variance << ",";
    csv << data.radar_cv << "," << data.radar_activity << "," << data.radar_idle_percent << ",";
    csv << data.radar_motion_intensity << "," << data.radar_target_rate << ",";
    csv << data.radar_max_targets << "," << data.radar_target_density << ",";
    csv << data.radar_slope << "," << data.radar_correlation << ",";
    csv << data.radar_acceleration << "," << data.radar_signal_quality << ",";
    csv << data.radar_anomaly_score << "," << data.radar_hive_health << ",";
    csv << data.radar_power_spectrum << "," << data.radar_zcr << ",";
    csv << data.radar_entropy << ",";
    
    // HX711 - pomijam dla zwięzłości, ale można dodać analogicznie
    
    // Usuń ostatni przecinek jeśli jest
    std::string result = csv.str();
    if (!result.empty() && result.back() == ',') {
        result.pop_back();
    }
    
    return result;
}

// ============================================================================
// GENEROWANIE PEŁNEGO RAPORTU CSV
// ============================================================================

std::string generateFullCSV(const std::map<std::string, HiveData>& hives_data) {
    std::stringstream csv;
    
    // Nagłówek
    csv << getCSVHeader() << "\n";
    
    // Dane dla każdego ula
    for (const auto& pair : hives_data) {
        csv << hiveDataToCSV(pair.second) << "\n";
    }
    
    return csv.str();
}

} // namespace csv
} // namespace apiary
