#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include "json_object_parser.h"
#include "measurement_validation.h"

int main() {
    const std::string pico_payload =
        R"({"hive_id":"UL-001","data_source":"pico","temp":24.5,"hum":65.2,"weight":45,"voc":35,"nox":12,"co2_available":true,"co2":450})";
    std::map<std::string, std::string> values;
    assert(apiary::parseJsonObjectScalars(pico_payload, values));
    std::string error;
    assert(apiary::validateHivePayloadFields(values, error));
    assert(values.at("hive_id") == "UL-001");
    assert(values.at("data_source") == "pico");
    assert(values.at("temp") == "24.5");
    assert(values.at("hum") == "65.2");
    assert(values.at("weight") == "45");
    assert(values.at("voc") == "35");
    assert(values.at("nox") == "12");
    assert(values.at("co2_available") == "true");
    assert(values.at("co2") == "450");

    const std::string no_co2_payload =
        R"({"hive_id":"UL-001","data_source":"pico","temp":24.5,"hum":65.2,"weight":45,"voc":35,"nox":12,"co2_available":false})";
    values.clear();
    assert(apiary::parseJsonObjectScalars(no_co2_payload, values));
    assert(apiary::validateHivePayloadFields(values, error));
    assert(values.find("co2") == values.end());

    const std::string missing_id =
        R"({"data_source":"pico","temp":24.5,"hum":65.2,"weight":45})";
    values.clear();
    assert(apiary::parseJsonObjectScalars(missing_id, values));
    assert(!apiary::validateHivePayloadFields(values, error));

    const std::string invalid_nox =
        R"({"hive_id":"UL-001","data_source":"pico","nox":501})";
    values.clear();
    assert(apiary::parseJsonObjectScalars(invalid_nox, values));
    assert(!apiary::validateHivePayloadFields(values, error));

    std::cout << "Pico UDP telemetry contract: PASS\n";
    return 0;
}
