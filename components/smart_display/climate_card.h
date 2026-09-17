#pragma once
// Fan and swing modes on the Guition's climate card (firmware 0.2.40+): one white card below the mode
// keys with a segmented row per setting, each behind its icon, where the CYD keeps them behind "more".
// The board profile owns the card and its scripts; this draws the rows into the card and reports a tap
// as the setting ('f' fan, 's' swing) and the index in the entity's own list.
#include "cyd_ui.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "lvgl.h"
#include "theme.h"
#include <algorithm>
#include <functional>
#include <string>

namespace climate_card {
constexpr unsigned MAX_CHOICES = 6;
struct Row { char kind = 0; const char *icon = ""; std::string modes, current; };
struct Look {
  lv_obj_t *card = nullptr;
  const lv_font_t *font = nullptr, *icon_font = nullptr;
  int row_h = 44, gap = 10, edge = 14, icon_w = 42;
};
inline Look look;
inline std::function<void(char, int)> choose;
inline Row rows[2];
inline int row_count = 0;
inline bool queued = false;

// Home Assistant's raw mode names read as words: "fan_only" -> "Fan only", "high" -> "High".
inline std::string pretty(std::string raw) {
  for (char &c : raw) if (c == '_') c = ' ';
  if (!raw.empty() && raw[0] >= 'a' && raw[0] <= 'z') raw[0] = static_cast<char>(raw[0] - 'a' + 'A');
  return raw;
}
inline unsigned choices(const std::string &modes) {
  unsigned n = 0;
  while (n < MAX_CHOICES && !cyd::list_item(modes, n).empty()) ++n;
  return n;
}
// Height of the card for `count` rows, so the profile can place the rest of the card before it is drawn.
inline int height(int count) { return count ? 2 * look.edge + count * look.row_h + (count - 1) * look.gap : 0; }
inline int card_width() {
  // The card is 94 % wide, like the setpoint card above it.
  return lv_display_get_horizontal_resolution(lv_display_get_default()) * 94 / 100;
}
inline int text_width(const std::string &text, const lv_font_t *font) {
  lv_point_t size;
  lv_text_get_size(&size, text.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  return size.x;
}
inline lv_obj_t *shape(lv_obj_t *parent, int x, int y, int w, int h, theme::Role color, int radius) {
  auto *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_color(o, theme::color(color), 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  return o;
}
inline void tapped(lv_event_t *e) {
  const auto code = static_cast<unsigned>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
  const char kind = static_cast<char>(code >> 8);
  const int index = static_cast<int>(code & 0xFF);
  if (!cyd::touch_guard.accept(esphome::millis(), 500 + static_cast<int>(code & 0xFFF))) {
    ESP_LOGI("touch", "tap on climate %c row ignored: %s", kind, cyd::touch_guard.reason().c_str());
    return;
  }
  if (choose) choose(kind, index);
}
// Draws the rows: an icon, then one rounded track with every choice as wide as its word plus an equal
// share of the room left; the chosen one is filled in the cards' blue.
inline void build() {
  queued = false;
  auto *card = look.card;
  if (!card || !look.font || !look.icon_font) return;
  lv_obj_clean(card);
  if (!row_count) { lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN); return; }
  lv_obj_set_height(card, height(row_count));
  lv_obj_remove_flag(card, LV_OBJ_FLAG_HIDDEN);
  const int width = card_width(), track_x = look.edge + look.icon_w, track_w = width - 2 * look.edge - look.icon_w;
  const int inset = std::max(3, look.row_h / 12), room = track_w - 2 * inset, icon_h = lv_font_get_line_height(look.icon_font);
  int y = look.edge;
  for (int r = 0; r < row_count; ++r) {
    const Row &row = rows[r];
    auto *icon = lv_label_create(card);
    lv_label_set_text(icon, row.icon);
    lv_obj_set_style_text_font(icon, look.icon_font, 0);
    lv_obj_set_style_text_color(icon, theme::color(theme::SUBTLE), 0);
    lv_obj_set_pos(icon, look.edge, y + (look.row_h - icon_h) / 2);
    shape(card, track_x, y, track_w, look.row_h, theme::TRACK, look.row_h / 2);
    const unsigned n = choices(row.modes);
    std::string labels[MAX_CHOICES];
    int widths[MAX_CHOICES], words = 0;
    for (unsigned i = 0; i < n; ++i) {
      labels[i] = pretty(cyd::list_item(row.modes, i));
      widths[i] = text_width(labels[i], look.font);
      words += widths[i];
    }
    const int share = n ? (room - words) / static_cast<int>(n) : 0;
    int sx = track_x + inset;
    for (unsigned i = 0; i < n; ++i) {
      const int sw = i + 1 == n ? track_x + track_w - inset - sx : (words <= room ? widths[i] + share : room / static_cast<int>(n));
      const bool selected = cyd::list_item(row.modes, i) == row.current;
      const int sh = look.row_h - 2 * inset;
      auto *segment = shape(card, sx, y + inset, sw, sh, theme::ACCENT, sh / 2);
      lv_obj_add_flag(segment, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_bg_opa(segment, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
      lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, LV_STATE_PRESSED);
      lv_obj_set_style_bg_color(segment, theme::color(selected ? theme::ACCENT_PRESSED : theme::ACCENT_TINT), LV_STATE_PRESSED);
      auto *label = lv_label_create(segment);
      lv_label_set_text(label, labels[i].c_str());
      lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_font(label, look.font, 0);
      lv_obj_set_style_text_color(label, theme::color(selected ? theme::ON_ACCENT : theme::INK), 0);
      lv_obj_set_width(label, std::min(sw - 6, widths[i] + 2));
      lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_center(label);
      const uintptr_t code = (static_cast<uintptr_t>(static_cast<unsigned char>(row.kind)) << 8) | i;
      lv_obj_add_event_cb(segment, tapped, LV_EVENT_SHORT_CLICKED, reinterpret_cast<void *>(code));
      sx += sw;
    }
    y += look.row_h + look.gap;
  }
}
// Called from the card's refresh, also while a row's own tap is still being handled: the rows are drawn
// once LVGL has finished that event, never under the finger that is lifting.
inline void show(const Row *list, int count) {
  row_count = std::clamp(count, 0, 2);
  for (int i = 0; i < row_count; ++i) rows[i] = list[i];
  if (queued) return;
  queued = true;
  lv_async_call([](void *) { build(); }, nullptr);
}
}  // namespace climate_card
