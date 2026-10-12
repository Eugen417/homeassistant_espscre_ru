#pragma once
#include <cstdint>

namespace screen_settings {
// Stable preference format. Change its version/key only with a migration.
struct Settings {
  uint32_t version = 1;
  int32_t standby_enabled = 1, standby_seconds = 600, brightness = 100;
  int32_t standby_brightness = 20, night_enabled = 1, night_start = 1320;
  int32_t night_end = 420, night_brightness = 10, show_clock = 1;
  int32_t clock_24h = 1, home_on_standby = 0;
  bool valid() const {
    return version == 1 && (standby_enabled == 0 || standby_enabled == 1) &&
      standby_seconds >= 60 && standby_seconds <= 86400 &&
      brightness >= 5 && brightness <= 100 && standby_brightness >= 0 &&
      standby_brightness <= brightness && (night_enabled == 0 || night_enabled == 1) &&
      night_start >= 0 && night_start < 1440 && night_end >= 0 && night_end < 1440 &&
      night_brightness >= 0 && night_brightness <= brightness &&
      (show_clock == 0 || show_clock == 1) && (clock_24h == 0 || clock_24h == 1) &&
      (home_on_standby == 0 || home_on_standby == 1);
  }
  bool operator==(const Settings &b) const {
    return version == b.version && standby_enabled == b.standby_enabled &&
      standby_seconds == b.standby_seconds && brightness == b.brightness &&
      standby_brightness == b.standby_brightness && night_enabled == b.night_enabled &&
      night_start == b.night_start && night_end == b.night_end &&
      night_brightness == b.night_brightness && show_clock == b.show_clock &&
      clock_24h == b.clock_24h && home_on_standby == b.home_on_standby;
  }
  bool night(int minute) const {
    if (!night_enabled || minute < 0 || minute >= 1440 || night_start == night_end) return false;
    return night_start < night_end ? minute >= night_start && minute < night_end
                                  : minute >= night_start || minute < night_end;
  }
  int dim_level(int minute) const { return night(minute) ? night_brightness : standby_brightness; }
};
inline Settings current;
}
