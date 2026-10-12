#include "screen_text_en.h"
#include "components/smart_display/screen_settings.h"
#include <cassert>
int main() {
  using screen_settings::Settings;
  Settings s;
  assert(s.valid() && s.standby_seconds == 600);
  assert(s.dim_level(-1) == 20); // No time sync must not accidentally select night.
  assert(s.dim_level(1319) == 20 && s.dim_level(1320) == 10);
  assert(s.dim_level(419) == 10 && s.dim_level(420) == 20);
  s.night_start = 60; s.night_end = 120;
  assert(!s.night(59) && s.night(60) && !s.night(120));
  s.night_end = 60; assert(!s.night(60) && !s.night(0));
  auto copy = s; assert(copy == s);
  copy.clock_24h = 0; assert(!(copy == s));
  s.brightness = 5; assert(!s.valid());
  s.standby_brightness = s.night_brightness = 0; assert(s.valid());
  s.standby_seconds = 86401; assert(!s.valid());
  s.standby_seconds = 60; s.show_clock = 2; assert(!s.valid());
  s.show_clock = 1; s.version = 2; assert(!s.valid());
}
