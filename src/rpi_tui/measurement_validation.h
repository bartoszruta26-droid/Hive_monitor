#pragma once

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

namespace apiary {

struct NumericRange {
    double minimum;
    double maximum;
    bool integer_only;
};

inline const std::map<std::string, NumericRange>& measurementRanges() {
    static const std::map<std::string, NumericRange> ranges = {
        {"temp", {-40, 85, false}}, {"temp_raw", {-40, 85, false}},\n        {"th_heat_index", {-80, 100, false}}, {"th_dew_point", {-100, 80, false}},\n        {"aq_iaq_index", {0, 500, false}}, {"hx711_slope_1h", {-1.0e6, 1.0e6, false}},
        {"hum", {0, 100, false}}, {"hum_raw", {0, 100, false}},
        {"weight", {0, 1000, false}}, {"bat", {0, 100, true}},
        {"co2", {0, 100000, true}}, {"co2_raw", {0, 100000, true}},
        {"aq_co2_mean", {0, 100000, true}}, {"voc", {0, 500, true}},
        {"voc_raw", {0, 500, true}}, {"iaq_index", {0, 500, false}},
        {"air_iaq", {0, 500, true}}, {"motion", {0, 1, true}},
        {"audio_raw", {0, 1023, true}}, {"vibration_raw", {0, 1023, true}},
        {"weight_raw", {-8388608, 8388607, true}},
        {"audio_rms", {0, 1000, false}}, {"audio_freq", {0, 200000, false}},
        {"spectral_centroid", {0, 200000, false}}, {"swarm_prob", {0, 100, false}},
        {"bee_activity", {0, 100, false}}, {"power_bee_band", {-200, 200, false}},
        {"crest_factor", {0, 1000, false}}, {"spectral_entropy", {0, 1000, false}},
        {"foraging_eff", {0, 100, false}}, {"hive_health", {0, 100, false}},
        {"aci", {0, 1000, false}}, {"bi", {0, 1000, false}},
        {"adi", {0, 1000, false}}, {"nhr", {0, 200, false}},
        {"loudness", {0, 200, false}}, {"stress_indicator", {0, 100, false}},
        {"radar_dist", {0, 10000, false}}, {"radar_energy", {0, 1.0e9, false}},
        {"radar_activity", {0, 100, false}}, {"signal_quality", {0, 100, false}},
        {"target_rate", {0, 1.0e6, false}}, {"entropy", {0, 1000, false}},
        {"anomaly_score", {0, 100, false}}, {"radar_health", {0, 100, false}},
        {"max_targets", {0, 1000, true}}, {"motion_intensity", {0, 100, false}},
        {"hx_mean", {-1.0e7, 1.0e7, false}}, {"hx_std", {0, 1.0e7, false}},
        {"hx_slope_1h", {-1.0e6, 1.0e6, false}},
        {"hx_slope_24h", {-1.0e6, 1.0e6, false}},
        {"nectar_inflow", {-1.0e5, 1.0e5, false}},
        {"consumption_rate", {-1.0e5, 1.0e5, false}},
        {"colony_growth", {-1.0e5, 1.0e5, false}},
        {"productivity", {0, 100, false}}, {"predicted_24h", {0, 1000, false}},
        {"hx_anomaly", {0, 100, false}}, {"winter_readiness", {0, 100, false}},
        {"starvation_risk", {0, 100, false}},
        {"heat_index", {-80, 100, false}}, {"dew_point", {-100, 80, false}},
        {"comfort_index", {0, 100, false}}, {"brood_stress", {0, 100, false}},
        {"temp_stability", {0, 100, false}}, {"mold_risk", {0, 100, false}},
        {"aq_voc_mean", {0, 100000, true}}, {"ventilation_need", {0, 100, false}},
        {"contamination_risk", {0, 100, false}}, {"aq_comfort", {0, 100, false}},
        {"piezo_rms", {0, 1000, false}}, {"piezo_freq", {0, 200000, false}},
        {"piezo_activity", {0, 100, false}}, {"bee_traffic", {0, 100, false}},
        {"predator_score", {0, 100, false}}, {"intrusion_prob", {0, 100, false}},
        {"pressure", {300, 1200, false}}, {"baro_trend_1h", {-1000, 1000, false}},
        {"weather_trend", {0, 100, false}}, {"storm_prob", {0, 100, false}},
        {"foraging_cond", {0, 100, false}}, {"lux", {0, 200000, true}},
        {"daylight_hours", {0, 24, false}}, {"circadian_sync", {0, 100, false}},
        {"foraging_idx", {0, 100, false}}, {"weight_rate", {-1000, 1000, false}},
        {"weight_trend", {-1000, 1000, false}}
    };
    return ranges;
}

inline bool validateHivePayloadFields(
    const std::map<std::string, std::string>& values,
    std::string& error
) {
    const auto hive = values.find("hive_id");
    if (hive == values.end() || hive->second.empty() || hive->second.size() > 64) {
        error = "hive_id is required and must contain 1..64 characters";
        return false;
    }
    for (unsigned char c : hive->second) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) {
            error = "hive_id contains unsupported characters";
            return false;
        }
    }

    const auto source = values.find("data_source");
    if (source != values.end() && source->second != "pico" &&
        source->second != "precomputed" && source->second != "nano") {
        error = "data_source must be pico, precomputed or nano";
        return false;
    }

    for (const auto& item : values) {
        const auto rule = measurementRanges().find(item.first);
        if (rule == measurementRanges().end()) continue;
        if (item.second.empty()) {
            error = item.first + " must be a numeric scalar";
            return false;
        }
        errno = 0;
        char* end = nullptr;
        const double value = std::strtod(item.second.c_str(), &end);
        if (errno == ERANGE || end == item.second.c_str() || !end || *end != '\0' ||
            !std::isfinite(value)) {
            error = item.first + " must be a finite number";
            return false;
        }
        const NumericRange& range = rule->second;
        if (value < range.minimum || value > range.maximum) {
            error = item.first + " is outside the permitted range";
            return false;
        }
        if (range.integer_only && std::floor(value) != value) {
            error = item.first + " must be an integer";
            return false;
        }
    }
    return true;
}

} // namespace apiary
