# Test results app 0.2.91 / firmware 0.2.77 (2026-09-20)

Four asks in one round: a live picture on a camera tile (issue #4, "show a camera preview on the tile as an
option"), an action behind the alert's button, media titles that roll by, and full-page tiles without a colour of
their own. The live picture is the one with design in it; the rest is small.

## Live pictures: the design and why

- **One picture per page, not per tile.** The screen already has two `online_image`s and a rule that one picture
  loads at a time (a download shares ESPHome's main loop with touch and drawing). Six tiles each loading their own
  picture would mean six downloads and six buffers. Instead the screen asks for all the live tiles of the page at
  once and the app serves one BMP: a strip of squares, top to bottom in slot order. Every tile draws its own square
  out of that one image with LVGL's image offset. A page with six live tiles costs one download of about 50 KB.
- **The corners come from the app.** LVGL's image radius (`clip_radius`) rounds the whole source image, not each
  square of it, and `clip_corner` would draw into a layer (`tests/test_layer_free.py`). So the app bakes the rounded
  corners into each square over that tile's own colour, as it does for a media cover; the screen sends the colour of
  every tile with the request (`bg`), so dark mode and pastel tiles come out right.
- **A third `online_image`** (`tile_image`) holds the strip: it lives beside a cover or a camera full screen instead
  of sharing their buffer, and PSRAM has room. Loading still waits its turn: after the alert's picture, a cover or
  the camera full screen, never under an open card, in standby or under a finger.
- **The pace is the tile's.** 15 or 30 s per tile; the page loads at the pace of its fastest tile, and the app fetches
  a camera again only when that camera's own pace has passed. Nobody loading means nothing fetched.
- **The size is the icon's.** The square takes the icon circle's place and size (54 px on a Guition tile, 128 px on a
  tile over the whole page), so the tile's text does not move.

## Host harness (Mac, host build of the Guition package, real add-on code)

`live_host.py` builds the Guition package as a host program with probe actions and drives it over the ESPHome API
against a demo home with five cameras (`live_demo_home.py`, the real `Manager.answer_camera` behind `/dev/camera`);
git-ignored tooling under `.esphome/readme-render/live/`. 27 checks, all passing:

- page 1 asks for its three live tiles (two single tiles and a wide one, one of them on a pastel) with size 54 and
  the three colours; the strip loads and every tile shows its own square (offsets 0, −54, −108), the circle hidden
  under it, the plain camera tile keeping its icon;
- the page loads again after 15 s; the 15 s camera is fetched again while the 30 s camera is not, and after 30 s
  both are;
- the camera full screen pauses the strip (no load for 17 s) and it loads again once closed, the pictures back;
- a full-page live tile alone on page 2 asks at 128 px and shows its picture at 128;
- a page without live tiles wants nothing and holds no picture; a media tile over the whole page still asks for its
  cover; a camera without a picture keeps its icon (the app answers with an empty entry and no link);
- back on page 1 the strip is asked for again; a card over the page pauses the loads and closing it resumes them;
  dark mode asks again with the dark grounds (`1A1A1A,4A2422,1A1A1A`) and shows the dark strip.

Found on the host, fixed before the bench: LVGL's default inner alignment centres a source that is taller than its
object, so every tile showed the square of its neighbour (and the corners of the wrong colour); the picture now sets
`LV_IMAGE_ALIGN_TOP_LEFT`. The host's `http_request` fails a 304 outright where the ESP32's `online_image` takes it as
"unchanged"; the harness counts both as a load.

The rolling title was checked the same way: two renders of a full-page media tile 1.5 s apart differ only in the rows
of the title and the artist line.

## Bench Guition (192.168.146.136, firmware 0.2.77 from this tree over OTA, local add-on 0.2.91 on the Yellow)

- `camera.buienradar` live on page 1: the screen asked for the strip at once, the radar showed as a rounded square
  in the icon's place on the dark tile (`capture_ui.py`), and every 15 s the screen loaded again. The radar changes
  every five minutes, so the app answered 304 in between, and ESPHome's `http_request` logged every one of those as
  `HTTP Request failed; Code: 304` and raised its error flag (`online_image` then said "Download skipped"). The app
  now serves a live strip whole every time, never a 304 (about 9 KB for one tile, 50 KB for six, on the LAN), and
  without the ETag `online_image` warned "No header with name etag found" and "... last-modified ..." at every load,
  so every picture now carries both headers. No "took a long time" warning during the loads.
- The alert with `action: persistent_notification.create` reached both bench screens; the button's report
  (`esphome.screen_alert`, `action: ok`, sent the way the screen sends it) made the notification once, and the add-on
  logged `Alert button on guition-wallbox: persistent_notification.create performed`.

## Builds

`tools/check.sh`: Python 516 tests, C++ tests, packages, icons, translations (9 languages, 1431 texts), editor tests
(106), types and build. `tools/check.sh --firmware` with ESPHome 2026.9.0: CYD 1,633,728 B, 89.0 % of the update slot
(832 B more than 0.2.90); Guition 2,058,368 B.
