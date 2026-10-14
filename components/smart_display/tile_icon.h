#pragma once
#include <cstdint>
#include <string>

namespace tile_icon {
// ESP Screen Manager sends a chosen icon as its Material Design Icons codepoint
// in hex ("F06B5"). Anything else is 0: the tile keeps its domain icon.
inline uint32_t codepoint(const std::string &hex) {
  if (hex.size() != 5) return 0;
  uint32_t value = 0;
  for (char c : hex) {
    int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    if (digit < 0) return 0;
    value = value << 4 | static_cast<uint32_t>(digit);
  }
  // MDI lives in Supplementary Private Use Area-A.
  return value >= 0xF0000 && value <= 0xFFFFD ? value : 0;
}
// Four-byte UTF-8 for a label; empty for 0.
inline std::string utf8(uint32_t cp) {
  if (cp < 0x10000 || cp > 0x10FFFF) return {};
  return {static_cast<char>(0xF0 | cp >> 18), static_cast<char>(0x80 | (cp >> 12 & 0x3F)),
          static_cast<char>(0x80 | (cp >> 6 & 0x3F)), static_cast<char>(0x80 | (cp & 0x3F))};
}
}  // namespace tile_icon
