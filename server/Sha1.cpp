#include "Sha1.h"
#include <cstdint>
#include <vector>

namespace sha1 {

namespace {

uint32_t rotl(uint32_t value, int bits) {
    return (value << bits) | (value >> (32 - bits));
}

} // namespace

std::string digest(const std::string& input) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    std::vector<unsigned char> msg(input.begin(), input.end());
    uint64_t bitLen = static_cast<uint64_t>(msg.size()) * 8;

    msg.push_back(0x80);
    while (msg.size() % 64 != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<unsigned char>((bitLen >> (i * 8)) & 0xFF));
    }

    for (size_t chunkStart = 0; chunkStart < msg.size(); chunkStart += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            size_t base = chunkStart + i * 4;
            w[i] = (static_cast<uint32_t>(msg[base]) << 24) |
                   (static_cast<uint32_t>(msg[base + 1]) << 16) |
                   (static_cast<uint32_t>(msg[base + 2]) << 8) |
                   (static_cast<uint32_t>(msg[base + 3]));
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = rotl(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    unsigned char out[20];
    uint32_t hs[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        out[i * 4 + 0] = static_cast<unsigned char>((hs[i] >> 24) & 0xFF);
        out[i * 4 + 1] = static_cast<unsigned char>((hs[i] >> 16) & 0xFF);
        out[i * 4 + 2] = static_cast<unsigned char>((hs[i] >> 8) & 0xFF);
        out[i * 4 + 3] = static_cast<unsigned char>(hs[i] & 0xFF);
    }

    return std::string(reinterpret_cast<char*>(out), 20);
}

} // namespace sha1
