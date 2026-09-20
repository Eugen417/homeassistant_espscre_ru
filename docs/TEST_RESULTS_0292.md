# Test results app 0.2.92 / firmware 0.2.78 (2026-09-20)

One ask, right after 0.2.91: the album cover on a media player tile, the way Home Assistant's own tile shows it, on the
same road as the live camera pictures. "Even via dezelfde endpoint, toch?" Yes.

## What changed, and why it is small

- **The strip takes media players.** The page's request (`tiles`, `size`, `bg`) and the answer (op `camera`,
  `t: "live"`) are the same as in 0.2.91; a media player tile with `display: cover` is simply among the tiles. The
  app tells the two apart by domain: a camera gets a snapshot at its pace, a media player its `entity_picture`,
  fetched again only when that address changes (the same fetch as the media card's cover, `CameraFeed.cover_raw`).
- **Two clocks in one download.** The screen loads the strip at the pace of its fastest camera; the app decides per
  entity whether anything new is fetched from Home Assistant. A page with a 15 s camera and a Sonos loads its strip
  every 15 s with the cover reused; a page of media tiles alone has no clock and loads once. A new track changes the
  picture's mark in the player's state, which is part of the page's wish, so the screen asks again at once.
- **Only players with a picture ask.** A radio station or a player that is off has no `entity_picture`; such a tile is
  not in the request and keeps its icon. The tile over the whole page keeps the card's big cover through the older
  route; a double-width tile keeps its controls (the cover is the standard layout with a picture).

## Host harness (Mac, host build of the Guition package, real add-on code)

`live_host.py` of 0.2.91 with a Sonos tile (cover) on the camera page and a page of two media tiles (a wide one with
the volume control and a radio without a picture); 31 checks, all passing:

- page 1 asks for its three live tiles and the Sonos together, with the four colours; the strip loads and every tile
  shows its own square (offsets 0, −54, −108, −162), the Sonos' cover where its icon was;
- the 15 s pace, the 30 s camera fetched every other load, the camera full screen and a card pausing the strip, a
  page turn and dark mode asking again: as in 0.2.91, now with the media tile in the list;
- the page of media tiles asks for the one player with a picture only, shows its cover beside the volume slider
  while the radio keeps its icon, loads once (no load in 17 s), and asks again the moment the player's picture
  address changes, loading the strip once more;
- a player without a picture, a media tile that shows its icon and a media tile over the whole page are not in the
  strip; the full-page tile still asks for its own cover.

Found on the way: the harness first expected the radio in the request; the firmware leaves a player without a
picture out, which is the better answer (no request for nothing).

## Bench Guition (192.168.146.136, firmware 0.2.78 from this tree over OTA, local add-on 0.2.92 on the Yellow)

- The Sonos in the living room (single tile, Display → Album cover) on page 1: as long as every Sonos played "TV"
  (no `entity_picture`) the tile kept its icon and the screen asked for nothing. The moment a track with art started,
  the state's picture mark reached the screen, the page asked for its strip (`Live pictures of
  media_player.sonos_woonkamer on Guition Wallbox` in the add-on's log) and the tile showed the cover in the icon's
  place (`capture_ui.py`, dark mode). A tap opened the media card with its big cover as before (69 KB, the older
  route; its 63-150 ms loop warnings while decoding are the same as in 0.2.77). Camera.max and Buienradar keep their
  live pictures beside it on page 8.

## Builds

`tools/check.sh` and `tools/check.sh --firmware` with ESPHome 2026.9.0: CYD 1,633,760 B, 89.0 % of the update slot (32 B more than 0.2.91); Guition 2,058,656 B.
