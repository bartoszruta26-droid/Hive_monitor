#pragma once

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <algorithm>
#include <ctime>
#include <map>
#include <mutex>
#include <vector>

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


class ConnectionRateLimiter {
public:
    static constexpr size_t MAX_TRACKED_IPS = 4096;
    static constexpr size_t MAX_CONNECTIONS_PER_SECOND = 10;
    static constexpr long long CLEANUP_INTERVAL_SEC = 60;

    bool allowConnection(const std::string& ip, long long now = std::time(nullptr)) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (now - last_cleanup_ >= CLEANUP_INTERVAL_SEC) cleanupLocked(now);

        auto entry = connection_times_.find(ip);
        if (entry == connection_times_.end()) {
            if (connection_times_.size() >= MAX_TRACKED_IPS) return false;
            entry = connection_times_.emplace(ip, std::vector<long long>{}).first;
        }

        auto& times = entry->second;
        times.erase(std::remove_if(times.begin(), times.end(),
            [now](long long timestamp) { return now - timestamp > 1; }), times.end());
        if (times.size() >= MAX_CONNECTIONS_PER_SECOND) return false;
        times.push_back(now);
        return true;
    }

    void cleanup(long long now = std::time(nullptr)) {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanupLocked(now);
    }

    size_t trackedAddressCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_times_.size();
    }

private:
    void cleanupLocked(long long now) {
        for (auto it = connection_times_.begin(); it != connection_times_.end();) {
            if (it->second.empty() || now - it->second.back() > CLEANUP_INTERVAL_SEC) {
                it = connection_times_.erase(it);
            } else {
                ++it;
            }
        }
        last_cleanup_ = now;
    }

    mutable std::mutex mutex_;
    std::map<std::string, std::vector<long long>> connection_times_;
    long long last_cleanup_ = 0;
};

} // namespace apiary
