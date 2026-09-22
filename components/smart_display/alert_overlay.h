#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "screen_text.h"
#include "tile_icon.h"
#include "tile_icon_names.h"
#include "tile_palette.h"

// The card a Home Assistant automation puts over the whole screen (api action
// show_alert). Pure text and lookups so both boards apply the same rules and
// tests/test_alert_overlay.cpp can check them; the profiles do the LVGL work.
namespace screen_alert {
constexpr const char *FALLBACK_ICON = "alert-outline";
constexpr const char *FALLBACK_BUTTON = "OK";
constexpr uint32_t DEFAULT_CARD_COLOR = 0;  // the normal card: theme::surface(0)
constexpr int MAX_TIMEOUT_SECONDS = 86400;

struct Alert {
  std::string title, subtitle, icon, button;  // icon: UTF-8 glyph for the icon font
  uint32_t color = DEFAULT_CARD_COLOR;
  int timeout_seconds = 0;  // 0: stays until OK
  bool flash = false;
};

inline bool blank(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
inline std::string trimmed(const std::string &text) {
  size_t begin = 0, end = text.size();
  while (begin < end && blank(text[begin])) ++begin;
  while (end > begin && blank(text[end - 1])) --end;
  return text.substr(begin, end - begin);
}
inline std::string lowercase(std::string text) {
  for (auto &c : text) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return text;
}
// Cuts on a UTF-8 boundary: LVGL must never get half a code point.
inline std::string clipped(const std::string &text, size_t max_bytes) {
  if (text.size() <= max_bytes) return text;
  size_t end = max_bytes;
  while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
  return text.substr(0, end);
}
// "doorbell", "mdi:doorbell" or the hex codepoint "F12E6" of a glyph the fonts carry;
// anything else draws the warning triangle.
inline uint32_t icon_codepoint(const std::string &value) {
  std::string name = lowercase(trimmed(value));
  if (name.rfind("mdi:", 0) == 0) name.erase(0, 4);
  if (uint32_t code = tile_icon::named(name)) return code;
  const uint32_t code = tile_icon::codepoint(name);
  if (code && tile_icon::carried(code)) return code;
  return tile_icon::named(FALLBACK_ICON);
}
inline int clamp_timeout(int seconds) {
  return seconds < 0 ? 0 : seconds > MAX_TIMEOUT_SECONDS ? MAX_TIMEOUT_SECONDS : seconds;
}
inline Alert make(const std::string &title, const std::string &subtitle, const std::string &icon,
                  const std::string &color, const std::string &button, int timeout, bool flash,
                  size_t title_max, size_t subtitle_max, size_t button_max) {
  Alert alert;
  alert.title = clipped(trimmed(title), title_max);
  if (alert.title.empty()) alert.title = screen_text::tr(screen_text::txt::alert_notification);
  alert.subtitle = clipped(trimmed(subtitle), subtitle_max);
  alert.button = clipped(trimmed(button), button_max);
  if (alert.button.empty()) alert.button = FALLBACK_BUTTON;
  alert.icon = tile_icon::utf8(icon_codepoint(icon));
  const uint32_t card = tile_palette::color(lowercase(trimmed(color)));
  alert.color = card ? card : DEFAULT_CARD_COLOR;
  alert.timeout_seconds = clamp_timeout(timeout);
  alert.flash = flash;
  return alert;
}

// Where the parts of the alert card stand (firmware 0.2.92+). A board states the card's table for the glass it
// was drawn for, and every board that ships states one that fits. The same board built standing up puts that
// table on narrower glass, so the card and everything across it is brought back by the one factor that makes it
// fit, and its height is capped at the glass. Only the widths move: the fonts are compiled into the firmware
// and cannot shrink with them, so a title that is one line high stays one line high.
//
// Pure numbers, so tests/test_alert_overlay.cpp checks them; the profiles do the LVGL work.
struct Frame {
  int card_w, card_h;
  int icon_x, text_x, text_w;
  int button_w, button_inset;
  int image_inset, image_width;
};
// `across` is what the card asks for sideways and `down` what it asks for downwards, both as the board states
// them. `room` is the glass, and `inset` the narrowest strip of it the card may not stand on.
inline int fit_percent(int across, int room, int inset) {
  const int space = room - 2 * inset;
  if (across <= 0 || space <= 0 || across <= space) return 100;
  return space * 100 / across;
}
inline int scaled(int value, int percent) { return percent >= 100 ? value : value * percent / 100; }
// What the card was brought back by on this screen, so a board that draws a picture in its alert (the frame and
// its inset live in the board file, because only a board with a camera has them) moves by the same factor.
inline int percent_applied = 100;

inline Frame frame(int card_w, int card_h, int icon_x, int text_x, int button_w, int button_inset,
                   int image_inset, int image_width, int screen_w, int screen_h, int inset) {
  const int percent = fit_percent(card_w, screen_w, inset);
  Frame f{};
  f.card_w = scaled(card_w, percent);
  f.card_h = card_h <= screen_h - 2 * inset ? card_h : screen_h - 2 * inset;
  f.icon_x = scaled(icon_x, percent);
  f.text_x = scaled(text_x, percent);
  f.button_w = scaled(button_w, percent);
  f.button_inset = scaled(button_inset, percent);
  f.image_inset = scaled(image_inset, percent);
  f.image_width = scaled(image_width, percent);
  if (f.image_width > f.card_w - 2 * f.image_inset) f.image_width = f.card_w - 2 * f.image_inset;
  // The text runs from where it starts to the far side of the card, keeping the icon's margin there.
  f.text_w = f.card_w - f.text_x - f.icon_x;
  if (f.text_w < 0) f.text_w = 0;
  return f;
}
}  // namespace screen_alert
