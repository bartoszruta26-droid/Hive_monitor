#pragma once

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/socket.h>

namespace apiary {

inline bool datagramWasTruncated(int messageFlags, ssize_t receivedBytes, size_t payloadCapacity) {
    return (messageFlags & MSG_TRUNC) != 0 ||
           receivedBytes > static_cast<ssize_t>(payloadCapacity);
}

inline bool isPrivateOrLoopbackIPv4(const in_addr& address) {
    const uint32_t host = ntohl(address.s_addr);
    const uint8_t first = static_cast<uint8_t>(host >> 24);
    const uint8_t second = static_cast<uint8_t>((host >> 16) & 0xff);
    return first == 10 ||
           first == 127 ||
           (first == 172 && second >= 16 && second <= 31) ||
           (first == 192 && second == 168) ||
           (first == 169 && second == 254);
}

inline bool parseIPv4BindAddress(const char* configured, const char* fallback, in_addr& address) {
    const char* selected = (configured == nullptr || *configured == '\0') ? fallback : configured;
    return selected != nullptr &&
           inet_pton(AF_INET, selected, &address) == 1 &&
           isPrivateOrLoopbackIPv4(address);
}

inline std::string ipv4AddressToString(const in_addr& address) {
    char text[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &address, text, sizeof(text)) == nullptr) return "unknown";
    return std::string(text);
}

} // namespace apiary
