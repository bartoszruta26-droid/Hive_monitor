/**
 * apiary_collector_csv.h
 * Moduł parsowania i generowania danych CSV dla 338+ parametrów uli
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

#ifndef APIARY_COLLECTOR_CSV_H
#define APIARY_COLLECTOR_CSV_H

#include <string>
#include <map>
#include <vector>
#include "apiary_collector_types.h"

// Przestrzeń nazw dla modułu CSV
namespace apiary {
namespace csv {

/**
 * Parsuje dane CSV z surowego stringa do struktury HiveData
 * @param raw_data Surowe dane CSV (oddzielone przecinkami)
 * @param source_ip Adres IP źródła danych
 * @param timestamp Timestamp danych
 * @param data Wyjściowa struktura HiveData do wypełnienia
 * @return true jeśli parsowanie powiodło się, false w przypadku błędu
 */
bool parseCSV(const std::string& raw_data, const std::string& source_ip, 
              long long timestamp, HiveData& data);

/**
 * Generuje nagłówek CSV dla wszystkich 338+ parametrów
 * @return String zawierający nagłówek CSV
 */
std::string getCSVHeader();

/**
 * Konwertuje strukturę HiveData do formatu CSV
 * @param data Struktura HiveData z danymi
 * @return String zawierający dane w formacie CSV
 */
std::string hiveDataToCSV(const HiveData& data);

/**
 * Generuje pełny raport CSV dla wszystkich uli
 * @param hives_data Mapa danych uli (hive_id -> HiveData)
 * @return String zawierający pełny raport CSV z nagłówkiem i danymi
 */
std::string generateFullCSV(const std::map<std::string, HiveData>& hives_data);

/**
 * Pomocnicza funkcja do bezpiecznego parsowania float z fallbackiem
 * @param parts Wektor części stringa po podziale
 * @param idx Indeks elementu do pobrania
 * @param defaultValue Wartość domyślna jeśli indeks poza zakresem
 * @return Wartość float lub defaultValue
 */
float safeGetFloat(const std::vector<std::string>& parts, size_t idx, float defaultValue = 0.0f);

/**
 * Pomocnicza funkcja do bezpiecznego parsowania int z fallbackiem
 * @param parts Wektor części stringa po podziale
 * @param idx Indeks elementu do pobrania
 * @param defaultValue Wartość domyślna jeśli indeks poza zakresem
 * @return Wartość int lub defaultValue
 */
int safeGetInt(const std::vector<std::string>& parts, size_t idx, int defaultValue = 0);

} // namespace csv
} // namespace apiary

#endif // APIARY_COLLECTOR_CSV_H
