#pragma once
#include <string>

namespace sha1 {

// Returns the 20-byte SHA-1 digest of `input` as a raw (non-hex) byte string.
std::string digest(const std::string& input);

} // namespace sha1
