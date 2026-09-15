#pragma once
#include "calibration_math.h"
#include "esphome/core/preferences.h"
#include "esphome/core/hal.h"
#include "lvgl.h"
#include <functional>
namespace screen_calibration {
inline bool active = false, pressed = false;
inline unsigned point = 0, sample = 0;
inline uint32_t down_at = 0;
inline lv_obj_t *screen{}, *home{}, *cross{}, *instructions{}, *status{};
inline Calibration calibration;
inline std::array<std::array<Point,3>,5> samples;
inline esphome::ESPPreferenceObject preference;
inline std::function<void(const Calibration &)> apply;
inline std::function<void(bool)> isolation;
inline void prompt(const char *message = "Hold each tap for a moment") {
  lv_obj_set_pos(cross, TARGETS[point].x-12, TARGETS[point].y-12);
  lv_label_set_text_fmt(instructions, "Tap the crosshair\nPoint %u/5 - tap %u/3", point+1, sample+1);
  lv_label_set_text(status, message);
}
inline void begin() {
  if (!screen) return;
  active = true; pressed = false; point = sample = 0;
  if (isolation) isolation(true);
  prompt(); lv_screen_load(screen);
}
inline void setup(lv_obj_t *home_screen, const lv_font_t *font, const lv_font_t *large,
                  int xmin, int xmax, int ymin, int ymax) {
  if (screen) return;
  home = home_screen;
  calibration.bounds[0]=xmin; calibration.bounds[1]=xmax; calibration.bounds[2]=ymin; calibration.bounds[3]=ymax;
  preference = esphome::global_preferences->make_preference<Calibration>(0x43594403);
  Calibration stored;
  bool valid = preference.load(&stored);
  for (int i = 0; i < 4; ++i) valid &= stored.bounds[i] == calibration.bounds[i];
  for (float coefficient : stored.c) valid &= std::isfinite(coefficient);
  screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(screen, font, 0);
  instructions = lv_label_create(screen);
  lv_obj_set_style_text_align(instructions, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(instructions, LV_ALIGN_CENTER, 0, -40);
  status = lv_label_create(screen); lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -42);
  cross = lv_label_create(screen); lv_label_set_text(cross, "+");
  lv_obj_set_size(cross, 24, 24); lv_obj_set_style_text_font(cross, large, 0);
  lv_obj_set_style_text_align(cross, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(cross, lv_color_hex(0xFFD34D), 0);
  if (valid) { calibration = stored; if (apply) apply(calibration); }
  else begin();
}
inline void press() { if (active) { down_at = esphome::millis(); pressed = true; } }
inline void release(int raw_x, int raw_y) {
  if (!active || !pressed) return;
  pressed = false;
  if (esphome::millis()-down_at < 80 || raw_x < 0 || raw_x > 4095 || raw_y < 0 || raw_y > 4095) { prompt("Tap gently, a bit longer"); return; }
  samples[point][sample++] = {double(raw_x), double(raw_y)};
  if (sample < 3) { prompt(); return; }
  double xs[3], ys[3];
  for (int i=0;i<3;++i) { xs[i]=samples[point][i].x; ys[i]=samples[point][i].y; }
  std::sort(xs,xs+3); std::sort(ys,ys+3);
  if (xs[2]-xs[0]>180 || ys[2]-ys[0]>180) { sample=0; prompt("Tap the same crosshair 3 times"); return; }
  point++; sample=0;
  if (point < 5) { prompt(); return; }
  std::array<Point,5> raw;
  for (unsigned n=0;n<5;++n) {
    for (int i=0;i<3;++i) { xs[i]=samples[n][i].x; ys[i]=samples[n][i].y; }
    std::sort(xs,xs+3);std::sort(ys,ys+3);raw[n]={xs[1],ys[1]};
  }
  Calibration next = calibration;
  if (!fit(raw,next)) { point=0;prompt("Measurement rejected; try again");return; }
  if (!preference.save(&next) || !esphome::global_preferences->sync()) { point=0;prompt("Save failed; try again");return; }
  calibration=next;
  if (apply) apply(calibration);
  active=false;
  if (isolation) isolation(false);
  lv_screen_load(home);
}
}
