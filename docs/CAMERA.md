# Camera images on a Guition

App 0.2.66 with firmware 0.2.57 shows camera images on a Guition 4848S040:

- **A camera tile.** Add a `camera.*` or `image.*` entity as a tile. A tap opens the image full
  screen, with the round back key at the top left like every card, and a spinner until the first
  image is there (firmware 0.2.73). The image is refreshed every four seconds while it is open. It
  is not video: ESPHome has no video decoder.
- **An alert with a picture.** Add `camera: camera.front_door` to the `esp_screens_show_alert`
  event. The card shows the picture of that moment across its top (it stays that picture), with
  the card's round corners (firmware 0.2.73); a tap on it opens the camera full screen over the
  alert, and Back returns to the alert.

The CYD has no memory for images (a 320×180 image needs 115 KB in one piece, the CYD's largest
free block is about 45 KB). It shows the alert without the picture, and the editor doesn't offer
camera tiles for it.

## The album cover on the media card

App 0.2.77 with firmware 0.2.64 uses the same road for a media player's picture. The media card
(a tap on a media player's tile, or a media tile of size *Full page*) shows the album cover of
what plays, with the title, the artist and the album, a progress bar and the keys under it.

- The screen asks ESP Screen Manager for the cover with the size it draws it at and the colour
  behind it (the event `esphome.screen_camera` with `size` and `bg`). The app fetches the
  picture where Home Assistant's state points (`entity_picture`), cuts it square, sizes it,
  rounds the corners over that colour and serves it on port 8098 as a BMP, like a camera image.
- A cover is fetched once per picture: the state carries a short mark of the picture, and the
  screen asks again only when the mark changes (a new track), when the card opens again or
  when the page turns back to a full-page media tile. Nothing polls.
- A player without a picture (a radio station, a player that is off) keeps the player's icon in
  the cover's place; the app answers with an empty link and the screen stops asking.
- The CYD shows the same card without the picture: its icon stands in for the cover.
- One picture loads at a time. An alert closes an open card and its cover. Under a media tile of
  size *Full page* the alert's picture goes first: the tile's cover waits until the alert's picture
  is there, and a cover already on its way finishes before the alert's picture starts.

## A doorbell

```yaml
triggers:
  - trigger: state
    entity_id: event.front_door_ding
actions:
  - event: esp_screens_show_alert
    event_data:
      title: Someone is at the door
      icon: doorbell
      camera: camera.front_door
      timeout: 60
```

Any `camera.*` entity works, and so does an `image.*` entity, such as the snapshot a doorbell or
motion integration keeps of its last event.

## How the image gets to the screen

The screen never talks to Home Assistant about images, and it never holds a Home Assistant token.

1. The screen asks ESP Screen Manager for a camera (the event `esphome.screen_camera`), or the app
   sends the picture of an alert by itself.
2. The app fetches the snapshot from Home Assistant with its own access (the same pictures the
   Home Assistant frontend shows), and makes it exactly as large as the screen draws it: at most
   480×480 full screen, 392×220 on an alert card, proportions kept.
3. It serves the result as an uncompressed 24-bit BMP on **port 8098** under a random link, and
   sends the link to the screen. ESPHome's `online_image` loads it.

While a camera is open, the screen loads its link every four seconds, one image at a time. The app
serves the last snapshot at once and starts fetching the next one, so each load gets a picture one
load younger: the picture changes at the screen's steady pace. (A fetch on its own clock next to the
screen's made the picture change after 1.5 s one time and 6 s the next.) A slow camera makes the
images older, never the screen slower, and nothing queues up. Nobody loading means nothing fetched.
A link that nobody loads for two minutes stops working; an alert's picture stays for half an hour.

**Why BMP.** ESPHome decodes a BMP piece by piece while it downloads (16 KB per round of its main
loop since firmware 0.2.73, 4 KB before). A JPEG of the full screen took 0.6 s in one piece on the
Guition, during which the screen missed taps. A full-screen BMP is about 390 KB; on the bench
Guition it comes in about 1.8 s (2.8 s with 4 KB).

## Network

- **Home Assistant OS:** the app publishes port 8098 on the Home Assistant host. Keep it at 8098 in
  the app's network settings; the screens need to reach Home Assistant's address on that port.
- **Docker** (docs/DOCKER.md): the container uses the host network, so 8098 is open as is. When
  the screens reach the host under another address, set `SCREEN_CAMERA_URL`, for example
  `http://192.168.1.20:8098`.
- The images travel unencrypted over the LAN, like the screens' other HTTP traffic. Links are
  random, short-lived, and only issued for a camera on that screen's tiles or in a recent alert.

## Memory and speed (Guition, measured 2026-09-17)

- Full screen: 480×270 RGB565 is 259 KB of PSRAM; the alert picture 172 KB. Both are freed when
  the camera or the alert closes. A new alert closes an open camera first.
- The internal heap stays level while a camera refreshes: 77.2 KB free after 91 images in six minutes,
  and PSRAM unchanged.
- Standby, **Back to page 1** and a new layout close the camera; nothing loads in standby.
- The first image (firmware 0.2.73, measured 2026-09-19 with an EZVIZ camera): the screen asks when
  the camera opens and loads the link as soon as it comes. About 2 s when the app still has the
  camera's last snapshot (it keeps one for 30 s after the last load), 4.5 s when it must ask Home
  Assistant first, of which 2.4 s is the camera's own snapshot. Firmware 0.2.72 took 3.4 s and
  5.8 s. Details in docs/TEST_RESULTS_0287.md.
- Wi-Fi without power save (firmware 0.2.74 sets `power_save_mode: none` itself) makes the screen
  wait on its Wi-Fi less often while an image comes in: a loop held over 50 ms in one of eleven
  opens, against five of twelve with power save.

## For developers

- `screen_manager/app/camera_feed.py`: fetching, sizing, links and the port.
- `components/smart_display/camera_view.h`: when to ask for a link and when to load again
  (`tests/test_camera_view.cpp`).
- `components/smart_display/runtime_tiles.h`: the full-screen view, the alert picture and the
  `camera` message.
- `packages/boards/guition-4848s040.yaml`: the two `online_image` components, the alert frame, and the diagnostic
  action `preview_camera` (an entity opens it, an empty entity closes it).
- `tests/test_camera.py`: the app side and the words both sides share.
