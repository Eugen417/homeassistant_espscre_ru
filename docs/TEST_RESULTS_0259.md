# Test results app 0.2.59 / firmware 0.2.51 (2026-09-16)

Max found the history card unusable ("no y axis, no real x axis") and chose option A of two designs: a line with
axes for numbers, with the touch readout of option B (a finger on the graph shows the value and time of that
moment). History for on/off, people and numbers too, fetched when the card opens, for 1 hour, 24 hours or a week
with the same number of points. During the work he asked for three changes, all in this release: the graph stays
as it is under a finger (only the words at the top change), the rings of the highest and lowest moment sit on the
line's peak and valley, and a sensor that only exists for two hours shows no data before that. See CHANGELOG
0.2.59 and the compatibility note in docs/RELEASING.md.

## Automated

- Python (`.venv-portal`): 282 tests OK. New `tests/test_history_card.py` (28): which entities get a line or a
  timeline; time-weighted averages with gaps; statistics rows (milliseconds, gaps, min and max); the highest and
  lowest moment; axes in round steps for °C, °F, %, W, kWh, negative values, lux, m³/h, ppm, V and a flat line;
  decimals from the display precision; time ticks every quarter hour, six hours or midnight, across a daylight
  saving change; a door's timeline with Home Assistant's words, runs with the real times of their state (an
  opening of 5 minutes in a 15-minute slot reads 01:00-01:05, 300 s), people with zones and their own colours,
  statuses beyond six states joining "Other"; a sensor two hours old (averages only for its own hours, also from
  statistics with one compiled hour, and a sensor with one state) and a binary sensor two hours old (no data before
  it); a motion sensor that changes every 30 s for a week (20,160 changes: 13 ms, 354 bytes); the largest possible
  timeline (96 runs, 32-character states) at 3.6 KB of the 4096 a screen takes. The manager: a number and a door
  each get one message through the screen's action, a day from hourly statistics and an hour from the changes;
  screens asking at once share one fetch and a reopened card uses the cache; a failed fetch sends and keeps nothing
  and the next question is answered; entities that are not on the screen, other inboxes, other ranges, a light and
  an offline screen get nothing; the event loop survives a failure. Firmware source checks: the event, the answer
  it waits for, the entities with a history card, range keys kept out of the disabled keys, the press lock, and
  `small_font` in both profiles and packages.
- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I. -I components/smart_display`): 15/15 PASS. New
  `test_history_view`: clock times in 24 and 12 hours with weekdays and negative offsets, axis times ("4 PM",
  "4:15 PM"), durations, numbers with thousands and glued units, the part and the run under a finger, the monotone
  line (a setpoint stepping 55-45-60 stays between 45 and 60, flat stays flat, every point on the line, a small
  buffer is not overrun) and `through` (a moment takes its part's point, a second moment in the same part joins
  beside it, a full buffer refuses).
- `generate_packages.py --check` green.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components, the same as for 0.2.58.

| Profile | RAM | Flash | vs 0.2.58 |
|---|---|---|---|
| CYD | 23.5% (77,060 B) | 77.8% (1,427,015 B) | +512 B RAM, +15,192 B flash |
| Guition | 24.5% (80,436 B) | 18.8% (1,530,331 B) | +512 B RAM, +15,068 B flash |

No warnings from `runtime_tiles.h` or `history_view.h` (the switch's part/state selector, moved from the old switch
card, now casts to `lv_style_selector_t`).

## Firmware on the Mac (host builds, driven over the ESPHome API)

A demo home with a week of history (door, coffee machine, a person with zones, a washing machine status, a boiler
setpoint, and a plug added two hours ago) answers with the real `Manager.card_history`; a virtual LVGL pointer
drives the card. Guition 40/40, CYD 40/40:

- Opening a card fires one `esphome.screen_history` event (inbox, entity, 24); the card shows the value now and
  "Loading history..." until the answer, then the line with High and Low and their times and the axis labels the
  message carries.
- A finger reads the average of the hour under it with its time range and "average of 1 h" in the accent colour;
  dragging moves the readout, past the edge it holds the last hour, lifting restores the header. The objects of the
  graph (count, positions, visibility) are identical before and under the finger, on the line and the timeline.
- "1 week" asks for 168 hours and waits with the week selected; a late answer for the day is ignored; the week's
  High and Low name the weekday and a week part reads as a weekday time range averaged over 7 h. "1 hour" asks for
  1 hour and a part reads "average"; tapping the range shown asks nothing; a new state updates the header without a
  new question; reopening starts on 24 hours and asks again.
- The door: the heading follows the state ("Off" before the answer, "Closed" with it, "Open" when the door opens
  while the card is open), "Open · 22 min" and "6 times"; a finger on the opening of 12:30 reads "12:30 – 12:35"
  and "5 min", the unavailable stretch "14:00 – 14:20" and "20 min".
- A person with zones (home, "once"), the week; a washing machine status without counts; the coffee machine with its
  toggle, which sends `switch.turn_off`; the boiler setpoint with its slider.
- The plug added two hours ago: its line only over its last three parts, High 1,850 W and Low; a finger before it
  existed reads "No data" with the time, on its last hour its average.
- Without an answer "No history available" after 8 s, and a late answer is still drawn; an empty hour says
  "No history in this period"; malformed history messages change nothing; no errors in the log.
- Full redraws on the host: the line card about 70 ms like the cover, weather and vacuum cards (the fill about
  6 ms, the line about 12 ms). The host's LVGL loop stalls for 1-2 s at random, on any card; the checks wait for
  the state they expect instead of fixed pauses.
- Renders of every card on both boards; the README shows the new history cards. All README screens were rendered
  again from the standard demo home: the pages and the alert are unchanged, the light card and both weather cards
  changed since 0.2.41 and were replaced. The Guition weather card's round back button lay half under the card
  (since 0.2.43); it now starts below the button, checked in a new render. On the CYD moving it squeezed the days
  too much, so the CYD keeps its layout. A playing media player's volume slider is grey on the tile since 0.2.43
  (Home Assistant colours it); left for a separate change.

## Not tested / for the owner

- A live read-only check against Home Assistant could not run: both local tokens were refused. The card uses the
  same two calls the tile graphs already use (`recorder/statistics_during_period`, now with min and max, and
  `/api/history/period`).
- Real hardware: on Studio 1 after the update, open a temperature, a door or a person, switch between 1 hour,
  24 hours and 1 week, and slide a finger over the graph. Free heap on a CYD with a week's timeline open.
