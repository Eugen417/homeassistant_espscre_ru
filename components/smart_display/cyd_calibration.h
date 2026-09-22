#pragma once
#include "calibration_math.h"
#include "esphome/core/preferences.h"
#include "esphome/core/hal.h"
#include "lvgl.h"
#include "theme.h"
#include "screen_text.h"
#include <functional>
namespace screen_calibration {
inline bool active = false, pressed = false;
inline unsigned point = 0, sample = 0;
inline uint32_t down_at = 0;
inline lv_obj_t *screen{}, *home{}, *cross{}, *instructions{}, *status{};
inline Calibration calibration;
inline std::array<std::array<Point,3>,5> samples;
// Where the screen itself put each tap. Together with the raw reading it says what the whole chain from
// panel to glass does, so the fit needs to know nothing about rotation or mirroring (firmware 0.2.92+).
inline std::array<std::array<Point,3>,5> screen_samples;
inline Point last_screen;
inline esphome::ESPPreferenceObject preference;
inline std::function<void(const Calibration &)> apply;
inline std::function<void(bool)> isolation;
inline void prompt(const char *message = nullptr) {
  if (!message) message = screen_text::tr(screen_text::txt::calibration_hold);
  const Point at = target(point);
  lv_obj_set_pos(cross, at.x-12, at.y-12);
  lv_label_set_text(instructions, screen_text::fill(screen_text::fill(screen_text::txt::calibration_crosshair, "point", (int) point + 1), "tap", std::to_string(sample + 1)).c_str());
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
  // The crosses stand on the canvas the person is looking at, so the wizard works the same on a screen built
  // standing up (firmware 0.2.92+). Which way the panel's axes run under that canvas is not worked out here:
  // the fit measures it from the taps themselves.
  auto *display = lv_display_get_default();
  bind(lv_display_get_horizontal_resolution(display), lv_display_get_vertical_resolution(display));
  calibration.bounds[0]=xmin; calibration.bounds[1]=xmax; calibration.bounds[2]=ymin; calibration.bounds[3]=ymax;
  preference = esphome::global_preferences->make_preference<Calibration>(0x43594403);
  Calibration stored;
  bool valid = preference.load(&stored);
  for (int i = 0; i < 4; ++i) valid &= stored.bounds[i] == calibration.bounds[i];
  for (float coefficient : stored.c) valid &= std::isfinite(coefficient);
  screen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(screen, theme::color(theme::CALIBRATION_PAGE), 0);
  lv_obj_set_style_text_color(screen, theme::color(theme::CALIBRATION_INK), 0);
  lv_obj_set_style_text_font(screen, font, 0);
  // Both lines keep a margin from the edges and wrap inside it, so they read on narrow glass too.
  const int text_width = canvas.width - 32;
  instructions = lv_label_create(screen);
  lv_obj_set_style_text_align(instructions, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(instructions, text_width);
  lv_label_set_long_mode(instructions, LV_LABEL_LONG_WRAP);
  lv_obj_align(instructions, LV_ALIGN_CENTER, 0, -40);
  status = lv_label_create(screen);
  lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(status, text_width);
  lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
  lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -42);
  cross = lv_label_create(screen); lv_label_set_text(cross, "+");
  lv_obj_set_size(cross, 24, 24); lv_obj_set_style_text_font(cross, large, 0);
  lv_obj_set_style_text_align(cross, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(cross, theme::color(theme::CALIBRATION_MARK), 0);
  if (valid) { calibration = stored; if (apply) apply(calibration); }
  else begin();
}
inline void press(int screen_x, int screen_y) {
  if (!active) return;
  down_at = esphome::millis(); pressed = true;
  last_screen = {double(screen_x), double(screen_y)};
}
inline void release(int raw_x, int raw_y) {
  if (!active || !pressed) return;
  pressed = false;
  if (esphome::millis()-down_at < 80 || raw_x < 0 || raw_x > 4095 || raw_y < 0 || raw_y > 4095) { prompt(screen_text::tr(screen_text::txt::calibration_gently)); return; }
  screen_samples[point][sample] = last_screen;
  samples[point][sample++] = {double(raw_x), double(raw_y)};
  if (sample < 3) { prompt(); return; }
  double xs[3], ys[3];
  for (int i=0;i<3;++i) { xs[i]=samples[point][i].x; ys[i]=samples[point][i].y; }
  std::sort(xs,xs+3); std::sort(ys,ys+3);
  if (xs[2]-xs[0]>180 || ys[2]-ys[0]>180) { sample=0; prompt(screen_text::tr(screen_text::txt::calibration_same)); return; }
  point++; sample=0;
  if (point < 5) { prompt(); return; }
  std::array<Point,5> raw, reported;
  for (unsigned n=0;n<5;++n) {
    for (int i=0;i<3;++i) { xs[i]=samples[n][i].x; ys[i]=samples[n][i].y; }
    std::sort(xs,xs+3);std::sort(ys,ys+3);raw[n]={xs[1],ys[1]};
    for (int i=0;i<3;++i) { xs[i]=screen_samples[n][i].x; ys[i]=screen_samples[n][i].y; }
    std::sort(xs,xs+3);std::sort(ys,ys+3);reported[n]={xs[1],ys[1]};
  }
  Calibration next = calibration;
  if (!fit(raw,reported,next)) { point=0;prompt(screen_text::tr(screen_text::txt::calibration_rejected));return; }
  if (!preference.save(&next) || !esphome::global_preferences->sync()) { point=0;prompt(screen_text::tr(screen_text::txt::calibration_save_failed));return; }
  calibration=next;
  if (apply) apply(calibration);
  active=false;
  if (isolation) isolation(false);
  lv_screen_load(home);
}
}
