# 0.4.12 acceptance (firmware 0.8.0)

This release brings the bedside clock: a tile that takes a whole page, with the time as large as the glass allows and
up to three round keys under it. A key is a tile without a cell of its own (docs/BEDSIDE.md). These are the checks
that were carried out, and what they did not cover.

## Automated checks

- `tools/check.sh` on the release commit: 867 Python tests, 33 C++ programs, the package, cell, board shape, entry
  file, icon and translation checks, and the editor's 360 tests, types, build and bundle.
- `tools/check.sh --firmware`: every board builds with ESPHome 2026.9.0. The CYD image is 1,670,928 B, 91.1 % of its
  slot, 17,248 B more than 0.4.11: about 13.5 KB for its 80-pixel digits and the rest for the clock and its keys. The
  Hosyond 4-inch, with the same 4 MB of flash, is 1,706,512 B, 93.0 %: its 155-pixel digits take about 50 KB. The owner
  accepted both.
- New tests: `tests/test_nightstand.py` (14) covers the document, the compiled tiles, the wire, the firmware gate, the
  tile events and the watched entities with keys. `web/tests/bedside.spec.ts` (6) covers the editor: a key placed,
  moved, swapped, dragged back onto the grid, and a full grid with a bedside clock that still packs.
- The host render harness (tools/render) draws the clock with three keys on every board, lying down and standing up:
  24 of 24 variants pass the self test, which fails a board where the digits fit no arrangement or a key leaves its
  clock. A comparison with the renders of the release before showed no change outside the new stage, apart from
  animations caught at another moment.
- A virtual finger on the host firmware (Guition): a tap on the lamp key sent `light.toggle` for that lamp, holding it
  opened the lamp's card, the lock key asked for a second tap and then sent `lock.unlock`, a tap on the temperature key
  opened its history card, and the back key returned to the clock.

## On the screens

Tested on 27 September 2026 on a 4-inch Guition 4848S040 and a CYD ESP32-2432S028 over OTA, with a real Home Assistant
and this release installed as a local add-on. The owner used the editor on the live pages and looked at the screens.

- **The editor:** the owner dragged tiles onto the round places and removed a key, which found the two editor bugs
  below.
- **Live states (Guition):** keys follow Home Assistant as a tile does. A switch turned off and on through Home
  Assistant turned its key grey and amber, a lamp turned on and off turned it amber and grey, and both were set back
  afterwards. The minute changed on time.
- **A lock and an alarm panel as keys (Guition):** a test lock and a test alarm panel, template entities over two
  input_select helpers with no real device behind them, removed afterwards. Through Home Assistant (the entities' own
  actions, and the helpers for the in-between states): the
  lock locked showed a green lock, unlocked a red open lock; the alarm disarmed showed a grey shield, arming an orange
  one with its slow beat, armed away a green one; triggered opened the alarm's card with the red bell, as a tile
  does; disarmed brought the grey shield back. Pushed directly to the screen: a jammed lock showed red with its mark.
- **The CYD:** a bedside page added to its layout with a lamp, the test lock and the test alarm panel as keys, on the
  final firmware. The add-on reported the layout applied; the lock unlocked and locked again and the alarm was armed
  for the night through Home Assistant, and the screen stayed up; its lowest free heap was 122 KB. This screen cannot
  save a picture of its glass, so what it drew was not captured.
- Neither screen restarted during these checks: the Guition ran 1 h 3 min after its last OTA, the CYD 36 minutes.

### Found and fixed on the way

- **Keys did not follow their state.** The add-on kept the keys inside their clock's options, so the list of entities
  it watches never held them. Keys are now tiles in the compiled list, and everything that reads that list treats
  them as tiles.
- **A drop on a round place stopped the editor** with "Tiles do not fit this screen grid": the check that packs the
  grid counted the keys as tiles that need a cell. It now leaves them out.
- **A round key had no remove key.** It now has the tile's own, placed at the circle's edge.
- **The digits of the 4.3-inch Waveshare were too large.** ESPHome resolves substitutions in the order it merges the
  files, and a size built from another computed size came out as 0, so the digits were sized for keys without names.
  The looks now set a plain flag, and ESPHome and `tools/profiles.py` agree on every board. The render self test found
  it.
- **The alarm's ring was cut off at the key's edge.** A key's circle fills its card, and a card cuts off what its
  children draw past its edge. The ring and the spring of an alarm or a lock now run on the key's card itself.

## Not covered

- A finger on the glass of a real screen on the keys; the taps were checked with the host firmware's virtual finger.
- The bedside clock on the other boards' glass: they were checked in the host renders only.
- A real alarm panel or lock: only the test entities above, and no real device was armed, disarmed, locked or
  unlocked.
