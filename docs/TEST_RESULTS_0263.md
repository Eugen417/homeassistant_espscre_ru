# Test results app 0.2.63 / firmware 0.2.54 (2026-09-17)

People keep a screen beside the bed, and its light look is too bright there at night. Max asked for a Dark mode
switch in the screen settings, controllable from Home Assistant like every other setting: the same design with only
other colours, a dark look that still has contrast here and there, little extra memory, and every colour in one place.
See CHANGELOG 0.2.63, docs/THEME.md and the compatibility note in docs/RELEASING.md.

## How the dark look was chosen

- Every colour in the firmware and both board profiles was collected first: in 0.2.53 more than 600 colour literals
  over eight headers and the two profiles, 112 different values, many of them the same grey or blue written again.
  They became 55 roles with a light and a dark value, nine named card colours and Home Assistant's state colours, all
  in `components/smart_display/theme.h`.
- The light values are the old design; the dark values were chosen on renders of every scene, looked at on the RGB565
  quantisation the panels show: a black page, graphite cards (`#1A1A1A`) with a hairline (`#292929`), soft white
  names (`#DADADA`) and grey secondary words, pastel card colours as deep versions of themselves, Home Assistant's
  state colours unchanged (a lamp's amber, heating's orange, the blue of a chosen key), deep state colours lifted
  just enough to read on graphite, and the pale circles and tracks as dark tints of the state colour.
- `tests/test_theme.cpp` holds the rules down: names and values at 7:1 or more on every card and card colour in both
  looks, secondary words 4.5:1, white on the dark blue key at least as readable as on the light one, surfaces
  stepping up from the page, and every dark grey on one RGB565 step for red, green and blue so it stays grey on the
  panel.

## Automated

- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 16/16 PASS. New `test_theme.cpp` (the light design pinned,
  LVGL's mix, the old hand-picked tints within two steps, contrast in both looks, neutral dark greys, the helpers);
  `test_settings_screen.cpp` checks the Dark mode row and where a switch's knob sits (`knob_x`).
- Python (`.venv-portal`): 311 tests OK. New `tests/test_theme.py` (9): no hex colour outside `theme.h` (the light's
  colour picker and the value card's colour wheel excepted), every role with a light and a dark value, every paint
  defined, filled once and used in both profiles and packages, the look applied at boot, after a change and in the
  redraw, the firmware's own paints, the setting wired through the screen, the app and the editor, and the two redraw
  fixes below. Updated: the palette, alert, off-look, settings entity and Claude skill tests.
- `tools/generate_packages.py --check` and `tools/generate_icons.py --check` (150 pickable icons, 184 glyphs) green.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components, as for 0.2.62, never flashed.

| Profile | RAM | Flash | vs 0.2.62 |
|---|---|---|---|
| CYD | 23.7% (77,500 B) | 77.4% (1,420,983 B) | +440 B RAM, -6,996 B flash |
| Guition | 24.7% (80,852 B) | 18.8% (1,524,639 B) | +416 B RAM, -7,256 B flash |

The static RAM, symbol by symbol on the CYD: 19 paint styles minus 5 removed styles (+172 B), the theme's pointers,
the table of the firmware's own paints and the Dark mode preference (+125 B), the Dark mode switch and its two actions
(+108 B), the alert colour global (+12 B), the light rows' icon pointer (+12 B) and one component more (+12 B). A first
version kept the profile's styles in a table with a `std::function` and cost +720 B; the list of `theme::fill()` calls
is now code in flash. Flash shrinks because the profiles' local colour setters became shared styles.

## Firmware on the Mac (host builds, driven over the ESPHome API)

The history demo home (twenty tiles over five pages on the Guition, seventeen over four on the CYD), every page, every
card (the climate card's mode picker too), both alerts, the busy spinner and every settings page, rendered from the
release candidate and compared pixel for pixel. No warnings or errors in any run's log.

| Comparison | Guition | CYD |
|---|---|---|
| Booted dark against booted light and switched to dark | 35/35 identical | 30/30 identical |
| Opened in light and switched while open (every card, the mode picker, an alert, a settings page, the spinner) against the same opened in dark | 23/23 identical | 19/19 identical |
| Booted dark and switched to light against booted light | 35/35 identical | 30/30 identical |
| Light against firmware 0.2.53 | 10 identical, 26 with the differences below | 10 identical, 21 with the differences below |

The light differences are the merges listed in docs/RELEASING.md (at most 7 steps of one channel, most of them gone
on the RGB565 panel), the Dark mode row and the firmware version on the settings page, and the two fixes below. The
tile showcase (keys, sliders and pastel card colours on both boards) gave the same kinds of light differences, and
its dark pages and cards were reviewed one by one.

Memory, measured in the LVGL tree of the host build with page 1 drawn (every object on every screen and layer, the
bytes malloc handed out for objects, their style lists and their local styles; a 64-bit host, so pointers are twice
the ESP32's):

| | Guition 0.2.53 | Guition 0.2.54 | CYD 0.2.53 | CYD 0.2.54 |
|---|---|---|---|---|
| Objects | 301 | 299 | 282 | 280 |
| Style entries (shared / local) | 964 (603 / 361) | 1038 (688 / 350) | 931 (589 / 342) | 1004 (673 / 331) |
| Properties in local styles | 1972 | 1784 | 1865 | 1679 |
| Bytes | 97,696 | 97,280 (-416) | 92,496 | 92,080 (-416) |

The paints add an entry to each widget that takes one and take away the colour properties it carried itself, and
two objects are gone (the colour card's page and the white sheet over it, now one fill), so the dark-capable firmware
uses a little less heap than 0.2.53. A change of look creates nothing: after 20 switches back and forth the object
count, the style entries and the local properties are exactly what they were, and the Guition host's heap stayed
within a few KB. The host's total heap is not usable for smaller effects: it differs by 1.5 MB between two runs at
the same step.

## Two older bugs the comparisons found

Comparing a screen that switched looks with one that booted in it showed two differences that were there in 0.2.53 as
well. Both are fixed; the light look changes only there.

- **Switch knobs.** `settings_screen::draw()` ends in `refresh()`, and `move_knob()` read the coordinates of the rows
  it had just created, before LVGL laid them out (0 wide), so every switch that was on showed its knob at the left
  until the next refresh (a tap on a setting). A change of look draws the page again, so turning Dark mode on threw
  its own knob to the left. The knob is now placed from the size `pill()` gave the track.
- **The sun's fill.** The palette pass gave the sun path's fill the tile's accent (orange) whenever the palette
  changed, while `render_sunpath` sets the sun's yellow on every render, so the sunlit area changed colour a minute
  later by itself. The sun path keeps its own fill.

## Physical screens (over the air)

Home Assistant 2026.9.2. ESP Screen Manager was updated from the GitHub store to 0.2.63 right after the release, and
both screens with **Update** in ESP Screens, which builds the package from `main`. Max looked at both screens and
switched them himself during the test: it looks good on the glass.

- **CYD 2.8in Display** (ESP32, no PSRAM): 0.2.51 to 0.2.54 in about ten minutes, build included.
  `switch.cyd_2_8in_display_dark_mode` appeared, off. It was switched 32 times: 16 from Home Assistant by the test,
  16 by Max, a second apart. The switch reported every new state within a second, and the screen never restarted
  (`last_boot` unchanged).
- CYD free heap (the debug sensor, every five minutes): 114,952 B shortly after the boot, then 104,696 (dark),
  104,700 (light), 104,692 (dark, after 16 quick switches), unchanged (light, after 10 more), 104,676 (dark) and
  103,088 (light, after Max's own testing). Largest free block 49,152 to 53,248 B, lowest free heap 85,600 B during
  the quick switching. Before the update, 0.2.51 ran at 104,704 B with a largest block of 49,152 B. The step from
  about 115 KB after a boot to 104.7 KB is not Dark mode: 0.2.51 made the same step this morning (116.2 to 104.2 KB
  at 05:30) without it.
- **Guition Wallbox** (ESP32-S3, LVGL in PSRAM): the update ran after 0.2.64 had landed on `main`, so it runs firmware
  0.2.55 (Dark mode plus the alert title fix). Dark mode on and off from Home Assistant: no restart, free internal
  heap 83,644 B (light), 83,536 (dark), 83,212 (light), largest block 40,960 B and lowest free heap 72,472 B unchanged.
  0.2.53 ran at 84,036 B.
- After the test the CYD was light again, as it was; Max turned the Guition dark himself. The add-on stays on 0.2.63
  from the store (0.2.64 is offered).
