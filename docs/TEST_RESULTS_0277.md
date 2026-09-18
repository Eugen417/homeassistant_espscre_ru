# Test results app 0.2.77 / firmware 0.2.64 (2026-09-18)

Max asked for a media card "like Spotify or Apple Music now playing" instead of the plain card with three text keys
and a thin volume slider, with the album cover through the camera-image route, working as the card behind a media
tile, as a tile over the whole page, and in the light and the dark look. Then: the CYD gets the new card without the
picture ("te risky"), a player does not always have a picture, and renders first so he could approve the look.
See CHANGELOG 0.2.77. Built on 0.2.76 (`bdb1f12`).

## What changed

- `components/smart_display/media_card.h` (new): the card's geometry and words, free of LVGL. A tall area (the
  Guition's card) stacks cover, title, artist · album, the bar row (elapsed | bar | total on one line), the keys
  (previous 52, play or pause 64 on the accent, next) and the volume row. A wide area (the CYD's card, a tile over the
  whole page) puts the cover at the left with the column beside it; what does not fit goes in order: the artist line,
  the times, then the cover shrinks. `tests/test_media_card.cpp` checks every shape for overlaps and order.
- `runtime_tiles.h`: `render_media_detail` (the card), `render_media_full` (the tile over the whole page, parts in
  `w.extra`), `media_action` shared by both (20 play/pause, 21 previous, 22 next, 23 mute, 24 turn on), the card's
  key handler pulled out of `detail_button` into `detail_command`, `media_progress` moved by `tick()` once a second,
  and the cover: `cover_want`/`cover_ready`/`cover_tick`/`cover_offer` around one `camera_view::Feed` that loads a
  link once (`once`, `loaded`), owned by the card or by a tile slot and dropped when the owner goes; a camera full
  screen takes the shared image buffer and the cover comes back after. The camera answer parser accepts `t: cover`.
  The palette pass leaves the media tile's parts alone (it painted them the card's grey). A player's state falls
  back to the screen's own word ("Playing") when Home Assistant sent none.
- `runtime_model.h`: `Extra` carries artist, album, picture mark, duration, position and its moment.
- `camera_view.h`: `open(entity, once)`, `loaded`; an empty answer to a one-time feed stops the asking.
- `theme.h`: `theme::of(lv_color_t)` (the 24-bit value of a drawn colour, for the cover's corner colour).
- `tile_controls.h`: `feature::MEDIA_TURN_ON`.
- App: `core.media_extras` (`x: artist, album, dur, pos, at, pic`), `COVER_MIN_FIRMWARE = 0.2.64`;
  `camera_feed.cover_supported/can_show_cover/cover_request/encode_cover`, `CameraFeed.cover()` (fetched when the
  picture address changed), `Link.cover`; `server.HomeAssistant.media_picture/media_image`, `Manager.cover_message`,
  `answer_camera` serves covers to a Guition on 0.2.64+ for players on its layout.
- Both profiles: firmware 0.2.64. Docs: CHANGELOG, README rows, docs/CAMERA.md "The album cover on the media card".

## Checks

- `tests/test_media_card.cpp` (geometry, progress, clock text, subtitle), `tests/test_camera_view.cpp` (one-time
  feed), all 18 C++ suites: pass.
- `tests/test_media_card.py` (14): extras, firmware wiring, cover encoding (corners, square crop, transparency),
  fetch-on-change and links, the app's answer through `Manager.answer_camera` (Guition 0.2.64 gets a link, a stream
  an empty one, a stranger nothing, older firmware and a CYD nothing). Whole Python suite: 409 + 14, pass.
- Host harness (`.esphome/readme-render/media/media_host.py`, real ESPHome API, virtual finger, the app's own
  `answer_camera` behind `/dev/cover`): **Guition 57/57, CYD 42/42.** Covered: the card of a playing player (title,
  artist · album, elapsed from the reported position and running on, keys enabled, slider at 32 %, bar track and
  fill), the cover asked for with size and page colour and loaded once over the placeholder, pause → `media_play_pause`
  with the keys faded during the wait and back after, next, mute (`is_volume_muted: true`), the slider drag →
  `volume_set` ≈ 0.8, closing drops the cover; a stream (no times, no cover request); a paused muted TV with few
  features (play key, muted key, "Muted", next and mute faded, previous not, a still bar, its cover); an off player
  ("Off", no times); dark mode (dark keys, the cover asked again with the dark page colour); a tile over the whole
  page (track beside the cover, keys, slider, "48 %", times, cover at the tile's size and colour, the bar running,
  the tile's pause key acting on the study speaker without opening the card, the tile's slider → `volume_set` ≈ 0.2,
  a new picture in Home Assistant → new mark → the tile asks again and loads the new cover, holding the tile opens the
  card which takes the cover at its own size, closing gives it back to the tile); a stream over the page (no times,
  no cover, the study cover dropped); an idle player ("Not playing"); a wide media tile with a volume control keeps
  its panel; no errors in the firmware log. The CYD asks for no cover anywhere.
- Renders (`render_media.py`, both boards, light and dark): card of a playing player, a stream, a paused TV, an off
  player, tiles over the page (study, radio, idle). Sent to Max for approval.
- ESP32 builds of both Easy Setup profiles: see the sizes below.
- Hardware: Studio 1 (Guition, this network's Home Assistant) — see below.

## Sizes

Both Easy Setup profiles compile with ESPHome 2026.6.2 (on Python 3.11, see the pitfalls), no warnings.

| Build | Flash | RAM |
|---|---|---|
| Guition (easy-guition-device) | 1,971,583 B, 24.3 % of 8 MB | 76,844 B, 23.5 % |
| CYD (easy-cyd-device) | 1,582,063 B, 86.2 % of the 1.835 MB update slot | 71,276 B, 21.8 % |

## Hardware

Pending at the time of writing.

## Pitfalls

- `render_time` on a host build freezes the clock, and the progress bar with it: the harness shifts the demo's
  report moments to now instead.
- LVGL's `LV_LABEL_LONG_DOT` writes the dots into the label's text: a probe sees "Miles Davis · Kin...".
- `tick()` re-enables every object in `detail_actions` after a wait: a key the player lacks must stay out of it.
- The tile palette pass paints every custom part; a mode that paints its own parts must be skipped there.
- Homebrew's python@3.14.7 on this Mac has a broken `pyexpat` (it wants a libexpat symbol macOS 26.2 lacks), so
  `platform.mac_ver()` returns '' and both pip's truststore and `uv` refuse the interpreter. The pioarduino platform
  recreates `~/.platformio/penv` from the running Python and then cannot install into it. The ESP32 builds of this
  release ran from an ESPHome 2026.6.2 environment on PlatformIO's own bundled Python 3.11 (`.esphome/venv311`,
  local); `brew reinstall python@3.14` (or expat) is the real fix. Also: never run two ESPHome builds at once, the
  second one clears the penv the first one uses.
