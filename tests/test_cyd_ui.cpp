#include "screen_text_en.h"
#include "../components/smart_display/cyd_ui.h"
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
  // Page buttons (0.2.72): Next, Next, Next at a finger's pace all count, so page 4 is three taps away
  // while the pages are still drawing; accept() would have dropped the second and third (600 ms).
  cyd::TouchGuard pager;
  pager.begin(2000); assert(pager.accept_repeat(2080, 12));
  pager.begin(2250); assert(pager.accept_repeat(2330, 12));                              // 250 ms after the last
  pager.begin(2500); assert(pager.accept_repeat(2580, 12));                              // and again
  pager.begin(2800); assert(!pager.accept(2880, 12));                                    // 300 ms after the last Next: the old
  assert(pager.accept_repeat(2880, 12));                                                 // rule's 600 ms window drops it, this takes it
  pager.begin(2950); assert(pager.accept_repeat(3030, 11));                              // Previous right after Next
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
  assert(moving.accept_slider(1350, 202)); // a second drag 150 ms after the first commit counts (0.2.81)
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
  assert(wide.reason().rfind("moved", 0) == 0);
  wide.begin(2000, 200, 200, 3);
  wide.update(400, 400, 5);                                                        // a second finger elsewhere
  assert(wide.accept(2050, 1)); // ignored: only contact 3 is followed; 50 ms is long enough here
  wide.begin(3000, 200, 200, 3);
  assert(!wide.accept(3010, 2)); // 10 ms is below the capacitive minimum
  assert(wide.reason().rfind("too short", 0) == 0);
  wide.begin(3100, 200, 200, 3);
  assert(wide.accept(3160, 2));
  assert(wide.reason().empty());
  assert(!wide.accept(3170, 2));
  assert(wide.reason() == "already handled in this contact");
  wide.begin(3200, 200, 200, 3);
  assert(!wide.accept(3260, 2)); // same tile within 600 ms
  assert(wide.reason() == "same button within the debounce window");
  // No movement limit (0, the Guition): drift never drops a tap. The tile checks that the finger let go on it
  // (runtime_tiles::event, firmware 0.2.65+), and a quick flick is LVGL's gesture, which consumes the contact.
  cyd::TouchGuard free;
  free.configure(0, 20);
  free.begin(100, 200, 200, 3);
  free.update(205, 200, 3); free.update(205, 200, 3); free.update(205, 200, 3);       // settled
  free.update(400, 260, 3);
  assert(free.distance() > 200);
  assert(free.accept(180, 1));  // 200 px of drift, still a tap
  free.begin(1000, 200, 200, 3);
  free.update(205, 200, 3); free.update(205, 200, 3); free.update(205, 200, 3);
  free.update(400, 200, 3);
  assert(free.accept_repeat(1060, 7));  // -/+ keys follow the same rule
  free.begin(2000, 200, 200, 3);
  free.consume();                        // a flick (the Guition's gesture handler)
  assert(!free.accept(2100, 1) && !free.accept_repeat(2100, 7) && !free.accept_slider(2100, 201));
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
  // Edge swipe (0.2.24, back on the touchscreen triggers since 0.2.29): only a touch that
  // starts in the side band flips a page, from the right edge leftwards "next", from the left
  // edge rightwards "previous", once per touch, slow or fast, more sideways than vertical,
  // never from the middle. Rotation follows ESPHome's pointer mapping; end() disarms.
  cyd::EdgeSwipe edge;
  // A board that never configured one has no edge swipe at all: the CYD turns its pages by another gesture,
  // and shared touch handling must not flip a page there on the default band.
  cyd::EdgeSwipe unconfigured;
  assert(!unconfigured.in_use());
  unconfigured.begin(2, 100);
  assert(!unconfigured.armed());
  assert(unconfigured.update(200, 100) == 0);

  edge.configure(480, 480, 32, 40);
  assert(edge.in_use());
  edge.begin(6, 240);
  assert(edge.armed());
  assert(edge.update(30, 242) == 0);   // not far enough yet
  assert(edge.update(48, 245) == -1);  // 42 px inward from the left edge: previous page
  assert(edge.update(90, 245) == 0);   // once per touch
  assert(!edge.armed());
  edge.begin(474, 100);
  assert(edge.update(430, 104) == 1);  // from the right edge: next page
  edge.begin(240, 240);
  assert(!edge.armed());
  assert(edge.update(300, 240) == 0);  // started in the middle: never a page swipe
  edge.begin(6, 240);
  assert(edge.update(50, 300) == 0);   // steeper than 45 degrees: 44 px sideways against 60 down
  assert(edge.update(-10, 240) == 0);  // moving outward: nothing
  assert(edge.update(60, 270) == -1);  // 54 sideways against 30 down: a slanted thumb swipe counts
  edge.begin(6, 240);
  edge.update(20, 250);
  assert(edge.inward() == 14 && edge.sideways() == 10);  // what the log reports for a swipe that ended early
  edge.end();
  assert(!edge.armed());
  assert(edge.update(300, 250) == 0);  // the next touch's first update, before begin(): nothing
  edge.begin(240, 6, 90);
  assert(edge.update(240, 60) == -1);  // rotated 90: the user's left edge is the panel's top
  edge.begin(240, 6, 270);
  assert(edge.update(240, 60) == 1);   // rotated 270: that same edge is the user's right
  edge.begin(6, 240, 180);
  assert(edge.update(60, 240) == 1);   // upside down: the panel's left is the user's right
  cyd::TouchGuard rollover;
  rollover.begin(std::numeric_limits<uint32_t>::max() - 30);
  assert(rollover.accept(50, 1)); // millis wraps after 49 days
  rollover.begin(60);
  assert(!rollover.accept(130, 1));
  // GT911 stray (0, 0): measured on Studio 1 as the last sample of a drag before lift-off.
  cyd::GhostTouch ghost;
  auto same = [](cyd::PointerRead a, bool pressed, int x, int y) { return a.pressed == pressed && a.x == x && a.y == y; };
  assert(same(ghost.filter({false, 0, 0}, 0, 0), false, 0, 0));      // released: nothing to hide
  assert(same(ghost.filter({true, 0, 0}, 0, 0), false, 0, 0));       // a touch cannot start in the corner
  assert(same(ghost.filter({true, 302, 250}, 0, 0), true, 302, 250)); // the finger on blue
  assert(same(ghost.filter({true, 0, 0}, 0, 0), true, 302, 250));    // stray sample: still on blue
  assert(same(ghost.filter({true, 0, 0}, 0, 0), true, 302, 250));    // and again
  assert(same(ghost.filter({false, 302, 250}, 0, 0), false, 302, 250)); // released where the finger was
  assert(ghost.dropped() == 3);
  assert(same(ghost.filter({true, 0, 240}, 0, 0), true, 0, 240));    // the left edge itself is a real touch
  assert(same(ghost.filter({true, 479, 479}, 479, 479), true, 0, 240)); // rotated 180: (0, 0) is that corner
  assert(same(ghost.filter({true, 0, 0}, 479, 479), true, 0, 0));    // and the top-left is a real pixel there
  // A dragged slider sent to its end on release is reported; a drag that ends there is not a jump.
  assert(cyd::release_jump(240, 0, 0, 360));      // hue: blue to red at the start
  assert(cyd::release_jump(120, 360, 0, 360));    // green to red at the end
  assert(!cyd::release_jump(20, 0, 0, 360));      // dragged to the start and let go
  assert(!cyd::release_jump(240, 238, 0, 360));   // a few pixels on lift-off
  assert(cyd::release_jump(80, -6, -6, 100));     // brightness: 80 % to the stub below 1 %
  // A slider let go within the edge band of the glass meant the end that lies in that band.
  assert(cyd::edge_snap(437, 29, 450, 480, 67) == 1);   // wide tile, a fast swipe the panel lost at 437
  assert(cyd::edge_snap(431, 29, 450, 480, 67) == 1);
  assert(cyd::edge_snap(412, 29, 450, 480, 67) == 0);   // let go just before the band
  assert(cyd::edge_snap(437, 29, 222, 480, 67) == 0);   // a narrow tile's end is far from the edge
  assert(cyd::edge_snap(20, 29, 450, 480, 67) == -1);   // off the left: the start
  assert(cyd::edge_snap(20, 258, 452, 480, 67) == 0);   // the right column's start is not in the band
  assert(cyd::edge_snap(437, 29, 450, 480, 0) == 0);    // band off
  // A second drag on the same slider right after the first counts (a slider sends once per contact, on release);
  // a tap on the tile that soon after the slider's commit is still a bounce for the tile's own id only.
  cyd::TouchGuard drags;
  drags.begin(3000); assert(drags.accept_slider(3200, 205));
  drags.begin(3300); assert(drags.accept_slider(3500, 205));                             // 300 ms after the last commit
  drags.begin(3550); assert(drags.accept(3650, 105));                                    // the tile itself: another id
  drags.begin(3700); assert(!drags.accept(3800, 105));                                   // the tile twice within 600 ms
}
