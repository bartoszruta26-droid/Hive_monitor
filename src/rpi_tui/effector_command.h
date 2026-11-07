#pragma once

#include <map>
#include <string>
#include <cstdint>

namespace apiary {

enum class EffectorKind {
    HeaterPwm, FanPwm, Pump, Valve1, Valve2, Relay, ServoAngle
};

struct EffectorCommand {
    std::string hiveId;
    std::string commandId;
    EffectorKind kind;
    int value;
    unsigned int relayChannel;
};

// The parser supplies scalar JSON values. This validator is deliberately
// independent of transport and hardware so the exact same limits can be
// exercised on the Raspberry Pi host and in firmware-side tests.
inline bool validateEffectorCommand(
    const std::map<std::string, std::string>& fields,
    EffectorCommand& command,
    std::string& error
) {
    const auto hive = fields.find("hive_id");
    const auto id = fields.find("command_id");
    const auto action = fields.find("action");
    const auto value = fields.find("value");
    if (hive == fields.end() || id == fields.end() ||
        action == fields.end() || value == fields.end()) {
        error = "missing_required_field";
        return false;
    }
    if (hive->second.empty() || hive->second.size() > 32 ||
        id->second.empty() || id->second.size() > 40) {
        error = "invalid_identifier";
        return false;
    }
    for (char c : id->second) {
        const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!safe) {
            error = "invalid_command_id";
            return false;
        }
    }

    int numeric = 0;
    const std::string& raw = value->second;
    if (raw.empty()) {
        error = "invalid_value";
        return false;
    }
    for (char c : raw) {
        if (c < '0' || c > '9') {
            error = "value_must_be_unsigned_integer";
            return false;
        }
        if (numeric > 100000000) {
            error = "value_out_of_range";
            return false;
        }
        numeric = numeric * 10 + (c - '0');
    }

    command.hiveId = hive->second;
    command.commandId = id->second;
    command.value = numeric;
    command.relayChannel = 0;

    const std::string& name = action->second;
    if (name == "heater_pwm" || name == "fan_pwm") {
        if (numeric > 255) { error = "pwm_out_of_range"; return false; }
        command.kind = name == "heater_pwm" ? EffectorKind::HeaterPwm : EffectorKind::FanPwm;
    } else if (name == "pump" || name == "valve1" || name == "valve2") {
        if (numeric > 1) { error = "boolean_out_of_range"; return false; }
        command.kind = name == "pump" ? EffectorKind::Pump :
                       (name == "valve1" ? EffectorKind::Valve1 : EffectorKind::Valve2);
    } else if (name == "servo_angle") {
        if (numeric > 180) { error = "servo_angle_out_of_range"; return false; }
        command.kind = EffectorKind::ServoAngle;
    } else if (name.size() == 6 && name.compare(0, 5, "relay") == 0 &&
               name[5] >= '1' && name[5] <= '8') {
        if (numeric > 1) { error = "boolean_out_of_range"; return false; }
        command.kind = EffectorKind::Relay;
        command.relayChannel = static_cast<unsigned int>(name[5] - '0');
    } else {
        error = "unsupported_action";
        return false;
    }
    error.clear();
    return true;
}

} // namespace apiary
