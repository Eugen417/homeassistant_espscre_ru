#pragma once
#include <array>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>

namespace runtime_tiles {
constexpr size_t MAX_TILES = 20;
constexpr size_t SLOTS_PER_PAGE = 6;
inline bool valid_entity(const std::string &entity) {
  if (entity.size() > 120) return false;
  auto dot = entity.find('.');
  if (dot == std::string::npos || dot == 0 || dot + 1 == entity.size()) return false;
  for (size_t i = 0; i < entity.size(); ++i)
    if (i != dot && !(entity[i] >= 'a' && entity[i] <= 'z') &&
        !(entity[i] >= '0' && entity[i] <= '9') && entity[i] != '_') return false;
  std::string domain = entity.substr(0, dot);
  // screen.* are built-in cards without a Home Assistant entity behind them.
  if (domain == "screen") return entity == "screen.clock";
  for (const auto *allowed : {"light", "switch", "input_boolean", "scene", "script", "climate", "vacuum", "fan", "cover", "sensor", "binary_sensor", "input_select", "select", "number", "input_number", "weather", "media_player", "button", "input_button", "sun", "timer", "person"})
    if (domain == allowed) return true;
  return false;
}
inline std::string state_revision(const std::string &state, const std::string &attributes) {
  return state + "\n" + attributes;
}
struct Forecast { std::string day, condition; float high = NAN, low = NAN; };
struct Tile {
  std::string entity, name, state, unit, modes, hvac_modes;
  std::array<std::string, 4> fan_speeds;
  unsigned fan_speed_count = 0;
  std::string fan_speed;
  float brightness = NAN, percentage = NAN, position = NAN;
  float current = NAN, target = NAN, humidity = NAN, minimum = 7, maximum = 35, step = 0.5f;
  int hue = 0, kelvin = 3000, min_kelvin = 0, max_kelvin = 0;
  bool received = false;
  bool has_hs_color = false;
  int saturation = 0;
  std::string tap = "auto", display = "standard", inline_control = "none", media_title;
  bool wide = false;
  std::array<std::string, 8> options;
  unsigned option_count = 0, history_hours = 24;
  std::array<float,24> history{};
  bool has_history = false;
  std::array<Forecast, 5> forecast;
  unsigned forecast_count = 0;
  std::string sunrise, sunset, duration, remaining;
  uint32_t timer_end = 0;
  float battery = NAN, volume = NAN;
  uint32_t supported = 0, background = 0;
  bool transparent = false;  // "Achtergrond: geen": card fill and border hidden, contents unchanged.
  std::string icon;  // UTF-8 glyph of a chosen icon the icon fonts contain; empty keeps the domain icon.
  std::string revision, pending_revision;
  uint32_t pending_since = 0;
  bool pending = false, confirmed = false, local_feedback = false;
  bool is_switch() const { return domain()=="switch" || domain()=="input_boolean"; }
  bool loading(uint32_t now) const { return pending && (now-pending_since < (is_switch()?150u:1000u) || (!confirmed && !local_feedback && now-pending_since < 6000)); }
  bool awaiting_action(uint32_t now) const { return loading(now) && !local_feedback; }
  void begin(uint32_t now, bool local=false) { pending=true; pending_since=now; confirmed=false; local_feedback=local; pending_revision=revision; }
  void observe(const std::string &next) { revision=next; if (pending && revision!=pending_revision) confirmed=true; }
  std::string domain() const { return entity.substr(0, entity.find('.')); }
  bool builtin() const { return domain() == "screen"; }
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
struct Model {
  std::array<Tile, MAX_TILES> tiles;
  size_t count = 0;
  std::string title = "Kies tegels in HA";
  bool configured = false;
  bool set_layout(const std::vector<std::string> &entities, const std::string &name, bool &changed) {
    if (entities.size() > MAX_TILES || name.size() > 96) return false;
    for (size_t i = 0; i < entities.size(); ++i) {
      if (!valid_entity(entities[i])) return false;
      for (size_t j = 0; j < i; ++j) if (entities[i] == entities[j]) return false;
    }
    changed = !configured || count != entities.size();
    for (size_t i = 0; i < entities.size(); ++i) if (tiles[i].entity != entities[i]) changed = true;
    // A title-only update must not interrupt an open control card.
    title = name.empty() ? "Thuis" : name;
    if (changed) {
      for (auto &tile : tiles) tile = Tile{};
      count = entities.size();
      for (size_t i = 0; i < count; ++i) tiles[i].entity = entities[i];
    }
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
}  // namespace runtime_tiles
