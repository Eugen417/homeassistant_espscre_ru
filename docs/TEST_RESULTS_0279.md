# Test results app 0.2.79 / firmware 0.2.66 (2026-09-18)

Max, after 0.2.78: "Blinds schuif is andersom in de overlay mode en op de tile (dicht is open bij de ander)", and on the
choice: "full closed = volledig ingekleurd voelt logischer". The cover card (0.2.58) draws the blind hanging from the
top, as Home Assistant's cover dialog does: the filled part is the closed part. The tile's small slider (and the strip
of a full-page blind) filled with the open part. Now both fill with the closed part.

## What changed

- `runtime_tiles::slider_value` gives a cover `(100 - position) / 100`; `commit_slider` sends `100 - value` as the
  position, so dragging right closes. The tile's text still shows the position (open %), as Home Assistant does.
- The editor's TileCard fills a cover's mini slider with `100 - current_position`.

## Automated

- `tools/check.sh`: Python, C++ 18/18, packages, icons, Vitest 77/77 (a new case: a blind at 30 % open fills 70 %),
  vue-tsc, build, bundle fresh.
- `tools/check.sh --firmware` (ESPHome 2026.6.2): CYD 1,652,384 B = 90.0 % of the 1,835,008 B slot (+0 B against 0.2.78),
  Guition 2,026,624 B = 24.9 %.

## Not tested

- On real hardware: the direction of a drag on a blind's tile slider and the full-page blind strip on both boards.
