# Test results app 0.2.128 / firmware 0.2.103 and app 0.2.129 / firmware 0.2.104 (2026-09-23)

Two releases, tested together: the alert card laid out on every glass with its camera picture in the camera's own
proportions (0.2.128), and the board catalog, New screen drawn from it, and every board built and checked on this
computer on every change (0.2.129). This is what was checked, and how.

## The alert on every glass (0.2.128)

- **One rule, twice.** The firmware's layout (`screen_alert::layout` in `alert_overlay.h`) and the add-on's copy
  (`screen_manager/app/alert_layout.py`) were compiled and run side by side on two looks, eight densities, nine by
  nine canvas sizes and seven picture shapes (none, 16:9, square, 3:4, 21:9, 9:16 and the old fixed frame): the same
  numbers in every case (`tests/test_alert_layout.py`). `tests/test_alert_overlay.cpp` holds the invariants: the picture
  above the words or on their left and never over them, the button inside the card on its right edge, the subtitle in
  whole lines.
- **Where the picture goes**, per board and camera (the frame the picture is sent at):

  | Board | 16:9 camera | Square camera | 3:4 doorbell |
  |---|---|---|---|
  | Guition 4-inch | above, 391 x 220 | above, 257 x 257 | above, 192 x 257 |
  | Guition 10.1-inch | above, 344 x 194 | above, 264 x 264 | above, 198 x 264 |
  | Waveshare 4.3, lying down | above, 348 x 196 | on the left, 342 x 342 | on the left, 288 x 384 |
  | Waveshare 4.3, standing up | above, 387 x 218 | above, 384 x 384 | above, 288 x 384 |
  | Waveshare 7 | above, 305 x 172 | above, 234 x 234 | above, 175 x 234 |

  The largest picture sent is about 430 KB (a square camera on the 4.3-inch standing up); most are 150 to 250 KB.
- **The picture's way.** One snapshot per alert, fetched once and measured from its header (turned the way its EXIF
  says); one encode per frame size, all sizes at the same time; each screen its own link. Tested with a wall of screens
  of different firmware (`tests/test_camera.py`): one fetch, every screen its own size, all from the same snapshot. A
  screen whose Override YAML changes its density gets the frame it really draws: the add-on works the card's fonts out
  exactly for the density and look the screen reports (checked against the TrueType files for every size from 8 to 96
  px, and against every board's own line heights).

## The catalog and New screen (0.2.129)

- `boards.yaml` is the one list; everything else about a board is worked out from its files (the size in inches, the
  touch controller, the grids, what it can do) and checked by `tests/test_board_catalog.py`, which also keeps New
  screen, the screen list and the translations free of any board's name.
- New screen was looked at in the editor against a demo home: all six boards in the catalog's order, each glass in its
  own proportions with the tiles of one page, the experimental ones marked, what each can do, and the CYD's display
  controller choice. The editor's tests (156) drive it: the list, the choice starting at the board file's value, only
  a changed choice sent, the orientation still asked only for glass that is not square.
- **Adding a board**, tried for real: `tools/new_board.py` from the Guition, the CYD and the Waveshare 4.3 templates.
  Each became a complete board that ESPHome reads (at the template's own resolution; at another one the template's
  display driver refuses the glass until its hardware is replaced, as it should), and one compiled. `check_packages`
  refuses the copied hardware until it is replaced, which is the point.
- The entry files are generated from the catalog now; their content was compared before and after as parsed YAML: all
  twelve the same, only the comments changed.
- The checkout entries moved to `checkout/<board>.yaml`; every board compiles from there. The add-on never wrote a root
  entry into a screen's YAML (checked in the history), so installed screens are not touched.

## Every board on this computer

`tools/render/run.py` (and `tools/check.sh --render`, and CI) builds every board of the catalog as a host program, lying
down and, where its glass is not square, standing up: ten programs. Each gets the demo layout and runs its own self
test, then every page, three alerts, a camera alert and Dark mode are saved as pictures.

- It found two things the self test had never been run for. The sun card's glow stood a pixel over the card's edge at
  sunrise and sunset on most boards (fixed: the path keeps half the glow, rounded up, from either side). And the self
  test still held a big value on a short cell to a rule the layout had left behind (the 4.3-inch lying down); the check
  now allows the digits' own top space the layout gives them. App 0.2.127 fails both checks as well.
- With both fixed: 10 of 10 pass. Compared with the renders of app 0.2.127: the CYD's seven pictures are identical to
  the pixel (its move onto the shared backlight and self-test code changes nothing it draws); the Guition and the
  4.3-inch differ where these releases change them (the alert, the sun card).
- A subtitle longer than the 160 bytes a CYD keeps stopped in the middle of a word; clipped text ends in "..." now.

## On the glass

A 4-inch Guition on USB, used as a test board (it is not paired with a Home Assistant):

- Flashed over USB with firmware 0.2.103, then 0.2.104. Both boot and join the Wi-Fi; about 105 KB of internal heap free.
- The self test on firmware 0.2.103 with the demo layout: three of ten page checks failed, the sun card's glow a pixel
  outside the card, exactly as the host program found it. On 0.2.104: 10 of 10 pass, no error line.
- The alerts over the API, with the screen's own picture of what it draws after each: plain, long (whole lines ending
  in "..."), with a button, and a camera alert whose picture came from this computer over the network at the size the
  add-on's own code gave it, the way ESP Screen Manager serves it. A 960 x 540 camera went out at 391 x 220 (259 KB) and
  was on the screen 1.6 s after the link; a standing 540 x 720 doorbell at 192 x 257 (148 KB), centred above the words,
  after 1.1 s. The picture's frame showed whole, nothing cut off. No error line.

Not checked on the glass: the CYD with firmware 0.2.104 (its backlight now takes the shared script: standby dims over
1.5 s, an alert's blinks fade for 60 ms). It compiles, runs its self test on the host and draws the same as before; its
first real run is the next update on a CYD.

## Gates

`tools/check.sh --all --render` with ESPHome 2026.9.0: 25 of 25. 623 Python tests, 24 C++ tests, the package check, the
cells, board shapes, entry files and icon checks, the translations, the editor's tests, types, build and bundle, the nine
overrides from GitHub issues, every board compiled, and every board on the host:

| Board | Firmware image | Share of its update slot |
|---|---|---|
| CYD | 1,628,464 B | 88.7 % |
| Guition 4-inch | 2,045,728 B | 25.2 % |
| Waveshare 4.3 | 2,302,592 B | 28.3 % |
| Guition 10.1-inch | 2,016,112 B | 24.8 % |
| Waveshare 7 | 1,831,552 B | 46.6 % |
| Waveshare 4B | 2,030,832 B | 25.0 % |

The CYD grows 688 bytes over app 0.2.127 (1,627,776 B), some 400 of them its move onto the shared backlight and self-test code.
