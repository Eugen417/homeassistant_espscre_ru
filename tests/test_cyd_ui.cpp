#include "../cyd_ui.h"
#include <cassert>
#include <limits>
int main() {
  using cyd::quantize_temperature;
  // Fan and swing mode chips read the n-th name of a JSON list attribute.
  assert(cyd::list_item("[\"auto\",\"low\",\"high\"]", 0) == "auto");
  assert(cyd::list_item("[\"auto\",\"low\",\"high\"]", 2) == "high");
  assert(cyd::list_item("[\"auto\",\"low\",\"high\"]", 3).empty());
  assert(cyd::list_item("", 0).empty());
  assert(cyd::list_item("[\"broken", 0).empty());
  assert(quantize_temperature(223, 160, 300, 5) == 225);
  assert(quantize_temperature(221, 160, 300, 5) == 220);
  assert(quantize_temperature(-1, 160, 300, 5) == 160);
  assert(quantize_temperature(999, 160, 299, 5) == 299);
  assert(quantize_temperature(201, 160, 300, 0) == 201);
  // -/+ keys: quick successive taps on the same key all count; bounce within 150 ms does not.
  cyd::TouchGuard r;
  r.begin(1000); assert(r.accept_repeat(1060, 7)); assert(!r.accept_repeat(1070, 7));  // one contact, one step
  r.begin(1200); assert(r.accept_repeat(1260, 7));                                      // 200 ms later: accepted
  r.begin(1300); assert(!r.accept_repeat(1340, 7));                                     // 80 ms after the last: bounce
  r.begin(1300); r.update(40, 0); assert(!r.accept_repeat(1500, 7));                   // a swipe is never a step
  r.begin(1600); assert(r.accept_repeat(1660, 8));                                      // the other key right away
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
  assert(moving.accept_slider(1200, 202)); // captured drag is a valid slider gesture
  assert(!moving.accept(1201, 2)); // no parent action after slider release
  assert(!moving.accept_slider(1202, 202)); // one send per contact
  moving.begin(1250);
  assert(!moving.accept_slider(1260, 202)); // noise remains rejected
  assert(!moving.accept_slider(1350, 202)); // same-control bounce remains rejected
  cyd::TouchGuard rollover;
  rollover.begin(std::numeric_limits<uint32_t>::max() - 30);
  assert(rollover.accept(50, 1)); // millis wraps after 49 days
  rollover.begin(60);
  assert(!rollover.accept(130, 1));
}
