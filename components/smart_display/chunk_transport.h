#pragma once
#include <string>
#include <cstdint>
namespace runtime_tiles {
// HA text.set_value is limited to 255 characters. Chunks contain base64 ASCII.
// A complete, ordered message is required before any configuration is applied.
class Chunks {
 public:
  std::string data;
  bool complete = false;
  bool accept(const std::string &packet, uint32_t now) {
    complete = false;
    if (packet.size() > 255) return false;
    auto a = packet.find('|'), b = packet.find('|', a + 1), c = packet.find('|', b + 1);
    if (a == std::string::npos || b == std::string::npos || c == std::string::npos || a == 0 || a > 24) return false;
    std::string token = packet.substr(0, a), number = packet.substr(a + 1, b - a - 1);
    if (number.empty() || number.size() > 2) return false;
    unsigned index = 0;
    for (char ch : number) { if (ch < '0' || ch > '9') return false; index = index * 10 + ch - '0'; }
    if (index > 31 || c != b + 2 || (packet[b+1] != '0' && packet[b+1] != '1')) return false;
    auto chunk = packet.substr(c + 1);
    if (chunk.empty() || chunk.size() > 200) return false;
    for (char ch : chunk) if (!(ch >= 'A' && ch <= 'Z') && !(ch >= 'a' && ch <= 'z') && !(ch >= '0' && ch <= '9') && ch != '+' && ch != '/' && ch != '=') return false;
    if (index == 0) { data.clear(); token_ = token; next_ = 0; }
    else if (now - last_ > 10000) { reset(); return false; }
    if (token != token_ || index != next_ || data.size() + chunk.size() > 5464) { reset(); return false; }
    data += chunk; next_++; last_ = now;
    complete = packet[b+1] == '1';
    if (complete) token_.clear();
    return true;
  }
  void reset() { data.clear(); token_.clear(); next_ = 0; complete = false; }
 private:
  std::string token_;
  unsigned next_ = 0;
  uint32_t last_ = 0;
};
}
