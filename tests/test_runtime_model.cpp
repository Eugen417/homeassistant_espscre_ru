#include "../components/smart_display/runtime_model.h"
#include <cassert>
int main() {
  using namespace runtime_tiles;
  Model m; bool changed = false;
  assert(!m.ready());
  assert(!m.set_layout({"light.a", "light.a"}, "Home", changed));
  assert(!m.set_layout({"light.a;script.bad"}, "Home", changed));
  assert(!m.set_layout({"light."}, "Home", changed));
  assert(!m.set_layout({"lock.frontdoor"}, "Home", changed));
  assert(m.set_layout({"light.a", "climate.office", "vacuum.robot"}, "Home", changed));
  assert(changed && !m.ready());
  for (size_t i = 0; i < m.count; ++i) { m.tiles[i].received = true; m.tiles[i].state = "off"; }
  assert(m.ready());
  assert(m.set_layout({"light.a", "climate.office", "vacuum.robot"}, "Kitchen", changed));
  assert(!changed && m.ready()); // periodic sync never interrupts an open card
  assert(!m.accepts(10, "light.a"));
  assert(!m.accepts(0, "light.removed"));
  assert(m.accepts(0, "light.a"));
  assert(m.set_layout({"vacuum.robot", "light.new"}, "Kitchen", changed));
  assert(changed && !m.ready());
  assert(m.tiles[0].state.empty()); // never reuse the old slot's state/actions
  assert(!m.accepts(0, "light.a"));
  assert(m.set_layout({}, "Empty", changed)); assert(changed && m.ready() && m.count == 0);
  std::vector<std::string> many;
  for (int i = 0; i < 21; ++i) many.push_back("light.a" + std::to_string(i));
  assert(!m.set_layout(many, "Too many", changed)); assert(m.count == 0);
  many.pop_back(); assert(m.set_layout(many,"Twenty",changed));
  assert(m.count==20 && m.accepts(19,"light.a19"));
  for(size_t i=0;i<m.count;++i){m.tiles[i].received=true;m.tiles[i].state="off";}
  assert(m.ready());
}
// Built-in and new Home Assistant domains, plus wide-tile packing.
static void test_domains_and_packing() {
  using namespace runtime_tiles;
  Model m; bool changed = false;
  assert(valid_entity("screen.clock") && !valid_entity("screen.other"));
  assert(valid_entity("sun.sun") && valid_entity("timer.kitchen") && valid_entity("person.max"));
  assert(m.set_layout({"screen.clock", "weather.home", "light.a", "sensor.b", "person.max", "timer.egg", "sun.sun"}, "Home", changed));
  assert(m.tiles[0].available() && !m.tiles[1].available());  // built-in cards need no HA state
  m.tiles[4].state = "home"; assert(m.tiles[4].active());
  m.tiles[5].state = "active"; assert(m.tiles[5].active());
  m.tiles[5].state = "idle"; assert(!m.tiles[5].active());
  std::array<Placement, MAX_TILES> p;
  assert(pack(m.tiles, 0, p) == 1);
  assert(pack(m.tiles, m.count, p) == 2 && p[6].page == 1 && p[6].slot == 0);
  // A wide tile after a left-column tile skips the right column.
  m.tiles[1].wide = true;
  assert(pack(m.tiles, m.count, p) == 2);
  assert(p[0].slot == 0 && p[1].slot == 2 && p[2].slot == 4 && p[3].slot == 5);
  assert(p[4].page == 1 && p[4].slot == 0 && p[6].page == 1 && p[6].slot == 2);
  // Wide tiles ending exactly on a page boundary do not open an empty page.
  for (auto &t : m.tiles) t.wide = false;
  m.tiles[0].wide = m.tiles[2].wide = true;
  assert(pack(m.tiles, 3, p) == 1 && p[0].slot == 0 && p[1].slot == 2 && p[2].slot == 4);
  m.tiles[3].wide = true;
  assert(pack(m.tiles, 4, p) == 2 && p[3].page == 1 && p[3].slot == 0);
}
struct RunExtra { RunExtra() { test_domains_and_packing(); } } run_extra;
// Explicit grid positions (0.2.26+): gaps stay empty, a wide card starts in the left column.
static void test_explicit_slots() {
  using namespace runtime_tiles;
  Model m; bool changed = false, moved = false;
  std::array<Placement, MAX_TILES> p;
  // Validation: one slot per entity, inside eight pages, no duplicates.
  assert(!m.set_layout({"light.a", "light.b"}, "Home", changed, {0}, moved));
  assert(!m.set_layout({"light.a", "light.b"}, "Home", changed, {0, 48}, moved));
  assert(!m.set_layout({"light.a", "light.b"}, "Home", changed, {3, 3}, moved));
  assert(!m.configured);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 5, 7}, moved));
  assert(changed && !moved && m.explicit_slots);
  assert(place(m, p) == 2 && p[0].page == 0 && p[0].slot == 0 && p[1].slot == 5 && p[2].page == 1 && p[2].slot == 1);
  // Same tiles elsewhere: states survive, only the pages re-place.
  for (size_t i = 0; i < m.count; ++i) { m.tiles[i].received = true; m.tiles[i].state = "on"; }
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 7}, moved));
  assert(!changed && moved && m.ready() && m.tiles[0].state == "on");
  assert(place(m, p) == 2 && p[0].slot == 2);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 7}, moved));
  assert(!changed && !moved);
  // A wide card on an odd slot snaps to its row start; the page count follows its footprint.
  m.tiles[2].wide = true;
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 11}, moved));
  assert(!changed && moved && place(m, p) == 2 && p[2].page == 1 && p[2].slot == 4);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 12}, moved));
  assert(place(m, p) == 3 && p[2].page == 2 && p[2].slot == 0);
  // An empty page between two used pages stays a page; the last slot of page eight is allowed.
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 1, 46}, moved));
  assert(place(m, p) == 8 && p[2].page == 7 && p[2].slot == 4);
  // Pages kept on purpose extend the count, never shrink it.
  m.pages = 3; assert(place(m, p) == 8);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 1, 2}, moved));
  assert(place(m, p) == 3); m.pages = 1; assert(place(m, p) == 1); m.pages = 200; assert(place(m, p) == MAX_PAGES); m.pages = 1;
  // Without slots (older manager) the in-order packing returns.
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {}, moved));
  assert(!changed && moved && !m.explicit_slots);
  assert(place(m, p) == 1 && p[0].slot == 0 && p[1].slot == 1 && p[2].slot == 2);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {}, moved));
  assert(!changed && !moved);
  // Changing the tiles resets everything, including the positions.
  assert(m.set_layout({"light.a", "light.z"}, "Home", changed, {1, 6}, moved));
  assert(changed && !moved && !m.ready() && place(m, p) == 2 && p[0].slot == 1 && p[1].page == 1);
}
struct RunSlots { RunSlots() { test_explicit_slots(); } } run_slots;
