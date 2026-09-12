#include "components/smart_display/runtime_model.h"
#include "components/smart_display/cyd_ui.h"
#include <cassert>

int main() {
  cyd::TouchGuard guard;
  guard.begin(0); guard.consume();
  assert(!guard.accept(100,1)); assert(!guard.accept_slider(100,1));
  guard.begin(200); assert(guard.accept(300,1));

  runtime_tiles::Tile t;
  t.revision="off"; t.begin(100);
  assert(t.loading(200)); assert(t.awaiting_action(200));
  t.observe("off"); assert(!t.confirmed);
  t.observe("on"); assert(t.confirmed);
  assert(t.loading(999)); assert(!t.loading(1100));
  t.begin(2000); t.observe("on");
  assert(t.loading(7999)); assert(!t.loading(8000)); assert(!t.awaiting_action(8000));
  t.begin(0xFFFFFFF0); assert(t.loading(20));
  t.begin(100,true);
  assert(t.loading(1099)); assert(!t.awaiting_action(1099)); assert(!t.loading(1100));

  // A plain switch changes state with identical attributes. This must confirm
  // immediately instead of leaving the tile blocked for the six-second timeout.
  runtime_tiles::Tile sw;
  sw.entity="switch.printer";
  sw.revision=runtime_tiles::state_revision("off","{}"); sw.begin(100);
  assert(sw.loading(249));
  sw.observe(runtime_tiles::state_revision("on","{}"));
  assert(!sw.loading(250));
  sw.begin(300); sw.observe(runtime_tiles::state_revision("on","{}"));
  assert(sw.loading(1000)); // Repeated identical data is not confirmation.
  assert(!sw.loading(6300));
  sw.entity="input_boolean.test"; sw.begin(0xFFFFFFF0); sw.observe("off");
  assert(sw.loading(20)); assert(!sw.loading(200));
}
