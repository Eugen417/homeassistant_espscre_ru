#pragma once
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>

namespace runtime_tiles {
constexpr size_t MAX_TILES = 20;
constexpr size_t SLOTS_PER_PAGE = 6;
// Explicit grid positions (0.2.26+) address at most eight pages of six slots.
constexpr size_t MAX_PAGES = 8;
constexpr size_t MAX_SLOTS = MAX_PAGES * SLOTS_PER_PAGE;
inline bool valid_entity(const std::string &entity) {
  if (entity.size() > 120) return false;
  auto dot = entity.find('.');
  if (dot == std::string::npos || dot == 0 || dot + 1 == entity.size()) return false;
  for (size_t i = 0; i < entity.size(); ++i)
    if (i != dot && !(entity[i] >= 'a' && entity[i] <= 'z') &&
        !(entity[i] >= '0' && entity[i] <= '9') && entity[i] != '_') return false;
  std::string domain = entity.substr(0, dot);
  // screen.* are built-in cards without a Home Assistant entity behind them.
  if (domain == "screen") return entity == "screen.clock" || entity == "screen.settings";
  for (const auto *allowed : {"light", "switch", "input_boolean", "scene", "script", "climate", "vacuum", "fan", "cover", "sensor", "binary_sensor", "input_select", "select", "number", "input_number", "weather", "media_player", "button", "input_button", "sun", "timer", "person"})
    if (domain == allowed) return true;
  return false;
}
// A tile keeps a fingerprint (FNV-1a) of its last state message instead of a copy of it. It is also an
// ArduinoJson writer: the firmware hashes the attributes while serializing them, without a string.
struct Fingerprint {
  uint32_t value = 2166136261u;
  size_t write(uint8_t c) { value = (value ^ c) * 16777619u; return 1; }
  size_t write(const uint8_t *data, size_t size) { for (size_t i = 0; i < size; ++i) write(data[i]); return size; }
  void add(const std::string &text) { write(reinterpret_cast<const uint8_t *>(text.data()), text.size()); }
};
inline uint32_t state_revision(const std::string &state, const std::string &attributes) {
  Fingerprint f;
  f.add(state);
  f.write('\n');
  f.add(attributes);
  return f.value;
}
struct Forecast { std::string day, condition; float high = NAN, low = NAN, rain = NAN, mm = NAN; };
struct Hour { std::string time, condition; float temp = NAN, rain = NAN, mm = NAN; };
// One row of choices on a vacuum card (firmware 0.2.39+), found by the manager on the robot's device:
// kind 'm' a cleaning mode select (Roborock: vacuum, mop or both), 'w' a water or mop intensity
// select, 's' the suction speeds of the vacuum itself. `values` go to Home Assistant, `labels` are
// shown; a cleaning mode carries one role letter per value (v vacuum only, m mop only, b both,
// a automatic: the robot or the app decides suction and water). `sent` is the value just tapped.
struct Choice {
  char kind = 0;
  std::string entity, current, roles, sent;
  std::vector<std::string> values, labels;
};
// What only some tiles carry: climate modes, a select's options, weather, sun and timer times, a media
// title, the vacuum rows. A light or a sensor has none of it, so a tile holds this block only while its
// state needs one: twenty tiles with these fields inline took 24 KB of the CYD's RAM, mostly empty.
struct Extra {
  // Climate: the modes as JSON lists, the current fan and swing mode, and what it is doing now.
  std::string hvac_modes, fan_modes, swing_modes, fan_mode, swing_mode, hvac_action;
  // A select's options, at most eight.
  std::vector<std::string> options;
  // Weather: up to five days and eight hours.
  std::vector<Forecast> forecast;
  std::vector<Hour> hours;
  float wind = NAN, feels = NAN;
  std::string wind_unit;
  std::string sunrise, sunset, duration, remaining;
  uint32_t timer_end = 0;
  std::string media_title;
  // Vacuum: its own speeds (at most four) and speed, the mode, water and suction rows (see Choice), and
  // from sensors of its device the room it is in and whether it charges.
  std::vector<std::string> fan_speeds;
  std::string fan_speed;
  std::vector<Choice> choices;
  std::string room;
  bool charging = false;
  // Cover (firmware 0.2.50+): the tilt of its slats, 0 closed to 100 open.
  float tilt = NAN;
  Choice *choice(char kind) { for (auto &c : choices) if (c.kind == kind) return &c; return nullptr; }
  bool empty() const {
    return hvac_modes.empty() && fan_modes.empty() && swing_modes.empty() && fan_mode.empty() && swing_mode.empty() &&
           hvac_action.empty() && options.empty() && forecast.empty() && hours.empty() && std::isnan(wind) &&
           std::isnan(feels) && wind_unit.empty() && sunrise.empty() && sunset.empty() && duration.empty() &&
           remaining.empty() && !timer_end && media_title.empty() && fan_speeds.empty() && fan_speed.empty() &&
           choices.empty() && room.empty() && !charging && std::isnan(tilt);
  }
};
// The Extra of a tile on the heap, copied along with the tile like an ordinary member.
struct ExtraBox {
  std::unique_ptr<Extra> ptr;
  ExtraBox() = default;
  ExtraBox(const ExtraBox &other) : ptr(other.ptr ? new Extra(*other.ptr) : nullptr) {}
  ExtraBox &operator=(const ExtraBox &other) { if (this != &other) ptr.reset(other.ptr ? new Extra(*other.ptr) : nullptr); return *this; }
  ExtraBox(ExtraBox &&) noexcept = default;
  ExtraBox &operator=(ExtraBox &&) noexcept = default;
};
struct Tile {
  std::string entity, name, state, unit, modes;
  float brightness = NAN, percentage = NAN, position = NAN;
  float current = NAN, target = NAN, humidity = NAN, minimum = 7, maximum = 35, step = 0.5f;
  int hue = 0, kelvin = 3000, min_kelvin = 0, max_kelvin = 0;
  bool received = false;
  bool has_hs_color = false;
  int saturation = 0;
  std::string tap = "auto", display = "standard", inline_control = "none";
  bool wide = false;
  // Direct control set on a wide card (firmware 0.2.19+); empty keeps the plain card.
  std::string controls, device_class;
  bool muted = false;
  // A -/+ edit shows at once and is sent as one call after a short pause; the
  // value stays until Home Assistant reports it (or a timeout clears it).
  float edit_value = NAN; uint32_t edit_since = 0; bool edit_sent = false;
  // Knob position a toggle shows while its command is under way.
  bool optimistic_on = false;
  // A sensor's graph: 24 samples over `history_hours`, empty without one.
  unsigned history_hours = 24;
  std::vector<float> history;
  bool has_history = false;
  // When a scene, script or button last ran (unix time), pre-computed by the manager.
  uint32_t last_run = 0;
  float battery = NAN, volume = NAN;
  uint32_t supported = 0, background = 0;
  bool transparent = false;  // "Background: none": card fill and border hidden, contents unchanged.
  std::string icon;  // UTF-8 glyph of a chosen icon the icon fonts contain; empty keeps the domain icon.
  uint32_t revision = 0, pending_revision = 0;  // state_revision() fingerprints
  uint32_t pending_since = 0;
  bool pending = false, confirmed = false, local_feedback = false;
  ExtraBox extra_box;
  const Extra &extra() const { static const Extra none; return extra_box.ptr ? *extra_box.ptr : none; }
  Extra *extra_ptr() { return extra_box.ptr.get(); }
  Extra &edit_extra() { if (!extra_box.ptr) extra_box.ptr.reset(new Extra()); return *extra_box.ptr; }
  // A state message's extras replace the block: it stays allocated while the tile needs one and is freed
  // when a state brings none.
  void set_extra(Extra &&next) {
    if (next.empty()) extra_box.ptr.reset();
    else if (extra_box.ptr) *extra_box.ptr = std::move(next);
    else extra_box.ptr.reset(new Extra(std::move(next)));
  }
  Choice *choice(char kind) { return extra_box.ptr ? extra_box.ptr->choice(kind) : nullptr; }
  const Choice *choice(char kind) const { for (auto &c : extra().choices) if (c.kind == kind) return &c; return nullptr; }
  bool is_switch() const { return domain()=="switch" || domain()=="input_boolean"; }
  bool loading(uint32_t now) const { return pending && (now-pending_since < (is_switch()?150u:1000u) || (!confirmed && !local_feedback && now-pending_since < 6000)); }
  bool awaiting_action(uint32_t now) const { return loading(now) && !local_feedback; }
  void begin(uint32_t now, bool local=false) { pending=true; pending_since=now; confirmed=false; local_feedback=local; pending_revision=revision; }
  void observe(uint32_t next) { revision=next; if (pending && revision!=pending_revision) confirmed=true; }
  std::string domain() const { return entity.substr(0, entity.find('.')); }
  bool builtin() const { return domain() == "screen"; }
  // Two built-in cards, and only one of them is a clock that has to be redrawn every minute.
  bool is_clock() const { return entity == "screen.clock"; }
  bool is_settings() const { return entity == "screen.settings"; }
  bool available() const { return builtin() || (received && state != "unknown" && state != "unavailable" && !state.empty()); }
  bool active() const {
    return state == "on" || state == "cleaning" || state == "active" || (domain() == "person" && state == "home") ||
           (domain() == "sun" && state == "above_horizon") || (domain() == "climate" && available() && state != "off");
  }
};
// Slot position of a tile within the fixed two-column, three-row pages.
struct Placement { uint8_t page = 0, slot = 0; };
// Wide tiles start in the left column and take the whole row; a right-column
// gap before them stays empty. Returns the page count (at least one).
inline unsigned pack(const std::array<Tile, MAX_TILES> &tiles, size_t count, std::array<Placement, MAX_TILES> &out) {
  unsigned position = 0;
  for (size_t i = 0; i < count && i < MAX_TILES; ++i) {
    if (tiles[i].wide && position % 2 == 1) ++position;
    out[i] = {static_cast<uint8_t>(position / SLOTS_PER_PAGE), static_cast<uint8_t>(position % SLOTS_PER_PAGE)};
    position += tiles[i].wide ? 2 : 1;
  }
  unsigned pages = (position + SLOTS_PER_PAGE - 1) / SLOTS_PER_PAGE;
  return pages ? pages : 1;
}
// Grid position per tile: the explicit slots when the manager sent them (a wide
// card always starts in the left column), else the in-order packing. Returns the
// page count (at least one); an empty page between two used ones stays a page.
struct Model;
inline unsigned place(const Model &m, std::array<Placement, MAX_TILES> &out);
struct Model {
  std::array<Tile, MAX_TILES> tiles;
  // Absolute grid slot per tile when the manager sent `slots` (0.2.26+): gaps stay
  // empty and a tile keeps its place. Without them the tiles pack in order.
  std::array<uint8_t, MAX_TILES> slots{};
  bool explicit_slots = false;
  // Pages the manager wants shown even when the last ones are still empty (0.2.26+).
  uint8_t pages = 1;
  size_t count = 0;
  std::string title = "Choose tiles in HA";
  bool configured = false;
  bool set_layout(const std::vector<std::string> &entities, const std::string &name, bool &changed) {
    bool moved = false;
    return set_layout(entities, name, changed, {}, moved);
  }
  // `changed`: the tiles differ, states restart. `moved`: same tiles on other
  // positions, the pages re-place without touching states or an open card.
  bool set_layout(const std::vector<std::string> &entities, const std::string &name, bool &changed,
                  const std::vector<uint8_t> &positions, bool &moved) {
    if (entities.size() > MAX_TILES || name.size() > 96) return false;
    for (size_t i = 0; i < entities.size(); ++i) {
      if (!valid_entity(entities[i])) return false;
      for (size_t j = 0; j < i; ++j) if (entities[i] == entities[j]) return false;
    }
    if (!positions.empty()) {
      if (positions.size() != entities.size()) return false;
      for (size_t i = 0; i < positions.size(); ++i) {
        if (positions[i] >= MAX_SLOTS) return false;
        for (size_t j = 0; j < i; ++j) if (positions[i] == positions[j]) return false;
      }
    }
    changed = !configured || count != entities.size();
    for (size_t i = 0; i < entities.size(); ++i) if (tiles[i].entity != entities[i]) changed = true;
    moved = !changed && (explicit_slots != !positions.empty());
    for (size_t i = 0; i < positions.size() && !changed; ++i) if (slots[i] != positions[i]) moved = true;
    // A title-only update must not interrupt an open control card.
    title = name.empty() ? "Home" : name;
    if (changed) {
      for (auto &tile : tiles) tile = Tile{};
      count = entities.size();
      for (size_t i = 0; i < count; ++i) tiles[i].entity = entities[i];
    }
    explicit_slots = !positions.empty();
    slots.fill(0);
    for (size_t i = 0; i < positions.size(); ++i) slots[i] = positions[i];
    configured = true;
    return true;
  }
  bool ready() const {
    if (!configured) return false;
    for (size_t i = 0; i < count; ++i) if (!tiles[i].received) return false;
    return true;
  }
  bool accepts(size_t index, const std::string &entity) const {
    return configured && index < count && tiles[index].entity == entity;
  }
};
inline unsigned place(const Model &m, std::array<Placement, MAX_TILES> &out) {
  if (!m.explicit_slots) return pack(m.tiles, m.count, out);
  unsigned last = 0;
  for (size_t i = 0; i < m.count && i < MAX_TILES; ++i) {
    unsigned slot = m.slots[i];
    if (m.tiles[i].wide) slot &= ~1u;
    out[i] = {static_cast<uint8_t>(slot / SLOTS_PER_PAGE), static_cast<uint8_t>(slot % SLOTS_PER_PAGE)};
    last = std::max(last, slot + (m.tiles[i].wide ? 2u : 1u));
  }
  unsigned pages = (last + SLOTS_PER_PAGE - 1) / SLOTS_PER_PAGE;
  return std::max({pages, 1u, std::min<unsigned>(m.pages, MAX_PAGES)});
}
}  // namespace runtime_tiles
