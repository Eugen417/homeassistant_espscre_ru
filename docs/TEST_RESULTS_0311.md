# The alarm panel, 0.3.11 acceptance (firmware 0.3.6)

Tested on 26 September 2026: on the host render harness for every board, and on three bench screens (a 4-inch
Guition 4848S040, a Waveshare ESP32-S3-Touch-LCD-4.3 and a CYD ESP32-2432S028) over OTA, with a real Home Assistant
and this add-on installed as a local add-on. No real alarm was armed or disarmed: the screens got a test alarm, a
state set through Home Assistant's REST API (`POST /api/states/alarm_control_panel.<test>`) with no integration
behind it, removed afterwards.

## What Home Assistant does, which the screen follows

Read in Home Assistant's own code before the design (core `alarm_control_panel`, its `manual` platform, the ESPHome
integration's `manager.py`, and the frontend's alarm dialog, alarm panel card and code dialog, 2026.9):

- The modes come from `supported_features`; the frontend orders them home, away, night, vacation, custom bypass,
  disarmed. Trigger is not offered, as in Home Assistant's own dialogs.
- A code is asked for when disarming and the panel has a `code_format`, or when arming and it also has
  `code_arm_required`, unless the entity has a default code in its registry options.
- Integrations answer a wrong code three ways: a `ServiceValidationError` (the manual alarm), which ESPHome passes back
  to the screen as a refusal; a `HomeAssistantError`, which ESPHome's action response route does not pass back; or
  nothing at all (Alarmo only logs a warning). The screen counts a refusal, or a state that has not moved after ten
  seconds, as a failed attempt.

## Automated checks

- `tools/check.sh`: 747 Python tests (new: `tests/test_alarm_panel.py`, which holds the modes, colours, icons and code
  rule against Home Assistant's and keeps the code out of logs and events), every C++ program (new:
  `tests/test_alarm_panel.cpp`: modes, the code rule, attempts across the `millis()` wrap, the lock times, and the
  card and keypad layouts on eight shapes of glass in both looks with one to six modes), and the editor's 295 tests.
- `tools/check.sh --firmware`: every board compiles with ESPHome 2026.9.0. The CYD image is 1,662,416 bytes of
  1,835,008 (90.6 %) after the rebase on 0.3.7; the alarm panel adds 12,960 bytes. The owner approved that growth on
  the condition that the CYD runs well on the glass, which it did (below).
- The host render harness (`tools/render/run.py --only alarm`) drove the whole flow on all 16 variants with a finger
  on the simulated touchscreen: the tile opens the card, a mode opens the keypad, the digits and OK send the action
  with the code (read back from the host program's action stream), the answer and the new states come back through
  the add-on's own messages, pending wakes the screen with the keypad, a refused code says so and counts, three lock
  the keypad for 30 seconds with every key disabled, triggered shows the Disarm key, and a panel without a code sends
  its mode without one. `render_alarm` checked that every key a finger uses lies inside the glass and over no other.
  All 16 passed.

## On the screens

- **The chain.** A test alarm in Home Assistant went through the add-on to all three screens as a tile. Its colour,
  icon and Home Assistant's own words, in the screen's language, followed every state, and the tile counted the exit delay down.
- **Waking.** Setting the test alarm to `pending` woke each screen and opened its card with the keypad to disarm,
  within four seconds on all three.
- **Snapshots of the Guition.** The card (the shield and a key per mode, the chosen mode in green), the keypad during
  the entry delay and the card while the alarm goes off were drawn as designed.
- **Soak.** With the card open and the alarm going off (the heartbeat running): 4 minutes on the CYD, 2 on the Guition
  and 2 on the Waveshare. No restart and no error in the log. Free heap on the CYD stayed at 136 KB (lowest since the
  start 129 KB), on the Guition 91 KB and on the Waveshare 98 KB. One "api took a long time" warning of 229 ms on the
  CYD, while a card was drawn.
- **No actions.** None of the screens sent an alarm action during the test.

## Not tested on the glass

- Typing a code with a finger and the lock after wrong codes: nobody was at the bench. The host render harness
  covered both with real touches through ESPHome's touchscreen.
- A real alarm integration (Alarmo's countdown, a refusal from a real panel).
