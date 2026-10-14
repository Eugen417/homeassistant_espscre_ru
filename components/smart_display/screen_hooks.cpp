// The screen's moments as lists (screen_hooks.h). Apart from main.cpp so each lambda that runs one holds a single call.
#include "screen_hooks.h"

namespace screen_hooks {

void run_tick(uint32_t now_ms) {
  for (auto &hook : tick()) hook(now_ms);
}
void run_cards_closed() {
  for (auto &hook : cards_closed()) hook();
}
bool settings_kept_closed() {
  for (auto &hook : keeps_settings_closed())
    if (hook()) return true;
  return false;
}
bool is_away() {
  for (auto &hook : away())
    if (hook()) return true;
  return false;
}
void run_alert_show() {
  for (auto &hook : alert_show()) hook();
}

}  // namespace screen_hooks
