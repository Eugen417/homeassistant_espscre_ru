# Test results 0.2.102 (firmware 0.2.87)

Home Assistant opens a page on a screen: `esphome.<screen>_show_page` with one field, `page`. Asked for on 2026-09-21
so an automation can put the page with a full-page player in front when a media player starts an album.

## What changed

- `packages/core.yaml`, `api: actions:`: `show_page` next to `open_settings`. It writes the page clock (`last_use_ms`,
  so Back to page 1 counts from the call, as after a touch), runs `wake_display` (a dimmed screen lights up, every card
  closes), closes the settings page, clamps `page` to 1..page_count() and draws the page through `show_tile_page`, the
  script a swipe and a Go to page tile use. One log line, `navigation: page N of M open from Home Assistant`.
- `screen_manager/app/core.py`: `SHOW_PAGE_MIN_FIRMWARE = '0.2.87'`; `FIRMWARE_VERSION` 0.2.87.
- `screen_manager/app/claude_skill.py`: a section "Open a page on a screen" before Standby and brightness, the intro
  names four things instead of three, and the description (under 200 characters) says "open a page".
- `README_EXTENDED.md`: "Open a page from an automation" after Wake and sleep. `screen_manager/CHANGELOG.md`, `config.yaml`.
- `tests/test_show_page.py` (new): every board carries the action with the clock, the wake, the close and the clamp;
  the skill and the guide explain it. `tests/test_wake_sleep.py` counts the new writer of the page clock;
  `tests/test_claude_skill.py` counts the new YAML example and checks it.

## Host test over the real ESPHome API (Guition build on the Mac, demo home with the real add-on)

`scratchpad/host/page_host.py`: the host program of the Guition (core + board file, SDL) with a `page_probe` action
that logs page, page count, settings page, open card, dimmed, alert and the age of the page clock. The demo home
(responsive-lab `demo_home.py` on the add-on code of this branch) serves the living room layout: five pages.
18 of 18 checks passed:

- Starts on page 1; `show_page 2` opens page 2, the log says `page 2 of 5 open from Home Assistant`, the page clock is
  younger than 3 s afterwards.
- `show_page 99` opens the last page (5); `show_page 0` and `show_page -5` open page 1.
- With the settings page open (`open_settings 1`), `show_page 3` closes it and opens page 3.
- With a sensor's card open, `show_page 2` closes it and opens page 2; with a light's own card open (`active_entity`
  set), `show_page 4` closes it and opens page 4.
- After **Sleep** (dimmed), `show_page 2` wakes the screen and opens page 2.
- With an alert up, `show_page 3` changes the page under it and leaves the alert in front; `dismiss_alert` takes the
  alert off and page 3 stays.
- Renders `last-page.png`, `alert-over-page3.png` and `page3.png` show the pager dots on the right page.

## Checks

- Fast tests: `test_show_page`, `test_claude_skill`, `test_wake_sleep`, `test_settings_page`, `test_release_lint`: 33 of 33.
- `tools/check.sh --all`: 20 of 20 over four boards, before the merge with 0.2.101 (CYD 1,619,184 B, 88.2 %) and again on the
  merged tree (CYD 1,619,616 B, 88.3 %; Guition 2,036,688 B; Waveshare 2,292,944 B; JC8012P4A1 2,007,456 B).

## Not done

- No real board was flashed: the bench boards are the owner's home screens and the USB board belongs to another
  round. The first call on a real screen is the owner's test, through **Update** on the screen and an automation or
  Developer tools → Actions with `esphome.<screen>_show_page`, `page: 4`.
- The Alerts cheatsheet in the editor does not list the action yet; the skill and the extended README do.
