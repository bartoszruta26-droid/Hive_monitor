#include <cstdint>
#include <cstdlib>
#include <iostream>
#include "../src/pico_refactored/include/safe_mode_policy.h"

namespace {
int failures = 0;

void check(bool condition, const char* name) {
    if (!condition) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}
}

int main() {
    using namespace safeModePolicy;

    // Temperature sensor detection, health and freshness boundary.
    check(heaterSensorReady(true, true, 0, 1000, 6999, 6000, 3),
          "fresh sample below max age");
    check(heaterSensorReady(true, true, 0, 1000, 7000, 6000, 3),
          "sample exactly at max age");
    check(!heaterSensorReady(true, true, 0, 1000, 7001, 6000, 3),
          "sample older than max age rejected");
    check(!heaterSensorReady(false, true, 0, 1000, 2000, 6000, 3),
          "undetected sensor rejected");
    check(!heaterSensorReady(true, false, 0, 1000, 2000, 6000, 3),
          "inactive sensor rejected");
    check(!heaterSensorReady(true, true, 3, 1000, 2000, 6000, 3),
          "error threshold reached rejected");
    check(heaterSensorReady(true, true, 2, 1000, 2000, 6000, 3),
          "error count below threshold accepted");
    check(!heaterSensorReady(true, true, 0, 0, 2000, 6000, 3),
          "missing valid sample rejected");
    check(heaterSensorReady(true, true, 0, 0xFFFFFF00u, 0x00000100u, 600u, 3),
          "millis rollover freshness handled");

    // Heater output: zero is always an allowed de-energizing command.
    check(mayEnergizeHeater(0, false, false), "zero duty allowed without sensor");
    check(mayEnergizeHeater(0, true, false), "zero duty allowed while latched");
    check(!mayEnergizeHeater(-1, false, true), "negative duty rejected");
    check(!mayEnergizeHeater(256, false, true), "duty above 255 rejected");
    check(mayEnergizeHeater(128, false, true), "valid heating request accepted");
    check(!mayEnergizeHeater(128, false, false), "heating blocked without healthy sensor");
    check(!mayEnergizeHeater(128, true, true), "heating blocked while main latch active");
    check(mayEnergizeHeater(255, false, true), "maximum duty accepted with healthy sensor");

    // Main latch release requires all three conditions.
    check(mayReleaseMain(true, true, true), "main release accepted when confirmed and healthy");
    check(!mayReleaseMain(false, true, true), "inactive main latch cannot be released");
    check(!mayReleaseMain(true, false, true), "main release requires operator confirmation");
    check(!mayReleaseMain(true, true, false), "main release requires healthy sensor");

    // Flowing Hive release is independent, but gated by the main latch and
    // explicit physical inspection acknowledgement.
    check(mayReleaseFlowingHive(true, false, true),
          "flow latch release allowed after main release and inspection");
    check(!mayReleaseFlowingHive(true, true, true),
          "flow latch cannot bypass main latch");
    check(!mayReleaseFlowingHive(true, false, false),
          "flow latch release requires physical inspection");
    check(!mayReleaseFlowingHive(false, false, true),
          "inactive flow latch cannot be released");

    if (failures != 0) {
        std::cerr << failures << " safe-mode policy check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Safe-mode policy tests: PASS\n";
    return EXIT_SUCCESS;
}
