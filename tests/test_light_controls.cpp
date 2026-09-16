#define LIGHT_CONTROLS_TEST
#include "../components/smart_display/light_controls.h"
#include <cassert>
int main() {
  using namespace light_controls;
  float value = -1;
  for (auto text : {"[120.5, 100]", "(120.5, 80.0)", "120.5"}) {
    assert(first_number(text, value)); assert(value == 120.5f);
  }
  for (auto text : {"None", "unknown", "unavailable", "[]", "nan", "inf", ""}) assert(!first_number(text, value));
  State a, b;
  a.update(0, "[239.6, 80]"); assert(a.hue == 240); assert(b.hue == 0);
  a.update(0, "None"); a.update(0, "[720, 100]"); assert(a.hue == 240);
  a.update(1, "2700"); a.update(1, "unavailable"); assert(a.kelvin == 2700);
  assert(!a.temperature_ready()); a.update(2, "2200"); assert(!a.temperature_ready());
  a.update(3, "5000"); assert(a.temperature_ready());
  assert(clamp(1800, a.minimum, a.maximum) == 2200);
  assert(clamp(6500, a.minimum, a.maximum) == 5000);
  a.update(3, "2000"); assert(!a.temperature_ready());
  a.update(3, "1e30"); assert(!a.temperature_ready());
  a.update(3, "5000"); assert(a.temperature_ready());
  assert(clamp(-1, 0, 360) == 0); assert(clamp(999, 0, 360) == 360);
}
