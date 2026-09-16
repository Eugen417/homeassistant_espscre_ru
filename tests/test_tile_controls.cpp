// c++ -std=c++17 -Wall -Wextra -pedantic tests/test_tile_controls.cpp -o /tmp/test_tile_controls && /tmp/test_tile_controls
#include "../components/smart_display/tile_controls.h"
#include <cassert>
#include <cmath>
#include <cstring>

using namespace tile_controls;
using runtime_tiles::Tile;

static Tile make(const char *entity, const char *state, uint32_t supported = 0) {
  Tile t; t.entity = entity; t.state = state; t.supported = supported; t.received = true; return t;
}

int main() {
  // -/+ steps snap to the entity's grid and stay inside its range.
  assert(step_value(20.0f, 0.5f, 16, 32, 1) == 20.5f);
  assert(step_value(20.3f, 0.5f, 16, 32, -1) == 20.0f);
  assert(step_value(31.8f, 1.0f, 16, 32, 1) == 32.0f);
  assert(step_value(16.0f, 1.0f, 16, 32, -1) == 16.0f);
  assert(step_value(NAN, 1.0f, 16, 32, 1) == 17.0f);
  assert(step_value(5.0f, 0.0f, NAN, NAN, 1) == 6.0f);
  assert(format_value(20.0f, 1.0f, "°") == "20°");
  assert(format_value(20.5f, 0.5f, "°") == "20.5°");
  assert(format_value(NAN, 0.5f, "°") == "--");

  // Curtain: horizontal arrows, open disabled when fully open, stop only when supported.
  Tile curtain = make("cover.curtains", "open", 15); curtain.device_class = "curtain"; curtain.position = 100; curtain.controls = "buttons";
  std::array<Key, 3> keys;
  assert(keys_for(curtain, keys) == 3);
  assert(!strcmp(keys[0].icon, glyph::EXPAND) && keys[0].command == COVER_OPEN && keys[0].disabled);
  assert(keys[1].command == COVER_STOP && !keys[1].disabled);
  assert(!strcmp(keys[2].icon, glyph::COLLAPSE) && keys[2].command == COVER_CLOSE && !keys[2].disabled);
  assert(key_action(curtain, COVER_CLOSE).service == "cover.close_cover");
  Tile shutter = make("cover.shutter", "closed", 3); shutter.controls = "buttons";
  assert(keys_for(shutter, keys) == 2 && !strcmp(keys[0].icon, glyph::UP) && keys[1].disabled);
  assert(status_text(curtain) == "Open · 100%");
  assert(status_text(shutter) == "Closed");

  // Vacuum: play while docked, pause while cleaning, dock disabled in the dock.
  Tile robot = make("vacuum.s8", "docked", 30524); robot.controls = "buttons";
  assert(keys_for(robot, keys) == 3);
  assert(!strcmp(keys[0].icon, glyph::PLAY) && keys[0].command == VACUUM_START && !keys[0].disabled);
  assert(keys[1].command == VACUUM_STOP && keys[1].disabled);
  assert(keys[2].command == VACUUM_DOCK && keys[2].disabled);
  assert(key_action(robot, VACUUM_START).service == "vacuum.start");
  robot.state = "cleaning";
  assert(keys_for(robot, keys) == 3 && keys[0].command == VACUUM_PAUSE && !keys[1].disabled && !keys[2].disabled);
  Tile old_robot = make("vacuum.old", "docked", feature::VACUUM_TURN_ON | feature::VACUUM_RETURN); old_robot.controls = "buttons";
  assert(keys_for(old_robot, keys) == 2 && key_action(old_robot, VACUUM_START).service == "vacuum.turn_on");

  // Media: playback keys follow supported_features; mute flips is_volume_muted.
  Tile sonos = make("media_player.sonos", "playing", 8321599); sonos.volume = 0.17f; sonos.edit_extra().media_title = "TV"; sonos.controls = "playback";
  assert(keys_for(sonos, keys) == 3 && !strcmp(keys[1].icon, glyph::PAUSE) && keys[1].command == MEDIA_PLAY_PAUSE);
  assert(status_text(sonos) == "TV · 17%");
  Action mute = key_action(sonos, MEDIA_MUTE);
  assert(mute.service == "media_player.volume_mute" && mute.key == "is_volume_muted" && mute.value == "true");
  sonos.muted = true; assert(key_action(sonos, MEDIA_MUTE).value == "false");
  Tile radio = make("media_player.radio", "idle", feature::MEDIA_PLAY | feature::MEDIA_PAUSE); radio.controls = "playback";
  assert(keys_for(radio, keys) == 1 && !strcmp(keys[0].icon, glyph::PLAY));
  assert(status_text(radio) == "Idle");

  // Climate: mode keys in priority order, at most three, the active one checked.
  Tile ac = make("climate.ac", "cool"); ac.edit_extra().hvac_modes = "[\"off\",\"heat_cool\",\"cool\",\"heat\",\"fan_only\",\"dry\"]"; ac.controls = "mode"; ac.current = 21.5f; ac.target = 20; ac.step = 1;
  assert(keys_for(ac, keys) == 3);
  assert(keys[0].arg == "off" && keys[1].arg == "heat" && keys[2].arg == "cool" && keys[2].checked && !keys[1].checked);
  Action mode = key_action(ac, HVAC_MODE, keys[1].arg);
  assert(mode.service == "climate.set_hvac_mode" && mode.key == "hvac_mode" && mode.value == "heat");
  assert(status_text(ac) == "Cool · 21.5°");
  ac.edit_extra().hvac_action = "cooling"; assert(status_text(ac) == "Cooling · 21.5°");
  assert(edit_target(ac) == 20 && edit_step(ac) == 1);
  Action set = edit_action(ac, step_value(edit_target(ac), edit_step(ac), ac.minimum, ac.maximum, 1));
  assert(set.service == "climate.set_temperature" && set.key == "temperature" && set.value == "21");
  assert(edit_action(ac, 20.5f).value == "20.5");

  // Numbers edit their own state; selects step through their options with wrap-around.
  Tile number = make("number.target", "55"); number.minimum = 0; number.maximum = 100; number.step = 5;
  assert(edit_target(number) == 55 && edit_action(number, step_value(55, 5, 0, 100, -1)).value == "50");
  assert(edit_action(number, 50).service == "number.set_value");
  Tile select = make("select.stand", "Comfort"); select.edit_extra().options = {"Eco", "Comfort", "Boost"}; select.controls = "stepper";
  assert(panel_kind(select) == "chevrons");
  assert(keys_for(select, keys) == 2 && !keys[0].disabled);
  assert(key_action(select, SELECT_NEXT).value == "Boost" && key_action(select, SELECT_PREVIOUS).value == "Eco");
  select.state = "Boost"; assert(key_action(select, SELECT_NEXT).value == "Eco");
  select.edit_extra().options.resize(1); assert(keys_for(select, keys) == 2 && keys[0].disabled);

  // Timer, run buttons and toggles.
  Tile timer = make("timer.eggs", "active"); timer.controls = "buttons";
  assert(keys_for(timer, keys) == 2 && keys[0].command == TIMER_PAUSE && !keys[1].disabled);
  timer.state = "idle"; assert(keys_for(timer, keys) == 2 && keys[0].command == TIMER_START && keys[1].disabled);
  assert(key_action(make("scene.evening", "unknown"), RUN).service == "scene.turn_on");
  assert(key_action(make("script.all", "off"), RUN).service == "script.turn_on");
  assert(key_action(make("input_button.bell", "unknown"), RUN).service == "input_button.press");
  assert(!strcmp(run_label("scene"), "Activate") && !strcmp(run_label("button"), "Press"));
  assert(key_action(make("switch.desk", "on"), TOGGLE).service == "switch.turn_off");
  assert(key_action(make("light.lamp", "off"), TOGGLE).service == "light.turn_on");
  assert(!key_action(make("sensor.x", "1"), TOGGLE).valid());
  assert(!key_action(make("sensor.x", "1"), RUN).valid());

  // Panel kinds.
  Tile lamp = make("light.lamp", "on"); lamp.controls = "brightness";
  assert(is_slider(panel_kind(lamp)) && !is_key_row(panel_kind(lamp)));
  assert(is_key_row("playback") && is_key_row("mode") && !is_key_row("setpoint"));

  // Climate mode colours follow Home Assistant; anything else is grey.
  assert(mode_color("heat") == 0xFF6F22 && mode_color("cool") == 0x2196F3 && mode_color("off") == 0x9E9E9E);

  // Vacuum rows: the cleaning mode decides which of suction and water the card shows.
  Tile pippa = make("vacuum.s8", "docked", 30524);
  auto row = [](char kind, const char *entity, const char *current, std::vector<std::string> values, const char *roles = "") {
    runtime_tiles::Choice c; c.kind = kind; c.entity = entity; c.current = current; c.values = values; c.labels = values; c.roles = roles; return c;
  };
  pippa.edit_extra().choices = {row('m', "select.woonkamer_s8_schoonmaakmodus", "vac_and_mop", {"vacuum", "vac_and_mop", "mop", "custom"}, "vbma"),
                   row('w', "select.s8_intensiteit_van_dweilen", "intense", {"mild", "standard", "intense"}),
                   row('s', "", "", {"quiet", "balanced", "turbo", "max", "max_plus"})};
  pippa.edit_extra().fan_speed = "max";
  settle_suction(pippa.edit_extra());
  assert(pippa.choice('s')->current == "max" && pippa.extra().choices.size() == 3);
  auto rows = vacuum_rows(pippa, 0);
  assert(rows.suction && rows.water && vacuum_role(pippa, 0) == 'b');
  pippa.choice('m')->current = "mop"; rows = vacuum_rows(pippa, 0);
  assert(!rows.suction && rows.water);
  pippa.choice('m')->current = "vacuum"; rows = vacuum_rows(pippa, 0);
  assert(rows.suction && !rows.water);
  pippa.choice('m')->current = "custom"; rows = vacuum_rows(pippa, 0);
  assert(!rows.suction && !rows.water && vacuum_role(pippa, 0) == 'a');
  pippa.choice('m')->current = "something_new"; assert(vacuum_role(pippa, 0) == 'b');
  // A tapped chip shows while Home Assistant is busy, and the old value once it gave up.
  pippa.choice('m')->current = "vac_and_mop"; pippa.choice('m')->sent = "mop";
  pippa.begin(1000);
  assert(shown_value(pippa, *pippa.choice('m'), 1500) == "mop" && vacuum_rows(pippa, 1500).water && !vacuum_rows(pippa, 1500).suction);
  assert(shown_value(pippa, *pippa.choice('m'), 9000) == "vac_and_mop");
  assert(choice_action(pippa, 'm', "mop").service == "select.select_option" && choice_action(pippa, 'm', "mop").key == "option");
  assert(choice_action(pippa, 's', "turbo").service == "vacuum.set_fan_speed" && choice_action(pippa, 's', "turbo").value == "turbo");
  // An older manager sends no suction row: the vacuum's own four speeds, with the card's names.
  Tile old_robot2 = make("vacuum.old", "docked", 30524);
  old_robot2.edit_extra().fan_speeds = {"quiet", "balanced"}; old_robot2.edit_extra().fan_speed = "balanced";
  settle_suction(old_robot2.edit_extra());
  assert(old_robot2.choice('s') && old_robot2.choice('s')->labels[1] == "Normal" && old_robot2.choice('s')->current == "balanced");
  rows = vacuum_rows(old_robot2, 0);
  assert(rows.suction && !rows.water && !choice_action(old_robot2, 'm', "mop").valid());
  return 0;
}
