#include "network_security.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    assert(!apiary::datagramWasTruncated(0, 0, 1023));
    assert(!apiary::datagramWasTruncated(0, 1022, 1023));
    assert(!apiary::datagramWasTruncated(0, 1023, 1023)); // exact-fit datagram
    assert(apiary::datagramWasTruncated(MSG_TRUNC, 1023, 1023));
    assert(apiary::datagramWasTruncated(0, 1024, 1023));

    in_addr address{};
    assert(apiary::parseIPv4BindAddress(nullptr, "127.0.0.1", address));
    assert(apiary::ipv4AddressToString(address) == "127.0.0.1");
    assert(apiary::parseIPv4BindAddress("192.168.10.25", "127.0.0.1", address));
    assert(apiary::ipv4AddressToString(address) == "192.168.10.25");
    assert(!apiary::parseIPv4BindAddress("not-an-ip", "127.0.0.1", address));

    std::cout << "Network safety helper tests passed\n";
    return 0;
}
