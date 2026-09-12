#pragma once
#include "runtime_model.h"
#include "screen_settings.h"
#include "esphome/core/preferences.h"
#include "cyd_ui.h"
#include "light_controls.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/api/api_server.h"
#include "esphome/core/hal.h"
#include "lvgl.h"
#include <functional>
#include <algorithm>

namespace runtime_tiles {
inline bool enabled = false;
inline bool light_theme = false;
inline Model model;
inline int active_index = -1;
inline uint32_t last_received = 0;
inline std::function<void()> layout_changed, refresh, dismiss, settings_changed;
inline esphome::ESPPreferenceObject settings_preference;
inline void load_settings() {
  settings_preference = esphome::global_preferences->make_preference<screen_settings::Settings>(0x53435231);
  screen_settings::Settings saved;
  if (settings_preference.load(&saved) && saved.valid()) screen_settings::current = saved;
}
inline bool parse_settings(JsonObject obj, screen_settings::Settings &s) {
  // Require the complete known schema; validate before touching any runtime state.
  if (obj.size() != 11) return false;
  const char *flags[] = {"standby_enabled", "night_enabled", "show_clock", "clock_24h", "home_on_standby"};
  int32_t *flag_values[] = {&s.standby_enabled, &s.night_enabled, &s.show_clock, &s.clock_24h, &s.home_on_standby};
  for (int i = 0; i < 5; ++i) {
    if (!obj[flags[i]].is<bool>()) return false;
    *flag_values[i] = obj[flags[i]].as<bool>();
  }
  const char *numbers[] = {"standby_seconds", "brightness", "standby_brightness", "night_start", "night_end", "night_brightness"};
  int32_t *values[] = {&s.standby_seconds, &s.brightness, &s.standby_brightness, &s.night_start, &s.night_end, &s.night_brightness};
  for (int i = 0; i < 6; ++i) {
    if (!obj[numbers[i]].is<int>() || obj[numbers[i]].is<bool>()) return false;
    *values[i] = obj[numbers[i]].as<int>();
  }
  return s.valid();
}
inline std::function<void(Tile &)> detail, detail_update;
struct Widgets { lv_obj_t *tile{}, *title{}, *value{}, *circle{}, *icon{}; size_t index{}; int cached_active = -1; };
inline std::array<Widgets, MAX_TILES> widgets;
inline bool fresh() { return model.ready() && esphome::millis() - last_received < 95000; }
inline float number(JsonVariant value, float fallback = NAN) {
  if (!value.is<float>() && !value.is<int>()) return fallback;
  float n = value.as<float>();
  return std::isfinite(n) ? n : fallback;
}
inline std::string string(JsonVariant value, size_t maximum = 160) {
  if (!value.is<const char *>()) return {};
  std::string s = value.as<std::string>();
  if (s.size() > maximum) {
    while (maximum > 0 && (static_cast<unsigned char>(s[maximum]) & 0xC0) == 0x80) --maximum;
    s.resize(maximum);
  }
  return s;
}
inline std::string list(JsonVariant value) {
  if (!value.is<JsonArray>()) return {};
  std::string out;
  serializeJson(value, out);
  return out.size() <= 512 ? out : "";
}
inline std::string receive(const std::string &payload) {
  if (!enabled) return "Gebruik het Easy Setup-profiel";
  if (payload.size() > 4096) return "Fout: bericht te groot";
  std::string result = "Fout: ongeldig bericht";
  esphome::json::parse_json(payload, [&](JsonObject root) -> bool {
    if (root["v"].as<int>() != 1) { result = "Fout: protocolversie"; return false; }
    auto op = string(root["op"]);
    if (op == "layout") {
      if (!root["entities"].is<JsonArray>() || !root["title"].is<const char *>()) return false;
      auto settings = screen_settings::current;
      if (!root["settings"].isNull() && (!root["settings"].is<JsonObject>() ||
          !parse_settings(root["settings"].as<JsonObject>(), settings))) {
        result = "Fout: scherminstellingen"; return false;
      }
      std::vector<std::string> entities;
      for (JsonVariant entity : root["entities"].as<JsonArray>()) {
        if (!entity.is<const char *>() || entities.size() == MAX_TILES) return false;
        entities.push_back(entity.as<std::string>());
      }
      bool changed = false;
      if (!model.set_layout(entities, string(root["title"], 96), changed)) return false;
      if (!(settings == screen_settings::current)) {
        screen_settings::current = settings;
        settings_preference.save(&settings);  // ESPHome batches flash writes; no write on keepalive.
        if (settings_changed) settings_changed();
      }
      if (changed) { active_index = -1; for (auto &w : widgets) w.cached_active = -1; if (dismiss) dismiss(); }
      last_received = esphome::millis();
      if (layout_changed) layout_changed();
      if (refresh) refresh();
      result = "Indeling ontvangen";
      return true;
    }
    if (op != "state" || !root["i"].is<unsigned>() || !root["a"].is<JsonObject>()) return false;
    unsigned index = root["i"].as<unsigned>();
    std::string entity = string(root["entity"], 120);
    if (!model.accepts(index, entity)) { result = "Fout: verouderde tegel"; return false; }
    Tile &tile = model.tiles[index];
    auto a = root["a"].as<JsonObject>();
    tile.name = string(root["name"], 80);
    tile.state = string(root["state"], 160);
    tile.unit = string(a["unit_of_measurement"], 20);
    tile.brightness = number(a["brightness"]);
    tile.percentage = number(a["percentage"]);
    tile.position = number(a["current_position"]);
    tile.current = number(a["current_temperature"]);
    tile.target = number(a["temperature"]);
    tile.humidity = number(a["current_humidity"]);
    tile.minimum = number(a["min_temp"], 7);
    tile.maximum = number(a["max_temp"], 35);
    tile.step = number(a["target_temp_step"], 0.5f);
    tile.modes = list(a["supported_color_modes"]);
    tile.hvac_modes = list(a["hvac_modes"]);
    float hue = number(a["hs_color"][0]);
    if (std::isfinite(hue)) tile.hue = std::lround(std::clamp(hue, 0.0f, 360.0f));
    float kelvin = number(a["color_temp_kelvin"]);
    if (std::isfinite(kelvin)) tile.kelvin = std::lround(std::clamp(kelvin, 1000.0f, 15000.0f));
    tile.min_kelvin = std::clamp(number(a["min_color_temp_kelvin"], 0), 0.0f, 15000.0f);
    tile.max_kelvin = std::clamp(number(a["max_color_temp_kelvin"], 0), 0.0f, 15000.0f);
    tile.fan_speed_count = 0;
    if (a["fan_speed_list"].is<JsonArray>()) for (JsonVariant speed : a["fan_speed_list"].as<JsonArray>()) {
      if (tile.fan_speed_count == 4) break;
      tile.fan_speeds[tile.fan_speed_count++] = string(speed, 48);
    }
    tile.received = true;
    last_received = esphome::millis();
    if (refresh) refresh();
    if (active_index == static_cast<int>(index) && detail_update) detail_update(tile);
    result = model.ready() ? "Gesynchroniseerd" : "Tegels laden";
    return true;
  });
  return result;
}
inline void action(const std::string &service, const std::string &entity) {
  if (!fresh() || !valid_entity(entity)) return;
  esphome::api::HomeassistantActionRequest request;
  request.service = esphome::StringRef(service);
  request.data.init(1);
  esphome::api::HomeassistantServiceMap entry;
  entry.key = esphome::StringRef("entity_id");
  entry.value = esphome::StringRef(entity);
  request.data.push_back(entry);
  esphome::api::global_api_server->send_homeassistant_action(request);
}
inline const char *icon_for(const Tile &tile) {
  auto d = tile.domain();
  if (d == "light") return "\U000F0335";
  if (d == "climate") return "\U000F001B";
  if (d == "vacuum") return "\U000F070D";
  if (d == "fan") return "\U000F0210";
  if (d == "cover") return "\U000F111C";
  if (d == "scene" || d == "script") return "\U000F04B9";
  if (d == "sensor" || d == "binary_sensor") return "\U000F029A";
  return "\U000F0425";
}
inline void event(lv_event_t *event) {
  auto &w = *static_cast<Widgets *>(lv_event_get_user_data(event));
  if (!enabled || !fresh() || w.index >= model.count) return;
  auto code = lv_event_get_code(event);
  if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED) return;
  if (!cyd::touch_guard.accept(esphome::millis(), 100 + w.index)) return;
  auto &tile = model.tiles[w.index];
  auto d = tile.domain();
  // Scenes/scripts often have timestamps or 'off'; unavailable devices never act.
  if (!tile.available()) return;
  bool open = code == LV_EVENT_LONG_PRESSED || d == "climate" || d == "vacuum" || d == "cover";
  if (open) {
    if (d == "light" || d == "climate" || d == "vacuum" || d == "fan" || d == "cover") {
      active_index = w.index;
      if (detail) detail(tile);
    }
    return;
  }
  if (d == "light" || d == "switch" || d == "input_boolean" || d == "fan") action(d + ".toggle", tile.entity);
  if (d == "scene" || d == "script") action(d + ".turn_on", tile.entity);
  if (d == "button" || d == "input_button") action(d + ".press", tile.entity);
}
inline void bind(size_t index, lv_obj_t *tile, lv_obj_t *title, lv_obj_t *value, lv_obj_t *circle, lv_obj_t *icon) {
  widgets[index] = {tile, title, value, circle, icon, index};
  lv_obj_add_event_cb(tile, event, LV_EVENT_SHORT_CLICKED, &widgets[index]);
  lv_obj_add_event_cb(tile, event, LV_EVENT_LONG_PRESSED, &widgets[index]);
}
inline void label(lv_obj_t *obj, const std::string &text) {
  if (text != lv_label_get_text(obj)) lv_label_set_text(obj, text.c_str());
}
inline void render(lv_obj_t *room) {
  if (!enabled) return;
  label(room, !model.configured ? "Kies tegels in HA" : !model.ready() ? "Tegels laden..." : !fresh() ? "HA niet verbonden" : model.title);
  for (size_t i = 0; i < model.count; ++i) {
    const auto &t = model.tiles[i]; auto &w = widgets[i];
    label(w.title, t.name.empty() ? t.entity : t.name);
    label(w.icon, icon_for(t));
    std::string value = t.state;
    if (!fresh() || !t.available()) value = "Niet beschikbaar";
    else if (t.domain() == "light" && t.state == "on" && std::isfinite(t.brightness)) value = std::to_string(static_cast<int>(std::lround(std::clamp(t.brightness, 0.0f, 255.0f) * 100 / 255))) + " %";
    else if (t.domain() == "climate" && std::isfinite(t.target)) { char b[32]; snprintf(b, sizeof(b), "%.1f°", t.target); value = b; }
    else if (value == "on") value = "Aan";
    else if (value == "off") value = "Uit";
    else if (value == "cleaning") value = "Bezig";
    else if (value == "docked") value = "In dock";
    else if (!t.unit.empty()) value += " " + t.unit;
    label(w.value, value);
    bool on = fresh() && t.active();
    if (w.cached_active == static_cast<int>(on)) continue;
    w.cached_active = on;
    uint32_t accent = t.domain() == "light" ? 0xE4B23C : t.domain() == "climate" ? 0xF18750 : 0x548BD4;
    lv_obj_set_style_bg_color(w.tile, lv_color_hex(light_theme ? 0xFFFFFF : (on ? 0xF5F1E8 : 0x526C85)), 0);
    lv_obj_set_style_border_width(w.tile, 1, 0);
    lv_obj_set_style_border_opa(w.tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(w.tile, lv_color_hex(light_theme ? 0xDDDDDD : (on ? 0xF5F1E8 : 0x9CB3C8)), 0);
    lv_obj_set_style_bg_color(w.circle, lv_color_hex(light_theme ? (on ? 0xD5EEFC : 0xF0F0F0) : (on ? accent : 0x263B50)), 0);
    lv_obj_set_style_text_color(w.icon, lv_color_hex(light_theme ? (on ? 0x009FE3 : 0x9E9E9E) : (on ? 0x172232 : 0xF3F5F7)), 0);
    lv_obj_set_style_text_color(w.title, lv_color_hex(light_theme ? 0x1B1B1B : (on ? 0x172232 : 0xF3F5F7)), 0);
    lv_obj_set_style_text_color(w.value, lv_color_hex(light_theme ? 0x616161 : (on ? 0x46525E : 0xF0F4F8)), 0);
  }
}
inline std::string vacuum_option(unsigned index) {
  if (active_index < 0 || static_cast<size_t>(active_index) >= model.count) return {};
  auto &tile = model.tiles[active_index];
  return index < tile.fan_speed_count ? tile.fan_speeds[index] : "";
}
}
