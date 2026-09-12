#include "components/smart_display/tile_palette.h"
#include <cassert>
int main() {
  assert(tile_palette::color("auto")==0);
  assert(tile_palette::color("")==0);
  assert(tile_palette::color("unknown-future-choice")==0);
  assert(tile_palette::color("red")!=0);
  assert(tile_palette::color("green")!=tile_palette::color("red"));
}
