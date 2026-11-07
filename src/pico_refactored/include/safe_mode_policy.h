#ifndef SAFE_MODE_POLICY_H
#define SAFE_MODE_POLICY_H
#include <stdint.h>

namespace safeModePolicy {
inline bool heaterSensorReady(bool detected, bool active, uint8_t consecutiveErrors,
                              uint32_t lastValidSample, uint32_t now,
                              uint32_t maxAgeMs, uint8_t maxErrors) {
    return detected && active && consecutiveErrors < maxErrors &&
           lastValidSample != 0 &&
           static_cast<uint32_t>(now - lastValidSample) <= maxAgeMs;
}
inline bool mayReleaseMain(bool mainLatchActive, bool operatorConfirmed,
                           bool heaterSensorHealthy) {
    return mainLatchActive && operatorConfirmed && heaterSensorHealthy;
}
inline bool mayReleaseFlowingHive(bool flowingHiveLatchActive, bool mainLatchActive,
                                  bool operatorConfirmed) {
    return flowingHiveLatchActive && !mainLatchActive && operatorConfirmed;
}
}
#endif
