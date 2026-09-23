# Page-owned layouts, 0.3.0 acceptance

Tested on 23 and 24 September 2026. Environment: an isolated Home Assistant OS VM with demo integrations, an explicitly selected disposable Guition 4848S040, and native host builds of the board catalog. Production screens and household device actions were excluded. The comparison starts at `803bb39`, add-on 0.2.132 and firmware 0.2.104.

## Automated contracts

`tools/check.sh` passes 684 Python tests, 25 C++ programs and 196 editor tests, plus package, grid, board, icon, translation, type and generated-bundle checks.

The page-specific tests cover stable references after reorder/copy/delete, Home on another page, pagination exclusions, bounded Back history, rectangular placement validation, grid adaptation, independently copied bars and empty layouts. Storage tests cover atomic replacement, a durable original backup, pending migrations, unknown versions, revision conflicts and interrupted writes. Delivery tests cover stale sessions, stale state, incomplete configurations, lost acknowledgments and automatic retry.

An editor test advances the clock by one week while one of two screens is updated. The affected screen becomes editable without another Save, its unsaved draft and workspace remain intact, and the other screen keeps its own update notice.

## Home Assistant and editor

The local Supervisor upgraded the installed add-on from 0.2.132 to 0.3.0. Its existing layout migrated once, and restarting the add-on preserved the migrated IDs and original backup. The editor and inventory were accessed through actual Home Assistant Ingress.

The new add-on continued delivering a representable layout to the older physical firmware. Updating that screen activated the saved page document automatically, without pressing Save. The reverse order produces the explicit update-add-on message rather than invoking a legacy firmware decoder.

Browser checks exercised Simple and Advanced, the entity library, colored tiles, an inline brightness slider, real recorder history, Home reassignment, page reorder, navigation links, pagination opt-out, Undo, map arrangement and the local navigation preview. The browser was also checked at a 390 by 844 viewport. Simple pages, including a page marked as a deeplink, have equal preview widths. The dark navigation dialog and its Back route were inspected visually.

Whole-chain controls used only HA demo entities. Two virtual taps toggled a demo light on and off; dragging the actual LVGL slider changed a demo light's brightness from 180 to 40. The original brightness was restored. A real demo-camera alert passed through HA, the manager and protocol 2, loaded its picture with valid geometry, and was dismissed through the virtual touchscreen.

## Physical memory measurements

Both matched measurements started after a cold boot, used the native API with the HA integration connected, and ran for 1,220 seconds. The manager was stopped during measurement. Each layout contained 48 graph tiles across eight pages. The old firmware used six global top-bar items; the new firmware used six items on every page.

| Final stable sample | Firmware 0.2.104 | Firmware 0.3.0 |
| --- | ---: | ---: |
| Free internal heap | 89,704 B | 89,740 B |
| Largest internal block | 47,104 B | 45,056 B |
| Minimum free internal heap | 81,456 B | 80,740 B |
| Free PSRAM | 5,719,260 B | 5,716,212 B |
| Largest PSRAM block | 5,636,096 B | 5,636,096 B |

Small differences in free internal memory are allocator and network variation, not evidence of a saving. The extra per-page metadata and bars cost about 3 KB of PSRAM in this workload. The firmware stores one configuration and no legacy layout decoder. The subsequent navigation paint correction adds no stored state or allocation.

A further 620-second run used maximum-length screen/page titles, tile names and top-bar values. Free internal heap stabilized at 83,540 B, with a 39,936 B largest block and 65,676 B minimum. PSRAM remained stable at 5,715,324 B.

Thirty cycles alternated an empty document with eight full graph pages, 60 configurations in 479.85 seconds. Each full configuration ran the board's page/overlay geometry self-test. Observed total free heap stayed between 5,795,232 and 5,798,884 B, without progressive loss. These are bounded workload measurements, not an indefinite soak or every possible combination of camera and media buffers.

## Board builds and host rendering

All six boards compile with the add-on's ESPHome 2026.9.0. All ten host variants pass, including portrait variants, 200 page checks, real touchscreen-path navigation, nested Back, Home fallback, hidden-footer Back, fixed tile height and recovery after changing settings under a protocol-error overlay. Host fingers move continuously between sensor samples; the older stepped motion could reset LVGL's velocity-based gesture accumulator on a portrait CYD.

| Board | ESPHome 2026.9.0 firmware size |
| --- | ---: |
| CYD | 1,631,120 B |
| Guition | 2,048,576 B |
| Waveshare 4.3 | 2,305,024 B |
| JC8012P4A1 | 2,021,360 B |
| Waveshare 7 | 1,834,112 B |
| Waveshare 4B | 2,034,144 B |

The CYD uses 88.9% of its OTA slot, leaving 203,888 B. Its growth against the baseline built with the same compiler is 2,656 B. With minimum ESPHome 2026.6.2 the CYD uses 1,656,464 B, 90.3%, leaving 178,544 B. That is the documented tight-budget warning, with growth of 2,608 B against the matching minimum-compiler baseline of 1,653,856 B. The P4 profile requires ESPHome 2026.8.0 and is not a 2026.6.2 target.

## Scope of physical evidence

The physical board was flashed and its compilation identity read back. Its protocol checks cover all seven lost-acknowledgment boundaries, a non-first Home, excluded pages, active-page preservation after reorder, Home fallback after deletion, all pages excluded, obsolete sessions, exact retries and refused configuration fields in state updates.

On that final image, an incompatible message produced a full opaque update-add-on screen. Toggling the footer setting while the error was visible and then resending the same document restored a pixel-identical LVGL image. This caught and fixed a bug where navigation paint inherited the temporary input block. The corresponding forced-error case remains in the host renderer tests. A mixed workload of 18 direct-control cards also passed the physical board's ten page/overlay checks after the final flash.

Before the final Back change, a physical user check confirmed that excluded pages disappeared from the paginator and Home returned to page 2. The final bottom Back and hidden-footer chevron are exercised through the real firmware's host touchscreen path. Those virtual touches do not measure the physical GT911 sensor, finger feel, panel color or real-glass timing. Physical acceptance on other board models was not performed in this run.
