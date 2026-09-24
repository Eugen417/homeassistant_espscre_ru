# Taller tile design study

This is a host-only LVGL design study, not a change to the device renderer or
editor. The developer gate, saved layout and control selections are unchanged.
The two directions are meant for visual review before implementation.

## Preservation boundary

This is a responsive extension, not a redesign of existing tiles. Existing
single-cell and double-width, single-row tiles keep their renderer, spacing,
fonts, colours and controls. Only a footprint with more than one row is eligible
for the new arrangement. The existing overlays are the visual and data reference.
The study changes no production renderer, so compact tiles are currently untouched.
A later implementation must prove this with identical before/after native renders.

## Directions

- **A, existing language:** the familiar icon circle, bold name and state form the
  heading. Additional height holds track information or temperature. Media uses
  the overlay's round transport keys with one blue primary action. Climate puts
  the target between the overlay's round minus and plus keys, with the measured
  temperature below. A bounded album cover uses the existing picture vocabulary.
- **B, alternative composition:** the same tile heading and control selection,
  with an explicitly selected photographic media background, or a climate card
  showing the room temperature above the familiar setpoint bar. This is a visual
  alternative for review, not an automatic display-mode change on resize.

The earlier experiment with automatic state-tinted card backgrounds and lighter
entity headings is no longer the reference direction. The original title weight,
white card surface and state-coloured icon are retained. Physical button targets
remain at least 7 mm even where the visible glyph is small.

The render set includes playback, volume with mute, HVAC modes, setpoint, light
brightness, vacuum actions, no controls and dark mode. These are separate control
choices, not controls automatically added because a tile is taller. Combining
playback and volume, or setpoint and modes, would require a separate product and
configuration decision. The optional artwork is a local fixture. The host compiles it as RGB and lets LVGL fit it without changing its aspect
ratio. Rounded image clipping here is host-only; the device implementation must
use the existing pre-rounded image pipeline instead of introducing layer buffers. This does not
implement image transport, caching or memory policy on a device. In particular,
a host image succeeding is not evidence that a small board can afford that image.
A device implementation must use the existing image capabilities and budget,
and retain the artwork-free composition on devices without that capability.

## Overlay inventory and data contract

The existing dispatch in `runtime_tiles::show_detail()` is the reference. Native
Guition captures were reviewed for media, climate, light, fan, vacuum, cover,
sensor history, switch history, number, select, weather, timer and sun. These use
synthetic entities and make no Home Assistant calls. The gallery includes them
when `existing-ui/guition` is present beside the generated concepts directory.
History and forecast captures without a supplied series show loading or empty
states, not a populated graph. Rendering them is not a physical interaction test.

| Existing view | Data and visual elements to reuse in taller tiles |
| --- | --- |
| Media | Player state, title, artist and album, optional cover, elapsed time and duration. The existing overlay hides progress for streams without a duration. Supported-feature flags govern transport and volume. Extra height must not select another control group. |
| Climate | Operating mode, measured temperature, target, limits and step. The overlay also knows mode choices and supported fan/swing rows. These are not automatically copied onto a resized tile. Unknown target and off are distinct states. |
| Light and fan | Brightness or percentage, power state, supported colour/effect capabilities and the existing slider vocabulary. More room does not add a second power control beside a selected slider. Colour and effect subviews remain reachable through the established detail flow. |
| Vacuum | State, optional battery and room, supported actions and available mode, suction and water choices. Missing battery or room must leave no fabricated value or empty label. |
| Cover | Movement state, position and supported stop/open/close or tilt capabilities. Reuse the existing purple position control and direction semantics. |
| Sensor, switch and number | Value, unit and precision, optional numeric history or state timeline. Reuse empty/loading/unavailable handling and history colours. A number's configured bounds and step still determine its slider. |
| Select | Current option and actual option list. More space does not manufacture additional options. |
| Weather | Condition, temperature and unit, optional feels-like, humidity, wind and available hourly/daily forecast. Do not assume every integration provides every field. |
| Timer and sun | Timer state and remaining duration, or the available sunrise/sunset times. Preserve the existing actions and formatting. |

The resized tile is not a complete overlay embedded in a cell. Its display mode,
explicit control choice and entity capabilities determine its contents first.
Available space then determines which optional details fit. The complete overlay
remains available through the existing interaction. Extra metadata is a candidate
for the extension, not a claim that every field is already drawn by this study.

## Layout contract

The host reads the real board catalog, density, look, grid, margins, gaps, header
height, footer height and font sizes. The layout functions receive the available
rectangle and fonts, not a board identifier.

The tile first reserves its physical control height, at least `ui::touch_min()`.
The rest is shared by the heading, primary information and optional detail. Font
line heights determine whether extra text fits. An optional cover block requires both width
and height. The photographic background uses a fixed dark scrim for text contrast.
The centred setpoint chooses its font using measured text width as well as height. Optional details give way before controls do. A narrow grid does not
invent a wider footprint: the one-column portrait profile renders 1 by 2.

Neighbouring cells in the concepts are schematic footprint markers, not replacement
designs for compact tiles. Their existing dimensions remain fixed. No free scrolling,
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

`TALL_RENDER_OUT` can direct a separate fixture run to a different output folder.

Run `tools/render/tall_concepts.py` using an ESPHome Python environment with Pillow
and SDL2. `ESPHOME` can select the CLI. Output is under
`.esphome/tall-design-review/concepts/out/`, including an HTML gallery and native
PNG snapshots. `tools/render/tall_concepts.h` is never included by production
firmware. Set `TALL_MEDIA_FIXTURE` to a local JSON file to review realistic media. It accepts
`title`, `artist` and an absolute `artwork` file path. Keep personal fixtures and
images outside Git. The generator does not fetch anything or call Home Assistant.
With no fixture it remains reproducible using generic text and LVGL artwork.

The gallery can show optional baseline snapshots from the adjacent
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
