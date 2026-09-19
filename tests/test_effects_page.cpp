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
  return 0;
}
