# Test results app 0.2.87 / firmware 0.2.73 (2026-09-19)

After the README picture refresh Max asked for four things:

- cameras in the README as tiles of their own ("zodat mensen snappen dat camera niet alleen tijdens alert is");
- the alert card's rounded corners on its camera picture, if that stays light;
- the spinner instead of "Loading image" on a camera that opens, and a look at why its first picture takes so long;
- a nicer starting screen: the text in the middle, a spinner under it.

Performance comes first for him, and the add-on often runs on a Raspberry Pi 4 class machine (his Home Assistant
Yellow is one), so work moved to the add-on is not free either. Everything below stays on ESPHome's and LVGL's own
means: `online_image`'s `buffer_size`, LVGL's spinner and image radius, no ESP-IDF or lwIP settings.

## Round corners on the alert's picture

### Three ways to round the picture

- **`clip_corner` on the frame.** LVGL 9.5 then draws the frame and its picture into a layer of the frame's size to
  clip them (392 × 220 ARGB8888, about 345 KB, on every redraw). The project keeps layers out:
  `tests/test_layer_free.py` refuses `clip_corner` (see the CYD memory research of 0.2.50).
- **Corners baked in by the add-on**, as for the media covers. The screen would copy a square picture as it does now,
  but the add-on would have to know the exact colour behind each corner: the alert's swatch in the light or the dark
  look, which only `theme.h` on the screen knows, and it would have to make the picture again (Pillow, on a Pi) when
  the look changes while the alert is up. A wrong guess shows as coloured corners.
- **LVGL's own image radius (chosen).** `lv_image` passes its `radius` style to the draw as `clip_radius`, and the
  software renderer's `radius_only()` (`lv_draw_sw_img.c`) blends the picture row by row with a mask of one row
  (392 bytes). Rows between the corners come back from the mask as fully covered and are copied as before; only the
  rows of the corners are blended through the mask. No layer and no allocation beyond that row.
  `LV_DRAW_SW_COMPLEX` is 1 in ESPHome's `lv_conf.h`, so this path is compiled in; rounded rectangles already use it.

### What changed

- `packages/boards/guition-4848s040.yaml`: `alert_image_frame` has radius 18, the radius of the alert card
  (`packages/core.yaml`), instead of 0, so the frame behind a picture that doesn't fill it is rounded too.
- `components/smart_display/runtime_tiles.h`: `camera_show(..., radius)` gives the picture that radius when it
  creates it; `camera_loaded()` passes the frame's radius for the alert's picture. The camera full screen and the
  media cover pass none and stay square.
- README.md: a camera row under "On the screen" (`guition-camera-tiles.png` new: a page with a camera, a live camera
  and a doorbell's last ring; the camera full screen; the alert with its round picture). README_EXTENDED.md: a
  Cameras bullet under "What you can configure" and the tiles picture beside "With a camera picture".

### Measured (host build on the Mac, `.esphome/readme-render/readme-0919/time_radius.py`)

The render time of the picture's frame after invalidating it (LVGL's render events minus the flushes to the SDL
window, which are the same for both), 40 redraws per run, four runs alternating between the two radii:

| Radius | Fastest | Median of the runs' medians |
|---|---|---|
| 0 (square, as before) | 1269 µs | 1340 µs |
| 18 (the card's) | 1352 µs | 1413 µs |

About 5 % more, and only when the picture is drawn: when the alert appears, when the camera full screen closes over
it, when something next to it is redrawn. A picture that stays up costs nothing afterwards. These are host numbers;
the Guition is slower in absolute terms, and per row the extra work stays one 392-byte memset and one blend call.

## The starting screen

Until the first layout the top bar read "Connecting to Home Assistant..." and then "Waiting for ESP Screens..." where
the screen's name goes (0.2.43). Now `render()` calls `boot_status()`, which puts the text in the middle of the page in
the name's font (`watch_font`, 27 px Guition, 18 px CYD) with a spinner under it (48/32 px, 24/16 px apart), the two
as one block in the middle; the name stays empty. The first layout deletes the block, spinner and all, so nothing
turns behind the tiles and no animation runs for the rest of the day. The block sits in the name's place in the
drawing order, under the tiles, the cards and an alert (an alert can come before the tiles). It is drawn in the
shared paints (`ink`, `spinner`), so Dark mode colours it like the rest. `ROOM_NAME`, the name in the profile until the
first `render()`, is empty instead of "Choose tiles in HA", which showed for a moment at every start. The text has no
"..." any more; the spinner says it is busy.

The three spinners of the firmware (a busy card, the starting screen, a camera that loads) come from one
`spinner_create()` in `runtime_tiles.h`; the busy card's looks the same as before.

## A camera that opens

### The spinner

The camera full screen shows the spinner in the middle until the first picture is there, instead of "Loading image".
Its ring uses the new theme role `CAMERA_TRACK` (0x393D42, the dark look's spinner ring), since the camera page is
black in both looks. A note ("No image from this camera") takes the spinner's place; the first picture ends both.

### Where the wait went

A camera opened full screen on the bench Guition (fw 0.2.72, 4 KB chunks), timed from the screen's own log
(`.esphome/camera-speed/timeline.py`), with Max's EZVIZ camera `camera.max`:

1. 0.2-0.35 s until the screen asked: `camera_tick()`, every 250 ms, sent the request, not `camera_open()`.
2. 2.6 s until the link came: Home Assistant's snapshot of the EZVIZ takes 2.4 s (the camera itself; measured the
   same in 0.2.66); the add-on's encoding is small next to it (a 1080p JPEG to the 480 × 270 BMP takes 10 ms on the
   Mac, several times that on a Pi 4 class machine).
3. Up to 0.25 s until the load started: the next tick again.
4. 2.8 s download: 388,854 bytes at 136 KB/s.

Step 4 is ESPHome's `online_image`: every round of the main loop it reads one `buffer_size` (then 4 KB) from the
connection and decodes it, and `esp_http_client_read` waits until that chunk is complete. 4 KB a round came to about
136 KB/s. A bigger chunk a round is faster (the table below), up to what one TCP connection with lwIP's 5,760-byte
window gets over this Wi-Fi, roughly 300 KB/s. The same 2.7-3.3 s was measured in 0.2.66 from the Mac and from the
Yellow: the limit was on the screen's side, not the sender's.

### What changed

- `camera_open()` asks right away; `camera_answer()` starts the load as soon as the link comes, as an alert's
  picture already did. Both through `camera_load()`, which, like the tick, never starts under a finger.
  `camera_tick()` keeps doing the refresh every 4 s and the retries.
- `online_image` `buffer_size` 16384 instead of 4096, for both images.

### Measured on the bench Guition

`.esphome/camera-speed/speed.py`: per open, from the `preview_camera` call (the same path as a tap on the tile) until
`online_image` reports the picture complete. Cold: 35 s after the last close, so the add-on has forgotten the snapshot
and asks Home Assistant for a new one. Warm: 5 s after the last close, so the add-on answers with the snapshot it has
and fetches the next one meanwhile. Three opens each (two cold for 32 KB); medians. The 0.2.73 builds went over the
air from `.esphome/camera-speed/guition-wallbox*.yaml` (the Device Builder's API, OTA and Wi-Fi settings, the packages
of this branch). One "cold" open of the power-save-off build found the snapshot still there and is left out.

| Build | Wi-Fi power save | Cold open | Warm open | Download | Longest loop wait during the opens |
|---|---|---|---|---|---|
| fw 0.2.72, 4 KB, tick | light (ESPHome's default) | 5.80 s | 3.40 s | 2.86 s (136 KB/s) | 131 ms (lvgl), 65 ms |
| 0.2.73, 16 KB | light | 4.36 s | 2.09 s | 1.74-1.91 s (216-223 KB/s) | 58, 61, 77 ms |
| 0.2.73, 16 KB | none | 4.24 s | 1.99 s | 1.73-1.80 s (223 KB/s) | none over 50 ms |
| 0.2.73, 32 KB | none | 3.94 s | 1.62 s | 1.42-1.45 s (271 KB/s) | 101 ms |
| 0.2.73, 16 KB (this release) | light | 4.55 s | 2.06 s | 1.75-1.90 s (208-218 KB/s) | 93, 107 ms |

The screen now asks 20-150 ms after the call (the API call and the log line's way back included) instead of
210-345 ms, and loads about 150 ms after asking when the add-on has the snapshot. "Longest loop wait" is ESPHome's own
"took a long time" warning (over 50 ms). With Wi-Fi power save, `online_image` now and then held a loop up to 107 ms
(65 ms with 4 KB, next to a 131 ms LVGL frame in the same opens); without power save nothing went over 50 ms.

- **16 KB, not 32 KB.** 32 KB saves 0.3 s more, but a read of 32 KB waits for several TCP windows (lwIP's is 5,760
  bytes in ESPHome's build) and held a loop 101 ms even without power save: a tap in that moment waits as long.
- **Wi-Fi power save.** The bench Guition's Device Builder YAML dates from before 0.2.24 and has no
  `power_save_mode: none`, so it runs ESPHome's default `light`: pings 103-191 ms (117 ms average) against 5-94 ms
  (21 ms) without it. With 16 KB chunks it made no difference to the download, but without it no loop waited over
  50 ms. The YAML the app writes has had `power_save_mode: none` since 0.2.24 (EASY_SETUP.md says why); the package
  doesn't set it, since it is the owner's Wi-Fi block.
- **What remains** of a cold open is the camera: 2.4 s of the 4.5 s is Home Assistant's snapshot of the EZVIZ. Only
  fetching before the tap would hide it (when a camera's page shows, or when the screen wakes). That polls cameras
  nobody looks at, and a battery camera wakes for it; it is left for Max to decide.
- Not changed: lwIP's TCP window and other ESP-IDF settings (`sdkconfig_options`). They would speed up the reads
  further but are memory and platform tuning outside ESPHome's defaults.

## Automated

- `tools/check.sh` (worktree, after the rebase on 0.2.86): Python 473 tests OK (two new: `test_camera` "the first
  picture waits for no tick", `test_header_bar` "a starting screen says what it waits for in the middle"; the
  position check of `render_header()` now looks in `render()` as a whole, and the spinner paint check in
  `spinner_create()`), C++ 19/19, packages, icons, editor tests, types, build and bundle PASS.
- `tools/check.sh --firmware --baseline 1677504`: CYD 1,678,816 B of 1,835,008 B = 91.5 % (156,192 B free),
  +1,312 B against 0.2.86 (WARN: the 90-93 % band, under the 8 KB that needs Max's OK); Guition 2,055,280 B = 25.3 %.

## Renders

- Host builds from this branch (`.esphome/readme-render/readme-0919/boot_camera_host.py`, both boards, light and
  dark): the starting screen with both texts, page 1 after the layout without it, and on the Guition the camera page
  with the spinner, with the picture and with the note.
- `guition-alert-camera.png` from the new firmware on the host: only the four corners of the picture changed
  (1,400 pixels of the 960 × 960 README image).
- `guition-camera-tiles.png`: the camera tiles page of the README demo home (Home Assistant's icons and words), the
  front door camera idle and grey, the garden camera live and amber, since 0.2.85's colours.

## Hardware

- Bench Guition Wallbox (192.168.146.136): four builds of this branch over the air for the table above (its first
  row is the toggle-key session's 0.2.72 bench build that was on it); it runs this release's firmware (16 KB, its
  own `light` Wi-Fi). An LVGL snapshot during an open shows the spinner on the black camera page. After a
  restart the add-on's layout was there before my own API connection (6.8 s), and the snapshot showed "Loading tiles..."
  with the tiles in place: the starting screen had gone as it should.
- The CYD: compiled only.

## Not tested

- The round alert picture on the panel: the add-on sends an alert picture only with the `esp_screens_show_alert`
  event, which goes to every screen, Studio 1 included. It shows with the next doorbell or camera alert.
- The starting screen on the panels themselves: it is up for the few seconds between the start and ESP Screens'
  layout. Max sees it at the next update or restart.
