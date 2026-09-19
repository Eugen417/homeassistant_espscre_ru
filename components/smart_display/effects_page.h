#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include "runtime_model.h"
#include "screen_text.h"

// A light's effects page (firmware 0.2.70+, app 0.2.83+): the modes of a lamp such as a WLED, reached from the
// sparkles key at the top right of the light's colour card. One card of rows, each the name of something the lamp
// offers and what it is set to (the light's effect, and the select entities of its device: colour palette, preset,
// playlist), and under it a slider card per number entity of the device (speed, intensity). A row opens a picker:
// LVGL's roller with every name Home Assistant lists at that moment (asked for when the picker opens, one page of
// names per message), and the check key at the top right sends the name the drum stands on. Nothing here knows a brand: the rows,
// their names and their icons come from Home Assistant through the add-on.
//
// Like the settings page, the page is built when it opens and thrown away when it closes, so it costs nothing while
// closed. The model part (percentages, texts, what the page can show) stays free of LVGL for tests/test_effects_page.cpp.
namespace effects_page {
using runtime_tiles::NumberRow;
using runtime_tiles::OptionRow;
using runtime_tiles::Tile;

// LightEntityFeature.EFFECT: the light offers an effect list.
constexpr uint32_t EFFECT_FEATURE = 4;
// Names a picker holds at most; a WLED lists about 220 effects.
constexpr size_t MAX_NAMES = 400;
// A chosen name stays on its row while Home Assistant still reports the old one, this long at most.
constexpr uint32_t SENT_HOLD_MS = 4000;

inline int clamp(int value, int low, int high) { return std::max(low, std::min(high, value)); }
// Whether a light has an effects page: it offers effects, or its device has rows to show.
inline bool available(const Tile &t) {
  return t.domain() == "light" && ((t.supported & EFFECT_FEATURE) != 0 || !t.extra().option_rows.empty() || !t.extra().number_rows.empty());
}
// A number's place on its slider, 0-100 % of its range.
inline int percent_of(float value, float low, float high) {
  if (!std::isfinite(value) || !(high > low)) return 0;
  return clamp(static_cast<int>(std::lround((value - low) * 100 / (high - low))), 0, 100);
}
// The number a slider stands for, on the entity's own step and inside its range.
inline float value_at(int percent, float low, float high, float step) {
  float value = low + (high - low) * clamp(percent, 0, 100) / 100.0f;
  if (step > 0) value = low + std::round((value - low) / step) * step;
  return std::min(high, std::max(low, value));
}
// A number as Home Assistant's action takes it: "128", "12.5", never "128.000000".
inline std::string number_text(float value) {
  char b[24];
  snprintf(b, sizeof(b), "%.3f", value);
  std::string text = b;
  while (text.size() > 1 && text.back() == '0') text.pop_back();
  if (text.back() == '.') text.pop_back();
  return text;
}
inline std::string percent_text(int percent) { return std::to_string(percent) + " %"; }
// The roller takes its names as one string, a line per name.
inline std::string joined(const std::vector<std::string> &names) {
  std::string out;
  for (size_t i = 0; i < names.size(); ++i) { if (i) out += '\n'; out += names[i]; }
  return out;
}
inline int index_of(const std::vector<std::string> &names, const std::string &name) {
  for (size_t i = 0; i < names.size(); ++i) if (names[i] == name) return static_cast<int>(i);
  return -1;
}
// The value a row shows: the name Home Assistant reports, a dash while it reports none.
inline std::string row_text(const std::string &current) {
  return current.empty() || current == "unknown" || current == "unavailable" || current == "None" ? "—" : current;
}

// Sizes of both boards, from the display's width like light_controls.
struct Metrics {
  int width = 480, height = 480, pad = 20, bar_y = 16, bar = 60, title_y = 35, rows_y = 100, row_h = 56, inset = 18,
      icon = 26, gap = 12, number_h = 92, track_h = 28, number_inset = 16, roller_rows = 5, roller_row_h = 56, roller_pad = 10,
      radius = 18, knob = 4;
};
inline Metrics metrics(int width, int height) {
  Metrics m;
  m.width = width; m.height = height;
  if (width < 480) {
    m.pad = 12; m.bar_y = 8; m.bar = 40; m.title_y = 21; m.rows_y = 52; m.row_h = 34; m.inset = 10; m.icon = 18; m.gap = 6;
    m.number_h = 44; m.track_h = 14; m.number_inset = 9; m.roller_row_h = 30; m.roller_pad = 5; m.radius = 10; m.knob = 3;
  }
  return m;
}
}  // namespace effects_page

#ifndef EFFECTS_PAGE_TEST
#include "esphome/core/log.h"
#include "lvgl.h"
#include "theme.h"
#include "tile_icon.h"
namespace effects_page {
// ---- what the board profile wires up at boot ----
inline const lv_font_t *title_font = nullptr, *row_font = nullptr, *roller_font = nullptr, *icon_font = nullptr;
inline std::function<const Tile *(const std::string &)> tile_of;                                            // a tile by entity
inline std::function<void(const std::string &, const std::string &, const std::string &, const std::string &)> send;  // service, entity, key, value
inline std::function<void(const std::string &, unsigned)> ask;                                               // options_request
inline std::function<int()> drift;                                                                           // finger travel this touch
inline int tap_limit = 0;
inline uint32_t (*now)() = nullptr;

// ---- state while open ----
inline lv_obj_t *root = nullptr, *picker = nullptr;
inline std::string entity;   // the light
struct RowDrawn { lv_obj_t *card = nullptr, *value = nullptr; std::string entity, sent; uint32_t sent_at = 0; bool light = false; std::string name; };
struct NumberDrawn { lv_obj_t *slider = nullptr, *value = nullptr; std::string entity; float low = 0, high = 100, step = 1; bool dirty = false; };
inline std::vector<RowDrawn> rows;
inline std::vector<NumberDrawn> numbers;
// The picker's names: one entity at a time, page by page from the add-on.
struct Options { std::string entity; std::vector<std::string> names; unsigned next = 0, pages = 0; bool complete = false; uint32_t asked_at = 0; };
inline Options options;
inline lv_obj_t *roller = nullptr, *roller_note = nullptr;
inline int picker_row = -1;

inline uint32_t clock() { return now ? now() : lv_tick_get(); }
// The page's sizes from the screen (a page just made has no width of its own until LVGL lays it out).
inline Metrics screen_metrics() {
  auto *display = lv_display_get_default();
  return metrics(lv_display_get_horizontal_resolution(display), lv_display_get_vertical_resolution(display));
}
inline bool visible() { return root != nullptr; }
inline bool steady() { return !drift || tap_limit <= 0 || drift() <= tap_limit; }
inline const Tile *tile() { return tile_of ? tile_of(entity) : nullptr; }
// Whether the colour card of this light shows the sparkles key.
inline bool offered(const std::string &light) { const Tile *t = tile_of ? tile_of(light) : nullptr; return t && available(*t); }

inline lv_obj_t *plain(lv_obj_t *parent, int x, int y, int w, int h) {
  auto *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_size(obj, w, h);
  return obj;
}
inline lv_obj_t *text(lv_obj_t *parent, const std::string &value, const lv_font_t *font, theme::Role color,
                      lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
  auto *label = lv_label_create(parent);
  lv_label_set_text(label, value.c_str());
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, theme::color(color), 0);
  lv_obj_set_style_text_align(label, align, 0);
  lv_obj_set_height(label, lv_font_get_line_height(font));
  return label;
}
inline lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h, int radius) {
  auto *obj = plain(parent, x, y, w, h);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(obj, theme::color(theme::CARD), 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, theme::color(theme::LINE), 0);
  lv_obj_set_style_radius(obj, radius, 0);
  return obj;
}
// The glyph a row shows: Home Assistant's icon when these fonts carry it, else one of the page's own.
inline std::string glyph(uint32_t icon, const char *fallback) {
  if (icon && icon_font) { lv_font_glyph_dsc_t dsc; if (lv_font_get_glyph_dsc(icon_font, &dsc, icon, 0)) return tile_icon::utf8(icon); }
  return fallback;
}
// A round key like the back key of every card, with a glyph in the middle.
inline lv_obj_t *round_key(lv_obj_t *parent, int x, int y, int size, const char *glyph, lv_event_cb_t handler) {
  auto *key = plain(parent, x, y, size, size);
  lv_obj_add_flag(key, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(key, theme::color(theme::KEY), 0);
  lv_obj_set_style_bg_color(key, theme::color(theme::KEY_PRESSED), LV_STATE_PRESSED);
  lv_obj_set_style_radius(key, LV_RADIUS_CIRCLE, 0);
  auto *label = text(key, glyph, icon_font ? icon_font : row_font, theme::INK);
  lv_obj_set_width(label, LV_SIZE_CONTENT);
  lv_obj_center(label);
  lv_obj_add_event_cb(key, handler, LV_EVENT_SHORT_CLICKED, nullptr);
  return key;
}
// The same top bar as every card: a round back key and the name in the middle, and on the picker the check key at
// the top right, where the colour card has its sparkles key.
inline lv_obj_t *top_bar(lv_obj_t *parent, const Metrics &m, const std::string &title, lv_event_cb_t back,
                         const char *right_glyph = nullptr, lv_event_cb_t right = nullptr) {
  auto *key = round_key(parent, m.pad, m.bar_y, m.bar, "\U000F004D", back);
  if (right_glyph && right) round_key(parent, m.width - m.pad - m.bar, m.bar_y, m.bar, right_glyph, right);
  const lv_font_t *heading = title_font ? title_font : row_font;
  auto *label = text(parent, title, heading, theme::INK, LV_TEXT_ALIGN_CENTER);
  lv_obj_set_width(label, m.width - 2 * (m.pad + m.bar + 8));
  lv_obj_set_pos(label, m.pad + m.bar + 8, m.bar_y + (m.bar - lv_font_get_line_height(heading)) / 2);
  return key;
}
inline lv_obj_t *page_root() {
  auto *obj = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(obj);
  lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);  // nothing leaks through to what lies under it
  lv_obj_set_style_bg_color(obj, theme::color(theme::PAGE_SOFT), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_move_foreground(obj);
  return obj;
}

// ---- the picker ----
inline void close_picker() {
  if (picker) { lv_obj_delete(picker); picker = nullptr; }
  roller = roller_note = nullptr;
  picker_row = -1;
}
inline void choose(const std::string &name) {
  if (picker_row < 0 || picker_row >= static_cast<int>(rows.size()) || name.empty()) return;
  auto &row = rows[picker_row];
  // What Home Assistant reports for the row now: the drum resting on it again sends nothing.
  std::string current;
  if (const Tile *t = tile()) {
    if (row.light) current = t->extra().effect;
    else for (auto &r : t->extra().option_rows) if (r.entity == row.entity) current = r.current;
  }
  if (name == row.sent || (row.sent.empty() && name == current)) return;
  row.sent = name; row.sent_at = clock();
  if (row.value) lv_label_set_text(row.value, name.c_str());
  ESP_LOGI("effects", "%s: %s", row.entity.c_str(), name.c_str());
  if (!send) return;
  if (row.light) send("light.turn_on", row.entity, "effect", name);
  else send("select.select_option", row.entity, "option", name);
}
// The check key: the name the drum stands on goes out, and the picker closes. Nothing is sent before it.
inline void confirm() {
  if (!roller) return;
  char name[64];
  lv_roller_get_selected_str(roller, name, sizeof(name));
  choose(name);
  close_picker();
}
inline void confirm_event(lv_event_t *) { if (steady()) confirm(); }
inline void show_roller() {
  if (!picker || !roller_note || options.names.empty()) return;
  auto *holder = lv_obj_get_parent(roller_note);
  const Metrics m = screen_metrics();
  lv_obj_add_flag(roller_note, LV_OBJ_FLAG_HIDDEN);
  const lv_font_t *font = roller_font ? roller_font : row_font;
  roller = lv_roller_create(holder);
  lv_obj_remove_style_all(roller);
  lv_obj_set_style_bg_opa(roller, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_text_font(roller, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(roller, theme::color(theme::MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(roller, std::max(0, m.roller_row_h - (int) lv_font_get_line_height(font)), LV_PART_MAIN);
  lv_obj_set_style_anim_duration(roller, 200, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_SELECTED);
  lv_obj_set_style_bg_color(roller, theme::color(theme::ACCENT_TINT), LV_PART_SELECTED);
  lv_obj_set_style_radius(roller, m.roller_row_h / 2, LV_PART_SELECTED);
  lv_obj_set_style_text_color(roller, theme::color(theme::INK), LV_PART_SELECTED);
  lv_obj_set_style_text_font(roller, font, LV_PART_SELECTED);
  lv_roller_set_options(roller, joined(options.names).c_str(), LV_ROLLER_MODE_NORMAL);
  lv_roller_set_visible_row_count(roller, m.roller_rows);
  // The card's width from the sizes, not from the card: a card made a moment ago has no width until LVGL lays it out.
  lv_obj_set_width(roller, m.width - 2 * m.pad - 2 * m.inset);
  lv_obj_center(roller);
  const auto &row = rows[picker_row];
  std::string current = !row.sent.empty() ? row.sent : row.light ? (tile() ? tile()->extra().effect : std::string()) : [&]() {
    if (auto *t = tile()) for (auto &r : t->extra().option_rows) if (r.entity == row.entity) return r.current;
    return std::string();
  }();
  int index = index_of(options.names, current);
  if (index >= 0) lv_roller_set_selected(roller, static_cast<uint32_t>(index), LV_ANIM_OFF);
}
// One page of names from the add-on (runtime_tiles::options_received); the roller appears with the last one.
inline void received(const std::string &for_entity, unsigned page, unsigned pages, std::vector<std::string> &&names) {
  if (!picker || for_entity != options.entity || options.complete) return;
  if (page != options.next) return;  // an answer to an older question, or one that arrived twice
  for (auto &name : names) { if (options.names.size() >= MAX_NAMES) break; options.names.push_back(std::move(name)); }
  options.next = page + 1;
  options.pages = std::max(1u, pages);
  if (options.next < options.pages && options.names.size() < MAX_NAMES) { options.asked_at = clock(); if (ask) ask(options.entity, options.next); return; }
  options.complete = true;
  if (options.names.empty()) { if (roller_note) lv_label_set_text(roller_note, screen_text::tr(screen_text::txt::effects_nothing)); return; }
  show_roller();
}
inline void picker_back(lv_event_t *) { if (steady()) close_picker(); }
inline void open_picker(int index) {
  if (!root || index < 0 || index >= static_cast<int>(rows.size())) return;
  close_picker();
  picker_row = index;
  const auto &row = rows[index];
  const Metrics m = screen_metrics();
  picker = page_root();
  top_bar(picker, m, row.name, picker_back, "\U000F012C", confirm_event);
  const int w = m.width - 2 * m.pad, h = m.roller_rows * m.roller_row_h + 2 * m.roller_pad;
  auto *holder = card(picker, m.pad, m.rows_y, w, h, m.radius);
  roller_note = text(holder, screen_text::tr(screen_text::txt::effects_loading), row_font, theme::MUTED, LV_TEXT_ALIGN_CENTER);
  lv_obj_set_width(roller_note, w - 2 * m.inset);
  lv_obj_center(roller_note);
  // The names of the same entity from a moment ago serve again; anything else is asked for afresh.
  if (options.entity == row.entity && options.complete && !options.names.empty()) { show_roller(); return; }
  options = Options{};
  options.entity = row.entity;
  options.asked_at = clock();
  if (ask) ask(row.entity, 0);
  else lv_label_set_text(roller_note, screen_text::tr(screen_text::txt::effects_not_connected));
}

// ---- the page ----
inline void row_event(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED || !steady()) return;
  open_picker(static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}
inline void preview(NumberDrawn &n) {
  if (n.value) lv_label_set_text(n.value, percent_text(lv_slider_get_value(n.slider)).c_str());
}
inline void slider_event(lv_event_t *e) {
  auto &n = numbers[reinterpret_cast<intptr_t>(lv_event_get_user_data(e))];
  const auto code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) { n.dirty = true; preview(n); }
  if (code == LV_EVENT_PRESS_LOST) n.dirty = false;
  if (code == LV_EVENT_RELEASED && n.dirty) {
    n.dirty = false;
    const float value = value_at(lv_slider_get_value(n.slider), n.low, n.high, n.step);
    ESP_LOGI("effects", "%s: %s", n.entity.c_str(), number_text(value).c_str());
    if (send) send("number.set_value", n.entity, "value", number_text(value));
  }
}
inline void back_event(lv_event_t *) { if (steady()) { close_picker(); if (root) { lv_obj_delete(root); root = nullptr; } rows.clear(); numbers.clear(); } }
inline void draw() {
  if (!root) return;
  lv_obj_clean(root);
  rows.clear(); numbers.clear();
  const Tile *t = tile();
  const Metrics m = screen_metrics();
  top_bar(root, m, t ? (t->name.empty() ? t->entity : t->name) : entity, back_event);
  const int w = m.width - 2 * m.pad;
  // The rows: the light's effect first, then the selects of its device.
  struct Spec { std::string entity, name, current, fallback; uint32_t icon; bool light; };
  std::vector<Spec> specs;
  if (t && (t->supported & EFFECT_FEATURE)) specs.push_back({t->entity, screen_text::tr(screen_text::txt::effects_effect), t->extra().effect, "\U000F0674", 0, true});
  if (t) for (auto &r : t->extra().option_rows) specs.push_back({r.entity, r.name, r.current, "\U000F0411", r.icon, false});
  int y = m.rows_y;
  if (!specs.empty()) {
    auto *holder = card(root, m.pad, y, w, static_cast<int>(specs.size()) * m.row_h, m.radius);
    for (size_t i = 0; i < specs.size(); ++i) {
      auto &spec = specs[i];
      auto *row = plain(holder, 0, static_cast<int>(i) * m.row_h, w, m.row_h);
      lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
      lv_obj_set_style_bg_color(row, theme::color(theme::CARD_PRESSED), LV_STATE_PRESSED);
      lv_obj_set_style_radius(row, m.radius, 0);
      lv_obj_add_event_cb(row, row_event, LV_EVENT_SHORT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
      const int icon_h = icon_font ? lv_font_get_line_height(icon_font) : 0, text_h = lv_font_get_line_height(row_font);
      int x = m.inset;
      if (icon_font) {
        auto *icon = text(row, glyph(spec.icon, spec.fallback.c_str()), icon_font, theme::MUTED);
        lv_obj_set_width(icon, LV_SIZE_CONTENT);
        lv_obj_set_pos(icon, m.inset - 2, (m.row_h - icon_h) / 2);
        x += m.icon + 6;
      }
      auto *name = text(row, spec.name, row_font, theme::INK);
      lv_obj_set_pos(name, x, (m.row_h - text_h) / 2);
      const int chevron_w = icon_font ? m.icon : 0;
      lv_obj_set_width(name, std::max(20, w / 2 - x));
      if (icon_font) {
        auto *chevron = text(row, "\U000F0142", icon_font, theme::CHEVRON);
        lv_obj_set_width(chevron, LV_SIZE_CONTENT);
        lv_obj_set_pos(chevron, w - m.inset - chevron_w + 2, (m.row_h - icon_h) / 2);
      }
      auto *value = text(row, row_text(spec.current), row_font, theme::MUTED, LV_TEXT_ALIGN_RIGHT);
      const int value_x = x + w / 2 - x + 4;
      lv_obj_set_pos(value, value_x, (m.row_h - text_h) / 2);
      lv_obj_set_width(value, std::max(20, w - m.inset - chevron_w - 4 - value_x));
      RowDrawn drawn; drawn.card = row; drawn.value = value; drawn.entity = spec.entity; drawn.light = spec.light; drawn.name = spec.name;
      rows.push_back(std::move(drawn));
    }
    y += static_cast<int>(specs.size()) * m.row_h + m.gap;
  }
  // The numbers: half-width slider cards side by side, like the brightness slider of the colour card.
  if (t && !t->extra().number_rows.empty()) {
    const size_t count = std::min<size_t>(2, t->extra().number_rows.size());
    const int card_w = count == 1 ? w : (w - m.gap) / 2;
    for (size_t i = 0; i < count; ++i) {
      const auto &n = t->extra().number_rows[i];
      const int x = m.pad + static_cast<int>(i) * (card_w + m.gap);
      auto *holder = card(root, x, y, card_w, m.number_h, m.radius);
      const int text_h = lv_font_get_line_height(row_font), text_y = m.number_inset * 3 / 4;
      int tx = m.number_inset;
      if (icon_font) {
        auto *icon = text(holder, glyph(n.icon, "\U000F00DF"), icon_font, theme::MUTED);
        lv_obj_set_width(icon, LV_SIZE_CONTENT);
        const int icon_h = lv_font_get_line_height(icon_font);
        lv_obj_set_pos(icon, m.number_inset - 2, text_y + (text_h - icon_h) / 2);
        tx += m.icon - 2 + 5;
      }
      auto *name = text(holder, n.name, row_font, theme::INK);
      lv_obj_set_pos(name, tx, text_y);
      lv_obj_set_width(name, std::max(20, card_w - tx - m.number_inset - 52));
      auto *value = text(holder, percent_text(percent_of(n.value, n.low, n.high)), row_font, theme::MUTED, LV_TEXT_ALIGN_RIGHT);
      lv_obj_set_width(value, 50);
      lv_obj_set_pos(value, card_w - m.number_inset - 50, text_y);
      // Corners, handle and shortest fill as the colour card's brightness slider: the fill keeps the track's radius
      // (a smaller one makes LVGL draw it into a buffer of its own on every redraw).
      const int track_x = m.number_inset, track_w = card_w - 2 * m.number_inset, track_h = m.track_h, track_y = m.number_h - m.number_inset - track_h + 2;
      auto *slider = lv_slider_create(holder);
      lv_obj_remove_style_all(slider);
      lv_obj_remove_flag(slider, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_pos(slider, track_x, track_y); lv_obj_set_size(slider, track_w, track_h);
      lv_obj_set_ext_click_area(slider, m.width >= 480 ? 12 : 8);
      const int corner = track_h * 12 / 42;
      lv_obj_set_style_radius(slider, corner, LV_PART_MAIN);
      lv_obj_set_style_radius(slider, corner, LV_PART_INDICATOR);
      lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_bg_color(slider, theme::color(theme::ACCENT_TINT), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
      lv_obj_set_style_bg_color(slider, theme::color(theme::ACCENT_BRIGHT), LV_PART_INDICATOR);
      const int handle = m.knob, back = std::max(1, track_h / 8) + handle / 2, half = track_h >> 1;
      lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
      lv_obj_set_style_bg_color(slider, theme::color(theme::KNOB), LV_PART_KNOB);
      lv_obj_set_style_radius(slider, 2, LV_PART_KNOB);
      lv_obj_set_style_pad_left(slider, back + handle / 2 - half, LV_PART_KNOB);
      lv_obj_set_style_pad_right(slider, handle / 2 - back - (track_h - half), LV_PART_KNOB);
      lv_obj_set_style_pad_top(slider, -(track_h / 4), LV_PART_KNOB);
      lv_obj_set_style_pad_bottom(slider, -(track_h / 4), LV_PART_KNOB);
      lv_slider_set_range(slider, 0, 100);
      lv_slider_set_value(slider, percent_of(n.value, n.low, n.high), LV_ANIM_OFF);
      NumberDrawn drawn; drawn.slider = slider; drawn.value = value; drawn.entity = n.entity; drawn.low = n.low; drawn.high = n.high; drawn.step = n.step;
      numbers.push_back(std::move(drawn));
      lv_obj_add_event_cb(slider, slider_event, LV_EVENT_ALL, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
    }
  }
}
// A state of the light while its page is open: rows follow Home Assistant, a row just chosen keeps its choice a
// moment, a slider under a finger is left alone.
inline void updated(const Tile &t) {
  if (!root || t.entity != entity) return;
  const uint32_t moment = clock();
  for (auto &row : rows) {
    std::string current;
    if (row.light) current = t.extra().effect;
    else for (auto &r : t.extra().option_rows) if (r.entity == row.entity) current = r.current;
    if (!row.sent.empty() && current != row.sent && moment - row.sent_at < SENT_HOLD_MS) continue;
    if (!row.sent.empty() && (current == row.sent || moment - row.sent_at >= SENT_HOLD_MS)) row.sent.clear();
    if (row.value) lv_label_set_text(row.value, row_text(current).c_str());
  }
  for (auto &n : numbers) {
    if (!n.slider || lv_obj_has_state(n.slider, LV_STATE_PRESSED) || n.dirty) continue;
    for (auto &r : t.extra().number_rows) if (r.entity == n.entity) {
      const int percent = percent_of(r.value, r.low, r.high);
      if (lv_slider_get_value(n.slider) != percent) { lv_slider_set_value(n.slider, percent, LV_ANIM_OFF); preview(n); }
    }
  }
}
inline void open(const std::string &light) {
  const Tile *t = tile_of ? tile_of(light) : nullptr;
  if (!t || !available(*t)) return;
  close_picker();
  entity = light;
  if (!root) root = page_root();
  lv_obj_move_foreground(root);
  draw();
  ESP_LOGI("effects", "Effects page of %s: %u rows, %u sliders", light.c_str(), (unsigned) rows.size(), (unsigned) numbers.size());
}
inline void close() {
  close_picker();
  rows.clear(); numbers.clear();
  options = Options{};
  if (root) { lv_obj_delete(root); root = nullptr; }
  entity.clear();
}
// A change of look while the page is open: drawn again in place, the picker closed.
inline void restyle() {
  if (!root) return;
  close_picker();
  lv_obj_set_style_bg_color(root, theme::color(theme::PAGE_SOFT), 0);
  draw();
}
}  // namespace effects_page
#endif
