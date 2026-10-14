#pragma once
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace esphome::xpt2046 {
// Optional affine calibration corrects skew as well as scale/offset.
// Identity by default; coefficients belong to an individual physical panel.
class RawCorrection {
 public:
  void set(float xx, float xy, float xc, float yx, float yy, float yc) {
    xx_ = xx; xy_ = xy; xc_ = xc; yx_ = yx; yy_ = yy; yc_ = yc;
  }
  int16_t x(int raw_x, int raw_y) const { return bound(xx_ * raw_x + xy_ * raw_y + xc_); }
  int16_t y(int raw_x, int raw_y) const { return bound(yx_ * raw_x + yy_ * raw_y + yc_); }
 private:
  static int16_t bound(float v) { return static_cast<int16_t>(std::clamp(std::lround(v), 0L, 4095L)); }
  float xx_{1}, xy_{0}, xc_{0}, yx_{0}, yy_{1}, yc_{0};
};

// XPT2046 ADC values can be wildly wrong as a finger first contacts/releases
// the resistive sheet. Do not expose a lone sample as a new LVGL target.
class TouchFilter {
 public:
  enum Result { HOLD, POSITION, RELEASE };
  Result sample(bool pressed, int16_t raw_x, int16_t raw_y) {
    if (!pressed) {
      candidate_count_ = 0;
      if (active_ && ++release_count_ < 2) return HOLD;
      active_ = false;
      release_count_ = 0;
      return RELEASE;
    }
    release_count_ = 0;
    if (active_ && near(raw_x, raw_y, x_, y_, 300)) {
      candidate_count_ = 0;
      x_ = (x_ + raw_x) / 2;
      y_ = (y_ + raw_y) / 2;
      return POSITION;
    }
    // Initial contact or a sudden jump: require three consistent samples.
    if (!candidate_count_ || !near(raw_x, raw_y, candidate_x_, candidate_y_, 100)) {
      candidate_x_ = raw_x;
      candidate_y_ = raw_y;
      candidate_count_ = 1;
      return HOLD;
    }
    candidate_x_ = (candidate_x_ * candidate_count_ + raw_x) / (candidate_count_ + 1);
    candidate_y_ = (candidate_y_ * candidate_count_ + raw_y) / (candidate_count_ + 1);
    if (++candidate_count_ < 3) return HOLD;
    x_ = candidate_x_;
    y_ = candidate_y_;
    active_ = true;
    candidate_count_ = 0;
    return POSITION;
  }
  int16_t x() const { return x_; }
  int16_t y() const { return y_; }
 private:
  static bool near(int x1, int y1, int x2, int y2, int distance) {
    return std::abs(x1 - x2) <= distance && std::abs(y1 - y2) <= distance;
  }
  int16_t x_{0}, y_{0}, candidate_x_{0}, candidate_y_{0};
  uint8_t candidate_count_{0}, release_count_{0};
  bool active_{false};
};
}  // namespace esphome::xpt2046
