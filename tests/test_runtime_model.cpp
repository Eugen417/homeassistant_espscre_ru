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
  for (int i = 0; i < 11; ++i) many.push_back("light.a" + std::to_string(i));
  assert(!m.set_layout(many, "Te veel", changed)); assert(m.count == 0);
}
