# Taller tile design study

This is a host-only LVGL design study, not a change to the device renderer or
editor. The developer gate, saved layout and control selections are unchanged.
The two directions are meant for visual review before implementation.

## Directions

- **A, calm stack:** a compact heading, information in the middle and the selected
  control group along the bottom. Climate puts the current temperature first and
  keeps the target between the minus and plus controls.
- **B, visual focus:** media uses a cover area where physical space permits it.
  Climate emphasizes the target, with minus and plus below and the current
  temperature as secondary information when it fits. Compact cards fall back to
  the simpler layout rather than shrinking touch targets.

The render set includes playback, volume with mute, HVAC modes, setpoint, light
brightness, vacuum actions, no controls and dark mode. These are separate control
choices, not controls automatically added because a tile is taller. Combining
playback and volume, or setpoint and modes, would require a separate product and
configuration decision. Cover artwork is a deliberately illustrative LVGL drawing;
this study does not implement downloading, caching or decoding a larger image.

## Layout contract

The host reads the real board catalog, density, look, grid, margins, gaps, header
height, footer height and font sizes. The layout functions receive the available
rectangle and fonts, not a board identifier.

The tile first reserves its physical control height, at least `ui::touch_min()`.
The rest is shared by the heading, primary information and optional detail. Font
line heights determine whether extra text fits. Album artwork requires both width
and height. Optional details give way before controls do. A narrow grid does not
invent a wider footprint: the one-column portrait profile renders 1 by 2.

Ordinary neighbouring cells keep their existing dimensions. No free scrolling,
extra tile capacity, storage migration or additional firmware compatibility model
is involved. A future implementation should extract the arithmetic into a pure
geometry module and use it for placement, hit testing and the editor preview.
The host drawing itself has no event handlers and is not touch acceptance.

## Existing behavior observed

Fresh renders from the current firmware show that 2 by 2 media and climate still
mainly use the original horizontal row. The 1 by 2 media card uses the stacked
icon/name/status fallback and does not show its selected playback group. The
current runtime gates panels on width. Supporting the proposed arrangement will
therefore also require changing control eligibility to use available physical
space, while preserving the user's explicit choice and entity capabilities.

The baseline captures cover CYD, Guition and Waveshare 4.3. The focused CYD run
also reports existing calendar geometry self-test failures during its card-cycle
checks. Its snapshots were produced, but that baseline run must not be described
as fully passing. A Guition full-suite camera diagnostic used an obsolete message;
the focused render run excludes unrelated camera diagnostics. Neither finding is
fixed or hidden by these design proposals.

## Reproduction and checks

Run `tools/render/tall_concepts.py` using an ESPHome Python environment with Pillow
and SDL2. `ESPHOME` can select the CLI. Output is under
`.esphome/tall-design-review/concepts/out/`, including an HTML gallery and native
PNG snapshots. `tools/render/tall_concepts.h` is never included by production
firmware. The gallery can show optional baseline snapshots from the adjacent
`baseline-focused` folder when those have been captured separately.

The proposals cover six configurations: CYD 2.8, Guition 4, Waveshare 4.3,
Waveshare 7, Guition P4 10.1 and Waveshare 4.3 portrait. The two 800 by 480 screens
have different physical densities and grids, and deliberately do not produce the
same tile layout. There are 120 actual LVGL snapshots. Each traverses the complete
widget tree and verifies that children remain inside their parent bounds.
Control keys assert the shared minimum touch dimensions. This is evidence about
these rendered configurations, not proof for every future board or translation.
Long labels, unavailable states, live entity changes, image memory, physical touch
and integration with the existing runtime remain implementation acceptance work.
