#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cyd {
// Work in tenths of a degree and always clamp AFTER rounding.
inline int quantize_temperature(int value, int minimum, int maximum, int step) {
  step = std::max(1, step);
  value = std::clamp(value, minimum, maximum);
  const int snapped = minimum + static_cast<int>(std::lround(
      static_cast<double>(value - minimum) / step)) * step;
  return std::clamp(snapped, minimum, maximum);
}

class TouchGuard {
 public:
  void begin(uint32_t now, int x = 0, int y = 0) {
    started_ = now;
    accepted_ = false;
    moved_ = false;
    start_x_ = x;
    start_y_ = y;
  }
  void update(int x, int y) {
    // A contact that jumps to another target must never become a tile tap.
    if (std::abs(x - start_x_) > 18 || std::abs(y - start_y_) > 18) moved_ = true;
  }
  void consume() { accepted_ = true; moved_ = true; }
  bool accept(uint32_t now, int tile) {
    return !moved_ && accept_slider(now, tile);
  }
  // Only for a slider which captured this contact and did not lose the press.
  // Consume the gesture so its parent can never also turn it into a tile tap.
  bool accept_slider(uint32_t now, int tile) {
    if (accepted_ || now - started_ < 60) return false;
    if (has_previous_ && tile == previous_tile_ && now - previous_ < 600) return false;
    accepted_ = true;
    has_previous_ = true;
    previous_tile_ = tile;
    previous_ = now;
    return true;
  }
 private:
  uint32_t started_{0}, previous_{0};
  int previous_tile_{-1};
  int start_x_{0}, start_y_{0};
  bool moved_{false};
  bool accepted_{false}, has_previous_{false};
};
inline TouchGuard touch_guard;
}  // namespace cyd
