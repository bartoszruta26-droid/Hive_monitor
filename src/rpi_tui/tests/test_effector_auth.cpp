#include "../../shared/effector_auth.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    uint8_t key[16] = {};
    for (uint8_t i = 0; i < 16; ++i) key[i] = i;
    uint8_t message[64] = {};
    for (uint8_t i = 0; i < 64; ++i) message[i] = i;

    static const uint64_t vectors[] = {
        0x726fdb47dd0e0e31ULL, 0x74f839c593dc67fdULL,
        0x0d6c8009d9a94f5aULL, 0x85676696d7fb7e2dULL,
        0xcf2794e0277187b7ULL, 0x18765564cd99a68dULL,
        0xcbc9466e58fee3ceULL, 0xab0200f58b01d137ULL,
        0x93f5f5799a932462ULL, 0x9e0082df0ba9e4b0ULL,
        0x7a5dbbc594ddb9f3ULL, 0xf4b32f46226bada7ULL,
        0x751e8fbc860ee5fbULL, 0x14ea5627c0843d90ULL,
        0xf723ca908e7af2eeULL, 0xa129ca6149be45e5ULL
    };
    for (size_t length = 0; length < sizeof(vectors) / sizeof(vectors[0]); ++length) {
        assert(apiary::auth::sipHash24(key, message, length) == vectors[length]);
    }

    uint8_t parsed[16] = {};
    assert(apiary::auth::parseKeyHex("000102030405060708090a0b0c0d0e0f", parsed));
    for (size_t i = 0; i < 16; ++i) assert(parsed[i] == i);
    assert(!apiary::auth::parseKeyHex("000102", parsed));
    assert(!apiary::auth::parseKeyHex("000102030405060708090a0b0c0d0e0g", parsed));

    const uint64_t tag = apiary::auth::sipHash24(key, message, 15);
    assert(apiary::auth::constantTimeEqual64(tag, vectors[15]));
    assert(!apiary::auth::constantTimeEqual64(tag, vectors[14]));

    std::cout << "SipHash-2-4 authentication vectors: PASS\n";
    return 0;
}
