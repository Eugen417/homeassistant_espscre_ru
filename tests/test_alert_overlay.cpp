#define THEME_TEST
#include "components/smart_display/alert_overlay.h"
#include <cassert>
#include <cstdio>
int main() {
  using screen_alert::make;
  const auto alert = make("Someone's at the door", "Door 3, back entrance", "doorbell", "orange", "Coming", 60, true, 48, 160, 12);
  assert(alert.title == "Someone's at the door" && alert.subtitle == "Door 3, back entrance" && alert.button == "Coming");
  assert(alert.icon == tile_icon::utf8(0xF12E6) && alert.color == 0xFFE1C6 && alert.timeout_seconds == 60 && alert.flash);
  // Icon: a name from the set, with mdi: prefix, spaces or capitals, or the hex codepoint of a carried glyph.
  for (const char *icon : {"doorbell", "mdi:doorbell", " Doorbell ", "F12E6", "f12e6", "mdi:F12E6"})
    assert(screen_alert::icon_codepoint(icon) == 0xF12E6);
  // Unknown names, a codepoint the fonts lack and junk draw the warning triangle.
  for (const char *icon : {"", "unknown", "mdi:", "F0000", "F12E", "\xF0\x9F\x94\x94", "mdi:not-in-the-set"})
    assert(screen_alert::icon_codepoint(icon) == 0xF002A);
  // Colour: palette names in any case; empty, none and anything unknown keep the normal card.
  assert(make("x", "", "", "Red", "", 0, false, 48, 160, 12).color == 0xFADADD);
  assert(make("x", "", "", " mint ", "", 0, false, 48, 160, 12).color == 0xD5F0EA);
  for (const char *color : {"", "none", "auto", "#ff0000", "0xFF0000"})
    assert(make("x", "", "", color, "", 0, false, 48, 160, 12).color == screen_alert::DEFAULT_CARD_COLOR);
  // The normal card is no colour of its own: the look draws it (theme::surface(0)).
  assert(screen_alert::DEFAULT_CARD_COLOR == 0);
  // Timeout: 0 and negatives wait for OK; a day is the ceiling.
  assert(make("x", "", "", "", "", 0, false, 48, 160, 12).timeout_seconds == 0);
  assert(make("x", "", "", "", "", -5, false, 48, 160, 12).timeout_seconds == 0);
  assert(make("x", "", "", "", "", 999999, false, 48, 160, 12).timeout_seconds == 86400);
  assert(!make("x", "", "", "", "", 0, false, 48, 160, 12).flash);
  // Title: trimmed, never empty, cut on a UTF-8 boundary; the subtitle keeps its line breaks.
  assert(make("   ", "", "", "", "", 0, false, 48, 160, 12).title == "Notification");
  assert(make("  Hi \n", "", "", "", "", 0, false, 48, 160, 12).title == "Hi");
  assert(make("\xC3\xA9\xC3\xA9\xC3\xA9", "", "", "", "", 0, false, 3, 160, 12).title == "\xC3\xA9");
  assert(make("x", "line 1\nline 2", "", "", "", 0, false, 48, 160, 12).subtitle == "line 1\nline 2");
  assert(make("x", std::string(300, 'a'), "", "", "", 0, false, 48, 160, 12).subtitle.size() == 160);
  assert(make(std::string(100, 'b'), "", "", "", "", 0, false, 48, 160, 12).title.size() == 48);
  // Button: trimmed and capped; empty falls back to OK.
  assert(make("x", "", "", "", "", 0, false, 48, 160, 12).button == "OK");
  assert(make("x", "", "", "", "  Open  ", 0, false, 48, 160, 12).button == "Open");
  assert(make("x", "", "", "", std::string(40, 'c'), 0, false, 48, 160, 12).button.size() == 12);
  // The generated table carries every font glyph by name.
  assert(tile_icon::named("lightbulb") == 0xF0335 && tile_icon::named("alert-outline") == 0xF002A);
  assert(tile_icon::named("nope") == 0 && tile_icon::named("") == 0);
  assert(tile_icon::carried(0xF0335) && tile_icon::carried(0xF002A) && !tile_icon::carried(0xF0000) && !tile_icon::carried(0));
  assert(tile_icon::NAME_COUNT > 150);
  std::puts("test_alert_overlay: PASS");
}
