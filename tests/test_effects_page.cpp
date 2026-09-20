#include "screen_text_en.h"
// c++ -std=c++17 -Wall -Wextra -pedantic tests/test_effects_page.cpp -o /tmp/test_effects_page && /tmp/test_effects_page
// The model of a light's effects page (firmware 0.2.70+): what it can show, how a number maps to its slider, the texts.
#define EFFECTS_PAGE_TEST
#define THEME_TEST
#include "../components/smart_display/effects_page.h"
#include "../components/smart_display/tile_controls.h"
#include <cassert>
#include <cmath>

using namespace effects_page;
using runtime_tiles::Tile;

int main() {
  // A light with the EFFECT feature or with rows from its device has a page; a plain light or a switch has none.
  Tile wled; wled.entity = "light.vuelta"; wled.supported = 36;
  assert(available(wled));
  Tile plain; plain.entity = "light.kitchen"; plain.supported = 32;
  assert(!available(plain));
  runtime_tiles::OptionRow palette; palette.entity = "select.vuelta_kleurenpalet"; palette.name = "Color palette"; palette.current = "Aurora"; palette.count = 75;
  plain.edit_extra().option_rows.push_back(palette);
  assert(available(plain));
  Tile sw; sw.entity = "switch.x"; sw.supported = 4;
  assert(!available(sw));

  // A number's slider: its place in the range, and the value a slider position stands for, on the entity's step.
  assert(percent_of(128, 0, 255) == 50);
  assert(percent_of(0, 0, 255) == 0);
  assert(percent_of(255, 0, 255) == 100);
  assert(percent_of(NAN, 0, 255) == 0);
  assert(percent_of(5, 10, 10) == 0);
  assert(value_at(50, 0, 255, 1) == 128.0f || value_at(50, 0, 255, 1) == 127.0f);
  assert(value_at(100, 0, 255, 1) == 255.0f);
  assert(value_at(0, 0, 255, 1) == 0.0f);
  assert(value_at(101, 0, 255, 1) == 255.0f);
  assert(value_at(50, 16, 32, 0.5f) == 24.0f);
  assert(value_at(33, 0, 10, 5) == 5.0f);
  assert(number_text(128) == "128");
  assert(number_text(12.5f) == "12.5");
  assert(number_text(0) == "0");
  assert(percent_text(50) == "50%");  // "50 %" where the language writes it so (screen_text::percent_sign)

  // The roller's text and the row that is chosen in it.
  std::vector<std::string> names = {"Solid", "Akemi", "TV Simulator"};
  assert(joined(names) == "Solid\nAkemi\nTV Simulator");
  assert(joined({}) == "");
  assert(index_of(names, "TV Simulator") == 2);
  assert(index_of(names, "Blink") == -1);

  // A row shows a dash where Home Assistant reports nothing.
  assert(row_text("Aurora") == "Aurora");
  assert(row_text("") == "—");
  assert(row_text("unknown") == "—");
  assert(row_text("unavailable") == "—");

  // The tile names the effect while one runs; WLED's Solid, Hue's off and Home Assistant's None are none.
  assert(tile_controls::effect_running("TV Simulator"));
  assert(tile_controls::effect_running("candle"));
  assert(!tile_controls::effect_running("Solid"));
  assert(!tile_controls::effect_running("solid "));
  assert(!tile_controls::effect_running("off"));
  assert(!tile_controls::effect_running("Off"));
  assert(!tile_controls::effect_running("None"));
  assert(!tile_controls::effect_running(""));
  assert(!tile_controls::effect_running("unknown"));

  // Both boards' sizes: the CYD's page fits four rows and two sliders in 240 px, the Guition's in 480.
  Metrics big = metrics(480, 480), small = metrics(320, 240);
  assert(big.rows_y + 4 * big.row_h + big.gap + big.number_h <= 480);
  assert(small.rows_y + 4 * small.row_h + small.gap + small.number_h <= 240);
  assert(big.rows_y + big.roller_rows * big.roller_row_h + 2 * big.roller_pad <= 480);
  assert(small.rows_y + small.roller_rows * small.roller_row_h + 2 * small.roller_pad <= 240);

  // Everything under the bar fits the glass, on every board and for every light. A WLED light brings an effect
  // row and the selects of its device (palette, preset, playlist) and two numbers (speed, intensity); on a
  // Waveshare 800x480 that stack was 69 px longer than the screen and the sliders were drawn past the edge.
  auto fits = [](const Metrics &m, const Placed &at, int rows, int numbers) {
    const int bottom = m.height - m.pad;
    if (at.rows_y + at.rows_h > bottom) return false;
    for (int i = 0; i < numbers; ++i)
      if (at.number_y[i] + at.number_h > bottom || at.number_x[i] + at.number_w > m.width - m.pad) return false;
    if (rows && numbers && !at.beside && at.number_y[0] < at.rows_y + at.rows_h) return false;
    if (numbers > 1 && at.number_x[0] == at.number_x[1] && at.number_y[0] == at.number_y[1]) return false;
    return at.row_h > 0 && (numbers == 0 || at.number_h > 0);
  };
  for (auto shape : {std::pair<int, int>{480, 480}, {320, 240}, {800, 480}, {240, 320}, {1024, 600}}) {
    const int scale = shape.first == 800 ? 217 : shape.first == 1024 ? 133 : shape.first == 320 || shape.first == 240 ? 143 : 170;
    ui::configure(scale, shape.first == 320 || shape.first == 240 ? "compact" : "standard");
    Metrics m = metrics(shape.first, shape.second);
    for (int rows = 0; rows <= 6; ++rows)
      for (int numbers = 0; numbers <= 2; ++numbers) {
        const Placed at = place(m, rows, numbers, ui::px(24));
        assert(fits(m, at, rows, numbers));
        assert(rows == 0 || at.row_h >= std::min(m.row_h, ui::touch_min()));
      }
  }
  // The Waveshare: wide glass, so the sliders stand beside the rows instead of being squeezed under them.
  ui::configure(217, "standard");
  Metrics wave = metrics(800, 480);
  const Placed four = place(wave, 4, 2, ui::px(24));
  assert(four.beside);
  assert(four.row_h == wave.row_h && four.number_h == wave.number_h);   // nothing had to give
  assert(four.number_x[0] == four.number_x[1] && four.number_y[1] > four.number_y[0]);
  assert(four.rows_w + wave.gap + four.number_w == wave.width - 2 * wave.pad);
  // A square screen keeps them under the rows, as before.
  ui::configure(170, "standard");
  Metrics guition = metrics(480, 480);
  const Placed two = place(guition, 2, 2, 24);
  assert(!two.beside && two.number_y[0] > two.rows_y && two.number_x[1] > two.number_x[0]);
  ui::configure(170, "standard");

  // The picker's list is cut to the memory the screen has: a board with room keeps the ceiling, a board with
  // almost none keeps nothing, and nothing in between ever asks for more than it can hold. Before this, a WLED
  // light's hundreds of effects aborted the firmware on an 800x480 board the moment the picker opened.
  assert(names_room(0) == MAX_NAMES);                 // nothing can say: the ceiling holds (the host)
  assert(names_room(200 * 1024) == MAX_NAMES);        // a Guition with room to spare
  assert(names_room(NAME_RESERVE) == 0);              // nothing left over: no names at all, and no crash
  assert(names_room(NAME_RESERVE / 2) == 0);
  const size_t tight = names_room(26 * 1024);         // the Waveshare as it really was
  assert(tight > 0 && tight < MAX_NAMES);
  assert(names_room(40 * 1024) > tight);              // more memory, more names
  return 0;
}
