#pragma once
#include "lvgl.h"
#include "esphome/core/log.h"

// On-demand 240x240 diagnostic image of LVGL's rendered screen. Nothing is
// allocated or streamed during normal use. The RGB display signal itself
// still needs a physical visual check.
namespace guition {
inline bool check_geometry(lv_obj_t *obj) {
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return true;
  bool ok = true;
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE) &&
      (area.x1 < 0 || area.y1 < 0 || area.x2 >= 480 || area.y2 >= 480)) {
    ESP_LOGE("ui_test", "GEOMETRY FAIL clickable=(%d,%d)-(%d,%d)",
             area.x1, area.y1, area.x2, area.y2);
    ok = false;
  }
  for (uint32_t i = 0; i < lv_obj_get_child_count(obj); ++i)
    ok = check_geometry(lv_obj_get_child(obj, i)) && ok;
  return ok;
}
inline lv_draw_buf_t *snapshot = nullptr;
inline int snapshot_row = 0;
inline void snapshot_begin() {
  if (snapshot) lv_draw_buf_destroy(snapshot);
  snapshot = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
  snapshot_row = 0;
  if (!snapshot || snapshot->header.w != 480 || snapshot->header.h != 480) {
    if (snapshot) lv_draw_buf_destroy(snapshot);
    snapshot = nullptr;
    ESP_LOGE("ui_image", "IMAGE FAIL allocation/dimensions");
    return;
  }
  ESP_LOGI("ui_image", "IMAGE BEGIN 240 240");
}
inline void snapshot_next_row() {
  if (!snapshot) return;
  const auto *row = reinterpret_cast<const uint16_t *>(
      snapshot->data + snapshot_row * 2 * snapshot->header.stride);
  static constexpr char hex[] = "0123456789abcdef";
  char line[961];
  for (int x = 0; x < 240; ++x) {
    uint16_t pixel = row[x * 2];
    for (int digit = 0; digit < 4; ++digit)
      line[x * 4 + digit] = hex[(pixel >> (12 - digit * 4)) & 15];
  }
  line[960] = 0;
  ESP_LOGI("ui_image", "ROW %03d %s", snapshot_row, line);
  if (++snapshot_row == 240) {
    lv_draw_buf_destroy(snapshot);
    snapshot = nullptr;
    ESP_LOGI("ui_image", "IMAGE COMPLETE");
  }
}
}  // namespace guition
