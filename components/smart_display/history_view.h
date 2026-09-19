#pragma once
// History on a detail card (firmware 0.2.51+): the pure parts. What a time, a duration or a value reads as, the
// smooth line through the averages, and which part of the line or which run of the timeline lies under a finger.
// The drawing is in runtime_tiles.h; tests/test_history_view.cpp covers this.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "screen_text.h"

namespace history_view {
constexpr int PARTS = 24;

// A run of the timeline: from `slot` to the next run's slot it shows `state` (an index into the legend, -1 without
// data). `begin`, `end` (seconds after the start of the range) and `seconds` are the real times of that state in it.
struct Run {
  uint16_t slot = 0;
  int8_t state = -1;
  uint32_t begin = 0, end = 0, seconds = 0;
};

// A clock time of a unix time in the screen's time zone (`offset` seconds east of UTC), as the screen's 12- or
// 24-hour setting writes it; `weekday` puts the day in front ("Tue 14:05") for a week, in the screen's language.
inline std::string clock(int64_t epoch, int32_t offset, bool h24, bool weekday) {
  using namespace screen_text;
  int64_t local = epoch + offset;
  int64_t days = local >= 0 ? local / 86400 : (local - 86399) / 86400;
  int seconds = static_cast<int>(local - days * 86400);
  int hour = seconds / 3600, minute = seconds % 3600 / 60;
  char b[24];
  if (h24) snprintf(b, sizeof(b), "%02d:%02d", hour, minute);
  else snprintf(b, sizeof(b), "%d:%02d %s", hour % 12 ? hour % 12 : 12, minute, tr(hour < 12 ? txt::time_am : txt::time_pm));
  if (!weekday) return b;
  // Day 0 of the unix epoch was a Thursday; the names count from Sunday.
  int sunday_first = static_cast<int>((((days + 4) % 7) + 7) % 7);
  return fill(fill(txt::date_weekday_time, "weekday", tr(txt::date_weekdays_short + sunday_first)), "time", b);
}

// A time under a graph: "06:00" on a 24-hour clock, "6 AM" or "6:15 AM" on a 12-hour one.
inline std::string axis_clock(int64_t epoch, int32_t offset, bool h24) {
  std::string text = clock(epoch, offset, h24, false);
  if (h24) return text;
  auto colon = text.find(':');
  if (colon != std::string::npos && text.compare(colon, 3, ":00") == 0) text.erase(colon, 3);
  return text;
}

// "45 s", "12 min", "3 h 20 min", "2 d 4 h": the time spent in a state, in the screen's language (screen.duration).
inline std::string duration(uint32_t seconds) {
  using namespace screen_text;
  const int s = static_cast<int>(seconds);
  if (s < 60) return fill(txt::duration_seconds, "n", s);
  if (s < 3600) return fill(txt::duration_minutes, "n", s / 60);
  if (s < 86400) {
    int minutes = s / 60 % 60;
    if (!minutes) return fill(txt::duration_hours, "n", s / 3600);
    return fill(fill(txt::duration_hours_minutes, "h", s / 3600), "m", std::to_string(minutes));
  }
  int hours = s / 3600 % 24;
  if (!hours) return fill(txt::duration_days, "n", s / 86400);
  return fill(fill(txt::duration_days_hours, "d", s / 86400), "h", std::to_string(hours));
}

// A value as Home Assistant writes it: fixed decimals, the language's separator between thousands and its decimal
// mark (screen.number: "1,234.5" in English, "1.234,5" in Dutch), "%" and "°" on the number and any other unit after
// a space.
inline std::string number(float value, int decimals, const std::string &unit) {
  if (!std::isfinite(value)) return "--";
  char b[40];
  snprintf(b, sizeof(b), "%.*f", std::clamp(decimals, 0, 4), value);
  std::string text = b, sign;
  if (text[0] == '-') { sign = "-"; text.erase(0, 1); }
  auto dot = text.find('.');
  std::string whole = text.substr(0, dot), rest = dot == std::string::npos ? "" : text.substr(dot);
  bool zero = whole.find_first_not_of('0') == std::string::npos && rest.find_first_not_of(".0") == std::string::npos;
  const std::string group = screen_text::group_mark();
  for (int i = static_cast<int>(whole.size()) - 3; i > 0; i -= 3) whole.insert(static_cast<size_t>(i), group);
  if (!rest.empty()) rest[0] = screen_text::decimal_mark();
  text = (zero ? "" : sign) + whole + rest;
  if (unit.empty()) return text;
  if (unit == "%" || unit == "°") return text + unit;
  return text + " " + unit;
}

// Where part `i` of 24 is drawn and read: the middle of its stretch of the range, as a fraction of the width.
inline float part_middle(int i) { return (static_cast<float>(i) + 0.5f) / PARTS; }

// The part under a finger at `fraction` of the width. It can be a part without a value (a sensor that did not
// exist yet, a time it was unavailable): the card then says so instead of reading a value from another hour.
inline int part_at(float fraction) {
  return std::clamp(static_cast<int>(std::floor(std::clamp(fraction, 0.0f, 1.0f) * PARTS)), 0, PARTS - 1);
}

// The run of the timeline under a finger: its index into `runs`, or -1.
inline int run_at(const std::vector<Run> &runs, uint16_t slots, float fraction) {
  if (runs.empty() || !slots) return -1;
  int slot = std::clamp(static_cast<int>(std::floor(std::clamp(fraction, 0.0f, 1.0f) * slots)), 0, slots - 1);
  int found = -1;
  for (size_t i = 0; i < runs.size(); ++i) {
    if (runs[i].slot <= slot) found = static_cast<int>(i);
    else break;
  }
  return found;
}

// The slot where run `i` ends (the next run's start, or the end of the timeline).
inline uint16_t run_end(const std::vector<Run> &runs, uint16_t slots, size_t i) {
  return i + 1 < runs.size() ? runs[i + 1].slot : slots;
}

// Puts a moment (x, y) on a line through points with rising x: the nearest point within `snap` (the same part of
// the range) takes its y, unless it is `keep` (a point that already holds a moment); otherwise the moment is added
// in its place. Returns the moment's index, or -1 when the points are full.
inline int through(float *xs, float *ys, unsigned &n, unsigned capacity, float x, float y, float snap, int keep = -1) {
  int nearest = -1;
  for (unsigned i = 0; i < n; ++i) {
    const float distance = std::fabs(xs[i] - x);
    if (static_cast<int>(i) != keep && distance <= snap && (nearest < 0 || distance < std::fabs(xs[nearest] - x))) nearest = static_cast<int>(i);
  }
  if (nearest >= 0) {
    ys[nearest] = y;
    return nearest;
  }
  if (n >= capacity) return -1;
  unsigned at = n;
  while (at > 0 && xs[at - 1] > x) {
    xs[at] = xs[at - 1];
    ys[at] = ys[at - 1];
    --at;
  }
  xs[at] = x;
  ys[at] = y;
  ++n;
  return static_cast<int>(at);
}

// A smooth line through points with rising x that never overshoots them (monotone cubic, Fritsch-Carlson): between
// two points it stays between their values, so a step in a setpoint does not dip below its lowest value and the
// highest and lowest moment stay on the line. Writes `steps` points per stretch and the last point into `out`
// (anything with x and y), at most `capacity`; returns how many.
template<typename Point>
inline unsigned monotone(const float *xs, const float *ys, unsigned n, unsigned steps, Point *out, unsigned capacity) {
  using Value = decltype(out[0].x);
  auto put = [&](unsigned at, float x, float y) {
    out[at].x = static_cast<Value>(std::lround(x));
    out[at].y = static_cast<Value>(std::lround(y));
  };
  if (!capacity) return 0;
  if (n < 2 || steps == 0) {
    unsigned count = std::min(n, capacity);
    for (unsigned i = 0; i < count; ++i) put(i, xs[i], ys[i]);
    return count;
  }
  constexpr unsigned MOST = 64;
  n = std::min(n, MOST);
  float slope[MOST], tangent[MOST];
  for (unsigned i = 0; i + 1 < n; ++i) {
    const float dx = xs[i + 1] - xs[i];
    slope[i] = dx > 0 ? (ys[i + 1] - ys[i]) / dx : 0.0f;
  }
  tangent[0] = slope[0];
  tangent[n - 1] = slope[n - 2];
  for (unsigned i = 1; i + 1 < n; ++i)
    tangent[i] = slope[i - 1] * slope[i] <= 0 ? 0.0f : (slope[i - 1] + slope[i]) / 2;
  for (unsigned i = 0; i + 1 < n; ++i) {
    if (slope[i] == 0) { tangent[i] = tangent[i + 1] = 0; continue; }
    const float a = tangent[i] / slope[i], b = tangent[i + 1] / slope[i], sum = a * a + b * b;
    if (sum > 9) {
      const float scale = 3 / std::sqrt(sum);
      tangent[i] = scale * a * slope[i];
      tangent[i + 1] = scale * b * slope[i];
    }
  }
  unsigned count = 0;
  for (unsigned i = 0; i + 1 < n && count + 1 < capacity; ++i) {
    const float h = xs[i + 1] - xs[i];
    for (unsigned s = 0; s < steps && count + 1 < capacity; ++s) {
      const float t = static_cast<float>(s) / steps, t2 = t * t, t3 = t2 * t;
      const float y = (2 * t3 - 3 * t2 + 1) * ys[i] + (t3 - 2 * t2 + t) * h * tangent[i] + (-2 * t3 + 3 * t2) * ys[i + 1] + (t3 - t2) * h * tangent[i + 1];
      put(count++, xs[i] + t * h, y);
    }
  }
  put(count++, xs[n - 1], ys[n - 1]);
  return count;
}
}  // namespace history_view
