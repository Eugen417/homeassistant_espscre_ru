# Test results 0.2.93 (firmware 0.2.79)

Hotfix for a CYD that restarted when the big slider of a card was dragged. Found and verified on Max's bench CYD
(cyd-2432s028, 192.168.146.134) on 2026-09-21, without USB: the log came over the native API.

## What went wrong

- Log at 09:30:07, right after "toggle a light on its tile, hold the tile at once, drag the slider":
  `lv_draw_buf_create_ex: No memory: 96x160, cf: 16, stride: 384, 61440Byte` followed by
  `lv_draw_layer_alloc_buf: Allocating layer buffer failed. Try later`, repeated every ~25 ms until the task watchdog
  reset the board (`Reason: Task wdt`, backtrace through `lv_refr` → `lv_draw_dispatch` → `lv_draw_layer_alloc_buf`,
  decoded with `xtensa-esp32-elf-addr2line` against the build's `firmware.elf`).
- 96 x 160 x 4 bytes is the overlay slider (OVERLAY_SLIDER_W x OVERLAY_SLIDER_H on the CYD) as an ARGB8888 layer.
  `lv_bar.c` draws a fill into a layer of its own when the fill's radius is smaller than the track's (`radius_issue`).
  The `open_overlay` script set the track to 28 and the fill to 0 on every open, overriding the widget's own
  `radius: ${OVERLAY_SLIDER_RADIUS}` on both parts.
- The CYD's largest free block was 63-65 KB on that build and 48-60 KB on firmware 0.2.51-0.2.62 (Home Assistant's
  `heap_largest_block` history over four days), so the layer only fitted when the heap happened to have a hole of
  61 440 bytes; a light just switched or an answer from Home Assistant took that hole away.

## What changed

- `packages/core.yaml`, script `open_overlay`: the two `lv_obj_set_style_radius(id(overlay_slider), ...)` lines are
  gone; the track and the fill keep the radius the widget was given (28 on a CYD, 42 on a Guition). The Guition's
  track no longer drops to 28 after the first card.
- `tests/test_layer_free.py`: `test_yaml_lambdas_keep_the_track_radius` reads the lambdas in the YAML profiles as
  the C++ test reads the headers. It fails on 0.2.92 (`overlay_slider: fill radius 0 differs from its track`) and
  passes here.

## Checks

- `python -m unittest discover -s tests`: 520 tests OK (ESPHome 2026.9.0 venv).
- CYD compile from this tree with the Device Builder profile (api/ota/wifi, no Easy extras): successful,
  1 537 527 bytes; not comparable to the Easy build the CHANGELOG sizes come from, and a lambda with two style
  calls fewer does not change the size in any way that matters.
- Not flashed from this tree: the bench CYD runs the responsive-lab build, which carries the same two-line removal
  (that round becomes 0.2.94 / firmware 0.2.80). Max's crash-on-drag reproduction is the check for that flash.
