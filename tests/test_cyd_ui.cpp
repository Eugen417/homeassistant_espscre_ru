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
  moving.update(51, 50); moving.update(50, 51); moving.update(50, 50);             // reference settled
  moving.update(50, 80);                                                           // 30 px beyond the default 18 px
  moving.update(50, 50);
  assert(!moving.accept(1200, 2)); // excursion stays cancelled, even on return
  assert(moving.accept_slider(1200, 202)); // captured drag is a valid slider gesture
  assert(!moving.accept(1201, 2)); // no parent action after slider release
  assert(!moving.accept_slider(1202, 202)); // one send per contact
  moving.begin(1250);
  assert(!moving.accept_slider(1260, 202)); // noise remains rejected
  assert(!moving.accept_slider(1350, 202)); // same-control bounce remains rejected
  // Per-board limits (0.2.23): a centimetre of drift on the Guition is still a tap, and the
  // reference settles over the first samples, so the landing wobble does not count.
  cyd::TouchGuard wide;
  wide.configure(67, 20);
  assert(wide.move_limit() == 67 && wide.min_press() == 20);
  wide.begin(100, 200, 200, 3);
  wide.update(206, 203, 3); wide.update(210, 205, 3); wide.update(212, 206, 3);   // finger flattens
  wide.update(262, 206, 3);                                                        // 55 px from the settled point
  assert(wide.distance() > 45 && wide.distance() <= 67);
  assert(wide.accept(180, 1)); // under one centimetre: still a tap
  wide.begin(1000, 200, 200, 3);
  wide.update(275, 200, 3); wide.update(300, 200, 3); wide.update(340, 200, 3);     // a real swipe: the reference
  wide.update(380, 200, 3);                                                        // settles, the finger keeps going
  assert(!wide.accept(1100, 1));
  assert(wide.reason().rfind("verplaatst", 0) == 0);
  wide.begin(2000, 200, 200, 3);
  wide.update(400, 400, 5);                                                        // a second finger elsewhere
  assert(wide.accept(2050, 1)); // ignored: only contact 3 is followed; 50 ms is long enough here
  wide.begin(3000, 200, 200, 3);
  assert(!wide.accept(3010, 2)); // 10 ms is below the capacitive minimum
  assert(wide.reason().rfind("te kort", 0) == 0);
  wide.begin(3100, 200, 200, 3);
  assert(wide.accept(3160, 2));
  assert(wide.reason().empty());
  assert(!wide.accept(3170, 2));
  assert(wide.reason() == "al verwerkt in dit contact");
  wide.begin(3200, 200, 200, 3);
  assert(!wide.accept(3260, 2)); // same tile within 600 ms
  assert(wide.reason() == "dezelfde knop binnen de dendertijd");
  cyd::TouchGuard resistive;
  resistive.configure(56, 60);
  resistive.begin(100, 50, 50);
  resistive.update(80, 50); resistive.update(50, 50);                              // the CYD's jump of 30 px is fine now
  assert(resistive.accept(180, 1));
  resistive.begin(1000, 50, 50);
  resistive.update(50, 120); resistive.update(50, 130); resistive.update(50, 140);
  resistive.update(50, 180);                                                       // 70 px past the settled point: a swipe
  assert(!resistive.accept(1100, 1));
  assert(!resistive.accept_repeat(1100, 1));
  cyd::TouchGuard rollover;
  rollover.begin(std::numeric_limits<uint32_t>::max() - 30);
  assert(rollover.accept(50, 1)); // millis wraps after 49 days
  rollover.begin(60);
  assert(!rollover.accept(130, 1));
}
