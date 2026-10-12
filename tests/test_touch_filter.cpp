#include "../components/xpt2046/touchscreen/touch_filter.h"
#include <cassert>
int main() {
  using F = esphome::xpt2046::TouchFilter;
  esphome::xpt2046::RawCorrection correction;
  assert(correction.x(100, 200) == 100 && correction.y(100, 200) == 200);
  correction.set(1.073915801f, 0.065912977f, -345.080450614f, 0.044372873f, 0.983997898f, -69.394322384f);
  const int corners[][4] = {{830,556,20,20}, {544,3461,299,20},
                            {3410,3391,299,219}, {3492,372,20,219}};
  for (const auto &p : corners) {
    const double sx = (correction.y(p[0], p[1]) - 274) * 320.0 / 3335;
    const double sy = (correction.x(p[0], p[1]) - 225) * 240.0 / 3575;
    assert(std::abs(sx - p[2]) < 5 && std::abs(sy - p[3]) < 5);
  }
  assert(correction.x(-30000, -30000) == 0);
  assert(correction.y(30000, 30000) == 4095);
  F f;
  // Measured bad top-left first samples must not reach LVGL immediately.
  assert(f.sample(true, 2067, 1873) == F::HOLD);
  assert(f.sample(true, 535, 500) == F::HOLD);
  assert(f.sample(true, 550, 510) == F::HOLD);
  assert(f.sample(true, 530, 495) == F::POSITION);
  assert(f.x() >= 530 && f.x() <= 550);
  assert(f.y() >= 495 && f.y() <= 510);
  // A far-away sample cannot move the pointer to another button.
  assert(f.sample(true, 3000, 3000) == F::HOLD);
  assert(f.sample(true, 540, 500) == F::POSITION);
  assert(f.x() < 600);
  // One dropped pressure sample doesn't create a second press/release.
  assert(f.sample(false, 0, 0) == F::HOLD);
  assert(f.sample(true, 550, 500) == F::POSITION);
  // A deliberate drag still works, including a large consistent movement.
  assert(f.sample(true, 700, 650) == F::POSITION);
  assert(f.sample(true, 2000, 2000) == F::HOLD);
  assert(f.sample(true, 2010, 2005) == F::HOLD);
  assert(f.sample(true, 1990, 2000) == F::POSITION);
  assert(f.x() > 1900);
  assert(f.sample(false, 0, 0) == F::HOLD);
  assert(f.sample(false, 0, 0) == F::RELEASE);
  // A lone ghost pulse must never start a gesture.
  assert(f.sample(true, 3900, 100) == F::HOLD);
  assert(f.sample(false, 0, 0) == F::RELEASE);
}
