# Test results app 0.2.78 / firmware 0.2.65 (2026-09-18)

Max: "ga eens je ultra best doen om de commits van vandaag lokaal 100% te testen", then "gedegen code analyse en check op
internet of dit best practise is", and after the report "can you address them?". This release fixes what that test round
of 0.2.72 to 0.2.76 found, with Max's choices for the product questions: a swipe is never a tap, the waking push also
switches a full-page switch, and the same page tile may sit on several pages. See CHANGELOG 0.2.78 and the compatibility
note in docs/RELEASING.md.

## The test round of 0.2.72 to 0.2.76 (before the fixes)

- Python suite at every release point: 386 (2cb3479, 9f5694b), 398 (1ed42dc), 409 (bb25353, 261fa82), 0 skips, no
  order dependence over 43 runs (reverse, shuffled, `-X dev`). Only the older test_camera.Feed timing flake failed.
- C++: 17/17 at every tag with the project's flags, ASan + UBSan, libc++ hardening, g++ 16 with `_GLIBCXX_DEBUG`, plus
  594 new runtime_model checks, 20,022 comparisons with the add-on's packing and 200,000 fuzz rounds, all clean.
- Editor: Vitest 46/46, vue-tsc clean, and the committed bundle rebuilt byte for byte at 1ed42dc, bb25353 and 261fa82.
- ESP32 builds of both boards, also fetched from GitHub the way users get them; the add-on image built and started.
- Host harnesses: full page 37/37 per board, page buttons 12/12 per board (the 0.2.71 rule fails 3/12 and 4/12, as it
  should), wake/sleep, settings, cover and history on the Guition without regressions; 4 bpp icons measured (2 grey
  levels became 15 to 16).
- Found and fixed here: the frozen second hand of a single analog clock (since 0.2.74), unsaved edits lost when clicking
  the open screen, dragging to page 2+ on a phone, Identify and Try it answering 500, a 48-tile layout too long for one
  message saving anyway, packing past slot 47, an offline screen counted as very old firmware, the 32-bit redraw mask,
  a crash instead of a refusal when memory runs out, a firm press on Previous/Next doing nothing, python as PID 1
  (every stop took 10 s), the 16 KB request limit, asterisks in What's new, docs that still said twenty tiles.

## Automated (this release)

- `tools/check.sh` (new; the same script runs in GitHub Actions), on this release rebased onto 0.2.77 (the media card):
  Python 454 tests OK, C++ 18/18 with `-Wall -Wextra -Werror`, packages and icons current, Vitest green, vue-tsc clean, build, and the committed editor bundle
  equal to a fresh build.
- `tools/check.sh --firmware` with ESPHome 2026.6.2, on copies of both board profiles with the fallback access point and
  captive portal users get:

| Board | firmware.bin | of the app slot | vs 0.2.75 |
|---|---|---|---|
| CYD | 1,652,384 B | 90.0 % of 1,835,008 B (183 KB free) | -3,200 B (Montserrat 14 no longer built: -13,936 B; the 0.2.77 media card adds the rest) |
| Guition | 2,026,608 B | 24.9 % of 8,126,464 B | |

The CYD sits at the edge of the new budget's "tight" band (90-93 %): the next release states its flash delta.

## Firmware on the Mac (host builds, virtual finger)

- Full-page harness (`fullpage_host.py`, 37 checks: taps, holds, drags, keys, page jumps, 48 tiles, six forecast/clock
  flips), on the fixes before the rebase onto 0.2.77: Guition 37/37, CYD 37/37. The CYD's first run gave 36/37: the
  harness repeats a tap when the host loop stretches the contact past 350 ms, and the firmware had already counted the
  first one (two light.toggle calls 0.7 s apart); the rerun without that stall passed 37/37.

## Not tested

- On real hardware: nothing of this release has run on a bench screen yet. To check on the glass: a single analog clock
  ticks; a flick across a full-page light on the Guition does nothing while a tap and a hold still work; one push on a
  dimmed screen with a full-page light switches it; a firm press on Next turns the page.
- Unit tests cover the repeated page tiles (tests/test_runtime_model.cpp and the Python suite), the memory refusal (a
  stand-in for the free block size on the host), a consumed contact after a flick and the 300 ms page-bar case. The
  gesture handler, the slide-off check and the waking push run inside LVGL and have code review only, no host harness
  run with a finger.
- Build machine note: Homebrew's Python 3.14.7 cannot load pyexpat on macOS 26.2 (system libexpat lacks
  `XML_SetAllocTrackerActivationThreshold`), which breaks PlatformIO's penv; the builds here ran with ESPHome 2026.6.2 in a
  Python 3.11 venv.
