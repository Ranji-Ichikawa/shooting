#include "Base64.h"

namespace base64 {

namespace {
const char kTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

std::string encode(const std::string& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= data.size()) {
        unsigned char b0 = data[i];
        unsigned char b1 = data[i + 1];
        unsigned char b2 = data[i + 2];
        out += kTable[b0 >> 2];
        out += kTable[((b0 & 0x03) << 4) | (b1 >> 4)];
        out += kTable[((b1 & 0x0F) << 2) | (b2 >> 6)];
        out += kTable[b2 & 0x3F];
        i += 3;
    }

    size_t remaining = data.size() - i;
    if (remaining == 1) {
        unsigned char b0 = data[i];
        out += kTable[b0 >> 2];
        out += kTable[(b0 & 0x03) << 4];
        out += "==";
    } else if (remaining == 2) {
        unsigned char b0 = data[i];
        unsigned char b1 = data[i + 1];
        out += kTable[b0 >> 2];
        out += kTable[((b0 & 0x03) << 4) | (b1 >> 4)];
        out += kTable[(b1 & 0x0F) << 2];
        out += "=";
    }

    return out;
}

} // namespace base64
