#pragma once
// An update on the glass (firmware 0.38.0). While new firmware comes in, ESPHome's OTA loop holds the main loop: LVGL
// draws nothing, and a screen looked frozen for a minute. ESPHome tells every listener how far the update is, from
// whichever OTA platform the screen's own YAML has (smart_display's __init__.py asks it for that), about once a second.
// The screen draws that at once, over everything, and says it restarts at the end. No YAML of the screen changes.
#include <cstdint>
#if defined(USE_OTA_STATE_LISTENER) && !defined(ESP_SCREEN_HOST)
#include "esphome/components/ota/ota_backend.h"
#endif

namespace ota_status {

enum class Phase : uint8_t { none, running, done, failed };
struct Progress {
  Phase phase = Phase::none;
  int percent = 0;
};
inline Progress now;
// The screen's drawing of it (runtime_tiles binds it): called on ESPHome's loop, in the middle of the update.
inline void (*draw)() = nullptr;
// Light the glass and close the screensaver before the first drawing (packages/core.yaml binds it). The loop that would
// run ESPHome's light transition is held by the update, so the board sets its backlight output at once.
inline void (*wake)() = nullptr;

#if defined(USE_OTA_STATE_LISTENER) && !defined(ESP_SCREEN_HOST)
class Listener : public esphome::ota::OTAGlobalStateListener {
 public:
  void on_ota_global_state(esphome::ota::OTAState state, float progress, uint8_t, esphome::ota::OTAComponent *) override {
    Progress next = now;
    switch (state) {
      case esphome::ota::OTA_STARTED: next = {Phase::running, 0}; break;
      case esphome::ota::OTA_IN_PROGRESS: next = {Phase::running, progress < 0 ? 0 : progress > 100 ? 100 : (int) progress}; break;
      case esphome::ota::OTA_COMPLETED: next = {Phase::done, 100}; break;
      default: next = {Phase::failed, now.percent}; break;
    }
    if (next.phase == now.phase && next.percent == now.percent) return;
    now = next;
    if (draw) draw();
  }
};
inline void watch() {
  static Listener listener;
  static bool watching = false;
  if (watching) return;
  watching = true;
  esphome::ota::get_global_ota_callback()->add_global_state_listener(&listener);
}
#else
inline void watch() {}
#endif

}  // namespace ota_status
