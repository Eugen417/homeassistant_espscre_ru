#include "components/smart_display/runtime_model.h"
#include "components/smart_display/cyd_ui.h"
#include <cassert>

int main() {
  cyd::TouchGuard guard;
  guard.begin(0); guard.consume();
  assert(!guard.accept(100,1)); assert(!guard.accept_slider(100,1));
  guard.begin(200); assert(guard.accept(300,1));

  // A tile keeps the fingerprint of its last state message, not the message itself.
  auto rev=[](const char *state){ return runtime_tiles::state_revision(state,""); };
  runtime_tiles::Tile t;
  t.revision=rev("off"); t.begin(100);
  assert(t.loading(200)); assert(t.awaiting_action(200));
  t.observe(rev("off")); assert(!t.confirmed);
  t.observe(rev("on")); assert(t.confirmed);
  assert(t.loading(999)); assert(!t.loading(1100));
  t.begin(2000); t.observe(rev("on"));
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
  sw.entity="input_boolean.test"; sw.begin(0xFFFFFFF0); sw.observe(rev("off"));
  assert(sw.loading(20)); assert(!sw.loading(200));

  // The fingerprint follows state and attributes, and the streaming writer (what receive() hands to
  // ArduinoJson) gives the same value as hashing the joined text.
  assert(runtime_tiles::state_revision("on","{\"brightness\":255}")!=runtime_tiles::state_revision("on","{\"brightness\":254}"));
  assert(runtime_tiles::state_revision("on","{}")!=runtime_tiles::state_revision("off","{}"));
  assert(runtime_tiles::state_revision("o","n{}")!=runtime_tiles::state_revision("on","{}"));
  runtime_tiles::Fingerprint f; f.add("on"); f.write('\n');
  const char json[]="{\"a\":1}"; f.write(reinterpret_cast<const uint8_t*>(json),sizeof(json)-1);
  assert(f.value==runtime_tiles::state_revision("on","{\"a\":1}"));
}
