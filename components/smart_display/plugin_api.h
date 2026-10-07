#pragma once
// The plugin API (docs/PLUGINS.md): what an ESPHome component of someone else may use to add a tile type and to hear
// about the screen's moments, without the core knowing that plugin exists.
//
// A plugin is a component of its own (`components/<name>/` in its repository) that derives from tessera::Plugin and
// registers its tile types in its constructor or setup(). The core keeps a list; nothing in the core names a plugin.
// A tile of a plugin is `plugin:<plugin>.<tile>` in a layout. The core gives it a card's drawing area while its page is
// on the glass, hands it what the add-on sent for it, ticks it once a second and lets it go when the card shows
// something else. A tile type the screen does not have (the plugin is not, or no longer, on this screen) is drawn by
// the core as a plain card with the tile's name: never an error, never a restart.
//
// Drawing follows the rules of the rest of the screen: sizes through ui::mm()/ui::px(), colours by a theme role, only
// the screen's fixed fonts, and nothing exists while the tile is not on the glass (docs/PLUGINS.md, "Drawing").
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <ArduinoJson.h>
#include "lvgl.h"
#include "theme.h"
#include "ui_scale.h"

namespace tessera {

// Major.minor (plugin_manifest.PLUGIN_API in the add-on is the same; a test keeps them equal). A plugin's manifest
// names the API it was written for (`api: "0.1"`); its component checks it when it is built. Major 0 is the time before
// the API is promised: every minor may change it, so a 0.x plugin builds on that minor only. From 1.0 a new hook raises
// the minor and only a break raises the major.
constexpr uint8_t PLUGIN_API_MAJOR = 0, PLUGIN_API_MINOR = 1;

// The screen's fixed fonts, largest first. A tile takes the largest that fits; a plugin brings no font of its own.
// VALUE is the big number of a watch card, HEADLINE a card's large words, TITLE a card's name, BODY its second line,
// BODY_LARGE the words on a key, ICON a tile's icon, ICON_SMALL the icons of the top bar. Every text font has the same
// letters (Latin with the accents of the screen's languages, digits and common signs); the icon fonts have Tessera's
// icon set only.
enum class Font : uint8_t { VALUE, HEADLINE, TITLE, BODY_LARGE, BODY, ICON, ICON_SMALL };

// What a tile gets when its card is made.
struct TileContext {
  lv_obj_t *parent;          // the card's drawing area: everything the tile makes goes in here, and dies with it
  int width, height;         // that area in pixels (the card's own padding is already off)
  uint8_t columns, rows;     // the cells of the grid the tile covers
  const char *name;          // the name given to the tile in the editor, "" for none
  const char *entity;        // the Home Assistant entity it belongs to (manifest `entity`), "" for none
  JsonObjectConst options;   // the tile's options as the editor set them (the manifest's `options`)
};

// A tile type's card. One object per card on the glass; a page switch on a board without PSRAM makes a new one.
class Tile {
 public:
  virtual ~Tile() = default;
  // Make the parts, in context.parent. Called once per card, before anything else.
  virtual void create(const TileContext &context) = 0;
  // What the add-on sent for this tile: for a tile with `data: <fetch>` the mapped answer ({"items": [...]} or the
  // fields of one object), plus "stale": true while the add-on shows its last good answer and "wait": "<why>" while it
  // has none (not_filled, asking, failed, too_large). A tile with an entity also gets "state", "name" and "attributes"
  // (the ones its manifest names; a moment named ..._at, ..._time or ...date as seconds since 1970), and is sent again
  // when that entity changes in Home Assistant. Called after create(), and again when the card is drawn after the
  // data changed (a card off the glass catches up when its page comes back).
  virtual void on_state(JsonObjectConst data) {}
  // Once a second while the card is on the glass, with the screen's clock (seconds since 1970, 0 until it is set).
  virtual void on_tick(uint32_t epoch) {}
  // The screen went light or dark: set the colours again (theme roles have another value now).
  virtual void on_theme() {}
  // A tap on the card that the touch filter let through.
  virtual void on_tap() {}
};

class Plugin;
struct TileType {
  Plugin *plugin;
  std::string id;            // the tile's id in the manifest
  std::string entity;        // plugin:<plugin>.<tile>
  std::function<Tile *()> make;
};

class Plugin {
 public:
  Plugin();
  virtual ~Plugin() = default;
  // The plugin's id and version, from its manifest (set by smart_display.register_plugin() in its __init__.py).
  const char *plugin_id() const { return id_; }
  const char *plugin_version() const { return version_; }
  // The screen's interface is up and its first page is on the glass.
  virtual void on_ready() {}
  // Every 250 ms, with millis(). Keep it short: the screen draws and takes taps in the same loop.
  virtual void on_tick(uint32_t now_ms) {}
  // The screen dimmed or went dark (true), or woke up again (false).
  virtual void on_standby(bool dark) {}
  // An update of the firmware starts: let go of large buffers.
  virtual void before_update() {}

  // A tile type of this plugin, by its id in the manifest. Call it in setup(). `make` returns a new card; the core
  // deletes it.
  void add_tile(const char *id, std::function<Tile *()> make);
  // Set by the code generation (smart_display.register_plugin() in the plugin's __init__.py), from the plugin's
  // manifest and its translations/<language>.json (part "screen", in the language the screen is built with).
  void set_identity(const char *id, const char *version) { id_ = id; version_ = version; }
  void set_text(const char *key, const char *text) { texts_.push_back({key, text}); }
  void set_memory(const char *tile, uint16_t bytes) { memory_.push_back({tile, bytes}); }
  // A text of the plugin's own, "" when it has none by that key.
  const char *text(const char *key) const;
  // What one tile of this type costs of the layout memory (the manifest's `memory`), 1024 when it was not set.
  uint16_t memory(const std::string &tile) const;

 private:
  std::vector<std::pair<const char *, const char *>> texts_;
  std::vector<std::pair<const char *, uint16_t>> memory_;
  const char *id_ = "", *version_ = "";
};

// The register. Each list is made on first use, so the order components are made in does not matter.
std::vector<Plugin *> &plugins();
std::vector<TileType> &tile_types();
const TileType *tile_type(const std::string &entity);

// ---- What the core offers a plugin ----
// The screen's clock: seconds since 1970, 0 until Home Assistant set it.
uint32_t epoch();
// A moment in the screen's own time zone (the one Home Assistant gave it).
struct LocalTime {
  int year, month, day;          // 2026, 1 to 12, 1 to 31
  int hour, minute, second;      // 0 to 23, 0 to 59, 0 to 59
  int weekday;                   // 1 Sunday to 7 Saturday
};
LocalTime local_time(uint32_t epoch);
// A time of day as the screen writes it (12 or 24 hours, as its settings say): "14:05" or "2:05 PM".
std::string clock_text(uint32_t epoch);
// "{n}" in `text` replaced by `n`; with "one | more" in `text` (Tessera's plural form), the part that fits `n`.
std::string format(const char *text, long n);
// "{name}" in `text` replaced by `value`, wherever the language put it.
std::string fill(const char *text, const char *name, const std::string &value);
// The card on the glass draws again in its next pass (after a change made outside on_state or on_tick).
void refresh();
// A Home Assistant action on an entity, as a tile's tap sends it: action("light.toggle", entity), or with one field
// (action("climate.set_temperature", entity, "temperature", "21")). The screen must be allowed to perform actions,
// and the plugin's manifest names the action under permissions.home_assistant_actions. False when it was not sent.
bool action(const char *service, const std::string &entity, const char *key = nullptr, const std::string &value = "");

namespace ui {
// Sizes: a physical size in millimetres of glass, or a size of the reference look in this board's pixels.
inline int mm(int millimetres) { return ::ui::mm(millimetres); }
inline int px(int pixels) { return ::ui::px(pixels); }
// The large look (4 inches and up) or the compact one (the CYD).
inline bool large() { return ::ui::large(); }
inline lv_color_t color(theme::Role role) { return theme::color(role); }
const lv_font_t *font(Font font);
inline int line_height(Font f) { const lv_font_t *face = font(f); return face ? lv_font_get_line_height(face) : 0; }
int text_width(const std::string &text, Font font);
// A label in `parent`, in this font and colour, one line, cut with dots when it is too long for the width it gets.
lv_obj_t *label(lv_obj_t *parent, Font font, theme::Role role = theme::INK);
// Set what a label shows, its font or its colour only when that differs (a repaint that finds nothing new is free).
void set_text(lv_obj_t *label, const std::string &text);
void set_font(lv_obj_t *label, Font font);
void set_color(lv_obj_t *label, theme::Role role);
// A rounded block (a line number's badge, a bar): no border, the radius of a key.
lv_obj_t *block(lv_obj_t *parent, theme::Role fill);
// The UTF-8 of an icon of Tessera's set by its codepoint (0xF0B5E), for a label in Font::ICON or ICON_SMALL.
std::string icon(uint32_t codepoint);
}  // namespace ui

}  // namespace tessera
