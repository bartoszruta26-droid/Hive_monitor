#pragma once

#include <stddef.h>
#include <stdint.h>

namespace apiary {
namespace auth {

// SipHash-2-4: compact keyed message authentication for small embedded devices.
// The 128-bit key MUST be unique per installation and supplied out-of-band.
// This primitive authenticates messages; it does not encrypt UDP traffic.
inline uint64_t rotateLeft64(uint64_t value, unsigned int bits) {
    return (value << bits) | (value >> (64U - bits));
}

inline uint64_t loadLittleEndian64(const uint8_t* data) {
    uint64_t value = 0;
    for (unsigned int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data[i]) << (8U * i);
    }
    return value;
}

inline void sipRound(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3) {
    v0 += v1; v1 = rotateLeft64(v1, 13); v1 ^= v0; v0 = rotateLeft64(v0, 32);
    v2 += v3; v3 = rotateLeft64(v3, 16); v3 ^= v2;
    v0 += v3; v3 = rotateLeft64(v3, 21); v3 ^= v0;
    v2 += v1; v1 = rotateLeft64(v1, 17); v1 ^= v2; v2 = rotateLeft64(v2, 32);
}

inline uint64_t sipHash24(const uint8_t key[16], const uint8_t* message, size_t length) {
    const uint64_t k0 = loadLittleEndian64(key);
    const uint64_t k1 = loadLittleEndian64(key + 8);
    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1;

    const size_t blocks = length / 8;
    for (size_t i = 0; i < blocks; ++i) {
        const uint64_t m = loadLittleEndian64(message + i * 8);
        v3 ^= m;
        sipRound(v0, v1, v2, v3); sipRound(v0, v1, v2, v3);
        v0 ^= m;
    }

    uint64_t tail = static_cast<uint64_t>(length & 0xffU) << 56;
    const size_t offset = blocks * 8;
    for (size_t i = 0; i < (length & 7U); ++i) {
        tail |= static_cast<uint64_t>(message[offset + i]) << (8U * i);
    }
    v3 ^= tail;
    sipRound(v0, v1, v2, v3); sipRound(v0, v1, v2, v3);
    v0 ^= tail;
    v2 ^= 0xffU;
    sipRound(v0, v1, v2, v3); sipRound(v0, v1, v2, v3);
    sipRound(v0, v1, v2, v3); sipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

inline bool parseKeyHex(const char* hex, uint8_t key[16]) {
    if (!hex) return false;
    for (size_t i = 0; i < 32; ++i) if (hex[i] == '\0') return false;
    if (hex[32] != '\0') return false;
    for (size_t i = 0; i < 16; ++i) {
        const char a = hex[i * 2], b = hex[i * 2 + 1];
        const auto nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        const int hi = nibble(a), lo = nibble(b);
        if (hi < 0 || lo < 0) return false;
        key[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

inline bool constantTimeEqual64(uint64_t a, uint64_t b) {
    uint64_t difference = a ^ b;
    difference |= difference >> 32;
    difference |= difference >> 16;
    difference |= difference >> 8;
    return static_cast<uint8_t>(difference) == 0;
}

} // namespace auth
} // namespace apiary
