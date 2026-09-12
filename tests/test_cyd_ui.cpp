#include "../cyd_ui.h"
#include <cassert>
#include <limits>
int main() {
  using cyd::quantize_temperature;
  assert(quantize_temperature(223, 160, 300, 5) == 225);
  assert(quantize_temperature(221, 160, 300, 5) == 220);
  assert(quantize_temperature(-1, 160, 300, 5) == 160);
  assert(quantize_temperature(999, 160, 299, 5) == 299);
  assert(quantize_temperature(201, 160, 300, 0) == 201);
  cyd::TouchGuard g;
  g.begin(100);
  assert(!g.accept(120, 1)); // resistive noise pulse
  assert(g.accept(180, 1));
  assert(!g.accept(800, 1)); // same physical gesture cannot fire twice
  g.begin(200);
  assert(!g.accept(280, 1)); // contact bounce cannot repeat the action
  g.begin(300);
  assert(g.accept(380, 2)); // another tile is still responsive
  g.begin(1000);
  assert(g.accept(1080, 2));
  cyd::TouchGuard moving;
  moving.begin(100, 50, 50);
  moving.update(52, 48);
  assert(moving.accept(180, 1)); // small resistive jitter remains usable
  moving.begin(1000, 50, 50);
  moving.update(50, 80);
  moving.update(50, 50);
  assert(!moving.accept(1200, 2)); // excursion stays cancelled, even on return
  cyd::TouchGuard rollover;
  rollover.begin(std::numeric_limits<uint32_t>::max() - 30);
  assert(rollover.accept(50, 1)); // millis wraps after 49 days
  rollover.begin(60);
  assert(!rollover.accept(130, 1));
}
