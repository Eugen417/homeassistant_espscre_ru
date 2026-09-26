# Rectangular tiles, 0.3.1 acceptance

Tested on 24 September 2026, on top of the page-owned layout refactor. The scope is two additional footprints, 1 by 2 and 2 by 2 cells, with existing card designs. The grid, number of cells and page capacity stay fixed. Full-page presentation remains a separate choice.

## Contracts and editor

`tools/check.sh` passes 697 Python tests, 25 C++ programs, the editor tests and type checks, and the generated bundle, package, board, icon and translation checks.

Automated coverage checks rectangular occupancy, overlap refusal, page boundaries, one-column boards, packing order, stable document round trips, grid adaptation, resizing beside occupied cells, and capability negotiation. A 2 by 2 card retains wide-card controls; a 1 by 2 card retains single-card behavior. Forecast and sun-path views still require two columns.

The real Home Assistant OS test add-on was upgraded to 0.3.1 with its existing layout intact. With the physical screen still on 0.3.0, it remained synchronized and the new sizes were absent from the editor. Updating the screen exposed both sizes automatically in the already-open inspector. The sender refuses unsupported sizes before starting a configuration transaction.

Both sizes were selected and saved through the browser and actual Home Assistant Ingress. The board acknowledged the saved revision. A tall tile was dragged into a free two-row rectangle and Undo restored it without moving its neighbor. A drop that would cross the bottom of a page left the layout unchanged.

No storage version change or migration was added. Width and height use the existing page-document placement. The firmware advertises its supported presentations and renders one configuration, without a second compatibility layout.

## Physical board

An explicitly selected disposable Guition 4848S040 was flashed over USB and its compilation identity read back. A four-page example uses demo Home Assistant entities: a tall light with a brightness slider, square media and climate cards, and tall graph and analog-clock cards. Snapshots of the real LVGL render were inspected. Ordinary neighboring tiles retain their size.

The board's self-test passed ten page geometry checks and fifty overlay render cycles with the seven-tile example. This checks rendering on the physical board, not finger feel or the GT911 sensor. No physical taps were performed for this extension and no household device actions were used.

## Responsiveness regression

The native host tests add tall light, graph and clock cards plus square media, climate and clock cards, with ordinary neighbors in uncovered cells. They check actual LVGL placement, dimensions and overlap. One-column profiles exercise only the tall footprint.

All ten host variants passed across the completed runs, totaling 380 page checks. Initial runs had intermittent missing simulated swipes on three variants; separate reruns passed the navigation assertions and rectangle checks. This remains a limitation of timing-sensitive host input tests, not evidence of physical touchscreen acceptance.

This exposed a clock edge case on a large portrait layout: the dial left no room for the adjacent digital labels. The renderer now applies its existing no-room behavior to wide multi-row clocks too, hiding the adjacent text and centering the dial. The fixture remains a regression check.

## Firmware builds

All six boards compile with ESPHome 2026.9.0 after the clock correction.

| Board | Firmware size |
| --- | ---: |
| CYD | 1,632,544 B |
| Guition | 2,050,000 B |
| Waveshare 4.3 | 2,306,432 B |
| JC8012P4A1 | 2,022,736 B |
| Waveshare 7 | 1,835,440 B |
| Waveshare 4B | 2,035,504 B |

The CYD retains 202,464 bytes in its OTA slot. Growth against the matching 0.3.0 build is 1,424 bytes. These are flash measurements; the long memory workloads documented for 0.3.0 were not repeated for this extension.

## Follow-up: Home-sized Back chevron

The hidden-footer Back control now uses a dedicated single-glyph MDI font, sized at build time to match Home's visible outline height. Both controls reserve Home's width and touch target, so the title stays fixed. This needs no runtime image-scaling buffer.

After this correction, the full fast checks and all six firmware builds passed again. CYD and Guition host runs passed 80 page checks, including hidden-footer Back with Home disabled. The host probes verify that Home and Back ink heights differ by no more than one raster pixel. A newly flashed physical Guition rendered both controls on the same page, and the title region was pixel-identical. No physical finger test was performed for this correction. The final CYD image is 1,632,800 bytes, 256 bytes larger than the rectangular-tile build above.

## Responsive tall controls

Tested on 24 September 2026. The controls a taller tile draws, built on the rectangular tiles above. The existing
single-row renderer is preserved.

### Implementation

- Additional height uses the familiar heading, round keys, sliders and colours.
  Media shows track information and optional dimmed artwork. Climate puts its
  setpoint between round keys, or shows measured temperature with selected modes.
- Size and control selection remain separate. Resizing preserves the existing
  selection; extra height never enables a control automatically. One control
  primary group is selected at a time; covers can also opt into slat tilt, and climate into its mode keys (added on
  25 September). Both sizes are offered in every editor since 25 September.
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

### Automated and rendering checks

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

### Control capability audit

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

The extension retains one primary control group, optional cover slat tilt, and the existing maximum of
three inline climate mode keys. Further modes and richer controls remain in the
detail overlay. It does not automatically place every possible HA control on a
tile. A duplicate ordinary tile on another page no longer prevents fetching the
artwork for an explicitly configured cover tile of the same entity.

### Physical acceptance and limits

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

### Background artwork and marquee start

Tall media titles now remain static while their background atlas is pending or
loading. Once the image is placed, the existing LVGL marquee starts with its
normal reading pause. Empty image responses and failed download attempts allow
fallback text to scroll. A subsequent download pauses it before image I/O starts.
Compact cards, full-page media cards and media overlays keep their existing
behavior. This reuses the image feed state and marquee helper without adding a
per-tile timer, animation implementation or image buffer.

The image-feed regression test covers initial requests, successful downloads,
refreshes, failed attempts, retries, new tracks and empty responses. Native
Guition LVGL tests also delayed the image response and separately returned no
image: each run checked four waiting and four resumed title states across tall
and 2-by-2 cards in both themes, rendered ten views and passed ten page checks.
These checks verify animation state transitions, not perceived smoothness on
physical hardware. The subsequent physical update below includes this follow-up.

### Tall switches and built-in action tiles

Tall on-tile switches keep a horizontal track with visible knob travel and the
required touch height. The heading shows the state once; the duplicate status
in the body is removed. Tall Settings and page-link tiles centre their icon,
name and optional subtitle as one measured stack. Compact single-row tiles keep
their existing layout. The editor preview follows these changes.

The focused native matrix covers on/off switch states and action tiles across
ten board/orientation variants: 452 renders each passed a geometry check before
capture, plus 100 final-batch page checks. The geometry check measures action
centring, text/icon separation and card bounds. The ordinary regression suite
passes, including the rebuilt editor bundle. The larger review gallery was
updated for the affected variants; its previously documented full-page vacuum
clipping issue is separate from this refinement.

The disposable Guition 4848S040 was flashed after checking its USB identity and
reading its existing API identity. The running firmware reports a compilation
time of 24 September 2026 at 16:59:55 +0200. Ten page checks and fifty overlay
render cycles passed on the physical board. This build also includes the
background-aware marquee change. No physical finger test was performed.

All six final firmware builds pass. The CYD image uses 1,640,384 bytes of its
1,835,008-byte update slot, with 194,624 bytes free. These short render/API checks
are not a long-term soak test.


### Optional cover slat controls

The editor now offers a separate slat-tilt checkbox for taller and full-page
cover tiles. Resizing does not select it. The primary choice remains independent:
none, open/stop/close, or position. Home Assistant must offer a tilt action before
an unselected checkbox is offered. Saved selections survive resizing and lost
capabilities, while unavailable controls stay inert.

The existing controls field stores `tilt`, `buttons_tilt`, or `position_tilt`.
No storage version, migration, duplicate renderer or image buffer was added.
Legacy firmware delivery rejects these selections with the existing update
notice. The minimum firmware is 0.3.1, the pending rectangular-tile release.

The firmware reuses the overlay's slat slider, feature-filtered tilt keys and
percentage action mapping. Layout measures the available body, fonts and
physical touch size. Keys may form a row when a vertical stack does not fit.
If the additional group cannot fit, the primary group stays on the tile and
slat controls remain in the detail view. Single-row designs are preserved.
Only visible controls allocate LVGL objects; the cover extension does not
allocate the custom-card graph-point buffer.

Validation after this addition:

- All 14 fast check groups pass: 734 Python tests, 26 C++ programs, 294 editor
  tests, types, generated resources and the committed editor bundle.
- Capability checks cover all 256 cover feature masks. The shared percentage
  helper checks clamping, opposite position/tilt directions and unavailable
  entities. The editor checks independent selection and shrinking a tile.
- 576 native LVGL captures across ten board/orientation variants, in light and
  dark themes, pass geometry checks. These include ordinary sizes, taller and
  full-page cards, position sliders, partial tilt-key support and covers that
  have lost tilt support. The runs also pass 100 page checks.
- Eight additional native event-path checks capture the exact outgoing actions
  for position, tilt and all three tilt keys. A cancelled drag, lost capability
  and a pending action each suppress dispatch. No calls are forwarded to HA.
- All six firmware builds pass. CYD uses 1,643,424 bytes of its 1,835,008-byte OTA
  slot, with 191,584 bytes free. Growth from the preceding switch/action polish
  is 3,040 bytes. This is a flash measurement, not a measured heap delta.
- The disposable Guition runs the build compiled on 24 September 2026 at
  17:37:35 +0200. Ten page checks and fifty overlay cycles pass. Three temporary
  configurations were saved through real Ingress, acknowledged by the board and
  captured: position plus tilt, no controls, and buttons plus tilt. The original
  document was restored unchanged and the board returned to its home page.

Physical captures verify the running device's renderer and configuration path.
They do not constitute a real-finger touch test. No household cover was moved.

## Hardware, 25 and 26 September 2026

Three bench screens on one Home Assistant: a 4-inch Guition, a CYD and a 4.3-inch Waveshare. The installed 0.2.133 app and
firmware 0.2.104 were the starting point.

- **Upgrade.** A copy of the existing app data was migrated offline first: every tile, page, title, top-bar item and
  setting came through, the backup was byte-identical to the original and a second start changed nothing. The app was then
  updated in place from 0.2.133 to 0.3.1 while the screens still ran 0.2.104: both kept their tiles, top bar and page
  swipes (checked on the glass). After updating the screens over the air, each applied its saved layout within seconds,
  without another save.
- **Stack.** The first 2 × 2 save restarted the Guition: the firmware's crash report named a stack overflow of the main
  task. Measured over the API, a layout replacement needs 8.1 to 8.3 KB on every board, past ESPHome's 8 KB default.
  With 16 KB the three screens kept about 8 KB spare through every test below.
- **Stress.** Every screen got an eight-page showcase (seven on the 3 × 3 Waveshare) with 1 × 2 and 2 × 2 tiles, climate
  with its modes, media with covers, live camera tiles, graphs and a detail page. Page tours at one page a second while
  the whole layout was replaced six times, camera and image tiles at 2 × 2 and 1 × 2 loading while their layout
  changed, and rapid saves: all applied within 2.8 to 6.2 s, no restart.
- **Soak.** One hour sampled every minute, then about six more hours: no disconnect or restart apart from the updates
  themselves, and free memory stayed level (Guition about 80 KB, Waveshare about 80 KB, CYD 110 to 160 KB).
- **Panel.** A full camera view on the Guition showed an occasional shifted frame while its picture loaded. With the
  shared RGB panel settings (64-byte data cache lines, the panel interrupt in internal RAM) it no longer did.
- **Renders.** The full host gallery (10 board variants, 4,450 images) passed every page check after the icon, stack and
  panel changes.

