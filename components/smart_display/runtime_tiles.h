#pragma once
#include "runtime_model.h"
#include "tile_palette.h"
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
inline void tick();
inline void refresh_detail(unsigned index);
inline int active_index = -1;
inline uint32_t last_received = 0;
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
struct Widgets { lv_obj_t *tile{}, *title{}, *value{}, *circle{}, *icon{}; size_t index{}; int cached_active = -1; lv_obj_t *slider{}, *progress{}, *unit{}; int title_x=0,title_y=0,value_x=0,value_y=0; const lv_font_t *value_font{}, *icon_font{}; };
inline std::array<Widgets, 10> widgets;
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
      if(!root["swipe_pages"].isNull() && !root["swipe_pages"].is<bool>())return false;
      if(!root["rotation"].isNull() && (!root["rotation"].is<unsigned>() ||
          root["rotation"].as<unsigned>()>270 || root["rotation"].as<unsigned>()%90!=0))return false;
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
    tile.background = tile_palette::color(string(options["background"],16));
    tile.tap = string(options["tap"]); if (tile.tap.empty()) tile.tap="auto";
    tile.display = string(options["display"]); if (tile.display.empty()) tile.display="standard";
    tile.inline_control = string(options["inline"]); if (tile.inline_control.empty()) tile.inline_control="none";
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
    if(tile.domain()=="weather") {tile.current=number(a["temperature"]);tile.unit=string(a["temperature_unit"],12);}
    tile.modes = list(a["supported_color_modes"]);
    tile.hvac_modes = list(a["hvac_modes"]);
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
    tile.received = true;
    for(auto &w:widgets)if(w.index==index)w.cached_active=-1;
    last_received = esphome::millis();
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
  lv_obj_set_style_text_font(label,detail_font,0);lv_obj_set_style_text_color(label,lv_color_hex(light_theme?0x202020:0xFFFFFF),0);
  lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);lv_obj_set_height(label,lv_font_get_line_height(detail_font));return label;
}
inline std::string detail_state(const Tile &t){
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
    for(auto &w:widgets)if(w.slider==slider){index=w.index;break;}
    bool changed=slider_changed;captured_slider=nullptr;slider_changed=false;
    if(changed && cyd::touch_guard.accept_slider(esphome::millis(),200+index))commit_slider(index,lv_slider_get_value(slider));
  }
}
inline lv_obj_t *detail_button(const char *text,int x,int y,int width,int height,int command){
  auto *button=lv_obj_create(detail_root);lv_obj_remove_style_all(button);lv_obj_set_pos(button,x,y);lv_obj_set_size(button,width,height);
  lv_obj_set_style_bg_color(button,lv_color_hex(light_theme?(command==0?0x009FE3:0xD9E6F0):0x34495E),0);lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);lv_obj_set_style_radius(button,12,0);lv_obj_add_flag(button,LV_OBJ_FLAG_CLICKABLE);
  auto *label=detail_label(button,text,6,0,width-12);lv_obj_center(label);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);if(command==0)lv_obj_set_style_text_color(label,lv_color_hex(0xFFFFFF),0);lv_obj_remove_flag(label,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(button,[](lv_event_t *e){
    int cmd=(intptr_t)lv_event_get_user_data(e);if(cmd==-1){hide_detail();return;}
    if(!fresh()||detail_index>=model.count || !cyd::touch_guard.accept(esphome::millis(),300+cmd))return;
    auto &t=model.tiles[detail_index];if(!t.available()||t.loading(esphome::millis()))return;
    if(cmd<4){const char *services[]={"vacuum.start","vacuum.pause","vacuum.return_to_base","vacuum.locate"};action(services[cmd],t.entity);}
    if(cmd>=10 && cmd<14 && cmd-10<(int)t.fan_speed_count)action("vacuum.set_fan_speed",t.entity,"fan_speed",t.fan_speeds[cmd-10]);
    if(cmd==20)action("media_player.media_play_pause",t.entity);
    if(cmd==21)action("media_player.media_previous_track",t.entity);
    if(cmd==22)action("media_player.media_next_track",t.entity);
    if(cmd>=30 && cmd<38 && cmd-30<(int)t.option_count)action(t.domain()+".select_option",t.entity,"option",t.options[cmd-30]);
  },LV_EVENT_SHORT_CLICKED,(void*)(intptr_t)command);
  lv_obj_set_style_bg_color(button,lv_color_hex(0x0075B0),LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(button,-2,LV_STATE_PRESSED);lv_obj_set_style_transform_height(button,-2,LV_STATE_PRESSED);
  lv_obj_set_style_opa(button,LV_OPA_50,LV_STATE_DISABLED);
  if(command>=0 && detail_action_count<16)detail_actions[detail_action_count++]=button;
  return button;
}
inline void show_detail(unsigned index){
  if(index>=model.count)return;detail_index=index;auto &t=model.tiles[index];
  if(!detail_font)detail_font=lv_obj_get_style_text_font(widgets[0].title,LV_PART_MAIN);
  if(!detail_root){detail_root=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(detail_root);lv_obj_set_size(detail_root,lv_pct(100),lv_pct(100));lv_obj_remove_flag(detail_root,LV_OBJ_FLAG_SCROLLABLE);}
  detail_action_count=0;detail_status=nullptr;detail_badge_status=nullptr;detail_switch=nullptr;lv_obj_clean(detail_root);lv_obj_remove_flag(detail_root,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(detail_root);
  lv_obj_set_style_bg_color(detail_root,lv_color_hex(light_theme?0xE7E7E7:0x202A38),0);lv_obj_set_style_bg_opa(detail_root,LV_OPA_COVER,0);
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
    uint32_t surface=light_theme?0xFFFFFF:0x2B3B4D, muted=light_theme?0xEAF5FC:0x344D63;
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
      lv_obj_set_style_text_color(status,lv_color_hex(light_theme?0x087BA8:0xFFFFFF),0);
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
    char text[80];snprintf(text,sizeof(text),"%.1f %s",t.current,t.unit.c_str());auto *value=detail_label(detail_root,text,pad,top,width-2*pad);if(watch_font){lv_obj_set_style_text_font(value,watch_font,0);lv_obj_set_height(value,lv_font_get_line_height(watch_font));}
  }
}
}

namespace runtime_tiles {
inline void refresh_detail(unsigned index){
  if(!detail_root || lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN) || detail_index!=index)return;
  auto *input=lv_indev_get_next(nullptr);if(input && lv_indev_get_state(input)==LV_INDEV_STATE_PRESSED)return;
  show_detail(index);
}
inline const char *icon_for(const Tile &tile) {
  auto d = tile.domain();
  if (d == "light") return "\U000F0335";
  if (d == "climate") return "\U000F001B";
  if (d == "vacuum") return "\U000F070D";
  if (d == "fan") return "\U000F0210";
  if (d == "cover") return "\U000F111C";
  if (d == "scene" || d == "script") return "\U000F04B9";
  if (d == "weather") return "\U000F029A";
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
  if (!tile.available() || tile.loading(esphome::millis()) || tile.tap=="none") return;
  bool open = code == LV_EVENT_LONG_PRESSED || d == "climate" || d == "vacuum" || d == "cover";
  if(code==LV_EVENT_SHORT_CLICKED && tile.tap=="detail")open=true;
  if(code==LV_EVENT_SHORT_CLICKED && tile.tap=="toggle")open=false;
  if(d=="media_player" && tile.tap=="toggle" && code==LV_EVENT_SHORT_CLICKED){action("media_player.toggle",tile.entity);return;}
  if(d=="sensor" || d=="binary_sensor" || d=="weather" || d=="number" || d=="input_number" || d=="select" || d=="input_select" || d=="media_player" || d=="vacuum") { tile.begin(esphome::millis(),true); active_index=w.index; show_detail(w.index); return; }
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
  w.icon_font=lv_obj_get_style_text_font(icon,LV_PART_MAIN);
  w.unit=lv_label_create(tile);lv_obj_set_style_text_font(w.unit,w.value_font,0);lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
  // Fixed one-line boxes prevent wrapped names from overlapping the state on both boards.
  lv_obj_set_height(title,lv_font_get_line_height(lv_obj_get_style_text_font(title,LV_PART_MAIN)));
  lv_obj_set_height(value,lv_font_get_line_height(w.value_font));
  lv_label_set_long_mode(title,LV_LABEL_LONG_DOT);lv_label_set_long_mode(value,LV_LABEL_LONG_DOT);
  w.progress=lv_obj_create(tile);lv_obj_remove_style_all(w.progress);lv_obj_set_size(w.progress,0,3);lv_obj_align(w.progress,LV_ALIGN_BOTTOM_LEFT,0,0);lv_obj_set_style_bg_color(w.progress,lv_color_hex(0x00A6ED),0);lv_obj_set_style_bg_opa(w.progress,LV_OPA_COVER,0);
  w.slider=lv_slider_create(tile);lv_obj_set_size(w.slider,lv_obj_get_width(tile)-24,lv_obj_get_height(tile)>80?28:10);lv_obj_align(w.slider,LV_ALIGN_BOTTOM_MID,0,-1);lv_slider_set_range(w.slider,0,1000);
  lv_obj_set_style_bg_color(w.slider,lv_color_hex(0x111111),LV_PART_KNOB);lv_obj_set_style_pad_hor(w.slider,lv_obj_get_height(tile)>80?-10:0,LV_PART_KNOB);lv_obj_set_style_pad_ver(w.slider,lv_obj_get_height(tile)>80?-5:1,LV_PART_KNOB);lv_obj_set_style_radius(w.slider,14,LV_PART_MAIN);lv_obj_set_style_radius(w.slider,14,LV_PART_INDICATOR);lv_obj_set_style_radius(w.slider,3,LV_PART_KNOB);lv_obj_set_style_bg_color(w.slider,lv_color_hex(0xFCE5B4),LV_PART_MAIN);lv_obj_set_style_bg_color(w.slider,lv_color_hex(0xFFB900),LV_PART_INDICATOR);
  lv_obj_set_style_opa(w.slider,LV_OPA_TRANSP,LV_PART_KNOB);
  lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(w.slider,LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_event_cb(w.slider,slider_event,LV_EVENT_ALL,(void*)(uintptr_t)index);
  lv_obj_add_event_cb(tile, event, LV_EVENT_SHORT_CLICKED, &widgets[index]);
  lv_obj_add_event_cb(tile, event, LV_EVENT_LONG_PRESSED, &widgets[index]);
}
inline void label(lv_obj_t *obj, const std::string &text) {
  if (text != lv_label_get_text(obj)) lv_label_set_text(obj, text.c_str());
}
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
  if(d=="sensor"){
    if(t.unit=="lx")return 0xFFC107;
    if(t.unit=="°C" || t.unit=="°F")return 0xFF6F22;
    if(t.unit=="kWh" || t.unit=="Wh")return 0x926BC7;
    if(t.unit=="%")return 0x009688;
  }
  return 0x2196F3;
}
inline void render(lv_obj_t *room) {
  if (!enabled) return;
  label(room, !model.configured ? "Kies tegels in HA" : !model.ready() ? "Tegels laden..." : !fresh() ? "HA niet verbonden" : model.title);
  for (size_t slot = 0; slot < 6; ++slot) {
    auto &w=widgets[slot];
    if(!w.tile || w.index>=model.count)continue;
    const auto &t = model.tiles[w.index];
    label(w.title, t.name.empty() ? t.entity : t.name);
    label(w.icon, icon_for(t));
    bool watch=t.display=="watch";
    std::string unit=watch?t.unit:"";
    std::string value = t.state;
    if (!fresh() || !t.available()) value = "Niet beschikbaar";
    else if (t.domain() == "light" && t.state == "on" && std::isfinite(t.brightness)) value = std::to_string(static_cast<int>(std::lround(std::clamp(t.brightness, 0.0f, 255.0f) * 100 / 255))) + " %";
    else if (t.domain() == "climate" && std::isfinite(t.target)) { char b[32]; snprintf(b, sizeof(b), "%.1f°", t.target); value = b; }
    else if (value == "on") value = "Aan";
    else if (value == "off") value = "Uit";
    else if (value == "cleaning") value = "Bezig";
    else if (value == "docked") value = "In dock";
    else if (!t.unit.empty() && !watch) value += " " + t.unit;
    bool pending=t.loading(esphome::millis());
    if(t.domain()=="weather" && std::isfinite(t.current)) {char b[32];snprintf(b,sizeof(b),"%.1f %s",t.current,t.unit.c_str());value=b;if(watch){snprintf(b,sizeof(b),"%.1f",t.current);value=b;}}
    if(t.domain()=="vacuum" && std::isfinite(t.battery))value += " / "+std::to_string((int)t.battery)+"%";
    label(w.value, pending ? "Bezig..." : value);
    lv_obj_set_width(w.progress,pending ? (esphome::millis()/120%5+1)*(lv_obj_get_width(w.tile)-24)/5 : 0);
    lv_obj_set_style_text_opa(w.icon,pending ? LV_OPA_40 : LV_OPA_COVER,0);
    bool mini=t.inline_control=="slider" && !watch && t.available();
    bool large_tile=lv_obj_get_height(w.tile)>80;
    if(!large_tile){lv_obj_set_style_pad_top(w.tile,watch?2:4,0);lv_obj_set_style_pad_bottom(w.tile,watch?2:4,0);}
    lv_obj_set_style_text_font(w.value,watch && watch_value_font ? watch_value_font : w.value_font,0);
    lv_obj_set_height(w.value,lv_font_get_line_height(lv_obj_get_style_text_font(w.value,LV_PART_MAIN)));
    lv_obj_update_layout(w.tile);
    int content_width=lv_obj_get_content_width(w.tile),content_height=lv_obj_get_content_height(w.tile);
    int title_height=lv_obj_get_height(w.title),value_height=lv_obj_get_height(w.value);
    int line_gap=large_tile?2:1,text_height=title_height+line_gap+value_height;
    int slider_height=large_tile?28:8;
    int header_height=mini?content_height-slider_height-(large_tile?6:3):content_height;
    int text_y=std::max(0,(header_height-text_height)/2);
    int circle_size=watch?(large_tile?26:18):mini?(large_tile?36:24):(large_tile?54:36);
    lv_obj_set_size(w.circle,circle_size,circle_size);
    lv_obj_set_style_text_font(w.icon,watch && watch_icon_font ? watch_icon_font : mini && mini_icon_font ? mini_icon_font : w.icon_font,0);
    lv_obj_center(w.icon);
    lv_obj_remove_flag(w.circle,LV_OBJ_FLAG_HIDDEN);
    int text_x=watch?0:mini?circle_size+(large_tile?8:6):w.title_x;
    lv_obj_set_pos(w.title,text_x,watch?0:(mini || !large_tile)?text_y:w.title_y);
    lv_obj_set_pos(w.value,watch?0:mini?text_x:w.value_x,
      watch?(large_tile?42:19):(mini || !large_tile)?text_y+title_height+line_gap:w.value_y);
    lv_obj_set_pos(w.circle,0,(mini || !large_tile)?std::max(0,(header_height-circle_size)/2):12);
    // Use the requested coordinates: LVGL getters still return the previous
    // layout until its next pass when a slot changes from watch/slider to normal.
    lv_obj_set_width(w.title,std::max(1,content_width-text_x));
    lv_obj_set_width(w.value,std::max(1,content_width-(watch?0:mini?text_x:w.value_x)));
    if(watch){
      int gap=large_tile?6:2,header=std::max(circle_size,title_height);
      int group_y=std::max(0,(content_height-header-gap-value_height)/2);
      int value_y=group_y+header+gap;
      lv_obj_set_pos(w.circle,0,group_y+(header-circle_size)/2);
      lv_obj_set_pos(w.title,circle_size+(large_tile?6:4),group_y+(header-title_height)/2);
      lv_obj_set_width(w.title,content_width-circle_size-(large_tile?6:4));
      label(w.unit,unit);
      lv_point_t size;lv_text_get_size(&size,unit.c_str(),w.value_font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
      int unit_width=unit.empty()?0:std::min((int)size.x,content_width-20);
      int number_width=content_width-(unit_width?unit_width+(large_tile?6:3):0);
      lv_obj_set_pos(w.value,0,value_y);lv_obj_set_width(w.value,number_width);
      lv_obj_set_pos(w.unit,content_width-unit_width,value_y+value_height-lv_font_get_line_height(w.value_font));
      lv_obj_set_size(w.unit,unit_width,lv_font_get_line_height(w.value_font));
      if(unit_width)lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
    }else lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(w.slider,content_width,slider_height);
    lv_obj_align(w.slider,LV_ALIGN_BOTTOM_MID,0,0);
    if(mini){lv_obj_remove_flag(w.slider,LV_OBJ_FLAG_HIDDEN);if(!lv_obj_has_state(w.slider,LV_STATE_PRESSED))lv_slider_set_value(w.slider,slider_value(t),LV_ANIM_OFF);}
    else lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
    bool on = fresh() && t.active();
    bool available=fresh() && t.available();
    int palette_state=(available?2:0)|(on?1:0);
    if (w.cached_active == palette_state) continue;
    w.cached_active = palette_state;
    uint32_t accent = domain_accent(t);
    auto color=lv_color_hex(t.is_switch() && !on ? 0x9E9E9E : accent);
    if(t.domain()=="light" && on && t.has_hs_color)
      color=lv_color_hsv_to_rgb(t.hue%360,t.saturation,100);
    bool dark_text=light_theme || t.background!=0;
    auto circle_color=available?lv_color_mix(color,lv_color_hex(dark_text?0xFFFFFF:0x263B50),dark_text?38:65):lv_color_hex(dark_text?0xF0F0F0:0x263B50);
    // Darken the foreground slightly: very pale bulbs still need a visible icon.
    auto icon_color=available?lv_color_mix(color,lv_color_hex(dark_text?0x333333:0xFFFFFF),dark_text?205:185):lv_color_hex(0x9E9E9E);
    lv_obj_set_style_bg_color(w.slider,color,LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(w.slider,lv_color_mix(color,lv_color_hex(dark_text?0xFFFFFF:0x263B50),30),LV_PART_MAIN);
    lv_obj_set_style_bg_color(w.tile, lv_color_hex(t.background ? t.background : (light_theme ? 0xFFFFFF : (on ? 0xF5F1E8 : 0x526C85))), 0);
    lv_obj_set_style_border_width(w.tile, 1, 0);
    lv_obj_set_style_border_opa(w.tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(w.tile, t.background ? lv_color_mix(lv_color_hex(t.background),lv_color_hex(0x000000),220) : lv_color_hex(light_theme ? 0xDDDDDD : (on ? 0xF5F1E8 : 0x9CB3C8)), 0);
    lv_obj_set_style_bg_color(w.circle,circle_color,0);
    lv_obj_set_style_text_color(w.icon,icon_color,0);
    lv_obj_set_style_text_color(w.unit,lv_color_hex(dark_text?0x46525E:(on?0x46525E:0xF0F4F8)),0);
    lv_obj_set_style_text_color(w.title, lv_color_hex(dark_text ? 0x1B1B1B : (on ? 0x172232 : 0xF3F5F7)), 0);
    lv_obj_set_style_text_color(w.value, lv_color_hex(t.background ? 0x46525E : (light_theme ? 0x616161 : (on ? 0x46525E : 0xF0F4F8))), 0);
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
    bool fits=title.x1>=content.x1 && title.x2<=content.x2 &&
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
      if(!watch && (mini || lv_obj_get_height(w.tile)<=80)){
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
    if(!fits)ESP_LOGE("ui_test","Tile geometry FAIL slot=%u title_y=%d..%d value_y=%d..%d content_y=%d..%d",(unsigned)w.index,title.y1,title.y2,value.y1,value.y2,content.y1,content.y2);
    if(w.index<model.count && model.tiles[w.index].background){
      bool palette_ok=lv_color_eq(lv_obj_get_style_bg_color(w.tile,LV_PART_MAIN),lv_color_hex(model.tiles[w.index].background)) &&
        lv_color_eq(lv_obj_get_style_text_color(w.title,LV_PART_MAIN),lv_color_hex(0x1B1B1B));
      if(!palette_ok)ESP_LOGE("ui_test","Tile palette FAIL slot=%u",(unsigned)w.index);
      fits=fits && palette_ok;
    }
    ok=ok && fits;
  }
  return ok;
}

inline void show_page(int &page, lv_obj_t *previous, lv_obj_t *next, lv_obj_t *number) {
  int pages=std::max(1,(int(model.count)+5)/6);
  page=std::clamp(page,0,pages-1);
  for(size_t slot=0;slot<widgets.size();++slot){
    auto &w=widgets[slot];if(!w.tile)continue;
    if(slot<6){w.index=page*6+slot;w.cached_active=-1;}
    if(slot<6 && w.index<model.count)lv_obj_remove_flag(w.tile,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(w.tile,LV_OBJ_FLAG_HIDDEN);
  }
  for(auto *control:{previous,next,number}){
    if(pages>1)lv_obj_remove_flag(control,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(control,LV_OBJ_FLAG_HIDDEN);
  }
  if(page==0)lv_obj_add_state(previous,LV_STATE_DISABLED);else lv_obj_remove_state(previous,LV_STATE_DISABLED);
  if(page==pages-1)lv_obj_add_state(next,LV_STATE_DISABLED);else lv_obj_remove_state(next,LV_STATE_DISABLED);
  std::string caption=std::to_string(page+1)+" / "+std::to_string(pages);lv_label_set_text(number,caption.c_str());
  if(refresh)refresh();
}

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
  for(auto &t:model.tiles)if(t.pending){redraw=true;if(!t.loading(esphome::millis()))t.pending=false;}
  if(redraw && refresh)refresh();
}
inline std::string vacuum_option(unsigned index) {
  if (active_index < 0 || static_cast<size_t>(active_index) >= model.count) return {};
  auto &tile = model.tiles[active_index];
  return index < tile.fan_speed_count ? tile.fan_speeds[index] : "";
}
}
