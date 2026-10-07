#pragma once
// The core's side of the plugin API (plugin_api.h, docs/PLUGINS.md): a plugin tile in a card, its moments, and what the
// screen says about its plugins. plugin_host.cpp holds it, apart from main.cpp, as energy_view.cpp does.
#include <cstdint>
#include <string>
#include <ArduinoJson.h>

namespace runtime_tiles {
struct Widgets;
struct Tile;
}

namespace plugin_host {
struct Card;  // one plugin tile in one card: the plugin's object and what it was made for
// What a tile of a plugin this screen does not have costs of the layout memory (the add-on's PLACEHOLDER_BYTES).
constexpr uint16_t PLACEHOLDER_BYTES = 64;

// Whether this screen has the tile type of `t` (plugin:<plugin>.<tile>).
bool known(const runtime_tiles::Tile &t);
// Draws the plugin tile into the slot's extra layer, `width` x `height` (the card's content).
void render(runtime_tiles::Widgets &w, const runtime_tiles::Tile &t, int width, int height);
// The card shows something else, or goes: its plugin tile goes with it.
void release(runtime_tiles::Widgets &w);
// A tap on a plugin tile's card.
void tap(runtime_tiles::Widgets &w);
// Once a second: every plugin card on the glass.
void tick_cards(uint32_t epoch);
// What one tile costs of the layout memory: the plugin's own number, or the placeholder's.
uint16_t bytes(const std::string &entity);

// The moments of every plugin.
void ready();
// Every 250 ms; `dimmed`: the screen is in standby (a change of it is on_standby).
void tick(uint32_t now_ms, bool dimmed);
void standby(bool dark);
void before_update();
// plugin_api and plugins in the screen's answer to the add-on's hello.
void hello(JsonObject root);
}  // namespace plugin_host
