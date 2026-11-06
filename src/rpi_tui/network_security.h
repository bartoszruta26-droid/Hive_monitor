#pragma once

#include <arpa/inet.h>
#include <cstddef>
#include <string>
#include <sys/socket.h>

namespace apiary {

inline bool datagramWasTruncated(int messageFlags, ssize_t receivedBytes, size_t payloadCapacity) {
    return (messageFlags & MSG_TRUNC) != 0 ||
           receivedBytes >= static_cast<ssize_t>(payloadCapacity);
}

inline bool parseIPv4BindAddress(const char* configured, const char* fallback, in_addr& address) {
    const char* selected = (configured == nullptr || *configured == '\0') ? fallback : configured;
    return selected != nullptr && inet_pton(AF_INET, selected, &address) == 1;
}

inline std::string ipv4AddressToString(const in_addr& address) {
    char text[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &address, text, sizeof(text)) == nullptr) return "unknown";
    return std::string(text);
}

} // namespace apiary
