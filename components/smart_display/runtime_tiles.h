#pragma once
#include "runtime_model.h"
#include "tile_palette.h"
#include "tile_icon.h"
#include "screen_settings.h"
#include "esphome/core/preferences.h"
#include "cyd_ui.h"
#include "light_controls.h"
#include "tile_controls.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/api/api_server.h"
#include "esphome/core/hal.h"
#include "esphome/core/util.h"
#include "esphome/core/time.h"
#include "lvgl.h"
#include <functional>
#include <algorithm>

namespace runtime_tiles {
inline bool enabled = false;
inline bool swipe_pages = false;
inline uint32_t rotation = 0;
inline bool rotation_supported = false;
inline esphome::ESPPreferenceObject rotation_preference;
inline esphome::ESPPreferenceObject swipe_preference;
inline Model model;
inline std::string inbox;
inline const lv_font_t *watch_font = nullptr;
inline const lv_font_t *mini_icon_font = nullptr;
inline const lv_font_t *watch_value_font = nullptr, *watch_icon_font = nullptr;
// Big digits for the clock card; the board profile sets it with the local time source.
inline const lv_font_t *clock_font = nullptr;
// Text in the -/+ pill and the run key of direct controls; the board profile sets it.
inline const lv_font_t *control_font = nullptr;
inline lv_obj_t *room_label = nullptr;  // remembered by render() so page switches can render synchronously
inline std::function<esphome::ESPTime()> now_time;
inline uint32_t now_epoch() { if (!now_time) return 0; auto t = now_time(); return t.is_valid() ? static_cast<uint32_t>(t.timestamp) : 0; }
inline void tick();
inline void refresh_detail(unsigned index);
inline const char *weather_icon(const std::string &condition);
inline const char *weather_text(const std::string &condition);
inline std::string timer_text(const Tile &t);
inline std::string last_run_text(uint32_t epoch);
inline int active_index = -1;
inline uint32_t last_received = 0;
// Seconds between the manager's full repeats; every layout message declares it (app
// 0.2.26+). Older managers get the 120 s that app 0.2.20 introduced.
inline uint32_t keepalive_seconds = 120;
inline std::function<void()> layout_changed, refresh, dismiss, settings_changed;
inline esphome::ESPPreferenceObject settings_preference;
inline void load_settings() {
  settings_preference = esphome::global_preferences->make_preference<screen_settings::Settings>(0x53435231);
  swipe_preference = esphome::global_preferences->make_preference<uint32_t>(0x53575031);
  uint32_t swipe_saved=0;
  if(swipe_preference.load(&swipe_saved))swipe_pages=swipe_saved==1;
  if(rotation_supported){
    rotation_preference=esphome::global_preferences->make_preference<uint32_t>(0x524F5431);
    uint32_t saved=0;
    if(rotation_preference.load(&saved) && saved<=270 && saved%90==0)rotation=saved;
  }
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
struct Widgets {
  lv_obj_t *tile{}, *title{}, *value{}, *circle{}, *icon{}; size_t index{}; int cached_active = -1; lv_obj_t *slider{}, *progress{}, *unit{};
  int title_x=0,title_y=0,value_x=0,value_y=0; const lv_font_t *value_font{}, *icon_font{};
  // Wide cards span both columns; custom cards (clock, forecast, graph) draw into `extra`.
  bool wide=false; int base_width=0; lv_obj_t *extra{}; std::string extra_mode; std::array<lv_obj_t *, 20> parts{}; lv_point_precise_t *points{};
  // Soft area under a polyline (graph, sun path), painted by the extra container's draw event.
  const lv_point_precise_t *fill_points{}; unsigned fill_count=0; int fill_x=0, fill_y=0, fill_base=0; lv_color_t fill_color{}; lv_opa_t fill_opa=0;
  // Direct controls on a wide card: a panel at the right with pill keys, a -/+ pill,
  // a slider or a toggle. Objects are rebuilt only when the control set changes.
  lv_obj_t *panel{}; std::string panel_mode; bool panel_dirty=false; int panel_w=0;
  std::array<lv_obj_t *,3> keys{}, key_icons{}; std::array<int,3> key_commands{}; std::array<std::string,3> key_args; std::array<int,3> key_checked{};
  lv_obj_t *pill{}, *pill_value{}, *knob{}, *control_slider{}; int knob_on=-1;
  lv_color_t panel_accent{}, panel_text{};
  // Busy sheet: a translucent white cover with a small spinner while a command is under way.
  lv_obj_t *busy{}, *spinner{};
};
constexpr unsigned POINT_BUFFER = 128;
inline std::array<Widgets, 10> widgets;
// All icon fonts carry the same generated glyph set, so the first bound one answers for all.
inline bool has_icon_glyph(uint32_t codepoint) {
  for (auto &w : widgets) if (w.icon_font) { lv_font_glyph_dsc_t dsc; return lv_font_get_glyph_dsc(w.icon_font, &dsc, codepoint, 0); }
  return false;
}
// Home Assistant's API link as ESPHome itself tracks it: gone the moment the socket drops,
// back the moment HA reconnects, no guessing from message age.
inline bool ha_connected() { return esphome::api_is_connected(); }
// The manager repeats the whole layout every keepalive; one missed round plus its 20 s
// loop slack and the sending itself are tolerated before the feed counts as gone.
inline bool feed_alive() { return esphome::millis() - last_received < keepalive_seconds * 2000 + 60000; }
inline bool fresh() { return model.ready() && ha_connected() && feed_alive(); }
// A dropped tap is logged with its reason, so a missed touch can be read from the ESPHome log
// instead of guessed: moved too far, too short, already used by this contact, or bounce.
inline bool allowed(uint32_t now, int tile, const std::string &what) {
  if (cyd::touch_guard.accept(now, tile)) return true;
  ESP_LOGI("touch", "tik op %s genegeerd: %s", what.c_str(), cyd::touch_guard.reason().c_str());
  return false;
}
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
      if(!root["swipe_pages"].isNull() && !root["swipe_pages"].is<bool>())return false;
      if(!root["rotation"].isNull() && (!root["rotation"].is<unsigned>() ||
          root["rotation"].as<unsigned>()>270 || root["rotation"].as<unsigned>()%90!=0))return false;
      if(!root["keepalive"].isNull() && (!root["keepalive"].is<unsigned>() ||
          root["keepalive"].as<unsigned>()<5 || root["keepalive"].as<unsigned>()>3600))return false;
      inbox = string(root["inbox"], 160);
      bool changed = false;
      if (!model.set_layout(entities, string(root["title"], 96), changed)) return false;
      if(root["swipe_pages"].is<bool>() && swipe_pages!=root["swipe_pages"].as<bool>()){
        swipe_pages=root["swipe_pages"].as<bool>();uint32_t saved=swipe_pages?1:0;swipe_preference.save(&saved);
      }
      bool rotation_changed=false;
      if(rotation_supported && root["rotation"].is<unsigned>() && rotation!=root["rotation"].as<unsigned>()){
        rotation=root["rotation"].as<unsigned>();rotation_preference.save(&rotation);rotation_changed=true;
      }
      if (!(settings == screen_settings::current)) {
        screen_settings::current = settings;
        settings_preference.save(&settings);  // ESPHome batches flash writes; no write on keepalive.
        rotation_changed=true;
      }
      if(rotation_changed && settings_changed)settings_changed();
      if (changed) { active_index = -1; for (auto &w : widgets) w.cached_active = -1; if (dismiss) dismiss(); }
      if (root["keepalive"].is<unsigned>()) keepalive_seconds = root["keepalive"].as<unsigned>();
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
    // ArduinoJson clears its destination string: serialize attributes first, then add state.
    std::string attributes; serializeJson(a, attributes);
    std::string revision = state_revision(string(root["state"]), attributes);
    bool was_confirmed=tile.confirmed;
    tile.observe(revision);
    if(tile.pending && !tile.local_feedback && !was_confirmed && tile.confirmed)
      ESP_LOGI("runtime_action","HA state received entity=%s elapsed=%u ms",entity.c_str(),(unsigned)(esphome::millis()-tile.pending_since));
    auto options = root["o"];
    std::string background=string(options["background"],16);
    tile.background = tile_palette::color(background);
    tile.transparent = tile_palette::transparent(background);
    // An icon these fonts lack (a newer set than this firmware) keeps the domain icon.
    uint32_t icon = tile_icon::codepoint(string(options["icon"], 8));
    tile.icon = icon && has_icon_glyph(icon) ? tile_icon::utf8(icon) : "";
    tile.tap = string(options["tap"]); if (tile.tap.empty()) tile.tap="auto";
    tile.display = string(options["display"]); if (tile.display.empty()) tile.display="standard";
    tile.inline_control = string(options["inline"]); if (tile.inline_control.empty()) tile.inline_control="none";
    // Direct controls (0.2.19+): the manager sends only the set a wide card really shows.
    tile.controls = string(options["controls"], 16);
    // Width arrives with the state, after the layout: re-pack the pages when it changes.
    bool was_wide = tile.wide;
    tile.wide = string(options["size"]) == "wide";
    bool repack = was_wide != tile.wide;
    // Pre-computed extras: the manager converts time zones and fetches forecasts.
    auto extra = root["x"];
    tile.forecast.clear();
    if (extra["days"].is<JsonArray>()) for (JsonVariant day : extra["days"].as<JsonArray>()) {
      if (tile.forecast.size() == 5) break;
      tile.forecast.emplace_back(); auto &f = tile.forecast.back();
      f.day = string(day["d"], 3); f.condition = string(day["c"], 20); f.high = number(day["h"]); f.low = number(day["l"]);
      f.rain = number(day["p"]); f.mm = number(day["r"]);
    }
    tile.hours.clear();
    if (extra["hours"].is<JsonArray>()) for (JsonVariant hour : extra["hours"].as<JsonArray>()) {
      if (tile.hours.size() == 8) break;
      tile.hours.emplace_back(); auto &h = tile.hours.back();
      h.time = string(hour["t"], 5); h.condition = string(hour["c"], 20); h.temp = number(hour["h"]); h.rain = number(hour["p"]); h.mm = number(hour["r"]);
    }
    tile.last_run = extra["last"].is<unsigned>() ? extra["last"].as<uint32_t>() : 0;
    tile.sunrise = string(extra["rise"], 5); tile.sunset = string(extra["set"], 5);
    tile.timer_end = extra["end"].is<unsigned>() ? extra["end"].as<uint32_t>() : 0;
    tile.duration = string(extra["dur"], 16); tile.remaining = string(extra["rem"], 16);
    tile.has_history=false;
    if (root["history"]["values"].is<JsonArray>()) {
      tile.history.fill(NAN);unsigned j=0;
      for (JsonVariant value:root["history"]["values"].as<JsonArray>()) {
        if(j==24) break; tile.history[j++]=number(value); }
      tile.has_history=j>0;tile.history_hours=std::clamp(root["history"]["hours"].as<unsigned>(),1u,24u);
    }
    tile.option_count=0;
    if (a["options"].is<JsonArray>()) for(JsonVariant option:a["options"].as<JsonArray>()) {
      if(tile.option_count==8)break;tile.options[tile.option_count++]=string(option,48); }
    tile.battery=number(a["battery_level"]);tile.volume=number(a["volume_level"]);
    tile.muted=a["is_volume_muted"].is<bool>() && a["is_volume_muted"].as<bool>();
    tile.device_class=string(a["device_class"],24);tile.hvac_action=string(a["hvac_action"],24);
    tile.media_title=string(a["media_title"],80);tile.supported=a["supported_features"].as<uint32_t>();
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
    if(tile.domain()=="number" || tile.domain()=="input_number") {
      tile.minimum=number(a["min"],0);tile.maximum=number(a["max"],100);tile.step=number(a["step"],1); }
    if(tile.domain()=="weather") {
      tile.current=number(a["temperature"]);tile.unit=string(a["temperature_unit"],12);
      tile.humidity=number(a["humidity"]);tile.wind=number(a["wind_speed"]);tile.wind_unit=string(a["wind_speed_unit"],8);tile.feels=number(a["apparent_temperature"]);
    }
    tile.modes = list(a["supported_color_modes"]);
    tile.hvac_modes = list(a["hvac_modes"]);
    tile.fan_modes = list(a["fan_modes"]); tile.swing_modes = list(a["swing_modes"]);
    tile.fan_mode = string(a["fan_mode"], 48); tile.swing_mode = string(a["swing_mode"], 48);
    float hue = number(a["hs_color"][0]);
    float saturation = number(a["hs_color"][1]);
    tile.has_hs_color = std::isfinite(hue) && std::isfinite(saturation);
    tile.saturation = tile.has_hs_color ? std::lround(std::clamp(saturation, 0.0f, 100.0f)) : 0;
    if (std::isfinite(hue)) tile.hue = std::lround(std::clamp(hue, 0.0f, 360.0f));
    float kelvin = number(a["color_temp_kelvin"]);
    if (std::isfinite(kelvin)) tile.kelvin = std::lround(std::clamp(kelvin, 1000.0f, 15000.0f));
    tile.min_kelvin = std::clamp(number(a["min_color_temp_kelvin"], 0), 0.0f, 15000.0f);
    tile.max_kelvin = std::clamp(number(a["max_color_temp_kelvin"], 0), 0.0f, 15000.0f);
    tile.fan_speed = string(a["fan_speed"], 48);
    tile.fan_speed_count = 0;
    if (a["fan_speed_list"].is<JsonArray>()) for (JsonVariant speed : a["fan_speed_list"].as<JsonArray>()) {
      if (tile.fan_speed_count == 4) break;
      tile.fan_speeds[tile.fan_speed_count++] = string(speed, 48);
    }
    // Home Assistant reports the edited value: the -/+ pill follows its state again.
    if(std::isfinite(tile.edit_value) && tile.edit_sent && std::fabs(tile_controls::edit_target(tile)-tile.edit_value)<0.051f)tile.edit_value=NAN;
    tile.received = true;
    for(auto &w:widgets)if(w.index==index)w.cached_active=-1;
    last_received = esphome::millis();
    if (repack && layout_changed) layout_changed();
    if (refresh) refresh();
    if (active_index == static_cast<int>(index) && detail_update) detail_update(tile);
    refresh_detail(index);
    result = model.ready() ? "Gesynchroniseerd" : "Tegels laden";
    return true;
  });
  return result;
}
inline void action(const std::string &service, const std::string &entity, const std::string &key="", const std::string &value="") {
  if (!fresh() || !valid_entity(entity)) return;
  esphome::api::HomeassistantActionRequest request;
  request.service = esphome::StringRef(service);
  request.data.init(key.empty() ? 1 : 2);
  esphome::api::HomeassistantServiceMap entry;
  entry.key = esphome::StringRef("entity_id");
  entry.value = esphome::StringRef(entity);
  request.data.push_back(entry);
  if(!key.empty()) {esphome::api::HomeassistantServiceMap param;param.key=esphome::StringRef(key);param.value=esphome::StringRef(value);request.data.push_back(param);}
  for(auto &tile:model.tiles) if(tile.entity==entity)tile.begin(esphome::millis());
  esphome::api::global_api_server->send_homeassistant_action(request);
  ESP_LOGI("runtime_action","Sent service=%s entity=%s",service.c_str(),entity.c_str());
  if(refresh)refresh();
}
inline void setting_event(const std::string &key, int value) {
  if(inbox.empty())return;
  esphome::api::HomeassistantActionRequest request;request.service=esphome::StringRef("esphome.screen_setting");request.is_event=true;
  std::string number=std::to_string(value);request.data.init(3);
  const std::string keys[]={"inbox","key","value"},values[]={inbox,key,number};
  for(int i=0;i<3;++i){esphome::api::HomeassistantServiceMap entry;entry.key=esphome::StringRef(keys[i]);entry.value=esphome::StringRef(values[i]);request.data.push_back(entry);}
  esphome::api::global_api_server->send_homeassistant_action(request);
}
} // namespace runtime_tiles
// Small shared native-LVGL detail cards. No images, canvas buffers or free scrolling.
namespace runtime_tiles {
inline lv_obj_t *detail_root=nullptr;
inline unsigned detail_index=0;
inline const lv_font_t *detail_font=nullptr;
inline lv_obj_t *detail_actions[16]{};
inline unsigned detail_action_count=0;
inline lv_obj_t *detail_status=nullptr;
inline lv_obj_t *detail_badge_status=nullptr;
inline lv_obj_t *detail_switch=nullptr;
inline lv_obj_t *detail_label(lv_obj_t *parent,const std::string &text,int x,int y,int width) {
  auto *label=lv_label_create(parent);lv_label_set_text(label,text.c_str());lv_obj_set_pos(label,x,y);lv_obj_set_width(label,width);
  lv_obj_set_style_text_font(label,detail_font,0);lv_obj_set_style_text_color(label,lv_color_hex(0x202020),0);
  lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);lv_obj_set_height(label,lv_font_get_line_height(detail_font));return label;
}
inline std::string detail_state(const Tile &t){
  if(t.domain()=="person")return t.state=="home"?"Thuis":t.state=="not_home"?"Niet thuis":t.state;
  if(t.domain()=="sun")return t.state=="above_horizon"?"Boven de horizon":"Onder de horizon";
  if(t.domain()=="timer")return timer_text(t);
  if(t.domain()=="script"||t.domain()=="scene"||t.domain()=="button"||t.domain()=="input_button")return t.state=="on"?"Bezig...":last_run_text(t.last_run);
  if(t.state=="on")return "Aan";
  if(t.state=="off")return "Uit";
  if(t.state=="docked")return "In dock";
  if(t.state=="cleaning")return "Bezig met schoonmaken";
  if(t.state=="paused")return "Gepauzeerd";
  if(t.state=="returning")return "Onderweg naar dock";
  if(t.state=="idle")return "Klaar";
  if(t.state=="error")return "Controleer de robot in HA";
  if(!t.available())return "Niet beschikbaar";
  return t.state;
}
inline void hide_detail(){if(detail_root)lv_obj_add_flag(detail_root,LV_OBJ_FLAG_HIDDEN);}
inline int slider_value(const Tile &t){
  auto d=t.domain();float value=0;
  if(d=="light")value=std::isfinite(t.brightness)?t.brightness/255:0;
  if(d=="fan")value=std::isfinite(t.percentage)?t.percentage/100:0;
  if(d=="cover")value=std::isfinite(t.position)?t.position/100:0;
  if(d=="media_player")value=std::isfinite(t.volume)?t.volume:0;
  if(d=="number"||d=="input_number") {char *end;float state=strtof(t.state.c_str(),&end);if(end!=t.state.c_str() && t.maximum>t.minimum)value=(state-t.minimum)/(t.maximum-t.minimum);}
  return std::clamp((int)std::lround(value*1000),0,1000);
}
inline lv_obj_t *captured_slider=nullptr;
inline bool slider_changed=false;
inline void slider_event(lv_event_t *e);
inline void commit_slider(unsigned i,int raw){
  if(i>=model.count || !fresh())return;auto &t=model.tiles[i];if(!t.available() || t.loading(esphome::millis()))return;
  float value=std::clamp(raw,0,1000)/1000.0f;auto d=t.domain();
  if(d=="light")action("light.turn_on",t.entity,"brightness",std::to_string((int)std::lround(value*255)));
  if(d=="fan")action("fan.set_percentage",t.entity,"percentage",std::to_string((int)std::lround(value*100)));
  if(d=="cover")action("cover.set_cover_position",t.entity,"position",std::to_string((int)std::lround(value*100)));
  if(d=="media_player")action("media_player.volume_set",t.entity,"volume_level",std::to_string(value));
  if(d=="number"||d=="input_number") {
    if(!std::isfinite(t.minimum)||!std::isfinite(t.maximum)||t.maximum<=t.minimum||t.step<=0)return;
    value=std::clamp(t.minimum+std::round(value*(t.maximum-t.minimum)/t.step)*t.step,t.minimum,t.maximum);
    action(d+".set_value",t.entity,"value",std::to_string(value)); }
}
inline void slider_event(lv_event_t *e){
  auto *slider=lv_event_get_target_obj(e);auto code=lv_event_get_code(e);
  if(code==LV_EVENT_PRESSED){captured_slider=slider;slider_changed=false;}
  if(code==LV_EVENT_VALUE_CHANGED && captured_slider==slider)slider_changed=true;
  if(code==LV_EVENT_PRESS_LOST && captured_slider==slider){captured_slider=nullptr;slider_changed=false;}
  if(code==LV_EVENT_RELEASED && captured_slider==slider){
    unsigned index=(uintptr_t)lv_event_get_user_data(e);
    for(auto &w:widgets)if(w.slider==slider || w.control_slider==slider){index=w.index;break;}
    bool changed=slider_changed;captured_slider=nullptr;slider_changed=false;
    if(changed && cyd::touch_guard.accept_slider(esphome::millis(),200+index))commit_slider(index,lv_slider_get_value(slider));
  }
}
inline lv_obj_t *detail_button(const char *text,int x,int y,int width,int height,int command){
  auto *button=lv_obj_create(detail_root);lv_obj_remove_style_all(button);lv_obj_set_pos(button,x,y);lv_obj_set_size(button,width,height);
  lv_obj_set_style_bg_color(button,lv_color_hex(command==0?0x009FE3:0xD9E6F0),0);lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);lv_obj_set_style_radius(button,12,0);lv_obj_add_flag(button,LV_OBJ_FLAG_CLICKABLE);
  auto *label=detail_label(button,text,6,0,width-12);lv_obj_center(label);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);if(command==0)lv_obj_set_style_text_color(label,lv_color_hex(0xFFFFFF),0);lv_obj_remove_flag(label,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(button,[](lv_event_t *e){
    int cmd=(intptr_t)lv_event_get_user_data(e);if(cmd==-1){hide_detail();return;}
    if(!fresh()||detail_index>=model.count || !allowed(esphome::millis(),300+cmd,"kaartknop "+model.tiles[detail_index].entity))return;
    auto &t=model.tiles[detail_index];if(!t.available()||t.loading(esphome::millis()))return;
    if(cmd<4){const char *services[]={"vacuum.start","vacuum.pause","vacuum.return_to_base","vacuum.locate"};action(services[cmd],t.entity);}
    if(cmd>=10 && cmd<14 && cmd-10<(int)t.fan_speed_count)action("vacuum.set_fan_speed",t.entity,"fan_speed",t.fan_speeds[cmd-10]);
    if(cmd==20)action("media_player.media_play_pause",t.entity);
    if(cmd==21)action("media_player.media_previous_track",t.entity);
    if(cmd==22)action("media_player.media_next_track",t.entity);
    if(cmd>=30 && cmd<38 && cmd-30<(int)t.option_count)action(t.domain()+".select_option",t.entity,"option",t.options[cmd-30]);
    if(cmd==40)action(t.state=="active"?"timer.pause":"timer.start",t.entity);
    if(cmd==41)action("timer.cancel",t.entity);
  },LV_EVENT_SHORT_CLICKED,(void*)(intptr_t)command);
  lv_obj_set_style_bg_color(button,lv_color_hex(0x0075B0),LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(button,-2,LV_STATE_PRESSED);lv_obj_set_style_transform_height(button,-2,LV_STATE_PRESSED);
  lv_obj_set_style_opa(button,LV_OPA_50,LV_STATE_DISABLED);
  if(command>=0 && detail_action_count<16)detail_actions[detail_action_count++]=button;
  return button;
}

// ---- Weather card: now, the next hours and the coming days, with rain ----
inline uint32_t weather_accent(const std::string &c) {
  if(c=="sunny")return 0xFFB300;
  if(c=="clear-night")return 0x6E41AB;
  if(c=="rainy"||c=="pouring"||c=="lightning-rainy"||c=="snowy-rainy")return 0x1E88E5;
  if(c=="snowy"||c=="hail")return 0x4FC3F7;
  if(c=="lightning")return 0xFFA000;
  if(c=="partlycloudy")return 0x7E9BB5;
  return 0x78909C;
}
inline lv_obj_t *detail_text(lv_obj_t *parent,const std::string &text,int x,int y,int width,const lv_font_t *font,lv_text_align_t align,uint32_t color){
  auto *l=detail_label(parent,text,x,y,std::max(1,width));lv_obj_set_style_text_font(l,font,0);lv_obj_set_height(l,lv_font_get_line_height(font));
  lv_obj_set_style_text_align(l,align,0);lv_obj_set_style_text_color(l,lv_color_hex(color),0);return l;
}
// "30%", "30% · 1.7 mm" or "1.7 mm": whatever the provider reports; empty when dry.
inline std::string rain_text(float chance,float mm,bool with_mm){
  char b[32];
  if(std::isfinite(chance) && chance>=0){
    if(with_mm && std::isfinite(mm) && mm>=0.05f){snprintf(b,sizeof(b),"%d%% · %.1f mm",(int)std::lround(chance),mm);return b;}
    snprintf(b,sizeof(b),"%d%%",(int)std::lround(chance));return b;
  }
  if(std::isfinite(mm) && mm>=0.05f){snprintf(b,sizeof(b),mm<10?"%.1f mm":"%.0f mm",mm);return b;}
  return "";
}
inline std::string degrees(float value){ if(!std::isfinite(value))return "--"; char b[16];snprintf(b,sizeof(b),"%.0f°",value);return b; }
inline lv_obj_t *detail_card(int x,int y,int w,int h){
  auto *card=lv_obj_create(detail_root);lv_obj_remove_style_all(card);lv_obj_remove_flag(card,LV_OBJ_FLAG_SCROLLABLE);lv_obj_remove_flag(card,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(card,x,y);lv_obj_set_size(card,w,h);
  lv_obj_set_style_bg_color(card,lv_color_hex(0xFFFFFF),0);lv_obj_set_style_bg_opa(card,LV_OPA_COVER,0);
  lv_obj_set_style_radius(card,lv_obj_get_style_radius(widgets[0].tile,LV_PART_MAIN),0);
  lv_obj_set_style_border_width(card,1,0);lv_obj_set_style_border_color(card,lv_color_hex(0xDDDDDD),0);
  return card;
}
// Two cards: "now" with the next hours, and the coming days. Bold highs, muted lows,
// rain in blue with a drop, so the eye finds temperature first and rain second.
inline void render_weather_detail(const Tile &t,bool large,int width,int height,int pad){
  if(detail_status){lv_obj_add_flag(detail_status,LV_OBJ_FLAG_HIDDEN);detail_status=nullptr;}
  const lv_font_t *big=watch_value_font?watch_value_font:detail_font;
  const lv_font_t *icon_font=widgets[0].icon_font?widgets[0].icon_font:detail_font;
  const lv_font_t *mini=mini_icon_font?mini_icon_font:icon_font;
  const lv_font_t *tiny=watch_icon_font?watch_icon_font:mini;
  const lv_font_t *small=widgets[0].value?lv_obj_get_style_text_font(widgets[0].value,LV_PART_MAIN):detail_font;
  uint32_t ink=0x202020,muted=0x6B6B6B,rain=0x1E88E5;
  int text_h=lv_font_get_line_height(detail_font),small_h=lv_font_get_line_height(small),mini_h=lv_font_get_line_height(mini),tiny_h=lv_font_get_line_height(tiny);
  int icon_h=lv_font_get_line_height(icon_font),big_h=lv_font_get_line_height(big),hero=std::max(icon_h,big_h);
  int card_pad=large?14:7,inner=width-2*pad-2*card_pad;
  unsigned columns=std::min<unsigned>(t.hours.size(),6);
  int hours_h=columns?small_h+mini_h+text_h+small_h+(large?16:6):0;
  int card_a_h=card_pad+hero+(columns?(large?14:8)+hours_h:0)+card_pad;
  int y=large?62:38;
  auto *now=detail_card(pad,y,width-2*pad,card_a_h);
  // Now: icon, temperature, condition, then feels-like / humidity / wind in one muted line.
  int cy=card_pad;char b[48];
  detail_text(now,t.available()?weather_icon(t.state):"\U000F0595",card_pad,cy+(hero-icon_h)/2,icon_h+8,icon_font,LV_TEXT_ALIGN_LEFT,weather_accent(t.state));
  int temp_x=card_pad+icon_h+(large?14:6),temp_w=large?92:50;
  detail_text(now,degrees(t.current),temp_x,cy+(hero-big_h)/2,temp_w,big,LV_TEXT_ALIGN_LEFT,ink);
  int text_x=temp_x+temp_w+(large?4:2),text_w=width-2*pad-card_pad-text_x;
  int lines_h=text_h+small_h+(large?2:0);
  detail_text(now,t.available()?weather_text(t.state):"Niet beschikbaar",text_x,cy+(hero-lines_h)/2,text_w,detail_font,LV_TEXT_ALIGN_LEFT,ink);
  std::string details;
  if(std::isfinite(t.feels)){snprintf(b,sizeof(b),"Voelt als %.0f°",t.feels);details=b;}
  if(std::isfinite(t.humidity)){snprintf(b,sizeof(b),"%d%%",(int)std::lround(t.humidity));details+=(details.empty()?"":" · ")+std::string(b);}
  if(std::isfinite(t.wind)){snprintf(b,sizeof(b),"%.0f %s",t.wind,t.wind_unit.empty()?"km/h":t.wind_unit.c_str());details+=(details.empty()?"":" · ")+std::string(b);}
  detail_text(now,details,text_x,cy+(hero-lines_h)/2+text_h+(large?2:0),text_w,small,LV_TEXT_ALIGN_LEFT,muted);
  // Next hours inside the same card: time, icon, temperature, rain per column.
  if(columns){
    int hy=cy+hero+(large?14:8),col=inner/(int)columns;
    for(unsigned i=0;i<columns;++i){
      const auto &h=t.hours[i];int x=card_pad+i*col;
      detail_text(now,h.time,x,hy,col,small,LV_TEXT_ALIGN_CENTER,muted);
      detail_text(now,weather_icon(h.condition),x,hy+small_h+(large?4:1),col,mini,LV_TEXT_ALIGN_CENTER,weather_accent(h.condition));
      detail_text(now,std::isfinite(h.temp)?degrees(h.temp):"",x,hy+small_h+mini_h+(large?8:2),col,detail_font,LV_TEXT_ALIGN_CENTER,ink);
      detail_text(now,rain_text(h.rain,h.mm,false),x,hy+small_h+mini_h+text_h+(large?8:3),col,small,LV_TEXT_ALIGN_CENTER,rain);
    }
  }
  y+=card_a_h+(large?12:6);
  // Coming days: a heading and a card with one row per day.
  if(!t.forecast.size()){detail_text(detail_root,"Geen dagvoorspelling van Home Assistant",pad,y,width-2*pad,small,LV_TEXT_ALIGN_LEFT,muted);return;}
  if(large){detail_text(detail_root,"Komende dagen",pad+4,y,width-2*pad,detail_font,LV_TEXT_ALIGN_LEFT,muted);y+=text_h+8;}
  int card_b_h=height-y-(large?10:4);
  auto *days=detail_card(pad,y,width-2*pad,card_b_h);
  int row_pad=large?8:4,row=(card_b_h-2*row_pad)/(int)t.forecast.size();
  int day_w=large?46:26,icon_x=card_pad+day_w,cond_x=icon_x+mini_h+(large?12:5);
  int high_w=large?52:30,low_w=large?46:28,rain_w=large?120:60,drop_w=tiny_h+(large?4:2);
  int temps_x=width-2*pad-card_pad-high_w-low_w,rain_x=temps_x-(large?14:6)-rain_w;
  for(unsigned i=0;i<t.forecast.size();++i){
    const auto &f=t.forecast[i];int ry=row_pad+i*row,tcy=ry+(row-text_h)/2,scy=ry+(row-small_h)/2;
    detail_text(days,f.day,card_pad,tcy,day_w,detail_font,LV_TEXT_ALIGN_LEFT,ink);
    detail_text(days,weather_icon(f.condition),icon_x,ry+(row-mini_h)/2,mini_h+6,mini,LV_TEXT_ALIGN_LEFT,weather_accent(f.condition));
    detail_text(days,weather_text(f.condition),cond_x,scy,std::max(1,rain_x-cond_x-4),small,LV_TEXT_ALIGN_LEFT,muted);
    std::string wet=rain_text(f.rain,f.mm,large);
    if(!wet.empty()){
      detail_text(days,"\U000F058E",rain_x,ry+(row-tiny_h)/2,drop_w,tiny,LV_TEXT_ALIGN_LEFT,rain);
      detail_text(days,wet,rain_x+drop_w,scy,rain_w-drop_w,small,LV_TEXT_ALIGN_LEFT,rain);
    }
    detail_text(days,std::isfinite(f.high)?degrees(f.high):"",temps_x,tcy,high_w,detail_font,LV_TEXT_ALIGN_RIGHT,ink);
    detail_text(days,std::isfinite(f.low)?degrees(f.low):"",temps_x+high_w,scy,low_w,small,LV_TEXT_ALIGN_RIGHT,muted);
  }
}
inline void show_detail(unsigned index){
  if(index>=model.count)return;detail_index=index;auto &t=model.tiles[index];
  if(!detail_font)detail_font=lv_obj_get_style_text_font(widgets[0].title,LV_PART_MAIN);
  if(!detail_root){detail_root=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(detail_root);lv_obj_set_size(detail_root,lv_pct(100),lv_pct(100));lv_obj_remove_flag(detail_root,LV_OBJ_FLAG_SCROLLABLE);}
  detail_action_count=0;detail_status=nullptr;detail_badge_status=nullptr;detail_switch=nullptr;lv_obj_clean(detail_root);lv_obj_remove_flag(detail_root,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(detail_root);
  lv_obj_set_style_bg_color(detail_root,lv_color_hex(0xE7E7E7),0);lv_obj_set_style_bg_opa(detail_root,LV_OPA_COVER,0);
  int width=lv_display_get_horizontal_resolution(lv_display_get_default()), height=lv_display_get_vertical_resolution(lv_display_get_default());
  bool large=width>=480;int pad=large?20:10, top=large?100:62, gap=large?12:6,bh=large?58:34,cw=(width-pad*2-gap)/2;
  auto *heading=detail_label(detail_root,t.name,pad,large?24:12,width-80);if(watch_font){lv_obj_set_style_text_font(heading,watch_font,0);lv_obj_set_height(heading,lv_font_get_line_height(watch_font));}
  detail_button("X",width-58,8,48,40,-1);
  std::string state=detail_state(t);
  detail_status=detail_label(detail_root,state+(t.unit.empty()?"":" "+t.unit),pad,large?60:35,width-2*pad);
  auto d=t.domain();
  if(t.is_switch()){
    detail_label(detail_root,"Tik om te schakelen",pad,top,width-2*pad);
    detail_switch=lv_switch_create(detail_root);
    lv_obj_set_size(detail_switch,large?240:140,large?112:64);
    lv_obj_set_pos(detail_switch,(width-(large?240:140))/2,top+(large?65:30));
    lv_obj_set_style_bg_color(detail_switch,lv_color_hex(0xB0B0B0),LV_PART_MAIN);
    lv_obj_set_style_bg_color(detail_switch,lv_color_hex(0xFFB900),LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(detail_switch,lv_color_hex(0xFFFFFF),LV_PART_KNOB);
    lv_obj_set_style_opa(detail_switch,LV_OPA_50,LV_STATE_DISABLED);
    if(t.state=="on")lv_obj_add_state(detail_switch,LV_STATE_CHECKED);
    lv_obj_add_event_cb(detail_switch,[](lv_event_t *e){
      if(detail_index>=model.count)return;
      auto &tile=model.tiles[detail_index];auto *control=lv_event_get_target_obj(e);
      bool allowed=fresh() && tile.available() && !tile.awaiting_action(esphome::millis()) &&
        cyd::touch_guard.accept(esphome::millis(),350);
      bool requested_on=lv_obj_has_state(control,LV_STATE_CHECKED);
      // Only HA's reported state is authoritative, including a refused/failed command.
      if(tile.state=="on")lv_obj_add_state(control,LV_STATE_CHECKED);else lv_obj_remove_state(control,LV_STATE_CHECKED);
      if(allowed)action(tile.domain()+(requested_on?".turn_on":".turn_off"),tile.entity);
    },LV_EVENT_VALUE_CHANGED,nullptr);
    detail_actions[detail_action_count++]=detail_switch;
  }else if(d=="vacuum"){
    // Native shapes keep the robot crisp without image buffers or extra layers.
    auto shape=[&](lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius){
      auto *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
      lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
      lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
    };
    uint32_t surface=0xFFFFFF, muted=0xEAF5FC;
    if(large){
      auto *hero=shape(detail_root,pad,100,width-2*pad,148,surface,24);
      shape(hero,18,14,120,120,muted,60);
      auto *robot=shape(hero,35,27,86,86,0xFFFFFF,43);
      lv_obj_set_style_border_width(robot,2,0);lv_obj_set_style_border_color(robot,lv_color_hex(0xCEDDE6),0);
      shape(robot,27,10,30,30,0xE3EBEF,15);shape(robot,35,18,14,14,0xA8BCC8,7);
      shape(robot,29,59,26,5,0x00A6ED,3);
      detail_label(hero,t.state=="cleaning"?"Aan het werk":t.state=="returning"?"Even opladen":t.state=="paused"?"Even pauze":"Klaar voor je huis",154,24,260);
      auto *badge=shape(hero,154,59,240,32,muted,16);
      auto *status=detail_label(badge,t.awaiting_action(esphome::millis())?"Opdracht verstuurd...":state,12,5,218);
      detail_badge_status=status;
      lv_obj_set_style_text_color(status,lv_color_hex(0x087BA8),0);
      detail_label(hero,std::isfinite(t.battery)?"Batterij  "+std::to_string((int)t.battery)+"%":"Verbonden via Home Assistant",154,107,260);
      detail_button(t.state=="cleaning"?"Pauzeer schoonmaken":"Start schoonmaken",pad,260,width-2*pad,58,t.state=="cleaning"?1:0);
      detail_button("Terug naar dock",pad,330,cw,48,2);
      detail_button("Vind mijn robot",pad+cw+gap,330,cw,48,3);
      detail_label(detail_root,"Zuigkracht",pad,394,width-2*pad);
      top=424;
    }else{
      auto *robot=shape(detail_root,pad,64,46,46,muted,23);
      shape(robot,16,8,14,14,0xA8BCC8,7);shape(robot,15,32,16,3,0x00A6ED,2);
      detail_label(detail_root,std::isfinite(t.battery)?"Batterij "+std::to_string((int)t.battery)+"%":"Robotstofzuiger",pad+58,66,width-2*pad-58);
      detail_label(detail_root,"Kies een actie",pad+58,87,width-2*pad-58);
      detail_button(t.state=="cleaning"?"Pauzeren":"Schoonmaken",pad,120,cw,38,t.state=="cleaning"?1:0);
      detail_button("Naar dock",pad+cw+gap,120,cw,38,2);
      detail_label(detail_root,"Zuigkracht",pad,168,width-2*pad);top=194;
    }
    if(!t.fan_speed_count)detail_label(detail_root,"Automatische zuigkracht",pad,top,width-2*pad);
    int count=std::max(1,(int)t.fan_speed_count),sw=(width-2*pad-gap*(count-1))/count;
    for(unsigned i=0;i<t.fan_speed_count;++i){
      std::string name=t.fan_speeds[i];
      if(name=="quiet")name="Stil";else if(name=="balanced")name="Normaal";else if(name=="turbo")name="Turbo";else if(name=="max")name="Max";
      auto *button=detail_button(name.c_str(),pad+i*(sw+gap),top,sw,large?36:30,10+i);
      bool selected=t.fan_speed==t.fan_speeds[i];
      lv_obj_set_style_bg_color(button,lv_color_hex(selected?0x009FE3:surface),0);
      if(selected)lv_obj_set_style_text_color(lv_obj_get_child(button,0),lv_color_hex(0xFFFFFF),0);
    }
  }else if(d=="sensor"){
    float minimum=INFINITY,maximum=-INFINITY;for(float value:t.history)if(t.has_history&&std::isfinite(value)){minimum=std::min(minimum,value);maximum=std::max(maximum,value);}
    if(!std::isfinite(minimum)){detail_label(detail_root,"Geen numerieke HA-historie",pad,top,width-2*pad);return;}
    char text[100];snprintf(text,sizeof(text),"%u uur / %.2f - %.2f %s",t.history_hours,minimum,maximum,t.unit.c_str());detail_label(detail_root,text,pad,top,width-2*pad);
    int chart_y=top+(large?46:28),chart_h=height-chart_y-30,bar_w=(width-pad*2)/24;
    for(unsigned i=0;i<24;++i){if(!std::isfinite(t.history[i]))continue;int h=maximum>minimum?8+(chart_h-8)*(t.history[i]-minimum)/(maximum-minimum):chart_h/2;
      auto *bar=lv_obj_create(detail_root);lv_obj_remove_style_all(bar);lv_obj_set_pos(bar,pad+i*bar_w,chart_y+chart_h-h);lv_obj_set_size(bar,std::max(2,bar_w-2),h);lv_obj_set_style_bg_color(bar,lv_color_hex(0x16A5E6),0);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,0);}
    detail_label(detail_root,std::to_string(t.history_hours)+" uur geleden",pad,height-24,(width-2*pad)/2);
    auto *now=detail_label(detail_root,"Nu",width/2,height-24,width/2-pad);lv_obj_set_style_text_align(now,LV_TEXT_ALIGN_RIGHT,0);
  }else if(d=="select"||d=="input_select"){
    for(unsigned i=0;i<t.option_count;++i)detail_button(t.options[i].c_str(),pad+(i%2)*(cw+gap),top+(i/2)*(bh+gap),cw,bh,30+i);
  }else if(d=="number"||d=="input_number"||d=="media_player"){
    if(d=="media_player"){
      detail_label(detail_root,t.media_title,pad,top,width-2*pad);top+=large?45:25;
      int w=(width-pad*2-2*gap)/3;
      detail_button("Vorige",pad,top,w,bh,21);detail_button("Play/pauze",pad+w+gap,top,w,bh,20);detail_button("Volgende",pad+2*(w+gap),top,w,bh,22);top+=bh+gap;
    }
    detail_label(detail_root,d=="media_player"?"Volume":"Waarde",pad,top,width-2*pad);
    auto *slider=lv_slider_create(detail_root);lv_obj_set_pos(slider,pad+12,top+(large?52:34));lv_obj_set_size(slider,width-2*pad-24,large?24:16);lv_slider_set_range(slider,0,1000);lv_slider_set_value(slider,slider_value(t),LV_ANIM_OFF);lv_obj_set_style_bg_color(slider,lv_color_hex(0x111111),LV_PART_KNOB);
    lv_obj_add_event_cb(slider,slider_event,LV_EVENT_ALL,(void*)(uintptr_t)index);
  }else if(d=="weather"){
    render_weather_detail(t,large,width,height,pad);
  }else if(d=="timer"){
    detail_label(detail_root,t.state=="active"?"Loopt":t.state=="paused"?"Gepauzeerd":"Staat stil",pad,top,width-2*pad);
    detail_button(t.state=="active"?"Pauzeer":"Start",pad,top+(large?50:30),cw,bh,40);
    detail_button("Annuleer",pad+cw+gap,top+(large?50:30),cw,bh,41);
  }else if(d=="sun"){
    detail_label(detail_root,"Zonsopgang "+t.sunrise,pad,top,width-2*pad);
    detail_label(detail_root,"Zonsondergang "+t.sunset,pad,top+lv_font_get_line_height(detail_font)+(large?10:4),width-2*pad);
  }
}
}

namespace runtime_tiles {
inline void refresh_detail(unsigned index){
  if(!detail_root || lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN) || detail_index!=index)return;
  auto *input=lv_indev_get_next(nullptr);if(input && lv_indev_get_state(input)==LV_INDEV_STATE_PRESSED)return;
  show_detail(index);
}
// Home Assistant weather conditions mapped to Material Design Icons glyphs.
inline const char *weather_icon(const std::string &condition) {
  if (condition == "sunny") return "\U000F0599";
  if (condition == "clear-night") return "\U000F0594";
  if (condition == "cloudy") return "\U000F0590";
  if (condition == "partlycloudy") return "\U000F0595";
  if (condition == "rainy") return "\U000F0597";
  if (condition == "pouring") return "\U000F0596";
  if (condition == "snowy") return "\U000F0598";
  if (condition == "snowy-rainy") return "\U000F067F";
  if (condition == "fog") return "\U000F0591";
  if (condition == "hail") return "\U000F0592";
  if (condition == "lightning") return "\U000F0593";
  if (condition == "lightning-rainy") return "\U000F067E";
  if (condition == "windy" || condition == "windy-variant") return "\U000F059D";
  if (condition == "exceptional") return "\U000F05D6";
  return "\U000F0595";
}
inline const char *weather_text(const std::string &condition) {
  if (condition == "sunny") return "Zonnig";
  if (condition == "clear-night") return "Heldere nacht";
  if (condition == "cloudy") return "Bewolkt";
  if (condition == "partlycloudy") return "Half bewolkt";
  if (condition == "rainy") return "Regen";
  if (condition == "pouring") return "Stortregen";
  if (condition == "snowy") return "Sneeuw";
  if (condition == "snowy-rainy") return "Natte sneeuw";
  if (condition == "fog") return "Mist";
  if (condition == "hail") return "Hagel";
  if (condition == "lightning") return "Onweer";
  if (condition == "lightning-rainy") return "Onweer en regen";
  if (condition == "windy" || condition == "windy-variant") return "Winderig";
  if (condition == "exceptional") return "Bijzonder weer";
  return condition.c_str();
}
inline const char *icon_for(const Tile &tile) {
  if (!tile.icon.empty()) return tile.icon.c_str();
  auto d = tile.domain();
  if (d == "light") return "\U000F0335";
  if (d == "climate") return "\U000F001B";
  if (d == "vacuum") return "\U000F070D";
  if (d == "fan") return "\U000F0210";
  if (d == "cover") return "\U000F111C";
  if (d == "scene" || d == "script") return "\U000F04B9";
  if (d == "weather") return tile.available() ? weather_icon(tile.state) : "\U000F0595";
  if (d == "sensor" || d == "binary_sensor") return "\U000F029A";
  if (d == "sun") return tile.state == "above_horizon" ? "\U000F059B" : "\U000F059C";
  if (d == "timer") return "\U000F051B";
  if (d == "person") return "\U000F0004";
  if (d == "screen") return "\U000F0150";
  return "\U000F0425";
}
// HA duration strings ("0:05:00") to seconds; 0 when unusable.
inline uint32_t duration_seconds(const std::string &text) {
  unsigned h = 0, m = 0, s = 0;
  if (sscanf(text.c_str(), "%u:%u:%u", &h, &m, &s) == 3) return h * 3600 + m * 60 + s;
  if (sscanf(text.c_str(), "%u:%u", &m, &s) == 2) return m * 60 + s;
  return 0;
}
inline std::string countdown(uint32_t seconds) {
  char b[16];
  if (seconds >= 3600) snprintf(b, sizeof(b), "%u:%02u:%02u", seconds / 3600, seconds / 60 % 60, seconds % 60);
  else snprintf(b, sizeof(b), "%u:%02u", seconds / 60, seconds % 60);
  return b;
}
// "Laatst 14:32" today, "Gisteren 14:32", else "Laatst 13 sep"; scripts and scenes have no useful on/off.
inline std::string month_short(const esphome::ESPTime &now);
inline std::string last_run_text(uint32_t epoch) {
  if (!epoch) return "Nog niet gestart";
  auto when = esphome::ESPTime::from_epoch_local(epoch);
  auto now = now_time ? now_time() : esphome::ESPTime{};
  if (!when.is_valid()) return "Nog niet gestart";
  char clock[8]; snprintf(clock, sizeof(clock), "%02d:%02d", when.hour, when.minute);
  if (now.is_valid() && now.year == when.year && now.day_of_year == when.day_of_year) return std::string("Laatst ") + clock;
  if (now.is_valid() && now.year == when.year && now.day_of_year == when.day_of_year + 1) return std::string("Gisteren ") + clock;
  return "Laatst " + std::to_string(when.day_of_month) + " " + month_short(when);
}
inline std::string timer_text(const Tile &t) {
  if (t.state == "active") { uint32_t now = now_epoch(); return countdown(t.timer_end > now && now ? t.timer_end - now : 0); }
  if (t.state == "paused") return "Pauze " + countdown(duration_seconds(t.remaining));
  return t.duration.empty() ? "Uit" : countdown(duration_seconds(t.duration));
}
inline std::string weekday_text(const esphome::ESPTime &now) {
  static const char *days[] = {"zondag", "maandag", "dinsdag", "woensdag", "donderdag", "vrijdag", "zaterdag"};
  return now.is_valid() && now.day_of_week >= 1 && now.day_of_week <= 7 ? days[now.day_of_week - 1] : "";
}
inline std::string month_short(const esphome::ESPTime &now) {
  static const char *months[] = {"jan", "feb", "mrt", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec"};
  return now.is_valid() && now.month >= 1 && now.month <= 12 ? months[now.month - 1] : "";
}
inline std::string date_text(const esphome::ESPTime &now) {
  static const char *months[] = {"januari", "februari", "maart", "april", "mei", "juni", "juli", "augustus", "september", "oktober", "november", "december"};
  if (!now.is_valid() || now.day_of_week < 1 || now.day_of_week > 7 || now.month < 1 || now.month > 12) return "";
  return weekday_text(now) + " " + std::to_string(now.day_of_month) + " " + months[now.month - 1];
}
inline void event(lv_event_t *event) {
  auto &w = *static_cast<Widgets *>(lv_event_get_user_data(event));
  if (!enabled || !fresh() || w.index >= model.count) return;
  auto code = lv_event_get_code(event);
  if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED) return;
  if (!allowed(esphome::millis(), 100 + w.index, model.tiles[w.index].entity)) return;
  auto &tile = model.tiles[w.index];
  auto d = tile.domain();
  // Scenes/scripts often have timestamps or 'off'; unavailable devices never act.
  if (!tile.available() || tile.loading(esphome::millis()) || tile.tap=="none") return;
  bool open = code == LV_EVENT_LONG_PRESSED || d == "climate" || d == "vacuum" || d == "cover";
  if(code==LV_EVENT_SHORT_CLICKED && tile.tap=="detail")open=true;
  if(code==LV_EVENT_SHORT_CLICKED && tile.tap=="toggle")open=false;
  if(d=="media_player" && tile.tap=="toggle" && code==LV_EVENT_SHORT_CLICKED){action("media_player.toggle",tile.entity);return;}
  if(d=="climate" && tile.tap=="toggle" && code==LV_EVENT_SHORT_CLICKED){action("climate.toggle",tile.entity);return;}
  if(tile.builtin())return;
  // A short tap runs or pauses the kitchen timer; holding opens the card with a cancel button.
  if(d=="timer" && !open){action(tile.state=="active"?"timer.pause":"timer.start",tile.entity);return;}
  if(d=="sensor" || d=="binary_sensor" || d=="weather" || d=="number" || d=="input_number" || d=="select" || d=="input_select" || d=="media_player" || d=="vacuum" || d=="sun" || d=="person" || d=="timer") { tile.begin(esphome::millis(),true); active_index=w.index; show_detail(w.index); return; }
  if (open) {
    if (d == "light" || d == "climate" || d == "vacuum" || d == "fan" || d == "cover") {
      active_index = w.index;
      tile.begin(esphome::millis(),true);
      if (detail) detail(tile);
    } else { active_index=w.index;show_detail(w.index); }
    return;
  }
  if (d == "light" || d == "switch" || d == "input_boolean" || d == "fan") action(d + ".toggle", tile.entity);
  if (d == "scene" || d == "script") action(d + ".turn_on", tile.entity);
  if (d == "button" || d == "input_button") action(d + ".press", tile.entity);
}
inline void bind(size_t index, lv_obj_t *tile, lv_obj_t *title, lv_obj_t *value, lv_obj_t *circle, lv_obj_t *icon) {
  lv_obj_update_layout(tile);
  // Compact cards need room for two text lines and a separate dimmer track.
  if(lv_obj_get_height(tile)<=80){lv_obj_set_style_pad_top(tile,4,0);lv_obj_set_style_pad_bottom(tile,4,0);}
  lv_obj_set_style_border_width(tile,1,0);
  lv_obj_update_layout(tile);
  widgets[index] = {tile, title, value, circle, icon, index};
  auto &w=widgets[index]; w.title_x=lv_obj_get_x(title);w.title_y=lv_obj_get_y(title);w.value_x=lv_obj_get_x(value);w.value_y=lv_obj_get_y(value);w.value_font=lv_obj_get_style_text_font(value,LV_PART_MAIN);
  w.base_width=lv_obj_get_width(tile);
  w.icon_font=lv_obj_get_style_text_font(icon,LV_PART_MAIN);
  w.unit=lv_label_create(tile);lv_obj_set_style_text_font(w.unit,w.value_font,0);lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
  // Fixed one-line boxes prevent wrapped names from overlapping the state on both boards.
  lv_obj_set_height(title,lv_font_get_line_height(lv_obj_get_style_text_font(title,LV_PART_MAIN)));
  lv_obj_set_height(value,lv_font_get_line_height(w.value_font));
  lv_label_set_long_mode(title,LV_LABEL_LONG_DOT);lv_label_set_long_mode(value,LV_LABEL_LONG_DOT);
  w.progress=lv_obj_create(tile);lv_obj_remove_style_all(w.progress);lv_obj_set_size(w.progress,0,3);lv_obj_align(w.progress,LV_ALIGN_BOTTOM_LEFT,0,0);lv_obj_add_flag(w.progress,LV_OBJ_FLAG_HIDDEN);
  w.slider=lv_slider_create(tile);lv_obj_set_size(w.slider,lv_obj_get_width(tile)-24,lv_obj_get_height(tile)>80?28:10);lv_obj_align(w.slider,LV_ALIGN_BOTTOM_MID,0,0);lv_slider_set_range(w.slider,0,1000);
  // A short white bar inside the fill as handle, like the control sliders (invisible before 0.2.20).
  int strip=lv_obj_get_height(tile)>80?28:10;
  lv_obj_set_style_bg_color(w.slider,lv_color_hex(0xFFFFFF),LV_PART_KNOB);lv_obj_set_style_bg_opa(w.slider,LV_OPA_COVER,LV_PART_KNOB);
  lv_obj_set_style_pad_hor(w.slider,-(strip/2-(strip>20?3:2)),LV_PART_KNOB);lv_obj_set_style_pad_ver(w.slider,-(strip/4),LV_PART_KNOB);
  lv_obj_set_style_border_width(w.slider,0,LV_PART_KNOB);lv_obj_set_style_shadow_width(w.slider,0,LV_PART_KNOB);
  lv_obj_set_style_radius(w.slider,14,LV_PART_MAIN);lv_obj_set_style_radius(w.slider,14,LV_PART_INDICATOR);lv_obj_set_style_radius(w.slider,2,LV_PART_KNOB);lv_obj_set_style_bg_color(w.slider,lv_color_hex(0xFCE5B4),LV_PART_MAIN);lv_obj_set_style_bg_color(w.slider,lv_color_hex(0xFFB900),LV_PART_INDICATOR);
  lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(w.slider,LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_event_cb(w.slider,slider_event,LV_EVENT_ALL,(void*)(uintptr_t)index);
  lv_obj_add_event_cb(tile, event, LV_EVENT_SHORT_CLICKED, &widgets[index]);
  lv_obj_add_event_cb(tile, event, LV_EVENT_LONG_PRESSED, &widgets[index]);
}
inline void label(lv_obj_t *obj, const std::string &text) {
  if (text != lv_label_get_text(obj)) lv_label_set_text(obj, text.c_str());
}
// lv_obj_set_style_* always invalidates the object; these only do so on a real change,
// which keeps a one-second clock tick or a busy spinner from redrawing whole cards.
inline void set_font(lv_obj_t *obj, const lv_font_t *value) { if (lv_obj_get_style_text_font(obj, LV_PART_MAIN) != value) lv_obj_set_style_text_font(obj, value, 0); }
inline void set_text_align(lv_obj_t *obj, lv_text_align_t value) { if (lv_obj_get_style_text_align(obj, LV_PART_MAIN) != value) lv_obj_set_style_text_align(obj, value, 0); }
inline void pad_vertical(lv_obj_t *obj, int value) {
  if (lv_obj_get_style_pad_top(obj, LV_PART_MAIN) != value) lv_obj_set_style_pad_top(obj, value, 0);
  if (lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN) != value) lv_obj_set_style_pad_bottom(obj, value, 0);
}
inline void set_line_width(lv_obj_t *obj, int value) { if (lv_obj_get_style_line_width(obj, LV_PART_MAIN) != value) lv_obj_set_style_line_width(obj, value, 0); }
// HA's default domain/state palette. Pastel circles also identify inactive domains.
// Custom Lovelace card/theme CSS is not an entity attribute and is not imported.
inline uint32_t domain_accent(const Tile &t) {
  auto d=t.domain();
  if(d=="light" || d=="switch" || d=="input_boolean" || d=="binary_sensor")return 0xFFC107;
  if(d=="climate"){
    if(t.state=="cool")return 0x2196F3;
    if(t.state=="fan_only")return 0x00BCD4;
    if(t.state=="auto")return 0x4CAF50;
    if(t.state=="heat_cool")return 0xFFC107;
    return t.state=="heat"?0xFF6F22:0xFF9800;
  }
  if(d=="vacuum")return t.state=="error"?0xF44336:0x009688;
  if(d=="fan")return 0x00BCD4;
  if(d=="cover")return 0x926BC7;
  if(d=="media_player")return 0x03A9F4;
  if(d=="scene" || d=="script")return 0x926BC7;
  if(d=="select" || d=="input_select")return 0x3F51B5;
  if(d=="number" || d=="input_number")return 0x009688;
  if(d=="weather")return t.state=="sunny"?0xFFC107:t.state=="clear-night"?0x6E41AB:0x03A9F4;
  if(d=="sun")return t.state=="above_horizon"?0xFF9800:0x6E41AB;
  if(d=="timer")return t.state=="active"?0x009688:t.state=="paused"?0xFF9800:0x9E9E9E;
  if(d=="person")return t.state=="home"?0x4CAF50:0x9E9E9E;
  if(d=="screen")return 0x2196F3;
  if(d=="sensor"){
    if(t.unit=="lx")return 0xFFC107;
    if(t.unit=="°C" || t.unit=="°F")return 0xFF6F22;
    if(t.unit=="kWh" || t.unit=="Wh")return 0x926BC7;
    if(t.unit=="%")return 0x009688;
  }
  return 0x2196F3;
}
// Custom cards draw into a transparent `extra` container; parts are rebuilt only
// when a slot changes mode, so paging keeps RAM use flat on the CYD.
inline void end_extra(Widgets &w) {
  if(!w.extra)return;
  lv_obj_add_flag(w.extra,LV_OBJ_FLAG_HIDDEN);
  w.fill_points=nullptr;w.fill_count=0;
  if(!w.extra_mode.empty()){lv_obj_clean(w.extra);w.parts.fill(nullptr);w.extra_mode.clear();delete[] w.points;w.points=nullptr;}
}
// Two triangles per segment between the polyline and its baseline. No canvas
// buffer is needed, so the CYD can afford it as well.
inline void extra_draw(lv_event_t *e) {
  auto &w=*static_cast<Widgets *>(lv_event_get_user_data(e));
  if(!w.fill_points || w.fill_count<2 || !w.fill_opa)return;
  auto *layer=lv_event_get_layer(e);lv_area_t area;lv_obj_get_coords(w.extra,&area);
  lv_draw_triangle_dsc_t dsc;lv_draw_triangle_dsc_init(&dsc);dsc.color=w.fill_color;dsc.opa=w.fill_opa;
  const lv_value_precise_t ox=area.x1+w.fill_x, oy=area.y1+w.fill_y, base=oy+w.fill_base;
  for(unsigned i=0;i+1<w.fill_count;++i){
    const auto &a=w.fill_points[i],&b=w.fill_points[i+1];
    dsc.p[0]={ox+a.x,oy+a.y};dsc.p[1]={ox+b.x,oy+b.y};dsc.p[2]={ox+b.x,base};lv_draw_triangle(layer,&dsc);
    dsc.p[1]=dsc.p[2];dsc.p[2]={ox+a.x,base};lv_draw_triangle(layer,&dsc);
  }
}
inline void begin_extra(Widgets &w,const char *mode,int width,int height) {
  if(!w.extra){
    w.extra=lv_obj_create(w.tile);lv_obj_remove_style_all(w.extra);lv_obj_remove_flag(w.extra,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(w.extra,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(w.extra,extra_draw,LV_EVENT_DRAW_MAIN,&w);
  }
  if(w.extra_mode!=mode){end_extra(w);w.extra_mode=mode;w.points=new lv_point_precise_t[POINT_BUFFER];w.cached_active=-1;}
  w.fill_points=nullptr;w.fill_count=0;
  lv_obj_set_pos(w.extra,0,0);lv_obj_set_size(w.extra,width,height);lv_obj_remove_flag(w.extra,LV_OBJ_FLAG_HIDDEN);
}
// Catmull-Rom curve through the samples: the trend reads smoothly without extra data.
inline unsigned smooth(const lv_point_precise_t *in,unsigned n,lv_point_precise_t *out,unsigned capacity,int width,int height) {
  if(n<2 || capacity<2){for(unsigned i=0;i<n && i<capacity;++i)out[i]=in[i];return n<capacity?n:capacity;}
  const unsigned steps=4;unsigned count=0;
  for(unsigned i=0;i+1<n;++i){
    const auto &p0=in[i?i-1:0],&p1=in[i],&p2=in[i+1],&p3=in[i+2<n?i+2:n-1];
    for(unsigned s=0;s<steps && count<capacity-1;++s){
      float t=float(s)/steps,t2=t*t,t3=t2*t;
      float x=0.5f*(2*p1.x+(-p0.x+p2.x)*t+(2*p0.x-5*p1.x+4*p2.x-p3.x)*t2+(-p0.x+3*p1.x-3*p2.x+p3.x)*t3);
      float y=0.5f*(2*p1.y+(-p0.y+p2.y)*t+(2*p0.y-5*p1.y+4*p2.y-p3.y)*t2+(-p0.y+3*p1.y-3*p2.y+p3.y)*t3);
      out[count++]={(lv_value_precise_t)std::clamp(x,0.0f,float(width-1)),(lv_value_precise_t)std::clamp(y,0.0f,float(height-1))};
    }
  }
  out[count++]=in[n-1];return count;
}
inline lv_obj_t *part_label(Widgets &w,unsigned i,const lv_font_t *font,int x,int y,int width,lv_text_align_t align,const std::string &text) {
  auto *&p=w.parts[i];
  if(!p){p=lv_label_create(w.extra);lv_label_set_long_mode(p,LV_LABEL_LONG_CLIP);lv_obj_remove_flag(p,LV_OBJ_FLAG_CLICKABLE);}
  set_font(p,font);set_text_align(p,align);
  lv_obj_set_pos(p,x,y);lv_obj_set_size(p,std::max(1,width),lv_font_get_line_height(font));label(p,text);return p;
}
inline lv_obj_t *part_dot(Widgets &w,unsigned i,int x,int y,int size) {
  auto *&p=w.parts[i];
  if(!p){p=lv_obj_create(w.extra);lv_obj_remove_style_all(p);lv_obj_set_style_bg_opa(p,LV_OPA_COVER,0);lv_obj_set_style_radius(p,LV_RADIUS_CIRCLE,0);lv_obj_remove_flag(p,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(p,LV_OBJ_FLAG_SCROLLABLE);}
  lv_obj_set_pos(p,x,y);lv_obj_set_size(p,size,size);return p;
}
// Points are relative to (x,y): the line object then covers only its own rectangle.
inline lv_obj_t *part_line(Widgets &w,unsigned i,lv_point_precise_t *points,unsigned count,int width,int x=0,int y=0) {
  auto *&p=w.parts[i];
  if(!p){p=lv_line_create(w.extra);lv_obj_remove_flag(p,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_line_rounded(p,true,0);}
  set_line_width(p,width);lv_line_set_points(p,points,count);lv_obj_set_pos(p,x,y);return p;
}
inline std::string time_text(esphome::ESPTime now) {
  return now.is_valid() ? now.strftime(screen_settings::current.clock_24h ? "%H:%M" : "%I:%M") : "--:--";
}
// Digital: big time over the date. Analog: index strokes (numerals at 12/3/6/9 on
// large cards) with hour and minute hands. A single card adds a calendar block
// beside the dial (weekday, big day number, short month); a wide card adds the
// digital time and the date instead. Parts: 0-11 marks, 12-13 hands, 14 centre,
// 15-17 text.
inline void render_clock(Widgets &w,const Tile &t,bool large,int width,int height) {
  bool analog=t.display=="analog";
  begin_extra(w,analog?(w.wide?"analog":"calendar"):"digital",width,height);
  auto now=now_time?now_time():esphome::ESPTime{};
  const lv_font_t *big=clock_font?clock_font:watch_value_font?watch_value_font:w.value_font;
  const lv_font_t *small=lv_obj_get_style_text_font(w.title,LV_PART_MAIN);
  // The date line only appears when both lines fit the card height.
  bool with_date=lv_font_get_line_height(big)+2+lv_font_get_line_height(small)<=height;
  int text_h=lv_font_get_line_height(big)+(with_date?2+lv_font_get_line_height(small):0);
  if(!analog){
    int y=std::max(0,(height-text_h)/2);
    part_label(w,15,big,0,y,width,LV_TEXT_ALIGN_CENTER,time_text(now));
    part_label(w,16,small,0,with_date?y+lv_font_get_line_height(big)+2:y,width,LV_TEXT_ALIGN_CENTER,with_date?date_text(now):"");
    return;
  }
  // The dial keeps the same size with or without a card behind it.
  int dial=std::min(height,width),cx=dial/2,cy=height/2,outer=dial/2-1,radius=dial/2-(large?4:2);
  for(int i=0;i<12;++i){
    float a=i*3.14159265f/6;bool cardinal=i%3==0;
    if(cardinal && large){
      // Numerals replace the four cardinal strokes; the box is one line high and wide.
      int box=lv_font_get_line_height(w.value_font)+4,ring=outer-11;
      part_label(w,i,w.value_font,cx+std::lround(ring*sinf(a))-box/2,cy-std::lround(ring*cosf(a))-box/2,box,LV_TEXT_ALIGN_CENTER,i==0?"12":std::to_string(i));
      continue;
    }
    int length=cardinal?(large?9:5):(large?5:3);
    auto *p=w.points+4+2*i;
    p[0]={(lv_value_precise_t)(cx+outer*sinf(a)),(lv_value_precise_t)(cy-outer*cosf(a))};
    p[1]={(lv_value_precise_t)(cx+(outer-length)*sinf(a)),(lv_value_precise_t)(cy-(outer-length)*cosf(a))};
    part_line(w,i,p,2,cardinal?(large?3:2):(large?2:1));
  }
  float hour=((now.is_valid()?now.hour%12:0)+(now.is_valid()?now.minute:0)/60.0f)*3.14159265f/6, minute=(now.is_valid()?now.minute:0)*3.14159265f/30;
  w.points[0]={(lv_value_precise_t)cx,(lv_value_precise_t)cy};w.points[1]={(lv_value_precise_t)(cx+radius*0.52f*sinf(hour)),(lv_value_precise_t)(cy-radius*0.52f*cosf(hour))};
  w.points[2]={(lv_value_precise_t)cx,(lv_value_precise_t)cy};w.points[3]={(lv_value_precise_t)(cx+radius*0.82f*sinf(minute)),(lv_value_precise_t)(cy-radius*0.82f*cosf(minute))};
  part_line(w,12,w.points,2,large?5:3);part_line(w,13,w.points+2,2,large?3:2);
  int center=large?8:4;part_dot(w,14,cx-center/2,cy-center/2,center);
  std::string day=now.is_valid()?std::to_string(now.day_of_month):"--";
  if(w.wide){
    int x=dial+(large?16:8),y=std::max(0,(height-text_h)/2);
    part_label(w,15,big,x,y,width-x,LV_TEXT_ALIGN_CENTER,time_text(now));
    part_label(w,16,small,x,with_date?y+lv_font_get_line_height(big)+2:y,width-x,LV_TEXT_ALIGN_CENTER,with_date?date_text(now):"");
    return;
  }
  int x=dial+(large?10:6),room=std::max(1,width-x);
  if(!large){
    // Compact cards: "13 sep" in the large-value font beside the dial.
    const lv_font_t *font=watch_value_font?watch_value_font:w.value_font;
    part_label(w,16,font,x,std::max(0,int(height-lv_font_get_line_height(font))/2),room,LV_TEXT_ALIGN_CENTER,day+" "+month_short(now));
    return;
  }
  // Calendar block: weekday over a big day number with the short month beside it.
  int top=std::max(0,int(height-lv_font_get_line_height(w.value_font)-lv_font_get_line_height(big))/2);
  part_label(w,15,w.value_font,x,top,room,LV_TEXT_ALIGN_CENTER,weekday_text(now));
  std::string month=month_short(now);
  lv_point_t day_size,month_size;
  lv_text_get_size(&day_size,day.c_str(),big,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  lv_text_get_size(&month_size,month.c_str(),small,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
  int gap=6,sx=x+std::max(0,int(room-(day_size.x+gap+month_size.x))/2),day_y=top+int(lv_font_get_line_height(w.value_font));
  part_label(w,16,big,sx,day_y,std::min(room,(int)day_size.x+2),LV_TEXT_ALIGN_LEFT,day);
  // Both baselines line up: LVGL measures base_line from the bottom of the line box.
  int month_y=day_y+(big->line_height-big->base_line)-(small->line_height-small->base_line);
  part_label(w,17,small,sx+day_size.x+gap,std::max(0,month_y),std::max(1,int(x+room-(sx+day_size.x+gap))),LV_TEXT_ALIGN_LEFT,month);
}
// Current conditions on the left, five day columns on the right (wide cards only).
inline void render_forecast(Widgets &w,const Tile &t,bool large,int width,int height) {
  begin_extra(w,"forecast",width,height);
  const lv_font_t *title_font=lv_obj_get_style_text_font(w.title,LV_PART_MAIN);
  const lv_font_t *temp_font=watch_value_font?watch_value_font:w.value_font,*day_icon=mini_icon_font?mini_icon_font:w.icon_font;
  int left=large?150:96,icon_h=lv_font_get_line_height(w.icon_font),temp_h=lv_font_get_line_height(temp_font),text_h=lv_font_get_line_height(w.value_font);
  int block=std::max(icon_h,temp_h)+2+text_h,y=std::max(0,(height-block)/2);
  char b[24];snprintf(b,sizeof(b),"%.0f°",t.current);
  auto *icon=part_label(w,0,w.icon_font,0,y+(std::max(icon_h,temp_h)-icon_h)/2,icon_h+4,LV_TEXT_ALIGN_LEFT,t.available()?weather_icon(t.state):"\U000F0595");
  lv_obj_set_width(icon,lv_font_get_line_height(w.icon_font)+4);
  part_label(w,1,temp_font,icon_h+6,y+(std::max(icon_h,temp_h)-temp_h)/2,left-icon_h-6,LV_TEXT_ALIGN_LEFT,std::isfinite(t.current)?b:"");
  part_label(w,2,w.value_font,0,y+std::max(icon_h,temp_h)+2,left-4,LV_TEXT_ALIGN_LEFT,weather_text(t.state));
  int column=(width-left)/5,day_h=lv_font_get_line_height(title_font),icon_col=lv_font_get_line_height(day_icon);
  for(unsigned k=0;k<5;++k){
    int x=left+k*column;bool has=k<t.forecast.size();const auto &f=t.forecast[k];
    char temps[24];if(has && std::isfinite(f.high))snprintf(temps,sizeof(temps),std::isfinite(f.low)?"%.0f/%.0f":"%.0f",f.high,f.low);else temps[0]=0;
    if(large){
      int rows=day_h+icon_col+text_h,top=std::max(0,(height-rows)/2);
      part_label(w,3+k*3,title_font,x,top,column,LV_TEXT_ALIGN_CENTER,has?f.day:"");
      part_label(w,4+k*3,day_icon,x,top+day_h,column,LV_TEXT_ALIGN_CENTER,has?weather_icon(f.condition):"");
      part_label(w,5+k*3,w.value_font,x,top+day_h+icon_col,column,LV_TEXT_ALIGN_CENTER,temps);
    }else{
      // Two rows on the CYD: day beside its icon, then the high/low pair.
      int rows=std::max(day_h,icon_col)+text_h,top=std::max(0,(height-rows)/2),day_w=column-icon_col-2;
      part_label(w,3+k*3,title_font,x,top+(std::max(day_h,icon_col)-day_h)/2,day_w,LV_TEXT_ALIGN_RIGHT,has?f.day:"");
      part_label(w,4+k*3,day_icon,x+day_w+2,top,icon_col,LV_TEXT_ALIGN_LEFT,has?weather_icon(f.condition):"");
      part_label(w,5+k*3,w.value_font,x,top+std::max(day_h,icon_col),column,LV_TEXT_ALIGN_CENTER,temps);
    }
  }
}
// Smoothed trend of the manager's 24 history samples with a soft fill beneath.
inline void render_graph(Widgets &w,const Tile &t,bool large,int x,int y,int width,int height) {
  begin_extra(w,"graph",x+width,y+height);
  float minimum=INFINITY,maximum=-INFINITY;for(float v:t.history)if(std::isfinite(v)){minimum=std::min(minimum,v);maximum=std::max(maximum,v);}
  lv_point_precise_t raw[24];unsigned n=0;int stroke=large?3:2,top=stroke;
  for(unsigned i=0;i<24;++i){
    if(!std::isfinite(t.history[i]))continue;
    float level=maximum>minimum?(t.history[i]-minimum)/(maximum-minimum):0.5f;
    raw[n++]={(lv_value_precise_t)(stroke/2+i*(width-stroke-1)/23),(lv_value_precise_t)(height-stroke/2-1-level*(height-stroke-1-top))};
  }
  if(n==1){raw[1]=raw[0];raw[1].x=(lv_value_precise_t)(width-1);n=2;}
  unsigned count=smooth(raw,n,w.points,POINT_BUFFER,width,height);
  part_line(w,0,w.points,count,stroke,x,y);
  w.fill_points=w.points;w.fill_count=count;w.fill_x=x;w.fill_y=y;w.fill_base=height;w.fill_opa=LV_OPA_20;
}
// Sun path: horizon, an arc from sunrise to sunset and the sun at the current
// position (or below the horizon at night). Wide cards only.
inline int minutes_of(const std::string &clock) {
  unsigned h=0,m=0;return sscanf(clock.c_str(),"%u:%u",&h,&m)==2 && h<24 && m<60 ? int(h*60+m) : -1;
}
inline void render_sunpath(Widgets &w,const Tile &t,bool large,int width,int height) {
  begin_extra(w,"sunpath",width,height);
  const lv_font_t *title_font=lv_obj_get_style_text_font(w.title,LV_PART_MAIN);
  int title_h=lv_font_get_line_height(title_font),text_h=lv_font_get_line_height(w.value_font);
  int horizon=height-text_h-(large?4:2),top=title_h+(large?4:2),x0=large?14:8,x1=width-x0;
  part_label(w,0,title_font,0,0,width,LV_TEXT_ALIGN_LEFT,t.name.empty()?"Zon":t.name);
  part_label(w,1,w.value_font,0,horizon+(large?3:1),width/2,LV_TEXT_ALIGN_LEFT,"op "+t.sunrise);
  part_label(w,2,w.value_font,width/2,horizon+(large?3:1),width/2,LV_TEXT_ALIGN_RIGHT,"onder "+t.sunset);
  auto now=now_time?now_time():esphome::ESPTime{};
  int rise=minutes_of(t.sunrise),set=minutes_of(t.sunset),minute=now.is_valid()?now.hour*60+now.minute:-1;
  bool day=t.state=="above_horizon";float fraction=0.5f;
  if(rise>=0 && set>=0 && minute>=0){
    if(day){int span=(set-rise+1440)%1440;if(!span)span=1;fraction=std::clamp(float((minute-rise+1440)%1440)/span,0.0f,1.0f);}
    else{int span=(rise-set+1440)%1440;if(!span)span=1;fraction=std::clamp(float((minute-set+1440)%1440)/span,0.0f,1.0f);}
  }
  const unsigned segments=40;float amplitude=day?float(horizon-top):float(height-text_h-horizon-(large?2:1));
  auto *arc=w.points,*travelled=w.points+segments+1,*line=w.points+2*segments+3;
  for(unsigned i=0;i<=segments;++i){
    float a=3.14159265f*i/segments;
    arc[i]={(lv_value_precise_t)(x0+(x1-x0)*float(i)/segments),(lv_value_precise_t)(day?horizon-sinf(a)*amplitude:horizon+sinf(a)*amplitude)};
  }
  unsigned filled=std::min(segments,unsigned(fraction*segments));
  for(unsigned i=0;i<=filled;++i)travelled[i]=arc[i];
  float sa=3.14159265f*fraction;
  lv_point_precise_t sun={(lv_value_precise_t)(x0+(x1-x0)*fraction),(lv_value_precise_t)(day?horizon-sinf(sa)*amplitude:horizon+sinf(sa)*amplitude)};
  travelled[filled+1]=sun;
  line[0]={(lv_value_precise_t)0,(lv_value_precise_t)horizon};line[1]={(lv_value_precise_t)(width-1),(lv_value_precise_t)horizon};
  uint32_t path=0xCFD8DC, accent=day?0xFF9800:0x5C6BC0, disc=day?0xFFB300:0xB0BEC5;
  lv_obj_set_style_line_color(part_line(w,3,line,2,2),lv_color_hex(path),0);
  lv_obj_set_style_line_color(part_line(w,4,arc,segments+1,large?2:1),lv_color_hex(path),0);
  lv_obj_set_style_line_color(part_line(w,5,travelled,filled+2,large?4:3),lv_color_hex(accent),0);
  int size=large?18:10,glow=size+(large?12:6);
  auto *halo=part_dot(w,6,int(sun.x)-glow/2,int(sun.y)-glow/2,glow);lv_obj_set_style_bg_color(halo,lv_color_hex(disc),0);lv_obj_set_style_bg_opa(halo,LV_OPA_30,0);
  lv_obj_set_style_bg_color(part_dot(w,7,int(sun.x)-size/2,int(sun.y)-size/2,size),lv_color_hex(disc),0);
  if(day){w.fill_points=travelled;w.fill_count=filled+2;w.fill_x=0;w.fill_y=0;w.fill_base=horizon;w.fill_color=lv_color_hex(0xFFB300);w.fill_opa=LV_OPA_20;}
}

// ---- Direct controls on wide cards (Home Assistant entity-row style) ----
struct PanelMetrics { int key_w, key_h, radius, gap, pill_w, pill_key, slider_w, slider_h, toggle_w, toggle_h, run_pad, text_gap, ext; };
inline PanelMetrics panel_metrics(bool large) {
  return large ? PanelMetrics{60,46,14,8,196,52,140,44,76,40,22,8,4} : PanelMetrics{40,34,9,4,128,36,90,30,48,26,14,6,6};
}
inline lv_obj_t *panel_obj(lv_obj_t *parent,bool clickable) {
  auto *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
  if(clickable)lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);else lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);
  return o;
}
inline void end_panel(Widgets &w) {
  if(!w.panel)return;
  lv_obj_add_flag(w.panel,LV_OBJ_FLAG_HIDDEN);w.panel_w=0;
  if(!w.panel_mode.empty()){
    if(captured_slider && captured_slider==w.control_slider)captured_slider=nullptr;
    lv_obj_clean(w.panel);w.keys.fill(nullptr);w.key_icons.fill(nullptr);w.key_checked.fill(-1);
    w.pill=w.pill_value=w.knob=w.control_slider=nullptr;w.knob_on=-1;w.panel_mode.clear();
  }
}
inline void control_event(lv_event_t *e);
// A pill key: rounded, pressed darker, checked in the accent, disabled faded.
inline lv_obj_t *panel_key(Widgets &w,unsigned n,lv_obj_t *parent,const PanelMetrics &m,int width,int height,bool transparent) {
  auto *key=panel_obj(parent,true);lv_obj_set_size(key,width,height);
  lv_obj_set_style_radius(key,transparent?LV_RADIUS_CIRCLE:m.radius,0);
  lv_obj_set_style_bg_opa(key,transparent?LV_OPA_TRANSP:LV_OPA_COVER,0);
  lv_obj_set_style_bg_opa(key,LV_OPA_COVER,LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(key,LV_OPA_COVER,LV_STATE_CHECKED);
  lv_obj_set_style_opa(key,LV_OPA_40,LV_STATE_DISABLED);
  lv_obj_set_ext_click_area(key,m.ext);
  size_t slot=&w-widgets.data();
  lv_obj_add_event_cb(key,control_event,LV_EVENT_SHORT_CLICKED,(void*)(uintptr_t)(slot*16+n));
  w.keys[n]=key;w.key_checked[n]=-1;
  return key;
}
inline lv_obj_t *panel_icon(Widgets &w,unsigned n,const lv_font_t *font) {
  auto *icon=lv_label_create(w.keys[n]);lv_obj_remove_flag(icon,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(icon,font,0);lv_obj_center(icon);w.key_icons[n]=icon;return icon;
}
// Build (once per control set) and lay out the panel; returns the width it takes
// from the text, including the gap, or 0 when the card shows no panel.
inline int layout_panel(Widgets &w,const Tile &t,bool large,int content_w,int content_h) {
  std::string mode=tile_controls::panel_kind(t);
  const PanelMetrics m=panel_metrics(large);
  const lv_font_t *icon_font=mini_icon_font?mini_icon_font:w.icon_font;
  const lv_font_t *text_font=control_font?control_font:lv_obj_get_style_text_font(w.title,LV_PART_MAIN);
  auto d=t.domain();
  if(!w.panel){w.panel=panel_obj(w.tile,false);}
  if(w.panel_mode!=mode){
    end_panel(w);w.panel_mode=mode;w.panel_dirty=true;
    if(tile_controls::is_key_row(mode)){
      for(unsigned n=0;n<3;++n){panel_key(w,n,w.panel,m,m.key_w,m.key_h,false);panel_icon(w,n,icon_font);}
    }else if(mode=="setpoint"||mode=="stepper"){
      w.pill=panel_obj(w.panel,false);lv_obj_set_size(w.pill,m.pill_w,m.key_h+2);
      lv_obj_set_style_radius(w.pill,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_opa(w.pill,LV_OPA_COVER,0);
      panel_key(w,0,w.pill,m,m.pill_key,m.key_h+2,true);panel_icon(w,0,icon_font);lv_label_set_text(w.key_icons[0],tile_controls::glyph::MINUS);
      panel_key(w,1,w.pill,m,m.pill_key,m.key_h+2,true);panel_icon(w,1,icon_font);lv_label_set_text(w.key_icons[1],tile_controls::glyph::PLUS);
      // Holding -/+ keeps stepping (LVGL repeats while pressed); one call goes out after the finger rests.
      for(unsigned n=0;n<2;++n)lv_obj_add_event_cb(w.keys[n],control_event,LV_EVENT_LONG_PRESSED_REPEAT,(void*)(uintptr_t)((&w-widgets.data())*16+n));
      lv_obj_set_pos(w.keys[0],0,0);lv_obj_set_pos(w.keys[1],m.pill_w-m.pill_key,0);
      w.pill_value=lv_label_create(w.pill);lv_obj_remove_flag(w.pill_value,LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_text_font(w.pill_value,text_font,0);lv_obj_set_style_text_align(w.pill_value,LV_TEXT_ALIGN_CENTER,0);
      lv_label_set_long_mode(w.pill_value,LV_LABEL_LONG_CLIP);
      lv_obj_set_size(w.pill_value,m.pill_w-2*m.pill_key,lv_font_get_line_height(text_font));
      lv_obj_set_pos(w.pill_value,m.pill_key,(m.key_h+2-lv_font_get_line_height(text_font))/2);
      w.key_commands[0]=tile_controls::STEP_DOWN;w.key_commands[1]=tile_controls::STEP_UP;
    }else if(tile_controls::is_slider(mode)){
      int h=mode=="volume"?m.slider_h:m.slider_h;
      auto *slider=lv_slider_create(w.panel);w.control_slider=slider;
      lv_obj_remove_flag(slider,LV_OBJ_FLAG_GESTURE_BUBBLE);lv_obj_remove_flag(slider,LV_OBJ_FLAG_SCROLLABLE);
      lv_slider_set_range(slider,0,1000);lv_obj_set_size(slider,mode=="volume"?m.slider_w:m.pill_w,h);
      lv_obj_set_style_pad_all(slider,0,LV_PART_MAIN);
      lv_obj_set_style_radius(slider,LV_RADIUS_CIRCLE,LV_PART_MAIN);lv_obj_set_style_radius(slider,LV_RADIUS_CIRCLE,LV_PART_INDICATOR);
      lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_INDICATOR);
      // The handle is a short white bar inside the fill, like Home Assistant's slider.
      lv_obj_set_style_radius(slider,2,LV_PART_KNOB);lv_obj_set_style_bg_color(slider,lv_color_hex(0xFFFFFF),LV_PART_KNOB);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB);
      lv_obj_set_style_pad_hor(slider,-(h/2-(large?3:2)),LV_PART_KNOB);lv_obj_set_style_pad_ver(slider,-(h/4),LV_PART_KNOB);
      lv_obj_set_style_border_width(slider,0,LV_PART_KNOB);lv_obj_set_style_shadow_width(slider,0,LV_PART_KNOB);
      lv_obj_set_ext_click_area(slider,m.ext+2);
      lv_obj_add_event_cb(slider,slider_event,LV_EVENT_ALL,(void*)(uintptr_t)w.index);
      if(mode=="volume"){panel_key(w,0,w.panel,m,m.key_h,m.key_h,false);panel_icon(w,0,icon_font);w.key_commands[0]=tile_controls::MEDIA_MUTE;}
    }else if(mode=="toggle"){
      panel_key(w,0,w.panel,m,m.toggle_w,m.toggle_h,false);lv_obj_set_style_radius(w.keys[0],LV_RADIUS_CIRCLE,0);
      w.knob=panel_obj(w.keys[0],false);lv_obj_set_size(w.knob,m.toggle_h-8,m.toggle_h-8);lv_obj_set_y(w.knob,4);
      lv_obj_set_style_radius(w.knob,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_opa(w.knob,LV_OPA_COVER,0);lv_obj_set_style_bg_color(w.knob,lv_color_hex(0xFFFFFF),0);
      w.key_commands[0]=tile_controls::TOGGLE;
    }else if(mode=="run"){
      const char *text=tile_controls::run_label(d);
      lv_point_t size;lv_text_get_size(&size,text,text_font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
      panel_key(w,0,w.panel,m,std::max(m.key_w*3/2,(int)size.x+2*m.run_pad),m.key_h,false);lv_obj_set_style_radius(w.keys[0],LV_RADIUS_CIRCLE,0);
      panel_icon(w,0,text_font);lv_label_set_text(w.key_icons[0],text);
      w.key_commands[0]=tile_controls::RUN;
    }else{end_panel(w);return 0;}
  }
  // Per-render contents: which keys, their icons and states; then the panel size and place.
  int panel_w=0,panel_h=m.key_h;uint32_t now=esphome::millis();
  auto set_checked=[&](unsigned n,bool checked){
    if(w.key_checked[n]==(int)checked)return;w.key_checked[n]=checked;
    if(checked)lv_obj_add_state(w.keys[n],LV_STATE_CHECKED);else lv_obj_remove_state(w.keys[n],LV_STATE_CHECKED);
    if(w.key_icons[n])lv_obj_set_style_text_color(w.key_icons[n],checked?lv_color_hex(0xFFFFFF):w.panel_text,0);
  };
  auto set_disabled=[&](unsigned n,bool disabled){if(disabled)lv_obj_add_state(w.keys[n],LV_STATE_DISABLED);else lv_obj_remove_state(w.keys[n],LV_STATE_DISABLED);};
  if(tile_controls::is_key_row(mode)){
    std::array<tile_controls::Key,3> keys;unsigned count=tile_controls::keys_for(t,keys);
    for(unsigned n=0;n<3;++n){
      if(n>=count){lv_obj_add_flag(w.keys[n],LV_OBJ_FLAG_HIDDEN);w.key_commands[n]=tile_controls::NONE;continue;}
      lv_obj_remove_flag(w.keys[n],LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(w.keys[n],n*(m.key_w+m.gap),0);
      label(w.key_icons[n],keys[n].icon);w.key_commands[n]=keys[n].command;w.key_args[n]=keys[n].arg;
      // The active mode key carries the accent; "off" stays neutral grey.
      if(keys[n].checked && w.key_checked[n]!=1)lv_obj_set_style_bg_color(w.keys[n],keys[n].arg=="off"?lv_color_hex(0x9E9E9E):w.panel_accent,LV_STATE_CHECKED);
      set_checked(n,keys[n].checked);set_disabled(n,keys[n].disabled);
    }
    panel_w=count?count*m.key_w+(count-1)*m.gap:0;
  }else if(mode=="setpoint"||mode=="stepper"){
    float shown=std::isfinite(t.edit_value)?t.edit_value:tile_controls::edit_target(t);
    std::string suffix=d=="climate"?"°":t.unit.empty()?"":" "+t.unit;
    label(w.pill_value,tile_controls::format_value(shown,tile_controls::edit_step(t),suffix.c_str()));
    panel_w=m.pill_w;panel_h=m.key_h+2;
  }else if(tile_controls::is_slider(mode)){
    bool has_slider=mode!="volume" || (t.supported & tile_controls::feature::MEDIA_VOLUME_SET);
    if(has_slider){
      lv_obj_remove_flag(w.control_slider,LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(w.control_slider,0,(panel_h-m.slider_h)/2);
      // Keep the dragged value while the command is under way; HA's report takes over afterwards.
      if(!lv_obj_has_state(w.control_slider,LV_STATE_PRESSED) && !(t.pending && !t.confirmed))lv_slider_set_value(w.control_slider,slider_value(t),LV_ANIM_OFF);
      panel_w=mode=="volume"?m.slider_w:m.pill_w;
    }else lv_obj_add_flag(w.control_slider,LV_OBJ_FLAG_HIDDEN);
    if(mode=="volume"){
      bool has_mute=t.supported & tile_controls::feature::MEDIA_VOLUME_MUTE;
      if(has_mute){
        lv_obj_remove_flag(w.keys[0],LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(w.keys[0],panel_w?panel_w+m.gap:0,0);
        label(w.key_icons[0],t.muted?tile_controls::glyph::MUTED:tile_controls::glyph::VOLUME);
        if(t.muted && w.key_checked[0]!=1)lv_obj_set_style_bg_color(w.keys[0],w.panel_accent,LV_STATE_CHECKED);
        set_checked(0,t.muted);panel_w+=(panel_w?m.gap:0)+m.key_h;
      }else lv_obj_add_flag(w.keys[0],LV_OBJ_FLAG_HIDDEN);
    }
  }else if(mode=="toggle"){
    bool on=t.pending && !t.confirmed ? t.optimistic_on : t.state=="on";
    if(on && w.key_checked[0]!=1)lv_obj_set_style_bg_color(w.keys[0],w.panel_accent,LV_STATE_CHECKED);
    set_checked(0,on);
    if(w.knob_on!=(int)on){w.knob_on=on;lv_obj_set_x(w.knob,on?m.toggle_w-(m.toggle_h-8)-4:4);}
    panel_w=m.toggle_w;panel_h=m.toggle_h;
  }else if(mode=="run"){
    panel_w=lv_obj_get_width(w.keys[0]);
  }
  (void)now;
  if(!panel_w){lv_obj_add_flag(w.panel,LV_OBJ_FLAG_HIDDEN);w.panel_w=0;return 0;}
  lv_obj_remove_flag(w.panel,LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(w.panel,panel_w,panel_h);lv_obj_set_pos(w.panel,content_w-panel_w,std::max(0,(content_h-panel_h)/2));
  w.panel_w=panel_w+m.text_gap;
  return w.panel_w;
}
// Colours follow the card palette; called with the rest of the palette when it changes.
inline void style_panel(Widgets &w,const Tile &t,lv_color_t accent,lv_color_t text) {
  if(!w.panel || w.panel_mode.empty())return;
  lv_color_t card=lv_color_hex(t.background?t.background:0xFFFFFF);
  lv_color_t key_bg=lv_color_mix(card,lv_color_hex(0x000000),236);
  lv_color_t key_pressed=lv_color_mix(card,lv_color_hex(0x000000),212);
  w.panel_accent=accent;w.panel_text=text;
  for(unsigned n=0;n<3;++n){
    auto *key=w.keys[n];if(!key)continue;
    bool in_pill=w.pill && lv_obj_get_parent(key)==w.pill;
    lv_obj_set_style_bg_color(key,in_pill?key_pressed:key_bg,0);
    lv_obj_set_style_bg_color(key,in_pill?lv_color_mix(card,lv_color_hex(0x000000),190):key_pressed,LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(key,w.key_args[n]=="off" && w.panel_mode=="mode"?lv_color_hex(0x9E9E9E):accent,LV_STATE_CHECKED);
    if(w.key_icons[n])lv_obj_set_style_text_color(w.key_icons[n],w.key_checked[n]==1?lv_color_hex(0xFFFFFF):text,0);
  }
  if(w.pill){lv_obj_set_style_bg_color(w.pill,key_bg,0);lv_obj_set_style_text_color(w.pill_value,text,0);}
  if(w.control_slider){
    lv_obj_set_style_bg_color(w.control_slider,lv_color_mix(accent,card,76),LV_PART_MAIN);
    lv_obj_set_style_bg_color(w.control_slider,accent,LV_PART_INDICATOR);
  }
  if(w.knob){lv_obj_set_style_bg_color(w.keys[0],lv_color_hex(0xD7DADF),0);lv_obj_set_style_bg_color(w.keys[0],lv_color_hex(0xC5C9CF),LV_STATE_PRESSED);}
}
inline void control_event(lv_event_t *e) {
  unsigned code=(uintptr_t)lv_event_get_user_data(e);unsigned slot=code/16,n=code%16;
  if(slot>=widgets.size() || n>=3)return;
  auto &w=widgets[slot];
  if(!enabled || !fresh() || w.index>=model.count || !w.keys[n])return;
  if(lv_obj_has_state(w.keys[n],LV_STATE_DISABLED))return;
  uint32_t now=esphome::millis();
  auto &t=model.tiles[w.index];
  int command=w.key_commands[n];
  bool step=command==tile_controls::STEP_DOWN || command==tile_controls::STEP_UP;
  bool held=lv_event_get_code(e)==LV_EVENT_LONG_PRESSED_REPEAT;
  if(held){ if(!step || now-t.edit_since<300)return; }  // three steps a second while holding
  else if(step){ if(!cyd::touch_guard.accept_repeat(now,400+slot*16+n)){ESP_LOGI("touch","tik op bediening %u genegeerd: %s",(unsigned)slot,cyd::touch_guard.reason().c_str());return;} }
  else if(!allowed(now,400+slot*16+n,"bediening "+std::to_string(slot)))return;
  if(!t.available())return;
  if(step){
    // Local at once, tap after tap; tick() sends the last value after a short pause.
    float current=std::isfinite(t.edit_value)?t.edit_value:tile_controls::edit_target(t);
    t.edit_value=tile_controls::step_value(current,tile_controls::edit_step(t),t.minimum,t.maximum,command==tile_controls::STEP_UP?1:-1);
    t.edit_since=now;t.edit_sent=false;
    if(w.pill_value){std::string suffix=t.domain()=="climate"?"°":t.unit.empty()?"":" "+t.unit;label(w.pill_value,tile_controls::format_value(t.edit_value,tile_controls::edit_step(t),suffix.c_str()));}
    return;
  }
  if(t.awaiting_action(now))return;
  if(command==tile_controls::TOGGLE)t.optimistic_on=t.state!="on";
  auto a=tile_controls::key_action(t,command,w.key_args[n]);
  if(a.valid())action(a.service,t.entity,a.key,a.value);
}
// A busy card is covered by a translucent white sheet with a small spinner until
// Home Assistant confirms; the sheet also swallows taps meanwhile.
inline void set_busy(Widgets &w,bool busy,bool large){
  if(!busy){if(w.busy)lv_obj_add_flag(w.busy,LV_OBJ_FLAG_HIDDEN);return;}
  if(!w.busy){
    w.busy=lv_obj_create(w.tile);lv_obj_remove_style_all(w.busy);lv_obj_remove_flag(w.busy,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(w.busy,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(w.busy,lv_color_hex(0xFFFFFF),0);lv_obj_set_style_bg_opa(w.busy,LV_OPA_60,0);
    lv_obj_set_style_radius(w.busy,lv_obj_get_style_radius(w.tile,LV_PART_MAIN),0);
#if LV_USE_SPINNER
    w.spinner=lv_spinner_create(w.busy);lv_spinner_set_anim_params(w.spinner,900,200);
    int size=large?30:20;lv_obj_set_size(w.spinner,size,size);lv_obj_center(w.spinner);lv_obj_remove_flag(w.spinner,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(w.spinner,large?4:3,LV_PART_MAIN);lv_obj_set_style_arc_width(w.spinner,large?4:3,LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(w.spinner,lv_color_hex(0xD9DDE2),LV_PART_MAIN);lv_obj_set_style_arc_color(w.spinner,lv_color_hex(0x1E88E5),LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(w.spinner,LV_OPA_TRANSP,LV_PART_KNOB);lv_obj_set_style_pad_all(w.spinner,0,LV_PART_KNOB);
#endif
  }
  if(!lv_obj_has_flag(w.busy,LV_OBJ_FLAG_HIDDEN))return;
  // Cover the whole card, padding included.
  lv_obj_set_pos(w.busy,-lv_obj_get_style_pad_left(w.tile,LV_PART_MAIN),-lv_obj_get_style_pad_top(w.tile,LV_PART_MAIN));
  lv_obj_set_size(w.busy,lv_obj_get_width(w.tile),lv_obj_get_height(w.tile));
  lv_obj_remove_flag(w.busy,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(w.busy);
}
inline void render(lv_obj_t *room) {
  if (!enabled) return;
  room_label=room;
  label(room, !model.configured ? "Kies tegels in HA" : !model.ready() ? "Tegels laden..." : !ha_connected() ? "HA niet verbonden" : !feed_alive() ? "ESP Screens niet actief" : model.title);
  for (size_t slot = 0; slot < 6; ++slot) {
    auto &w=widgets[slot];
    if(!w.tile || w.index>=model.count)continue;
    const auto &t = model.tiles[w.index];
    auto d=t.domain();
    label(w.title, t.name.empty() ? t.entity : t.name);
    label(w.icon, icon_for(t));
    bool watch=t.display=="watch";
    std::string unit=watch?t.unit:"";
    std::string value = t.state;
    if (!fresh() || !t.available()) value = "Niet beschikbaar";
    else if (d == "light" && t.state == "on" && std::isfinite(t.brightness)) value = std::to_string(static_cast<int>(std::lround(std::clamp(t.brightness, 0.0f, 255.0f) * 100 / 255))) + " %";
    else if (d == "climate" && std::isfinite(t.target)) { char b[32]; snprintf(b, sizeof(b), "%.1f°", t.target); value = b; }
    else if (d == "person") value = t.state=="home"?"Thuis":t.state=="not_home"?"Weg":t.state;
    else if (d == "sun") value = !t.sunrise.empty() && !t.sunset.empty() ? t.sunrise+" - "+t.sunset : t.state=="above_horizon"?"Boven de horizon":"Onder de horizon";
    else if (d == "timer") value = timer_text(t);
    else if (d == "script" || d == "scene" || d == "button" || d == "input_button") value = t.state == "on" ? "Bezig..." : last_run_text(t.last_run);
    else if (value == "on") value = "Aan";
    else if (value == "off") value = "Uit";
    else if (value == "cleaning") value = "Bezig";
    else if (value == "docked") value = "In dock";
    else if (!t.unit.empty() && !watch) value += " " + t.unit;
    bool pending=t.loading(esphome::millis());
    if(d=="weather" && std::isfinite(t.current)) {char b[32];snprintf(b,sizeof(b),"%.1f %s",t.current,t.unit.c_str());value=b;if(watch){snprintf(b,sizeof(b),"%.1f",t.current);value=b;}}
    if(d=="vacuum" && std::isfinite(t.battery))value += " / "+std::to_string((int)t.battery)+"%";
    // Direct controls: only a wide card in the standard layout has room for the panel.
    bool with_panel=w.wide && !t.controls.empty() && !t.builtin() && !watch && t.inline_control!="slider" && fresh() && t.available();
    if(with_panel){std::string status=tile_controls::status_text(t);if(!status.empty())value=status;}
    label(w.value, value);
    bool mini=t.inline_control=="slider" && !watch && t.available();
    bool large_tile=lv_obj_get_height(w.tile)>80;
    set_busy(w,pending && !t.builtin(),large_tile);
    // Cards that replace the name/status layout entirely.
    bool clock=t.builtin(), forecast=d=="weather" && t.display=="forecast" && w.wide && t.forecast.size()>0 && fresh() && t.available();
    bool sunpath=d=="sun" && t.display=="sunpath" && w.wide && !t.sunrise.empty() && !t.sunset.empty() && fresh() && t.available();
    bool graph=d=="sensor" && t.display=="graph" && t.has_history && !clock;
    bool custom=clock||forecast||sunpath;
    if(!large_tile)pad_vertical(w.tile,watch||custom||graph?2:4);
    set_font(w.value,watch && watch_value_font ? watch_value_font : w.value_font);
    lv_obj_set_height(w.value,lv_font_get_line_height(lv_obj_get_style_text_font(w.value,LV_PART_MAIN)));
    lv_obj_update_layout(w.tile);
    int content_width=lv_obj_get_content_width(w.tile),content_height=lv_obj_get_content_height(w.tile);
    for(auto *o:{w.title,w.value,w.circle,w.unit}){if(custom)lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN);}
    if(custom){
      lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);end_panel(w);
      if(clock)render_clock(w,t,large_tile,content_width,content_height);
      else if(sunpath)render_sunpath(w,t,large_tile,content_width,content_height);
      else render_forecast(w,t,large_tile,content_width,content_height);
    }else{
    int title_height=lv_obj_get_height(w.title),value_height=lv_obj_get_height(w.value);
    int line_gap=large_tile?2:1,text_height=title_height+line_gap+value_height;
    int slider_height=large_tile?28:8;
    // A single-width graph takes the slider strip; a wide graph takes the right half.
    bool graph_strip=graph && !w.wide, graph_side=graph && w.wide;
    int chart_w=graph_side?content_width*55/100:0;
    int panel_w=with_panel && !graph?layout_panel(w,t,large_tile,content_width,content_height):0;
    if(!panel_w)end_panel(w);
    int header_height=(mini||graph_strip)?content_height-slider_height-(large_tile?6:3):content_height;
    int text_y=std::max(0,(header_height-text_height)/2);
    int circle_size=watch?(large_tile?26:18):(mini||graph_strip)?(large_tile?36:24):(large_tile?54:36);
    lv_obj_set_size(w.circle,circle_size,circle_size);
    const lv_font_t *icon_font=watch && watch_icon_font ? watch_icon_font : (mini||graph_strip) && mini_icon_font ? mini_icon_font : w.icon_font;
    if(lv_obj_get_style_text_font(w.icon,LV_PART_MAIN)!=icon_font){set_font(w.icon,icon_font);lv_obj_center(w.icon);}
    int text_x=watch?0:(mini||graph_strip)?circle_size+(large_tile?8:6):w.title_x;
    lv_obj_set_pos(w.title,text_x,watch?0:(mini||graph_strip||!large_tile)?text_y:w.title_y);
    lv_obj_set_pos(w.value,watch?0:(mini||graph_strip)?text_x:w.value_x,
      watch?(large_tile?42:19):(mini||graph_strip||!large_tile)?text_y+title_height+line_gap:w.value_y);
    lv_obj_set_pos(w.circle,0,(mini||graph_strip||!large_tile)?std::max(0,(header_height-circle_size)/2):12);
    // Use the requested coordinates: LVGL getters still return the previous
    // layout until its next pass when a slot changes from watch/slider to normal.
    int text_room=content_width-chart_w-(graph_side?(large_tile?10:6):0)-panel_w;
    lv_obj_set_width(w.title,std::max(1,text_room-text_x));
    lv_obj_set_width(w.value,std::max(1,text_room-(watch?0:(mini||graph_strip)?text_x:w.value_x)));
    if(watch){
      int gap=large_tile?6:2,header=std::max(circle_size,title_height);
      int group_y=std::max(0,(content_height-header-gap-value_height)/2);
      int value_y=group_y+header+gap;
      lv_obj_set_pos(w.circle,0,group_y+(header-circle_size)/2);
      lv_obj_set_pos(w.title,circle_size+(large_tile?6:4),group_y+(header-title_height)/2);
      lv_obj_set_width(w.title,text_room-circle_size-(large_tile?6:4));
      label(w.unit,unit);
      lv_point_t size;lv_text_get_size(&size,unit.c_str(),w.value_font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
      int unit_width=unit.empty()?0:std::min((int)size.x,text_room-20);
      int number_width=text_room-(unit_width?unit_width+(large_tile?6:3):0);
      lv_obj_set_pos(w.value,0,value_y);lv_obj_set_width(w.value,number_width);
      lv_obj_set_pos(w.unit,text_room-unit_width,value_y+value_height-lv_font_get_line_height(w.value_font));
      lv_obj_set_size(w.unit,unit_width,lv_font_get_line_height(w.value_font));
      if(unit_width)lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
    }else lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(w.slider,content_width,slider_height);
    if(mini){lv_obj_remove_flag(w.slider,LV_OBJ_FLAG_HIDDEN);if(!lv_obj_has_state(w.slider,LV_STATE_PRESSED))lv_slider_set_value(w.slider,slider_value(t),LV_ANIM_OFF);}
    else lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
    if(graph_strip)render_graph(w,t,large_tile,0,content_height-slider_height,content_width,slider_height);
    else if(graph_side)render_graph(w,t,large_tile,content_width-chart_w,0,chart_w,content_height);
    else end_extra(w);
    }
    bool on = fresh() && t.active();
    bool available=fresh() && t.available();
    int palette_state=(available?2:0)|(on?1:0);
    if (w.cached_active == palette_state && !w.panel_dirty) continue;
    w.cached_active = palette_state;w.panel_dirty=false;
    uint32_t accent = domain_accent(t);
    auto color=lv_color_hex((t.is_switch()||d=="person"||d=="timer") && !on ? 0x9E9E9E : accent);
    if(d=="light" && on && t.has_hs_color)
      color=lv_color_hsv_to_rgb(t.hue%360,t.saturation,100);
    auto circle_color=available?lv_color_mix(color,lv_color_hex(0xFFFFFF),38):lv_color_hex(0xF0F0F0);
    // Darken the foreground slightly: very pale bulbs still need a visible icon.
    auto icon_color=available?lv_color_mix(color,lv_color_hex(0x333333),205):lv_color_hex(0x9E9E9E);
    lv_obj_set_style_bg_color(w.slider,color,LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(w.slider,lv_color_mix(color,lv_color_hex(0xFFFFFF),30),LV_PART_MAIN);
    lv_obj_set_style_bg_color(w.tile, lv_color_hex(t.background ? t.background : 0xFFFFFF), 0);
    lv_obj_set_style_border_width(w.tile, 1, 0);
    // "Achtergrond: geen" hides only the card; geometry and padding stay identical,
    // and the pressed flash still shows because it lives on the PRESSED state.
    lv_obj_set_style_bg_opa(w.tile, t.transparent ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_border_opa(w.tile, t.transparent ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(w.tile, t.background ? lv_color_mix(lv_color_hex(t.background),lv_color_hex(0x000000),220) : lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_bg_color(w.circle,circle_color,0);
    lv_obj_set_style_text_color(w.icon,icon_color,0);
    auto title_color=lv_color_hex(0x1B1B1B);
    auto value_color=lv_color_hex(t.background ? 0x46525E : 0x616161);
    lv_obj_set_style_text_color(w.unit,lv_color_hex(0x46525E),0);
    lv_obj_set_style_text_color(w.title, title_color, 0);
    lv_obj_set_style_text_color(w.value, value_color, 0);
    style_panel(w,t,color,title_color);
    // Custom parts follow the card palette: text like the title, lines/dots in the accent.
    // The sun path sets its own colours on every render.
    w.fill_color=color;
    for(unsigned i=0;i<w.parts.size();++i){
      auto *p=w.parts[i];if(!p)continue;
      bool muted=w.extra_mode=="forecast" ? i>=2 && i%3==2 : w.extra_mode=="sunpath" ? i>=1 : w.extra_mode=="calendar" ? i==15||i==17 : i==16;
      if(lv_obj_check_type(p,&lv_label_class))lv_obj_set_style_text_color(p,muted?value_color:title_color,0);
      else if(w.extra_mode=="sunpath")continue;
      else if(lv_obj_check_type(p,&lv_line_class))lv_obj_set_style_line_color(p,w.extra_mode=="graph"?color:i<12?value_color:i==13?icon_color:title_color,0);
      else lv_obj_set_style_bg_color(p,i==14?icon_color:value_color,0);
    }
  }
}

// Inspect actual LVGL coordinates, including padding and the loaded font metrics.
inline bool check_tile_geometry() {
  bool ok=true;
  for(auto &w:widgets){
    if(!w.tile || lv_obj_has_flag(w.tile,LV_OBJ_FLAG_HIDDEN))continue;
    lv_obj_update_layout(w.tile);
    lv_area_t title,value,track,content;
    lv_obj_get_content_coords(w.tile,&content);
    lv_obj_get_coords(w.title,&title);lv_obj_get_coords(w.value,&value);
    bool custom=lv_obj_has_flag(w.title,LV_OBJ_FLAG_HIDDEN);
    bool fits=true;
    if(w.wide && widgets[1].tile){
      // A wide card ends exactly where the right column ends.
      lv_area_t left,right;lv_obj_get_coords(w.tile,&left);lv_obj_get_coords(widgets[1].tile,&right);
      int expected=lv_obj_get_x(widgets[1].tile)-lv_obj_get_x(widgets[0].tile)+w.base_width;
      fits=lv_obj_get_width(w.tile)==expected;
      if(!fits)ESP_LOGE("ui_test","Wide width FAIL slot=%u width=%d expected=%d",(unsigned)w.index,lv_obj_get_width(w.tile),expected);
    }
    if(!custom){
      fits=fits && title.x1>=content.x1 && title.x2<=content.x2 &&
        value.x1>=content.x1 && value.x2<=content.x2 && title.y2<value.y1 && value.y2<=content.y2;
      if(!lv_obj_has_flag(w.slider,LV_OBJ_FLAG_HIDDEN)){
        lv_obj_get_coords(w.slider,&track);
        fits=fits && value.y2<track.y1 && track.y2<=content.y2;
      }
      if(!lv_obj_has_flag(w.circle,LV_OBJ_FLAG_HIDDEN)){
        lv_area_t circle;lv_obj_get_coords(w.circle,&circle);
        bool mini=!lv_obj_has_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
        fits=fits && circle.x1>=content.x1 && circle.x2<title.x1 && circle.y1>=content.y1;
        if(mini)fits=fits && circle.y2<track.y1;
        else fits=fits && circle.y2<=content.y2;
        if(!fits)ESP_LOGE("ui_test","Icon bounds slot=%u circle=%d,%d..%d,%d title_x=%d content=%d,%d..%d,%d",(unsigned)w.index,circle.x1,circle.y1,circle.x2,circle.y2,title.x1,content.x1,content.y1,content.x2,content.y2);
        bool watch=w.index<model.count && model.tiles[w.index].display=="watch";
        bool graph=w.extra_mode=="graph";
        if(!watch && !graph && (mini || lv_obj_get_height(w.tile)<=80)){
          int header_bottom=mini?track.y1-(lv_obj_get_height(w.tile)>80?6:3)-1:content.y2;
          int center_twice=content.y1+header_bottom;
          fits=fits && std::abs(circle.y1+circle.y2-center_twice)<=2 &&
            std::abs(title.y1+value.y2-center_twice)<=2;
        }
      }
      if(!lv_obj_has_flag(w.unit,LV_OBJ_FLAG_HIDDEN)){
        lv_area_t unit;lv_obj_get_coords(w.unit,&unit);
        fits=fits && value.x2<unit.x1 && unit.x2<=content.x2 && unit.y2<=content.y2 && unit.y1>title.y2;
      }
    }
    if(w.panel && !lv_obj_has_flag(w.panel,LV_OBJ_FLAG_HIDDEN)){
      // Direct controls stay inside the card, right of the name and status, and inside their panel.
      lv_area_t panel;lv_obj_get_coords(w.panel,&panel);
      bool inside=panel.x1>=content.x1 && panel.x2<=content.x2 && panel.y1>=content.y1 && panel.y2<=content.y2 && (custom || (title.x2<panel.x1 && value.x2<panel.x1));
      for(uint32_t i=0;i<lv_obj_get_child_count(w.panel);++i){
        auto *child=lv_obj_get_child(w.panel,i);if(lv_obj_has_flag(child,LV_OBJ_FLAG_HIDDEN))continue;
        lv_area_t part;lv_obj_get_coords(child,&part);
        inside=inside && part.x1>=panel.x1 && part.x2<=panel.x2 && part.y1>=panel.y1 && part.y2<=panel.y2;
      }
      if(!inside)ESP_LOGE("ui_test","Panel bounds slot=%u mode=%s panel=%d,%d..%d,%d title_x2=%d content=%d,%d..%d,%d",(unsigned)w.index,w.panel_mode.c_str(),panel.x1,panel.y1,panel.x2,panel.y2,title.x2,content.x1,content.y1,content.x2,content.y2);
      fits=fits && inside;
    }
    if(w.extra && !lv_obj_has_flag(w.extra,LV_OBJ_FLAG_HIDDEN)){
      // Custom parts stay inside the card; a graph never runs into the text.
      lv_area_t extra;lv_obj_get_coords(w.extra,&extra);
      fits=fits && extra.x1>=content.x1 && extra.x2<=content.x2 && extra.y1>=content.y1 && extra.y2<=content.y2;
      for(auto *p:w.parts){
        if(!p || lv_obj_has_flag(p,LV_OBJ_FLAG_HIDDEN))continue;
        lv_area_t part;lv_obj_get_coords(p,&part);
        bool inside=part.x1>=content.x1 && part.x2<=content.x2 && part.y1>=content.y1 && part.y2<=content.y2;
        if(!inside)ESP_LOGE("ui_test","Part bounds slot=%u mode=%s part=%d,%d..%d,%d content=%d,%d..%d,%d",(unsigned)w.index,w.extra_mode.c_str(),part.x1,part.y1,part.x2,part.y2,content.x1,content.y1,content.x2,content.y2);
        fits=fits && inside;
        if(w.extra_mode=="graph" && !custom)fits=fits && (w.wide?part.x1>value.x2:part.y1>value.y2);
      }
    }
    if(!fits)ESP_LOGE("ui_test","Tile geometry FAIL slot=%u mode=%s wide=%d title_y=%d..%d value_y=%d..%d content_y=%d..%d",(unsigned)w.index,w.extra_mode.c_str(),w.wide,title.y1,title.y2,value.y1,value.y2,content.y1,content.y2);
    if(w.index<model.count && model.tiles[w.index].background){
      bool palette_ok=lv_color_eq(lv_obj_get_style_bg_color(w.tile,LV_PART_MAIN),lv_color_hex(model.tiles[w.index].background)) &&
        lv_color_eq(lv_obj_get_style_text_color(w.title,LV_PART_MAIN),lv_color_hex(0x1B1B1B));
      if(!palette_ok)ESP_LOGE("ui_test","Tile palette FAIL slot=%u",(unsigned)w.index);
      fits=fits && palette_ok;
    }
    if(w.index<model.count){
      bool bare=model.tiles[w.index].transparent;
      bool opa_ok=(lv_obj_get_style_bg_opa(w.tile,LV_PART_MAIN)==LV_OPA_TRANSP)==bare && (lv_obj_get_style_border_opa(w.tile,LV_PART_MAIN)==LV_OPA_TRANSP)==bare;
      if(!opa_ok)ESP_LOGE("ui_test","Tile background FAIL slot=%u transparent=%d",(unsigned)w.index,bare);
      fits=fits && opa_ok;
    }
    ok=ok && fits;
  }
  return ok;
}

inline unsigned page_count() {
  std::array<Placement,MAX_TILES> placement;
  return pack(model.tiles,model.count,placement);
}
// Page switches feel immediate: the new page's card frames (right widths, no
// contents, light skeleton style) appear in the very next frame, and a one-shot
// LVGL timer fills them in right after. Keepalives and re-packing on the same
// page apply directly. Nothing is allocated, so the CYD stays comfortable.
inline lv_obj_t *nav_prev=nullptr,*nav_next=nullptr,*nav_number=nullptr;
inline int applied_page=-1,target_page=0;
inline lv_timer_t *page_timer=nullptr;
// Slot assignment plus card widths and visibility for a page; contents are untouched.
inline int place_page(int page) {
  std::array<Placement,MAX_TILES> placement;
  int pages=pack(model.tiles,model.count,placement);
  page=std::clamp(page,0,pages-1);
  int wide_width=widgets[0].tile && widgets[1].tile ? lv_obj_get_x(widgets[1].tile)-lv_obj_get_x(widgets[0].tile)+widgets[0].base_width : 2*widgets[0].base_width;
  for(size_t slot=0;slot<widgets.size();++slot){widgets[slot].index=MAX_TILES;widgets[slot].wide=false;widgets[slot].cached_active=-1;}
  for(size_t i=0;i<model.count;++i)if(placement[i].page==page){auto &w=widgets[placement[i].slot];w.index=i;w.wide=model.tiles[i].wide;}
  for(size_t slot=0;slot<widgets.size();++slot){
    auto &w=widgets[slot];if(!w.tile)continue;
    if(slot<SLOTS_PER_PAGE && w.index<model.count){lv_obj_set_width(w.tile,w.wide?wide_width:w.base_width);lv_obj_remove_flag(w.tile,LV_OBJ_FLAG_HIDDEN);}
    else{lv_obj_add_flag(w.tile,LV_OBJ_FLAG_HIDDEN);end_extra(w);end_panel(w);}
  }
  for(auto *control:{nav_prev,nav_next,nav_number}){
    if(pages>1)lv_obj_remove_flag(control,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(control,LV_OBJ_FLAG_HIDDEN);
  }
  if(page==0)lv_obj_add_state(nav_prev,LV_STATE_DISABLED);else lv_obj_remove_state(nav_prev,LV_STATE_DISABLED);
  if(page==pages-1)lv_obj_add_state(nav_next,LV_STATE_DISABLED);else lv_obj_remove_state(nav_next,LV_STATE_DISABLED);
  for(auto *control:{nav_prev,nav_next})if(lv_obj_get_child_count(control))
    lv_obj_set_style_text_opa(lv_obj_get_child(control,0),lv_obj_has_state(control,LV_STATE_DISABLED)?LV_OPA_30:LV_OPA_COVER,0);
  std::string caption=std::to_string(page+1)+" / "+std::to_string(pages);lv_label_set_text(nav_number,caption.c_str());
  return page;
}
inline void apply_page(int page) {
  applied_page=place_page(page);
  if(room_label)render(room_label);else if(refresh)refresh();
}
inline void skeleton_page(int page) {
  place_page(page);
  uint32_t bg=0xF4F5F7, border=0xE3E5E8;
  for(size_t slot=0;slot<SLOTS_PER_PAGE;++slot){
    auto &w=widgets[slot];if(!w.tile || lv_obj_has_flag(w.tile,LV_OBJ_FLAG_HIDDEN))continue;
    for(auto *o:{w.title,w.value,w.circle,w.unit,w.slider,w.extra,w.panel,w.busy})if(o)lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
    // A card-less tile shows no skeleton frame either.
    bool bare=w.index<model.count && model.tiles[w.index].transparent;
    lv_obj_set_style_bg_opa(w.tile,bare?LV_OPA_TRANSP:LV_OPA_COVER,0);
    lv_obj_set_style_border_opa(w.tile,bare?LV_OPA_TRANSP:LV_OPA_COVER,0);
    lv_obj_set_style_bg_color(w.tile,lv_color_hex(bg),0);
    lv_obj_set_style_border_color(w.tile,lv_color_hex(border),0);
  }
}
inline void page_timer_done(lv_timer_t *) { page_timer=nullptr; apply_page(target_page); }
inline void show_page(int &page, lv_obj_t *previous, lv_obj_t *next, lv_obj_t *number) {
  nav_prev=previous;nav_next=next;nav_number=number;
  page=std::clamp(page,0,int(page_count())-1);
  target_page=page;
  if(applied_page<0 || page==applied_page){if(page_timer){lv_timer_delete(page_timer);page_timer=nullptr;}apply_page(page);return;}
  skeleton_page(page);
  if(page_timer)lv_timer_reset(page_timer);
  else{page_timer=lv_timer_create(page_timer_done,60,nullptr);lv_timer_set_repeat_count(page_timer,1);}
}

inline uint32_t last_live_second=0;
inline bool was_fresh=false;
inline void tick() {
  if(detail_root && !lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN) && detail_index<model.count){
    auto &t=model.tiles[detail_index];bool waiting=t.awaiting_action(esphome::millis());
    for(unsigned i=0;i<detail_action_count;++i){if(waiting||!fresh()||!t.available())lv_obj_add_state(detail_actions[i],LV_STATE_DISABLED);else lv_obj_remove_state(detail_actions[i],LV_STATE_DISABLED);}
    std::string status=waiting?(t.confirmed?"Bevestigd door Home Assistant":"Opdracht verstuurd..."):detail_state(t);
    if(detail_status)label(detail_status,status+(!waiting && !t.unit.empty()?" "+t.unit:""));
    if(detail_badge_status)label(detail_badge_status,status);
    if(detail_switch && !lv_obj_has_state(detail_switch,LV_STATE_PRESSED)){
      if(t.state=="on")lv_obj_add_state(detail_switch,LV_STATE_CHECKED);
      else lv_obj_remove_state(detail_switch,LV_STATE_CHECKED);
    }
  }
  if(!enabled)return;
  bool redraw=false;
  // HA dropping or returning and the feed timing out change every card at once.
  bool now_fresh=fresh();
  if(now_fresh!=was_fresh){was_fresh=now_fresh;redraw=true;}
  for(auto &t:model.tiles)if(t.pending && !t.loading(esphome::millis())){t.pending=false;redraw=true;}
  // A -/+ edit goes out as one call once the finger rests; a value HA never reports is dropped after a while.
  for(size_t i=0;i<model.count;++i){
    auto &t=model.tiles[i];if(!std::isfinite(t.edit_value))continue;
    uint32_t now=esphome::millis();
    if(!t.edit_sent){
      if(now-t.edit_since<700 || t.awaiting_action(now))continue;
      auto a=tile_controls::edit_action(t,t.edit_value);
      if(a.valid()){t.edit_sent=true;t.edit_since=now;action(a.service,t.entity,a.key,a.value);}else t.edit_value=NAN;
    }else if(now-t.edit_since>10000){t.edit_value=NAN;redraw=true;}
  }
  // Clocks and running timers advance once per second without any HA traffic.
  uint32_t second=esphome::millis()/1000;
  if(second!=last_live_second){
    last_live_second=second;
    for(size_t slot=0;slot<SLOTS_PER_PAGE;++slot){
      auto &w=widgets[slot];if(!w.tile || w.index>=model.count || lv_obj_has_flag(w.tile,LV_OBJ_FLAG_HIDDEN))continue;
      const auto &t=model.tiles[w.index];
      if(t.builtin() || (t.domain()=="timer" && t.state=="active") || (t.domain()=="sun" && second%60==0))redraw=true;
    }
  }
  if(redraw && refresh)refresh();
}
inline std::string vacuum_option(unsigned index) {
  if (active_index < 0 || static_cast<size_t>(active_index) >= model.count) return {};
  auto &tile = model.tiles[active_index];
  return index < tile.fan_speed_count ? tile.fan_speeds[index] : "";
}
}
