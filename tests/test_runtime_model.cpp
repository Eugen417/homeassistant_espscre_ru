#include "screen_text_en.h"
#include "../components/smart_display/runtime_model.h"
#include <cassert>
int main() {
  using namespace runtime_tiles;
  Model m; bool changed = false;
  assert(!m.ready());
  assert(!m.set_layout({"light.a", "light.a"}, "Home", changed));
  assert(!m.set_layout({"light.a;script.bad"}, "Home", changed));
  assert(!m.set_layout({"light."}, "Home", changed));
  assert(!m.set_layout({"lock.frontdoor"}, "Home", changed));
  assert(m.set_layout({"light.a", "climate.office", "vacuum.robot"}, "Home", changed));
  assert(changed && !m.ready());
  for (size_t i = 0; i < m.count; ++i) { m.tiles[i].received = true; m.tiles[i].state = "off"; }
  assert(m.ready());
  assert(m.set_layout({"light.a", "climate.office", "vacuum.robot"}, "Kitchen", changed));
  assert(!changed && m.ready()); // periodic sync never interrupts an open card
  assert(!m.accepts(10, "light.a"));
  assert(!m.accepts(0, "light.removed"));
  assert(m.accepts(0, "light.a"));
  assert(m.set_layout({"vacuum.robot", "light.new"}, "Kitchen", changed));
  assert(changed && !m.ready());
  assert(m.tiles[0].state.empty()); // never reuse the old slot's state/actions
  assert(!m.accepts(0, "light.a"));
  assert(m.set_layout({}, "Empty", changed)); assert(changed && m.ready() && m.count == 0);
  std::vector<std::string> many;
  // Eight pages of six slots (firmware 0.2.62+): 48 tiles fit, a 49th is refused.
  for (int i = 0; i < 49; ++i) many.push_back("light.a" + std::to_string(i));
  assert(!m.set_layout(many, "Too many", changed)); assert(m.count == 0);
  many.pop_back(); assert(m.set_layout(many,"Forty-eight",changed));
  assert(m.count==48 && m.tiles.size()==48 && m.accepts(47,"light.a47"));
  // A screen pays only for the tiles it has: twelve tiles, twelve on the heap.
  assert(m.set_layout({"light.a0","light.a1","light.a2","light.a3","light.a4","light.a5","light.a6","light.a7","light.a8","light.a9","light.a10","light.a11"},"Twelve",changed));
  assert(changed && m.count==12 && m.tiles.size()==12);
  for(size_t i=0;i<m.count;++i){m.tiles[i].received=true;m.tiles[i].state="off";}
  assert(m.ready());
}
// Built-in and new Home Assistant domains, plus wide-tile packing.
static void test_domains_and_packing() {
  using namespace runtime_tiles;
  Model m; bool changed = false;
  assert(valid_entity("screen.clock") && !valid_entity("screen.other"));
  assert(valid_entity("sun.sun") && valid_entity("timer.kitchen") && valid_entity("person.max"));
  assert(m.set_layout({"screen.clock", "weather.home", "light.a", "sensor.b", "person.max", "timer.egg", "sun.sun"}, "Home", changed));
  assert(m.tiles[0].available() && !m.tiles[1].available());  // built-in cards need no HA state
  // A scene or button that never ran is unknown in Home Assistant and can still be pressed; other unknown states can't act.
  for (const char *entity : {"scene.evening", "button.restart", "input_button.doorbell"}) {
    runtime_tiles::Tile never; never.entity = entity; never.received = true; never.state = "unknown";
    assert(never.available());
    never.state = "unavailable"; assert(!never.available());
  }
  { runtime_tiles::Tile lamp; lamp.entity = "light.lamp"; lamp.received = true; lamp.state = "unknown"; assert(!lamp.available()); }
  { runtime_tiles::Tile waiting; waiting.entity = "scene.evening"; waiting.state = "unknown"; assert(!waiting.available()); }
  m.tiles[4].received = m.tiles[5].received = true;
  m.tiles[4].state = "home"; assert(m.tiles[4].active());
  m.tiles[5].state = "active"; assert(m.tiles[5].active());
  m.tiles[5].state = "idle"; assert(!m.tiles[5].active());
  std::array<Placement, MAX_TILES> p;
  assert(pack(m.tiles, 0, p) == 1);
  assert(pack(m.tiles, m.count, p) == 2 && p[6].page == 1 && p[6].slot == 0);
  // A wide tile after a left-column tile skips the right column.
  m.tiles[1].wide = true;
  assert(pack(m.tiles, m.count, p) == 2);
  assert(p[0].slot == 0 && p[1].slot == 2 && p[2].slot == 4 && p[3].slot == 5);
  assert(p[4].page == 1 && p[4].slot == 0 && p[6].page == 1 && p[6].slot == 2);
  // Wide tiles ending exactly on a page boundary do not open an empty page.
  for (auto &t : m.tiles) t.wide = false;
  m.tiles[0].wide = m.tiles[2].wide = true;
  assert(pack(m.tiles, 3, p) == 1 && p[0].slot == 0 && p[1].slot == 2 && p[2].slot == 4);
  m.tiles[3].wide = true;
  assert(pack(m.tiles, 4, p) == 2 && p[3].page == 1 && p[3].slot == 0);
  // A full tile (firmware 0.2.62+) takes a page of its own: it starts one when its page is in use, and the
  // tiles after it start the next page. Tiles at page starts keep their page.
  for (auto &t : m.tiles) { t.wide = false; t.full = false; }
  m.tiles[1].full = true; m.tiles[1].wide = true;
  assert(m.tiles[1].cells() == 6 && m.tiles[0].cells() == 1);
  assert(pack(m.tiles, 3, p) == 3 && p[0].page == 0 && p[1].page == 1 && p[1].slot == 0 && p[2].page == 2 && p[2].slot == 0);
  m.tiles[0].full = true; m.tiles[0].wide = true;
  assert(pack(m.tiles, 3, p) == 3 && p[0].page == 0 && p[0].slot == 0 && p[1].page == 1 && p[2].page == 2);
  m.tiles[1].full = m.tiles[1].wide = false;
  assert(pack(m.tiles, 3, p) == 2 && p[1].page == 1 && p[1].slot == 0 && p[2].page == 1 && p[2].slot == 1);
  for (auto &t : m.tiles) { t.wide = false; t.full = false; }
}
struct RunExtra { RunExtra() { test_domains_and_packing(); } } run_extra;
// Explicit grid positions (0.2.26+): gaps stay empty, a wide card starts in the left column.
static void test_explicit_slots() {
  using namespace runtime_tiles;
  Model m; bool changed = false, moved = false;
  std::array<Placement, MAX_TILES> p;
  // Validation: one slot per entity, inside eight pages, no duplicates.
  assert(!m.set_layout({"light.a", "light.b"}, "Home", changed, {0}, moved));
  assert(!m.set_layout({"light.a", "light.b"}, "Home", changed, {0, 48}, moved));
  assert(!m.set_layout({"light.a", "light.b"}, "Home", changed, {3, 3}, moved));
  assert(!m.configured);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 5, 7}, moved));
  assert(changed && !moved && m.explicit_slots);
  assert(place(m, p) == 2 && p[0].page == 0 && p[0].slot == 0 && p[1].slot == 5 && p[2].page == 1 && p[2].slot == 1);
  // Same tiles elsewhere: states survive, only the pages re-place.
  for (size_t i = 0; i < m.count; ++i) { m.tiles[i].received = true; m.tiles[i].state = "on"; }
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 7}, moved));
  assert(!changed && moved && m.ready() && m.tiles[0].state == "on");
  assert(place(m, p) == 2 && p[0].slot == 2);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 7}, moved));
  assert(!changed && !moved);
  // A wide card on an odd slot snaps to its row start; the page count follows its footprint.
  m.tiles[2].wide = true;
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 11}, moved));
  assert(!changed && moved && place(m, p) == 2 && p[2].page == 1 && p[2].slot == 4);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {2, 5, 12}, moved));
  assert(place(m, p) == 3 && p[2].page == 2 && p[2].slot == 0);
  // An empty page between two used pages stays a page; the last slot of page eight is allowed.
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 1, 46}, moved));
  assert(place(m, p) == 8 && p[2].page == 7 && p[2].slot == 4);
  // Pages kept on purpose extend the count, never shrink it.
  m.pages = 3; assert(place(m, p) == 8);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 1, 2}, moved));
  assert(place(m, p) == 3); m.pages = 1; assert(place(m, p) == 1); m.pages = 200; assert(place(m, p) == MAX_PAGES); m.pages = 1;
  // Without slots (older manager) the in-order packing returns.
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {}, moved));
  assert(!changed && moved && !m.explicit_slots);
  assert(place(m, p) == 1 && p[0].slot == 0 && p[1].slot == 1 && p[2].slot == 2);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {}, moved));
  assert(!changed && !moved);
  // A full tile (firmware 0.2.62+) on any slot of a page snaps to that page's first slot and fills the page.
  assert(valid_entity("screen.page_1") && valid_entity("screen.page_8") && !valid_entity("screen.page_9") && !valid_entity("screen.page") && !valid_entity("screen.page_"));
  { Tile nav; nav.entity = "screen.page_3"; assert(nav.is_page() && nav.page_target() == 3 && nav.builtin() && nav.available()); }
  m.tiles[1].full = true; m.tiles[1].wide = true;
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 9, 12}, moved));
  assert(!changed && moved && place(m, p) == 3 && p[1].page == 1 && p[1].slot == 0 && p[2].page == 2 && p[2].slot == 0);
  assert(m.set_layout({"light.a", "light.b", "light.c"}, "Home", changed, {0, 47, 1}, moved));
  assert(place(m, p) == 8 && p[1].page == 7 && p[1].slot == 0);
  m.tiles[1].full = m.tiles[1].wide = false;
  // Changing the tiles resets everything, including the positions.
  assert(m.set_layout({"light.a", "light.z"}, "Home", changed, {1, 6}, moved));
  assert(changed && !moved && !m.ready() && place(m, p) == 2 && p[0].slot == 1 && p[1].page == 1);
}
struct RunSlots { RunSlots() { test_explicit_slots(); } } run_slots;
// What only some tiles carry lives in an Extra that exists only while a state needs it.
static void test_extra() {
  using namespace runtime_tiles;
  Tile lamp;
  assert(!lamp.extra_ptr() && lamp.extra().options.empty() && !lamp.choice('m'));
  lamp.set_extra(Extra{});
  assert(!lamp.extra_ptr());  // an empty block is never allocated
  Extra climate; climate.hvac_modes = "[\"off\",\"heat\"]"; climate.hvac_action = "heating";
  lamp.set_extra(std::move(climate));
  Extra *kept = lamp.extra_ptr();
  assert(kept && lamp.extra().hvac_action == "heating");
  Extra again; again.hvac_action = "idle";
  lamp.set_extra(std::move(again));
  assert(lamp.extra_ptr() == kept && lamp.extra().hvac_action == "idle" && lamp.extra().hvac_modes.empty());
  Tile copy = lamp;  // a copy owns its own block
  copy.edit_extra().hvac_action = "cooling";
  assert(lamp.extra().hvac_action == "idle" && copy.extra().hvac_action == "cooling");
  lamp.set_extra(Extra{});
  assert(!lamp.extra_ptr() && lamp.extra().hvac_action.empty());
  Extra robot; Choice mode; mode.kind = 'm'; mode.values = {"vacuum", "mop"}; robot.choices.push_back(mode); robot.room = "Kitchen";
  Tile vacuum; vacuum.set_extra(std::move(robot));
  assert(vacuum.choice('m') && vacuum.choice('m')->values.size() == 2 && !vacuum.choice('w') && vacuum.extra().room == "Kitchen");
  vacuum.choice('m')->sent = "mop";
  assert(static_cast<const Tile &>(vacuum).choice('m')->sent == "mop");
  // A new layout resets the slots, blocks included.
  Model m; bool changed = false;
  assert(m.set_layout({"vacuum.robot"}, "Home", changed));
  m.tiles[0] = vacuum;
  assert(m.set_layout({"light.a"}, "Home", changed) && changed && !m.tiles[0].extra_ptr());
}
struct RunTileExtra { RunTileExtra() { test_extra(); } } run_tile_extra;
// Sliders keep their colour the way Home Assistant's tile sliders do: grey for an off light or fan and a media
// player that is off or in standby, coloured for a player that plays, a cover open or closed and a number.
static void test_slider_colours() {
  using namespace runtime_tiles;
  Model m; bool changed = false;
  assert(m.set_layout({"light.a", "fan.b", "media_player.c", "cover.d", "number.e", "input_number.f"}, "Home", changed));
  auto &light = m.tiles[0], &fan = m.tiles[1], &player = m.tiles[2], &cover = m.tiles[3], &number = m.tiles[4], &setpoint = m.tiles[5];
  player.state = "playing"; assert(!player.slider_active());  // nothing received yet
  for (size_t i = 0; i < m.count; ++i) m.tiles[i].received = true;
  light.state = "on"; assert(light.slider_active());
  light.state = "off"; assert(!light.slider_active());
  fan.state = "on"; assert(fan.slider_active());
  fan.state = "off"; assert(!fan.slider_active());
  for (const char *state : {"playing", "paused", "idle", "buffering", "on"}) { player.state = state; assert(player.slider_active()); }
  for (const char *state : {"off", "standby", "unavailable", "unknown"}) { player.state = state; assert(!player.slider_active()); }
  for (const char *state : {"open", "opening", "closing", "closed"}) { cover.state = state; assert(cover.slider_active()); }
  cover.state = "unavailable"; assert(!cover.slider_active());
  number.state = "21.5"; assert(number.slider_active());
  number.state = "unknown"; assert(!number.slider_active());
  setpoint.state = "55.0"; assert(setpoint.slider_active());
}
struct RunSliderColours { RunSliderColours() { test_slider_colours(); } } run_slider_colours;
// Home Assistant's stateActive() (firmware 0.2.71+): what it calls inactive is grey on the screen. (`lights` was the
// full-page tint of firmware 0.2.62 to 0.2.76, gone since 0.2.77: the cases stay as a record of the rule.)
static void test_state_active() {
  using namespace runtime_tiles;
  struct Case { const char *entity, *state; bool active, lights; };
  for (const Case &c : std::initializer_list<Case>{
           {"climate.airco", "off", false, false}, {"climate.airco", "cool", true, true}, {"climate.airco", "heat", true, true},
           {"climate.airco", "heat_cool", true, true}, {"climate.airco", "fan_only", true, true}, {"climate.airco", "dry", true, true},
           {"light.a", "on", true, true}, {"light.a", "off", false, false}, {"switch.a", "on", true, true},
           {"input_boolean.a", "off", false, false}, {"fan.a", "on", true, true}, {"fan.a", "off", false, false},
           {"cover.a", "open", true, true}, {"cover.a", "opening", true, true}, {"cover.a", "closing", true, true},
           {"cover.a", "closed", false, false}, {"media_player.a", "playing", true, true}, {"media_player.a", "paused", true, true},
           {"media_player.a", "idle", true, true}, {"media_player.a", "on", true, true}, {"media_player.a", "standby", false, false},
           {"media_player.a", "off", false, false}, {"vacuum.a", "cleaning", true, true}, {"vacuum.a", "returning", true, true},
           {"vacuum.a", "error", true, true}, {"vacuum.a", "docked", false, false}, {"vacuum.a", "idle", false, false},
           {"vacuum.a", "paused", false, false}, {"script.a", "on", true, true}, {"script.a", "off", false, false},
           {"timer.a", "active", true, true}, {"timer.a", "paused", false, false}, {"timer.a", "idle", false, false},
           {"camera.a", "streaming", true, true}, {"camera.a", "recording", true, true}, {"camera.a", "idle", false, false},
           {"person.a", "home", true, true}, {"person.a", "Work", true, true}, {"person.a", "not_home", false, false},
           {"binary_sensor.a", "on", true, true}, {"binary_sensor.a", "off", false, false},
           // Always active in Home Assistant: coloured, but never lit.
           {"sensor.a", "21.5", true, false}, {"number.a", "3", true, false}, {"input_number.a", "3", true, false},
           {"select.a", "eco", true, false}, {"input_select.a", "eco", true, false}, {"weather.a", "rainy", true, false},
           {"sun.sun", "above_horizon", true, false}, {"sun.sun", "below_horizon", true, false},
           {"scene.a", "2026-09-18T20:00:00+00:00", true, false}, {"scene.a", "unknown", true, false},
           {"button.a", "unknown", true, false}, {"input_button.a", "unknown", true, false}, {"image.a", "unknown", true, false},
           // "off" is inactive in every domain, a select's option too; unknown too, except where the state is a moment.
           {"select.a", "off", false, false}, {"sensor.a", "unknown", false, false}, {"light.a", "unavailable", false, false},
           {"scene.a", "unavailable", false, false}}) {
    Tile t; t.entity = c.entity; t.state = c.state; t.received = true;
    assert(t.active() == c.active);
    (void) c.lights;
  }
  // Nothing received yet is inactive; a built-in card has no state and is always active.
  { Tile t; t.entity = "light.a"; t.state = "on"; assert(!t.active()); }
  for (const char *entity : {"screen.clock", "screen.settings", "screen.page_2"}) {
    Tile t; t.entity = entity; assert(t.active());
  }
  // A closed blind: grey card, coloured slider.
  { Tile t; t.entity = "cover.a"; t.state = "closed"; t.received = true; assert(!t.active() && t.slider_active()); }
}
struct RunStateActive { RunStateActive() { test_state_active(); } } run_state_active;
// A navigation tile may sit on several pages of one screen (firmware 0.2.65+); a Home Assistant entity still appears once.
static void test_repeated_page_tiles() {
  using namespace runtime_tiles;
  Model m; bool changed = false, moved = false;
  std::array<Placement, MAX_TILES> p;
  assert(m.set_layout({"light.x", "screen.page_1", "screen.page_1", "sensor.y"}, "Home", changed, {0, 6, 12, 13}, moved));
  assert(changed && m.count == 4 && place(m, p) == 3 && p[1].page == 1 && p[2].page == 2);
  assert(m.tiles[1].is_page() && m.tiles[2].is_page() && m.tiles[1].page_target() == 1 && m.tiles[2].page_target() == 1);
  // Each copy is its own tile: a state message reaches it by its index, so both get their options.
  assert(m.accepts(1, "screen.page_1") && m.accepts(2, "screen.page_1") && !m.accepts(3, "screen.page_1"));
  m.tiles[1].icon = "a"; m.tiles[2].icon = "b";
  assert(m.tiles[1].icon != m.tiles[2].icon);
  for (size_t i = 0; i < m.count; ++i) m.tiles[i].received = true;
  assert(m.ready());
  assert(m.set_layout({"light.x", "screen.page_1", "screen.page_1", "sensor.y"}, "Home", changed, {0, 6, 12, 13}, moved));
  assert(!changed && !moved && m.ready());  // the repeat of the same layout keeps everything
  assert(m.set_layout({"screen.page_2", "screen.page_2", "screen.page_2"}, "Menu", changed));
  // Every Home Assistant entity and the other built-in cards stay unique.
  assert(!m.set_layout({"light.x", "screen.page_1", "light.x"}, "Home", changed, {0, 6, 12}, moved));
  assert(!m.set_layout({"screen.clock", "screen.clock"}, "Home", changed));
  assert(!m.set_layout({"screen.settings", "screen.page_1", "screen.settings"}, "Home", changed));
  assert(!m.set_layout({"screen.page_9", "screen.page_9"}, "Home", changed));
  assert(m.count == 3 && m.title == "Menu");  // a refused layout leaves the one on screen alone
}
struct RunRepeatedPages { RunRepeatedPages() { test_repeated_page_tiles(); } } run_repeated_pages;
// A title of its own for a page (firmware 0.2.90+): the screen's title stands on every page unless that page says
// something else, and a page that says nothing hands the bar back to the screen's title.
static void test_page_titles() {
  using namespace runtime_tiles;
  Model m;
  bool changed = false;
  assert(m.set_layout({"light.a"}, "Home", changed));
  assert(m.title_of(0) == "Home" && m.title_of(3) == "Home" && m.title_of(-1) == "Home");
  m.page_titles = {"", "Kitchen", "Bedroom"};
  assert(m.title_of(0) == "Home");        // empty: the screen's own
  assert(m.title_of(1) == "Kitchen");
  assert(m.title_of(2) == "Bedroom");
  assert(m.title_of(7) == "Home");        // past the list: the screen's own
  assert(m.title_of(-1) == "Home");
  // The screen's title changing carries every page that inherits it.
  assert(m.set_layout({"light.a"}, "Upstairs", changed));
  assert(m.title_of(0) == "Upstairs" && m.title_of(1) == "Kitchen");
}
struct RunPageTitles { RunPageTitles() { test_page_titles(); } } run_page_titles;
// A longer list needs one block for every tile (firmware 0.2.65+): without it the layout is refused before anything
// changes, with a reason for the manager, instead of std::vector writing tiles through a null pointer on the ESP32.
static size_t test_room = 0;
static void test_tile_room() {
  using namespace runtime_tiles;
  assert(!tile_room);  // the host has room and asks nothing
  Model m; bool changed = false;
  std::vector<std::string> twelve, forty_eight;
  for (int i = 0; i < 48; ++i) (i < 12 ? twelve : forty_eight).push_back("light.r" + std::to_string(i));
  for (int i = 0; i < 12; ++i) forty_eight.push_back("light.r" + std::to_string(i));
  assert(m.set_layout(twelve, "Twelve", changed) && m.count == 12 && m.refusal.empty());
  for (size_t i = 0; i < m.count; ++i) { m.tiles[i].received = true; m.tiles[i].state = "on"; }
  tile_room = [] { return test_room; };
  test_room = 48 * sizeof(Tile) - 1;
  assert(!m.set_layout(forty_eight, "Forty-eight", changed));
  assert(m.refusal == "Error: no memory for 48 tiles");
  assert(m.configured && m.count == 12 && m.title == "Twelve" && m.ready() && m.tiles[0].state == "on");
  // Fewer tiles than the list already holds need no new block, and a malformed layout is no memory question.
  assert(m.set_layout({"light.r0", "light.r1"}, "Two", changed) && changed && m.count == 2 && m.refusal.empty());
  assert(!m.set_layout({"light.r0", "light.r0"}, "Twice", changed) && m.refusal.empty());
  test_room = 48 * sizeof(Tile);
  assert(m.set_layout(forty_eight, "Forty-eight", changed) && m.count == 48 && m.refusal.empty());
  // An empty screen asks for its first tiles the same way.
  Model fresh;
  test_room = 0;
  assert(!fresh.set_layout({"light.a"}, "One", changed) && !fresh.configured && fresh.refusal == "Error: no memory for 1 tiles");
  assert(fresh.set_layout({}, "None", changed) && fresh.configured);
  tile_room = nullptr;
}
struct RunTileRoom { RunTileRoom() { test_tile_room(); } } run_tile_room;
// One redraw bit for each of the 48 tiles (firmware 0.2.65+); 32 bits sent tiles 33-48 through a full redraw.
static void test_tile_bits() {
  using namespace runtime_tiles;
  assert(tile_bit(0) == 1 && tile_bit(31) == (uint64_t{1} << 31) && tile_bit(47) == (uint64_t{1} << 47));
  assert(tile_bit(MAX_TILES - 1) && !tile_bit(64) && !tile_bit(MAX_TILES + 100));
  uint64_t dirty = tile_bit(33) | tile_bit(47);
  assert((dirty & tile_bit(33)) && (dirty & tile_bit(47)) && !(dirty & tile_bit(1)) && !(dirty & tile_bit(32)));
  for (size_t a = 0; a < MAX_TILES; ++a) for (size_t b = a + 1; b < MAX_TILES; ++b) assert(!(tile_bit(a) & tile_bit(b)));
}
struct RunTileBits { RunTileBits() { test_tile_bits(); } } run_tile_bits;
// Clock texts without sscanf (firmware 0.2.75+): the same answers the sscanf versions gave.
static void test_clock_texts() {
  using namespace runtime_tiles;
  assert(duration_seconds("0:05:00") == 300 && duration_seconds("1:02:03") == 3723 && duration_seconds("12:00:00") == 43200);
  assert(duration_seconds("5:30") == 330 && duration_seconds("0:00:05.5") == 5 && duration_seconds("5:00:") == 300);
  assert(duration_seconds("1:2:3:4") == 3723);
  assert(duration_seconds("") == 0 && duration_seconds("12") == 0 && duration_seconds("1 day, 2:03:04") == 0);
  assert(duration_seconds("idle") == 0 && duration_seconds(":05") == 0);
  assert(minutes_of("06:45") == 405 && minutes_of("0:00") == 0 && minutes_of("23:59") == 1439 && minutes_of("07:30:00") == 450);
  assert(minutes_of("24:00") == -1 && minutes_of("12:60") == -1 && minutes_of("12") == -1 && minutes_of("") == -1);
  assert(minutes_of("x6:45") == -1 && minutes_of(" 6: 45") == 405 && duration_seconds("1: 59:") == 119);
  // A running timer never shows more than its duration, whatever the clocks' whole seconds make of it.
  assert(timer_left(1003, 999, "0:00:03") == 3 && timer_left(1003, 1000, "0:00:03") == 3 && timer_left(1003, 1001, "0:00:03") == 2);
  assert(timer_left(1003, 1003, "0:00:03") == 0 && timer_left(1003, 1010, "0:00:03") == 0 && timer_left(1003, 0, "0:00:03") == 0);
  assert(timer_left(1300, 1000, "0:05:00") == 300 && timer_left(1300, 1000, "") == 300 && timer_left(1300, 1000, "idle") == 300);
  unsigned v[3] = {9, 9, 9};
  assert(clock_parts("7:8", v, 3) == 2 && v[0] == 7 && v[1] == 8 && v[2] == 9);
}
struct RunClockTexts { RunClockTexts() { test_clock_texts(); } } run_clock_texts;
