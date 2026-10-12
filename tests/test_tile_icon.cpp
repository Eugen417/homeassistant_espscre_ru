#include "screen_text_en.h"
#include "components/smart_display/tile_icon.h"
#include <cassert>
int main() {
  assert(tile_icon::codepoint("F06B5")==0xF06B5);
  assert(tile_icon::codepoint("f1d17")==0xF1D17);
  // Names, short or long hex, non-MDI ranges and junk keep the domain icon.
  for (const char *bad : {"", "lamp", "auto", "F06B", "F06B50", "006B5", "F06G5", " F06B", "-F06B"})
    assert(tile_icon::codepoint(bad)==0);
  assert(tile_icon::utf8(0xF06B5)=="\U000F06B5");
  assert(tile_icon::utf8(0xF0335)=="\U000F0335");
  assert(tile_icon::utf8(0).empty() && tile_icon::utf8(0x41).empty());
}
