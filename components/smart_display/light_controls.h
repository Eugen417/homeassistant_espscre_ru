#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace light_controls {
inline int clamp(int value, int low, int high) { return std::max(low, std::min(high, value)); }
// HA serializes list attributes as either JSON lists or Python tuples.
inline bool first_number(const std::string &text, float &value) {
  const char *p = text.c_str();
  while (*p == ' ' || *p == '[' || *p == '(') ++p;
  char *end;
  float parsed = std::strtof(p, &end);
  if (end == p || !std::isfinite(parsed)) return false;
  value = parsed;
  return true;
}
struct State {
  const char *entity = "";
  int hue = 0, kelvin = 3000, minimum = 0, maximum = 0;
  bool temperature_ready() const { return minimum >= 1000 && maximum <= 15000 && maximum > minimum; }
  void update(unsigned attribute, const std::string &text) {
    float number;
    if (!first_number(text, number)) return;
    if (attribute == 0 && number >= 0 && number <= 360) hue = clamp(std::lround(number), 0, 360);
    if (attribute == 1 && number >= 1000 && number <= 15000) kelvin = std::lround(number);
    if (attribute == 2) minimum = number >= 1000 && number <= 15000 ? std::lround(number) : 0;
    if (attribute == 3) maximum = number >= 1000 && number <= 15000 ? std::lround(number) : 0;
  }
};
}

#ifndef LIGHT_CONTROLS_TEST
#include "esphome/components/api/api_server.h"
#include "lvgl.h"
#include <functional>
namespace light_controls {
inline State states[8];
inline State fallback;
inline State *active = &fallback;
struct Row { lv_obj_t *box{}, *slider{}, *value{}; bool dirty = false; unsigned index{}; };
inline Row rows[3];
inline std::function<void(int)> commits[3];
inline bool ready = false;
inline bool demo = false;
inline void state_updated(State *state);
inline int top, spacing;
inline void subscribe(const char *const *entities, size_t count) {
#ifdef USE_API_HOMEASSISTANT_STATES
  static const char *attributes[] = {"hs_color", "color_temp_kelvin", "min_color_temp_kelvin", "max_color_temp_kelvin"};
  size_t slot = 0;
  for (size_t i = 0; i < count && slot < 8; ++i) {
    if (std::string(entities[i]).rfind("light.", 0) != 0) continue;
    bool duplicate = false;
    for (size_t j = 0; j < slot; ++j) if (std::string(states[j].entity) == entities[i]) duplicate = true;
    if (duplicate) continue;
    State *state = &states[slot++];
    state->entity = entities[i];
    for (unsigned a = 0; a < 4; ++a)
      esphome::api::global_api_server->subscribe_home_assistant_state(state->entity, attributes[a],
        [state, a](esphome::StringRef value) { state->update(a, std::string(value.c_str(), value.size())); state_updated(state); });
  }
#else
  (void) entities;
  (void) count;
#endif
}
inline void preview(Row &row) {
  int value = lv_slider_get_value(row.slider);
  if (row.index == 0) {
    lv_label_set_text_fmt(row.value, "%d°", value);
    lv_obj_set_style_bg_color(row.slider, lv_color_hsv_to_rgb(value % 360, 100, 100), LV_PART_KNOB);
  } else if (row.index == 1) {
    lv_label_set_text_fmt(row.value, "%d K", value);
  } else lv_label_set_text_fmt(row.value, "%d %%", value);
}
inline void event(lv_event_t *e) {
  auto &row = *static_cast<Row *>(lv_event_get_user_data(e));
  auto code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) { row.dirty = true; preview(row); }
  if (code == LV_EVENT_PRESS_LOST) row.dirty = false;
  if (code == LV_EVENT_RELEASED && row.dirty) {
    row.dirty = false;
    int value = lv_slider_get_value(row.slider);
    if (row.index == 0) active->hue = value;
    if (row.index == 1) {
      if (!active->temperature_ready()) return;
      value = clamp(value, active->minimum, active->maximum);
      active->kelvin = value;
    }
    if (!demo && commits[row.index]) commits[row.index](value);
  }
}
inline lv_obj_t *plain(lv_obj_t *parent, int x, int y, int w, int h) {
  auto *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
  return obj;
}
inline void setup(lv_obj_t *parent, const lv_font_t *font, int width, int height, bool light_theme = false) {
  if (ready) return;
  ready = true;
  bool large = width >= 480;
  top = large ? 94 : 55; spacing = large ? 110 : 58;
  int margin = large ? 24 : 16, w = width - 2 * margin;
  int track_x = 14, track_y = large ? 43 : 28, track_h = large ? 28 : 14;
  int track_w = w - 28;
  const char *names[] = {"Kleur", "Wittemperatuur", "Helderheid"};
  const uint32_t rainbow[] = {0xFF0000, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF, 0xFF0000};
  for (unsigned i = 0; i < 3; ++i) {
    auto &row = rows[i]; row.index = i;
    row.box = plain(parent, margin, top + i * spacing, w, spacing - 2);
    lv_obj_set_style_text_font(row.box, font, 0);
    lv_obj_set_style_text_color(row.box, lv_color_hex(light_theme ? 0x1B1B1B : 0xEEEEF2), 0);
    auto *label = lv_label_create(row.box); lv_label_set_text(label, names[i]);
    lv_obj_set_pos(label, 2, 0);
    row.value = lv_label_create(row.box); lv_obj_align(row.value, LV_ALIGN_TOP_RIGHT, -2, 0);
    unsigned segments = i == 0 ? 6 : 1;
    for (unsigned n = 0; n < segments; ++n) {
      int start = n * track_w / segments, end = (n + 1) * track_w / segments;
      auto *stripe = plain(row.box, track_x + start, track_y, end - start, track_h);
      lv_obj_set_style_bg_opa(stripe, LV_OPA_COVER, 0);
      lv_obj_set_style_bg_color(stripe, lv_color_hex(i == 0 ? rainbow[n] : i == 1 ? 0xFF9C32 : 0x40404A), 0);
      lv_obj_set_style_bg_grad_color(stripe, lv_color_hex(i == 0 ? rainbow[n + 1] : i == 1 ? 0xC6E6FF : 0xFFFFFF), 0);
      lv_obj_set_style_bg_grad_dir(stripe, LV_GRAD_DIR_HOR, 0);
    }
    row.slider = lv_slider_create(row.box);
    lv_obj_remove_style_all(row.slider);
    lv_obj_remove_flag(row.slider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(row.slider, track_x, track_y); lv_obj_set_size(row.slider, track_w, track_h);
    lv_obj_set_ext_click_area(row.slider, large ? 12 : 8);
    lv_obj_set_style_bg_opa(row.slider, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row.slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(row.slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_bg_color(row.slider, lv_color_hex(0x111111), LV_PART_KNOB);
    lv_obj_set_style_radius(row.slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(row.slider, large ? 5 : 6, LV_PART_KNOB);
    lv_obj_set_style_border_width(row.slider, 2, LV_PART_KNOB);
    lv_obj_set_style_border_color(row.slider, lv_color_hex(0x111111), LV_PART_KNOB);
    lv_slider_set_range(row.slider, i == 0 ? 0 : i == 1 ? 2000 : 1, i == 0 ? 360 : i == 1 ? 6500 : 100);
    lv_obj_add_event_cb(row.slider, event, LV_EVENT_ALL, &row);
    preview(row);
  }
}
// Late capability attributes may arrive after opening the card. Enable the
// waiting temperature slider without disturbing an in-progress gesture.
inline void state_updated(State *state) {
  if (!ready || state != active || !state->temperature_ready()) return;
  auto &row = rows[1];
  if (!lv_obj_has_state(row.slider, LV_STATE_DISABLED)) return;
  lv_slider_set_range(row.slider, state->minimum, state->maximum);
  lv_slider_set_value(row.slider, state->kelvin, LV_ANIM_OFF);
  lv_obj_remove_state(row.slider, LV_STATE_DISABLED);
  preview(row); row.dirty = false;
}
inline void open(const std::string &entity, bool color, bool temperature, int brightness) {
  active = &fallback;
  for (auto &state : states) if (entity == state.entity) { active = &state; break; }
  int y = top;
  for (unsigned i = 0; i < 3; ++i) {
    auto &row = rows[i];
    bool visible = i == 0 ? color : i == 1 ? temperature : true;
    if (!visible) { lv_obj_add_flag(row.box, LV_OBJ_FLAG_HIDDEN); continue; }
    lv_obj_remove_flag(row.box, LV_OBJ_FLAG_HIDDEN); lv_obj_set_y(row.box, y); y += spacing;
    lv_obj_remove_state(row.slider, LV_STATE_DISABLED);
    if (i == 1 && !active->temperature_ready()) lv_obj_add_state(row.slider, LV_STATE_DISABLED);
    if (i == 1 && active->temperature_ready()) lv_slider_set_range(row.slider, active->minimum, active->maximum);
    lv_slider_set_value(row.slider, i == 0 ? active->hue : i == 1 ? active->kelvin : clamp(brightness, 1, 100), LV_ANIM_OFF);
    preview(row); row.dirty = false;
    if (i == 1 && !active->temperature_ready()) lv_label_set_text(row.value, "Even wachten");
  }
}
inline bool self_test() {
  auto *previous = active;
  State saved_fallback = fallback;
  int saved_values[3]; bool saved_hidden[3];
  std::function<void(int)> saved[3];
  int calls[3] = {0, 0, 0};
  for (unsigned i = 0; i < 3; ++i) {
    saved[i] = commits[i]; saved_values[i] = lv_slider_get_value(rows[i].slider);
    saved_hidden[i] = lv_obj_has_flag(rows[i].box, LV_OBJ_FLAG_HIDDEN);
    commits[i] = [&, i](int value) { if (value == (i == 0 ? 180 : i == 1 ? 4000 : 65)) ++calls[i]; };
  }
  fallback.minimum = 2200; fallback.maximum = 5000;
  open("diagnostic", true, true, 50);
  bool ok = true;
  for (unsigned i = 0; i < 3; ++i) {
    auto *slider = rows[i].slider;
    lv_slider_set_value(slider, i == 0 ? 180 : i == 1 ? 4000 : 65, LV_ANIM_OFF);
    for (int n = 0; n < 20; ++n) lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, nullptr);
    ok &= calls[i] == 0;
    lv_obj_send_event(slider, LV_EVENT_RELEASED, nullptr);
    lv_obj_send_event(slider, LV_EVENT_RELEASED, nullptr);
    ok &= calls[i] == 1;
    lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_send_event(slider, LV_EVENT_PRESS_LOST, nullptr);
    lv_obj_send_event(slider, LV_EVENT_RELEASED, nullptr);
    ok &= calls[i] == 1;
  }
  for (unsigned mask = 0; mask < 4; ++mask) {
    open("diagnostic", mask & 1, mask & 2, 50);
    ok &= lv_obj_has_flag(rows[0].box, LV_OBJ_FLAG_HIDDEN) == !(mask & 1);
    ok &= lv_obj_has_flag(rows[1].box, LV_OBJ_FLAG_HIDDEN) == !(mask & 2);
    ok &= !lv_obj_has_flag(rows[2].box, LV_OBJ_FLAG_HIDDEN);
  }
  fallback = saved_fallback;
  open(previous->entity, !saved_hidden[0], !saved_hidden[1], saved_values[2]);
  active = previous;
  for (unsigned i = 0; i < 3; ++i) {
    commits[i] = saved[i];
    lv_slider_set_value(rows[i].slider, saved_values[i], LV_ANIM_OFF);
    preview(rows[i]); rows[i].dirty = false;
  }
  return ok;
}

}
#endif
