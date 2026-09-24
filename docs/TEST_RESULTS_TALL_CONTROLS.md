# Responsive tall controls acceptance

Tested on 24 September 2026. This extends the developer-only rectangular tile
support in [0.3.1](TEST_RESULTS_031.md), using the approved media B and climate A
compositions. The existing single-row renderer is preserved.

## Implementation

- Additional height uses the familiar heading, round keys, sliders and colours.
  Media shows track information and optional dimmed artwork. Climate puts its
  setpoint between round keys, or shows measured temperature with selected modes.
- Size and control selection remain separate. Resizing preserves the existing
  selection; extra height never enables a control automatically. One control
  group is selected at a time. The editor exposes the larger layouts only with
  `SCREEN_EDITOR_ENV=development`.
- Geometry uses the actual available rectangle, measured fonts and physical
  touch limits. Optional secondary text gives way before controls. A layout too
  small for usable controls retains the existing detail action.
- Overflowing media titles use LVGL's circular marquee with an initial pause and
  a repeat pause. State updates do not reset the long-label mode. The editor
  measures actual text overflow and respects reduced-motion preferences.
- The add-on crops, rounds and darkens artwork. A page uses the existing shared
  RGB565 image buffer, bounded by the native canvas, rather than one bitmap per
  tile. The CYD retains the artwork-free composition. Editor image requests stay
  relative to Ingress and expose prepared pixels, not HA credentials or source
  URLs. Missing artwork retains the ordinary tile.
- No storage version, migration or firmware compatibility renderer was added.

## Automated and rendering checks

The complete fast check suite passes: 732 Python tests, 26 C++ programs, 292 editor
tests, TypeScript, generated resources and a freshly built editor bundle.
The geometry test sweeps 21,884 valid arrangements and checks containment,
separation and minimum touch dimensions.

Ten native LVGL board/orientation variants pass 280 page checks, including CYD,
Guition 4-inch, the large JC8012P4A1 and all Waveshare profiles. Native image
transport was also exercised for narrow tall, 2-by-2 and double-width media
cards, including returning from a backdrop to the compact thumbnail layout.

A separate before/after comparison of 39 existing compact cards and overlays is
pixel-identical. It covers media, climate, light, fan, vacuum, cover, history,
number, select, weather, timer and sun. This is a finite regression set, not a
claim about every possible Home Assistant state or grid override.

All six firmware profiles compile with ESPHome 2026.9.0 and the CYD flash-budget
check passes. The CYD image is 1,639,376 bytes with 195,632 bytes left in its OTA slot. The source image, atlas bounds, malformed requests, overlapping
frames, image fallback, HTTP caching, relative editor endpoint and supported
media controls have regression coverage.

## Control capability audit

The chosen group is intersected with actual Home Assistant capabilities. The
preview follows the firmware's supported playback, volume/mute, cover, vacuum,
timer, select and climate keys. Existing selections remain saved if a capability
is lost, but unsupported tall panels are hidden and their events rejected.
On/off-only lights do not get brightness, non-position covers do not get a
position slider, and a thermostat supporting only a temperature range is not
offered a single-target control. Play-only and pause-only players use their
specific supported action rather than an unsupported combined action.

Regression cases include all 64 combinations of the lower media feature bits,
play-only support, cover direction and end stops, vacuum pause/dock subsets,
selects with fewer than two options and unavailable entities. The local test HA
returned capability descriptions for 40 entities across 11 domains. This was a
read-only catalogue check, not 40 real-device action tests.

The extension retains one selected control group and the existing maximum of
three inline climate mode keys. Further modes and richer controls remain in the
detail overlay. It does not automatically place every possible HA control on a
tile. A duplicate ordinary tile on another page no longer prevents fetching the
artwork for an explicitly configured cover tile of the same entity.

## Physical acceptance and limits

The explicitly authorized disposable Guition 4848S040 was updated and its
identity and compilation time read back. With the local Home Assistant test
installation, ten page checks and fifty overlay render cycles passed. The
four-page example covers tall light, media, climate, graph and clock cards.
An actual LVGL snapshot confirms album artwork arriving through the add-on and
rendering behind white text and the playback key.

The first artwork attempt exposed a missing port forward in the local test VM.
Restoring its existing camera route made the image load; the ordinary tile
remained usable while the image was unavailable. This was a lab routing issue,
not a firmware configuration migration.

These were software-driven render and API checks. No physical finger taps or
household actions were performed. Occasional 55–169 ms self-test loop warnings
were observed. This run does not establish long-term uptime, real-finger feel,
or physical acceptance on the other five boards. Unsupported Unicode in a
track title remains constrained by the existing firmware fonts.

The final browser walkthrough verified the translated climate caption, real
media-key subset, album image and single place to choose tall controls. Browser
automation subsequently timed out during the marquee follow-up, so that last
preview and capability polish was covered by component tests and a production
build instead. Native LVGL captures three seconds apart confirm movement of
long titles in both narrow tall and 2-by-2 media cards. The existing firmware
marquee helper is shared with the media overlay and full-page card.
