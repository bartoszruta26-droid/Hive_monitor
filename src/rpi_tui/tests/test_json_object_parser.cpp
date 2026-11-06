#include "json_object_parser.h"

#include <cassert>
#include <map>
#include <string>

int main() {
    std::map<std::string, std::string> values;
    assert(apiary::parseJsonObjectScalars(
        R"({"hive_id":"UL-1","temp":23.75,"nested":{"temp":99},"escaped":"UL\\\"-2","unicode":"Pas\u0142eka","items":[1,{"x":2}],"enabled":true})",
        values));
    assert(values.at("hive_id") == "UL-1");
    assert(values.at("temp") == "23.75");
    assert(values.at("nested").empty());
    assert(values.at("escaped") == "UL\\\"-2");
    assert(values.at("unicode") == "Pasłeka");
    assert(values.at("items").empty());
    assert(values.at("enabled") == "true");

    assert(!apiary::parseJsonObjectScalars(R"({"temp":1,})", values));
    assert(!apiary::parseJsonObjectScalars(R"({"temp":1,"temp":2})", values));
    assert(!apiary::parseJsonObjectScalars(R"({"temp":NaN})", values));
    assert(!apiary::parseJsonObjectScalars(R"({"temp":"unterminated})", values));
    assert(!apiary::parseJsonObjectScalars(R"({"temp":1} trailing)", values));
    assert(!apiary::parseJsonObjectScalars(R"({"temp":"bad\qescape"})", values));
    assert(!apiary::parseJsonObjectScalars(R"({"temp":"\uD800"})", values));
    return 0;
}
