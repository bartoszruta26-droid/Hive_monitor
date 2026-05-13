/**
 * apiary_collector_parser.h
 * Moduł parsowania danych JSON i CSV dla kolektora uli
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#ifndef APIARY_COLLECTOR_PARSER_H
#define APIARY_COLLECTOR_PARSER_H

#include "apiary_collector_types.h"
#include <string>
#include <map>

namespace apiary {
namespace collector {

/**
 * Parsuje dane JSON i aktualizuje strukturę HiveData
 * @param json_str String z danymi JSON
 * @param data Struktura HiveData do wypełnienia
 * @param source_ip Adres IP źródła danych
 * @param timestamp Czas otrzymania danych
 * @return true jeśli parsowanie powiodło się
 */
bool parseJSON(const std::string& json_str, HiveData& data, 
               const std::string& source_ip, long long timestamp);

/**
 * Parsuje dane CSV i aktualizuje strukturę HiveData
 * @param csv_str String z danymi CSV
 * @param data Struktura HiveData do wypełnienia
 * @param source_ip Adres IP źródła danych
 * @param timestamp Czas otrzymania danych
 * @return true jeśli parsowanie powiodło się
 */
bool parseCSV(const std::string& csv_str, HiveData& data,
              const std::string& source_ip, long long timestamp);

/**
 * Detekcja typu danych źródłowych (pico vs nano)
 * @param data Struktura HiveData z danymi
 * @return "pico" jeśli dane prekomputowane, "nano" jeśli surowe
 */
std::string detectDataSource(const HiveData& data);

} // namespace collector
} // namespace apiary

#endif // APIARY_COLLECTOR_PARSER_H
