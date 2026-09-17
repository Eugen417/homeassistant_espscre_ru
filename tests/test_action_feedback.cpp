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
  t.entity="light.lamp";
  t.revision=rev("off"); t.begin(100);
  // Nothing is drawn in the first 400 ms, so a command Home Assistant confirms in 350 ms looks instant. A second
  // finger is ignored from the first moment all the same.
  assert(t.waiting(200)); assert(!t.loading(200));
  assert(t.loading(500));
  t.observe(rev("off")); assert(!t.confirmed);
  t.observe(rev("on")); assert(t.confirmed);
  assert(!t.waiting(500)); assert(!t.loading(500));

  // Without a state the wait runs to three seconds, not six.
  t.begin(2000); t.observe(rev("on"));
  assert(t.waiting(4999)); assert(t.loading(2400)); assert(!t.loading(2399));
  assert(!t.waiting(5000)); assert(!t.loading(5000));

  // "It worked" without a new state (a stop on a cover that already stands still) ends the wait 800 ms later.
  t.begin(2000); t.observe(rev("on")); t.answered_at=2600;
  assert(t.waiting(3399)); assert(!t.waiting(3400)); assert(!t.loading(3400));
  t.begin(2000); assert(t.answered_at==0);

  // A tap that opens a card draws no sheet at all, and the clock wrapping around changes nothing.
  t.begin(100,true);
  assert(!t.waiting(200)); assert(!t.loading(1099));
  t.begin(0xFFFFFFF0); t.observe(rev("on"));
  assert(t.waiting(20)); assert(!t.loading(20)); assert(t.loading(0xFFFFFFF0+400));

  // A switch changes state with identical attributes. That still confirms at once, and within the first 400 ms
  // nothing was drawn anyway.
  runtime_tiles::Tile sw;
  sw.entity="switch.printer";
  sw.revision=runtime_tiles::state_revision("off","{}"); sw.begin(100);
  assert(!sw.loading(249));
  sw.observe(runtime_tiles::state_revision("on","{}"));
  assert(!sw.waiting(250)); assert(!sw.loading(500));
  sw.begin(300); sw.observe(runtime_tiles::state_revision("on","{}"));
  assert(sw.waiting(1000)); // Repeated identical data is not confirmation.
  assert(!sw.waiting(3300));

  // On / off shows the new stand at once, as Home Assistant's own switch does.
  runtime_tiles::Tile lamp;
  lamp.entity="light.lamp"; lamp.state="off"; lamp.revision=rev("off");
  lamp.optimistic(true); lamp.begin(100);
  assert(lamp.state=="on"); assert(lamp.optimistic_tap); assert(lamp.optimistic_on);
  // Home Assistant refuses, or never answers: the old stand comes back.
  lamp.undo_optimistic();
  assert(lamp.state=="off"); assert(!lamp.optimistic_tap);
  lamp.undo_optimistic(); assert(lamp.state=="off");
  // A state message always wins: it clears the flag, so a later wait that runs out changes nothing.
  lamp.optimistic(true); lamp.begin(200);
  lamp.state="off"; lamp.observe(rev("off"));
  assert(!lamp.optimistic_tap);
  lamp.undo_optimistic(); assert(lamp.state=="off");

  // The fingerprint follows state and attributes, and the streaming writer (what receive() hands to
  // ArduinoJson) gives the same value as hashing the joined text.
  assert(runtime_tiles::state_revision("on","{\"brightness\":255}")!=runtime_tiles::state_revision("on","{\"brightness\":254}"));
  assert(runtime_tiles::state_revision("on","{}")!=runtime_tiles::state_revision("off","{}"));
  assert(runtime_tiles::state_revision("o","n{}")!=runtime_tiles::state_revision("on","{}"));
  runtime_tiles::Fingerprint f; f.add("on"); f.write('\n');
  const char json[]="{\"a\":1}"; f.write(reinterpret_cast<const uint8_t*>(json),sizeof(json)-1);
  assert(f.value==runtime_tiles::state_revision("on","{\"a\":1}"));
}
