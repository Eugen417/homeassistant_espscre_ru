#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "tile_icon.h"
#include "tile_icon_names.h"
#include "tile_palette.h"

// The card a Home Assistant automation puts over the whole screen (api action
// show_alert). Pure text and lookups so both boards apply the same rules and
// tests/test_alert_overlay.cpp can check them; the profiles do the LVGL work.
namespace screen_alert {
constexpr const char *FALLBACK_ICON = "alert-outline";
constexpr const char *FALLBACK_TITLE = "Notification";
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
  if (alert.title.empty()) alert.title = FALLBACK_TITLE;
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
}  // namespace screen_alert
