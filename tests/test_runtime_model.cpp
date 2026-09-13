#include "../components/smart_display/runtime_model.h"
#include <cassert>
int main() {
  using namespace runtime_tiles;
  Model m; bool changed = false;
  assert(!m.ready());
  assert(!m.set_layout({"light.a", "light.a"}, "Thuis", changed));
  assert(!m.set_layout({"light.a;script.bad"}, "Thuis", changed));
  assert(!m.set_layout({"light."}, "Thuis", changed));
  assert(!m.set_layout({"lock.frontdoor"}, "Thuis", changed));
  assert(m.set_layout({"light.a", "climate.office", "vacuum.robot"}, "Thuis", changed));
  assert(changed && !m.ready());
  for (size_t i = 0; i < m.count; ++i) { m.tiles[i].received = true; m.tiles[i].state = "off"; }
  assert(m.ready());
  assert(m.set_layout({"light.a", "climate.office", "vacuum.robot"}, "Keuken", changed));
  assert(!changed && m.ready()); // periodic sync never interrupts an open card
  assert(!m.accepts(10, "light.a"));
  assert(!m.accepts(0, "light.removed"));
  assert(m.accepts(0, "light.a"));
  assert(m.set_layout({"vacuum.robot", "light.new"}, "Keuken", changed));
  assert(changed && !m.ready());
  assert(m.tiles[0].state.empty()); // never reuse the old slot's state/actions
  assert(!m.accepts(0, "light.a"));
  assert(m.set_layout({}, "Leeg", changed)); assert(changed && m.ready() && m.count == 0);
  std::vector<std::string> many;
  for (int i = 0; i < 21; ++i) many.push_back("light.a" + std::to_string(i));
  assert(!m.set_layout(many, "Te veel", changed)); assert(m.count == 0);
  many.pop_back(); assert(m.set_layout(many,"Twintig",changed));
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
  assert(m.set_layout({"screen.clock", "weather.home", "light.a", "sensor.b", "person.max", "timer.egg", "sun.sun"}, "Thuis", changed));
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
