#include "screen_text_en.h"
// clang++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_light_card.cpp -o /tmp/test_light_card && /tmp/test_light_card
#include "../components/smart_display/light_card.h"
#include <cassert>
#include <cstdio>

using namespace light_card;

static bool inside(const Rect &r, int width, int height) {
  return r.x >= 0 && r.right() <= width && r.y >= 0 && r.bottom() <= height;
}
static bool apart(const Rect &a, const Rect &b) {
  return a.empty() || b.empty() || a.right() <= b.x || b.right() <= a.x || a.bottom() <= b.y || b.bottom() <= a.y;
}

static Metrics guition() {
  ui::configure(170, "standard");
  Metrics m; m.value_h = 46; m.text_h = 22; m.icon_h = 42;
  m.touch = ui::touch_min(); m.pad = ui::px(20); m.bar = ui::px(60); m.bar_x = ui::px(16); m.bar_y = ui::px(16);
  return m;
}
static Metrics cyd() {
  ui::configure(143, "compact");
  Metrics m; m.large = false; m.value_h = 28; m.text_h = 14; m.icon_h = 28;
  m.touch = ui::touch_min(); m.pad = ui::px(10); m.bar = ui::px(40); m.bar_x = ui::px(10); m.bar_y = ui::px(8);
  return m;
}
static Metrics waveshare() {
  ui::configure(217, "standard");
  Metrics m; m.value_h = 60; m.text_h = 28; m.icon_h = 54;
  m.touch = ui::touch_min(); m.pad = ui::px(20); m.bar = ui::px(60); m.bar_x = ui::px(16); m.bar_y = ui::px(16);
  return m;
}

// Nothing outside the glass, nothing over anything else, the slider inside its card, the power key a finger.
static void sound(const Metrics &m, const Layout &l, int width, int height) {
  const Rect *parts[] = {&l.card, &l.value, &l.caption};
  for (const Rect *r : parts) if (!r->empty()) assert(inside(*r, width, height));
  for (unsigned a = 0; a < 3; ++a)
    for (unsigned b = a + 1; b < 3; ++b) assert(apart(*parts[a], *parts[b]));
  assert(!l.card.empty() && !l.slider.empty() && !l.value.empty());
  assert(inside(l.power, width, height) && l.power.w >= m.touch && l.power.h >= m.touch);
  assert(l.power.y + l.power.h <= l.card.y);
  // The slider stands in its card, keeps a thumb's width, and is centred on it.
  assert(l.slider.x >= l.card.x && l.slider.right() <= l.card.right());
  assert(l.slider.y >= l.card.y && l.slider.bottom() <= l.card.bottom());
  assert(l.slider.w == std::min(m.slider_w(), l.card.w - 2 * m.edge()));
  assert(std::abs(l.slider.cx() - l.card.cx()) <= 1);
  // The icon rides on the slider, never above its top edge.
  if (!l.icon.empty()) {
    assert(l.icon.x >= l.slider.x && l.icon.right() <= l.slider.right());
    assert(l.icon.y >= l.slider.y && l.icon.bottom() <= l.slider.bottom());
  }
  // The value reads as the state line: under the card in one column, beside the slider in two.
  if (l.columns == 1) {
    assert(l.value.y == l.card.bottom() && l.value.w == l.card.w);
    if (!l.caption.empty()) assert(l.caption.y == l.value.bottom());
  } else {
    assert(l.value.x >= l.card.right() + ui::column_gap());
    assert(l.caption.empty() || l.caption.y == l.value.bottom());
  }
}

static void sweep(const Metrics &m, int width, int height) {
  const int columns = stacked_height(m) > height && width * 2 >= height * 3 ? 2 : 1;
  sound(m, layout(m, width, height, columns), width, height);
  sound(m, layout(m, width, height, 1), width, height);
}

int main() {
  // The boards that ship, in one column: a slider as tall as the old overlay drew, the value under it.
  {
    Metrics m = guition();
    Layout l = layout(m, 480, 480, 1);
    sound(m, l, 480, 480);
    assert(l.columns == 1 && !l.caption.empty() && !l.icon.empty());
    assert(l.slider.w == 144 && l.slider.h >= 250);          // the overlay drew 144 x 300
    assert(l.caption.bottom() <= 480 - m.margin());
  }
  {
    Metrics m = cyd();
    Layout l = layout(m, 320, 240, 1);
    sound(m, l, 320, 240);
    assert(l.slider.w == 96 && l.slider.h >= m.slider_min_h());   // the overlay drew 96 x 160
  }
  {
    Metrics m = waveshare();
    Layout l = layout(m, 800, 480, 1);
    sound(m, l, 800, 480);
    assert(l.slider.h >= m.slider_min_h());
  }
  // Wide and short glass puts the value beside the slider and gives the card the whole height.
  {
    Metrics m = guition();
    Layout l = layout(m, 800, 272, 2);
    sound(m, l, 800, 272);
    assert(l.columns == 2 && l.value.x >= l.card.right() + ui::column_gap());
    // Glass this short cannot give the slider its full height; it gives it everything there is, which is
    // what the two columns were for. Nothing else is left to take: the slider is the card.
    assert(l.card.bottom() == 272 - m.margin() && l.slider.h == l.card.h - 2 * m.edge());
  }
  // Every shape, in both looks.
  sweep(guition(), 480, 480);
  sweep(cyd(), 320, 240);
  sweep(cyd(), 240, 320);
  sweep(waveshare(), 800, 480);
  sweep(waveshare(), 480, 800);
  { Metrics m = guition(); sweep(m, 480, 272); sweep(m, 1024, 600); sweep(m, 320, 480); sweep(m, 240, 320); }
  printf("test_light_card: ok\n");
  return 0;
}
