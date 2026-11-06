#include "measurement_validation.h"

#include <cassert>
#include <map>
#include <string>

int main() {
    std::string error;
    std::map<std::string, std::string> valid = {
        {"hive_id", "UL-001"}, {"data_source", "pico"},
        {"temp", "34.5"}, {"hum", "65.2"}, {"weight", "48.25"},
        {"bat", "97"}, {"co2", "450"}, {"voc", "35"}, {"motion", "1"},
        {"audio_freq", "250"}, {"pressure", "1013.25"}, {"lux", "2500"}
    };
    assert(apiary::validateHivePayloadFields(valid, error));

    auto invalid = valid;
    invalid["temp"] = "NaN";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["temp"] = "12.3garbage";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["temp"] = std::string("1\0junk", 6);
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["hum"] = "100.01";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["bat"] = "101";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["motion"] = "0.5";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["pressure"] = "2000";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["audio_freq"] = "-1";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["hive_id"] = "../etc/passwd";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid.erase("hive_id");
    assert(!apiary::validateHivePayloadFields(invalid, error));
    invalid = valid;
    invalid["data_source"] = "unknown";
    assert(!apiary::validateHivePayloadFields(invalid, error));
    return 0;
}
