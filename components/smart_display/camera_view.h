#pragma once
// A camera image full screen (firmware 0.2.57+, Guition): when to ask ESP Screen Manager for a link and when to load it
// again. Pure bookkeeping, free of LVGL and ESPHome, so tests/test_camera_view.cpp can check it on a PC; runtime_tiles.h
// draws the view and the board's online_image loads the link.
//
// The app answers `esphome.screen_camera` with a link to a BMP it keeps fresh (screen_manager/app/camera_feed.py).
// Loading shares the main loop with touch and drawing, so the view loads one image at a time and leaves time free after
// each. The rhythm is fixed rather than as fast as the Wi-Fi allows: a picture every four seconds, every time, reads as
// smoother than one after three seconds and the next after five. Never a queue: a slow load makes the next one later.
#include <cstdint>
#include <string>

namespace camera_view {
constexpr uint32_t REFRESH_MS = 4000;      // from the start of one load to the start of the next: a steady rhythm
constexpr uint32_t GAP_MS = 800;           // at least this long between the end of one load and the next
constexpr uint32_t ASK_AGAIN_MS = 10000;   // a link that does not come (or an app without an image) is asked for again
constexpr uint8_t MAX_FAILURES = 3;        // a link that fails this often in a row is old: ask for a new one
constexpr uint32_t PENDING_MS = 20000;     // an alert's camera announced this long before the alert still belongs to it

struct Feed {
  std::string entity, url;
  uint32_t asked_at = 0, started_at = 0, finished_at = 0;
  bool asked = false, loading = false, shown = false, empty = false;  // empty: the app said it has no image
  uint8_t failures = 0;

  void open(const std::string &camera) { *this = Feed{}; entity = camera; }
  bool open() const { return !entity.empty(); }
  bool should_ask(uint32_t now) const { return open() && !loading && url.empty() && (!asked || now - asked_at >= ASK_AGAIN_MS); }
  void ask(uint32_t now) { asked = true; asked_at = now; }
  // The app's answer: a link, or none (no image from Home Assistant, or a camera this screen may not show).
  void link(const std::string &address) {
    url = address;
    empty = address.empty();
    failures = 0;
    started_at = finished_at = 0;
  }
  bool should_load(uint32_t now) const {
    if (!open() || loading || url.empty()) return false;
    if (!started_at) return true;
    return now - started_at >= REFRESH_MS && now - finished_at >= GAP_MS;
  }
  void start(uint32_t now) { loading = true; started_at = now ? now : 1; }
  void finish(uint32_t now, bool ok) {
    loading = false;
    finished_at = now;
    if (ok) { shown = true; failures = 0; return; }
    if (++failures >= MAX_FAILURES) { url.clear(); asked = false; failures = 0; }
  }
};
}  // namespace camera_view
