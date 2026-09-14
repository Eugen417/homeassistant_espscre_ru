#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace cyd {
// The n-th quoted item of a JSON list such as ["auto","low","high"]; empty when absent.
inline std::string list_item(const std::string &json, unsigned index) {
  size_t pos = 0; unsigned n = 0;
  while ((pos = json.find('"', pos)) != std::string::npos) {
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    if (n++ == index) return json.substr(pos + 1, end - pos - 1);
    pos = end + 1;
  }
  return {};
}
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
  enum Reject { NONE, MOVED, TOO_SHORT, USED, BOUNCE };
  // Board tuning, set from the profile on boot: how far the finger may drift (pixels, about
  // one centimetre) before a tap is dropped, and the shortest contact that counts. A resistive
  // panel (CYD) bounces on landing and lift-off; a capacitive one (Guition) does not.
  void configure(int move_limit_px, uint32_t min_press_ms) {
    move_limit_ = move_limit_px;
    min_press_ = min_press_ms;
  }
  int move_limit() const { return move_limit_; }
  uint32_t min_press() const { return min_press_; }
  void begin(uint32_t now, int x = 0, int y = 0, int contact = 0) {
    started_ = now;
    accepted_ = false;
    moved_ = false;
    contact_ = contact;
    samples_ = 1;
    sum_x_ = anchor_x_ = x;
    sum_y_ = anchor_y_ = y;
    distance_ = 0;
    reject_ = NONE;
  }
  int contact() const { return contact_; }
  // Follows only the contact that started this touch: a second finger elsewhere is no movement.
  // The reference point settles over the first samples, because a finger flattens as it lands;
  // a tap is dropped once the finger is farther than the limit from that point.
  void update(int x, int y, int contact = 0) {
    if (contact != contact_) return;
    if (samples_ < SETTLE_SAMPLES) {
      ++samples_;
      sum_x_ += x;
      sum_y_ += y;
      anchor_x_ = sum_x_ / samples_;
      anchor_y_ = sum_y_ / samples_;
    }
    const long dx = x - anchor_x_, dy = y - anchor_y_;
    const int distance = static_cast<int>(std::lround(std::sqrt(static_cast<double>(dx * dx + dy * dy))));
    distance_ = std::max(distance_, distance);
    if (distance > move_limit_) moved_ = true;
  }
  int distance() const { return distance_; }
  void consume() { accepted_ = true; moved_ = true; }
  bool accept(uint32_t now, int tile) {
    if (moved_) { reject_ = MOVED; return false; }
    return accept_slider(now, tile);
  }
  // For -/+ keys: every clean tap counts, even the third within a second, so a
  // setpoint moves several steps in one go. Only bounce (same key within `gap`) is dropped.
  bool accept_repeat(uint32_t now, int tile, uint32_t gap = 150) {
    if (moved_) { reject_ = MOVED; return false; }
    if (accepted_) { reject_ = USED; return false; }
    if (!long_enough(now, std::min<uint32_t>(min_press_, 40))) return false;
    if (has_previous_ && tile == previous_tile_ && now - previous_ < gap) { reject_ = BOUNCE; return false; }
    remember(now, tile);
    return true;
  }
  // Only for a slider which captured this contact and did not lose the press.
  // Consume the gesture so its parent can never also turn it into a tile tap.
  bool accept_slider(uint32_t now, int tile) {
    if (accepted_) { reject_ = USED; return false; }
    if (!long_enough(now, min_press_)) return false;
    if (has_previous_ && tile == previous_tile_ && now - previous_ < 600) { reject_ = BOUNCE; return false; }
    remember(now, tile);
    return true;
  }
  // Why the last accept() refused, for the touch log; empty after a success.
  std::string reason() const {
    switch (reject_) {
      case MOVED: return "verplaatst " + std::to_string(distance_) + " px (grens " + std::to_string(move_limit_) + ")";
      case TOO_SHORT: return "te kort (" + std::to_string(duration_) + " ms, minimaal " + std::to_string(min_press_) + ")";
      case USED: return "al verwerkt in dit contact";
      case BOUNCE: return "dezelfde knop binnen de dendertijd";
      default: return "";
    }
  }
 private:
  static constexpr int SETTLE_SAMPLES = 4;
  bool long_enough(uint32_t now, uint32_t minimum) {
    duration_ = now - started_;
    if (duration_ < minimum) { reject_ = TOO_SHORT; return false; }
    return true;
  }
  void remember(uint32_t now, int tile) {
    accepted_ = true;
    has_previous_ = true;
    previous_tile_ = tile;
    previous_ = now;
    reject_ = NONE;
  }
  int move_limit_{18};
  uint32_t min_press_{60};
  uint32_t started_{0}, previous_{0}, duration_{0};
  int previous_tile_{-1};
  int contact_{0}, samples_{1};
  long sum_x_{0}, sum_y_{0};
  int anchor_x_{0}, anchor_y_{0}, distance_{0};
  bool moved_{false};
  bool accepted_{false}, has_previous_{false};
  Reject reject_{NONE};
};
inline TouchGuard touch_guard;
}  // namespace cyd
