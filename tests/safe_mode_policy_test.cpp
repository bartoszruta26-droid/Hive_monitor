#include <cassert>
#include <cstdint>
#include "../src/pico_refactored/include/safe_mode_policy.h"
int main() {
    using namespace safeModePolicy;
    assert(heaterSensorReady(true,true,0,1000,6999,6000,3));
    assert(!heaterSensorReady(false,true,0,1000,2000,6000,3));
    assert(!heaterSensorReady(true,false,0,1000,2000,6000,3));
    assert(!heaterSensorReady(true,true,3,1000,2000,6000,3));
    assert(!heaterSensorReady(true,true,0,0,2000,6000,3));
    assert(!heaterSensorReady(true,true,0,1000,7001,6000,3));
    assert(heaterSensorReady(true,true,0,0xFFFFFF00u,0x00000100u,600u,3));
    assert(mayReleaseMain(true,true,true));
    assert(!mayReleaseMain(false,true,true));
    assert(!mayReleaseMain(true,false,true));
    assert(!mayReleaseMain(true,true,false));
    assert(mayReleaseFlowingHive(true,false,true));
    assert(!mayReleaseFlowingHive(true,true,true));
    assert(!mayReleaseFlowingHive(true,false,false));
    assert(!mayReleaseFlowingHive(false,false,true));
}
