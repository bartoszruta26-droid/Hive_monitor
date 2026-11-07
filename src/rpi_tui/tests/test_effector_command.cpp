#include "effector_command.h"
#include "json_object_parser.h"

#include <cassert>
#include <iostream>
#include <map>
#include <string>

int main() {
    const std::string valid =
        R"({"hive_id":"UL-001","command_id":"cmd-42","action":"heater_pwm","value":120})";
    std::map<std::string, std::string> fields;
    apiary::EffectorCommand command{};
    std::string error;
    assert(apiary::parseJsonObjectScalars(valid, fields));
    assert(apiary::validateEffectorCommand(fields, command, error));
    assert(command.hiveId == "UL-001");
    assert(command.commandId == "cmd-42");
    assert(command.kind == apiary::EffectorKind::HeaterPwm);
    assert(command.value == 120);

    const auto rejected = [&](const std::string& json) {
        fields.clear();
        assert(apiary::parseJsonObjectScalars(json, fields));
        assert(!apiary::validateEffectorCommand(fields, command, error));
        assert(!error.empty());
    };

    rejected(R"({"hive_id":"UL-001","command_id":"x","action":"heater_pwm","value":256})");
    rejected(R"({"hive_id":"UL-001","command_id":"x","action":"servo_angle","value":181})");
    rejected(R"({"hive_id":"UL-001","command_id":"x","action":"relay9","value":1})");
    rejected(R"({"hive_id":"UL-001","command_id":"x","action":"pump","value":2})");
    rejected(R"({"hive_id":"UL-001","command_id":"x","action":"release_safe_mode","value":1})");
    rejected(R"({"hive_id":"UL-001","command_id":"bad/id","action":"fan_pwm","value":1})");
    rejected(R"({"hive_id":"UL-001","action":"pump","value":1})");

    fields.clear();
    assert(apiary::parseJsonObjectScalars(
        R"({"hive_id":"UL-001","command_id":"r8","action":"relay8","value":1})", fields));
    assert(apiary::validateEffectorCommand(fields, command, error));
    assert(command.kind == apiary::EffectorKind::Relay);
    assert(command.relayChannel == 8);

    std::cout << "Effector command validation: PASS\n";
    return 0;
}
