# Test results app 0.2.72 / firmware 0.2.61 (2026-09-18)

Max: "als ik snel naar pagina 4 wil, kan ik niet snel doorklikken, dan moet je steeds wachten totdat de pagina geladen
is". Next, Next, Next on the page bar took three waits of about half a second each. This release lets the page buttons
keep up with the finger. See CHANGELOG 0.2.72 and the compatibility note in docs/RELEASING.md. Built on 0.2.71
(`2cb3479`).

## What happened before

- The `< Previous` and `Next >` bars ran their tap through `cyd::touch_guard.accept()`, the guard of every tile: it
  drops a second tap on the same tile within 600 ms as a bounce, so one contact can never switch a light twice. On
  the page bar that window dropped the second and third tap of Next, Next, Next. The page itself was never the
  problem: since 0.2.42 a page switch places the new page in the next frame and fills its cards two per refresh, and
  a new switch drops a fill still under way.
- The -/+ keys of the setpoint pill had the same complaint in 0.2.25 and got `accept_repeat()`: every clean tap
  counts, only a contact within 150 ms of the last one on the same key is a bounce. The page buttons now use it.

## Automated

- C++ (`clang++ -std=c++17 -Wall -Wextra -Werror -I.`): 17/17 PASS in a clean export of the committed tree plus this
  round's changes. `tests/test_cyd_ui.cpp` now holds the page buttons: three Next taps 250 ms apart all count, the
  old `accept()` still refuses within its window, and Previous right after Next counts.
- Python (`.venv-portal`): 386 tests OK, the same set as 0.2.71 (firmware only; the app raises `FIRMWARE_VERSION`).
- `generate_packages.py --check` green.
- The working tree also carried another session's uncommitted `runtime_model.h` / `runtime_tiles.h` work (the
  48-tile round, 0.2.73); every build and test below ran from a clean export with only this round's changes.

## Firmware on the Mac (host builds, driven over the ESPHome API)

`pager_host.py` (scratchpad, after `wake_sleep_host.py`): the living room layout of the demo home on 8099 (17 tiles,
four pages), a `pager_taps` action that taps the page bar from an LVGL timer (`count` taps `gap_ms` apart, each a
90 ms contact ending in `LV_EVENT_SHORT_CLICKED` on the button, as the touch layer produces it), an `old_rule` mode
that taps the way firmware 0.2.60 did, and a probe that logs the page, the page number label and the fill state.
**Guition 12/12, CYD 12/12** (the first run judged the bounce and old-rule bursts by the spacing asked for, and
the host's LVGL loop stalls about a second on the first frame of a burst, macOS throttling the SDL window, so
two taps meant to be 80 ms apart landed 1040 ms apart; the checks now judge every tap against the spacing that
really happened, and the second run passed on both boards).

| Check | Guition | CYD |
|---|---|---|
| The living room layout shows four pages, page 1 drawn | pass | pass |
| Next, Next, Next 200 ms apart lands on page 4 (4 / 4) | pass | pass |
| Every one of those taps moved a page: 2, 3, 4, none refused | pass | pass |
| Page 4 finishes drawing afterwards | pass | pass |
| Two more Next taps on the last page change nothing | pass | pass |
| Previous, Previous, Previous 200 ms apart lands on page 1 | pass | pass |
| Three Next taps 160 ms apart (six a second) land on page 4 | pass | pass |
| Three Previous taps 160 ms apart land on page 1 | pass | pass |
| Bounce: a Next within 150 ms of the last accepted tap is refused, a slower one counts and moves a page | pass | pass |
| The old rule (0.2.60): a Next within 600 ms of the last accepted tap was refused | pass | pass |
| Back on page 1 afterwards | pass | pass |
| No errors in the log | pass | pass |

The bounce burst on the Guition landed at 98, 94 and 81 ms and was accepted, refused, accepted, refused, as the
rule says (the third tap came 192 ms after the first accepted one); the CYD's at 84, 84 and 96 ms the same way. The
old-rule burst on the Guition landed at 208, 202 and 203 ms and was accepted, refused, refused, accepted: that is
what Max felt, one page per 600 ms, however fast he tapped.

## Builds (ESPHome 2026.6.2)

Check profiles with the generated packages and local components; no errors or compiler warnings.

| Profile | RAM | Flash | vs 0.2.71 |
|---|---|---|---|
| Guition | 25.7% (84,372 B) | 21.0% (1,708,943 B) | same RAM, -24 B flash |
| CYD | 24.0% (78,796 B) | 79.2% (1,453,647 B) | same RAM, -36 B flash |

The check builds carry a device name of their own (`check-<board>-f9`) and placeholder Wi-Fi, API and OTA values, so
the sizes differ from the bench profiles by the length of those strings; the code is the same.

## Not tested

- Real hardware: neither bench screen ran this firmware before the release. Max's own screens get it through Update;
  the tap that started this is the one to try there, on the Guition and on the CYD.
- A resistive panel's lift-off bounce on the CYD is covered by the 40 ms minimum contact and the 150 ms gap, the same
  net the -/+ keys have had since 0.2.25 (physically tested then), not by a new measurement.
