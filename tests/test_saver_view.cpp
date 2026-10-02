#include "screen_text_en.h"
// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_saver_view.cpp -o /tmp/test_saver_view && /tmp/test_saver_view
#include "../components/smart_display/saver_view.h"
#include <cassert>
#include <cstdio>

using namespace saver_view;

static bool inside(const Rect &r, int width, int height) { return r.x >= 0 && r.y >= 0 && r.right() <= width && r.bottom() <= height; }

// Every glass a board has, lying and standing: the CYD's 320x240 to the ten-inch Guition's 1280x800.
static const int GLASS[][2] = {{320, 240}, {240, 320}, {480, 320}, {320, 480}, {480, 480}, {720, 720}, {800, 480},
                               {480, 800}, {1024, 600}, {600, 1024}, {1280, 800}, {800, 1280}};

static void words_on_every_glass() {
  for (const auto &g : GLASS) {
    const int width = g[0], height = g[1];
    const Shape s = shape(width, height);
    const Words w = words(s, width, height, 24, 32, 25, 6);
    assert(inside(w.first, width, height) && inside(w.second, width, height));
    assert(w.second.y >= w.first.bottom());
    // Beside or under the cover the words keep off it; over a picture they stand at the bottom left.
    if (s == Shape::side) assert(w.first.x >= height);
    if (s == Shape::top) assert(w.first.y >= width);
    if (s == Shape::fill) assert(w.first.x == 24 && height - w.second.bottom() == 24);
    // A camera's single line.
    const Words one = words(Shape::fill, width, height, 24, 25, 0, 6);
    assert(inside(one.first, width, height) && height - one.first.bottom() == 24 && one.second.w == 0);
    std::printf("%4dx%-4d %s\n", width, height, s == Shape::fill ? "fill" : s == Shape::side ? "cover at the left" : "cover at the top");
  }
  // A title on two lines (firmware 0.30.0+): the block grows upward, the line under it stays where it was and the two
  // never overlap, on every glass.
  for (const auto &g : GLASS) {
    const int width = g[0], height = g[1];
    const Shape s = shape(width, height);
    const Words one = words(s, width, height, 24, 32, 25, 6), two = words(s, width, height, 24, 2 * 32, 25, 6);
    assert(fits(s, one, width, height) && two.second.y >= two.first.bottom());
    // Only the CYD standing up lacks the room under its cover: its title keeps one line.
    assert(fits(s, two, width, height) == !(width == 240 && height == 320));
    if (fits(s, two, width, height)) assert(inside(two.first, width, height) && inside(two.second, width, height));
    if (s == Shape::fill) assert(two.second.y == one.second.y && two.first.y == one.first.y - 32);
  }
  assert(title_lines(32, 32) == 1 && title_lines(64, 32) == 2 && title_lines(96, 32) == 2 && title_lines(0, 32) == 1 && title_lines(40, 0) == 1);
  // About square fills, longer glass does not: four to five either way.
  assert(shape(480, 480) == Shape::fill && shape(500, 400) == Shape::fill && shape(400, 500) == Shape::fill);
  assert(shape(501, 400) == Shape::side && shape(400, 501) == Shape::top && shape(480, 320) == Shape::side);
}

static void clock_in_the_middle() {
  const ClockLayout l = clock(480, 480, 120, 32, 12);
  assert(l.time.y + l.time.h + 12 == l.date.y);
  assert(l.time.y - (480 - l.date.bottom()) <= 1 && l.time.y - (480 - l.date.bottom()) >= -1);
  assert(l.time.w == 480 && l.date.w == 480);
}

int main() {
  words_on_every_glass();
  clock_in_the_middle();
  std::puts("saver view ok");
}
