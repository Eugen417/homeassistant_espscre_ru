#include "screen_text_en.h"
// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_weather_card.cpp -o /tmp/test_weather_card && /tmp/test_weather_card
#include "../components/smart_display/weather_card.h"
#include <cassert>
#include <cstdio>

using namespace weather_card;

static bool inside(const Rect &r, int width, int height) {
  return r.x >= 0 && r.right() <= width && r.y >= 0 && r.bottom() <= height;
}
static bool apart(const Rect &a, const Rect &b) {
  return a.empty() || b.empty() || a.right() <= b.x || b.right() <= a.x || a.bottom() <= b.y || b.bottom() <= a.y;
}

// The three boards that ship, and two shapes they will meet: the fonts' line heights as their look draws them.
static Metrics guition() {
  ui::configure(170, "standard");
  Metrics m; m.text_h = 22; m.small_h = 20; m.mini_h = 32; m.tiny_h = 22; m.icon_h = 50; m.big_h = 46;
  m.hour_w = 42; m.top = ui::px(84); m.pad = ui::px(20);
  return m;
}
static Metrics cyd() {
  ui::configure(143, "compact");
  Metrics m; m.large = false; m.text_h = 14; m.small_h = 14; m.mini_h = 22; m.tiny_h = 15; m.icon_h = 35; m.big_h = 28;
  m.hour_w = 30; m.top = ui::px(38); m.pad = ui::px(10);
  return m;
}
static Metrics waveshare() {
  ui::configure(217, "standard");
  Metrics m; m.text_h = 28; m.small_h = 25; m.mini_h = 41; m.tiny_h = 28; m.icon_h = 65; m.big_h = 60;
  m.hour_w = 53; m.top = ui::px(84); m.pad = ui::px(20);
  return m;
}

// Nothing outside the glass, nothing over anything else, every day row readable, and the days that do not fit
// on this page reachable on the next one.
static void sound(const Metrics &m, const Layout &l, int width, int height, int days) {
  const Rect *blocks[] = {&l.now, &l.heading, &l.days, &l.pager};
  for (const Rect *r : blocks) if (!r->empty()) assert(inside(*r, width, height));
  for (unsigned a = 0; a < 4; ++a)
    for (unsigned b = a + 1; b < 4; ++b) assert(apart(*blocks[a], *blocks[b]));
  assert(!l.now.empty() && !l.days.empty());
  // The hour strip lives inside the "now" card, under its hero.
  if (l.hour_columns) {
    assert(l.hours.x >= m.card_pad() && l.hours.right() <= l.now.w - m.card_pad());
    assert(l.hours.bottom() <= l.now.h - m.card_pad());
    assert(l.hour_columns >= 2 && l.hour_columns * m.hour_w <= l.hours.w);
  } else {
    assert(l.hours.empty());
  }
  // The day rows: readable, inside their card, and the columns of a row in order and never on top of each other.
  assert(l.rows >= 1 && l.row_h >= m.min_row());
  assert(2 * m.row_pad() + l.rows * l.row_h <= l.days.h + l.row_h);
  assert(l.day_x + l.day_w <= l.icon_x && l.icon_x + m.mini_h <= l.cond_x);
  assert(l.cond_x + l.cond_w <= l.rain_x || l.rain_w == 0);
  assert(l.rain_x + l.rain_w <= l.high_x && l.high_x + l.high_w == l.low_x);
  assert(l.low_x + l.low_w <= l.days.w - m.card_pad() + 1);
  assert(l.drop_w <= l.rain_w || l.rain_w == 0);
  // Every day is on some page, and a card with a pager has one.
  assert(l.pages >= 1 && l.pages * l.rows >= days);
  assert((l.pages > 1) == !l.pager.empty());
  if (l.pages > 1) assert(l.first_day(l.pages - 1) < days && l.pager.h >= m.pager_h());
  assert(l.first_day(0) == 0);
}

// Every shape the card can land on, with one to eight hours and one to five days.
static void sweep(const Metrics &m, int width, int height) {
  for (int hours = 0; hours <= 8; ++hours)
    for (int days = 1; days <= 5; ++days) {
      const int columns = stacked_height(m, width, hours, days) > height &&
                          width * 2 >= height * 3 && width >= 2 * m.min_column() + ui::column_gap() ? 2 : 1;
      sound(m, layout(m, width, height, hours, days, columns), width, height, days);
    }
}

int main() {
  // The two first boards draw what they always drew: one column, six hours, five days that fill their card.
  {
    Metrics m = guition();
    Layout l = layout(m, 480, 480, 8, 5, 1);
    sound(m, l, 480, 480, 5);
    assert(l.columns == 1 && l.hour_columns == 6 && l.hour_rain && l.rows == 5 && l.pages == 1);
    assert(l.now.y == ui::px(84) && l.now.h == 202 && l.days.h == 142 && l.row_h == 25);
    assert(!l.heading.empty() && l.days.y == l.heading.bottom() + ui::px(8));
    assert(l.high_x == 328 && l.rain_x == 194 && l.cond_x == 104 && l.cond_w == 86);
  }
  {
    Metrics m = cyd();
    Layout l = layout(m, 320, 240, 8, 5, 1);
    sound(m, l, 320, 240, 5);
    assert(l.columns == 1 && l.hour_columns == 6 && l.hour_rain && l.rows == 5 && l.pages == 1);
    assert(l.heading.empty() && l.now.h == 127 && l.days.h == 65 && l.row_h == 11);
    assert(l.high_x == 235 && l.rain_x == 169);
  }
  // The Waveshare (800 x 480): the stack does not fit, so the two blocks stand beside each other and every day
  // keeps a row of its own. Stacked, the days card was 51 px tall with five rows in it.
  {
    Metrics m = waveshare();
    assert(stacked_height(m, 800, 8, 5) > 480);
    Layout l = layout(m, 800, 480, 8, 5, 2);
    sound(m, l, 800, 480, 5);
    assert(l.columns == 2 && l.rows == 5 && l.pages == 1 && l.hour_columns >= 5 && l.hour_rain);
    assert(l.days.x >= l.now.right() + ui::column_gap() && l.days.h > 300 && l.row_h > 2 * m.min_row());
    assert(l.now.y == l.days.y);
  }
  // The same glass in one column: the stack gives up its hour strip, which is the only concession that buys
  // real room, and every day keeps a row. The heading comes back with it: a state the card no longer needs to
  // give is not given (the strip went, so the word can stay).
  {
    Metrics m = waveshare();
    Layout l = layout(m, 800, 480, 8, 5, 1);
    sound(m, l, 800, 480, 5);
    assert(l.hour_columns == 0 && l.rows == 5 && l.pages == 1 && !l.heading.empty());
    assert(l.days.bottom() <= 480 - m.margin() && l.row_h >= m.min_row());
  }
  // Glass too short for five days however the card gives (a 480 x 272 panel): what is left over goes on a next
  // page, with the pager under the card and every day reachable.
  {
    Metrics m = guition();
    Layout l = layout(m, 480, 272, 8, 5, 1);
    sound(m, l, 480, 272, 5);
    assert(l.pages > 1 && !l.pager.empty() && l.pager.bottom() <= 272);
    assert(l.first_day(1) == l.rows && l.first_day(l.pages - 1) + l.rows >= 5);
  }
  // Glass that is short for its width (a 480 x 272 panel) and glass that is long: both stay sound.
  sweep(guition(), 480, 480);
  sweep(cyd(), 320, 240);
  sweep(waveshare(), 800, 480);
  sweep(waveshare(), 480, 800);
  { Metrics m = guition(); sweep(m, 480, 272); sweep(m, 1024, 600); sweep(m, 240, 320); }
  { Metrics m = cyd(); sweep(m, 240, 320); sweep(m, 320, 480); }
  // A card that has to give: the rain under the hours goes first, then the heading, then the strip itself.
  {
    Metrics m = guition();
    Layout tight = layout(m, 480, 330, 8, 5, 1);
    sound(m, tight, 480, 330, 5);
    assert(!tight.hour_rain || tight.heading.empty() || tight.hour_columns == 0 || tight.pages > 1);
    Layout tighter = layout(m, 480, 260, 8, 5, 1);
    sound(m, tighter, 480, 260, 5);
    assert(tighter.hour_columns == 0 || !tighter.hour_rain);
  }
  printf("test_weather_card: ok\n");
  return 0;
}
