#pragma once
#include "lvgl.h"
#include "esphome/core/log.h"

// On-demand 240x240 diagnostic image of LVGL's rendered screen. Nothing is
// allocated or streamed during normal use. The RGB display signal itself
// still needs a physical visual check.
namespace screen_diagnostics {
inline bool check_geometry(lv_obj_t *obj) {
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return true;
  bool ok = true;
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE) &&
      (area.x1 < 0 || area.y1 < 0 || area.x2 >= lv_display_get_horizontal_resolution(lv_display_get_default()) ||
       area.y2 >= lv_display_get_vertical_resolution(lv_display_get_default()))) {
    ESP_LOGE("ui_test", "GEOMETRY FAIL clickable=(%d,%d)-(%d,%d)",
             (int) area.x1, (int) area.y1, (int) area.x2, (int) area.y2);
    ok = false;
  }
  for (uint32_t i = 0; i < lv_obj_get_child_count(obj); ++i)
    ok = check_geometry(lv_obj_get_child(obj, i)) && ok;
  return ok;
}
#if LV_USE_SNAPSHOT
inline lv_draw_buf_t *snapshot = nullptr;
// LVGL's top layer (an alert, a camera full screen) when something is shown on it, with its transparency.
inline lv_draw_buf_t *overlay = nullptr;
inline int snapshot_row = 0;
inline void snapshot_begin() {
  if (snapshot) lv_draw_buf_destroy(snapshot);
  if (overlay) lv_draw_buf_destroy(overlay);
  overlay = nullptr;
  snapshot = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
  snapshot_row = 0;
  const int width = lv_display_get_horizontal_resolution(lv_display_get_default());
  const int height = lv_display_get_vertical_resolution(lv_display_get_default());
  if (!snapshot || (int) snapshot->header.w != width || (int) snapshot->header.h != height) {
    if (snapshot) lv_draw_buf_destroy(snapshot);
    snapshot = nullptr;
    ESP_LOGE("ui_image", "IMAGE FAIL allocation/dimensions");
    return;
  }
  for (uint32_t i = 0; i < lv_obj_get_child_count(lv_layer_top()); ++i)
    if (!lv_obj_has_flag(lv_obj_get_child(lv_layer_top(), i), LV_OBJ_FLAG_HIDDEN)) {
      overlay = lv_snapshot_take(lv_layer_top(), LV_COLOR_FORMAT_ARGB8888);
      if (overlay && ((int) overlay->header.w != width || (int) overlay->header.h != height)) { lv_draw_buf_destroy(overlay); overlay = nullptr; }
      break;
    }
  ESP_LOGI("ui_image", "IMAGE BEGIN 240 240%s", overlay ? " with top layer" : "");
}
// The top layer's pixel over the page's, as the panel shows them.
inline uint16_t layered(uint16_t pixel, int x, int y) {
  if (!overlay) return pixel;
  const uint8_t *p = overlay->data + y * overlay->header.stride + x * 4;  // B, G, R, A
  const unsigned alpha = p[3];
  if (!alpha) return pixel;
  const unsigned r = ((pixel >> 11) & 31) * 255 / 31, g = ((pixel >> 5) & 63) * 255 / 63, b = (pixel & 31) * 255 / 31;
  const unsigned mr = (p[2] * alpha + r * (255 - alpha)) / 255, mg = (p[1] * alpha + g * (255 - alpha)) / 255,
                 mb = (p[0] * alpha + b * (255 - alpha)) / 255;
  return static_cast<uint16_t>((mr * 31 / 255) << 11 | (mg * 63 / 255) << 5 | (mb * 31 / 255));
}
inline void snapshot_next_row() {
  if (!snapshot) return;
  const auto *row = reinterpret_cast<const uint16_t *>(
      snapshot->data + snapshot_row * 2 * snapshot->header.stride);
  static constexpr char hex[] = "0123456789abcdef";
  char line[961];
  for (int x = 0; x < 240; ++x) {
    uint16_t pixel = layered(row[x * 2], x * 2, snapshot_row * 2);
    for (int digit = 0; digit < 4; ++digit)
      line[x * 4 + digit] = hex[(pixel >> (12 - digit * 4)) & 15];
  }
  line[960] = 0;
  ESP_LOGI("ui_image", "ROW %03d %s", snapshot_row, line);
  if (++snapshot_row == 240) {
    lv_draw_buf_destroy(snapshot);
    snapshot = nullptr;
    if (overlay) lv_draw_buf_destroy(overlay);
    overlay = nullptr;
    ESP_LOGI("ui_image", "IMAGE COMPLETE");
  }
}
#endif
}  // namespace screen_diagnostics
