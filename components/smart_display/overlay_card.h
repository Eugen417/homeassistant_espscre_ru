#pragma once
// The frame every overlay gets: the card a tap opens (a light, a thermostat, a blind, a vacuum, a graph).
//
// Two rules, and they hold on every board:
//
//  * **The content is never wider than a hand spans.** A thermostat whose − and + sit at the far edges of a
//    ten-inch panel takes two hands; ui::control_max_width is the width one hand does span, and a card wider
//    than that keeps that width. On a CYD and a Guition nothing is capped: their glass is narrower already.
//  * **What is capped, is centred.** A card that does not fill the glass sits in the middle of it, left to
//    right and top to bottom, like a dialog. Not centring it would leave it stuck against a corner.
//
// A card that is a *picture* says so and is not capped: the media card's cover art and a camera's image are
// nicer the bigger they are (`overlay_card::frame(root, overlay_card::picture)`).
//
// Everything a finger must hit keeps at least `ui::touch_min()` of glass, whatever the card drew: a thin
// track may look thin, but its touch area is grown to the finger's size (`overlay_card::touchable`).
#include <algorithm>
#include <cstdint>
#include "lvgl.h"
#include "ui_scale.h"

namespace overlay_card {

enum Kind : bool { controls = false, picture = true };

inline int screen_width() { return lv_display_get_horizontal_resolution(lv_display_get_default()); }
inline int screen_height() { return lv_display_get_vertical_resolution(lv_display_get_default()); }

// The width a card's content may take: capped for controls, the whole glass for a picture. A card in two
// columns may take twice the cap, so each column keeps its own reach.
inline int content_width(Kind kind = controls, int columns = 1) {
  const int room = screen_width();
  if (kind == picture) return room;
  return std::min(room, columns * ui::control_max_width() + (columns - 1) * ui::column_gap());
}

// The room every card keeps between its content and the edge of its area, whatever the board and whatever the
// card: the vacuum's hero, the blind's keys, the thermostat's setpoint and the graph all start here.
inline int pad() { return ui::px(ui::large() ? 20 : 10); }

// Whether a card stands in one column or two. Glass that is short for its width - a 480 x 272 panel, a wide
// seven-inch - cannot give a tall control the millimetres it needs while half its width goes unused; such a
// card lays its controls beside the rest instead of under it, the way a web page turns a stack into two
// columns when the viewport allows. `need_height` is what the one-column form asks for, `min_column` the
// narrowest a column may be. Tall glass keeps one column, and so does glass too narrow to split.
inline int columns(int need_height, int min_column) {
  if (need_height <= screen_height()) return 1;
  return screen_width() >= 2 * min_column + ui::column_gap() ? 2 : 1;
}

// Give a card's root its room and put it in the middle of the glass, left to right.
inline void frame(lv_obj_t *root, Kind kind = controls, int columns = 1) {
  if (!root) return;
  const int width = content_width(kind, columns);
  lv_obj_set_width(root, width);
  lv_obj_set_x(root, (screen_width() - width) / 2);
  lv_obj_set_y(root, 0);
}

// Once a card is drawn, put its content in the middle of the glass from top to bottom as well. The first
// `pinned` children are the card's own top bar (the back key and the name): those stay where they are, at the
// top, whatever the content below them does. Only the block below the bar moves, and only when it leaves more
// than a finger's worth of room.
inline void centre(lv_obj_t *root, uint32_t pinned = 0) {
  if (!root) return;
  lv_obj_update_layout(root);
  int bar = 0, top = INT32_MAX, bottom = 0;
  for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
    lv_obj_t *child = lv_obj_get_child(root, i);
    if (!child || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
    const int y = lv_obj_get_y(child), end = y + lv_obj_get_height(child);
    if (i < pinned) { bar = std::max(bar, end); continue; }
    top = std::min(top, y);
    bottom = std::max(bottom, end);
  }
  if (bottom <= 0 || top == INT32_MAX) return;
  const int room = screen_height() - bar - (bottom - top);
  if (room <= ui::touch_min()) return;
  const int shift = bar + room / 2 - top;
  if (shift == 0) return;
  for (uint32_t i = pinned; i < lv_obj_get_child_count(root); ++i) {
    lv_obj_t *child = lv_obj_get_child(root, i);
    if (child && !lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) lv_obj_set_y(child, lv_obj_get_y(child) + shift);
  }
}

// Anything a finger must hit: the drawn size may be thinner than a finger, the touch area may not.
inline void touchable(lv_obj_t *object, int drawn_thickness) {
  if (!object) return;
  const int grow = std::max(0, (ui::touch_min() - drawn_thickness) / 2);
  lv_obj_set_ext_click_area(object, grow);
}

}  // namespace overlay_card
