#pragma once
// Direct controls on the right half of a double-width card, like Home Assistant's
// own entity rows. Pure logic only: which keys a card shows, what they send, how
// a -/+ step lands on the entity's grid, and the status line beside them. The
// LVGL drawing lives in runtime_tiles.h; tests/test_tile_controls.cpp covers this.
#include "runtime_model.h"
#include "theme.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace tile_controls {
// Home Assistant supported_features bits.
namespace feature {
constexpr uint32_t COVER_OPEN = 1, COVER_CLOSE = 2, COVER_POSITION = 4, COVER_STOP = 8;
constexpr uint32_t COVER_OPEN_TILT = 16, COVER_CLOSE_TILT = 32, COVER_STOP_TILT = 64, COVER_TILT_POSITION = 128;
constexpr uint32_t MEDIA_PAUSE = 1, MEDIA_VOLUME_SET = 4, MEDIA_VOLUME_MUTE = 8, MEDIA_PREVIOUS = 16, MEDIA_NEXT = 32, MEDIA_TURN_ON = 128, MEDIA_PLAY = 16384;
constexpr uint32_t VACUUM_TURN_ON = 1, VACUUM_TURN_OFF = 2, VACUUM_PAUSE = 4, VACUUM_STOP = 8, VACUUM_RETURN = 16, VACUUM_START = 8192;
}
// Material Design Icons glyphs the icon fonts carry (tile_icons.py FIXED).
namespace glyph {
constexpr const char *PLAY = "\U000F040A", *PAUSE = "\U000F03E4", *STOP = "\U000F04DB", *NEXT = "\U000F04AD", *PREVIOUS = "\U000F04AE";
constexpr const char *VOLUME = "\U000F057E", *MUTED = "\U000F0581", *UP = "\U000F005D", *DOWN = "\U000F0045";
constexpr const char *EXPAND = "\U000F084E", *COLLAPSE = "\U000F084C", *DOCK = "\U000F05F8", *PLUS = "\U000F0415", *MINUS = "\U000F0374";
constexpr const char *LEFT = "\U000F0141", *RIGHT = "\U000F0142", *CLOSE = "\U000F0156", *POWER = "\U000F0425", *FIRE = "\U000F0238";
constexpr const char *SNOWFLAKE = "\U000F0717", *HEAT_COOL = "\U000F1A79", *AUTO = "\U000F1B17", *DRY = "\U000F058E", *FAN = "\U000F0210";
constexpr const char *BLINDS_OPEN = "\U000F1011", *BLINDS = "\U000F00AC";
}
enum Command {
  NONE = 0, COVER_OPEN, COVER_STOP, COVER_CLOSE, VACUUM_START, VACUUM_PAUSE, VACUUM_STOP, VACUUM_DOCK,
  MEDIA_PREVIOUS, MEDIA_PLAY_PAUSE, MEDIA_NEXT, MEDIA_MUTE, TIMER_START, TIMER_PAUSE, TIMER_CANCEL,
  HVAC_MODE, SELECT_PREVIOUS, SELECT_NEXT, RUN, TOGGLE, STEP_DOWN, STEP_UP,
  COVER_OPEN_TILT, COVER_STOP_TILT, COVER_CLOSE_TILT
};
struct Key { const char *icon = ""; int command = NONE; std::string arg; bool checked = false, disabled = false; };
struct Action { std::string service, key, value; bool valid() const { return !service.empty(); } };
using runtime_tiles::Tile;

// Panel kinds: keys (a row of pill buttons), stepper (-/+ pill), slider, toggle, run.
inline bool is_key_row(const std::string &c) { return c == "buttons" || c == "mode" || c == "playback" || c == "chevrons"; }
inline bool is_slider(const std::string &c) { return c == "volume" || c == "brightness" || c == "speed" || c == "position" || c == "slider"; }
// A select's "stepper" is a pair of chevron keys; numbers and climate get the -/+ pill.
inline std::string panel_kind(const Tile &t) {
  auto c = t.controls, d = t.domain();
  if (c == "stepper" && (d == "select" || d == "input_select")) return "chevrons";
  return c;
}
inline bool has_mode(const std::string &hvac_modes, const char *mode) { return hvac_modes.find("\"" + std::string(mode) + "\"") != std::string::npos; }
// Home Assistant's colour for a climate mode (the card and its mode keys use it).
inline uint32_t mode_color(const std::string &mode) {
  using namespace theme::ha;
  if (mode == "heat") return DEEP_ORANGE;
  if (mode == "cool") return BLUE;
  if (mode == "heat_cool") return AMBER;
  if (mode == "auto") return GREEN;
  if (mode == "fan_only") return CYAN;
  if (mode == "dry") return ORANGE;
  return GREY;
}
inline const char *mode_icon(const std::string &mode) {
  if (mode == "off") return glyph::POWER;
  if (mode == "heat") return glyph::FIRE;
  if (mode == "cool") return glyph::SNOWFLAKE;
  if (mode == "heat_cool") return glyph::HEAT_COOL;
  if (mode == "auto") return glyph::AUTO;
  if (mode == "dry") return glyph::DRY;
  return glyph::FAN;
}
// Covers that move sideways get the horizontal arrows, like Home Assistant.
inline bool sideways_cover(const std::string &device_class) {
  return device_class == "curtain" || device_class == "awning" || device_class == "door" || device_class == "gate";
}
inline float numeric_state(const Tile &t) {
  char *end; float value = std::strtof(t.state.c_str(), &end);
  return end != t.state.c_str() && std::isfinite(value) ? value : NAN;
}
// Value the -/+ pill edits: the climate setpoint or the number itself.
inline float edit_target(const Tile &t) { return t.domain() == "climate" ? t.target : numeric_state(t); }
inline float edit_step(const Tile &t) {
  float step = t.step;
  if (!std::isfinite(step) || step <= 0) step = t.domain() == "climate" ? 0.5f : 1.0f;
  return step;
}
// One step up or down, snapped to the entity's grid and kept inside its range.
inline float step_value(float current, float step, float minimum, float maximum, int direction) {
  if (!std::isfinite(step) || step <= 0) step = 1;
  bool has_min = std::isfinite(minimum), has_max = std::isfinite(maximum) && (!has_min || maximum >= minimum);
  float origin = has_min ? minimum : 0;
  if (!std::isfinite(current)) current = has_min ? minimum : 0;
  float next = origin + std::round((current + direction * step - origin) / step) * step;
  if (has_min) next = std::fmax(next, minimum);
  if (has_max) next = std::fmin(next, maximum);
  return next;
}
// Whole degrees when the entity steps by whole units, one decimal otherwise.
inline std::string format_value(float value, float step, const char *suffix) {
  if (!std::isfinite(value)) return "--";
  char b[24]; snprintf(b, sizeof(b), step >= 1 ? "%.0f%s" : "%.1f%s", value, suffix); return b;
}
inline const char *climate_mode_text(const std::string &mode) {
  if (mode == "off") return "Off";
  if (mode == "heat") return "Heat";
  if (mode == "cool") return "Cool";
  if (mode == "heat_cool") return "Heat/Cool";
  if (mode == "auto") return "Auto";
  if (mode == "dry") return "Dry";
  if (mode == "fan_only") return "Fan only";
  return mode.c_str();
}
inline const char *climate_action_text(const std::string &action) {
  if (action == "heating") return "Heating";
  if (action == "cooling") return "Cooling";
  if (action == "idle") return "Idle";
  if (action == "off") return "Off";
  if (action == "drying") return "Drying";
  if (action == "fan") return "Fan";
  if (action == "preheating") return "Preheating";
  if (action == "defrosting") return "Defrosting";
  return "";
}
inline const char *cover_state_text(const std::string &state) {
  if (state == "open") return "Open";
  if (state == "closed") return "Closed";
  if (state == "opening") return "Opening";
  if (state == "closing") return "Closing";
  return state.c_str();
}
inline const char *media_state_text(const std::string &state) {
  if (state == "playing") return "Playing";
  if (state == "paused") return "Paused";
  if (state == "idle") return "Idle";
  if (state == "standby") return "Standby";
  if (state == "buffering") return "Loading";
  if (state == "on") return "On";
  if (state == "off") return "Off";
  return state.c_str();
}
// A binary sensor's state in Home Assistant's words for its device class: a door is Open or Closed, a leak sensor
// Wet or Dry. The same words as the add-on's header_bar.BINARY_STATES, which the top bar and the history card show;
// tests/test_binary_words.py keeps the two tables equal. Without a class, or with one this table lacks: On and Off.
struct BinaryWords { const char *device_class, *on, *off; };
constexpr BinaryWords BINARY_WORDS[] = {
    {"battery", "Low", "Normal"}, {"battery_charging", "Charging", "Not charging"},
    {"carbon_monoxide", "Danger", "Safe"}, {"cold", "Cold", "Normal"}, {"connectivity", "Connected", "Disconnected"},
    {"door", "Open", "Closed"}, {"garage_door", "Open", "Closed"}, {"gas", "Danger", "Safe"},
    {"heat", "Hot", "Normal"}, {"light", "Light", "Dark"}, {"lock", "Open", "Locked"}, {"moisture", "Wet", "Dry"},
    {"motion", "Motion", "No motion"}, {"moving", "Moving", "Still"}, {"occupancy", "Occupied", "Clear"},
    {"opening", "Open", "Closed"}, {"plug", "Plugged in", "Unplugged"}, {"power", "On", "Off"},
    {"presence", "Home", "Away"}, {"problem", "Problem", "OK"}, {"running", "Active", "Inactive"},
    {"safety", "Unsafe", "Safe"}, {"smoke", "Smoke", "No smoke"}, {"sound", "Sound", "Silent"},
    {"tamper", "Tampering", "OK"}, {"update", "Update", "Up to date"}, {"vibration", "Vibration", "Still"},
    {"window", "Open", "Closed"},
};
inline const char *binary_state_text(const std::string &device_class, bool on) {
  for (const auto &words : BINARY_WORDS)
    if (device_class == words.device_class) return on ? words.on : words.off;
  return on ? "On" : "Off";
}
// Status line beside a control panel: what Home Assistant shows under the name.
inline std::string status_text(const Tile &t) {
  auto d = t.domain(); char b[48];
  if (d == "climate") {
    std::string text = climate_action_text(t.extra().hvac_action);
    if (text.empty()) text = climate_mode_text(t.state);
    if (std::isfinite(t.current)) { snprintf(b, sizeof(b), " · %.1f°", t.current); text += b; }
    return text;
  }
  if (d == "cover") {
    std::string text = cover_state_text(t.state);
    if (std::isfinite(t.position)) { snprintf(b, sizeof(b), " · %d%%", (int) std::lround(t.position)); text += b; }
    return text;
  }
  if (d == "media_player") {
    const std::string &title = t.extra().media_title;
    std::string text = (t.state == "playing" || t.state == "paused") && !title.empty() ? title : media_state_text(t.state);
    if (std::isfinite(t.volume) && (t.supported & feature::MEDIA_VOLUME_SET)) { snprintf(b, sizeof(b), " · %d%%", (int) std::lround(t.volume * 100)); text += b; }
    return text;
  }
  return {};
}
// The cover card (firmware 0.2.50+) offers what Home Assistant's own dialog does: a position slider when the
// cover reports positions, a tilt slider for slats, open, stop and close, and the tilt keys of slats that
// tilt without a position. A cover whose features are not known yet gets the three keys.
struct CoverCard { bool position = false, tilt = false, keys = false, tilt_keys = false; };
inline CoverCard cover_card(const Tile &t) {
  CoverCard card;
  const uint32_t f = t.supported;
  if (!f) { card.keys = true; return card; }
  card.position = f & feature::COVER_POSITION;
  card.tilt = f & feature::COVER_TILT_POSITION;
  card.keys = f & (feature::COVER_OPEN | feature::COVER_CLOSE | feature::COVER_STOP);
  card.tilt_keys = !card.tilt && (f & (feature::COVER_OPEN_TILT | feature::COVER_CLOSE_TILT | feature::COVER_STOP_TILT));
  return card;
}
// The card's status line: the state, the position, and the tilt where the cover has one.
inline std::string cover_card_status(const Tile &t) {
  std::string text = status_text(t);
  if ((t.supported & feature::COVER_TILT_POSITION) && std::isfinite(t.extra().tilt)) {
    char b[24]; snprintf(b, sizeof(b), " · Tilt %d%%", (int) std::lround(t.extra().tilt)); text += b;
  }
  return text;
}
// Open, stop and close as the cover supports them (all three while its features are unknown). A key that
// cannot move the cover further is disabled, unless the cover is on its way the other way.
inline unsigned cover_keys(const Tile &t, std::array<Key, 3> &out) {
  unsigned n = 0;
  const uint32_t f = t.supported ? t.supported : feature::COVER_OPEN | feature::COVER_STOP | feature::COVER_CLOSE;
  bool sideways = sideways_cover(t.device_class);
  bool fully_open = std::isfinite(t.position) ? t.position >= 99.5f : t.state == "open";
  bool fully_closed = std::isfinite(t.position) ? t.position <= 0.5f : t.state == "closed";
  if (f & feature::COVER_OPEN) out[n++] = Key{sideways ? glyph::EXPAND : glyph::UP, COVER_OPEN, "", t.state == "opening", fully_open && t.state != "closing"};
  if (f & feature::COVER_STOP) out[n++] = Key{glyph::STOP, COVER_STOP, "", false, false};
  if (f & feature::COVER_CLOSE) out[n++] = Key{sideways ? glyph::COLLAPSE : glyph::DOWN, COVER_CLOSE, "", t.state == "closing", fully_closed && t.state != "opening"};
  return n;
}
inline unsigned cover_tilt_keys(const Tile &t, std::array<Key, 3> &out) {
  unsigned n = 0;
  const float tilt = t.extra().tilt;
  if (t.supported & feature::COVER_OPEN_TILT) out[n++] = Key{glyph::BLINDS_OPEN, COVER_OPEN_TILT, "", false, std::isfinite(tilt) && tilt >= 99.5f};
  if (t.supported & feature::COVER_STOP_TILT) out[n++] = Key{glyph::STOP, COVER_STOP_TILT, "", false, false};
  if (t.supported & feature::COVER_CLOSE_TILT) out[n++] = Key{glyph::BLINDS, COVER_CLOSE_TILT, "", false, std::isfinite(tilt) && tilt <= 0.5f};
  return n;
}
// The row of up to three pill keys for a key-row panel; returns how many.
inline unsigned keys_for(const Tile &t, std::array<Key, 3> &out) {
  auto d = t.domain(); auto c = panel_kind(t); unsigned n = 0;
  auto add = [&](const char *icon, int command, bool disabled = false, bool checked = false, const std::string &arg = "") {
    if (n < out.size()) out[n++] = Key{icon, command, arg, checked, disabled};
  };
  if (c == "buttons" && d == "cover") {
    // The tile keys are the card's keys, without the direction a moving cover shows there.
    if (t.supported) for (unsigned i = 0, count = cover_keys(t, out); i < count; ++i) { out[i].checked = false; ++n; }
  } else if (c == "buttons" && d == "vacuum") {
    bool cleaning = t.state == "cleaning";
    if (cleaning && (t.supported & feature::VACUUM_PAUSE)) add(glyph::PAUSE, VACUUM_PAUSE);
    else if (cleaning && (t.supported & feature::VACUUM_TURN_OFF) && !(t.supported & feature::VACUUM_START)) add(glyph::PAUSE, VACUUM_STOP);
    else if (t.supported & (feature::VACUUM_START | feature::VACUUM_TURN_ON)) add(glyph::PLAY, VACUUM_START, cleaning);
    if (t.supported & feature::VACUUM_STOP) add(glyph::STOP, VACUUM_STOP, !cleaning && t.state != "returning");
    if (t.supported & feature::VACUUM_RETURN) add(glyph::DOCK, VACUUM_DOCK, t.state == "docked" || t.state == "returning");
  } else if (c == "buttons" && d == "timer") {
    bool active = t.state == "active";
    add(active ? glyph::PAUSE : glyph::PLAY, active ? TIMER_PAUSE : TIMER_START);
    add(glyph::CLOSE, TIMER_CANCEL, t.state == "idle");
  } else if (c == "playback") {
    if (t.supported & feature::MEDIA_PREVIOUS) add(glyph::PREVIOUS, MEDIA_PREVIOUS);
    if (t.supported & (feature::MEDIA_PLAY | feature::MEDIA_PAUSE)) add(t.state == "playing" ? glyph::PAUSE : glyph::PLAY, MEDIA_PLAY_PAUSE);
    if (t.supported & feature::MEDIA_NEXT) add(glyph::NEXT, MEDIA_NEXT);
  } else if (c == "mode") {
    // Three keys fit beside the name; Home Assistant lists modes in device order.
    for (const char *mode : {"off", "heat", "cool", "heat_cool", "auto", "dry", "fan_only"})
      if (n < 3 && has_mode(t.extra().hvac_modes, mode)) add(mode_icon(mode), HVAC_MODE, false, t.state == mode, mode);
  } else if (c == "chevrons") {
    add(glyph::LEFT, SELECT_PREVIOUS, t.extra().options.size() < 2);
    add(glyph::RIGHT, SELECT_NEXT, t.extra().options.size() < 2);
  }
  return n;
}
inline std::string neighbour_option(const Tile &t, int direction) {
  const auto &options = t.extra().options;
  if (options.empty()) return {};
  int current = 0, count = (int) options.size();
  for (int i = 0; i < count; ++i) if (options[i] == t.state) current = i;
  return options[(current + direction + count) % count];
}
inline const char *run_label(const std::string &domain) {
  if (domain == "scene") return "Activate";
  if (domain == "script") return "Run";
  return "Press";
}
// The Home Assistant action behind a key. STEP_* are handled locally (debounced) and return nothing.
inline Action key_action(const Tile &t, int command, const std::string &arg = "") {
  auto d = t.domain();
  switch (command) {
    case COVER_OPEN: return {"cover.open_cover", "", ""};
    case COVER_STOP: return {"cover.stop_cover", "", ""};
    case COVER_CLOSE: return {"cover.close_cover", "", ""};
    case COVER_OPEN_TILT: return {"cover.open_cover_tilt", "", ""};
    case COVER_STOP_TILT: return {"cover.stop_cover_tilt", "", ""};
    case COVER_CLOSE_TILT: return {"cover.close_cover_tilt", "", ""};
    case VACUUM_START: return {(t.supported & feature::VACUUM_START) ? "vacuum.start" : "vacuum.turn_on", "", ""};
    case VACUUM_PAUSE: return {"vacuum.pause", "", ""};
    case VACUUM_STOP: return {(t.supported & feature::VACUUM_STOP) ? "vacuum.stop" : "vacuum.turn_off", "", ""};
    case VACUUM_DOCK: return {"vacuum.return_to_base", "", ""};
    case MEDIA_PREVIOUS: return {"media_player.media_previous_track", "", ""};
    case MEDIA_PLAY_PAUSE: return {"media_player.media_play_pause", "", ""};
    case MEDIA_NEXT: return {"media_player.media_next_track", "", ""};
    case MEDIA_MUTE: return {"media_player.volume_mute", "is_volume_muted", t.muted ? "false" : "true"};
    case TIMER_START: return {"timer.start", "", ""};
    case TIMER_PAUSE: return {"timer.pause", "", ""};
    case TIMER_CANCEL: return {"timer.cancel", "", ""};
    case HVAC_MODE: return arg.empty() ? Action{} : Action{"climate.set_hvac_mode", "hvac_mode", arg};
    case SELECT_PREVIOUS: case SELECT_NEXT: {
      std::string option = neighbour_option(t, command == SELECT_NEXT ? 1 : -1);
      return option.empty() ? Action{} : Action{d + ".select_option", "option", option};
    }
    case RUN:
      if (d == "scene" || d == "script") return {d + ".turn_on", "", ""};
      if (d == "button" || d == "input_button") return {d + ".press", "", ""};
      return {};
    case TOGGLE:
      if (d == "light" || d == "switch" || d == "input_boolean" || d == "fan") return {d + (t.state == "on" ? ".turn_off" : ".turn_on"), "", ""};
      return {};
    default: return {};
  }
}
// ---- Vacuum card rows (firmware 0.2.39+) ----
// The value a row shows as chosen: the one just tapped while Home Assistant has not answered yet.
inline const std::string &shown_value(const Tile &t, const runtime_tiles::Choice &c, uint32_t now) {
  return !c.sent.empty() && t.waiting(now) ? c.sent : c.current;
}
// Role of the cleaning mode in use: 'v' vacuum only, 'm' mop only, 'b' both, 'a' automatic; 'b' when
// the robot has no mode select or reports a mode the manager did not know.
inline char vacuum_role(const Tile &t, uint32_t now) {
  const auto *mode = t.choice('m');
  if (!mode) return 'b';
  const std::string &value = shown_value(t, *mode, now);
  for (size_t i = 0; i < mode->values.size() && i < mode->roles.size(); ++i)
    if (mode->values[i] == value) return mode->roles[i];
  return 'b';
}
// Rows under the mode: suction unless the robot only mops, water (when the robot has a water select)
// unless it only vacuums; an automatic mode leaves both to the robot or its app.
struct VacuumRows { bool suction = false, water = false; };
inline VacuumRows vacuum_rows(const Tile &t, uint32_t now) {
  char role = vacuum_role(t, now);
  VacuumRows rows;
  rows.suction = role != 'm' && role != 'a' && t.choice('s');
  rows.water = role != 'v' && role != 'a' && t.choice('w');
  return rows;
}
// Labels for a manager that sends no suction row (app before 0.2.46): the vacuum's own speed names.
inline std::string speed_label(const std::string &speed) {
  if (speed == "quiet") return "Quiet";
  if (speed == "balanced") return "Normal";
  if (speed == "turbo") return "Turbo";
  if (speed == "max") return "Max";
  return speed;
}
// The suction row a vacuum card shows: the manager's (filtered, labelled) speeds, else the first four
// of the vacuum's own list. Its value is always the vacuum's fan_speed.
inline void settle_suction(runtime_tiles::Extra &x) {
  auto *row = x.choice('s');
  if (!row && !x.fan_speeds.empty()) {
    runtime_tiles::Choice legacy; legacy.kind = 's';
    for (const auto &speed : x.fan_speeds) { legacy.values.push_back(speed); legacy.labels.push_back(speed_label(speed)); }
    x.choices.push_back(std::move(legacy));
    row = &x.choices.back();
  }
  if (row) row->current = x.fan_speed;
}
// Whether a light runs an effect worth naming on its tile (firmware 0.2.70+): WLED's "Solid" is its plain colour, a Hue's
// "off" and Home Assistant's "None" mean no effect.
inline bool effect_running(const std::string &effect) {
  std::string lower;
  for (char c : effect) lower += static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
  while (!lower.empty() && lower.back() == ' ') lower.pop_back();
  return !lower.empty() && lower != "solid" && lower != "off" && lower != "none" && lower != "unknown" && lower != "unavailable";
}
// The service call behind a chip: a select option on the device, or the vacuum's own fan speed.
inline Action choice_action(const Tile &t, char kind, const std::string &value) {
  if (kind == 's') return {"vacuum.set_fan_speed", "fan_speed", value};
  const auto *row = t.choice(kind);
  if (!row || row->entity.empty()) return {};
  return {"select.select_option", "option", value};
}

// The debounced -/+ edit lands as one service call.
inline Action edit_action(const Tile &t, float value) {
  if (!std::isfinite(value)) return {};
  char b[24]; snprintf(b, sizeof(b), "%.2f", value);
  std::string text = b;
  while (text.size() > 1 && text.back() == '0') text.pop_back();
  if (text.back() == '.') text.pop_back();
  auto d = t.domain();
  if (d == "climate") return {"climate.set_temperature", "temperature", text};
  if (d == "number" || d == "input_number") return {d + ".set_value", "value", text};
  return {};
}

// ---- What a finger on a tile does ----
// The tile's own `tap` choice comes first. `toggle` sends <domain>.toggle on every domain (firmware 0.2.58+): the app
// only offers it where Home Assistant lists that action for the entity, such as a cover, which stops while it moves.
// Otherwise the domain decides, as before: holding or `detail` opens a card, a short tap switches, runs or presses.
// CARD is the runtime detail card (show_detail), OVERLAY the board's own light, fan and climate card; `busy` marks the
// tile busy for a moment while the card opens.
// CUSTOM (firmware 0.2.58+) is an action of the tile's own choosing from Home Assistant's list, with its data.
enum class TapRoute : uint8_t { NONE, ACTION, CARD, OVERLAY, CUSTOM };
struct Tap { TapRoute route = TapRoute::NONE; std::string service; bool busy = false; };
inline bool runtime_card_domain(const std::string &d) {
  return d == "sensor" || d == "binary_sensor" || d == "weather" || d == "number" || d == "input_number" || d == "select" ||
         d == "input_select" || d == "media_player" || d == "vacuum" || d == "cover" || d == "sun" || d == "person" || d == "timer";
}
inline Tap tap_route(const Tile &t, bool hold) {
  const std::string d = t.domain();
  if (t.tap == "none") return {};
  // A tile whose action didn't arrive (an app before 0.2.67) taps automatically, as older firmware does.
  if (!hold && t.tap == "action" && !t.extra().action.empty()) return {TapRoute::CUSTOM, t.extra().action};
  if (!hold && t.tap == "toggle") return {TapRoute::ACTION, d + ".toggle"};
  bool open = hold || t.tap == "detail" || d == "climate" || d == "vacuum" || d == "cover";
  // A short tap runs or pauses the timer; holding opens the card with a cancel button.
  if (d == "timer" && !open) return {TapRoute::ACTION, t.state == "active" ? "timer.pause" : "timer.start"};
  if (runtime_card_domain(d)) return {TapRoute::CARD, "", true};
  if (open) return d == "light" || d == "climate" || d == "fan" ? Tap{TapRoute::OVERLAY, "", true} : Tap{TapRoute::CARD, "", false};
  if (d == "light" || d == "switch" || d == "input_boolean" || d == "fan") return {TapRoute::ACTION, d + ".toggle"};
  if (d == "scene" || d == "script") return {TapRoute::ACTION, d + ".turn_on"};
  if (d == "button" || d == "input_button") return {TapRoute::ACTION, d + ".press"};
  return {};
}
}  // namespace tile_controls
