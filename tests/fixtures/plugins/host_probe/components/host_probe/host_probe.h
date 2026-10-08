#pragma once
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::host_probe {

// The probe: every moment of the plugin API as one log line, tag "probe" (tools/render/run.py reads them).
class HostProbe : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void on_ready() override;
  void on_interval(uint32_t now_ms) override;
  void on_standby(bool dark) override;
  void before_update() override;
  void on_cards_closed() override;
  void on_alert() override;
  void on_touch() override;
  bool settings(tessera::SettingsPage &page) override;
  void on_message(JsonObjectConst message) override;
  // The first tile made asks the app one question (tessera::send); the answer comes back in on_message.
  void ask_once();
  // What happened at boot, before the harness could listen: said again with every tile made (the "status" line).
  bool ready_ = false, interval_ = false;
  int updates_ = 0;
  unsigned rows_ = 0;

 private:
  bool asked_ = false, interval_said_ = false, bar_said_ = false, toggle_ = false;
  int number_ = 3;
};

}  // namespace esphome::host_probe
