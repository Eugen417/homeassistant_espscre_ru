#include "screen_text_en.h"
// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_history_view.cpp -o /tmp/test_history_view && /tmp/test_history_view
#include "../components/smart_display/history_view.h"
#include <cassert>
#include <cstring>

using namespace history_view;

int main() {
  // 2026-09-16 14:40 UTC is 16:40 in Amsterdam (UTC+2), a Wednesday.
  const int64_t moment = 1789569600;
  assert(clock(moment, 7200, true, false) == "16:40");
  assert(clock(moment, 7200, false, false) == "4:40 PM");
  assert(clock(moment, 7200, true, true) == "Wed 16:40");
  assert(clock(moment - 16 * 3600 - 40 * 60, 7200, true, true) == "Wed 00:00");
  assert(clock(moment - 17 * 3600, 7200, false, true) == "Tue 11:40 PM");
  assert(clock(0, 0, true, true) == "Thu 00:00");
  assert(clock(-60, 0, false, true) == "Wed 11:59 PM");
  assert(clock(moment, -18000, true, false) == "09:40");
  assert(axis_clock(moment - 40 * 60, 7200, true) == "16:00");
  assert(axis_clock(moment - 40 * 60, 7200, false) == "4 PM");
  assert(axis_clock(moment - 25 * 60, 7200, false) == "4:15 PM");
  assert(axis_clock(moment - 16 * 3600 - 40 * 60, 7200, false) == "12 AM");

  assert(duration(42) == "42 s");
  assert(duration(900) == "15 min");
  assert(duration(3600) == "1 h");
  assert(duration(12000) == "3 h 20 min");
  assert(duration(86400 * 2 + 3600 * 4) == "2 d 4 h");
  assert(duration(86400 * 7) == "7 d");

  assert(number(21.43f, 1, "°C") == "21.4 °C");
  assert(number(40, 0, "%") == "40%");
  assert(number(1234.5f, 1, "W") == "1,234.5 W");
  assert(number(-2.5f, 1, "°") == "-2.5°");
  assert(number(-0.04f, 1, "°C") == "0.0 °C");
  assert(number(1234567, 0, "") == "1,234,567");
  assert(number(NAN, 1, "W") == "--");

  assert(part_at(0.0f) == 0 && part_at(-0.2f) == 0);
  assert(part_at(0.5f) == 12 && part_at(0.49f) == 11);
  assert(part_at(0.999f) == 23 && part_at(1.0f) == 23 && part_at(1.5f) == 23);
  assert(std::fabs(part_middle(0) - 1.0f / 48) < 1e-6f && std::fabs(part_middle(23) - 47.0f / 48) < 1e-6f);

  std::vector<Run> runs;
  for (auto [slot, state] : std::vector<std::pair<int, int>>{{0, 0}, {4, 1}, {5, 0}, {40, 1}, {41, 0}}) {
    Run run;
    run.slot = static_cast<uint16_t>(slot);
    run.state = static_cast<int8_t>(state);
    runs.push_back(run);
  }
  assert(run_at(runs, 96, 0.0f) == 0);
  assert(run_at(runs, 96, 4.5f / 96) == 1);
  assert(run_at(runs, 96, 0.3f) == 2);
  assert(run_at(runs, 96, 1.0f) == 4);
  assert(run_end(runs, 96, 1) == 5 && run_end(runs, 96, 4) == 96);
  assert(run_at({}, 96, 0.5f) == -1);

  // The line never leaves the values it passes: a setpoint that steps from 55 to 45 and up to 60 stays within.
  struct P { int32_t x, y; };
  const float xs[] = {0, 10, 20, 30, 40, 50, 60}, ys[] = {55, 55, 55, 45, 45, 60, 60};
  P line[64];
  const unsigned count = monotone(xs, ys, 7, 4, line, 64);
  assert(count == 6 * 4 + 1);
  for (unsigned i = 0; i < count; ++i) assert(line[i].y >= 45 && line[i].y <= 60);
  for (unsigned i = 0; i + 1 < count; ++i) assert(line[i].x <= line[i + 1].x);
  assert(line[0].x == 0 && line[0].y == 55 && line[count - 1].x == 60 && line[count - 1].y == 60);
  // Flat stretches stay flat, and every given point is on the line.
  for (unsigned i = 0; i <= 4; ++i) assert(line[i].y == 55);
  for (unsigned i = 0; i < 7; ++i) assert(line[i * 4 < count ? i * 4 : count - 1].y == static_cast<int32_t>(ys[i]));
  // A small buffer is filled, not overrun, and ends on the last point.
  P small[5];
  assert(monotone(xs, ys, 7, 4, small, 5) == 5 && small[4].x == 60);
  // The highest moment takes the place of the nearest point in its part, so the line's peak is where its ring is.
  float px[8] = {0, 10, 20, 30}, py[8] = {5, 6, 7, 6};
  unsigned points = 4;
  const int high = through(px, py, points, 8, 22, 9, 5);
  assert(high == 2 && points == 4 && px[2] == 20 && py[2] == 9);
  // The lowest moment in the same part keeps the highest: it joins the line beside it.
  const int low = through(px, py, points, 8, 18, 1, 5, high);
  assert(low == 2 && points == 5 && px[2] == 18 && py[2] == 1 && px[3] == 20 && py[3] == 9);
  // Far from every point (a part without values) it is added.
  float gx[8] = {0, 10, 40}, gy[8] = {5, 5, 5};
  unsigned gaps = 3;
  assert(through(gx, gy, gaps, 8, 25, 8, 5) == 2 && gaps == 4 && gx[3] == 40);
  P peak[64];
  const unsigned drawn = monotone(px, py, points, 4, peak, 64);
  int top = 0, bottom = 100;
  for (unsigned i = 0; i < drawn; ++i) {
    top = std::max(top, peak[i].y);
    bottom = std::min(bottom, peak[i].y);
  }
  assert(top == 9 && bottom == 1);
  float full_x[2] = {0, 1}, full_y[2] = {0, 1};
  unsigned full = 2;
  assert(through(full_x, full_y, full, 2, 50, 3, 5) == -1 && full == 2);
  // One point, or none.
  assert(monotone(xs, ys, 1, 4, line, 64) == 1 && monotone(xs, ys, 0, 4, line, 64) == 0);
  return 0;
}
