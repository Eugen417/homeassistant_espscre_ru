#pragma once
// The screensaver (firmware 0.29.0+, app 0.4.48): where its words go. Pure arithmetic, free of LVGL and ESPHome, so
// tests/test_saver_view.cpp checks every board's glass on a PC; runtime_tiles.h draws it.
//
// ESP Screen Manager decides what the screensaver shows (screen_manager/app/screen_saver.py) and the screen shows it
// when Auto standby dims it, instead of the dimmed tiles. A camera or a cover is one picture over the whole glass that
// the app made for it (camera_feed.encode_saver): a camera fills the glass; a cover fills it while the glass is about
// square, and on longer glass takes the full height at the left or the full width at the top, the rest in the cover's
// own colour. The whole picture is a little darker, so the few words over it read. The clock is the time with the date.
// Nothing on it takes a tap: the first touch wakes the screen, as it always did in standby.
#include <algorithm>
#include "media_card.h"

namespace saver_view {
using media_card::Rect;

// How a cover lies on glass of this size: the same rule as camera_feed.saver_shape (SAVER_SQUARE, five to four),
// guarded by tests/test_screen_saver.py.
enum class Shape { fill, side, top };
inline Shape shape(int width, int height) {
  if (width * 4 <= height * 5 && height * 4 <= width * 5) return Shape::fill;
  return width > height ? Shape::side : Shape::top;
}

// The title and the line under it. Over a picture that fills the glass they stand at the bottom left; beside or under
// a cover they stand in the cover's colour, in the middle of that room, left-aligned at its margin.
struct Words { Rect first, second; };
inline Words words(Shape s, int width, int height, int margin, int first_h, int second_h, int gap) {
  const int block = first_h + (second_h ? gap + second_h : 0);
  int x = margin, w = width - 2 * margin, y;
  if (s == Shape::side) {
    x = height + margin;
    w = width - height - 2 * margin;
    y = (height - block) / 2;
  } else if (s == Shape::top) {
    y = width + (height - width - block) / 2;
  } else {
    y = height - margin - block;
  }
  Words out;
  out.first = {x, y, std::max(1, w), first_h};
  if (second_h) out.second = {x, y + first_h + gap, std::max(1, w), second_h};
  return out;
}

// A title too long for one line takes two (firmware 0.30.0+), and the block grows upward with it: the line under it
// stays where it was. `text_h` is the height the title needs at the words' width, `line_h` one line of its font.
// Before, the label wrapped by itself and its second line lay over the line under it.
constexpr int TITLE_LINES = 2;
inline int title_lines(int text_h, int line_h) {
  if (line_h <= 0) return 1;
  return std::max(1, std::min(TITLE_LINES, (text_h + line_h / 2) / line_h));
}

// Whether words laid out that way stand whole in their room: on the glass, and under a cover at the top not over it.
// The smallest glass standing up has no room for a second line under its cover; the title then keeps one, with dots.
inline bool fits(Shape s, const Words &w, int width, int height) {
  const int bottom = w.second.h ? w.second.bottom() : w.first.bottom();
  return w.first.y >= (s == Shape::top ? width : 0) && bottom <= height;
}

// The clock: the time as large as the glass allows and the date under it, together in the middle. `digits_h` is the
// height of the digits the screen picked, `date_h` the date's line.
struct ClockLayout { Rect time, date; };
inline ClockLayout clock(int width, int height, int digits_h, int date_h, int gap) {
  const int group = digits_h + gap + date_h;
  const int y = (height - group) / 2;
  return {{0, y, width, digits_h}, {0, y + digits_h + gap, width, date_h}};
}
}  // namespace saver_view
