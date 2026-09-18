#pragma once
#include "runtime_model.h"
#include "header_bar.h"
#include "tile_palette.h"
#include "theme.h"
#include "tile_icon.h"
#include "screen_settings.h"
#include "settings_screen.h"
#include "esphome/core/preferences.h"
#include "cyd_ui.h"
#include "light_controls.h"
#include "tile_controls.h"
#include "history_view.h"
#include "camera_view.h"
#include "media_card.h"
#include "swipe_profile.h"
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
// Swiping, rotation and going back to page 1 live in settings_screen, next to the other settings the
// screen can change itself; these names stay as the way the rest of the firmware reaches them.
using settings_screen::swipe_pages;
using settings_screen::rotation;
using settings_screen::rotation_supported;
using settings_screen::auto_home;
using settings_screen::auto_home_seconds;
inline esphome::ESPPreferenceObject rotation_preference;
inline esphome::ESPPreferenceObject swipe_preference;
inline esphome::ESPPreferenceObject home_preference;
inline esphome::ESPPreferenceObject dark_preference;
inline Model model;
inline std::string inbox;
inline const lv_font_t *watch_font = nullptr;
inline const lv_font_t *mini_icon_font = nullptr;
// The large icon font of a full-page card (firmware 0.2.62+): the domain icons only, so the flash stays free.
inline const lv_font_t *big_icon_font = nullptr;
// Page shown by the navigation bar and the swipe (the profile's tile_page), remembered by show_page.
inline int *shown_page = nullptr;
inline void go_to_page(int page);
inline const lv_font_t *watch_value_font = nullptr, *watch_icon_font = nullptr;
// Big digits for the clock card; the board profile sets it with the local time source.
inline const lv_font_t *clock_font = nullptr;
// Text in the -/+ pill and the run key of direct controls; the board profile sets it.
inline const lv_font_t *control_font = nullptr;
// The smallest regular text (sublabel): axis labels and the legend of the history card.
inline const lv_font_t *small_font = nullptr;
inline lv_obj_t *room_label = nullptr;  // remembered by render() so page switches can render synchronously
// Top bar (0.2.32+): the profile's clock label only lends its place and margin; values use the
// profile's text font, icons its small icon font. Set by the board profile at boot.
inline header_bar::Bar header;
inline lv_obj_t *time_label = nullptr;
inline const lv_font_t *header_text_font = nullptr, *header_icon_font = nullptr;
inline void render_header();
inline std::function<esphome::ESPTime()> now_time;
// True while the screen is in use: the profile reports standby (also the night level) as not awake.
// The analog clock's second hand runs only then; a standby screen stays on its minute redraws.
inline std::function<bool()> screen_awake;
inline bool awake() { return !screen_awake || screen_awake(); }
inline uint32_t now_epoch() { if (!now_time) return 0; auto t = now_time(); return t.is_valid() ? static_cast<uint32_t>(t.timestamp) : 0; }
inline void tick();
inline void refresh_tile(size_t index);
// Style setters that only touch a property when it changes (defined with the card renderers below).
inline void set_color(lv_obj_t *obj, lv_style_prop_t prop, lv_color_t color, lv_style_selector_t selector = 0);
inline void set_number(lv_obj_t *obj, lv_style_prop_t prop, int32_t number, lv_style_selector_t selector = 0);
inline void set_font(lv_obj_t *obj, const lv_font_t *value);
#ifdef SWIPE_PROFILE
inline void swipe_test(unsigned count, unsigned interval_ms, unsigned back_ms, JsonObject tuning);
#endif
inline void refresh_header_only();
inline void refresh_all();
inline void refresh_detail(unsigned index);
inline const char *weather_icon(const std::string &condition);
inline const char *weather_text(const std::string &condition);
inline std::string timer_text(const Tile &t);
inline std::string last_run_text(uint32_t epoch);
inline void history_received();
// Camera images full screen and on an alert (firmware 0.2.57+, the Guition binds them; see the end of this file).
inline void camera_open(const std::string &entity, const std::string &name);
inline void camera_answer(const std::string &view, const std::string &entity, const std::string &url);
inline bool camera_supported();
// The media card's album cover (firmware 0.2.64+): the card or a tile over the whole page says which cover it shows,
// the board's online_image loads it; see the end of this file.
enum class CoverOwner : uint8_t { NONE, DETAIL, TILE };
inline void cover_want(const std::string &entity, const std::string &picture, int size, uint32_t background, CoverOwner owner, size_t slot);
inline lv_image_dsc_t *cover_ready(const std::string &entity, int size, uint32_t background);
// The card's cover on screen, and the part a media tile keeps its cover in; both go with the image buffer.
inline lv_obj_t *media_detail_picture = nullptr;
constexpr unsigned MEDIA_PICTURE = 14;
inline void media_action(Tile &t, int cmd);
inline uint32_t domain_accent(const Tile &t);
inline const char *icon_for(const Tile &tile);
inline void label(lv_obj_t *obj, const std::string &text);
inline int active_index = -1;
inline uint32_t last_received = 0;
// Seconds between the manager's full repeats; every layout message declares it (app
// 0.2.26+). Older managers get the 120 s that app 0.2.20 introduced.
inline uint32_t keepalive_seconds = 120;
// Revision of the layout the manager sent last (app 0.2.39+). A keepalive ping carries the
// manager's revision; a mismatch (a restart, a demo layout) asks for the whole layout again.
inline std::string layout_rev;
// History on a detail card (firmware 0.2.51+): the last history the manager sent (app 0.2.59+ answers
// history_request with op "history"), for the card that asked.
struct HistoryState { std::string label; uint32_t color = 0; uint32_t seconds = 0; };
struct History {
  std::string entity, unit;
  uint32_t hours = 0, start = 0, end = 0, began = 0;
  int32_t offset = 0;
  bool line = true, has_high = false, has_low = false;
  float values[history_view::PARTS]{};
  bool has[history_view::PARTS]{};
  float bottom = 0, top = 1, high = 0, low = 0;
  uint32_t high_at = 0, low_at = 0;
  int decimals = 1, active = -1;
  std::vector<std::pair<float, std::string>> ticks;
  std::vector<uint32_t> times;
  uint16_t slots = 0;
  std::vector<history_view::Run> runs;
  std::vector<HistoryState> states;
  // Home Assistant's words for the states (raw state, words), so the heading follows a state that changes.
  std::vector<std::pair<std::string, std::string>> words;
};
inline History history;
// The range the open card shows (1, 24 or 168 hours); what it asked for last and when; whether that answer came.
inline uint32_t history_hours = 24, history_asked_at = 0, history_asked_hours = 0, history_received_at = 0;
inline std::string history_asked_entity;
inline bool history_answered = false;
inline std::function<void()> layout_changed, refresh, dismiss, settings_changed;
inline esphome::ESPPreferenceObject settings_preference;
// The two extra values of 0.2.44+ in one record: going back to page 1 by itself, and after how long.
struct HomeTimeout { uint32_t enabled = 1, seconds = 120; };
inline void load_settings() {
  settings_preference = esphome::global_preferences->make_preference<screen_settings::Settings>(0x53435231);
  swipe_preference = esphome::global_preferences->make_preference<uint32_t>(0x53575031);
  home_preference = esphome::global_preferences->make_preference<HomeTimeout>(0x484F4D31);
  dark_preference = esphome::global_preferences->make_preference<uint32_t>(0x44524B31);
  uint32_t swipe_saved=0;
  if(swipe_preference.load(&swipe_saved))swipe_pages=swipe_saved==1;
  uint32_t dark_saved=0;
  if(dark_preference.load(&dark_saved))settings_screen::dark_mode=dark_saved==1;
  HomeTimeout home;
  if(home_preference.load(&home) && home.seconds>=30 && home.seconds<=3600){
    auto_home=home.enabled?1:0;auto_home_seconds=(int32_t)home.seconds;
  }
  if(rotation_supported){
    rotation_preference=esphome::global_preferences->make_preference<uint32_t>(0x524F5431);
    uint32_t saved=0;
    if(rotation_preference.load(&saved) && saved<=270 && saved%90==0)rotation=(int32_t)saved;
  }
  screen_settings::Settings saved;
  if (settings_preference.load(&saved) && saved.valid()) screen_settings::current = saved;
}
// Everything the screen remembers, written in one go. ESPHome batches the flash writes, so a row of
// taps on -/+ costs one write, and a value that did not change costs nothing.
inline void persist_settings() {
  if (screen_settings::current.valid()) settings_preference.save(&screen_settings::current);
  uint32_t swipe = swipe_pages ? 1 : 0;
  swipe_preference.save(&swipe);
  HomeTimeout home{(uint32_t) (auto_home ? 1 : 0), (uint32_t) std::clamp<int32_t>(auto_home_seconds, 30, 3600)};
  home_preference.save(&home);
  uint32_t dark = settings_screen::dark_mode ? 1 : 0;
  dark_preference.save(&dark);
  if (rotation_supported) { uint32_t turned = (uint32_t) rotation; rotation_preference.save(&turned); }
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
  // Full (firmware 0.2.62+) takes the whole page: the double-width card's head on top, a control at the bottom.
  bool wide=false, full=false; int base_width=0, base_height=0; const lv_font_t *title_font{};
  // `extra_full`: the size the parts were built for; a slot that changes between full and double width rebuilds them.
  lv_obj_t *extra{}; std::string extra_mode; bool extra_full=false; std::array<lv_obj_t *, 36> parts{}; lv_point_precise_t *points{};
  // Analog clock: centre and radius of the dial, so the second hand can move without a card redraw.
  int hand_cx=0, hand_cy=0, hand_r=0, hand_width=1;
  // A media tile over the whole page (firmware 0.2.64+): the width of its progress bar, so the fill can run once a
  // second without a card redraw.
  int media_bar_w=0;
  // Soft area under a polyline (graph, sun path), painted by the extra container's draw event.
  const lv_point_precise_t *fill_points{}; unsigned fill_count=0; int fill_x=0, fill_y=0, fill_base=0; lv_color_t fill_color{}; lv_opa_t fill_opa=0;
  // Direct controls on a wide card: a panel at the right with pill keys, a -/+ pill,
  // a slider or a toggle. Objects are rebuilt only when the control set changes.
  lv_obj_t *panel{}; std::string panel_mode; bool panel_dirty=false, panel_full=false; int panel_w=0;
  std::array<lv_obj_t *,3> keys{}, key_icons{}; std::array<int,3> key_commands{}; std::array<std::string,3> key_args; std::array<int,3> key_checked{};
  lv_obj_t *pill{}, *pill_value{}, *knob{}, *control_slider{}; int knob_on=-1;
  lv_color_t panel_accent{}, panel_text{};
  // Busy sheet: a translucent white cover with a small spinner while a command is under way.
  lv_obj_t *busy{}, *spinner{}; bool busy_drawn=false;
  // Page fill skeleton: a sheet in the card's colour over its contents until the card is drawn.
  lv_obj_t *veil{};
};
constexpr unsigned POINT_BUFFER = 128;
inline std::array<Widgets, 10> widgets;
// All icon fonts carry the same generated glyph set, so the first bound one answers for all.
inline bool has_icon_glyph(uint32_t codepoint) {
  for (auto &w : widgets) if (w.icon_font) { lv_font_glyph_dsc_t dsc; return lv_font_get_glyph_dsc(w.icon_font, &dsc, codepoint, 0); }
  return false;
}
// Whether `font` draws the glyph a label's UTF-8 text starts with (a four-byte Material Design icon).
inline bool font_has(const lv_font_t *font, const std::string &utf8) {
  if (!font || utf8.size() < 4) return false;
  const auto *b = reinterpret_cast<const unsigned char *>(utf8.data());
  uint32_t cp = (uint32_t(b[0] & 7) << 18) | (uint32_t(b[1] & 0x3F) << 12) | (uint32_t(b[2] & 0x3F) << 6) | (b[3] & 0x3F);
  lv_font_glyph_dsc_t dsc; return lv_font_get_glyph_dsc(font, &dsc, cp, 0);
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
  ESP_LOGI("touch", "tap on %s ignored: %s", what.c_str(), cyd::touch_guard.reason().c_str());
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
  if (!enabled) return "Use the Easy Setup profile";
  if (payload.size() > 4096) return "Error: message too large";
  std::string result = "Error: invalid message";
  esphome::json::parse_json(payload, [&](JsonObject root) -> bool {
    if (root["v"].as<int>() != 1) { result = "Error: protocol version"; return false; }
    auto op = string(root["op"]);
    if (op == "layout") {
      if (!root["entities"].is<JsonArray>() || !root["title"].is<const char *>()) return false;
      auto settings = screen_settings::current;
      if (!root["settings"].isNull() && (!root["settings"].is<JsonObject>() ||
          !parse_settings(root["settings"].as<JsonObject>(), settings))) {
        result = "Error: screen settings"; return false;
      }
      std::vector<std::string> entities;
      for (JsonVariant entity : root["entities"].as<JsonArray>()) {
        if (!entity.is<const char *>() || entities.size() == MAX_TILES) return false;
        entities.push_back(entity.as<std::string>());
      }
      if(!root["swipe_pages"].isNull() && !root["swipe_pages"].is<bool>())return false;
      if(!root["auto_home"].isNull() && !root["auto_home"].is<bool>())return false;
      if(!root["auto_home_seconds"].isNull() && (!root["auto_home_seconds"].is<unsigned>() ||
          root["auto_home_seconds"].as<unsigned>()<30 || root["auto_home_seconds"].as<unsigned>()>3600))return false;
      if(!root["rotation"].isNull() && (!root["rotation"].is<unsigned>() ||
          root["rotation"].as<unsigned>()>270 || root["rotation"].as<unsigned>()%90!=0))return false;
      if(!root["keepalive"].isNull() && (!root["keepalive"].is<unsigned>() ||
          root["keepalive"].as<unsigned>()<5 || root["keepalive"].as<unsigned>()>3600))return false;
      // Explicit grid positions (0.2.26+), one absolute slot per entity; absent on older managers.
      std::vector<uint8_t> positions;
      if (!root["slots"].isNull()) {
        if (!root["slots"].is<JsonArray>()) return false;
        for (JsonVariant slot : root["slots"].as<JsonArray>()) {
          if (!slot.is<unsigned>() || slot.as<unsigned>() >= MAX_SLOTS || positions.size() == MAX_TILES) return false;
          positions.push_back(static_cast<uint8_t>(slot.as<unsigned>()));
        }
      }
      inbox = string(root["inbox"], 160);
      bool changed = false, moved = false, was_configured = model.configured;
      const std::string previous_title = model.title;
      const uint8_t previous_pages = model.pages;
      if (!model.set_layout(entities, string(root["title"], 96), changed, positions, moved)) {
        if (!model.refusal.empty()) {
          ESP_LOGW("runtime", "Layout of %u tiles refused: %s", static_cast<unsigned>(entities.size()), model.refusal.c_str());
          result = model.refusal;
        }
        return false;
      }
      // Empty pages the user keeps on purpose; absent on older managers.
      model.pages = root["pages"].is<unsigned>() ? static_cast<uint8_t>(std::clamp<unsigned>(root["pages"].as<unsigned>(), 1, MAX_PAGES)) : 1;
      if(root["swipe_pages"].is<bool>() && swipe_pages!=root["swipe_pages"].as<bool>()){
        swipe_pages=root["swipe_pages"].as<bool>();uint32_t saved=swipe_pages?1:0;swipe_preference.save(&saved);
      }
      if((root["auto_home"].is<bool>() && auto_home!=root["auto_home"].as<bool>()) ||
         (root["auto_home_seconds"].is<unsigned>() && auto_home_seconds!=(int32_t)root["auto_home_seconds"].as<unsigned>())){
        if(root["auto_home"].is<bool>())auto_home=root["auto_home"].as<bool>();
        if(root["auto_home_seconds"].is<unsigned>())auto_home_seconds=(int32_t)root["auto_home_seconds"].as<unsigned>();
        HomeTimeout home{(uint32_t)(auto_home?1:0),(uint32_t)auto_home_seconds};home_preference.save(&home);
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
      if (moved) ESP_LOGI("runtime", "tiles moved: pages rearranged");
      if (root["keepalive"].is<unsigned>()) keepalive_seconds = root["keepalive"].as<unsigned>();
      layout_rev = string(root["rev"], 16);
      last_received = esphome::millis();
      // A repeat of the layout on screen (the hourly repeat, a save that changed nothing here) only
      // refreshes the feed: drawing the whole page again stalls touch input for a few hundred ms,
      // long enough to spoil a slider drag. The tile states that follow redraw their own tiles.
      if (changed || moved || !was_configured || rotation_changed || model.pages != previous_pages || model.title != previous_title) {
        if (layout_changed) layout_changed();
        refresh_all();
      }
      // A repeat of the same layout keeps the inbox state as it is: no new recorder row.
      result = changed || moved || !was_configured ? "Layout received" : model.ready() ? "Synced" : "Loading tiles";
      return true;
    }
    if (op == "ping") {
      // Keepalive without content (app 0.2.39+): only the revision of the layout the manager holds.
      if (!root["rev"].is<const char *>()) return false;
      if(!root["keepalive"].isNull() && (!root["keepalive"].is<unsigned>() ||
          root["keepalive"].as<unsigned>()<5 || root["keepalive"].as<unsigned>()>3600))return false;
      if (root["keepalive"].is<unsigned>()) keepalive_seconds = root["keepalive"].as<unsigned>();
      last_received = esphome::millis();
      if (!model.configured || layout_rev != string(root["rev"], 16)) { result = "Resend needed"; return true; }
      result = model.ready() ? "Synced" : "Loading tiles";
      return true;
    }
    if (op == "header") {
      // Validate every item before replacing the bar; an icon these fonts lack is left out.
      if (!root["items"].is<JsonArray>()) return false;
      header_bar::Bar next;
      for (JsonVariant value : root["items"].as<JsonArray>()) {
        if (next.count == header_bar::MAX_ITEMS || !value.is<JsonObject>()) return false;
        header_bar::Item item;
        item.kind = header_bar::kind(string(value["k"], 8));
        if (item.kind == header_bar::Kind::none) return false;
        uint32_t icon = tile_icon::codepoint(string(value["i"], 8));
        item.icon = icon && has_icon_glyph(icon) ? icon : 0;
        item.text = string(value["t"], header_bar::TEXT_BYTES);
        item.epoch = value["e"].is<unsigned>() ? value["e"].as<uint32_t>() : 0;
        item.has_color = header_bar::color(string(value["c"], 8), item.color);
        if (item.kind == header_bar::Kind::ago && item.epoch <= 0) return false;
        next.items[next.count++] = item;
      }
      next.received = true;
      bool same = header.received && header.count == next.count;
      for (size_t i = 0; same && i < next.count; ++i) same = header.items[i] == next.items[i];
      header = next;
      last_received = esphome::millis();
      if (!same) refresh_header_only();
      // The same status as a tile state, so a changing value never flips the inbox entity.
      result = model.ready() ? "Synced" : "Loading tiles";
      return true;
    }
    if (op == "camera") {
      // A link to a camera's image (app 0.2.66+): the answer to camera_request ("full"), or an alert's image ("alert",
      // announced with an empty link before show_alert and sent again with the link), or the media card's cover
      // ("cover", app 0.2.77+). Only ESP Screens' own port.
      const std::string view = string(root["t"], 8), entity = string(root["e"], 120), url = string(root["u"], 240);
      if (!valid_entity(entity) || (view != "full" && view != "alert" && view != "cover")) return false;
      if (!url.empty() && url.rfind("http://", 0) != 0) return false;
      camera_answer(view, entity, url);
      result = model.ready() ? "Synced" : "Loading tiles";
      return true;
    }
    if (op == "history") {
      // A detail card's history (app 0.2.59+), the answer to history_request. Checked whole before it replaces
      // the one the screen holds.
      History next;
      next.entity = string(root["entity"], 120);
      next.hours = root["hours"] | 0u;
      if (!valid_entity(next.entity) || (next.hours != 1 && next.hours != 24 && next.hours != 168)) return false;
      // An answer to an earlier question (another card, another range) would push out the one the card waits for.
      if (next.entity != history_asked_entity || next.hours != history_asked_hours) {
        result = model.ready() ? "Synced" : "Loading tiles";
        return true;
      }
      next.start = root["start"] | 0u;
      next.end = root["end"] | 0u;
      next.offset = std::clamp(root["off"] | 0, -14 * 3600, 14 * 3600);
      if (next.end <= next.start) return false;
      next.line = string(root["kind"], 12) != "timeline";
      if (root["xt"].is<JsonArray>()) for (JsonVariant moment : root["xt"].as<JsonArray>()) {
        if (next.times.size() == 8) break;
        if (moment.is<unsigned>()) next.times.push_back(moment.as<uint32_t>());
      }
      if (next.line) {
        unsigned i = 0;
        if (root["values"].is<JsonArray>()) for (JsonVariant value : root["values"].as<JsonArray>()) {
          if (i == history_view::PARTS) break;
          float v = number(value);
          next.has[i] = std::isfinite(v);
          next.values[i] = next.has[i] ? v : 0;
          ++i;
        }
        next.bottom = number(root["dom"][0], 0);
        next.top = number(root["dom"][1], 1);
        if (!(next.top > next.bottom)) next.top = next.bottom + 1;
        if (root["yt"].is<JsonArray>()) for (JsonVariant tick : root["yt"].as<JsonArray>()) {
          if (next.ticks.size() == 6) break;
          float v = number(tick[0]);
          if (std::isfinite(v)) next.ticks.emplace_back(v, string(tick[1], 16));
        }
        next.high = number(root["hi"][0]);
        next.has_high = std::isfinite(next.high);
        next.high_at = root["hi"][1] | 0u;
        next.low = number(root["lo"][0]);
        next.has_low = std::isfinite(next.low);
        next.low_at = root["lo"][1] | 0u;
        next.decimals = std::clamp(root["dec"] | 1, 0, 4);
        next.unit = string(root["unit"], 16);
      } else {
        next.slots = static_cast<uint16_t>(std::clamp(root["slots"] | 96u, 1u, 96u));
        if (root["states"].is<JsonArray>()) for (JsonVariant state : root["states"].as<JsonArray>()) {
          if (next.states.size() == 7) break;
          HistoryState s;
          s.label = string(state[0], 24);
          s.color = std::strtoul(string(state[1], 8).c_str(), nullptr, 16);
          s.seconds = state[2] | 0u;
          next.states.push_back(std::move(s));
        }
        if (root["seg"].is<JsonArray>()) for (JsonVariant item : root["seg"].as<JsonArray>()) {
          if (next.runs.size() == 96) break;
          const unsigned slot = item[0] | 0u;
          const int state = item[1] | -1;
          if (slot >= next.slots || state >= static_cast<int>(next.states.size()) || state < -1) return false;
          history_view::Run run;
          run.slot = static_cast<uint16_t>(slot);
          run.state = static_cast<int8_t>(state);
          run.begin = item[2] | 0u;
          run.end = std::max<uint32_t>(run.begin, item[3] | 0u);
          run.seconds = item[4] | 0u;
          next.runs.push_back(run);
        }
        if (root["words"].is<JsonArray>()) for (JsonVariant pair : root["words"].as<JsonArray>()) {
          if (next.words.size() == 12) break;
          next.words.emplace_back(string(pair[0], 32), string(pair[1], 24));
        }
        next.began = root["began"] | 0u;
        next.active = root["active"] | -1;
        if (next.active >= static_cast<int>(next.states.size())) next.active = -1;
      }
      history = std::move(next);
      history_received_at = esphome::millis();
      history_answered = true;
      history_received();
      result = model.ready() ? "Synced" : "Loading tiles";
      return true;
    }
#ifdef SWIPE_PROFILE
    if (op == "swipe_test") {
      // Diagnostic builds only: page switches without a finger, `n` of them `ms` apart; `back`
      // (ms) swipes straight back after each one, like a quick second swipe.
      swipe_test(root["n"] | 20u, root["ms"] | 1200u, root["back"] | 0u, root);
      result = "Swipe test started";
      return true;
    }
#endif
    if (op != "state" || !root["i"].is<unsigned>() || !root["a"].is<JsonObject>()) return false;
    unsigned index = root["i"].as<unsigned>();
    std::string entity = string(root["entity"], 120);
    if (!model.accepts(index, entity)) { result = "Error: outdated tile"; return false; }
    Tile &tile = model.tiles[index];
    auto a = root["a"].as<JsonObject>();
    // The fingerprint of state, attributes and extras (a vacuum's mode or water select changes only
    // there), hashed while serializing so no copy of the message stays behind.
    Fingerprint revision;
    revision.add(string(root["state"]));
    revision.write('\n');
    serializeJson(a, revision);
    if (!root["x"].isNull()) serializeJson(root["x"], revision);
    bool was_confirmed=tile.confirmed;
    tile.observe(revision.value);
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
    bool was_wide = tile.wide, was_full = tile.full;
    std::string size = string(options["size"]);
    tile.full = size == "full";
    tile.wide = tile.full || size == "wide";
    bool repack = was_wide != tile.wide || was_full != tile.full;
    // Pre-computed extras: the manager converts time zones and fetches forecasts. What only some tiles
    // carry is collected in `next` and replaces the tile's Extra at the end (see Tile::set_extra).
    auto extra = root["x"];
    Extra next;
    // A tap's own Home Assistant action (app 0.2.67+): {"s": action, "d": [[key, text]], "t": [[key, template]]}.
    auto act = options["act"];
    if (act.is<JsonObject>()) {
      std::string service = string(act["s"], 64);
      auto pairs = [](JsonVariant list, std::vector<std::pair<std::string, std::string>> &out) {
        if (!list.is<JsonArray>()) return;
        for (JsonVariant pair : list.as<JsonArray>()) {
          if (out.size() == 8 || !pair.is<JsonArray>() || pair.as<JsonArray>().size() != 2) continue;
          std::string key = string(pair[0], 32);
          if (!key.empty()) out.emplace_back(std::move(key), string(pair[1], 400));
        }
      };
      if (valid_action(service)) {
        next.action = std::move(service);
        pairs(act["d"], next.action_data);
        pairs(act["t"], next.action_templates);
      }
    }
    if (extra["days"].is<JsonArray>()) for (JsonVariant day : extra["days"].as<JsonArray>()) {
      if (next.forecast.size() == 5) break;
      next.forecast.emplace_back(); auto &f = next.forecast.back();
      f.day = string(day["d"], 3); f.condition = string(day["c"], 20); f.high = number(day["h"]); f.low = number(day["l"]);
      f.rain = number(day["p"]); f.mm = number(day["r"]);
    }
    if (extra["hours"].is<JsonArray>()) for (JsonVariant hour : extra["hours"].as<JsonArray>()) {
      if (next.hours.size() == 8) break;
      next.hours.emplace_back(); auto &h = next.hours.back();
      h.time = string(hour["t"], 5); h.condition = string(hour["c"], 20); h.temp = number(hour["h"]); h.rain = number(hour["p"]); h.mm = number(hour["r"]);
    }
    tile.last_run = extra["last"].is<unsigned>() ? extra["last"].as<uint32_t>() : 0;
    next.sunrise = string(extra["rise"], 5); next.sunset = string(extra["set"], 5);
    next.timer_end = extra["end"].is<unsigned>() ? extra["end"].as<uint32_t>() : 0;
    next.duration = string(extra["dur"], 16); next.remaining = string(extra["rem"], 16);
    tile.has_history=false;
    if (root["history"]["values"].is<JsonArray>()) {
      tile.history.assign(24,NAN);unsigned j=0;
      for (JsonVariant value:root["history"]["values"].as<JsonArray>()) {
        if(j==24) break; tile.history[j++]=number(value); }
      tile.has_history=j>0;tile.history_hours=std::clamp(root["history"]["hours"].as<unsigned>(),1u,24u);
    } else if (!tile.history.empty()) { tile.history.clear(); tile.history.shrink_to_fit(); }
    if (a["options"].is<JsonArray>()) for(JsonVariant option:a["options"].as<JsonArray>()) {
      if(next.options.size()==8)break;next.options.push_back(string(option,48)); }
    tile.battery=number(a["battery_level"]);tile.volume=number(a["volume_level"]);
    tile.muted=a["is_volume_muted"].is<bool>() && a["is_volume_muted"].as<bool>();
    tile.device_class=string(a["device_class"],24);next.hvac_action=string(a["hvac_action"],24);
    next.media_title=string(a["media_title"],80);tile.supported=a["supported_features"].as<uint32_t>();
    // The media card (firmware 0.2.64+, app 0.2.77+): the artist, the album, the track's length and position, and a
    // mark of the cover picture; an app from before sends none of them and the card shows what it has.
    next.media_artist=string(extra["artist"],80);next.media_album=string(extra["album"],80);next.media_picture=string(extra["pic"],16);
    next.media_duration=extra["dur"].is<unsigned>()?extra["dur"].as<uint32_t>():0;
    next.media_position=extra["pos"].is<unsigned>()?extra["pos"].as<uint32_t>():0;
    next.media_position_at=extra["at"].is<unsigned>()?extra["at"].as<uint32_t>():0;
    tile.name = string(root["name"], 80);
    tile.state = string(root["state"], 160);
    tile.unit = string(a["unit_of_measurement"], 20);
    tile.brightness = number(a["brightness"]);
    tile.percentage = number(a["percentage"]);
    tile.slider_reported(esphome::millis());
    tile.position = number(a["current_position"]);
    next.tilt = number(a["current_tilt_position"]);
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
      tile.humidity=number(a["humidity"]);next.wind=number(a["wind_speed"]);next.wind_unit=string(a["wind_speed_unit"],8);next.feels=number(a["apparent_temperature"]);
    }
    tile.modes = list(a["supported_color_modes"]);
    next.hvac_modes = list(a["hvac_modes"]);
    next.fan_modes = list(a["fan_modes"]); next.swing_modes = list(a["swing_modes"]);
    next.fan_mode = string(a["fan_mode"], 48); next.swing_mode = string(a["swing_mode"], 48);
    float hue = number(a["hs_color"][0]);
    float saturation = number(a["hs_color"][1]);
    tile.has_hs_color = std::isfinite(hue) && std::isfinite(saturation);
    tile.saturation = tile.has_hs_color ? std::lround(std::clamp(saturation, 0.0f, 100.0f)) : 0;
    if (std::isfinite(hue)) tile.hue = std::lround(std::clamp(hue, 0.0f, 360.0f));
    float kelvin = number(a["color_temp_kelvin"]);
    if (std::isfinite(kelvin)) tile.kelvin = std::lround(std::clamp(kelvin, 1000.0f, 15000.0f));
    tile.min_kelvin = std::clamp(number(a["min_color_temp_kelvin"], 0), 0.0f, 15000.0f);
    tile.max_kelvin = std::clamp(number(a["max_color_temp_kelvin"], 0), 0.0f, 15000.0f);
    next.fan_speed = string(a["fan_speed"], 48);
    if (a["fan_speed_list"].is<JsonArray>()) for (JsonVariant speed : a["fan_speed_list"].as<JsonArray>()) {
      if (next.fan_speeds.size() == 4) break;
      next.fan_speeds.push_back(string(speed, 48));
    }
    // Vacuum rows (app 0.2.46+): the cleaning mode and water selects of the robot's device and the
    // suction speeds to offer, each at most six; the battery sensor when the vacuum has no attribute.
    // A chip just tapped keeps its choice while Home Assistant is still busy with it, so another update
    // of the robot (its battery, say) does not flip the row back for a moment.
    std::vector<std::pair<char, std::string>> tapped;
    if (tile.waiting(esphome::millis())) for (auto &c : tile.extra().choices) if (!c.sent.empty()) tapped.emplace_back(c.kind, c.sent);
    auto choice = [&](const char *key, char kind) {
      auto c = extra[key];
      if (!c["o"].is<JsonArray>()) return;
      Choice row; row.kind = kind;
      row.entity = string(c["e"], 120); row.current = string(c["s"], 48); row.roles = string(c["r"], 6);
      if (kind != 's' && !valid_entity(row.entity)) return;
      for (JsonVariant value : c["o"].as<JsonArray>()) { if (row.values.size() == 6) break; row.values.push_back(string(value, 48)); }
      if (c["l"].is<JsonArray>()) for (JsonVariant label : c["l"].as<JsonArray>()) {
        if (row.labels.size() == row.values.size()) break; row.labels.push_back(string(label, 24)); }
      while (row.labels.size() < row.values.size()) row.labels.push_back(row.values[row.labels.size()]);
      if (!row.values.empty()) next.choices.push_back(std::move(row));
    };
    if (tile.domain() == "vacuum") { choice("mode", 'm'); choice("water", 'w'); choice("fan", 's'); tile_controls::settle_suction(next); }
    for (auto &[kind, value] : tapped) if (auto *c = next.choice(kind)) if (c->current != value) c->sent = value;
    if (!std::isfinite(tile.battery)) tile.battery = number(extra["bat"]);
    next.charging = extra["chg"].is<int>() && extra["chg"].as<int>() == 1;
    next.room = string(extra["room"], 32);
    next.state_word = string(extra["w"], 32);
    tile.set_extra(std::move(next));
    // Home Assistant reports the edited value: the -/+ pill follows its state again.
    if(std::isfinite(tile.edit_value) && tile.edit_sent && std::fabs(tile_controls::edit_target(tile)-tile.edit_value)<0.051f)tile.edit_value=NAN;
    tile.received = true;
    for(auto &w:widgets)if(w.index==index)w.cached_active=-1;
    last_received = esphome::millis();
    if (repack && layout_changed) layout_changed();
    refresh_tile(index);
    if (active_index == static_cast<int>(index) && detail_update) detail_update(tile);
    refresh_detail(index);
    result = model.ready() ? "Synced" : "Loading tiles";
    return true;
  });
  return result;
}
// Between the manager's messages nothing redraws a card, so a command under way gets its own 200 ms LVGL timer: it
// brings the busy sheet up once the grace has passed, takes it away as soon as Home Assistant answered or the wait ran
// out, and deletes itself when no tile waits any more (firmware 0.2.59+).
inline lv_timer_t *busy_timer{};
inline void end_wait(size_t index) {
  auto &t = model.tiles[index];
  t.pending = false;
  t.undo_optimistic();
  if (auto *x = t.extra_ptr()) for (auto &c : x->choices) c.sent.clear();
}
inline void busy_watch(lv_timer_t *) {
  const uint32_t now = esphome::millis();
  bool any = false;
  for (auto &w : widgets) {
    if (!w.tile || w.index >= model.count) continue;
    auto &t = model.tiles[w.index];
    if (!t.pending) continue;
    if (!t.waiting(now) && !t.confirmed) end_wait(w.index);
    any = any || t.pending;
    if (t.loading(now) != w.busy_drawn) refresh_tile(w.index);
  }
  if (!any && busy_timer) { lv_timer_delete(busy_timer); busy_timer = nullptr; }
}
inline void watch_busy() { if (!busy_timer) busy_timer = lv_timer_create(busy_watch, 200, nullptr); }

#ifdef USE_API_HOMEASSISTANT_ACTION_RESPONSES
// Home Assistant answers an action sent with a call id (firmware 0.2.58+, Home Assistant 2025.10+). ESPHome keeps each
// answer's callback until the answer arrives and has no timeout of its own, so a tap asks for at most four answers at a
// time and ends the ones Home Assistant never gives (actions not allowed, an older Home Assistant) after eight seconds.
// Only a refusal shows: the tile says Refused for a moment instead of waiting.
struct WatchedCall { uint32_t id = 0, since = 0; };
inline std::array<WatchedCall, 4> watched_calls{};
inline const char *const NO_ANSWER = "no answer";
inline void watch_call(esphome::api::HomeassistantActionRequest &request, const std::string &entity) {
  static uint32_t last_id = 0x5C000000u;  // apart from ESPHome's own counter for YAML actions, which starts at 1
  auto slot = std::find_if(watched_calls.begin(), watched_calls.end(), [](const WatchedCall &c) { return c.id == 0; });
  if (slot == watched_calls.end()) return;
  const uint32_t id = ++last_id;
  *slot = {id, esphome::millis()};
  request.call_id = id;
  esphome::api::global_api_server->register_action_response_callback(id, [id, entity](const esphome::api::ActionResponse &answer) {
    for (auto &c : watched_calls) if (c.id == id) c = {};
    if (answer.is_success() || answer.get_error_message().c_str() == NO_ANSWER) {
      // "It worked" without a new state (a stop on a cover that already stands still) ends the wait in a moment
      // instead of running to the cap.
      if (answer.is_success())
        for (size_t i = 0; i < model.tiles.size(); ++i)
          if (model.tiles[i].entity == entity && model.tiles[i].pending && !model.tiles[i].answered_at)
            model.tiles[i].answered_at = std::max<uint32_t>(1, esphome::millis());
      return;
    }
    ESP_LOGW("runtime_action", "Home Assistant refused the action for %s: %.*s", entity.c_str(),
             (int) answer.get_error_message().size(), answer.get_error_message().c_str());
    for (size_t i = 0; i < model.tiles.size(); ++i) if (model.tiles[i].entity == entity) {
      model.tiles[i].pending = false;
      model.tiles[i].undo_optimistic();
      model.tiles[i].release_slider();
      model.tiles[i].refused_at = std::max<uint32_t>(1, esphome::millis());
      refresh_tile(i);
    }
  });
}
inline void expire_calls(uint32_t now) {
  for (auto &c : watched_calls) {
    if (!c.id || now - c.since < 8000) continue;
    const uint32_t id = c.id;
    c = {};
    esphome::api::global_api_server->handle_action_response(id, false, esphome::StringRef(NO_ANSWER, 9));
  }
}
#endif
// Marks every tile of the entity busy and sends; `watch` asks Home Assistant for an answer (a tap).
inline void send_action(esphome::api::HomeassistantActionRequest &request, const std::string &entity, bool watch) {
  for(size_t i=0;i<model.tiles.size();++i) if(model.tiles[i].entity==entity){model.tiles[i].begin(esphome::millis());model.tiles[i].refused_at=0;refresh_tile(i);}
  watch_busy();
#ifdef USE_API_HOMEASSISTANT_ACTION_RESPONSES
  if (watch) watch_call(request, entity);
#endif
  esphome::api::global_api_server->send_homeassistant_action(request);
}
inline void action(const std::string &service, const std::string &entity, const std::string &key="", const std::string &value="", bool watch=true) {
  if (!fresh() || !valid_entity(entity)) return;
  esphome::api::HomeassistantActionRequest request;
  request.service = esphome::StringRef(service);
  request.data.init(key.empty() ? 1 : 2);
  esphome::api::HomeassistantServiceMap entry;
  entry.key = esphome::StringRef("entity_id");
  entry.value = esphome::StringRef(entity);
  request.data.push_back(entry);
  if(!key.empty()) {esphome::api::HomeassistantServiceMap param;param.key=esphome::StringRef(key);param.value=esphome::StringRef(value);request.data.push_back(param);}
  send_action(request, entity, watch);
  ESP_LOGI("runtime_action","Sent service=%s entity=%s",service.c_str(),entity.c_str());
}
// A tap's own action (firmware 0.2.58+): the tile's entity with the data the app sent, text as data and the values Home
// Assistant renders itself (numbers, lists, true or false) as a data_template.
inline void perform(const Tile &tile) {
  const auto &x = tile.extra();
  if (!fresh() || !valid_entity(tile.entity) || x.action.empty()) return;
  esphome::api::HomeassistantActionRequest request;
  request.service = esphome::StringRef(x.action);
  request.data.init(1 + x.action_data.size());
  esphome::api::HomeassistantServiceMap target;
  target.key = esphome::StringRef("entity_id");
  target.value = esphome::StringRef(tile.entity);
  request.data.push_back(target);
  for (const auto &pair : x.action_data) {
    esphome::api::HomeassistantServiceMap entry;
    entry.key = esphome::StringRef(pair.first);
    entry.value = esphome::StringRef(pair.second);
    request.data.push_back(entry);
  }
  request.data_template.init(x.action_templates.size());
  for (const auto &pair : x.action_templates) {
    esphome::api::HomeassistantServiceMap entry;
    entry.key = esphome::StringRef(pair.first);
    entry.value = esphome::StringRef(pair.second);
    request.data_template.push_back(entry);
  }
  send_action(request, tile.entity, true);
  ESP_LOGI("runtime_action", "Sent service=%s entity=%s (%u values)", x.action.c_str(), tile.entity.c_str(),
           (unsigned) (x.action_data.size() + x.action_templates.size()));
}
// A detail card asks the manager for its history (app 0.2.59+ answers with op "history"). An event, like
// setting_event: it needs no permission to call Home Assistant actions.
inline void history_request(const std::string &entity, uint32_t hours) {
  if (inbox.empty()) return;
  esphome::api::HomeassistantActionRequest request;
  request.service = esphome::StringRef("esphome.screen_history");
  request.is_event = true;
  const std::string span = std::to_string(hours);
  const std::string keys[] = {"inbox", "entity", "hours"}, values[] = {inbox, entity, span};
  request.data.init(3);
  for (int i = 0; i < 3; ++i) {
    esphome::api::HomeassistantServiceMap entry;
    entry.key = esphome::StringRef(keys[i]);
    entry.value = esphome::StringRef(values[i]);
    request.data.push_back(entry);
  }
  esphome::api::global_api_server->send_homeassistant_action(request);
  history_asked_at = esphome::millis();
  history_answered = false;
  history_asked_entity = entity;
  history_asked_hours = hours;
  ESP_LOGI("history", "asked for %u h of %s", static_cast<unsigned>(hours), entity.c_str());
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
inline lv_obj_t *detail_actions[32]{};
inline unsigned detail_action_count=0;
inline lv_obj_t *detail_status=nullptr;
inline lv_obj_t *detail_badge_status=nullptr;
inline lv_obj_t *detail_switch=nullptr;
inline lv_obj_t *detail_label(lv_obj_t *parent,const std::string &text,int x,int y,int width) {
  auto *label=lv_label_create(parent);lv_label_set_text(label,text.c_str());lv_obj_set_pos(label,x,y);lv_obj_set_width(label,width);
  lv_obj_set_style_text_font(label,detail_font,0);lv_obj_set_style_text_color(label,theme::color(theme::INK),0);
  lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);lv_obj_set_height(label,lv_font_get_line_height(detail_font));return label;
}
inline std::string detail_state(const Tile &t){
  if(t.domain()=="person")return t.state=="home"?"Home":t.state=="not_home"?"Away":t.state;
  if(t.domain()=="sun")return t.state=="above_horizon"?"Above the horizon":"Below the horizon";
  if(t.domain()=="timer")return timer_text(t);
  if(t.domain()=="script"||t.domain()=="scene"||t.domain()=="button"||t.domain()=="input_button")return t.state=="on"?"Running...":last_run_text(t.last_run);
  if(t.domain()=="binary_sensor"&&(t.state=="on"||t.state=="off"))return tile_controls::binary_state_text(t.device_class,t.state=="on");
  if(t.state=="on")return "On";
  if(t.state=="off")return "Off";
  if(t.state=="docked")return "Docked";
  if(t.state=="cleaning")return "Cleaning";
  if(t.state=="paused")return "Paused";
  if(t.state=="returning")return "Returning to dock";
  if(t.state=="idle")return "Idle";
  if(t.state=="error")return "Check the robot in HA";
  if(!t.available())return "Unavailable";
  // Home Assistant's word where the screen has none of its own (firmware 0.2.58+).
  if(!t.extra().state_word.empty())return t.extra().state_word;
  return t.state;
}
inline void hide_detail(){if(detail_root)lv_obj_add_flag(detail_root,LV_OBJ_FLAG_HIDDEN);}
inline int slider_value(const Tile &t){
  auto d=t.domain();float value=0;
  if(d=="light")value=std::isfinite(t.brightness)?t.brightness/255:0;
  if(d=="fan")value=std::isfinite(t.percentage)?t.percentage/100:0;
  // A blind's bar is the blind itself: the closed part is filled, so a closed cover is a full bar, as on its card and in
  // Home Assistant's own cover dialog (firmware 0.2.66+; before, the bar filled with the open part, the other way round).
  if(d=="cover")value=std::isfinite(t.position)?(100-t.position)/100:1;
  if(d=="media_player")value=std::isfinite(t.volume)?t.volume:0;
  if(d=="number"||d=="input_number") {char *end;float state=strtof(t.state.c_str(),&end);if(end!=t.state.c_str() && t.maximum>t.minimum)value=(state-t.minimum)/(t.maximum-t.minimum);}
  return std::clamp((int)std::lround(value*1000),0,1000);
}
inline lv_obj_t *captured_slider=nullptr;
inline bool slider_changed=false;
inline int slider_held=0;
inline void slider_event(lv_event_t *e);
inline void commit_slider(unsigned i,int raw){
  if(i>=model.count || !fresh())return;auto &t=model.tiles[i];if(!t.available() || t.waiting(esphome::millis()))return;
  float value=std::clamp(raw,0,1000)/1000.0f;auto d=t.domain();
  // A light's slider stops at 1 %, as in Home Assistant; tapping the card turns it off.
  // The slider stays where the finger left it while the light fades towards it (Tile::hold_slider).
  if(d=="light"){int sent=std::max(3,(int)std::lround(value*255));action("light.turn_on",t.entity,"brightness",std::to_string(sent));t.hold_slider(esphome::millis(),sent);}
  if(d=="fan"){int sent=(int)std::lround(value*100);action("fan.set_percentage",t.entity,"percentage",std::to_string(sent));t.hold_slider(esphome::millis(),sent);}
  if(d=="cover")action("cover.set_cover_position",t.entity,"position",std::to_string(100-(int)std::lround(value*100)));
  if(d=="media_player"){action("media_player.volume_set",t.entity,"volume_level",std::to_string(value));t.hold_slider(esphome::millis(),value);}
  if(d=="number"||d=="input_number") {
    if(!std::isfinite(t.minimum)||!std::isfinite(t.maximum)||t.maximum<=t.minimum||t.step<=0)return;
    value=std::clamp(t.minimum+std::round(value*(t.maximum-t.minimum)/t.step)*t.step,t.minimum,t.maximum);
    action(d+".set_value",t.entity,"value",std::to_string(value)); }
}
// Where the finger landed on a slider, the value it had, and whether it landed beside the strip (in the tile's lower
// half that the strip claims, slider_zone) and moved: a press beside the strip that never moves is a tap on the tile.
inline lv_point_t slider_press{0,0};inline int slider_press_value=0;inline bool slider_press_beside=false,slider_press_moved=false,slider_press_held=false;
// The card a strip belongs to, for a press beside the strip that turns out to be the card's tap or hold.
inline lv_obj_t *strip_card(lv_obj_t *slider){for(auto &w:widgets)if(w.slider==slider)return w.tile;return nullptr;}
inline void slider_event(lv_event_t *e){
  auto *slider=lv_event_get_target_obj(e);auto code=lv_event_get_code(e);
  if(code==LV_EVENT_PRESSED){captured_slider=slider;slider_changed=false;slider_press_moved=false;slider_press_beside=false;slider_press_held=false;
    slider_press_value=lv_slider_get_value(slider);
    if(auto *indev=lv_indev_active()){lv_indev_get_point(indev,&slider_press);lv_area_t a;lv_obj_get_coords(slider,&a);slider_press_beside=slider_press.x<a.x1 || slider_press.x>a.x2 || slider_press.y<a.y1 || slider_press.y>a.y2;}
    // Beside the strip the card lights up as under a tap, until the finger starts dragging.
    if(slider_press_beside)if(auto *card=strip_card(slider))lv_obj_add_state(card,LV_STATE_PRESSED);}
  if(code==LV_EVENT_PRESSING && captured_slider==slider && !slider_press_moved){
    if(auto *indev=lv_indev_active()){lv_point_t p;lv_indev_get_point(indev,&p);if(std::abs(p.x-slider_press.x)>=10 || std::abs(p.y-slider_press.y)>=10){slider_press_moved=true;
      if(auto *card=strip_card(slider))lv_obj_remove_state(card,LV_STATE_PRESSED);}}}
  // A finger resting beside the strip holds the card, as it would without the strip's claim on the card.
  if(code==LV_EVENT_LONG_PRESSED && captured_slider==slider && slider_press_beside && !slider_press_moved && !slider_press_held){
    if(auto *card=strip_card(slider)){slider_press_held=true;lv_obj_send_event(card,LV_EVENT_LONG_PRESSED,nullptr);}}
  if((code==LV_EVENT_RELEASED || code==LV_EVENT_PRESS_LOST) && slider_press_beside)if(auto *card=strip_card(slider))lv_obj_remove_state(card,LV_STATE_PRESSED);
  // On release LVGL sets the value once more from the last touch point: log it when that throws a
  // dragged slider to one of its ends, so a stray touch sample shows up (cyd::release_jump).
  if(code==LV_EVENT_VALUE_CHANGED && captured_slider==slider){
    auto *indev=lv_indev_active();int raw=lv_slider_get_value(slider);
    if(!indev || lv_indev_get_state(indev)!=LV_INDEV_STATE_RELEASED)slider_held=raw;
    else if(slider_changed && cyd::release_jump(slider_held,raw,lv_slider_get_min_value(slider),lv_slider_get_max_value(slider))){
      lv_point_t point;lv_indev_get_point(indev,&point);
      ESP_LOGW("slider","Tile slider jumped on release: %d -> %d (touch x=%d y=%d)",slider_held,raw,(int)point.x,(int)point.y);
    }
  }
  // The range reaches below zero only to draw the round end at 0 (slider_handle).
  if(code==LV_EVENT_VALUE_CHANGED && lv_slider_get_value(slider)<0)lv_slider_set_value(slider,0,LV_ANIM_OFF);
  if(code==LV_EVENT_VALUE_CHANGED && captured_slider==slider)slider_changed=true;
  if(code==LV_EVENT_PRESS_LOST && captured_slider==slider){captured_slider=nullptr;slider_changed=false;}
  if(code==LV_EVENT_RELEASED && captured_slider==slider){
    unsigned index=(uintptr_t)lv_event_get_user_data(e);
    // A slider among a card's own parts (the media tile's volume) belongs to the tile the slot shows now.
    for(auto &w:widgets)if(w.slider==slider || w.control_slider==slider || (w.extra && lv_obj_get_parent(slider)==w.extra)){index=w.index;break;}
    bool changed=slider_changed;captured_slider=nullptr;slider_changed=false;
    // A finger that landed beside a tile's strip and let go without moving tapped the tile (or held it, sent above):
    // the value LVGL set from that point on release goes back, and the tile's own rules decide what the tap does.
    if(slider_press_beside && !slider_press_moved){
      if(auto *card=strip_card(slider)){
        lv_slider_set_value(slider,slider_press_value,LV_ANIM_OFF);
        ESP_LOGI("slider","%s beside the strip of %s: the tile's",slider_press_held?"Hold":"Tap",model.tiles[index].entity.c_str());
        if(!slider_press_held)lv_obj_send_event(card,LV_EVENT_SHORT_CLICKED,nullptr);
        return;}
    }
    // A finger let go within the edge band of the glass meant the slider's end (cyd::edge_snap).
    if(auto *indev=lv_indev_active();indev && lv_obj_get_width(slider)>=lv_obj_get_height(slider)){
      lv_point_t p;lv_indev_get_point(indev,&p);lv_area_t a;lv_obj_get_coords(slider,&a);
      int screen=lv_display_get_horizontal_resolution(lv_obj_get_display(slider));
      int snap=cyd::edge_snap(p.x,a.x1,a.x2,screen,cyd::edge_snap_band);
      if(snap){int end=snap>0?(int)lv_slider_get_max_value(slider):std::max(0,(int)lv_slider_get_min_value(slider));
        ESP_LOGI("slider","Let go %d px from the %s edge: slider %d -> %d",snap>0?screen-1-(int)p.x:(int)p.x,snap>0?"right":"left",lv_slider_get_value(slider),end);
        lv_slider_set_value(slider,end,LV_ANIM_OFF);changed=true;}
    }
    if(changed && cyd::touch_guard.accept_slider(esphome::millis(),200+index))commit_slider(index,lv_slider_get_value(slider));
  }
}
inline void show_detail(unsigned index);
// A chip on the vacuum card: the service call goes out, the chip shows the choice at once, and the card
// waits for Home Assistant like after its other buttons. The card is drawn again once this tap's event
// has finished, because a chosen mode can add or remove the suction and water rows.
inline void choose(Tile &t,Choice &row,const std::string &value){
  if(value==row.current)return;
  auto a=tile_controls::choice_action(t,row.kind,value);if(!a.valid())return;
  action(a.service,row.kind=='s'?t.entity:row.entity,a.key,a.value);
  row.sent=value;t.begin(esphome::millis());
  lv_async_call([](void *){if(detail_root && !lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN))show_detail(detail_index);},nullptr);
}
// What a key on a card does; `cmd` is the key's command. Shared by the plain keys (detail_button) and the round
// keys of the media card. -1 is Back.
inline void detail_command(int cmd){
  if(cmd==-1){hide_detail();return;}
  // History ranges (firmware 0.2.51+): redraw after this event, which belongs to a key the redraw deletes.
  if(cmd>=160&&cmd<163){
    static const uint32_t hours[]={1,24,168};
    if(detail_index<model.count&&allowed(esphome::millis(),300+cmd,"history range")&&history_hours!=hours[cmd-160]){
      history_hours=hours[cmd-160];
      lv_async_call([](void *){if(detail_root&&!lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN))show_detail(detail_index);},nullptr);
    }
    return;
  }
  if(!fresh()||detail_index>=model.count || !allowed(esphome::millis(),300+cmd,"card button "+model.tiles[detail_index].entity))return;
  auto &t=model.tiles[detail_index];if(!t.available()||t.waiting(esphome::millis()))return;
  if(cmd<4){const char *services[]={"vacuum.start","vacuum.pause","vacuum.return_to_base","vacuum.locate"};action(services[cmd],t.entity);}
  // Vacuum rows: 10-15 suction, 50-55 cleaning mode, 60-65 water.
  static const std::pair<int,char> rows[]={{10,'s'},{50,'m'},{60,'w'}};
  for(const auto &[first,kind]:rows)
    if(cmd>=first && cmd<first+6){auto *row=t.choice(kind);if(row && cmd-first<(int)row->values.size())choose(t,*row,row->values[cmd-first]);}
  // The media card's keys (20-23); its volume slider goes through commit_slider.
  if(cmd>=20 && cmd<=24)media_action(t,cmd);
  if(cmd>=30 && cmd<38 && cmd-30<(int)t.extra().options.size())action(t.domain()+".select_option",t.entity,"option",t.extra().options[cmd-30]);
  // Cover keys: 70 + tile_controls::Command (open, stop, close and the tilt keys).
  if(cmd>=70 && cmd<130){auto a=tile_controls::key_action(t,cmd-70);if(a.valid()&&t.domain()=="cover")action(a.service,t.entity,a.key,a.value);}
  if(cmd==40)action(t.state=="active"?"timer.pause":"timer.start",t.entity);
  if(cmd==41)action("timer.cancel",t.entity);
}
inline lv_obj_t *detail_button(const char *text,int x,int y,int width,int height,int command){
  auto *button=lv_obj_create(detail_root);lv_obj_remove_style_all(button);lv_obj_set_pos(button,x,y);lv_obj_set_size(button,width,height);
  lv_obj_set_style_bg_color(button,theme::color(command==0?theme::ACCENT:theme::BUTTON),0);lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);lv_obj_set_style_radius(button,12,0);lv_obj_add_flag(button,LV_OBJ_FLAG_CLICKABLE);
  auto *label=detail_label(button,text,6,0,width-12);lv_obj_center(label);lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_CENTER,0);if(command==0)lv_obj_set_style_text_color(label,theme::color(theme::ON_ACCENT),0);lv_obj_remove_flag(label,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(button,[](lv_event_t *e){detail_command((intptr_t)lv_event_get_user_data(e));},LV_EVENT_SHORT_CLICKED,(void*)(intptr_t)command);
  lv_obj_set_style_bg_color(button,theme::color(theme::ACCENT_PRESSED),LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(button,-2,LV_STATE_PRESSED);lv_obj_set_style_transform_height(button,-2,LV_STATE_PRESSED);
  lv_obj_set_style_opa(button,LV_OPA_50,LV_STATE_DISABLED);
  if(command>=0 && detail_action_count<32)detail_actions[detail_action_count++]=button;
  return button;
}

// ---- Weather card: now, the next hours and the coming days, with rain ----
// The colour of a condition's icon, as this look draws it on a card.
inline uint32_t weather_accent(const std::string &c) {
  using namespace theme::ha;
  if(c=="sunny")return theme::foreground(SUNNY);
  if(c=="clear-night")return theme::foreground(DEEP_PURPLE);
  if(c=="rainy"||c=="pouring"||c=="lightning-rainy"||c=="snowy-rainy")return theme::foreground(RAIN);
  if(c=="snowy"||c=="hail")return theme::foreground(SNOW);
  if(c=="lightning")return theme::foreground(LIGHTNING);
  if(c=="partlycloudy")return theme::foreground(PARTLY_CLOUDY);
  return theme::foreground(CLOUDY);
}
inline lv_obj_t *detail_text(lv_obj_t *parent,const std::string &text,int x,int y,int width,const lv_font_t *font,lv_text_align_t align,uint32_t color){
  auto *l=detail_label(parent,text,x,y,std::max(1,width));lv_obj_set_style_text_font(l,font,0);lv_obj_set_height(l,lv_font_get_line_height(font));
  lv_obj_set_style_text_align(l,align,0);lv_obj_set_style_text_color(l,lv_color_hex(color),0);return l;
}
inline lv_obj_t *detail_text(lv_obj_t *parent,const std::string &text,int x,int y,int width,const lv_font_t *font,lv_text_align_t align,theme::Role role){
  return detail_text(parent,text,x,y,width,font,align,theme::hex(role));
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
  lv_obj_set_style_bg_color(card,theme::color(theme::CARD),0);lv_obj_set_style_bg_opa(card,LV_OPA_COVER,0);
  lv_obj_set_style_radius(card,lv_obj_get_style_radius(widgets[0].tile,LV_PART_MAIN),0);
  lv_obj_set_style_border_width(card,1,0);lv_obj_set_style_border_color(card,theme::color(theme::LINE),0);
  return card;
}
// Two cards: "now" with the next hours, and the coming days. Bold highs, muted lows,
// rain in blue with a drop, so the eye finds temperature first and rain second.
inline void render_weather_detail(const Tile &t,bool large,int width,int height,int pad){
  const Extra &weather=t.extra();
  if(detail_status){lv_obj_add_flag(detail_status,LV_OBJ_FLAG_HIDDEN);detail_status=nullptr;}
  const lv_font_t *big=watch_value_font?watch_value_font:detail_font;
  const lv_font_t *icon_font=widgets[0].icon_font?widgets[0].icon_font:detail_font;
  const lv_font_t *mini=mini_icon_font?mini_icon_font:icon_font;
  const lv_font_t *tiny=watch_icon_font?watch_icon_font:mini;
  const lv_font_t *small=widgets[0].value?lv_obj_get_style_text_font(widgets[0].value,LV_PART_MAIN):detail_font;
  const uint32_t ink=theme::hex(theme::INK),muted=theme::hex(theme::SUBTLE),rain=theme::foreground(theme::ha::RAIN);
  int text_h=lv_font_get_line_height(detail_font),small_h=lv_font_get_line_height(small),mini_h=lv_font_get_line_height(mini),tiny_h=lv_font_get_line_height(tiny);
  int icon_h=lv_font_get_line_height(icon_font),big_h=lv_font_get_line_height(big),hero=std::max(icon_h,big_h);
  int card_pad=large?14:7,inner=width-2*pad-2*card_pad;
  unsigned columns=std::min<unsigned>(weather.hours.size(),6);
  int hours_h=columns?small_h+mini_h+text_h+small_h+(large?16:6):0;
  int card_a_h=card_pad+hero+(columns?(large?14:8)+hours_h:0)+card_pad;
  // The Guition starts below the round back button of the top bar (60 px at 16); the CYD needs every pixel for the
  // coming days and starts where it did.
  int y=large?84:38;
  auto *now=detail_card(pad,y,width-2*pad,card_a_h);
  // Now: icon, temperature, condition, then feels-like / humidity / wind in one muted line.
  int cy=card_pad;char b[48];
  detail_text(now,t.available()?weather_icon(t.state):"\U000F0595",card_pad,cy+(hero-icon_h)/2,icon_h+8,icon_font,LV_TEXT_ALIGN_LEFT,weather_accent(t.state));
  int temp_x=card_pad+icon_h+(large?14:6),temp_w=large?92:50;
  detail_text(now,degrees(t.current),temp_x,cy+(hero-big_h)/2,temp_w,big,LV_TEXT_ALIGN_LEFT,ink);
  int text_x=temp_x+temp_w+(large?4:2),text_w=width-2*pad-card_pad-text_x;
  int lines_h=text_h+small_h+(large?2:0);
  detail_text(now,t.available()?weather_text(t.state):"Unavailable",text_x,cy+(hero-lines_h)/2,text_w,detail_font,LV_TEXT_ALIGN_LEFT,ink);
  std::string details;
  if(std::isfinite(weather.feels)){snprintf(b,sizeof(b),"Feels like %.0f°",weather.feels);details=b;}
  if(std::isfinite(t.humidity)){snprintf(b,sizeof(b),"%d%%",(int)std::lround(t.humidity));details+=(details.empty()?"":" · ")+std::string(b);}
  if(std::isfinite(weather.wind)){snprintf(b,sizeof(b),"%.0f %s",weather.wind,weather.wind_unit.empty()?"km/h":weather.wind_unit.c_str());details+=(details.empty()?"":" · ")+std::string(b);}
  detail_text(now,details,text_x,cy+(hero-lines_h)/2+text_h+(large?2:0),text_w,small,LV_TEXT_ALIGN_LEFT,muted);
  // Next hours inside the same card: time, icon, temperature, rain per column.
  if(columns){
    int hy=cy+hero+(large?14:8),col=inner/(int)columns;
    for(unsigned i=0;i<columns;++i){
      const auto &h=weather.hours[i];int x=card_pad+i*col;
      detail_text(now,h.time,x,hy,col,small,LV_TEXT_ALIGN_CENTER,muted);
      detail_text(now,weather_icon(h.condition),x,hy+small_h+(large?4:1),col,mini,LV_TEXT_ALIGN_CENTER,weather_accent(h.condition));
      detail_text(now,std::isfinite(h.temp)?degrees(h.temp):"",x,hy+small_h+mini_h+(large?8:2),col,detail_font,LV_TEXT_ALIGN_CENTER,ink);
      detail_text(now,rain_text(h.rain,h.mm,false),x,hy+small_h+mini_h+text_h+(large?8:3),col,small,LV_TEXT_ALIGN_CENTER,rain);
    }
  }
  y+=card_a_h+(large?12:6);
  // Coming days: a heading and a card with one row per day.
  if(!weather.forecast.size()){detail_text(detail_root,"No daily forecast from Home Assistant",pad,y,width-2*pad,small,LV_TEXT_ALIGN_LEFT,muted);return;}
  if(large){detail_text(detail_root,"Coming days",pad+4,y,width-2*pad,detail_font,LV_TEXT_ALIGN_LEFT,muted);y+=text_h+8;}
  int card_b_h=height-y-(large?10:4);
  auto *days=detail_card(pad,y,width-2*pad,card_b_h);
  int row_pad=large?8:4,row=(card_b_h-2*row_pad)/(int)weather.forecast.size();
  int day_w=large?46:26,icon_x=card_pad+day_w,cond_x=icon_x+mini_h+(large?12:5);
  int high_w=large?52:30,low_w=large?46:28,rain_w=large?120:60,drop_w=tiny_h+(large?4:2);
  int temps_x=width-2*pad-card_pad-high_w-low_w,rain_x=temps_x-(large?14:6)-rain_w;
  for(unsigned i=0;i<weather.forecast.size();++i){
    const auto &f=weather.forecast[i];int ry=row_pad+i*row,tcy=ry+(row-text_h)/2,scy=ry+(row-small_h)/2;
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
// ---- Vacuum card ----
// A robot with its state and battery, the two commands used most, and one block that says how it cleans:
// the cleaning mode, then suction and water. Only the rows the chosen mode uses are shown, and they sit
// below the mode, so a tap never moves the control under the finger. Native shapes and labels only.
// Native shapes keep the robot crisp without image buffers or extra layers.
inline lv_obj_t *detail_shape(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius){
  auto *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
  lv_obj_set_style_radius(o,radius,0);lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
  lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
inline lv_obj_t *detail_shape(lv_obj_t *parent,int x,int y,int w,int h,theme::Role role,int radius){
  return detail_shape(parent,x,y,w,h,theme::hex(role),radius);
}
inline int text_width(const std::string &text,const lv_font_t *font){
  lv_point_t size;lv_text_get_size(&size,text.c_str(),font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);return size.x;
}
// The state colour, as Home Assistant colours a vacuum: teal while cleaning, blue on the way back, amber
// when paused, red on an error; a docked or idle robot keeps the card's own blue. `tint` is the halo.
struct VacuumLook { uint32_t accent, tint; };
inline VacuumLook vacuum_look(const std::string &state){
  using namespace theme::ha;
  const uint32_t accent=state=="cleaning"?TEAL:state=="returning"?BLUE:state=="paused"?ORANGE:state=="error"?RED:SKY;
  // A robot at rest has the quietest halo.
  return {accent,theme::tint(accent,accent==SKY?22:36)};
}
// A robot seen from above on a halo: a white body with a rim, the laser turret, a light bar in the state colour.
inline void vacuum_robot(lv_obj_t *parent,int x,int y,int size,const VacuumLook &look){
  auto *halo=detail_shape(parent,x,y,size,size,look.tint,size/2);
  int r=size*72/100,rim=std::max(1,size/44);
  auto *body=detail_shape(halo,(size-r)/2,(size-r)/2,r,r,theme::ROBOT_BODY,r/2);
  lv_obj_set_style_border_width(body,rim,0);lv_obj_set_style_border_color(body,theme::color(theme::ROBOT_RIM),0);
  int c=r-2*rim;auto s=[&](int v){return std::max(1,v*c/100);};
  detail_shape(body,s(32),s(10),s(36),s(36),theme::ROBOT_TOP,s(18));
  detail_shape(body,s(41),s(19),s(18),s(18),theme::ROBOT_LENS,s(9));
  detail_shape(body,s(31),s(70),s(38),std::max(3,s(7)),look.accent,s(4));
}
// A battery like a phone's: outline, level fill (red below 20 %) and a small cap.
inline void vacuum_battery(lv_obj_t *parent,int x,int y,int w,int h,float level){
  int line=std::max(1,h/7),fill_w=w-4*line,fill=std::clamp((int)std::lround(fill_w*level/100.0f),0,fill_w);
  auto *shell=detail_shape(parent,x,y,w,h,theme::CARD,std::max(2,h/4));
  lv_obj_set_style_bg_opa(shell,LV_OPA_TRANSP,0);
  lv_obj_set_style_border_width(shell,line,0);lv_obj_set_style_border_color(shell,theme::color(theme::BATTERY),0);
  if(fill)detail_shape(shell,line,line,fill,h-4*line,level<20?theme::foreground(theme::ha::ALARM):theme::hex(theme::SLATE),std::max(1,h/9));
  detail_shape(parent,x+w,y+h*3/10,std::max(2,line+1),h-2*(h*3/10),theme::BATTERY,1);
}
// Battery meter, level and a green bolt while charging, in one row from `x`; returns the row's width.
// Without a parent it only measures.
inline int vacuum_power(lv_obj_t *parent,const Tile &t,int x,int y,const lv_font_t *font,bool large){
  const lv_font_t *bolt_font=large && watch_icon_font?watch_icon_font:mini_icon_font;
  std::string percent=std::to_string((int)std::lround(t.battery))+"%";
  int h=lv_font_get_line_height(font),meter_w=large?30:22,meter_h=large?15:11,gap=large?10:6,words=text_width(percent,font);
  int bolt_w=t.extra().charging && bolt_font?text_width("\U000F0241",bolt_font):0;
  int width=meter_w+3+gap+words+(bolt_w?gap/2+bolt_w:0);
  if(!parent)return width;
  vacuum_battery(parent,x,y+(h-meter_h)/2,meter_w,meter_h,t.battery);
  int px=x+meter_w+3+gap;
  detail_text(parent,percent,px,y,words+2,font,LV_TEXT_ALIGN_LEFT,theme::MUTED);
  if(bolt_w)detail_text(parent,"\U000F0241",px+words+gap/2,y+(h-lv_font_get_line_height(bolt_font))/2,bolt_w+2,bolt_font,LV_TEXT_ALIGN_LEFT,theme::foreground(theme::ha::CHARGING));
  return width;
}
// Put a detail_button's words in `font`, sized to the words and centred (or at `x` when given).
inline lv_obj_t *button_words(lv_obj_t *button,const lv_font_t *font,uint32_t color,int available,int x=-1){
  auto *label=lv_obj_get_child(button,0);
  lv_obj_set_style_text_font(label,font,0);lv_obj_set_style_text_color(label,lv_color_hex(color),0);
  lv_obj_set_size(label,std::min(available,text_width(lv_label_get_text(label),font)+2),lv_font_get_line_height(font));
  if(x<0)lv_obj_center(label);else lv_obj_align(label,LV_ALIGN_LEFT_MID,x,0);
  return label;
}
// A command button with its icon before the words, the two centred as one group.
inline lv_obj_t *vacuum_command(const char *icon,const char *text,int x,int y,int w,int h,int command,bool primary,
                                const lv_font_t *font,const lv_font_t *icon_font,int radius){
  auto *button=detail_button(text,x,y,w,h,command);
  const uint32_t ink=theme::hex(primary?theme::ON_ACCENT:theme::INK);
  lv_obj_set_style_radius(button,radius,0);
  lv_obj_set_style_bg_color(button,theme::color(primary?theme::ACCENT:theme::CARD),0);
  if(!primary){
    lv_obj_set_style_border_width(button,1,0);lv_obj_set_style_border_color(button,theme::color(theme::LINE),0);
    lv_obj_set_style_bg_color(button,theme::color(theme::KEY),LV_STATE_PRESSED);
  }
  int gap=std::max(6,h/7),icon_w=text_width(icon,icon_font),words=text_width(text,font);
  int group=icon_w+gap+words,left=std::max(4,(w-group)/2);
  auto *glyph=lv_label_create(button);lv_label_set_text(glyph,icon);lv_obj_remove_flag(glyph,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(glyph,icon_font,0);lv_obj_set_style_text_color(glyph,lv_color_hex(ink),0);
  lv_obj_align(glyph,LV_ALIGN_LEFT_MID,left,0);
  button_words(button,font,ink,w-left-icon_w-gap-4,left+icon_w+gap);
  return button;
}
// A segmented control: one rounded track, each option as wide as its words plus an equal share of the
// room left, the chosen one filled in blue. Commands first..first+5.
inline void vacuum_segments(const Tile &t,const Choice &row,int first,int x,int y,int w,int h,uint32_t track,bool border,const lv_font_t *font){
  int n=std::min<int>((int)row.values.size(),6);if(!n)return;
  auto *bar=detail_shape(detail_root,x,y,w,h,track,h/2);
  if(border){lv_obj_set_style_border_width(bar,1,0);lv_obj_set_style_border_color(bar,theme::color(theme::LINE),0);}
  int inset=std::max(3,h/12),room=w-2*inset,words=0;int widths[6];
  for(int i=0;i<n;++i){widths[i]=text_width(row.labels[i],font);words+=widths[i];}
  int share=(room-words)/n;
  const std::string &chosen=tile_controls::shown_value(t,row,esphome::millis());
  int sx=x+inset;
  for(int i=0;i<n;++i){
    int sw=i==n-1?x+w-inset-sx:(words<=room?widths[i]+share:room/n);
    bool selected=row.values[i]==chosen;
    auto *segment=detail_button(row.labels[i].c_str(),sx,y+inset,sw,h-2*inset,first+i);
    lv_obj_set_style_radius(segment,(h-2*inset)/2,0);
    lv_obj_set_style_bg_color(segment,theme::color(theme::ACCENT),0);
    lv_obj_set_style_bg_opa(segment,selected?LV_OPA_COVER:LV_OPA_TRANSP,0);
    lv_obj_set_style_bg_opa(segment,LV_OPA_COVER,LV_STATE_PRESSED);
    if(!selected)lv_obj_set_style_bg_color(segment,theme::color(theme::ACCENT_TINT),LV_STATE_PRESSED);
    button_words(segment,font,theme::hex(selected?theme::ON_ACCENT:theme::INK),sw-6);
    sx+=sw;
  }
}
inline void render_vacuum_detail(Tile &t,bool large,int width,int height,int pad){
  uint32_t now=esphome::millis();
  auto rows=tile_controls::vacuum_rows(t,now);
  const Choice *mode=t.choice('m'),*suction=rows.suction?t.choice('s'):nullptr,*water=rows.water?t.choice('w'):nullptr;
  bool automatic=mode && tile_controls::vacuum_role(t,now)=='a';
  const lv_font_t *text=large?detail_font:(control_font?control_font:detail_font);
  const lv_font_t *big=watch_font?watch_font:detail_font;
  const lv_font_t *icons=mini_icon_font?mini_icon_font:detail_font;
  VacuumLook look=vacuum_look(t.state);
  bool cleaning=t.state=="cleaning",paused=t.state=="paused";
  bool battery=std::isfinite(t.battery),on_the_way=cleaning||paused||t.state=="returning";
  int inner=width-2*pad,gap=large?12:6,radius=lv_obj_get_style_radius(widgets[0].tile,LV_PART_MAIN);
  std::string state=t.loading(now)?"Command sent...":detail_state(t);
  // A robot with little to set gets a hero on the small screen too; one with mode and water rows uses a status row.
  bool small_hero=!large && !mode && !t.choice('w');
  int y=large?92:52;
  if(large || small_hero){
    int hero_h=large?108:60,robot=large?84:48,edge=large?12:6;
    auto *hero=detail_card(pad,y,inner,hero_h);
    vacuum_robot(hero,edge,(hero_h-robot)/2,robot,look);
    // Locate, when the robot can do it (supported_features 512; unknown features keep the button).
    bool locate=large && (!t.supported || (t.supported & 512));
    int key=48,text_x=edge+robot+(large?18:12),text_w=inner-text_x-(locate?key+2*edge:edge);
    int line=lv_font_get_line_height(big),text_h=lv_font_get_line_height(text),space=large?6:3;
    bool room=on_the_way && !t.extra().room.empty();
    int block=line+(room?space+text_h:0)+(battery?space+text_h:0),ty=(hero_h-block)/2;
    detail_badge_status=detail_text(hero,state,text_x,ty,text_w,big,LV_TEXT_ALIGN_LEFT,theme::INK);
    ty+=line;
    if(room){ty+=space;detail_text(hero,t.extra().room,text_x,ty,text_w,text,LV_TEXT_ALIGN_LEFT,theme::MUTED);ty+=text_h;}
    if(battery){ty+=space;vacuum_power(hero,t,text_x,ty,text,large);}
    if(locate){
      auto *find=detail_button("",pad+inner-edge-key-6,y+(hero_h-key)/2,key,key,3);
      lv_obj_set_style_radius(find,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(find,theme::color(theme::TRACK),0);
      lv_obj_set_style_bg_color(find,theme::color(theme::KEY_PRESSED),LV_STATE_PRESSED);
      auto *glyph=lv_obj_get_child(find,0);lv_obj_set_style_text_font(glyph,icons,0);lv_label_set_text(glyph,"\U000F034E");
      lv_obj_set_style_text_color(glyph,theme::color(theme::SLATE),0);lv_obj_set_size(glyph,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_center(glyph);
    }
    y+=hero_h+gap;
  }else{
    // Status row: a dot in the state colour, the state (and the room on the way), the battery at the right.
    int line=lv_font_get_line_height(text),dot=8,power_w=battery?vacuum_power(nullptr,t,0,0,text,false):0;
    detail_shape(detail_root,pad+2,y+(line-dot)/2,dot,dot,look.accent,dot/2);
    int words=inner-dot-10-power_w-12,state_w=std::min(words,text_width(state,text)+2);
    detail_badge_status=detail_text(detail_root,state,pad+dot+10,y,state_w,text,LV_TEXT_ALIGN_LEFT,theme::INK);
    if(on_the_way && !t.extra().room.empty() && words-state_w>24)
      detail_text(detail_root," · "+t.extra().room,pad+dot+10+state_w,y,words-state_w,text,LV_TEXT_ALIGN_LEFT,theme::MUTED);
    if(battery)vacuum_power(detail_root,t,pad+inner-power_w,y,text,false);
    y+=line+(gap+4);
  }
  // Start (or pause, or resume) as the one blue button, dock beside it.
  int action_h=large?54:(small_hero?40:36),start_w=large?(inner-gap)*2/3:(inner-gap)/2;
  vacuum_command(cleaning?"\U000F03E4":"\U000F040A",cleaning?(large?"Pause cleaning":"Pause"):paused?"Resume":(large?"Start cleaning":"Clean"),
                 pad,y,start_w,action_h,cleaning?1:0,true,text,icons,large?radius:action_h/2);
  vacuum_command("\U000F05F8","Dock",pad+start_w+gap,y,inner-start_w-gap,action_h,2,false,text,icons,large?radius:action_h/2);
  y+=action_h+gap;
  if(!mode && !suction && !water){
    auto *note=detail_text(detail_root,"Automatic suction power",pad,y+gap,inner,text,LV_TEXT_ALIGN_CENTER,theme::SUBTLE);(void)note;
    return;
  }
  // How it cleans. The Guition groups the rows on one white card; the small screen has no room for a card.
  int mode_h=large?48:34,row_h=large?44:32,step=large?10:6,edge=large?14:0,icon_w=large?40:26;
  int note_h=lv_font_get_line_height(text);
  int block=(mode?mode_h:0)+(suction?(mode?step:0)+row_h:0)+(water?((mode||suction)?step:0)+row_h:0)+(automatic?step+note_h+step:0);
  const uint32_t track=theme::hex(large?theme::TRACK:theme::CARD);
  if(large)detail_card(pad,y,inner,block+2*edge);
  int x=pad+edge,w=inner-2*edge;
  y+=edge;
  if(mode){vacuum_segments(t,*mode,50,x,y,w,mode_h,track,!large,text);y+=mode_h+step;}
  auto level=[&](const Choice &row,int first,const char *icon){
    detail_text(detail_root,icon,x,y+(row_h-lv_font_get_line_height(icons))/2,icon_w,icons,LV_TEXT_ALIGN_LEFT,theme::SUBTLE);
    vacuum_segments(t,row,first,x+icon_w,y,w-icon_w,row_h,track,!large,text);
    y+=row_h+step;
  };
  if(suction)level(*suction,10,"\U000F0210");
  if(water)level(*water,60,"\U000F058C");
  if(automatic){
    bool smart=tile_controls::shown_value(t,*mode,now).find("smart")!=std::string::npos;
    detail_text(detail_root,smart?"The robot chooses suction and water":"Suction and water as set per room",x,y,w,text,LV_TEXT_ALIGN_CENTER,theme::SUBTLE);
  }
}
// ---- Cover card (firmware 0.2.50+): Home Assistant's cover dialog in the style of the vacuum and climate cards ----
// A tall slider per movement: the position as a blind hanging from the top (a fully open cover shows none of it)
// and the tilt as a handle over slats, each with its value below, like Home Assistant's own sliders. Open, stop
// and close as a row of pill keys under them; the key of the direction the cover moves is filled. A slider
// sends its value when the finger lifts, and shows it while it moves.
// Home Assistant's purple for covers; the track and the slats are its tints.
inline constexpr uint32_t COVER_ACCENT = theme::ha::PURPLE;
inline uint32_t cover_track(){return theme::tint(COVER_ACCENT,37);}
inline uint32_t cover_slats(){return theme::tint(COVER_ACCENT,80);}
inline lv_obj_t *cover_values[2]{};
inline std::string cover_status_line(const Tile &t){return t.available()?tile_controls::cover_card_status(t):"Unavailable";}
inline void cover_slider_event(lv_event_t *e){
  auto *slider=lv_event_get_target_obj(e);auto code=lv_event_get_code(e);
  const bool tilt=(uintptr_t)lv_event_get_user_data(e)==1;
  if(code==LV_EVENT_VALUE_CHANGED){
    // The range reaches past 0 and 1000 only to keep the handle inside the track.
    int raw=lv_slider_get_value(slider);
    if(raw<0||raw>1000){raw=std::clamp(raw,0,1000);lv_slider_set_value(slider,raw,LV_ANIM_OFF);}
    int percent=(int)std::lround(raw/10.0f);
    if(auto *value=cover_values[tilt?1:0])label(value,std::to_string(tilt?percent:100-percent)+"%");
    return;
  }
  if(code!=LV_EVENT_RELEASED||detail_index>=model.count)return;
  auto &t=model.tiles[detail_index];
  if(!fresh()||!t.available()||!cyd::touch_guard.accept_slider(esphome::millis(),390+(tilt?1:0)))return;
  int percent=(int)std::lround(std::clamp<int>(lv_slider_get_value(slider),0,1000)/10.0f);
  if(tilt)action("cover.set_cover_tilt_position",t.entity,"tilt_position",std::to_string(percent));
  else action("cover.set_cover_position",t.entity,"position",std::to_string(100-percent));
}
// One slider. The position fills from the top by how far the cover is closed; the tilt has no fill and its
// handle moves over slats drawn behind it, thicker towards the closed end as in Home Assistant.
inline lv_obj_t *cover_slider(lv_obj_t *parent,int x,int y,int w,int h,float value,bool tilt){
  int radius=std::max(8,w/7),handle_h=std::max(4,w/18),handle_w=tilt?w*3/5:w*2/5,inset=std::max(6,w/9);
  if(tilt){
    auto *slats=detail_shape(parent,x,y,w,h,cover_track(),radius);
    const int count=10,pitch=(h-2*radius/2)/count;
    for(int i=0;i<count;++i){
      int thick=std::max(2,pitch*(20+60*i/(count-1))/100);
      detail_shape(slats,inset,radius/2+i*pitch+(pitch-thick)/2,w-2*inset,thick,cover_slats(),thick/2);
    }
  }
  auto *slider=lv_slider_create(parent);lv_obj_remove_style_all(slider);
  lv_obj_set_pos(slider,x,y);lv_obj_set_size(slider,w,h);lv_slider_set_orientation(slider,LV_SLIDER_ORIENTATION_VERTICAL);
  lv_obj_set_style_radius(slider,radius,LV_PART_MAIN);lv_obj_set_style_radius(slider,radius,LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider,lv_color_hex(cover_track()),LV_PART_MAIN);lv_obj_set_style_bg_opa(slider,tilt?LV_OPA_TRANSP:LV_OPA_COVER,LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider,lv_color_hex(COVER_ACCENT),LV_PART_INDICATOR);lv_obj_set_style_bg_opa(slider,tilt?LV_OPA_TRANSP:LV_OPA_COVER,LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider,tilt?lv_color_hex(COVER_ACCENT):theme::color(theme::KNOB),LV_PART_KNOB);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB);
  lv_obj_set_style_radius(slider,LV_RADIUS_CIRCLE,LV_PART_KNOB);
  lv_obj_set_style_opa(slider,LV_OPA_50,LV_STATE_DISABLED);
  // LVGL centres the knob on the end of the fill in a square as wide as the slider; the pads shrink it to a
  // handle bar. The position's handle sits inside the blind, an inset above its edge; the tilt's is centred.
  int side=-(w-handle_w)/2;
  lv_obj_set_style_pad_left(slider,side,LV_PART_KNOB);lv_obj_set_style_pad_right(slider,side,LV_PART_KNOB);
  if(tilt){
    int squeeze=-(w-2*handle_h)/2;
    lv_obj_set_style_pad_top(slider,squeeze,LV_PART_KNOB);lv_obj_set_style_pad_bottom(slider,squeeze,LV_PART_KNOB);
    // A margin past both ends keeps the handle on the track at 0 and 100 %.
    int margin=1000*(inset+handle_h)/std::max(1,h-2*(inset+handle_h));
    lv_slider_set_range(slider,-margin,1000+margin);
    lv_slider_set_value(slider,std::isfinite(value)?(int)std::lround(std::clamp(value,0.0f,100.0f)*10):500,LV_ANIM_OFF);
  }else{
    lv_obj_set_style_pad_top(slider,inset+handle_h-w/2,LV_PART_KNOB);lv_obj_set_style_pad_bottom(slider,1-inset-w/2,LV_PART_KNOB);
    // Reversed, the fill hangs from the top; the stub past 0 holds the handle of a cover that is fully open.
    int stub=inset+handle_h+inset,below=1000*stub/std::max(1,h-stub);
    lv_slider_set_range(slider,1000,-below);
    lv_slider_set_value(slider,std::isfinite(value)?(int)std::lround((100.0f-std::clamp(value,0.0f,100.0f))*10):0,LV_ANIM_OFF);
  }
  lv_obj_add_event_cb(slider,cover_slider_event,LV_EVENT_VALUE_CHANGED,(void*)(uintptr_t)(tilt?1:0));
  lv_obj_add_event_cb(slider,cover_slider_event,LV_EVENT_RELEASED,(void*)(uintptr_t)(tilt?1:0));
  if(detail_action_count<32)detail_actions[detail_action_count++]=slider;
  return slider;
}
// A row of pill keys with an icon each, as the climate card's modes. A key that cannot move the cover further
// is drawn disabled and left out of the keys the card enables again once Home Assistant answers.
inline void cover_key_row(const std::array<tile_controls::Key,3> &keys,unsigned count,int x,int y,int w,int h,int gap,const lv_font_t *icons){
  int key_w=count?(w-gap*((int)count-1))/(int)count:0;
  for(unsigned i=0;i<count;++i){
    const auto &key=keys[i];
    auto *button=detail_button("",x+(int)i*(key_w+gap),y,key_w,h,70+key.command);
    lv_obj_set_style_radius(button,h/2,0);
    lv_obj_set_style_bg_color(button,key.checked?lv_color_hex(COVER_ACCENT):theme::color(theme::CARD),0);
    lv_obj_set_style_bg_color(button,key.checked?lv_color_hex(theme::ha::PURPLE_PRESSED):theme::color(theme::KEY),LV_STATE_PRESSED);
    lv_obj_set_style_border_width(button,key.checked?0:1,0);lv_obj_set_style_border_color(button,theme::color(theme::LINE),0);
    auto *glyph=lv_obj_get_child(button,0);
    lv_obj_set_style_text_font(glyph,icons,0);lv_label_set_text(glyph,key.icon);
    lv_obj_set_style_text_color(glyph,theme::color(key.checked?theme::ON_ACCENT:theme::INK),0);
    lv_obj_set_size(glyph,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_center(glyph);
    if(key.disabled){lv_obj_add_state(button,LV_STATE_DISABLED);if(detail_action_count && detail_actions[detail_action_count-1]==button)--detail_action_count;}
  }
}
// A battery-powered cover shows its level at the top right, across from the back key: the meter over the
// percentage, red below 20 % (the battery sensor of its device, app 0.2.58+).
inline void cover_battery(const Tile &t,bool large,int width){
  if(!std::isfinite(t.battery))return;
  const lv_font_t *font=large?detail_font:(control_font?control_font:detail_font);
  int bar=large?60:40,bar_x=large?16:10,bar_y=large?16:8,meter_w=large?30:22,meter_h=large?15:11,gap=large?4:2;
  int level=(int)std::lround(std::clamp(t.battery,0.0f,100.0f));
  int line=lv_font_get_line_height(font),block=meter_h+gap+line,cx=width-bar_x-bar/2,y=bar_y+(bar-block)/2;
  vacuum_battery(detail_root,cx-meter_w/2-1,y,meter_w,meter_h,t.battery);
  detail_text(detail_root,std::to_string(level)+"%",cx-bar/2-8,y+meter_h+gap,bar+16,font,LV_TEXT_ALIGN_CENTER,theme::MUTED);
}
inline void render_cover_detail(Tile &t,bool large,int width,int height,int pad){
  auto card=tile_controls::cover_card(t);
  cover_battery(t,large,width);
  const lv_font_t *text=large?detail_font:(control_font?control_font:detail_font);
  const lv_font_t *big=watch_font?watch_font:detail_font;
  cover_values[0]=cover_values[1]=nullptr;
  int inner=width-2*pad,gap=large?12:6,key_h=large?64:38,bottom=height-(large?18:6);
  // The tile icons' own size where it fits the keys (42 px on the Guition, 28 on the CYD).
  const lv_font_t *key_icons=widgets[0].icon_font && lv_font_get_line_height(widgets[0].icon_font)<=key_h-6?widgets[0].icon_font:(mini_icon_font?mini_icon_font:detail_font);
  std::array<tile_controls::Key,3> keys,tilt_keys;
  unsigned key_count=card.keys?tile_controls::cover_keys(t,keys):0,tilt_count=card.tilt_keys?tile_controls::cover_tilt_keys(t,tilt_keys):0;
  int rows=(key_count?1:0)+(tilt_count?1:0);
  int keys_y=bottom-rows*key_h-(rows>1?gap:0),top=large?108:72;
  int box_h=keys_y-gap-top;
  if(card.position||card.tilt){
    detail_card(pad,top,inner,box_h);
    struct Part{bool tilt;float value;const char *caption;};
    Part parts[2];int n=0;
    if(card.position)parts[n++]={false,t.position,"Position"};
    if(card.tilt)parts[n++]={true,t.extra().tilt,"Tilt"};
    int value_h=lv_font_get_line_height(big),caption_h=lv_font_get_line_height(text);
    auto percent=[](float value){return std::isfinite(value)?std::to_string((int)std::lround(std::clamp(value,0.0f,100.0f)))+"%":std::string("--");};
    if(large){
      // Columns: the slider with its value and name below.
      int edge=16,slider_h=box_h-2*edge-value_h-caption_h+4,slider_w=n==2?120:140,column_gap=48;
      int x=pad+(inner-(n*slider_w+(n-1)*column_gap))/2;
      for(int i=0;i<n;++i,x+=slider_w+column_gap){
        cover_slider(detail_root,x,top+edge,slider_w,slider_h,parts[i].value,parts[i].tilt);
        int ty=top+edge+slider_h+2;
        cover_values[parts[i].tilt?1:0]=detail_text(detail_root,percent(parts[i].value),x-30,ty,slider_w+60,big,LV_TEXT_ALIGN_CENTER,theme::INK);
        detail_text(detail_root,parts[i].caption,x-30,ty+value_h,slider_w+60,text,LV_TEXT_ALIGN_CENTER,theme::SUBTLE);
      }
    }else{
      // The small screen puts the value and name beside each slider.
      int edge=6,slider_h=box_h-2*edge,slider_w=n==2?48:56,label_w=n==2?76:96,column_gap=n==2?16:0;
      int column=slider_w+8+label_w,x=pad+(inner-(n*column+(n-1)*column_gap))/2;
      for(int i=0;i<n;++i,x+=column+column_gap){
        cover_slider(detail_root,x,top+edge,slider_w,slider_h,parts[i].value,parts[i].tilt);
        int ty=top+(box_h-value_h-caption_h)/2;
        cover_values[parts[i].tilt?1:0]=detail_text(detail_root,percent(parts[i].value),x+slider_w+8,ty,label_w,big,LV_TEXT_ALIGN_LEFT,theme::INK);
        detail_text(detail_root,parts[i].caption,x+slider_w+8,ty+value_h,label_w,text,LV_TEXT_ALIGN_LEFT,theme::SUBTLE);
      }
    }
  }else{
    // Open and close only (a garage door, a gate): the cover's icon on a halo and its state, as the vacuum's hero.
    detail_card(pad,top,inner,box_h);
    const lv_font_t *icon_font=widgets[0].icon_font?widgets[0].icon_font:mini_icon_font;
    int halo=std::min(box_h-24,large?120:72);
    auto *ring=detail_shape(detail_root,pad+(inner-halo)/2,top+(box_h-halo)/2,halo,halo,cover_track(),halo/2);
    if(icon_font){auto *icon=detail_text(ring,icon_for(t),0,(halo-lv_font_get_line_height(icon_font))/2,halo,icon_font,LV_TEXT_ALIGN_CENTER,theme::foreground(COVER_ACCENT));(void)icon;}
  }
  int y=keys_y;
  if(key_count){cover_key_row(keys,key_count,pad,y,inner,key_h,gap,key_icons);y+=key_h+gap;}
  if(tilt_count)cover_key_row(tilt_keys,tilt_count,pad,y,inner,key_h,gap,key_icons);
}
// ---- History card (firmware 0.2.51+): numbers as a line with axes, states as a timeline ----
// Sensors, numbers, switches, binary sensors and people. The card asks the manager for the chosen range when it
// opens (an hour, a day or a week; always 24 averages or 96 slots) and draws what comes back. A finger on the
// graph shows the value or state and its time at the top, until it lifts.
inline bool history_card(const Tile &t){
  auto d=t.domain();
  return d=="sensor"||d=="binary_sensor"||d=="switch"||d=="input_boolean"||d=="person"||d=="number"||d=="input_number";
}
// Before the history arrives: a line for numbers, a timeline for the rest.
inline bool history_line_guess(const Tile &t){
  auto d=t.domain();
  if(d=="number"||d=="input_number")return true;
  if(d!="sensor"||t.device_class=="enum"||t.device_class=="timestamp"||t.device_class=="date")return false;
  if(!t.unit.empty())return true;
  char *end=nullptr;std::strtof(t.state.c_str(),&end);return end!=t.state.c_str();
}
// The open card's parts that a finger changes, and the plot: x, y, w, h on the screen.
struct HistoryChart {
  lv_obj_t *value=nullptr,*first=nullptr,*second=nullptr,*area=nullptr,*status=nullptr;
  int x=0,y=0,w=0,h=0;
  float bottom=0,top=1;
  // The highest and lowest moment the card shows: the history's, or the value now when it goes beyond them.
  float high=NAN,low=NAN;
  uint32_t high_at=0,low_at=0;
  bool line=true,ready=false;
  uint32_t accent=theme::ha::BLUE;
  std::string value_text,first_text,second_text;
  lv_point_precise_t *points=nullptr;unsigned count=0;
};
inline HistoryChart history_chart;
inline void history_forget(){
  auto &c=history_chart;c.value=c.first=c.second=c.area=c.status=nullptr;c.count=0;c.ready=false;c.high=c.low=NAN;
}
inline float history_y(float value){
  const auto &c=history_chart;
  return c.y+c.h-(std::clamp(value,c.bottom,c.top)-c.bottom)/(c.top-c.bottom)*c.h;
}
// Where a moment of the range lies across the plot, from its left edge.
inline float history_x(uint32_t at){
  const auto &h=history;const auto &c=history_chart;
  return float(std::clamp(at,h.start,h.end)-h.start)/float(std::max<uint32_t>(1,h.end-h.start))*(c.w-1);
}
// The history holds the open card's entity and range, and is new: the answer to this opening, or less than a
// minute old (a card opened again shows it at once while it asks).
inline bool history_fits(const Tile &t){
  return history.entity==t.entity&&history.hours==history_hours&&
    (history_answered||esphome::millis()-history_received_at<60000);
}
// Home Assistant's word for the state now: from this entity's history words, else the card's own. The history can
// still be the previous card's while this one waits for its answer.
inline std::string history_words(const Tile &t){
  if(!t.available())return "Unavailable";
  if(history.entity==t.entity)for(const auto &pair:history.words)if(pair.first==t.state)return pair.second;
  return detail_state(t);
}
inline std::string history_clock(uint32_t epoch,bool weekday){
  return history_view::clock(epoch,history.offset,screen_settings::current.clock_24h!=0,weekday);
}
// The soft area under the line: two triangles per segment down to the plot's bottom, no layer buffer.
inline void history_fill(lv_event_t *e){
  const auto &c=history_chart;if(!c.line||c.count<2||!c.area)return;
  auto *layer=lv_event_get_layer(e);lv_area_t area;lv_obj_get_coords(c.area,&area);
  lv_draw_triangle_dsc_t dsc;lv_draw_triangle_dsc_init(&dsc);dsc.color=lv_color_hex(c.accent);dsc.opa=theme::fill_opacity();
  const lv_value_precise_t ox=area.x1,oy=area.y1,base=area.y2+1;
  for(unsigned i=0;i+1<c.count;++i){
    const auto &a=c.points[i],&b=c.points[i+1];
    dsc.p[0]={ox+a.x,oy+a.y};dsc.p[1]={ox+b.x,oy+b.y};dsc.p[2]={ox+b.x,base};lv_draw_triangle(layer,&dsc);
    dsc.p[1]=dsc.p[2];dsc.p[2]={ox+a.x,base};lv_draw_triangle(layer,&dsc);
  }
}
// The timeline: one rectangle per run, drawn in the bar's own draw event instead of an object each.
inline void history_bar(lv_event_t *e){
  const auto &h=history;if(h.line||!h.slots)return;
  auto *obj=lv_event_get_current_target_obj(e);auto *layer=lv_event_get_layer(e);
  lv_area_t area;lv_obj_get_coords(obj,&area);const int w=lv_area_get_width(&area);
  lv_draw_rect_dsc_t dsc;lv_draw_rect_dsc_init(&dsc);dsc.bg_opa=LV_OPA_COVER;
  for(size_t i=0;i<h.runs.size();++i){
    const int state=h.runs[i].state;
    const int x1=area.x1+w*h.runs[i].slot/h.slots,x2=area.x1+w*history_view::run_end(h.runs,h.slots,i)/h.slots-1;
    dsc.bg_color=lv_color_hex(state<0||state>=static_cast<int>(h.states.size())?theme::hex(theme::TRACK):theme::state(h.states[state].color));
    lv_area_t piece{x1,area.y1,std::max(x1,x2),area.y2};lv_draw_rect(layer,&dsc,&piece);
  }
}
inline void history_restore(){
  auto &c=history_chart;
  if(c.value){label(c.value,c.value_text);lv_obj_set_style_text_color(c.value,theme::color(theme::INK),0);}
  if(c.first)label(c.first,c.first_text);
  if(c.second)label(c.second,c.second_text);
}
// A finger on the graph: the value (or state) of the moment under it, and when, at the top of the card. The graph
// itself stays as it is.
inline void history_scrub(lv_event_t *e){
  auto &c=history_chart;const auto &h=history;auto code=lv_event_get_code(e);
  if(code==LV_EVENT_RELEASED||code==LV_EVENT_PRESS_LOST){history_restore();return;}
  if((code!=LV_EVENT_PRESSED&&code!=LV_EVENT_PRESSING)||!c.ready||!c.value||!lv_indev_active())return;
  lv_point_t point;lv_indev_get_point(lv_indev_active(),&point);
  const float fraction=std::clamp(float(point.x-c.x)/float(std::max(1,c.w)),0.0f,1.0f);
  const bool week=h.hours==168;
  if(c.line){
    // A part without a value (before the sensor existed, while it was unavailable) says so.
    const int i=history_view::part_at(fraction);
    const uint64_t span=h.end-h.start;
    const uint32_t begin=h.start+static_cast<uint32_t>(span*i/history_view::PARTS),finish=h.start+static_cast<uint32_t>(span*(i+1)/history_view::PARTS);
    label(c.value,h.has[i]?history_view::number(h.values[i],h.decimals,h.unit):"No data");
    lv_obj_set_style_text_color(c.value,lv_color_hex(h.has[i]?theme::foreground(c.accent):theme::hex(theme::SUBTLE)),0);
    if(c.first)label(c.first,history_clock(begin,week)+" \u2013 "+history_clock(finish,false));
    if(c.second)label(c.second,!h.has[i]?"":h.hours==1?"average":"average of "+history_view::duration(finish-begin));
  }else{
    // A run reads the real times of its state, not the slots it covers: a door open for 5 minutes says 5 min.
    const int r=history_view::run_at(h.runs,h.slots,fraction);if(r<0)return;
    const auto &run=h.runs[r];
    const uint32_t begin=h.start+run.begin,finish=h.start+run.end;
    label(c.value,run.state<0||run.state>=static_cast<int>(h.states.size())?"No data":h.states[run.state].label);
    if(c.first)label(c.first,history_clock(begin,week)+" \u2013 "+history_clock(finish,week&&finish-begin>=43200));
    if(c.second)label(c.second,history_view::duration(run.seconds));
  }
}
// The transparent area a finger scrubs, over the graph and a little around it; it keeps the finger while it drags.
inline void history_touch(int x,int y,int w,int h){
  auto *area=lv_obj_create(detail_root);lv_obj_remove_style_all(area);lv_obj_set_pos(area,x,y);lv_obj_set_size(area,w,h);
  lv_obj_add_flag(area,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_flag(area,LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_remove_flag(area,LV_OBJ_FLAG_SCROLLABLE);lv_obj_remove_flag(area,LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_event_cb(area,history_scrub,LV_EVENT_ALL,nullptr);
}
// Times below the graph: round clock times (weekdays for a week) where they fit, and "Now" at the end.
inline void history_times(int x,int w,int y,const lv_font_t *font,bool large){
  const auto &h=history;
  const lv_font_t *bold=detail_font?detail_font:font;
  const int now_w=text_width("Now",bold),gap=large?10:6;
  detail_text(detail_root,"Now",x+w-now_w,y,now_w+2,bold,LV_TEXT_ALIGN_LEFT,theme::INK);
  int last=x-1000;
  for(uint32_t at:h.times){
    if(at<=h.start||at>=h.end)continue;
    const std::string words=h.hours==168?history_view::clock(at,h.offset,true,true).substr(0,3)
      :history_view::axis_clock(at,h.offset,screen_settings::current.clock_24h!=0);
    const int tw=text_width(words,font),cx=x+static_cast<int>(std::lround(float(at-h.start)/float(h.end-h.start)*w));
    const int left=std::max(x-(large?8:4),cx-tw/2);
    if(left<=last+gap||left+tw>x+w-now_w-gap)continue;
    detail_shape(detail_root,cx,y-(large?7:4),1,large?5:3,theme::TICK,0);
    detail_text(detail_root,words,left,y,tw+2,font,LV_TEXT_ALIGN_LEFT,theme::SUBTLE);
    last=left+tw;
  }
}
inline void render_history_line(bool large,int card_x,int card_y,int card_w,int card_h,const lv_font_t *small,float current){
  auto &c=history_chart;const auto &h=history;
  // The value now can lie outside the history (the last hour is not in the statistics yet): the plot makes room.
  c.bottom=h.bottom;c.top=h.top;
  if(std::isfinite(current)&&(current<c.bottom||current>c.top)){
    const float margin=(std::max(c.top,current)-std::min(c.bottom,current))*0.06f;
    if(current<c.bottom)c.bottom=current-margin;else c.top=current+margin;
  }
  const int label_h=lv_font_get_line_height(small),edge=large?14:8;
  int label_w=0;for(const auto &tick:h.ticks)label_w=std::max(label_w,text_width(tick.second,small));
  c.x=card_x+edge+label_w+(label_w?(large?10:5):0);c.w=card_x+card_w-(large?18:10)-c.x;
  c.y=card_y+(large?22:10);c.h=card_y+card_h-(label_h+(large?16:8))-c.y;
  if(c.w<40||c.h<24)return;
  for(const auto &tick:h.ticks){
    const int ty=static_cast<int>(std::lround(history_y(tick.first)));
    detail_shape(detail_root,c.x,ty,c.w,1,theme::GRID,0);
    detail_text(detail_root,tick.second,card_x+edge,ty-label_h/2,label_w+2,small,LV_TEXT_ALIGN_RIGHT,theme::SUBTLE);
  }
  // Through the middle of each part, from the left edge when the range begins with a value, to "Now" at the right
  // (the value now); a curve that never overshoots them, so the axis and the highest and lowest moment stay true.
  // The averages only reach the highest and lowest moment in their hour, so the line also runs through those two
  // moments: its peak and its valley are where the rings are.
  float xs[history_view::PARTS+4],ys[history_view::PARTS+4];unsigned n=0;
  for(int i=0;i<history_view::PARTS;++i){
    if(!h.has[i])continue;
    const float y=history_y(h.values[i])-c.y;
    if(i==0){xs[n]=0;ys[n++]=y;}
    xs[n]=history_view::part_middle(i)*(c.w-1);ys[n++]=y;
  }
  if(n&&std::isfinite(current)){xs[n]=float(c.w-1);ys[n++]=history_y(current)-c.y;}
  // A moment takes the place of its part's point, so the curve stays smooth; its ring goes where the point is.
  float high_x=history_x(c.high_at),low_x=history_x(c.low_at);
  if(n){
    const float snap=(c.w-1)/float(2*history_view::PARTS);
    int kept=-1;
    if(std::isfinite(c.high)&&(kept=history_view::through(xs,ys,n,history_view::PARTS+4,high_x,history_y(c.high)-c.y,snap))>=0)high_x=xs[kept];
    if(std::isfinite(c.low)){
      const int at=history_view::through(xs,ys,n,history_view::PARTS+4,low_x,history_y(c.low)-c.y,snap,kept);
      if(at>=0)low_x=xs[at];
    }
  }
  if(!c.points)c.points=new lv_point_precise_t[POINT_BUFFER];
  c.count=n>=2?history_view::monotone(xs,ys,n,4,c.points,POINT_BUFFER):0;
  c.area=lv_obj_create(detail_root);lv_obj_remove_style_all(c.area);lv_obj_set_pos(c.area,c.x,c.y);lv_obj_set_size(c.area,c.w,c.h);
  lv_obj_remove_flag(c.area,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(c.area,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(c.area,history_fill,LV_EVENT_DRAW_MAIN,nullptr);
  if(c.count>=2){
    auto *stroke=lv_line_create(detail_root);lv_obj_remove_flag(stroke,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_line_rounded(stroke,true,0);lv_obj_set_style_line_width(stroke,large?3:2,0);
    lv_obj_set_style_line_color(stroke,lv_color_hex(c.accent),0);
    lv_line_set_points(stroke,c.points,c.count);lv_obj_set_pos(stroke,c.x,c.y);
  }
  // The highest and lowest moment as rings; the Guition writes their values beside them.
  const int ring=large?12:8;
  auto marker=[&](float value,float at_x,bool high){
    const int mx=c.x+static_cast<int>(std::lround(at_x));
    const int my=static_cast<int>(std::lround(history_y(value)));
    auto *o=detail_shape(detail_root,mx-ring/2,my-ring/2,ring,ring,theme::CARD,ring/2);
    lv_obj_set_style_border_width(o,large?3:2,0);lv_obj_set_style_border_color(o,lv_color_hex(c.accent),0);
    if(!large)return;
    const std::string words=history_view::number(value,h.decimals,"");
    const int lh=lv_font_get_line_height(detail_font),tw=text_width(words,detail_font)+2;
    int ly=high?my-ring/2-lh-2:my+ring/2+2;
    if(ly<card_y+4)ly=my+ring/2+2;
    if(ly+lh>c.y+c.h+6)ly=my-ring/2-lh-2;
    const int lx=std::max(c.x,std::min(mx-tw/2,c.x+c.w-tw));
    detail_text(detail_root,words,lx,ly,tw,detail_font,LV_TEXT_ALIGN_CENTER,theme::INK);
  };
  if(std::isfinite(c.high))marker(c.high,high_x,true);
  if(std::isfinite(c.low)&&c.low!=c.high)marker(c.low,low_x,false);
  if(std::isfinite(current)){
    const int size=large?12:8;
    auto *o=detail_shape(detail_root,c.x+c.w-1-size/2,static_cast<int>(std::lround(history_y(current)))-size/2,size,size,c.accent,size/2);
    lv_obj_set_style_border_width(o,2,0);lv_obj_set_style_border_color(o,theme::color(theme::CARD),0);
  }
  history_times(c.x,c.w,c.y+c.h+(large?8:4),small,large);
  history_touch(c.x-(large?14:8),card_y,c.w+(large?28:16),card_h);
}
inline void render_history_timeline(bool large,int card_x,int card_y,int card_w,int card_h,const lv_font_t *small){
  auto &c=history_chart;const auto &h=history;
  const int edge=large?16:8,label_h=lv_font_get_line_height(small),times_gap=large?8:4,legend_gap=large?16:6;
  const int row_h=label_h+(large?10:3),square=large?14:8;
  c.x=card_x+edge;c.w=card_w-2*edge;c.h=large?56:24;
  // Bar, times and legend as one block in the middle of the card; a legend that does not fit loses its last rows.
  const int above=c.h+times_gap+label_h+legend_gap;
  const int rows=std::max(1,std::min(static_cast<int>((h.states.size()+1)/2),(card_h-2*edge-above+row_h-label_h)/row_h));
  const int block=above+rows*row_h-(row_h-label_h);
  c.y=card_y+std::max(edge,(card_h-block)/2);
  auto *bar=lv_obj_create(detail_root);lv_obj_remove_style_all(bar);lv_obj_set_pos(bar,c.x,c.y);lv_obj_set_size(bar,c.w,c.h);
  lv_obj_remove_flag(bar,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(bar,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(bar,history_bar,LV_EVENT_DRAW_MAIN,nullptr);
  history_times(c.x,c.w,c.y+c.h+times_gap,small,large);
  // Every state with its colour and its time, in two columns.
  const int top=c.y+above,col_w=c.w/2;
  const size_t shown=std::min<size_t>(h.states.size(),static_cast<size_t>(rows)*2);
  for(size_t i=0;i<shown;++i){
    const int lx=c.x+static_cast<int>(i%2)*col_w,ly=top+static_cast<int>(i/2)*row_h;
    detail_shape(detail_root,lx,ly+(label_h-square)/2,square,square,theme::state(h.states[i].color),large?4:2);
    const std::string time=history_view::duration(h.states[i].seconds);
    const int tw=text_width(time,small)+2,words_x=lx+square+(large?10:5),time_x=lx+col_w-(large?14:8)-tw;
    detail_text(detail_root,h.states[i].label,words_x,ly,std::max(8,time_x-words_x-6),small,LV_TEXT_ALIGN_LEFT,theme::INK);
    detail_text(detail_root,time,time_x,ly,tw,small,LV_TEXT_ALIGN_LEFT,theme::SUBTLE);
  }
  history_touch(c.x-(large?14:8),card_y,c.w+(large?28:16),c.y+c.h+(large?24:12)-card_y);
}
// The range: an hour, a day or a week, as one segmented row. It asks the manager, not Home Assistant, so
// waiting for a command does not lock it.
inline void history_ranges(int x,int y,int w,int h){
  static const uint32_t hours[]={1,24,168};
  static const char *const words[]={"1 hour","24 hours","1 week"};
  const lv_font_t *font=control_font?control_font:detail_font;
  auto *track=detail_shape(detail_root,x,y,w,h,theme::CARD,h/2);
  lv_obj_set_style_border_width(track,1,0);lv_obj_set_style_border_color(track,theme::color(theme::LINE),0);
  const int inset=std::max(3,h/10),sw=(w-2*inset)/3;
  for(int i=0;i<3;++i){
    const bool selected=history_hours==hours[i];
    const int segment_w=i==2?w-2*inset-2*sw:sw;
    auto *segment=detail_button(words[i],x+inset+i*sw,y+inset,segment_w,h-2*inset,160+i);
    lv_obj_set_style_radius(segment,(h-2*inset)/2,0);
    lv_obj_set_style_bg_color(segment,theme::color(theme::ACCENT),0);
    lv_obj_set_style_bg_opa(segment,selected?LV_OPA_COVER:LV_OPA_TRANSP,0);
    lv_obj_set_style_bg_opa(segment,LV_OPA_COVER,LV_STATE_PRESSED);
    if(!selected)lv_obj_set_style_bg_color(segment,theme::color(theme::ACCENT_TINT),LV_STATE_PRESSED);
    button_words(segment,font,theme::hex(selected?theme::ON_ACCENT:theme::INK),segment_w-6);
    if(detail_action_count&&detail_actions[detail_action_count-1]==segment)--detail_action_count;
  }
}
inline void render_history_detail(const Tile &t,bool large,int width,int height,int pad){
  auto &c=history_chart;const auto &h=history;auto d=t.domain();
  history_forget();
  // Asked once per opening and range; again after 30 s without an answer (see tick).
  if(history_asked_entity!=t.entity||history_asked_hours!=history_hours||(!history_fits(t)&&esphome::millis()-history_asked_at>30000))
    history_request(t.entity,history_hours);
  c.ready=history_fits(t);
  const bool line=c.ready?h.line:history_line_guess(t);
  c.line=line;c.accent=domain_accent(t);
  const lv_font_t *big=watch_value_font?watch_value_font:(watch_font?watch_font:detail_font);
  const lv_font_t *text=control_font?control_font:detail_font;
  const lv_font_t *small=small_font?small_font:detail_font;
  // The header: the value now (or the state), and at the right what the history says about it.
  char *end=nullptr;const float parsed=std::strtof(t.state.c_str(),&end);
  const bool numeric=end!=t.state.c_str()&&std::isfinite(parsed)&&t.available();
  const float current=numeric?parsed:NAN;
  c.first_text.clear();c.second_text.clear();
  if(line){
    int decimals=0;const auto dot=t.state.find('.');if(dot!=std::string::npos)decimals=static_cast<int>(std::min<size_t>(4,t.state.size()-dot-1));
    if(c.ready)decimals=h.decimals;
    c.value_text=!t.available()?"Unavailable":numeric?history_view::number(current,decimals,t.unit):t.state;
    if(c.ready){
      // The value now counts too: it can lie beyond the history (the running hour is not in the statistics yet).
      if(h.has_high){c.high=h.high;c.high_at=h.high_at;}
      if(h.has_low){c.low=h.low;c.low_at=h.low_at;}
      if(std::isfinite(current)&&std::isfinite(c.high)&&current>c.high){c.high=current;c.high_at=h.end;}
      if(std::isfinite(current)&&std::isfinite(c.low)&&current<c.low){c.low=current;c.low_at=h.end;}
    }
    if(std::isfinite(c.high))c.first_text="High "+history_view::number(c.high,h.decimals,h.unit)+" \u00B7 "+history_clock(c.high_at,h.hours==168);
    if(std::isfinite(c.low))c.second_text="Low "+history_view::number(c.low,h.decimals,h.unit)+" \u00B7 "+history_clock(c.low_at,h.hours==168);
  }else{
    c.value_text=history_words(t);
    if(c.ready&&h.active>=0){
      c.first_text=h.states[h.active].label+" \u00B7 "+history_view::duration(h.states[h.active].seconds);
      c.second_text=h.began==1?"once":h.began?std::to_string(h.began)+" times":"";
    }
  }
  const int row=large?84:50,row_h=lv_font_get_line_height(big),text_h=lv_font_get_line_height(text);
  int right=width-pad-(large?8:4);
  if(t.is_switch()){
    const int sw=large?84:52,sh=large?46:28;
    detail_switch=lv_switch_create(detail_root);
    lv_obj_set_size(detail_switch,sw,sh);lv_obj_set_pos(detail_switch,right-sw,row+(row_h-sh)/2);
    lv_obj_set_style_bg_color(detail_switch,theme::color(theme::SWITCH_OFF),LV_PART_MAIN);
    lv_obj_set_style_bg_color(detail_switch,lv_color_hex(theme::ha::SWITCH_ON),static_cast<lv_style_selector_t>(LV_PART_INDICATOR)|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(detail_switch,theme::color(theme::KNOB),LV_PART_KNOB);
    lv_obj_set_style_opa(detail_switch,LV_OPA_50,LV_STATE_DISABLED);
    if(t.state=="on")lv_obj_add_state(detail_switch,LV_STATE_CHECKED);
    lv_obj_add_event_cb(detail_switch,[](lv_event_t *e){
      if(detail_index>=model.count)return;
      auto &tile=model.tiles[detail_index];auto *control=lv_event_get_target_obj(e);
      bool allowed=fresh() && tile.available() && !tile.waiting(esphome::millis()) &&
        cyd::touch_guard.accept(esphome::millis(),350);
      bool requested_on=lv_obj_has_state(control,LV_STATE_CHECKED);
      // Only HA's reported state is authoritative, including a refused/failed command.
      if(tile.state=="on")lv_obj_add_state(control,LV_STATE_CHECKED);else lv_obj_remove_state(control,LV_STATE_CHECKED);
      if(allowed){tile.optimistic(requested_on);action(tile.domain()+(requested_on?".turn_on":".turn_off"),tile.entity);}
    },LV_EVENT_VALUE_CHANGED,nullptr);
    if(detail_action_count<32)detail_actions[detail_action_count++]=detail_switch;
    right-=sw+(large?14:8);
  }
  // The value column fits the widest text a finger can show there, so a readout never runs into the texts beside it.
  int widest=text_width(c.value_text,big);
  if(c.ready)widest=std::max(widest,text_width("No data",big));
  if(c.ready&&line){for(int i=0;i<history_view::PARTS;++i)if(h.has[i])widest=std::max(widest,text_width(history_view::number(h.values[i],h.decimals,h.unit),big));}
  else if(c.ready){for(const auto &s:h.states)widest=std::max(widest,text_width(s.label,big));}
  const int value_x=pad+(large?8:4),value_w=std::min(widest+6,(width-2*pad)*11/20);
  c.value=detail_text(detail_root,c.value_text,value_x,row,value_w,big,LV_TEXT_ALIGN_LEFT,theme::INK);
  const int words_x=value_x+value_w+(large?12:6),words_w=std::max(20,right-words_x),words_y=row+(row_h-2*text_h)/2;
  c.first=detail_text(detail_root,c.first_text,words_x,words_y,words_w,text,LV_TEXT_ALIGN_RIGHT,theme::MUTED);
  c.second=detail_text(detail_root,c.second_text,words_x,words_y+text_h,words_w,text,LV_TEXT_ALIGN_RIGHT,theme::MUTED);
  int top=row+std::max(row_h,2*text_h)+(large?10:4);
  if(d=="number"||d=="input_number"){
    const int slider_h=large?24:14;
    auto *slider=lv_slider_create(detail_root);lv_obj_set_pos(slider,pad+(large?14:10),top+(large?10:5));
    lv_obj_set_size(slider,width-2*pad-(large?28:20),slider_h);lv_slider_set_range(slider,0,1000);
    lv_slider_set_value(slider,slider_value(t),LV_ANIM_OFF);lv_obj_set_style_bg_color(slider,theme::color(theme::SLIDER_KNOB),LV_PART_KNOB);
    lv_obj_add_event_cb(slider,slider_event,LV_EVENT_ALL,(void*)(uintptr_t)detail_index);
    top+=slider_h+(large?22:12);
  }
  const int range_h=large?48:28,bottom=height-(large?16:6),range_y=bottom-range_h,card_h=range_y-(large?10:5)-top;
  detail_card(pad,top,width-2*pad,card_h);
  const bool empty=c.ready&&(line?std::none_of(h.has,h.has+history_view::PARTS,[](bool v){return v;}):h.states.empty());
  if(!c.ready||empty){
    c.status=detail_text(detail_root,!c.ready?"Loading history...":"No history in this period",pad,top+(card_h-text_h)/2,width-2*pad,text,LV_TEXT_ALIGN_CENTER,theme::SUBTLE);
  }else if(line){
    render_history_line(large,pad,top,width-2*pad,card_h,small,current);
  }else{
    render_history_timeline(large,pad,top,width-2*pad,card_h,small);
  }
  history_ranges(pad,range_y,width-2*pad,range_h);
}
// ---- The media card (firmware 0.2.64+) ----
// "Now playing" as a phone shows it: the album cover (app 0.2.77+ serves it, a Guition draws it; the CYD keeps the
// placeholder), the title, the artist and the album, a progress bar that runs while the track plays, round keys for
// previous, play or pause and next, and the volume row. media_card.h decides where everything goes for a tall card and
// a wide one; the same parts draw the card here and a tile over the whole page (render_media_full). The keys go
// through detail_command like every card's, the slider through commit_slider.
inline lv_obj_t *media_progress_fill=nullptr,*media_elapsed_label=nullptr;  // the card's, moved by tick() once a second
inline int media_bar_width=0;
inline media_card::Rect media_art_rect;  // where the card's cover goes once it is here
inline uint32_t media_accent(){return theme::foreground(theme::ha::LIGHT_BLUE);}
inline const lv_font_t *tile_icon_font(){for(auto &w:widgets)if(w.icon_font)return w.icon_font;return mini_icon_font?mini_icon_font:detail_font;}
inline media_card::Metrics media_metrics(bool large){
  media_card::Metrics m;m.large=large;
  const lv_font_t *title=watch_font?watch_font:detail_font,*artist=control_font?control_font:detail_font,*small=small_font?small_font:detail_font;
  m.title_h=lv_font_get_line_height(title);m.artist_h=lv_font_get_line_height(artist);m.small_h=lv_font_get_line_height(small);
  return m;
}
// What the keys do, on the card and on a tile over the whole page: 20 play or pause, 21 previous, 22 next, 23 mute.
inline void media_action(Tile &t,int cmd){
  if(cmd==20)action("media_player.media_play_pause",t.entity);
  if(cmd==21)action("media_player.media_previous_track",t.entity);
  if(cmd==22)action("media_player.media_next_track",t.entity);
  if(cmd==23)action("media_player.volume_mute",t.entity,"is_volume_muted",t.muted?"false":"true");
  if(cmd==24)action("media_player.turn_on",t.entity);
}
// An off or standby player shows one key: power, when the player can be turned on from here.
inline bool media_off(const Tile &t){return t.state=="off" || t.state=="standby";}
inline std::string media_volume_text(const Tile &t){
  if(t.muted)return "Muted";
  if(!std::isfinite(t.volume))return "";
  return std::to_string((int)std::lround(std::clamp(t.volume,0.0f,1.0f)*100))+" %";
}
// A rounded box without a style of its own: the art's placeholder, the bar's track and its fill.
inline lv_obj_t *media_box(lv_obj_t *parent,lv_obj_t *existing,const media_card::Rect &r,uint32_t color,int radius){
  auto *o=existing;
  if(!o){o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);}
  lv_obj_set_pos(o,r.x,r.y);lv_obj_set_size(o,std::max(1,r.w),std::max(1,r.h));
  set_color(o,LV_STYLE_BG_COLOR,theme::rgb(color));set_number(o,LV_STYLE_RADIUS,radius);
  return o;
}
// A round key: a glyph on a grey circle, the play key on the accent, the mute key bare beside the slider. Faded while
// it cannot be used. `existing` keeps a tile's key across redraws; the card builds its keys anew each time.
inline lv_obj_t *media_key(lv_obj_t *parent,lv_obj_t *existing,const media_card::Rect &r,const char *glyph,const lv_font_t *font,bool accent,bool bare,bool enabled,lv_event_cb_t cb,void *user){
  auto *key=existing;
  if(!key){
    key=lv_obj_create(parent);lv_obj_remove_style_all(key);
    lv_obj_set_style_radius(key,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_opa(key,bare?LV_OPA_TRANSP:LV_OPA_COVER,0);lv_obj_set_style_bg_opa(key,LV_OPA_COVER,LV_STATE_PRESSED);
    lv_obj_set_style_opa(key,LV_OPA_40,LV_STATE_DISABLED);
    lv_obj_add_flag(key,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(key,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_ext_click_area(key,bare?10:6);
    auto *icon=lv_label_create(key);lv_obj_remove_flag(icon,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(key,cb,LV_EVENT_SHORT_CLICKED,user);
  }
  lv_obj_set_pos(key,r.x,r.y);lv_obj_set_size(key,r.w,r.h);
  set_color(key,LV_STYLE_BG_COLOR,theme::color(accent?theme::ACCENT:theme::KEY));
  set_color(key,LV_STYLE_BG_COLOR,theme::color(accent?theme::ACCENT_PRESSED:theme::KEY_PRESSED),LV_STATE_PRESSED);
  auto *icon=lv_obj_get_child(key,0);
  if(font)set_font(icon,font);
  set_color(icon,LV_STYLE_TEXT_COLOR,theme::color(accent?theme::ON_ACCENT:theme::INK));
  label(icon,glyph);lv_obj_center(icon);
  if(enabled)lv_obj_remove_state(key,LV_STATE_DISABLED);else lv_obj_add_state(key,LV_STATE_DISABLED);
  return key;
}
// The volume slider: the media colour on a pale track, a round white knob, the range every card slider has.
inline lv_obj_t *media_slider(lv_obj_t *parent,lv_obj_t *existing,const media_card::Rect &r,const Tile &t,bool large,bool enabled,void *user){
  auto *s=existing;
  if(!s){
    s=lv_slider_create(parent);lv_obj_remove_style_all(s);lv_slider_set_range(s,0,1000);
    lv_obj_set_style_bg_opa(s,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_opa(s,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_bg_opa(s,LV_OPA_COVER,LV_PART_KNOB);
    lv_obj_set_style_radius(s,LV_RADIUS_CIRCLE,LV_PART_MAIN);lv_obj_set_style_radius(s,LV_RADIUS_CIRCLE,LV_PART_INDICATOR);lv_obj_set_style_radius(s,LV_RADIUS_CIRCLE,LV_PART_KNOB);
    lv_obj_set_style_pad_all(s,large?4:3,LV_PART_KNOB);lv_obj_set_style_bg_opa(s,LV_OPA_80,(lv_style_selector_t)LV_PART_KNOB|(lv_style_selector_t)LV_STATE_PRESSED);
    lv_obj_set_style_opa(s,LV_OPA_40,LV_STATE_DISABLED);
    lv_obj_set_ext_click_area(s,large?12:8);
    lv_obj_add_event_cb(s,slider_event,LV_EVENT_ALL,user);
  }
  lv_obj_set_pos(s,r.x,r.y);lv_obj_set_size(s,std::max(1,r.w),r.h);
  set_color(s,LV_STYLE_BG_COLOR,theme::rgb(theme::tint(theme::ha::LIGHT_BLUE,64)),LV_PART_MAIN);
  set_color(s,LV_STYLE_BG_COLOR,theme::rgb(t.muted?theme::hex(theme::OFF):media_accent()),LV_PART_INDICATOR);
  set_color(s,LV_STYLE_BG_COLOR,theme::color(theme::SLIDER_KNOB),LV_PART_KNOB);
  if(!lv_obj_has_state(s,LV_STATE_PRESSED) && lv_slider_get_value(s)!=slider_value(t))lv_slider_set_value(s,slider_value(t),LV_ANIM_OFF);
  if(enabled){lv_obj_remove_state(s,LV_STATE_DISABLED);lv_obj_add_flag(s,LV_OBJ_FLAG_CLICKABLE);}
  else{lv_obj_add_state(s,LV_STATE_DISABLED);lv_obj_remove_flag(s,LV_OBJ_FLAG_CLICKABLE);}
  return s;
}
// The cover over its placeholder: LVGL's image widget exists only on a board whose profile draws images.
inline lv_obj_t *media_picture_show(lv_obj_t *parent,lv_obj_t *existing,const media_card::Rect &r,lv_image_dsc_t *src){
#if LV_USE_IMAGE
  if(!src || !src->data)return existing;
  auto *p=existing;
  if(!p){p=lv_image_create(parent);lv_obj_remove_flag(p,LV_OBJ_FLAG_CLICKABLE);}
  lv_image_set_src(p,src);lv_obj_set_pos(p,r.x,r.y);lv_obj_set_size(p,r.w,r.h);lv_obj_invalidate(p);
  return p;
#else
  (void)parent;(void)r;(void)src;return existing;
#endif
}
// The bar's fill and the elapsed time follow the track: once a second from tick(), without a redraw.
inline void media_progress(const Tile &t,lv_obj_t *fill,lv_obj_t *elapsed,int bar_w){
  const auto &x=t.extra();
  if(!fill || !x.media_duration)return;
  const bool play=media_card::playing(t.state);
  const int p=media_card::progress(x.media_position,x.media_position_at,now_epoch(),play,x.media_duration);
  const int h=lv_obj_get_style_height(fill,LV_PART_MAIN);
  const int w=std::max(h,bar_w*std::max(0,p)/1000);
  if(lv_obj_get_style_width(fill,LV_PART_MAIN)!=w)lv_obj_set_width(fill,w);
  if(elapsed)label(elapsed,media_card::clock_text(media_card::elapsed_seconds(x.media_position,x.media_position_at,now_epoch(),play,x.media_duration)));
}
// The card: everything under the top bar, from `top` down.
inline void render_media_detail(Tile &t,unsigned index,bool large,int width,int height,int top){
  using namespace media_card;
  using namespace tile_controls;
  const auto &x=t.extra();
  const Metrics m=media_metrics(large);
  const Layout l=layout(m,width,std::max(60,height-top-(large?12:6)));
  auto at=[&](Rect r){r.y+=top;return r;};
  const bool usable=fresh()&&t.available(),track=usable&&has_track(t.state),play=media_card::playing(t.state);
  const uint32_t f=t.supported;auto can=[&](uint32_t bit){return usable&&(!f||(f&bit));};
  // The cover, or its placeholder with the player's icon; the cover comes over it once the app served it.
  auto *frame=media_box(detail_root,nullptr,at(l.art),theme::tint(theme::ha::LIGHT_BLUE,51),l.art_radius);
  const std::string glyph=icon_for(t);
  const lv_font_t *placeholder_font=big_icon_font&&font_has(big_icon_font,glyph)?big_icon_font:tile_icon_font();
  auto *icon=lv_label_create(frame);lv_obj_remove_flag(icon,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_text_font(icon,placeholder_font,0);
  lv_obj_set_style_text_color(icon,theme::rgb(theme::icon(theme::ha::LIGHT_BLUE)),0);lv_label_set_text(icon,glyph.c_str());lv_obj_center(icon);
  media_art_rect=at(l.art);media_detail_picture=nullptr;
  const uint32_t ground=theme::hex(theme::PAGE);
  if(camera_supported()&&track&&!x.media_picture.empty()){
    cover_want(t.entity,x.media_picture,l.art.w,ground,CoverOwner::DETAIL,0);
    media_detail_picture=media_picture_show(detail_root,nullptr,media_art_rect,cover_ready(t.entity,l.art.w,ground));
  }
  // Title, artist · album.
  const lv_font_t *title_font=watch_font?watch_font:detail_font,*artist_font=control_font?control_font:detail_font,*small=small_font?small_font:detail_font;
  const lv_text_align_t align=l.wide?LV_TEXT_ALIGN_LEFT:LV_TEXT_ALIGN_CENTER;
  detail_text(detail_root,track&&!x.media_title.empty()?x.media_title:std::string(idle_text(usable?t.state:"unavailable")),l.title.x,l.title.y+top,l.title.w,title_font,align,theme::INK);
  if(l.artist)detail_text(detail_root,track?subtitle(x.media_artist,x.media_album):std::string(),l.artist_line.x,l.artist_line.y+top,l.artist_line.w,artist_font,align,theme::MUTED);
  // The progress bar: the fill runs while the track plays; a stream without a length has no bar to show.
  media_progress_fill=nullptr;media_elapsed_label=nullptr;media_bar_width=l.bar.w;
  if(track && x.media_duration){
    media_box(detail_root,nullptr,at(l.bar),theme::hex(theme::TRACK),LV_RADIUS_CIRCLE);
    Rect fill=at(l.bar);fill.w=std::max(l.bar.h,l.bar.w*std::max(0,progress(x.media_position,x.media_position_at,now_epoch(),play,x.media_duration))/1000);
    media_progress_fill=media_box(detail_root,nullptr,fill,media_accent(),LV_RADIUS_CIRCLE);
    if(l.times){
      media_elapsed_label=detail_text(detail_root,clock_text(elapsed_seconds(x.media_position,x.media_position_at,now_epoch(),play,x.media_duration)),l.elapsed.x,l.elapsed.y+top,l.elapsed.w,small,LV_TEXT_ALIGN_LEFT,theme::SUBTLE);
      detail_text(detail_root,clock_text(x.media_duration),l.total.x,l.total.y+top,l.total.w,small,LV_TEXT_ALIGN_RIGHT,theme::SUBTLE);
    }
  }
  // The keys: previous, play or pause on the accent, next; the mute key bare at the start of the volume row. An off
  // player shows one power key instead, and no volume row: it reports no volume.
  auto cb=[](lv_event_t *e){detail_command((intptr_t)lv_event_get_user_data(e));};
  const lv_font_t *key_font=mini_icon_font?mini_icon_font:detail_font;
  std::vector<lv_obj_t *> keys;
  if(usable && media_off(t)){
    if(can(feature::MEDIA_TURN_ON))keys.push_back(media_key(detail_root,nullptr,at(l.play),glyph::POWER,tile_icon_font(),true,false,true,cb,(void*)(intptr_t)24));
  }else{
    keys={media_key(detail_root,nullptr,at(l.prev),glyph::PREVIOUS,key_font,false,false,can(feature::MEDIA_PREVIOUS),cb,(void*)(intptr_t)21),
          media_key(detail_root,nullptr,at(l.play),play?glyph::PAUSE:glyph::PLAY,tile_icon_font(),true,false,can(feature::MEDIA_PLAY|feature::MEDIA_PAUSE),cb,(void*)(intptr_t)20),
          media_key(detail_root,nullptr,at(l.next),glyph::NEXT,key_font,false,false,can(feature::MEDIA_NEXT),cb,(void*)(intptr_t)22)};
    if(std::isfinite(t.volume)){
      keys.push_back(media_key(detail_root,nullptr,at(l.mute),t.muted?glyph::MUTED:glyph::VOLUME,key_font,false,true,can(feature::MEDIA_VOLUME_MUTE),cb,(void*)(intptr_t)23));
      media_slider(detail_root,nullptr,at(l.volume),t,large,can(feature::MEDIA_VOLUME_SET),(void*)(uintptr_t)index);
      detail_text(detail_root,media_volume_text(t),l.percent.x,l.percent.y+top,l.percent.w,small,LV_TEXT_ALIGN_RIGHT,theme::MUTED);
    }
  }
  // Only the keys the player supports join the card's actions: tick() enables those again after a wait, and a key
  // the player lacks stays faded.
  for(auto *k:keys)if(!lv_obj_has_state(k,LV_STATE_DISABLED) && detail_action_count<32)detail_actions[detail_action_count++]=k;
}
inline void show_detail(unsigned index){
  if(index>=model.count)return;
  // A card that opens starts on a day (an hour for a tile whose graph shows one); switching ranges keeps it open.
  if(!detail_root||lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN)||detail_index!=index){
    history_hours=model.tiles[index].history_hours==1?1:24;history_asked_entity.clear();
  }
  detail_index=index;auto &t=model.tiles[index];
  if(!detail_font)detail_font=lv_obj_get_style_text_font(widgets[0].title,LV_PART_MAIN);
  if(!detail_root){detail_root=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(detail_root);lv_obj_set_size(detail_root,lv_pct(100),lv_pct(100));lv_obj_remove_flag(detail_root,LV_OBJ_FLAG_SCROLLABLE);}
  detail_action_count=0;detail_status=nullptr;detail_badge_status=nullptr;detail_switch=nullptr;history_forget();
  media_progress_fill=nullptr;media_elapsed_label=nullptr;media_detail_picture=nullptr;lv_obj_clean(detail_root);lv_obj_remove_flag(detail_root,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(detail_root);
  lv_obj_set_style_bg_color(detail_root,theme::color(theme::PAGE),0);lv_obj_set_style_bg_opa(detail_root,LV_OPA_COVER,0);
  int width=lv_display_get_horizontal_resolution(lv_display_get_default()), height=lv_display_get_vertical_resolution(lv_display_get_default());
  bool large=width>=480;int pad=large?20:10, top=large?100:62, gap=large?12:6,bh=large?58:34,cw=(width-pad*2-gap)/2;
  // The same top bar as the board's own cards: a round back arrow at the left, the name centred.
  int bar=large?60:40,bar_x=large?16:10,bar_y=large?16:8;
  auto *back=detail_button("",bar_x,bar_y,bar,bar,-1);lv_obj_set_style_radius(back,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(back,theme::color(theme::KEY),0);
  auto *arrow=lv_obj_get_child(back,0);if(mini_icon_font)lv_obj_set_style_text_font(arrow,mini_icon_font,0);lv_label_set_text(arrow,"\U000F004D");lv_obj_set_size(arrow,LV_SIZE_CONTENT,LV_SIZE_CONTENT);lv_obj_center(arrow);
  const lv_font_t *title_font=watch_font?watch_font:detail_font;
  auto *heading=detail_label(detail_root,t.name,bar_x+bar+8,bar_y+(bar-lv_font_get_line_height(title_font))/2,width-2*(bar_x+bar+8));
  lv_obj_set_style_text_font(heading,title_font,0);lv_obj_set_height(heading,lv_font_get_line_height(title_font));lv_obj_set_style_text_align(heading,LV_TEXT_ALIGN_CENTER,0);
  auto d=t.domain();
  std::string state=d=="cover"?cover_status_line(t):detail_state(t);
  // The vacuum and history cards draw their own state.
  const bool with_history=history_card(t);
  if(d!="vacuum"&&d!="media_player"&&!with_history){detail_status=detail_label(detail_root,state+(t.unit.empty()?"":" "+t.unit),pad,large?80:50,width-2*pad);lv_obj_set_style_text_align(detail_status,LV_TEXT_ALIGN_CENTER,0);lv_obj_set_style_text_color(detail_status,theme::color(theme::MUTED),0);}
  if(with_history){
    render_history_detail(t,large,width,height,pad);
  }else if(d=="vacuum"){
    render_vacuum_detail(t,large,width,height,pad);
  }else if(d=="cover"){
    if(detail_status && control_font){lv_obj_set_style_text_font(detail_status,control_font,0);lv_obj_set_height(detail_status,lv_font_get_line_height(control_font));}
    render_cover_detail(t,large,width,height,pad);
  }else if(d=="select"||d=="input_select"){
    const auto &options=t.extra().options;
    for(unsigned i=0;i<options.size();++i)detail_button(options[i].c_str(),pad+(i%2)*(cw+gap),top+(i/2)*(bh+gap),cw,bh,30+i);
  }else if(d=="media_player"){
    // "Now playing" (firmware 0.2.64+): the cover, the track, a running progress bar, round keys and the volume row.
    render_media_detail(t,index,large,width,height,bar_y+bar+(large?8:4));
  }else if(d=="weather"){
    render_weather_detail(t,large,width,height,pad);
  }else if(d=="timer"){
    detail_label(detail_root,t.state=="active"?"Running":t.state=="paused"?"Paused":"Stopped",pad,top,width-2*pad);
    detail_button(t.state=="active"?"Pause":"Start",pad,top+(large?50:30),cw,bh,40);
    detail_button("Cancel",pad+cw+gap,top+(large?50:30),cw,bh,41);
  }else if(d=="sun"){
    detail_label(detail_root,"Sunrise "+t.extra().sunrise,pad,top,width-2*pad);
    detail_label(detail_root,"Sunset "+t.extra().sunset,pad,top+lv_font_get_line_height(detail_font)+(large?10:4),width-2*pad);
  }
}
}

namespace runtime_tiles {
inline void history_received(){
  if(detail_root&&!lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN)&&detail_index<model.count&&
     model.tiles[detail_index].entity==history.entity&&history_hours==history.hours)refresh_detail(detail_index);
}
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
  if (condition == "sunny") return "Sunny";
  if (condition == "clear-night") return "Clear, night";
  if (condition == "cloudy") return "Cloudy";
  if (condition == "partlycloudy") return "Partly cloudy";
  if (condition == "rainy") return "Rainy";
  if (condition == "pouring") return "Pouring";
  if (condition == "snowy") return "Snowy";
  if (condition == "snowy-rainy") return "Snowy, rainy";
  if (condition == "fog") return "Fog";
  if (condition == "hail") return "Hail";
  if (condition == "lightning") return "Lightning";
  if (condition == "lightning-rainy") return "Lightning, rainy";
  if (condition == "windy" || condition == "windy-variant") return "Windy";
  if (condition == "exceptional") return "Exceptional";
  return condition.c_str();
}
inline const char *icon_for(const Tile &tile) {
  if (!tile.icon.empty()) return tile.icon.c_str();
  auto d = tile.domain();
  // Home Assistant's own icon: a bulb, crossed out while off. A chosen icon stays, as in Home Assistant.
  if (d == "light" && tile.state == "off") return "\U000F0E4F";
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
  if (d == "screen") return tile.is_settings() ? "\U000F0493" : tile.is_page() ? "\U000F0054" : "\U000F0150";
  if (d == "camera" || d == "image") return "\U000F07AE";
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
// "Last 14:32" today, "Yesterday 14:32", else "Last 13 Sep"; scripts and scenes have no useful on/off.
inline std::string month_short(const esphome::ESPTime &now);
inline std::string last_run_text(uint32_t epoch) {
  if (!epoch) return "Never run";
  auto when = esphome::ESPTime::from_epoch_local(epoch);
  auto now = now_time ? now_time() : esphome::ESPTime{};
  if (!when.is_valid()) return "Never run";
  char clock[8]; snprintf(clock, sizeof(clock), "%02d:%02d", when.hour, when.minute);
  if (now.is_valid() && now.year == when.year && now.day_of_year == when.day_of_year) return std::string("Last ") + clock;
  if (now.is_valid() && now.year == when.year && now.day_of_year == when.day_of_year + 1) return std::string("Yesterday ") + clock;
  return "Last " + std::to_string(when.day_of_month) + " " + month_short(when);
}
inline std::string timer_text(const Tile &t) {
  const Extra &x = t.extra();
  if (t.state == "active") { uint32_t now = now_epoch(); return countdown(x.timer_end > now && now ? x.timer_end - now : 0); }
  if (t.state == "paused") return "Paused " + countdown(duration_seconds(x.remaining));
  return x.duration.empty() ? "Off" : countdown(duration_seconds(x.duration));
}
inline std::string weekday_text(const esphome::ESPTime &now) {
  static const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  return now.is_valid() && now.day_of_week >= 1 && now.day_of_week <= 7 ? days[now.day_of_week - 1] : "";
}
inline std::string month_short(const esphome::ESPTime &now) {
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return now.is_valid() && now.month >= 1 && now.month <= 12 ? months[now.month - 1] : "";
}
inline std::string date_text(const esphome::ESPTime &now) {
  static const char *months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
  if (!now.is_valid() || now.day_of_week < 1 || now.day_of_week > 7 || now.month < 1 || now.month > 12) return "";
  return weekday_text(now) + " " + std::to_string(now.day_of_month) + " " + months[now.month - 1];
}
// Where the finger is, or where it let go, in the screen's coordinates; false without a finger (a test's event).
inline bool finger_at(lv_point_t &point) {
  auto *indev = lv_indev_active();
  if (!indev || lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) return false;
  lv_indev_get_point(indev, &point);
  return true;
}
inline void event(lv_event_t *event) {
  auto &w = *static_cast<Widgets *>(lv_event_get_user_data(event));
  if (!enabled || w.index >= model.count) return;
  auto code = lv_event_get_code(event);
  if (code != LV_EVENT_SHORT_CLICKED && code != LV_EVENT_LONG_PRESSED) return;
  // A finger that slid off the card is not a tap or a hold on it (the Guition, firmware 0.2.65+). LVGL keeps the press
  // on the object it started on (LV_OBJ_FLAG_PRESS_LOCK, which LVGL 9.5 sets on every child) and clicks it on release
  // wherever the finger is. The CYD's drift limit already drops such a tap; the Guition has none (TOUCH_MOVE_LIMIT_PX 0,
  // so a firm press that drifts still counts) and asks where the finger let go instead. The lock itself stays: without
  // it a finger that slides on to a slider would press and drag that slider.
  lv_point_t point;
  if (cyd::touch_guard.move_limit() <= 0 && finger_at(point) && !lv_obj_hit_test(w.tile, &point)) {
    ESP_LOGI("touch", "tap on %s ignored: %s outside the tile", model.tiles[w.index].entity.c_str(),
             code == LV_EVENT_LONG_PRESSED ? "held" : "let go");
    return;
  }
  // The built-in cards need nothing from Home Assistant: the settings card opens the screen's own page with
  // the link down too, as its card says (firmware 0.2.49+), and the clock card does nothing under a finger.
  if (model.tiles[w.index].builtin()) {
    auto &card = model.tiles[w.index];
    if (card.is_page()) {
      // A navigation tile goes to its page on a short tap, with the link down too; holding does nothing.
      if (code != LV_EVENT_SHORT_CLICKED || card.tap == "none" || card.page_target() < 1) return;
      if (allowed(esphome::millis(), 100 + w.index, card.entity)) go_to_page(card.page_target() - 1);
      return;
    }
    if (!card.is_settings() || card.tap == "none" || (settings_screen::may_open && !settings_screen::may_open())) return;
    if (allowed(esphome::millis(), 100 + w.index, card.entity)) settings_screen::open();
    return;
  }
  if (!fresh()) return;
  if (!allowed(esphome::millis(), 100 + w.index, model.tiles[w.index].entity)) return;
  auto &tile = model.tiles[w.index];
  auto d = tile.domain();
  // Scenes/scripts often have timestamps or 'off'; unavailable devices never act.
  if (!tile.available() || tile.waiting(esphome::millis()) || tile.tap=="none") return;
  // A camera or an image entity opens full screen on a board that draws images (firmware 0.2.57+).
  if(d=="camera" || d=="image"){if(camera_supported())camera_open(tile.entity,tile.name);return;}
  // tile_controls::tap_route decides; tests/test_tile_controls.cpp keeps every older tap choice routed as before.
  auto tap = tile_controls::tap_route(tile, code == LV_EVENT_LONG_PRESSED);
  switch (tap.route) {
    case tile_controls::TapRoute::ACTION:
      // On / off shows the new stand at once, as Home Assistant's switch does; other actions have nothing to show yet.
      if (tap.service == d + ".toggle" && (d == "light" || d == "switch" || d == "input_boolean" || d == "fan"))
        tile.optimistic(tile.state != "on");
      action(tap.service, tile.entity, "", "", true);
      return;
    case tile_controls::TapRoute::CUSTOM:
      perform(tile);
      return;
    case tile_controls::TapRoute::CARD:
      if (tap.busy) tile.begin(esphome::millis(), true);
      active_index = w.index;
      show_detail(w.index);
      return;
    case tile_controls::TapRoute::OVERLAY:
      active_index = w.index;
      tile.begin(esphome::millis(), true);
      if (detail) detail(tile);
      return;
    default:
      return;
  }
}
// The push that wakes a screen in standby only wakes it (dim_wake_overlay), with one exception (firmware 0.2.65+): on a
// page that is one full tile which switches something on or off, the push also switches it, as a light switch on the
// wall does in the dark. It is handed to the tile as its own tap, so it goes the way a tap on a lit screen goes (the
// touch guard, the busy sheet, the new stand at once) and counts once. A push on the tile's slider or keys, the top bar
// or the page bar only wakes, as does a flick. Called by both boards' dim_wake_overlay; true when it tapped the tile.
inline bool wake_tap() {
  auto &w = widgets[0];
  if (!enabled || !fresh() || !w.tile || !w.full || w.index >= model.count || lv_obj_has_flag(w.tile, LV_OBJ_FLAG_HIDDEN)) return false;
  const auto &t = model.tiles[w.index];
  const auto route = tile_controls::tap_route(t, false);
  if (t.builtin() || route.route != tile_controls::TapRoute::ACTION || route.service != t.domain() + ".toggle") return false;
  lv_point_t point;
  if (!finger_at(point) || lv_indev_search_obj(w.tile, &point) != w.tile) return false;
  lv_obj_send_event(w.tile, LV_EVENT_SHORT_CLICKED, nullptr);
  return true;
}
// The same guard for any local style: a page switch mostly hands a slot the colours it already has,
// and each real set refreshes the style and invalidates the object (about 0.3 ms on the Guition).
inline void set_color(lv_obj_t *obj, lv_style_prop_t prop, lv_color_t color, lv_style_selector_t selector) {
  lv_style_value_t current;
  if (lv_obj_get_local_style_prop(obj, prop, &current, selector) == LV_STYLE_RES_FOUND && lv_color_eq(current.color, color)) return;
  lv_style_value_t value{}; value.color = color;
  lv_obj_set_local_style_prop(obj, prop, value, selector);
}
inline void set_number(lv_obj_t *obj, lv_style_prop_t prop, int32_t number, lv_style_selector_t selector) {
  lv_style_value_t current;
  if (lv_obj_get_local_style_prop(obj, prop, &current, selector) == LV_STYLE_RES_FOUND && current.num == number) return;
  lv_style_value_t value{}; value.num = number;
  lv_obj_set_local_style_prop(obj, prop, value, selector);
}
// Card sliders follow Home Assistant's control slider (a 42 px track there): corners of 12 and
// 8 px on track and fill, a white handle 4 px wide and half the height, an eighth of the height in
// from the end of the fill, and a fill that never gets shorter than a third of the height. LVGL
// centres the knob on the end of the fill, so the knob is narrowed and moved back; the range starts
// below zero, so at 0 the fill is still that short stub holding the handle. Values below zero never
// leave the slider (see slider_event). Small strips keep round ends: the corners would not show.
inline int slider_handle_width(int height) { return height > 20 ? 4 : 2; }
inline int slider_stub(int height) { return std::max(height / 3, 2 * std::max(1, height / 8) + slider_handle_width(height)); }
inline void slider_handle(lv_obj_t *slider, int width, int height) {
  int handle = slider_handle_width(height), inset = std::max(1, height / 8) + handle / 2, half = height >> 1;
  // Track and fill share one radius: a fill rounded less than its track makes LVGL draw the fill into
  // a buffer of its own size on every redraw (15 KB on a CYD card), which the CYD's heap can't spare.
  int corner = height > 20 ? height * 12 / 42 : LV_RADIUS_CIRCLE;
  set_number(slider, LV_STYLE_RADIUS, corner, LV_PART_MAIN);
  set_number(slider, LV_STYLE_RADIUS, corner, LV_PART_INDICATOR);
  set_number(slider, LV_STYLE_PAD_LEFT, inset + handle / 2 - half, LV_PART_KNOB);
  set_number(slider, LV_STYLE_PAD_RIGHT, handle / 2 - inset - (height - half), LV_PART_KNOB);
  set_number(slider, LV_STYLE_PAD_TOP, -(height / 4), LV_PART_KNOB);
  set_number(slider, LV_STYLE_PAD_BOTTOM, -(height / 4), LV_PART_KNOB);
  int stub = slider_stub(height), below = width > stub ? 1000 * stub / (width - stub) : 0;
  if (lv_slider_get_min_value(slider) != -below || lv_slider_get_max_value(slider) != 1000) lv_slider_set_range(slider, -below, 1000);
}
// An off light or fan shows only the grey track, as in Home Assistant: no fill, no handle.
inline void slider_bar(lv_obj_t *slider, bool shown) {
  set_number(slider, LV_STYLE_BG_OPA, shown ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_INDICATOR);
  set_number(slider, LV_STYLE_BG_OPA, shown ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_KNOB);
}
inline bool slider_bar_shown(const Tile &t, bool on) { auto d = t.domain(); return on || (d != "light" && d != "fan"); }
inline void set_hidden(lv_obj_t *obj, bool hidden) { if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); else lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN); }
// A card's size as its styles request it. LVGL's own getters only follow after a layout pass, and
// that pass walks every object on the screen, so the cards are laid out without asking for one.
inline int tile_width(const Widgets &w) {
  int32_t width = lv_obj_get_style_width(w.tile, LV_PART_MAIN);
  return LV_COORD_IS_PX(width) ? width : lv_obj_get_width(w.tile);
}
inline int tile_height(const Widgets &w) {
  int32_t height = lv_obj_get_style_height(w.tile, LV_PART_MAIN);
  return LV_COORD_IS_PX(height) ? height : lv_obj_get_height(w.tile);
}
inline int content_width(const Widgets &w) {
  return tile_width(w) - lv_obj_get_style_space_left(w.tile, LV_PART_MAIN) - lv_obj_get_style_space_right(w.tile, LV_PART_MAIN);
}
inline int content_height(const Widgets &w) {
  return tile_height(w) - lv_obj_get_style_space_top(w.tile, LV_PART_MAIN) - lv_obj_get_style_space_bottom(w.tile, LV_PART_MAIN);
}
// A card's strip is thin (8 px on a CYD card), so the whole card belongs to it: a finger that lands above the strip
// still drags it. A press that never moves taps or holds the card instead (slider_event), so nothing is lost.
inline int slider_zone(const Widgets &w, int strip) {
  return std::max(8, (int) lv_obj_get_style_space_top(w.tile, LV_PART_MAIN) + content_height(w) - strip);
}
inline void bind(size_t index, lv_obj_t *tile, lv_obj_t *title, lv_obj_t *value, lv_obj_t *circle, lv_obj_t *icon) {
  lv_obj_update_layout(tile);
  // Compact cards need room for two text lines and a separate dimmer track.
  if(lv_obj_get_height(tile)<=80){lv_obj_set_style_pad_top(tile,4,0);lv_obj_set_style_pad_bottom(tile,4,0);}
  lv_obj_set_style_border_width(tile,1,0);
  lv_obj_update_layout(tile);
  widgets[index] = {tile, title, value, circle, icon, index};
  auto &w=widgets[index]; w.title_x=lv_obj_get_x(title);w.title_y=lv_obj_get_y(title);w.value_x=lv_obj_get_x(value);w.value_y=lv_obj_get_y(value);w.value_font=lv_obj_get_style_text_font(value,LV_PART_MAIN);
  w.base_width=lv_obj_get_width(tile);w.base_height=lv_obj_get_height(tile);
  w.title_font=lv_obj_get_style_text_font(title,LV_PART_MAIN);
  w.icon_font=lv_obj_get_style_text_font(icon,LV_PART_MAIN);
  w.unit=lv_label_create(tile);lv_obj_set_style_text_font(w.unit,w.value_font,0);lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
  lv_label_set_long_mode(w.unit,LV_LABEL_LONG_DOT);
  // Fixed one-line boxes prevent wrapped names from overlapping the state on both boards.
  lv_obj_set_height(title,lv_font_get_line_height(lv_obj_get_style_text_font(title,LV_PART_MAIN)));
  lv_obj_set_height(value,lv_font_get_line_height(w.value_font));
  lv_label_set_long_mode(title,LV_LABEL_LONG_DOT);lv_label_set_long_mode(value,LV_LABEL_LONG_DOT);
  w.progress=lv_obj_create(tile);lv_obj_remove_style_all(w.progress);lv_obj_set_size(w.progress,0,3);lv_obj_align(w.progress,LV_ALIGN_BOTTOM_LEFT,0,0);lv_obj_add_flag(w.progress,LV_OBJ_FLAG_HIDDEN);
  w.slider=lv_slider_create(tile);lv_obj_set_size(w.slider,lv_obj_get_width(tile)-24,lv_obj_get_height(tile)>80?28:10);lv_obj_align(w.slider,LV_ALIGN_BOTTOM_MID,0,0);lv_slider_set_range(w.slider,0,1000);
  // A short white bar inside the fill as handle, like the control sliders (invisible before 0.2.20).
  int strip=lv_obj_get_height(tile)>80?28:10;
  lv_obj_add_style(w.slider,theme::style(theme::Paint::knob),LV_PART_KNOB);lv_obj_set_style_bg_opa(w.slider,LV_OPA_COVER,LV_PART_KNOB);
  slider_handle(w.slider,lv_obj_get_width(tile)-24,strip);
  lv_obj_set_style_border_width(w.slider,0,LV_PART_KNOB);lv_obj_set_style_shadow_width(w.slider,0,LV_PART_KNOB);
  // Track and fill follow the card's palette (render_slot), which is drawn before the slider shows.
  lv_obj_set_style_radius(w.slider,2,LV_PART_KNOB);
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
  using namespace theme::ha;
  auto d=t.domain();
  if(d=="light" || d=="switch" || d=="input_boolean" || d=="binary_sensor")return AMBER;
  // A climate card that is off or in an unknown mode keeps HA's orange climate circle.
  if(d=="climate"){uint32_t c=tile_controls::mode_color(t.state);return c==GREY?ORANGE:c;}
  if(d=="vacuum")return t.state=="error"?RED:TEAL;
  if(d=="fan")return CYAN;
  if(d=="cover")return PURPLE;
  if(d=="media_player")return LIGHT_BLUE;
  if(d=="scene" || d=="script")return PURPLE;
  if(d=="select" || d=="input_select")return INDIGO;
  if(d=="number" || d=="input_number")return TEAL;
  if(d=="weather")return t.state=="sunny"?AMBER:t.state=="clear-night"?DEEP_PURPLE:LIGHT_BLUE;
  if(d=="sun")return t.state=="above_horizon"?ORANGE:DEEP_PURPLE;
  if(d=="timer")return t.state=="active"?TEAL:t.state=="paused"?ORANGE:GREY;
  if(d=="person")return t.state=="home"?GREEN:GREY;
  if(d=="screen")return BLUE;
  if(d=="sensor"){
    if(t.unit=="lx")return AMBER;
    if(t.unit=="°C" || t.unit=="°F")return DEEP_ORANGE;
    if(t.unit=="kWh" || t.unit=="Wh")return PURPLE;
    if(t.unit=="%")return TEAL;
  }
  return BLUE;
}
// Custom cards draw into a transparent `extra` container; parts are rebuilt only
// when a slot changes mode, so paging keeps RAM use flat on the CYD.
// A slot whose next card draws no custom part only hides them: paging back to the clock or
// graph in that slot reuses its objects instead of creating them again (tens of ms per card).
inline void hide_extra(Widgets &w) {
  if(!w.extra)return;
  if(!lv_obj_has_flag(w.extra,LV_OBJ_FLAG_HIDDEN))lv_obj_add_flag(w.extra,LV_OBJ_FLAG_HIDDEN);
  w.fill_points=nullptr;w.fill_count=0;
}
inline void end_extra(Widgets &w) {
  if(!w.extra)return;
  hide_extra(w);
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
  if(w.extra_mode!=mode || w.extra_full!=w.full){end_extra(w);w.extra_mode=mode;w.extra_full=w.full;w.points=new lv_point_precise_t[POINT_BUFFER];w.cached_active=-1;}
  w.fill_points=nullptr;w.fill_count=0;
  // Parts that were hidden kept the colours of the card they last showed.
  if(lv_obj_has_flag(w.extra,LV_OBJ_FLAG_HIDDEN)){w.cached_active=-1;lv_obj_remove_flag(w.extra,LV_OBJ_FLAG_HIDDEN);}
  lv_obj_set_pos(w.extra,0,0);lv_obj_set_size(w.extra,width,height);
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
// 15-17 text, 18 the second hand (points 28-29), shown while the screen is awake.
inline void second_hand(Widgets &w,const esphome::ESPTime &now) {
  float a=(now.is_valid()?now.second:0)*3.14159265f/30;
  w.points[28]={(lv_value_precise_t)(w.hand_cx-w.hand_r*0.2f*sinf(a)),(lv_value_precise_t)(w.hand_cy+w.hand_r*0.2f*cosf(a))};
  w.points[29]={(lv_value_precise_t)(w.hand_cx+w.hand_r*0.92f*sinf(a)),(lv_value_precise_t)(w.hand_cy-w.hand_r*0.92f*cosf(a))};
  auto *p=part_line(w,18,w.points+28,2,w.hand_width);
  set_hidden(p,!(awake() && now.is_valid()));
}
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
  // A full-page card centres its dial; the digital time and date beside a wide dial stay off it.
  int dial=std::min(height,width),cx=(w.full?(width-dial)/2:0)+dial/2,cy=height/2,outer=dial/2-1,radius=dial/2-(large?4:2);
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
  // A thin red second hand with a short tail, under the centre dot; tick() moves it every second.
  w.hand_cx=cx;w.hand_cy=cy;w.hand_r=radius;w.hand_width=large?2:1;
  bool new_hand=!w.parts[18];
  second_hand(w,now);
  if(new_hand)lv_obj_move_to_index(w.parts[18],lv_obj_get_index(w.parts[14]));
  std::string day=now.is_valid()?std::to_string(now.day_of_month):"--";
  if(w.full){
    part_label(w,15,big,0,0,1,LV_TEXT_ALIGN_CENTER,"");
    part_label(w,16,small,0,0,1,LV_TEXT_ALIGN_CENTER,"");
    return;
  }
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
inline int block_min(int icon_h,int temp_h,int text_h){return std::max(icon_h,temp_h)+2+text_h;}
// Current conditions on the left, five day columns on the right (wide cards only).
inline void render_forecast(Widgets &w,const Tile &t,bool large,int width,int height) {
  begin_extra(w,"forecast",width,height);
  const lv_font_t *title_font=lv_obj_get_style_text_font(w.title,LV_PART_MAIN);
  const lv_font_t *temp_font=watch_value_font?watch_value_font:w.value_font,*day_icon=mini_icon_font?mini_icon_font:w.icon_font;
  int left=large?150:96,icon_h=lv_font_get_line_height(w.icon_font),temp_h=lv_font_get_line_height(temp_font),text_h=lv_font_get_line_height(w.value_font);
  // A full-page card (firmware 0.2.62+) adds the next hours under the days: time, icon and temperature per column.
  int day_h=lv_font_get_line_height(title_font),icon_col=lv_font_get_line_height(day_icon);
  unsigned hours=w.full?std::min<size_t>(t.extra().hours.size(),large?6:4):0;
  int hours_h=hours?day_h+icon_col+text_h:0;
  int top_h=hours?std::max(block_min(icon_h,temp_h,text_h),height-hours_h-(large?12:6)):height;
  for(unsigned j=0;j<6;++j){
    if(j<hours)continue;
    for(unsigned k=18+3*j;k<21+3*j;++k)if(w.parts[k])lv_obj_add_flag(w.parts[k],LV_OBJ_FLAG_HIDDEN);
  }
  if(hours){
    int column=width/hours,y0=height-hours_h;
    for(unsigned j=0;j<hours;++j){
      const auto &h=t.extra().hours[j];int x=j*column;
      char temp[16];if(std::isfinite(h.temp))snprintf(temp,sizeof(temp),"%.0f°",h.temp);else temp[0]=0;
      for(unsigned k=18+3*j;k<21+3*j;++k)if(w.parts[k])lv_obj_remove_flag(w.parts[k],LV_OBJ_FLAG_HIDDEN);
      part_label(w,18+3*j,day_icon,x,y0+day_h,column,LV_TEXT_ALIGN_CENTER,weather_icon(h.condition));
      part_label(w,19+3*j,w.value_font,x,y0+day_h+icon_col,column,LV_TEXT_ALIGN_CENTER,temp);
      part_label(w,20+3*j,title_font,x,y0,column,LV_TEXT_ALIGN_CENTER,h.time);
    }
  }
  height=top_h;
  int block=std::max(icon_h,temp_h)+2+text_h,y=std::max(0,(height-block)/2);
  char b[24];snprintf(b,sizeof(b),"%.0f°",t.current);
  auto *icon=part_label(w,0,w.icon_font,0,y+(std::max(icon_h,temp_h)-icon_h)/2,icon_h+4,LV_TEXT_ALIGN_LEFT,t.available()?weather_icon(t.state):"\U000F0595");
  lv_obj_set_width(icon,lv_font_get_line_height(w.icon_font)+4);
  part_label(w,1,temp_font,icon_h+6,y+(std::max(icon_h,temp_h)-temp_h)/2,left-icon_h-6,LV_TEXT_ALIGN_LEFT,std::isfinite(t.current)?b:"");
  part_label(w,2,w.value_font,0,y+std::max(icon_h,temp_h)+2,left-4,LV_TEXT_ALIGN_LEFT,weather_text(t.state));
  int column=(width-left)/5;
  for(unsigned k=0;k<5;++k){
    static const Forecast no_day;int x=left+k*column;bool has=k<t.extra().forecast.size();const auto &f=has?t.extra().forecast[k]:no_day;
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
  for(unsigned i=0;i<t.history.size() && i<24;++i){
    if(!std::isfinite(t.history[i]))continue;
    float level=maximum>minimum?(t.history[i]-minimum)/(maximum-minimum):0.5f;
    raw[n++]={(lv_value_precise_t)(stroke/2+i*(width-stroke-1)/23),(lv_value_precise_t)(height-stroke/2-1-level*(height-stroke-1-top))};
  }
  if(n==1){raw[1]=raw[0];raw[1].x=(lv_value_precise_t)(width-1);n=2;}
  unsigned count=smooth(raw,n,w.points,POINT_BUFFER,width,height);
  part_line(w,0,w.points,count,stroke,x,y);
  w.fill_points=w.points;w.fill_count=count;w.fill_x=x;w.fill_y=y;w.fill_base=height;w.fill_opa=theme::fill_opacity();
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
  part_label(w,0,title_font,0,0,width,LV_TEXT_ALIGN_LEFT,t.name.empty()?"Sun":t.name);
  part_label(w,1,w.value_font,0,horizon+(large?3:1),width/2,LV_TEXT_ALIGN_LEFT,"rise "+t.extra().sunrise);
  part_label(w,2,w.value_font,width/2,horizon+(large?3:1),width/2,LV_TEXT_ALIGN_RIGHT,"set "+t.extra().sunset);
  auto now=now_time?now_time():esphome::ESPTime{};
  int rise=minutes_of(t.extra().sunrise),set=minutes_of(t.extra().sunset),minute=now.is_valid()?now.hour*60+now.minute:-1;
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
  const uint32_t path=theme::hex(theme::SUN_PATH), accent=day?theme::ha::ORANGE:theme::foreground(theme::ha::NIGHT_SKY), disc=day?theme::ha::SUNNY:theme::hex(theme::MOON);
  lv_obj_set_style_line_color(part_line(w,3,line,2,2),lv_color_hex(path),0);
  lv_obj_set_style_line_color(part_line(w,4,arc,segments+1,large?2:1),lv_color_hex(path),0);
  lv_obj_set_style_line_color(part_line(w,5,travelled,filled+2,large?4:3),lv_color_hex(accent),0);
  int size=large?18:10,glow=size+(large?12:6);
  auto *halo=part_dot(w,6,int(sun.x)-glow/2,int(sun.y)-glow/2,glow);lv_obj_set_style_bg_color(halo,lv_color_hex(disc),0);lv_obj_set_style_bg_opa(halo,LV_OPA_30,0);
  lv_obj_set_style_bg_color(part_dot(w,7,int(sun.x)-size/2,int(sun.y)-size/2,size),lv_color_hex(disc),0);
  if(day){w.fill_points=travelled;w.fill_count=filled+2;w.fill_x=0;w.fill_y=0;w.fill_base=horizon;w.fill_color=lv_color_hex(theme::ha::SUNNY);w.fill_opa=theme::fill_opacity();}
}

// ---- Direct controls on wide cards (Home Assistant entity-row style) ----
struct PanelMetrics { int key_w, key_h, radius, gap, pill_w, pill_key, slider_w, slider_h, toggle_w, toggle_h, run_pad, text_gap, ext; };
inline PanelMetrics panel_metrics(bool large) {
  return large ? PanelMetrics{60,46,14,8,196,52,140,44,76,40,22,8,4} : PanelMetrics{40,34,9,4,128,36,90,30,48,26,14,6,6};
}
// The same controls at the bottom of a full-page card (firmware 0.2.62+): keys a thumb finds without looking.
inline PanelMetrics panel_metrics_full(bool big) {
  return big ? PanelMetrics{120,84,24,16,300,84,400,56,120,60,30,8,4} : PanelMetrics{80,44,12,8,200,48,260,34,76,40,18,6,4};
}
inline lv_obj_t *panel_obj(lv_obj_t *parent,bool clickable) {
  auto *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
  if(clickable)lv_obj_add_flag(o,LV_OBJ_FLAG_CLICKABLE);else lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);
  return o;
}
// Like the custom parts: a card without controls hides the slot's panel and a later card with
// the same control set shows it again.
inline void hide_panel(Widgets &w) {
  if(!w.panel)return;
  if(!lv_obj_has_flag(w.panel,LV_OBJ_FLAG_HIDDEN))lv_obj_add_flag(w.panel,LV_OBJ_FLAG_HIDDEN);
  w.panel_w=0;
  if(captured_slider && captured_slider==w.control_slider)captured_slider=nullptr;
}
inline void end_panel(Widgets &w) {
  if(!w.panel)return;
  hide_panel(w);
  if(!w.panel_mode.empty()){
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
  const PanelMetrics m=w.full?panel_metrics_full(w.base_height>80):panel_metrics(large);
  const lv_font_t *icon_font=w.full?w.icon_font:mini_icon_font?mini_icon_font:w.icon_font;
  const lv_font_t *text_font=control_font?control_font:lv_obj_get_style_text_font(w.title,LV_PART_MAIN);
  auto d=t.domain();
  if(!w.panel){w.panel=panel_obj(w.tile,false);}
  if(w.panel_mode!=mode || w.panel_full!=w.full){
    end_panel(w);w.panel_mode=mode;w.panel_full=w.full;w.panel_dirty=true;
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
      lv_obj_set_size(slider,mode=="volume"?m.slider_w:m.pill_w,h);
      lv_obj_set_style_pad_all(slider,0,LV_PART_MAIN);
      lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_INDICATOR);
      // The handle is a short white bar inside the fill, like Home Assistant's slider.
      lv_obj_set_style_radius(slider,2,LV_PART_KNOB);lv_obj_add_style(slider,theme::style(theme::Paint::knob),LV_PART_KNOB);lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB);
      slider_handle(slider,mode=="volume"?m.slider_w:m.pill_w,h);
      lv_obj_set_style_border_width(slider,0,LV_PART_KNOB);lv_obj_set_style_shadow_width(slider,0,LV_PART_KNOB);
      lv_obj_set_ext_click_area(slider,m.ext+2);
      lv_obj_add_event_cb(slider,slider_event,LV_EVENT_ALL,(void*)(uintptr_t)w.index);
      if(mode=="volume"){panel_key(w,0,w.panel,m,m.key_h,m.key_h,false);panel_icon(w,0,icon_font);w.key_commands[0]=tile_controls::MEDIA_MUTE;}
    }else if(mode=="toggle"){
      panel_key(w,0,w.panel,m,m.toggle_w,m.toggle_h,false);lv_obj_set_style_radius(w.keys[0],LV_RADIUS_CIRCLE,0);
      w.knob=panel_obj(w.keys[0],false);lv_obj_set_size(w.knob,m.toggle_h-8,m.toggle_h-8);lv_obj_set_y(w.knob,4);
      lv_obj_set_style_radius(w.knob,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_opa(w.knob,LV_OPA_COVER,0);lv_obj_add_style(w.knob,theme::style(theme::Paint::knob),0);
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
    if(w.key_icons[n])lv_obj_set_style_text_color(w.key_icons[n],checked?theme::color(theme::ON_ACCENT):w.panel_text,0);
  };
  auto set_disabled=[&](unsigned n,bool disabled){if(disabled)lv_obj_add_state(w.keys[n],LV_STATE_DISABLED);else lv_obj_remove_state(w.keys[n],LV_STATE_DISABLED);};
  if(tile_controls::is_key_row(mode)){
    std::array<tile_controls::Key,3> keys;unsigned count=tile_controls::keys_for(t,keys);
    for(unsigned n=0;n<3;++n){
      if(n>=count){lv_obj_add_flag(w.keys[n],LV_OBJ_FLAG_HIDDEN);w.key_commands[n]=tile_controls::NONE;continue;}
      lv_obj_remove_flag(w.keys[n],LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(w.keys[n],n*(m.key_w+m.gap),0);
      label(w.key_icons[n],keys[n].icon);w.key_commands[n]=keys[n].command;w.key_args[n]=keys[n].arg;
      // The active mode key carries the accent; "off" stays neutral grey.
      if(keys[n].checked && w.key_checked[n]!=1)lv_obj_set_style_bg_color(w.keys[n],keys[n].arg=="off"?theme::color(theme::OFF):w.panel_accent,LV_STATE_CHECKED);
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
    // The requested width: a key created in this pass has no coordinates before LVGL's layout.
    panel_w=lv_obj_get_style_width(w.keys[0],LV_PART_MAIN);
  }
  (void)now;
  if(!panel_w){hide_panel(w);return 0;}
  // A panel shown again kept the colours of the card it last served.
  if(lv_obj_has_flag(w.panel,LV_OBJ_FLAG_HIDDEN)){lv_obj_remove_flag(w.panel,LV_OBJ_FLAG_HIDDEN);w.panel_dirty=true;}
  lv_obj_set_size(w.panel,panel_w,panel_h);
  if(w.full)lv_obj_set_pos(w.panel,std::max(0,(content_w-panel_w)/2),std::max(0,content_h-panel_h));
  else lv_obj_set_pos(w.panel,content_w-panel_w,std::max(0,(content_h-panel_h)/2));
  w.panel_w=panel_w+m.text_gap;
  return w.panel_w;
}
// Colours follow the card palette; called with the rest of the palette when it changes.
inline void style_panel(Widgets &w,const Tile &t,lv_color_t accent,lv_color_t text) {
  if(!w.panel || w.panel_mode.empty())return;
  const uint32_t surface=theme::surface(t.background);
  lv_color_t card=lv_color_hex(surface);
  lv_color_t key_bg=lv_color_hex(theme::key_on(surface,236));
  lv_color_t key_pressed=lv_color_hex(theme::key_on(surface,212));
  w.panel_accent=accent;w.panel_text=text;
  for(unsigned n=0;n<3;++n){
    auto *key=w.keys[n];if(!key)continue;
    bool in_pill=w.pill && lv_obj_get_parent(key)==w.pill;
    set_color(key,LV_STYLE_BG_COLOR,in_pill?key_pressed:key_bg);
    set_color(key,LV_STYLE_BG_COLOR,in_pill?lv_color_hex(theme::key_on(surface,190)):key_pressed,LV_STATE_PRESSED);
    set_color(key,LV_STYLE_BG_COLOR,w.key_args[n]=="off" && w.panel_mode=="mode"?theme::color(theme::OFF):accent,LV_STATE_CHECKED);
    if(w.key_icons[n])set_color(w.key_icons[n],LV_STYLE_TEXT_COLOR,w.key_checked[n]==1?theme::color(theme::ON_ACCENT):text);
  }
  if(w.pill){set_color(w.pill,LV_STYLE_BG_COLOR,key_bg);set_color(w.pill_value,LV_STYLE_TEXT_COLOR,text);}
  if(w.control_slider){
    bool on=fresh() && t.slider_active();
    lv_color_t fill=on?accent:theme::color(theme::OFF);
    // The track is the fill colour at 20 % over the card, as in Home Assistant.
    set_color(w.control_slider,LV_STYLE_BG_COLOR,lv_color_mix(fill,card,51),LV_PART_MAIN);
    set_color(w.control_slider,LV_STYLE_BG_COLOR,fill,LV_PART_INDICATOR);
    slider_bar(w.control_slider,slider_bar_shown(t,on));
  }
  if(w.knob){set_color(w.keys[0],LV_STYLE_BG_COLOR,theme::color(theme::PANEL_TOGGLE_OFF));set_color(w.keys[0],LV_STYLE_BG_COLOR,theme::color(theme::PANEL_TOGGLE_OFF_PRESSED),LV_STATE_PRESSED);}
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
  else if(step){ if(!cyd::touch_guard.accept_repeat(now,400+slot*16+n)){ESP_LOGI("touch","tap on control %u ignored: %s",(unsigned)slot,cyd::touch_guard.reason().c_str());return;} }
  else if(!allowed(now,400+slot*16+n,"control "+std::to_string(slot)))return;
  if(!t.available())return;
  if(step){
    // Local at once, tap after tap; tick() sends the last value after a short pause.
    float current=std::isfinite(t.edit_value)?t.edit_value:tile_controls::edit_target(t);
    t.edit_value=tile_controls::step_value(current,tile_controls::edit_step(t),t.minimum,t.maximum,command==tile_controls::STEP_UP?1:-1);
    t.edit_since=now;t.edit_sent=false;
    if(w.pill_value){std::string suffix=t.domain()=="climate"?"°":t.unit.empty()?"":" "+t.unit;label(w.pill_value,tile_controls::format_value(t.edit_value,tile_controls::edit_step(t),suffix.c_str()));}
    return;
  }
  if(t.waiting(now))return;
  if(command==tile_controls::TOGGLE)t.optimistic(t.state!="on");
  auto a=tile_controls::key_action(t,command,w.key_args[n]);
  if(a.valid())action(a.service,t.entity,a.key,a.value);
}
// A busy card is covered by a translucent white sheet with a small spinner until
// Home Assistant confirms; the sheet also swallows taps meanwhile.
inline void set_busy(Widgets &w,bool busy,bool large){
  if(!busy){if(w.busy)lv_obj_add_flag(w.busy,LV_OBJ_FLAG_HIDDEN);return;}
  if(!w.busy){
    w.busy=lv_obj_create(w.tile);lv_obj_remove_style_all(w.busy);lv_obj_remove_flag(w.busy,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(w.busy,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(w.busy,theme::style(theme::Paint::veil),0);lv_obj_set_style_bg_opa(w.busy,LV_OPA_60,0);
    lv_obj_set_style_radius(w.busy,lv_obj_get_style_radius(w.tile,LV_PART_MAIN),0);
#if LV_USE_SPINNER
    w.spinner=lv_spinner_create(w.busy);lv_spinner_set_anim_params(w.spinner,900,200);
    int size=large?30:20;lv_obj_set_size(w.spinner,size,size);lv_obj_center(w.spinner);lv_obj_remove_flag(w.spinner,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(w.spinner,large?4:3,LV_PART_MAIN);lv_obj_set_style_arc_width(w.spinner,large?4:3,LV_PART_INDICATOR);
    lv_obj_add_style(w.spinner,theme::style(theme::Paint::spinner),LV_PART_MAIN);lv_obj_set_style_arc_color(w.spinner,lv_color_hex(theme::ha::RAIN),LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(w.spinner,LV_OPA_TRANSP,LV_PART_KNOB);lv_obj_set_style_pad_all(w.spinner,0,LV_PART_KNOB);
#endif
  }
  // Cover the whole card, padding included, at the width the card asks for: a slot that just turned
  // wide is still single width in LVGL's own coordinates. Unchanged sizes cost LVGL nothing.
  lv_obj_set_pos(w.busy,-lv_obj_get_style_pad_left(w.tile,LV_PART_MAIN),-lv_obj_get_style_pad_top(w.tile,LV_PART_MAIN));
  lv_obj_set_size(w.busy,tile_width(w),tile_height(w));
  lv_obj_remove_flag(w.busy,LV_OBJ_FLAG_HIDDEN);
  // Controls and custom parts created after the sheet would otherwise paint over its right side.
  if(lv_obj_get_index(w.busy)!=(int32_t)lv_obj_get_child_count(w.tile)-1)lv_obj_move_foreground(w.busy);
}
// The card that takes a whole page (firmware 0.2.62+). Without a control it is one big button: the icon in a
// large circle with the name and the state under it, and the whole card lights up in the state colour while
// on (see the palette below), so a wall switch reads from across the room and a push anywhere works. With a
// small slider, direct controls or a graph the double-width card's head stays on top and the control takes
// a strip at the bottom, so a tap anywhere else still does what a tap on the tile does. The built-in cards
// (clock, forecast, sun path) simply get the whole page.
// A media player over the whole page (firmware 0.2.64+): the head as on every full card, and under it the media card
// itself, wide: the cover at the left, the track, the bar and the keys beside it, the volume row along the bottom.
// Parts: 0 placeholder, 1 its icon, 2 title, 3 artist, 4 track, 5 fill, 6 elapsed, 7 total, 8-10 keys, 11 mute,
// 12 slider, 13 percent, MEDIA_PICTURE (14) the cover. Built once per slot and moved on every redraw.
inline void media_tile_key_event(lv_event_t *e){
  unsigned code=(uintptr_t)lv_event_get_user_data(e);unsigned slot=code/16,n=code%16;
  if(slot>=widgets.size() || n>3)return;
  auto &w=widgets[slot];
  if(!enabled || !fresh() || w.index>=model.count || w.extra_mode!="media")return;
  if(lv_obj_has_state(lv_event_get_target_obj(e),LV_STATE_DISABLED))return;
  const uint32_t now=esphome::millis();
  if(!allowed(now,500+slot*16+n,"media key "+std::to_string(slot)))return;
  auto &t=model.tiles[w.index];
  if(!t.available() || t.waiting(now))return;
  static const int commands[]={21,20,22,23};
  media_action(t,n==1 && media_off(t)?24:commands[n]);
}
inline void render_media_full(Widgets &w,const Tile &t,bool big,int content_w,int content_h,int head_h){
  using namespace media_card;
  using namespace tile_controls;
  const auto &x=t.extra();
  const Metrics m=media_metrics(big);
  const int top=head_h+(big?8:4);
  const Layout l=layout(m,content_w,std::max(40,content_h-top-(big?4:2)));
  begin_extra(w,"media",content_w,content_h);
  auto at=[&](Rect r){r.y+=top;return r;};
  const bool usable=fresh()&&t.available(),track=usable&&has_track(t.state),play=media_card::playing(t.state);
  const uint32_t f=t.supported;auto can=[&](uint32_t bit){return usable&&(!f||(f&bit));};
  const size_t slot=&w-widgets.data();
  // The placeholder and the player's icon; the cover comes over them once the app served it.
  w.parts[0]=media_box(w.extra,w.parts[0],at(l.art),theme::tint(theme::ha::LIGHT_BLUE,51),l.art_radius);
  const std::string glyph=icon_for(t);
  const lv_font_t *placeholder_font=big_icon_font&&font_has(big_icon_font,glyph)?big_icon_font:w.icon_font;
  if(!w.parts[1]){w.parts[1]=lv_label_create(w.parts[0]);lv_obj_remove_flag(w.parts[1],LV_OBJ_FLAG_CLICKABLE);}
  set_font(w.parts[1],placeholder_font);set_color(w.parts[1],LV_STYLE_TEXT_COLOR,theme::rgb(theme::icon(theme::ha::LIGHT_BLUE)));label(w.parts[1],glyph);lv_obj_center(w.parts[1]);
  const uint32_t ground=theme::of(lv_obj_get_style_bg_color(w.tile,LV_PART_MAIN));
  lv_image_dsc_t *src=nullptr;
  // A card open over the page owns the cover then; the tile asks again once the card closes (cover_tick).
  const bool card_open=detail_root && !lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN);
  if(camera_supported()&&track&&!x.media_picture.empty()&&!card_open){cover_want(t.entity,x.media_picture,l.art.w,ground,CoverOwner::TILE,slot);src=cover_ready(t.entity,l.art.w,ground);}
  if(src)w.parts[MEDIA_PICTURE]=media_picture_show(w.extra,w.parts[MEDIA_PICTURE],at(l.art),src);
  else if(w.parts[MEDIA_PICTURE]){lv_obj_delete(w.parts[MEDIA_PICTURE]);w.parts[MEDIA_PICTURE]=nullptr;}
  // Title, artist · album, left-aligned beside the cover.
  const lv_font_t *title_font=watch_font?watch_font:w.title_font,*artist_font=control_font?control_font:w.title_font,*small=small_font?small_font:w.title_font;
  auto text=[&](unsigned i,const lv_font_t *font,const Rect &r,lv_text_align_t align,const std::string &value,theme::Role role){
    auto *p=part_label(w,i,font,r.x,r.y+top,r.w,align,value);lv_label_set_long_mode(p,LV_LABEL_LONG_DOT);set_color(p,LV_STYLE_TEXT_COLOR,theme::color(role));lv_obj_remove_flag(p,LV_OBJ_FLAG_HIDDEN);return p;
  };
  text(2,title_font,l.title,LV_TEXT_ALIGN_LEFT,track&&!x.media_title.empty()?x.media_title:std::string(idle_text(usable?t.state:"unavailable")),theme::INK);
  if(l.artist)text(3,artist_font,l.artist_line,LV_TEXT_ALIGN_LEFT,track?subtitle(x.media_artist,x.media_album):std::string(),theme::MUTED);
  else if(w.parts[3])lv_obj_add_flag(w.parts[3],LV_OBJ_FLAG_HIDDEN);
  // The progress bar and its times; a stream without a length has none.
  w.media_bar_w=l.bar.w;
  const bool timed=track&&x.media_duration;
  if(timed){
    w.parts[4]=media_box(w.extra,w.parts[4],at(l.bar),theme::hex(theme::TRACK),LV_RADIUS_CIRCLE);lv_obj_remove_flag(w.parts[4],LV_OBJ_FLAG_HIDDEN);
    Rect fill=at(l.bar);fill.w=std::max(l.bar.h,l.bar.w*std::max(0,progress(x.media_position,x.media_position_at,now_epoch(),play,x.media_duration))/1000);
    w.parts[5]=media_box(w.extra,w.parts[5],fill,media_accent(),LV_RADIUS_CIRCLE);lv_obj_remove_flag(w.parts[5],LV_OBJ_FLAG_HIDDEN);
  }else for(unsigned i:{4u,5u})if(w.parts[i])lv_obj_add_flag(w.parts[i],LV_OBJ_FLAG_HIDDEN);
  if(timed&&l.times){
    text(6,small,l.elapsed,LV_TEXT_ALIGN_LEFT,clock_text(elapsed_seconds(x.media_position,x.media_position_at,now_epoch(),play,x.media_duration)),theme::SUBTLE);
    text(7,small,l.total,LV_TEXT_ALIGN_RIGHT,clock_text(x.media_duration),theme::SUBTLE);
  }else for(unsigned i:{6u,7u})if(w.parts[i])lv_obj_add_flag(w.parts[i],LV_OBJ_FLAG_HIDDEN);
  // The keys and the volume row. Their events carry the slot: the tile in it may change with the page. An off player
  // shows one power key and no volume row.
  const lv_font_t *key_font=mini_icon_font?mini_icon_font:w.icon_font;
  auto user=[&](unsigned n){return (void*)(uintptr_t)(slot*16+n);};
  auto show=[&](unsigned i,bool on){if(w.parts[i]){if(on)lv_obj_remove_flag(w.parts[i],LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(w.parts[i],LV_OBJ_FLAG_HIDDEN);}};
  const bool off=usable && media_off(t), volume=!off && std::isfinite(t.volume);
  if(off){
    w.parts[9]=media_key(w.extra,w.parts[9],at(l.play),glyph::POWER,w.icon_font,true,false,can(feature::MEDIA_TURN_ON),media_tile_key_event,user(1));
    show(9,can(feature::MEDIA_TURN_ON));
  }else{
    w.parts[8]=media_key(w.extra,w.parts[8],at(l.prev),glyph::PREVIOUS,key_font,false,false,can(feature::MEDIA_PREVIOUS),media_tile_key_event,user(0));
    w.parts[9]=media_key(w.extra,w.parts[9],at(l.play),play?glyph::PAUSE:glyph::PLAY,w.icon_font,true,false,can(feature::MEDIA_PLAY|feature::MEDIA_PAUSE),media_tile_key_event,user(1));
    w.parts[10]=media_key(w.extra,w.parts[10],at(l.next),glyph::NEXT,key_font,false,false,can(feature::MEDIA_NEXT),media_tile_key_event,user(2));
    show(9,true);
  }
  show(8,!off);show(10,!off);
  if(volume){
    w.parts[11]=media_key(w.extra,w.parts[11],at(l.mute),t.muted?glyph::MUTED:glyph::VOLUME,key_font,false,true,can(feature::MEDIA_VOLUME_MUTE),media_tile_key_event,user(3));
    w.parts[12]=media_slider(w.extra,w.parts[12],at(l.volume),t,big,can(feature::MEDIA_VOLUME_SET),(void*)(uintptr_t)w.index);
    text(13,small,l.percent,LV_TEXT_ALIGN_RIGHT,media_volume_text(t),theme::MUTED);
  }
  show(11,volume);show(12,volume);show(13,volume);
}
inline void render_full(Widgets &w,const Tile &t,bool custom,bool clock,bool sunpath,bool graph,bool mini,bool with_panel,bool large,
                        const std::string &value,const std::string &unit,int content_w,int content_h) {
  const bool big=w.base_height>80;  // the board: a Guition card is large, a CYD card small
  lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(w.progress,LV_OBJ_FLAG_HIDDEN);
  if(custom){
    lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);hide_panel(w);
    // The forecast and the sun path keep their board's layout (the CYD's two-row days); a taller dial is fine.
    if(clock)render_clock(w,t,large,content_w,content_h);
    else if(sunpath)render_sunpath(w,t,big,content_w,content_h);
    else render_forecast(w,t,big,content_w,content_h);
    return;
  }
  int value_h=lv_font_get_line_height(lv_obj_get_style_text_font(w.value,LV_PART_MAIN));
  int gap=big?12:6;
  // A media player that answers gets the media card under its head (firmware 0.2.64+); unavailable, it is the plain card.
  const bool media=t.domain()=="media_player" && fresh() && t.available();
  if(!mini && !with_panel && !graph && !media){
    hide_extra(w);hide_panel(w);lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
    const lv_font_t *name_font=room_label?lv_obj_get_style_text_font(room_label,LV_PART_MAIN):w.title_font;
    int name_h=lv_font_get_line_height(name_font),circle=big?128:64;
    int block=circle+gap+name_h+2+value_h,top=std::max(0,(content_h-block)/2);
    lv_obj_set_size(w.circle,circle,circle);lv_obj_set_pos(w.circle,std::max(0,(content_w-circle)/2),top);
    // The big icon font carries the domain icons; another chosen icon keeps its usual size in the big circle.
    const lv_font_t *icon_font=big_icon_font && font_has(big_icon_font,icon_for(t))?big_icon_font:w.icon_font;
    if(lv_obj_get_style_text_font(w.icon,LV_PART_MAIN)!=icon_font){set_font(w.icon,icon_font);lv_obj_center(w.icon);}
    set_font(w.title,name_font);set_text_align(w.title,LV_TEXT_ALIGN_CENTER);set_text_align(w.value,LV_TEXT_ALIGN_CENTER);
    lv_obj_set_height(w.title,name_h);
    lv_obj_set_pos(w.title,0,top+circle+gap);lv_obj_set_width(w.title,content_w);
    lv_obj_set_pos(w.value,0,top+circle+gap+name_h+2);lv_obj_set_width(w.value,content_w);
    if(!unit.empty())label(w.value,value+" "+unit);
    return;
  }
  // The head as on the double-width card: the circle at the left, name and state beside it.
  set_font(w.title,w.title_font);set_text_align(w.title,LV_TEXT_ALIGN_LEFT);set_text_align(w.value,LV_TEXT_ALIGN_LEFT);
  int title_h=lv_font_get_line_height(w.title_font);
  lv_obj_set_height(w.title,title_h);
  int head_h=std::max<int>(1,w.base_height-lv_obj_get_style_space_top(w.tile,LV_PART_MAIN)-lv_obj_get_style_space_bottom(w.tile,LV_PART_MAIN));
  int circle=big?54:36,line_gap=big?2:1,text_y=std::max(0,(head_h-(title_h+line_gap+value_h))/2);
  lv_obj_set_size(w.circle,circle,circle);
  if(lv_obj_get_style_text_font(w.icon,LV_PART_MAIN)!=w.icon_font){set_font(w.icon,w.icon_font);lv_obj_center(w.icon);}
  lv_obj_set_pos(w.circle,0,big?12:std::max(0,(head_h-circle)/2));
  lv_obj_set_pos(w.title,w.title_x,big?w.title_y:text_y);
  lv_obj_set_pos(w.value,w.value_x,big?w.value_y:text_y+title_h+line_gap);
  lv_obj_set_width(w.title,std::max(1,content_w-w.title_x));lv_obj_set_width(w.value,std::max(1,content_w-w.value_x));
  if(media){
    hide_panel(w);lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
    render_media_full(w,t,big,content_w,content_h,head_h);
    return;
  }
  if(mini){
    // The small slider becomes a strip a thumb finds at the bottom; the room above it is the button.
    hide_extra(w);hide_panel(w);
    int strip=big?64:40;
    lv_obj_set_size(w.slider,content_w,strip);lv_obj_set_ext_click_area(w.slider,slider_zone(w,strip));
    slider_handle(w.slider,content_w,strip);
    lv_obj_remove_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
    if(!lv_obj_has_state(w.slider,LV_STATE_PRESSED) && lv_slider_get_value(w.slider)!=slider_value(t))lv_slider_set_value(w.slider,slider_value(t),LV_ANIM_OFF);
    return;
  }
  lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
  if(graph){
    hide_panel(w);
    render_graph(w,t,large,0,head_h+gap,content_w,std::max(8,content_h-head_h-gap));
    return;
  }
  // Direct controls at the bottom, centred; the room between shows what the double-width card had no room for.
  hide_extra(w);
  if(!layout_panel(w,t,large,content_w,content_h)){hide_panel(w);return;}
  int panel_h=lv_obj_get_style_height(w.panel,LV_PART_MAIN);
  int mid_top=head_h,mid_h=content_h-head_h-gap-panel_h;
  // The large value font carries digits, the degree sign and the percent sign; the clock font only digits and a colon.
  std::string middle;const lv_font_t *mid_font=watch_value_font?watch_value_font:w.value_font;
  auto d=t.domain();
  if(d=="climate" && std::isfinite(t.current)){char b[32];snprintf(b,sizeof(b),"%.1f°",t.current);middle=b;}
  else if(d=="cover" && std::isfinite(t.position)){middle=std::to_string(static_cast<int>(std::lround(t.position)))+" %";}
  else if(d=="media_player" && !t.extra().media_title.empty()){middle=t.extra().media_title;mid_font=room_label?lv_obj_get_style_text_font(room_label,LV_PART_MAIN):w.title_font;}
  int mid_font_h=lv_font_get_line_height(mid_font);
  if(middle.empty() || mid_h<mid_font_h)return;
  set_font(w.unit,mid_font);set_text_align(w.unit,LV_TEXT_ALIGN_CENTER);label(w.unit,middle);
  lv_obj_set_pos(w.unit,0,mid_top+std::max(0,(mid_h-mid_font_h)/2));lv_obj_set_size(w.unit,content_w,mid_font_h);
  lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
}
// One card: name, status, icon, layout and palette. Everything reads the model; nothing is created
// unless the card's custom parts or controls change kind.
inline void render_slot(size_t slot) {
  swipe_profile::SlotTimer slot_timer(slot);
  swipe_profile::Lap lap;
  auto &w=widgets[slot];
  if(!w.tile || w.index>=model.count)return;
  const auto &t = model.tiles[w.index];
  auto d=t.domain();
  label(w.title, t.name.empty() ? t.entity : t.name);
  label(w.icon, icon_for(t));
  bool watch=t.display=="watch";
  std::string unit=watch?t.unit:"";
  std::string value = t.state;
  // Nothing in Home Assistant stands behind the settings card, so it says the same with the link down.
  if (t.is_settings()) value = "Tap to open";
  else if (t.is_page()) value = "Page " + std::to_string(t.page_target());
  else if (!fresh() || !t.available()) value = "Unavailable";
  else if (t.refused_at && esphome::millis() - t.refused_at < 4000) value = "Refused";
  else if (d == "light" && t.state == "on" && std::isfinite(t.brightness)) value = std::to_string(static_cast<int>(std::lround(std::clamp(t.brightness, 0.0f, 255.0f) * 100 / 255))) + " %";
  else if (d == "climate" && std::isfinite(t.target)) { char b[32]; snprintf(b, sizeof(b), "%.1f°", t.target); value = b; }
  else if (d == "person") value = t.state=="home"?"Home":t.state=="not_home"?"Away":t.state;
  else if (d == "sun") value = !t.extra().sunrise.empty() && !t.extra().sunset.empty() ? t.extra().sunrise+" - "+t.extra().sunset : t.state=="above_horizon"?"Above the horizon":"Below the horizon";
  else if (d == "timer") value = timer_text(t);
  else if (d == "script" || d == "scene" || d == "button" || d == "input_button") value = t.state == "on" ? "Running..." : last_run_text(t.last_run);
  else if (d == "camera") value = t.state == "streaming" ? "Live" : t.state == "recording" ? "Recording" : "Tap to view";
  else if (d == "image") value = t.last_run ? "Tap to view" : "No image yet";
  else if (d == "binary_sensor" && (value == "on" || value == "off")) value = tile_controls::binary_state_text(t.device_class, value == "on");
  else if (value == "on") value = "On";
  else if (value == "off") value = "Off";
  else if (value == "cleaning") value = "Cleaning";
  else if (value == "docked") value = "Docked";
  // Home Assistant's word where the screen has none of its own (firmware 0.2.58+): a cover says Open, a washer Rinsing.
  else if (!t.extra().state_word.empty()) value = t.extra().state_word;
  // A player's state in the screen's own words where Home Assistant sent none (firmware 0.2.64+).
  else if (d == "media_player") value = tile_controls::media_state_text(t.state);
  else if (!t.unit.empty() && !watch) value += " " + t.unit;
  bool pending=t.loading(esphome::millis());
  if(d=="weather" && std::isfinite(t.current)) {char b[32];snprintf(b,sizeof(b),"%.1f %s",t.current,t.unit.c_str());value=b;if(watch){snprintf(b,sizeof(b),"%.1f",t.current);value=b;}}
  if(d=="vacuum" && std::isfinite(t.battery))value += " / "+std::to_string((int)t.battery)+"%";
  // Direct controls: only a wide card in the standard layout has room for the panel.
  bool with_panel=w.wide && !t.controls.empty() && !t.builtin() && !watch && t.inline_control!="slider" && fresh() && t.available();
  if(with_panel){std::string status=tile_controls::status_text(t);if(!status.empty())value=status;}
  label(w.value, value);
  bool mini=t.inline_control=="slider" && !watch && t.available();
  bool large_tile=tile_height(w)>80;
  lap(swipe_profile::TEXT);
  // Cards that replace the name/status layout entirely.
  bool clock=t.is_clock(), forecast=d=="weather" && t.display=="forecast" && w.wide && t.extra().forecast.size()>0 && fresh() && t.available();
  bool sunpath=d=="sun" && t.display=="sunpath" && w.wide && !t.extra().sunrise.empty() && !t.extra().sunset.empty() && fresh() && t.available();
  bool graph=d=="sensor" && t.display=="graph" && t.has_history && !clock;
  bool custom=clock||forecast||sunpath;
  if(!large_tile)pad_vertical(w.tile,watch||custom||graph?2:4);
  set_font(w.value,watch && watch_value_font ? watch_value_font : w.value_font);
  // Both text boxes are one line high; the sizes come from the styles, not from a layout pass.
  int title_height=lv_font_get_line_height(lv_obj_get_style_text_font(w.title,LV_PART_MAIN));
  int value_height=lv_font_get_line_height(lv_obj_get_style_text_font(w.value,LV_PART_MAIN));
  lv_obj_set_height(w.value,value_height);
  int content_w=content_width(w),content_h=content_height(w);
  lap(swipe_profile::LAYOUT);
  w.busy_drawn = pending && !t.builtin();
  set_busy(w,w.busy_drawn,large_tile);
  lap(swipe_profile::BUSY);
  for(auto *o:{w.title,w.value,w.circle,w.unit})set_hidden(o,custom);
  if(w.full){
    lap(swipe_profile::GEOMETRY);
    render_full(w,t,custom,clock,sunpath,graph,mini,with_panel,large_tile,value,unit,content_w,content_h);
    lap(swipe_profile::CUSTOM);
  }else if(custom){
    lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);hide_panel(w);
    lap(swipe_profile::GEOMETRY);
    if(clock)render_clock(w,t,large_tile,content_w,content_h);
    else if(sunpath)render_sunpath(w,t,large_tile,content_w,content_h);
    else render_forecast(w,t,large_tile,content_w,content_h);
    lap(swipe_profile::CUSTOM);
  }else{
  // A slot that just held a full card gets its own name font, one-line box and left-aligned text back.
  set_font(w.title,w.title_font);set_text_align(w.title,LV_TEXT_ALIGN_LEFT);set_text_align(w.value,LV_TEXT_ALIGN_LEFT);
  title_height=lv_font_get_line_height(w.title_font);lv_obj_set_height(w.title,title_height);
  int line_gap=large_tile?2:1,text_height=title_height+line_gap+value_height;
  int slider_height=large_tile?28:8;
  // A single-width graph takes the slider strip; a wide graph takes the right half.
  bool graph_strip=graph && !w.wide, graph_side=graph && w.wide;
  int chart_w=graph_side?content_w*55/100:0;
  lap(swipe_profile::GEOMETRY);
  int panel_w=with_panel && !graph?layout_panel(w,t,large_tile,content_w,content_h):0;
  if(!panel_w)hide_panel(w);
  lap(swipe_profile::PANEL);
  int header_height=(mini||graph_strip)?content_h-slider_height-(large_tile?6:3):content_h;
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
  int text_room=content_w-chart_w-(graph_side?(large_tile?10:6):0)-panel_w;
  // A navigation tile ends in a chevron, as a row in Home Assistant's settings does.
  const lv_font_t *chevron_font=mini_icon_font?mini_icon_font:w.icon_font;
  int chevron_w=t.is_page() && !watch?lv_font_get_line_height(chevron_font):0;
  if(chevron_w)text_room-=chevron_w+(large_tile?8:4);
  lv_obj_set_width(w.title,std::max(1,text_room-text_x));
  lv_obj_set_width(w.value,std::max(1,text_room-(watch?0:(mini||graph_strip)?text_x:w.value_x)));
  if(watch){
    int gap=large_tile?6:2,header=std::max(circle_size,title_height);
    int group_y=std::max(0,(content_h-header-gap-value_height)/2);
    int value_y=group_y+header+gap;
    lv_obj_set_pos(w.circle,0,group_y+(header-circle_size)/2);
    lv_obj_set_pos(w.title,circle_size+(large_tile?6:4),group_y+(header-title_height)/2);
    lv_obj_set_width(w.title,text_room-circle_size-(large_tile?6:4));
    set_font(w.unit,w.value_font);set_text_align(w.unit,LV_TEXT_ALIGN_LEFT);
    label(w.unit,unit);
    lv_point_t size;lv_text_get_size(&size,unit.c_str(),w.value_font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);
    int unit_width=unit.empty()?0:std::min((int)size.x,text_room-20);
    int number_width=text_room-(unit_width?unit_width+(large_tile?6:3):0);
    lv_obj_set_pos(w.value,0,value_y);lv_obj_set_width(w.value,number_width);
    lv_obj_set_pos(w.unit,text_room-unit_width,value_y+value_height-lv_font_get_line_height(w.value_font));
    lv_obj_set_size(w.unit,unit_width,lv_font_get_line_height(w.value_font));
    set_hidden(w.unit,!unit_width);
  }else if(chevron_w){
    set_font(w.unit,chevron_font);set_text_align(w.unit,LV_TEXT_ALIGN_RIGHT);label(w.unit,"\U000F0142");
    lv_obj_set_pos(w.unit,content_w-chevron_w,std::max(0,(header_height-chevron_w)/2));lv_obj_set_size(w.unit,chevron_w,chevron_w);
    lv_obj_remove_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
  }else lv_obj_add_flag(w.unit,LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(w.slider,content_w,slider_height);lv_obj_set_ext_click_area(w.slider,slider_zone(w,slider_height));
  slider_handle(w.slider,content_w,slider_height);
  if(mini){lv_obj_remove_flag(w.slider,LV_OBJ_FLAG_HIDDEN);if(!lv_obj_has_state(w.slider,LV_STATE_PRESSED) && lv_slider_get_value(w.slider)!=slider_value(t))lv_slider_set_value(w.slider,slider_value(t),LV_ANIM_OFF);}
  else lv_obj_add_flag(w.slider,LV_OBJ_FLAG_HIDDEN);
  lap(swipe_profile::GEOMETRY);
  if(graph_strip)render_graph(w,t,large_tile,0,content_h-slider_height,content_w,slider_height);
  else if(graph_side)render_graph(w,t,large_tile,content_w-chart_w,0,chart_w,content_h);
  else hide_extra(w);
  lap(swipe_profile::CUSTOM);
  }
  bool on = fresh() && t.active();
  // Sliders colour more states than the card itself does, such as a playing media player (Tile::slider_active).
  bool slider_on = fresh() && t.slider_active();
  bool available=fresh() && t.available();
  int palette_state=(available?2:0)|(on?1:0)|(slider_on?4:0);
  if (w.cached_active == palette_state && !w.panel_dirty) { lap(swipe_profile::GEOMETRY); return; }
  w.cached_active = palette_state;w.panel_dirty=false;
  uint32_t accent = domain_accent(t);
  // Off is grey, as in Home Assistant, so a light or a door sensor shows its state at a glance.
  uint32_t state_color=(t.is_switch()||d=="light"||d=="binary_sensor"||d=="person"||d=="timer") && !on ? theme::STATE_OFF : accent;
  if(d=="light" && on && t.has_hs_color)
    state_color=lv_color_to_u32(lv_color_hsv_to_rgb(t.hue%360,t.saturation,100))&0xFFFFFF;
  auto color=lv_color_hex(theme::state(state_color));
  auto circle_color=lv_color_hex(available?theme::tint(state_color,38):theme::hex(theme::TRACK));
  // Very pale bulbs still need a visible icon: a touch darker on a light card, a touch lighter on a dark one.
  auto icon_color=lv_color_hex(available?theme::icon(state_color):theme::hex(theme::OFF));
  // Off: a grey track without fill or handle, as in Home Assistant.
  const uint32_t fill=slider_on?state_color:theme::STATE_OFF;
  auto fill_color=lv_color_hex(theme::state(fill));
  set_color(w.slider,LV_STYLE_BG_COLOR,fill_color,LV_PART_INDICATOR);
  set_color(w.slider,LV_STYLE_BG_COLOR,lv_color_hex(theme::tint(fill,51)),LV_PART_MAIN);
  slider_bar(w.slider,slider_bar_shown(t,slider_on));
  // A full-page card lights up in its state colour while on (firmware 0.2.62+): an amber lamp, a purple scene.
  bool lit=w.full && on && available && !t.transparent;
  set_color(w.tile,LV_STYLE_BG_COLOR,lv_color_hex(lit?theme::tint(state_color,51):theme::surface(t.background)));
  set_number(w.tile,LV_STYLE_BORDER_WIDTH,1);
  // "Background: none" hides only the card; geometry and padding stay identical,
  // and the pressed flash still shows because it lives on the PRESSED state.
  set_number(w.tile,LV_STYLE_BG_OPA,t.transparent ? LV_OPA_TRANSP : LV_OPA_COVER);
  set_number(w.tile,LV_STYLE_BORDER_OPA,t.transparent ? LV_OPA_TRANSP : LV_OPA_COVER);
  set_color(w.tile,LV_STYLE_BORDER_COLOR,lv_color_hex(lit?theme::tint(state_color,110):theme::outline(t.background)));
  set_color(w.circle,LV_STYLE_BG_COLOR,circle_color);
  set_color(w.icon,LV_STYLE_TEXT_COLOR,icon_color);
  auto title_color=theme::color(theme::INK);
  auto value_color=theme::color(t.background ? theme::SLATE : theme::MUTED);
  set_color(w.unit,LV_STYLE_TEXT_COLOR,theme::color(t.is_page()?theme::CHEVRON:theme::SLATE));
  set_color(w.title,LV_STYLE_TEXT_COLOR,title_color);
  set_color(w.value,LV_STYLE_TEXT_COLOR,value_color);
  style_panel(w,t,color,title_color);
  // Custom parts follow the card palette: text like the title, lines/dots in the accent.
  // The sun path sets its own colours on every render, the sunlit area under its arc too: taking the
  // accent here made that area orange after a palette change and yellow again after the next minute.
  if(w.extra_mode!="sunpath")w.fill_color=color;
  // The media tile (firmware 0.2.64+) paints its own parts on every render: keys, the bar and the cover's placeholder
  // in the media colours, not the card's.
  for(unsigned i=0;i<w.parts.size() && w.extra_mode!="media";++i){
    auto *p=w.parts[i];if(!p)continue;
    bool muted=w.extra_mode=="forecast" ? i>=2 && i%3==2 : w.extra_mode=="sunpath" ? i>=1 : w.extra_mode=="calendar" ? i==15||i==17 : i==16;
    if(lv_obj_check_type(p,&lv_label_class))set_color(p,LV_STYLE_TEXT_COLOR,muted?value_color:title_color);
    else if(w.extra_mode=="sunpath")continue;
    else if(lv_obj_check_type(p,&lv_line_class))set_color(p,LV_STYLE_LINE_COLOR,w.extra_mode=="graph"?color:i==18?lv_color_hex(theme::foreground(theme::ha::ALARM)):i<12?value_color:i==13?icon_color:title_color);
    else set_color(p,LV_STYLE_BG_COLOR,i==14?icon_color:value_color);
  }
  lap(swipe_profile::PALETTE);
}
// What the next render() draws besides the name and the top bar: the cards of the tiles a state
// message or a tick named, or every card. A refresh that names nothing (the board's own triggers,
// such as the minute tick and time sync) draws every card.
inline uint64_t dirty_tiles=0;
inline bool dirty_all=false, dirty_header=false;
inline void mark_tile(size_t index) { if(uint64_t bit=tile_bit(index))dirty_tiles|=bit; else dirty_all=true; }
inline void refresh_tile(size_t index) { mark_tile(index); if(refresh)refresh(); }
inline void refresh_header_only() { dirty_header=true; if(refresh)refresh(); }
inline void refresh_all() { dirty_all=true; if(refresh)refresh(); }
// Slots whose new page content is still to come: the page fill draws them, render() leaves them.
inline std::array<bool,SLOTS_PER_PAGE> slot_pending{};
// Cards per fill step, the swipe pass included; the next waiting slot; the step timer, and whether
// LVGL refreshed the screen since the last step.
inline size_t FILL_STEP_CARDS=2;
inline size_t fill_next=SLOTS_PER_PAGE;
inline lv_timer_t *fill_timer=nullptr;
inline bool fill_refreshed=false;
inline uint32_t fill_step_ms=0;
inline bool fill_cards(size_t cards);
// Before the first layout the screen is starting: HA connects, then ESP Screens sends the tiles.
inline void render(lv_obj_t *room) {
  if (!enabled) return;
  room_label=room; swipe_profile::Lap lap;
  label(room, !model.configured ? (!ha_connected() ? "Connecting to Home Assistant..." : "Waiting for ESP Screens...") : !model.ready() ? "Loading tiles..." : !ha_connected() ? "HA not connected" : !feed_alive() ? "ESP Screens not active" : model.title);
  render_header();
  lap(swipe_profile::HEADER);
  bool all=dirty_all || (!dirty_tiles && !dirty_header);
  uint64_t tiles=dirty_tiles;
  dirty_all=dirty_header=false;dirty_tiles=0;
  for (size_t slot = 0; slot < SLOTS_PER_PAGE; ++slot) {
    const auto &w=widgets[slot];
    if(slot_pending[slot] || w.index>=model.count)continue;
    if(all || (tiles & tile_bit(w.index)))render_slot(slot);
  }
}

// ---- Top bar ----
// Drawn in the board's pixels by the editor's rules (app.js barLayout): every value, the time too,
// in one font on the name's baseline; icons and the dial centred on the height of the digits; the
// gaps measured between glyph ink, so every icon sits equally close to its value.
// An icon either shows the slate paint or a colour of its own (the item's state colour); `own` and `icon_color`
// remember which, so an unchanged icon is not styled again.
struct HeaderSlot { lv_obj_t *icon{}, *text{}; uint32_t icon_color = 0; bool own = false; };
inline lv_obj_t *header_root = nullptr, *header_ring = nullptr;
inline std::array<lv_obj_t *, 2> header_hands{};
inline std::array<HeaderSlot, header_bar::MAX_ITEMS> header_slots{};
inline lv_point_precise_t header_points[4]{};
inline int header_dial_key = -1;
inline void set_visible(lv_obj_t *obj, bool visible) {
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) != visible) return;
  if (visible) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
// Ink edges of a text from its label's left edge, and its advance (what LVGL sizes a label by).
struct TextInk { int left = 0, right = 0, advance = 0; };
inline TextInk text_ink(const lv_font_t *font, const std::string &text) {
  TextInk ink;
  bool first = true;
  size_t i = 0;
  for (uint32_t cp = header_bar::next_codepoint(text, i); cp; cp = header_bar::next_codepoint(text, i)) {
    lv_font_glyph_dsc_t g;
    if (!lv_font_get_glyph_dsc(font, &g, cp, 0)) continue;
    if (g.box_w > 0) {
      if (first) { ink.left = ink.advance + g.ofs_x; first = false; }
      ink.right = ink.advance + g.ofs_x + g.box_w;
    }
    ink.advance += g.adv_w;
  }
  return ink;
}
inline lv_obj_t *header_part(lv_obj_t *parent) {
  auto *part = lv_label_create(parent);
  lv_obj_remove_flag(part, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(part, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_style(part, theme::style(theme::Paint::slate), 0);
  return part;
}
// `live`: Home Assistant's values may show; without its link or the manager's feed only clocks stay.
inline void draw_header(bool live) {
  if (!room_label || !time_label || !header_text_font || !header_icon_font) return;
  set_visible(time_label, false);
  auto *page = lv_obj_get_parent(room_label);
  if (!header_root) {
    header_root = lv_obj_create(page);
    lv_obj_remove_style_all(header_root);
    lv_obj_remove_flag(header_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(header_root, LV_OBJ_FLAG_SCROLLABLE);
    // The clock label's place in the drawing order: under the tiles, cards and overlays.
    lv_obj_move_to_index(header_root, lv_obj_get_index(time_label));
    for (auto &slot : header_slots) {
      slot.icon = header_part(header_root);
      slot.text = header_part(header_root);
      lv_obj_set_style_text_font(slot.icon, header_icon_font, 0);
      lv_obj_set_style_text_font(slot.text, header_text_font, 0);
    }
    header_ring = lv_obj_create(header_root);
    lv_obj_remove_style_all(header_ring);
    lv_obj_remove_flag(header_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(header_ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(header_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_style(header_ring, theme::style(theme::Paint::slate), 0);
    lv_obj_set_style_border_opa(header_ring, LV_OPA_COVER, 0);
    lv_obj_add_flag(header_ring, LV_OBJ_FLAG_HIDDEN);
    for (auto *&hand : header_hands) {
      hand = lv_line_create(header_root);
      lv_obj_remove_flag(hand, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_line_rounded(hand, true, 0);
      lv_obj_add_style(hand, theme::style(theme::Paint::slate), 0);
      lv_obj_add_flag(hand, LV_OBJ_FLAG_HIDDEN);
    }
    // A long name ends in dots instead of running under the items.
    lv_label_set_long_mode(room_label, LV_LABEL_LONG_DOT);
  }
  // Geometry from the profile's own widgets: the name's margin and baseline, the clock's right margin.
  const lv_font_t *name_font = lv_obj_get_style_text_font(room_label, LV_PART_MAIN);
  int page_w = lv_obj_get_width(page), left = lv_obj_get_x(room_label);
  // int32_t is long on the ESP32 toolchain: keep the arithmetic in int.
  int width = std::max(0, page_w + static_cast<int>(lv_obj_get_style_x(time_label, LV_PART_MAIN)) - left);
  int baseline = lv_obj_get_y(room_label) + (name_font->line_height - name_font->base_line);
  lv_obj_set_pos(header_root, 0, 0);
  lv_obj_set_size(header_root, page_w, baseline + name_font->line_height);
  lv_font_glyph_dsc_t zero, dial_glyph;
  if (!lv_font_get_glyph_dsc(header_text_font, &zero, '0', 0) || !zero.box_h) return;
  // Twice the digits' ink centre keeps the halves exact.
  int middle2 = 2 * (baseline - zero.ofs_y) - zero.box_h;
  auto gaps = header_bar::gaps(zero.box_h);
  // The dial is as large as a round icon (clock-outline) of the icon font.
  int dial = lv_font_get_glyph_dsc(header_icon_font, &dial_glyph, 0xF0150, 0) && dial_glyph.box_h ? dial_glyph.box_h : zero.box_h * 3 / 2;

  // The manager's items; until it sends them, the clock of show_clock as before the top bar.
  header_bar::Bar fallback;
  if (!header.received && screen_settings::current.show_clock) { fallback.items[0].kind = header_bar::Kind::clock; fallback.count = 1; }
  const header_bar::Bar &bar = header.received ? header : fallback;
  auto now = now_time ? now_time() : esphome::ESPTime{};
  struct Part { size_t item = 0; uint32_t icon = 0; int icon_left = 0, icon_w = 0, text_left = 0, text_w = 0, width = 0; bool dial = false; std::string text; };
  std::array<Part, header_bar::MAX_ITEMS> parts;
  std::array<int, header_bar::MAX_ITEMS> widths{};
  size_t count = 0;
  for (size_t i = 0; i < bar.count; ++i) {
    const auto &item = bar.items[i];
    using header_bar::Kind;
    if ((item.kind == Kind::text || item.kind == Kind::ago) && !live) continue;
    Part p;
    p.item = i;
    if (item.kind == Kind::analog) { p.dial = true; p.width = dial; }
    else {
      p.text = item.kind == Kind::clock ? (now.is_valid() ? now.strftime(screen_settings::current.clock_24h ? "%H:%M" : "%I:%M") : std::string("--:--"))
             : item.kind == Kind::date ? (now.is_valid() ? header_bar::date_text(now.day_of_week, now.day_of_month, now.month) : std::string("—"))
             : item.kind == Kind::ago ? header_bar::ago_text(item.epoch, now_epoch()) : item.text;
      lv_font_glyph_dsc_t g;
      if (item.icon && lv_font_get_glyph_dsc(header_icon_font, &g, item.icon, 0) && g.box_w) { p.icon = item.icon; p.icon_left = g.ofs_x; p.icon_w = g.box_w; }
      auto ink = text_ink(header_text_font, p.text);
      p.text_left = ink.left;
      p.text_w = std::max(0, ink.right - ink.left);
      p.width = p.icon_w + (p.icon && p.text_w ? gaps.icon : 0) + p.text_w;
    }
    if (!p.width) continue;
    widths[count] = p.width;
    parts[count++] = p;
  }
  lv_point_t name_size;
  lv_text_get_size(&name_size, lv_label_get_text(room_label), name_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_EXPAND);
  auto placement = header_bar::place(widths.data(), count, gaps, width, name_size.x);
  lv_obj_set_width(room_label, std::max(1, std::min<int>(name_size.x, placement.name_room)));
  lv_obj_set_height(room_label, lv_font_get_line_height(name_font));

  std::array<bool, header_bar::MAX_ITEMS> icon_on{}, text_on{};
  bool dial_on = false;
  for (size_t k = placement.first; k < count; ++k) {
    const auto &p = parts[k];
    auto &slot = header_slots[k];
    int x = left + placement.x[k];
    if (p.dial) {
      int top = (middle2 - dial) / 2, stroke = std::max(1, (dial + 5) / 10), hand = std::max(1, (dial * 75 + 500) / 1000);
      lv_obj_set_pos(header_ring, x, top);
      lv_obj_set_size(header_ring, dial, dial);
      if (lv_obj_get_style_border_width(header_ring, LV_PART_MAIN) != stroke) lv_obj_set_style_border_width(header_ring, stroke, 0);
      int minute = now.is_valid() ? now.hour * 60 + now.minute : 0;
      int key = ((minute * 1024 + x) * 1024 + top) * 64 + dial;
      if (key != header_dial_key) {
        header_dial_key = key;
        float centre = dial / 2.0f, hour_angle = (minute % 720) * 3.14159265f / 360, minute_angle = (minute % 60) * 3.14159265f / 30;
        header_points[0] = header_points[2] = {(lv_value_precise_t)centre, (lv_value_precise_t)centre};
        header_points[1] = {(lv_value_precise_t)(centre + 0.24f * dial * sinf(hour_angle)), (lv_value_precise_t)(centre - 0.24f * dial * cosf(hour_angle))};
        header_points[3] = {(lv_value_precise_t)(centre + 0.34f * dial * sinf(minute_angle)), (lv_value_precise_t)(centre - 0.34f * dial * cosf(minute_angle))};
        for (size_t h = 0; h < header_hands.size(); ++h) {
          lv_line_set_points(header_hands[h], header_points + 2 * h, 2);
          lv_obj_set_pos(header_hands[h], x, top);
          set_line_width(header_hands[h], hand);
        }
      }
      dial_on = true;
      continue;
    }
    if (p.icon) {
      lv_font_glyph_dsc_t g;
      lv_font_get_glyph_dsc(header_icon_font, &g, p.icon, 0);
      label(slot.icon, tile_icon::utf8(p.icon));
      // LVGL draws a glyph's ink from (line_height - base_line) - box_h - ofs_y below the label top.
      int ink_top = (middle2 - g.box_h) / 2;
      lv_obj_set_pos(slot.icon, x - g.ofs_x, ink_top - ((header_icon_font->line_height - header_icon_font->base_line) - g.box_h - g.ofs_y));
      const auto &item = bar.items[p.item];
      // The words, icons and dial of the top bar take the slate paint; an item's own colour sits on top of it.
      const uint32_t color = item.has_color ? theme::foreground(item.color) : 0;
      if (slot.own != item.has_color || slot.icon_color != color) {
        if (item.has_color) lv_obj_set_style_text_color(slot.icon, lv_color_hex(color), 0);
        else lv_obj_remove_local_style_prop(slot.icon, LV_STYLE_TEXT_COLOR, 0);
        slot.own = item.has_color;
        slot.icon_color = color;
      }
      icon_on[k] = true;
      x += p.icon_w + (p.text_w ? gaps.icon : 0);
    }
    if (p.text_w) {
      label(slot.text, p.text);
      lv_obj_set_pos(slot.text, x - p.text_left, baseline - (header_text_font->line_height - header_text_font->base_line));
      text_on[k] = true;
    }
  }
  for (size_t k = 0; k < header_slots.size(); ++k) { set_visible(header_slots[k].icon, icon_on[k]); set_visible(header_slots[k].text, text_on[k]); }
  set_visible(header_ring, dial_on);
  for (auto *hand : header_hands) set_visible(hand, dial_on);
}
inline void render_header() {
  if (enabled) draw_header(ha_connected() && feed_alive());
}

// Inspect actual LVGL coordinates, including padding and the loaded font metrics.
inline bool check_tile_geometry() {
  bool ok=true;
  if(fill_next<SLOTS_PER_PAGE){ESP_LOGW("ui_test","page fill still under way at the check");fill_cards(SLOTS_PER_PAGE);}
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
    if(w.full && widgets[4].tile){
      // A full card ends exactly where the third row ends.
      int expected=lv_obj_get_y(widgets[4].tile)-lv_obj_get_y(widgets[0].tile)+w.base_height;
      if(lv_obj_get_height(w.tile)!=expected){fits=false;ESP_LOGE("ui_test","Full height FAIL slot=%u height=%d expected=%d",(unsigned)w.index,lv_obj_get_height(w.tile),expected);}
    }
    if(!custom && w.full){
      // Everything inside the card, the name above the state, a slider or the controls below them.
      fits=fits && title.x1>=content.x1 && title.x2<=content.x2 && value.x1>=content.x1 && value.x2<=content.x2 &&
        title.y1>=content.y1 && title.y2<value.y1 && value.y2<=content.y2;
      lv_area_t circle;lv_obj_get_coords(w.circle,&circle);
      fits=fits && circle.x1>=content.x1 && circle.x2<=content.x2 && circle.y1>=content.y1 && circle.y2<=content.y2;
      if(!lv_obj_has_flag(w.slider,LV_OBJ_FLAG_HIDDEN)){
        lv_obj_get_coords(w.slider,&track);
        fits=fits && value.y2<track.y1 && track.y2<=content.y2;
      }
    }else if(!custom){
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
        bool graph=w.extra && !lv_obj_has_flag(w.extra,LV_OBJ_FLAG_HIDDEN) && w.extra_mode=="graph";
        if(!watch && !graph && (mini || lv_obj_get_height(w.tile)<=80)){
          int header_bottom=mini?track.y1-(lv_obj_get_height(w.tile)>80?6:3)-1:content.y2;
          int center_twice=content.y1+header_bottom;
          fits=fits && std::abs(circle.y1+circle.y2-center_twice)<=2 &&
            std::abs(title.y1+value.y2-center_twice)<=2;
        }
      }
      if(!lv_obj_has_flag(w.unit,LV_OBJ_FLAG_HIDDEN)){
        // A large value's unit sits under the name beside the number; a navigation tile's chevron beside both.
        lv_area_t unit;lv_obj_get_coords(w.unit,&unit);
        bool chevron=w.index<model.count && model.tiles[w.index].is_page();
        fits=fits && value.x2<unit.x1 && unit.x2<=content.x2 && unit.y2<=content.y2 && unit.y1>=content.y1 && (chevron ? title.x2<unit.x1 : unit.y1>title.y2);
      }
    }
    if(w.panel && !lv_obj_has_flag(w.panel,LV_OBJ_FLAG_HIDDEN)){
      // Direct controls stay inside the card, right of the name and status, and inside their panel.
      lv_area_t panel;lv_obj_get_coords(w.panel,&panel);
      bool inside=panel.x1>=content.x1 && panel.x2<=content.x2 && panel.y1>=content.y1 && panel.y2<=content.y2 && (custom || (w.full ? value.y2<panel.y1 : title.x2<panel.x1 && value.x2<panel.x1));
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
        if(w.extra_mode=="graph" && !custom)fits=fits && (w.wide && !w.full?part.x1>value.x2:part.y1>value.y2);
      }
    }
    if(!fits)ESP_LOGE("ui_test","Tile geometry FAIL slot=%u mode=%s wide=%d title_y=%d..%d value_y=%d..%d content_y=%d..%d",(unsigned)w.index,w.extra_mode.c_str(),w.wide,title.y1,title.y2,value.y1,value.y2,content.y1,content.y2);
    if(w.index<model.count && model.tiles[w.index].background){
      bool palette_ok=lv_color_eq(lv_obj_get_style_bg_color(w.tile,LV_PART_MAIN),lv_color_hex(theme::surface(model.tiles[w.index].background))) &&
        lv_color_eq(lv_obj_get_style_text_color(w.title,LV_PART_MAIN),theme::color(theme::INK));
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
  return place(model,placement);
}
// Page switches feel immediate without blocking touch: the swipe pass places the new page (page
// number, card widths), draws its first two cards and gives the others a light skeleton frame, all
// in the very next frame. Every following LVGL refresh draws the next two cards, so the loop, and
// the touch polling in it, runs between the steps, and a new swipe drops a fill still under way.
// Keepalives and re-packing on the same page draw at once. Nothing is allocated.
inline lv_obj_t *nav_prev=nullptr,*nav_next=nullptr,*nav_number=nullptr;
inline int applied_page=-1;
// Slot assignment plus card widths and visibility for a page; contents are untouched.
inline int place_page(int page) {
  swipe_profile::Lap lap;
  std::array<Placement,MAX_TILES> placement;
  int pages=place(model,placement);
  page=std::clamp(page,0,pages-1);
  int wide_width=widgets[0].tile && widgets[1].tile ? lv_obj_get_x(widgets[1].tile)-lv_obj_get_x(widgets[0].tile)+widgets[0].base_width : 2*widgets[0].base_width;
  // A full card (firmware 0.2.62+) reaches from the first row to the end of the third.
  int full_height=widgets[0].tile && widgets[4].tile ? lv_obj_get_y(widgets[4].tile)-lv_obj_get_y(widgets[0].tile)+widgets[0].base_height : 3*widgets[0].base_height;
  for(size_t slot=0;slot<widgets.size();++slot){widgets[slot].index=MAX_TILES;widgets[slot].wide=false;widgets[slot].full=false;widgets[slot].cached_active=-1;}
  for(size_t i=0;i<model.count;++i)if(placement[i].page==page){auto &w=widgets[placement[i].slot];w.index=i;w.wide=model.tiles[i].wide;w.full=model.tiles[i].full;}
  for(size_t slot=0;slot<widgets.size();++slot){
    auto &w=widgets[slot];if(!w.tile)continue;
    if(slot<SLOTS_PER_PAGE && w.index<model.count){lv_obj_set_size(w.tile,w.wide?wide_width:w.base_width,w.full?full_height:w.base_height);lv_obj_remove_flag(w.tile,LV_OBJ_FLAG_HIDDEN);}
    else{lv_obj_add_flag(w.tile,LV_OBJ_FLAG_HIDDEN);hide_extra(w);hide_panel(w);}
  }
  for(auto *control:{nav_prev,nav_next,nav_number})set_hidden(control,pages<=1);
  if(page==0)lv_obj_add_state(nav_prev,LV_STATE_DISABLED);else lv_obj_remove_state(nav_prev,LV_STATE_DISABLED);
  if(page==pages-1)lv_obj_add_state(nav_next,LV_STATE_DISABLED);else lv_obj_remove_state(nav_next,LV_STATE_DISABLED);
  for(auto *control:{nav_prev,nav_next})if(lv_obj_get_child_count(control))
    set_number(lv_obj_get_child(control,0),LV_STYLE_TEXT_OPA,lv_obj_has_state(control,LV_STATE_DISABLED)?LV_OPA_30:LV_OPA_COVER);
  label(nav_number,std::to_string(page+1)+" / "+std::to_string(pages));
  lap(swipe_profile::PLACE);
  return page;
}
// The skeleton frame is the empty card: its contents hidden under a sheet in the card's own colour
// (the page colour for a card without a background) that covers the content area. The sheet is a
// plain rectangle inside the card's padding, so it has no corners, border or layer to render, and
// while it is up LVGL draws nothing under it. Drawing the card takes the sheet away again.
inline lv_color_t page_color(const Widgets &w) {
  for(auto *o=lv_obj_get_parent(w.tile);o;o=lv_obj_get_parent(o))
    if(lv_obj_get_style_bg_opa(o,LV_PART_MAIN)>=LV_OPA_MAX)return lv_obj_get_style_bg_color(o,LV_PART_MAIN);
  return theme::color(theme::PAGE);
}
inline void skeleton(Widgets &w) {
  for(auto *o:{w.title,w.value,w.circle,w.unit,w.slider,w.extra,w.panel,w.busy})if(o)lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
  if(!w.veil){
    w.veil=lv_obj_create(w.tile);lv_obj_remove_style_all(w.veil);
    lv_obj_remove_flag(w.veil,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(w.veil,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(w.veil,LV_OPA_COVER,0);
  }
  const Tile *t=w.index<model.count?&model.tiles[w.index]:nullptr;
  set_color(w.veil,LV_STYLE_BG_COLOR,t && t->transparent?page_color(w):lv_color_hex(theme::surface(t?t->background:0)));
  lv_obj_set_pos(w.veil,0,0);
  lv_obj_set_size(w.veil,content_width(w),content_height(w));
  lv_obj_remove_flag(w.veil,LV_OBJ_FLAG_HIDDEN);
  if(lv_obj_get_index(w.veil)!=(int32_t)lv_obj_get_child_count(w.tile)-1)lv_obj_move_foreground(w.veil);
}
inline void drop_veil(Widgets &w) {
  if(w.veil && !lv_obj_has_flag(w.veil,LV_OBJ_FLAG_HIDDEN))lv_obj_add_flag(w.veil,LV_OBJ_FLAG_HIDDEN);
}
// Draws up to `cards` waiting cards in slot order; true once the page is complete.
inline bool fill_cards(size_t cards) {
  for(size_t drawn=0;fill_next<SLOTS_PER_PAGE && drawn<cards;++fill_next){
    if(!slot_pending[fill_next])continue;
    slot_pending[fill_next]=false;render_slot(fill_next);drop_veil(widgets[fill_next]);++drawn;
  }
  while(fill_next<SLOTS_PER_PAGE && !slot_pending[fill_next])++fill_next;
  fill_step_ms=esphome::millis();fill_refreshed=false;
  if(fill_next<SLOTS_PER_PAGE)return false;
  if(fill_timer)lv_timer_pause(fill_timer);
  swipe_profile::content_complete();
  return true;
}
inline void cancel_fill() {
  fill_next=SLOTS_PER_PAGE;slot_pending.fill(false);
  if(fill_timer)lv_timer_pause(fill_timer);
}
inline void fill_timer_done(lv_timer_t *) {
  // One step per refresh, so a step's frame reaches the glass before the next step is drawn. A step
  // that changed no pixel starts no refresh; the fill then moves on after a short wait.
  if(fill_next>=SLOTS_PER_PAGE || (!fill_refreshed && esphome::millis()-fill_step_ms<40))return;
  swipe_profile::FillTimer timer;
  fill_cards(FILL_STEP_CARDS);
}
inline void apply_page(int page) {
  cancel_fill();
  applied_page=place_page(page);
  for(auto &w:widgets)drop_veil(w);
  if(room_label){dirty_all=true;render(room_label);}else refresh_all();
}
inline void show_page(int &page, lv_obj_t *previous, lv_obj_t *next, lv_obj_t *number) {
  nav_prev=previous;nav_next=next;nav_number=number;shown_page=&page;
  int requested=page;
  page=std::clamp(page,0,int(page_count())-1);
  // A swipe past the first or last page: the page on screen is already right, so nothing is
  // drawn again. A layout change always lands in range or on a different page.
  if(requested!=page && page==applied_page)return;
  if(applied_page<0 || page==applied_page || !room_label){apply_page(page);return;}
  swipe_profile::begin(applied_page,page);
  swipe_profile::SkeletonTimer timer;
  cancel_fill();
  applied_page=place_page(page);
  swipe_profile::Lap lap;
  for(size_t slot=0;slot<SLOTS_PER_PAGE;++slot){
    auto &w=widgets[slot];
    slot_pending[slot]=w.tile && w.index<model.count;
    if(slot_pending[slot])skeleton(w);
  }
  for(size_t slot=SLOTS_PER_PAGE;slot<widgets.size();++slot)drop_veil(widgets[slot]);
  lap(swipe_profile::SKELETON);
  // The skeleton frame goes out first; the fill starts on the refresh after it.
  fill_next=0;fill_step_ms=esphome::millis();fill_refreshed=false;
  if(!fill_timer){
    fill_timer=lv_timer_create(fill_timer_done,1,nullptr);
    if(auto *display=lv_display_get_default())
      lv_display_add_event_cb(display,[](lv_event_t *){fill_refreshed=true;},LV_EVENT_REFR_READY,nullptr);
  }
  lv_timer_resume(fill_timer);
}
// A navigation tile (screen.page, firmware 0.2.62+): the page it names, kept within the pages the screen has.
inline void go_to_page(int page) {
  if(!shown_page || !nav_number)return;
  *shown_page=std::clamp(page,0,int(page_count())-1);
  show_page(*shown_page,nav_prev,nav_next,nav_number);
}

#ifdef SWIPE_PROFILE
inline unsigned swipe_test_left=0, swipe_test_back=0;
inline bool swipe_test_returning=false;
inline lv_timer_t *swipe_test_timer=nullptr;
inline void swipe_test_step(lv_timer_t *timer) {
  if(!shown_page || !swipe_test_left || page_count()<2){lv_timer_pause(timer);swipe_test_left=0;ESP_LOGI("swipe_prof","swipe test finished");return;}
  int pages=page_count(),from=*shown_page;
  *shown_page=swipe_test_returning?from-1:(from+1<pages?from+1:0);
  if(*shown_page<0)*shown_page=pages-1;
  show_page(*shown_page,nav_prev,nav_next,nav_number);
  if(swipe_test_back && !swipe_test_returning){swipe_test_returning=true;lv_timer_set_period(timer,swipe_test_back);return;}
  swipe_test_returning=false;--swipe_test_left;
  lv_timer_set_period(timer,lv_timer_get_user_data(timer)?(uint32_t)(uintptr_t)lv_timer_get_user_data(timer):1200);
}
inline void swipe_test(unsigned count, unsigned interval_ms, unsigned back_ms, JsonObject tuning) {
  // Cards per fill step for the measurement.
  if(tuning["cards"].is<unsigned>())FILL_STEP_CARDS=std::clamp<unsigned>(tuning["cards"].as<unsigned>(),1,6);
  ESP_LOGI("swipe_prof","tuning: %u cards per step",(unsigned)FILL_STEP_CARDS);
  swipe_test_left=std::min(count,200u);swipe_test_back=back_ms;swipe_test_returning=false;
  interval_ms=std::clamp(interval_ms,200u,10000u);
  if(!swipe_test_timer)swipe_test_timer=lv_timer_create(swipe_test_step,interval_ms,(void*)(uintptr_t)interval_ms);
  lv_timer_set_user_data(swipe_test_timer,(void*)(uintptr_t)interval_ms);
  lv_timer_set_period(swipe_test_timer,interval_ms);lv_timer_reset(swipe_test_timer);lv_timer_resume(swipe_test_timer);
  ESP_LOGI("swipe_prof","swipe test: %u page switches every %u ms, back after %u ms",swipe_test_left,interval_ms,back_ms);
}
#endif
inline uint32_t last_live_second=0;
inline int last_clock_minute=-2;
inline bool was_fresh=false;
inline void tick() {
  if(detail_root && !lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN) && detail_index<model.count){
    auto &t=model.tiles[detail_index];bool waiting=t.loading(esphome::millis());
    // The history the card waits for: drawn once it is here (a finger on the screen holds that back), asked for
    // again every 30 s, and after 8 s without it (an app from before 0.2.59, Home Assistant away) the card says so.
    if(history_chart.status&&!history_chart.ready){
      const uint32_t now=esphome::millis();
      if(history_fits(t))refresh_detail(detail_index);
      else if(now-history_asked_at>30000)history_request(t.entity,history_hours);
      else if(now-history_asked_at>8000)label(history_chart.status,"No history available");
    }
    for(unsigned i=0;i<detail_action_count;++i){if(waiting||!fresh()||!t.available())lv_obj_add_state(detail_actions[i],LV_STATE_DISABLED);else lv_obj_remove_state(detail_actions[i],LV_STATE_DISABLED);}
    std::string status=waiting?(t.confirmed?"Confirmed by Home Assistant":"Command sent..."):t.domain()=="cover"?cover_status_line(t):detail_state(t);
    if(detail_status)label(detail_status,status+(!waiting && !t.unit.empty()?" "+t.unit:""));
    // The vacuum card lays its state out with the room and battery beside it, so a new text draws the
    // card again; once Home Assistant answered, the new state says enough.
    if(detail_badge_status && t.domain()=="vacuum"){
      status=waiting && !t.confirmed?"Command sent...":detail_state(t);
      if(status!=lv_label_get_text(detail_badge_status))refresh_detail(detail_index);
    }else if(detail_badge_status)label(detail_badge_status,status);
    if(detail_switch && !lv_obj_has_state(detail_switch,LV_STATE_PRESSED)){
      if(t.state=="on")lv_obj_add_state(detail_switch,LV_STATE_CHECKED);
      else lv_obj_remove_state(detail_switch,LV_STATE_CHECKED);
    }
  }
  if(!enabled)return;
  // Only the cards that change are drawn again: a clock or a finished command redraws its own card.
  bool redraw=false;
  auto card=[&](size_t index){ mark_tile(index); redraw=true; };
  // HA dropping or returning and the feed timing out change every card at once.
  bool now_fresh=fresh();
  if(now_fresh!=was_fresh){was_fresh=now_fresh;dirty_all=true;redraw=true;}
#ifdef USE_API_HOMEASSISTANT_ACTION_RESPONSES
  expire_calls(esphome::millis());
#endif
  for(size_t i=0;i<model.tiles.size();++i){
    auto &t=model.tiles[i];
    if(t.refused_at && esphome::millis()-t.refused_at>=4000){t.refused_at=0;card(i);}
    // A held slider whose light never got there shows what Home Assistant last reported again.
    if(std::isfinite(t.slider_sent) && !t.slider_holding(esphome::millis())){t.release_slider();card(i);}
    if(!t.pending || t.waiting(esphome::millis()))continue;
    // A vacuum chip Home Assistant never confirmed goes back to what the robot reports.
    bool sent=false;if(auto *x=t.extra_ptr())for(auto &c:x->choices)sent=sent||!c.sent.empty();
    end_wait(i);card(i);
    if(sent)refresh_detail(i);
  }
  // A -/+ edit goes out as one call once the finger rests; a value HA never reports is dropped after a while.
  for(size_t i=0;i<model.count;++i){
    auto &t=model.tiles[i];if(!std::isfinite(t.edit_value))continue;
    uint32_t now=esphome::millis();
    if(!t.edit_sent){
      if(now-t.edit_since<700 || t.waiting(now))continue;
      auto a=tile_controls::edit_action(t,t.edit_value);
      if(a.valid()){t.edit_sent=true;t.edit_since=now;action(a.service,t.entity,a.key,a.value);}else t.edit_value=NAN;
    }else if(now-t.edit_since>10000){t.edit_value=NAN;card(i);}
  }
  // Running timers advance once per second without any HA traffic; clocks show hours and minutes,
  // so they are drawn again only when the minute (or the time's validity) changes.
  uint32_t second=esphome::millis()/1000;
  if(second!=last_live_second){
    last_live_second=second;
    // The media card's bar runs on while the track plays (firmware 0.2.64+).
    if(media_progress_fill && detail_root && !lv_obj_has_flag(detail_root,LV_OBJ_FLAG_HIDDEN) && detail_index<model.count)
      media_progress(model.tiles[detail_index],media_progress_fill,media_elapsed_label,media_bar_width);
    auto now=now_time?now_time():esphome::ESPTime{};
    int minute=now.is_valid()?now.day_of_year*1440+now.hour*60+now.minute:-1;
    bool new_minute=minute!=last_clock_minute;last_clock_minute=minute;
    for(size_t slot=0;slot<SLOTS_PER_PAGE;++slot){
      auto &w=widgets[slot];if(!w.tile || w.index>=model.count || lv_obj_has_flag(w.tile,LV_OBJ_FLAG_HIDDEN))continue;
      const auto &t=model.tiles[w.index];
      if((t.is_clock() && new_minute) || (t.domain()=="timer" && t.state=="active") || (t.domain()=="sun" && second%60==0))card(w.index);
      // A media tile over the whole page: its bar runs on while the track plays (firmware 0.2.64+).
      if(w.extra_mode=="media" && w.extra && !lv_obj_has_flag(w.extra,LV_OBJ_FLAG_HIDDEN) && w.parts[5] && !lv_obj_has_flag(w.parts[5],LV_OBJ_FLAG_HIDDEN))media_progress(t,w.parts[5],w.parts[6],w.media_bar_w);
      // The second hand moves on its own: only its line is redrawn, and it hides during standby. Only while the
      // slot's parts are a dial: right after a page switch the slot already names the clock while its parts still
      // belong to the card drawn before (a forecast's hour labels take part 18 too), until the fill draws the dial.
      // A single dial is drawn as "calendar", a wide or full one as "analog"; firmware 0.2.62-0.2.64 moved only the
      // latter, so the hand of a single clock stood still (0.2.65). Part 18 must be the hand's own line either way.
      else if(w.parts[18] && lv_obj_check_type(w.parts[18],&lv_line_class) && w.points &&
              (w.extra_mode=="analog" || w.extra_mode=="calendar") && t.is_clock() && t.display=="analog")second_hand(w,now);
    }
  }
  if(redraw && refresh)refresh();
}
// A change of look (Dark mode). The paints follow by themselves (theme::set_dark); what the tiles, the top bar and an
// open card painted in code is drawn again here, in the same pass, so no frame shows half of each look.
inline void restyle() {
  for (auto &w : widgets) { w.cached_active = -1; w.panel_dirty = true; }
  for (auto &slot : header_slots) { slot.own = true; slot.icon_color = UINT32_MAX; }
  if (room_label) { dirty_all = true; render(room_label); }
  if (detail_root && !lv_obj_has_flag(detail_root, LV_OBJ_FLAG_HIDDEN) && detail_index < model.count) show_detail(detail_index);
}
inline std::string vacuum_option(unsigned index) {
  if (active_index < 0 || static_cast<size_t>(active_index) >= model.count) return {};
  auto &tile = model.tiles[active_index];
  const auto &speeds = tile.extra().fan_speeds;
  return index < speeds.size() ? speeds[index] : "";
}
}

// ---- Camera images (firmware 0.2.57+) ----
// A camera or image tile, and an alert's image, open the camera full screen: the image as large as fits, the round back
// key at the top left like on every card, the name beside it. ESP Screen Manager fetches the snapshot, sizes it for this
// screen and serves it on its own port; camera_view::Feed decides when to ask for a link and when to load it again. The
// board binds the two online_images (the full view and the alert's frame) through ImageHooks; a board without them (the
// CYD) never opens the view. Nothing here exists while no camera is open, apart from the alert's small frame.
namespace runtime_tiles {
struct ImageHooks {
  std::function<void(const std::string &)> load;  // set the online_image's URL and download it
  std::function<void()> release;                  // free the decoded image
  std::function<lv_image_dsc_t *()> source;       // the decoded image for LVGL
};
inline ImageHooks camera_full, camera_thumb;
inline camera_view::Feed camera;
// Freeing the image waits for the next tick: ending a download under way can take a few hundred ms, and Back should
// show the page below at once.
inline bool camera_release_due = false;
inline lv_obj_t *camera_root = nullptr, *camera_picture = nullptr, *camera_note = nullptr, *camera_back = nullptr, *camera_title = nullptr;
// The alert's image: the board's frame at the bottom left of the card, the camera the app announced for the next alert
// and the one the card on screen shows.
inline lv_obj_t *alert_frame = nullptr, *alert_picture = nullptr, *alert_frame_icon = nullptr;
inline std::function<void(bool)> alert_room;  // the board makes the card taller for the frame, or back
inline std::string alert_announced, alert_camera, alert_url;
inline uint32_t alert_announced_at = 0, alert_retry_at = 0, alert_shown_at = 0;
inline uint8_t alert_retries = 0;
inline bool alert_thumb_loading = false;
constexpr uint8_t ALERT_IMAGE_RETRIES = 3;  // an alert's picture is worth another try after a failed connection
// One picture loads at a time (firmware 0.2.64+): a download shares ESPHome's loop with touch and drawing, and two at
// once (a cover and an alert's picture) would double the time a tap can wait. An alert closes every card (wake_display
// runs close_cards), so the card's cover goes with it; a media tile over the whole page stays under the alert. Its
// cover waits while the alert's picture is announced, on its way or about to be tried again; a cover already on its
// way finishes and the alert's picture starts right after it. A full camera and the cover share one image, so they
// never load together. Memory is not the limit here: the pictures live in PSRAM (a cover ~70 KB, the alert's 172 KB).
inline bool alert_image_due() {
  if (alert_camera.empty() || alert_picture) return false;
  if (alert_thumb_loading || alert_retry_at) return true;
  // The link has not come yet: wait for it as long as an announced camera still belongs to its alert.
  return alert_url.empty() && esphome::millis() - alert_shown_at < camera_view::PENDING_MS;
}

inline bool camera_supported() { return static_cast<bool>(camera_full.load); }
inline bool camera_visible() { return camera_root != nullptr; }

// ---- The album cover of the media card (firmware 0.2.64+) ----
// The card, or a media tile over the whole page, says which cover it wants: the player, the mark of its picture, the
// size and the colour behind the rounded corners (cover_want). The app answers `esphome.screen_camera` with a link to
// a BMP of exactly that (op "camera", t "cover"); the board's full online_image loads it once and the picture stays
// until the mark changes, the owner goes (the card closes, the page turns) or a camera opens full screen: the camera
// and the cover share that one image buffer, and the camera wins. Everything runs from camera_tick().
struct CoverWish { std::string entity, picture; int size = 0; uint32_t background = 0; CoverOwner owner = CoverOwner::NONE; size_t slot = 0; };
inline CoverWish cover_wish;
inline camera_view::Feed cover;
inline void camera_release();
inline void camera_request(const std::string &entity, int size = 0, uint32_t background = 0);
// The pictures on screen go before their buffer does.
inline void cover_forget_pictures() {
  if (media_detail_picture) { lv_obj_delete(media_detail_picture); media_detail_picture = nullptr; }
  for (auto &w : widgets) if (w.extra_mode == "media" && w.parts[MEDIA_PICTURE]) { lv_obj_delete(w.parts[MEDIA_PICTURE]); w.parts[MEDIA_PICTURE] = nullptr; }
}
inline void cover_release() {
  const bool had = cover.open();
  cover = camera_view::Feed{};
  cover_forget_pictures();
  if (had && !camera_root) camera_release_due = true;
}
inline void cover_want(const std::string &entity, const std::string &picture, int size, uint32_t background, CoverOwner owner, size_t slot) {
  if (!camera_supported()) return;
  const bool same = cover_wish.entity == entity && cover_wish.picture == picture && cover_wish.size == size && cover_wish.background == background;
  cover_wish = CoverWish{entity, picture, size, background, owner, slot};
  if (same) return;
  cover_release();  // another cover: asked for on the next tick
}
inline lv_image_dsc_t *cover_ready(const std::string &entity, int size, uint32_t background) {
  if (!cover.loaded || cover.entity != entity || cover_wish.size != size || cover_wish.background != background) return nullptr;
  auto *src = camera_full.source();
  return src && src->data ? src : nullptr;
}
inline void cover_drop() { cover_wish = CoverWish{}; cover_release(); }
// Whether the owner still shows the cover: the card open on that player, or the tile on screen in its slot.
inline bool cover_visible() {
  if (cover_wish.owner == CoverOwner::DETAIL)
    return detail_root && !lv_obj_has_flag(detail_root, LV_OBJ_FLAG_HIDDEN) && detail_index < model.count && model.tiles[detail_index].entity == cover_wish.entity;
  if (cover_wish.owner == CoverOwner::TILE && cover_wish.slot < widgets.size()) {
    if (detail_root && !lv_obj_has_flag(detail_root, LV_OBJ_FLAG_HIDDEN)) return false;  // a card covers the page
    auto &w = widgets[cover_wish.slot];
    return w.tile && !lv_obj_has_flag(w.tile, LV_OBJ_FLAG_HIDDEN) && w.index < model.count && model.tiles[w.index].entity == cover_wish.entity && w.extra_mode == "media";
  }
  return false;
}
// Nobody holds the cover and a media tile with a picture is on the page (the card over it just closed, or the
// page turned): the tile draws itself again and asks.
inline void cover_offer() {
  if (detail_root && !lv_obj_has_flag(detail_root, LV_OBJ_FLAG_HIDDEN)) return;
  for (auto &w : widgets) {
    if (!w.tile || lv_obj_has_flag(w.tile, LV_OBJ_FLAG_HIDDEN) || w.index >= model.count || w.extra_mode != "media") continue;
    const auto &t = model.tiles[w.index];
    if (t.extra().media_picture.empty() || !media_card::has_track(t.state)) continue;
    refresh_tile(w.index);
    return;
  }
}
// The cover is here: onto the card at once, or the tile draws itself again with it.
inline void cover_arrived() {
  if (!cover_visible()) return;
  if (cover_wish.owner == CoverOwner::DETAIL) media_detail_picture = media_picture_show(detail_root, media_detail_picture, media_art_rect, camera_full.source());
  else refresh_tile(widgets[cover_wish.slot].index);
  ESP_LOGI("camera", "cover of %s shown", cover.entity.c_str());
}
inline void cover_tick(uint32_t now) {
  if (camera_root || !camera_supported()) return;
  if (cover_wish.owner == CoverOwner::NONE) { cover_offer(); return; }
  if (!cover_visible()) { cover_drop(); return; }
  if (!awake()) return;
  if (!cover.open()) cover.open(cover_wish.entity, true);
  if (alert_image_due()) return;  // the alert's picture first
  if (cover.should_ask(now)) {
    if (!fresh()) return;
    cover.ask(now);
    camera_request(cover.entity, cover_wish.size, cover_wish.background);
  } else if (cover.should_load(now)) {
    auto *input = lv_indev_get_next(nullptr);
    if (input && lv_indev_get_state(input) == LV_INDEV_STATE_PRESSED) return;
    cover.start(now);
    ESP_LOGI("camera", "cover load %s", cover.entity.c_str());
    camera_full.load(cover.url);
  }
}

// Asks ESP Screen Manager for a link (app 0.2.66+ answers with op "camera"). An event, like history_request.
// A cover (firmware 0.2.64+) adds the size it wants and the colour behind its rounded corners; the app bakes both in.
inline void camera_request(const std::string &entity, int size, uint32_t background) {
  if (inbox.empty()) return;
  esphome::api::HomeassistantActionRequest request;
  request.service = esphome::StringRef("esphome.screen_camera");
  request.is_event = true;
  char size_text[12] = "", background_text[8] = "";
  if (size > 0) { snprintf(size_text, sizeof(size_text), "%d", size); snprintf(background_text, sizeof(background_text), "%06X", (unsigned) background); }
  const std::string keys[] = {"inbox", "entity", "size", "bg"}, values[] = {inbox, entity, size_text, background_text};
  const int count = size > 0 ? 4 : 2;
  request.data.init(count);
  for (int i = 0; i < count; ++i) {
    esphome::api::HomeassistantServiceMap entry;
    entry.key = esphome::StringRef(keys[i]);
    entry.value = esphome::StringRef(values[i]);
    request.data.push_back(entry);
  }
  esphome::api::global_api_server->send_homeassistant_action(request);
  ESP_LOGI("camera", "asked for %s", entity.c_str());
}

inline void camera_note_text(const char *text) {
  if (!camera_note) return;
  lv_label_set_text(camera_note, text);
  if (text[0]) lv_obj_remove_flag(camera_note, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(camera_note, LV_OBJ_FLAG_HIDDEN);
}

inline void camera_release() {
  camera_release_due = false;
  if (camera_full.release) camera_full.release();
}

inline void camera_close() {
  if (!camera_root) return;
  lv_obj_delete(camera_root);
  camera_root = camera_picture = camera_note = camera_back = camera_title = nullptr;
  camera_release_due = true;
  ESP_LOGI("camera", "closed %s", camera.entity.c_str());
  camera = camera_view::Feed{};
}

inline void camera_open(const std::string &entity, const std::string &name) {
  if (!camera_supported() || !valid_entity(entity)) return;
  camera_close();
  // The last camera's image, or a media card's cover, goes before this one loads into the same online_image; the
  // cover is asked for again once the camera closes (cover_tick).
  cover_release();
  if (camera_release_due) camera_release();
  camera.open(entity);
  const int width = lv_display_get_horizontal_resolution(lv_display_get_default());
  const bool large = width >= 480;
  // On the top layer: above the tiles, every card and an alert, which is there again after Back.
  camera_root = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(camera_root);
  lv_obj_set_size(camera_root, lv_pct(100), lv_pct(100));
  lv_obj_remove_flag(camera_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(camera_root, LV_OBJ_FLAG_CLICKABLE);  // nothing reaches the tiles below
  lv_obj_set_style_bg_color(camera_root, theme::color(theme::CAMERA_PAGE), 0);
  lv_obj_set_style_bg_opa(camera_root, LV_OPA_COVER, 0);
  camera_note = lv_label_create(camera_root);
  if (detail_font) lv_obj_set_style_text_font(camera_note, detail_font, 0);
  lv_obj_set_style_text_color(camera_note, theme::color(theme::CAMERA_NOTE), 0);
  lv_obj_center(camera_note);
  camera_note_text("Loading image");
  // The same top bar as a tile's card: a round back arrow at the left, the name centred.
  const int bar = large ? 60 : 40, bar_x = large ? 16 : 10, bar_y = large ? 16 : 8;
  camera_back = lv_obj_create(camera_root);
  lv_obj_remove_style_all(camera_back);
  lv_obj_set_pos(camera_back, bar_x, bar_y);
  lv_obj_set_size(camera_back, bar, bar);
  lv_obj_add_flag(camera_back, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(camera_back, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(camera_back, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(camera_back, theme::color(theme::KEY), 0);
  lv_obj_set_style_bg_color(camera_back, theme::color(theme::KEY_PRESSED), LV_STATE_PRESSED);
  auto *arrow = lv_label_create(camera_back);
  if (mini_icon_font) lv_obj_set_style_text_font(arrow, mini_icon_font, 0);
  lv_obj_set_style_text_color(arrow, theme::color(theme::INK), 0);
  lv_label_set_text(arrow, "\U000F004D");
  lv_obj_center(arrow);
  lv_obj_add_event_cb(camera_back, [](lv_event_t *) {
    // Closed after this event: the key that sends it goes with the view.
    lv_async_call([](void *) { camera_close(); }, nullptr);
  }, LV_EVENT_SHORT_CLICKED, nullptr);
  const lv_font_t *title_font = watch_font ? watch_font : detail_font;
  camera_title = lv_label_create(camera_root);
  if (title_font) lv_obj_set_style_text_font(camera_title, title_font, 0);
  lv_obj_set_style_text_color(camera_title, theme::color(theme::CAMERA_INK), 0);
  lv_obj_set_style_text_align(camera_title, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(camera_title, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(camera_title, bar_x + bar + 8, bar_y + (bar - (title_font ? lv_font_get_line_height(title_font) : 20)) / 2);
  lv_obj_set_size(camera_title, width - 2 * (bar_x + bar + 8), title_font ? lv_font_get_line_height(title_font) : 20);
  lv_label_set_text(camera_title, name.c_str());
  ESP_LOGI("camera", "open %s", entity.c_str());
}

// The board's interval (250 ms): ask for a link, or load the image again when it is time. Loading happens here, in
// ESPHome's loop, and never while the screen is in standby.
inline void camera_tick() {
  const uint32_t now = esphome::millis();
  if (camera_release_due && !camera_root) camera_release();
  // A retry, or an alert's picture that waited for a cover on its way (one picture at a time).
  if (alert_retry_at && now >= alert_retry_at && !cover.loading) {
    alert_retry_at = 0;
    if (!alert_camera.empty() && !alert_url.empty() && camera_thumb.load) {
      alert_thumb_loading = true;
      ESP_LOGI("camera", "alert picture load");
      camera_thumb.load(alert_url);
    }
  }
  cover_tick(now);
  if (!camera_root || !awake()) return;
  if (camera.should_ask(now)) {
    if (!fresh()) return;
    camera.ask(now);
    camera_request(camera.entity);
  } else if (camera.should_load(now)) {
    // Not under a finger: a load that starts now would hold up the tap that is on its way.
    auto *input = lv_indev_get_next(nullptr);
    if (input && lv_indev_get_state(input) == LV_INDEV_STATE_PRESSED) return;
    camera.start(now);
    camera_full.load(camera.url);
  }
}

inline void alert_picture_clear() {
  alert_retry_at = 0;
  if (alert_picture) { lv_obj_delete(alert_picture); alert_picture = nullptr; }
  if (alert_frame_icon) lv_obj_remove_flag(alert_frame_icon, LV_OBJ_FLAG_HIDDEN);
  if (alert_thumb_loading || alert_camera.size()) { if (camera_thumb.release) camera_thumb.release(); }
  alert_thumb_loading = false;
}

// alert_show: the card gets its frame when the app announced a camera for it just before. A new alert closes the camera.
inline void alert_prepare() {
  camera_close();
  alert_picture_clear();
  const bool with_image = camera_supported() && alert_frame && !alert_announced.empty() &&
                          esphome::millis() - alert_announced_at < camera_view::PENDING_MS;
  alert_camera = with_image ? alert_announced : std::string();
  alert_shown_at = esphome::millis();
  alert_url.clear();
  alert_announced.clear();
  if (alert_frame) {
    if (with_image) lv_obj_remove_flag(alert_frame, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(alert_frame, LV_OBJ_FLAG_HIDDEN);
  }
  if (alert_room) alert_room(with_image);
}

// alert_dismiss: the frame goes, and so does its image.
inline void alert_clear() {
  alert_picture_clear();
  alert_camera.clear();
  alert_url.clear();
  if (alert_frame) lv_obj_add_flag(alert_frame, LV_OBJ_FLAG_HIDDEN);
}

inline void camera_answer(const std::string &view, const std::string &entity, const std::string &url) {
  if (!camera_supported()) return;
  if (view == "full") {
    if (!camera_root || camera.entity != entity) return;
    camera.link(url);
    if (url.empty() && !camera.shown) camera_note_text("No image from this camera");
    return;
  }
  if (view == "cover") {  // the media card's album cover (firmware 0.2.64+)
    if (!cover.open() || cover.entity != entity) return;
    cover.link(url);
    if (url.empty()) ESP_LOGI("camera", "no cover for %s", entity.c_str());
    return;
  }
  if (url.empty()) {  // announced before its alert
    alert_announced = entity;
    alert_announced_at = esphome::millis();
    return;
  }
  if (entity != alert_camera || !alert_frame || lv_obj_has_flag(alert_frame, LV_OBJ_FLAG_HIDDEN)) return;
  alert_picture_clear();
  alert_url = url;
  alert_retries = 0;
  // A cover on its way finishes first, and a dropped one is closed first (on the next tick); camera_tick starts this
  // one right after.
  if (cover.loading || camera_release_due) {
    alert_retry_at = esphome::millis() | 1;
    ESP_LOGI("camera", "alert picture waits for the cover");
    return;
  }
  alert_thumb_loading = true;
  ESP_LOGI("camera", "alert picture load");
  camera_thumb.load(url);
}

// LVGL's image widget is only built for a board whose profile draws images (the Guition's hidden seed); the CYD's has none.
inline void camera_show(lv_obj_t *parent, lv_obj_t *&picture, lv_image_dsc_t *source, bool fresh_pixels) {
#if LV_USE_IMAGE
  if (!source || !source->data) return;
  if (!picture) {
    picture = lv_image_create(parent);
    lv_obj_remove_flag(picture, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(picture, source);
    lv_obj_center(picture);
    return;
  }
  if (!fresh_pixels) return;
  // The same buffer with new pixels. LVGL keeps no decoded copy of an RGB565 image (its image cache is off in
  // ESPHome's build), so drawing the area again shows them.
  lv_image_set_src(picture, source);
  lv_obj_invalidate(picture);
#else
  (void) parent; (void) picture; (void) source; (void) fresh_pixels;
#endif
}

// The board's online_image triggers. `thumb`: the alert's frame; `cached`: the app answered 304, the image is unchanged.
inline void camera_loaded(bool thumb, bool cached) {
  if (thumb) {
    alert_thumb_loading = false;
    if (alert_camera.empty() || !alert_frame) return;
    camera_show(alert_frame, alert_picture, camera_thumb.source(), !cached);
    if (alert_picture && alert_frame_icon) lv_obj_add_flag(alert_frame_icon, LV_OBJ_FLAG_HIDDEN);
    ESP_LOGI("camera", "alert picture shown");
    return;
  }
  if (!camera_root) {
    // The media card's cover (firmware 0.2.64+): the same online_image, loaded once.
    if (cover.loading) { cover.finish(esphome::millis(), true); ESP_LOGI("camera", "cover loaded"); cover_arrived(); }
    return;
  }
  if (!camera.loading) return;  // a cover's download that ended after the camera opened: not this camera's picture
  camera.finish(esphome::millis(), true);
  const bool first = camera_picture == nullptr;
  camera_show(camera_root, camera_picture, camera_full.source(), !cached);
  if (first && camera_picture) {
    camera_note_text("");
    lv_obj_move_foreground(camera_back);
    lv_obj_move_foreground(camera_title);
  }
}

inline void camera_failed(bool thumb) {
  if (thumb) {
    alert_thumb_loading = false;
    ESP_LOGI("camera", "alert picture failed");
    if (!alert_camera.empty() && !alert_picture && alert_retries < ALERT_IMAGE_RETRIES) {
      ++alert_retries;
      alert_retry_at = esphome::millis() + 1500;
    }
    return;
  }
  if (!camera_root) {
    if (cover.loading) { cover.finish(esphome::millis(), false); ESP_LOGI("camera", "cover failed"); }  // tried again after the gap, three times at most
    return;
  }
  if (!camera.loading) return;
  camera.finish(esphome::millis(), false);
  if (!camera.shown) camera_note_text("No image from this camera");
}
}  // namespace runtime_tiles
