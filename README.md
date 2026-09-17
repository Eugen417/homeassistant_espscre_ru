# Home Assistant ESP Screens

<p align="center">
  <img src="docs/images/photo-guition-page-2.jpg" width="49%" alt="The Guition 4-inch screen on a table: the second page with scenes and scripts, the robot vacuum and a table lamp with a brightness slider">
  <img src="docs/images/photo-guition-vacuum.jpg" width="49%" alt="The vacuum card on the Guition: start cleaning, return to dock, find my robot and the suction power">
</p>

**A touch screen for every room that you lay out yourself, and manage from Home Assistant.**

**Why.** Your phone is in the other room and a tablet on the wall is expensive. A small ESP32 touch
panel costs a fraction of that, sits on a table or in a wall box, and is always at hand for the
lights, the heating or the vacuum.

**What.** Firmware for two affordable panels, the 2.8-inch CYD and the 4-inch Guition, with up to
twenty tiles over four pages: lights, climate, blinds and curtains, the vacuum, media, the weather,
history graphs, clocks and timers. An automation can put an alert on every screen when someone rings
the bell, and a Guition shows who is there with the doorbell camera's picture.

**How.** ESP Screen Manager is an app inside Home Assistant. It flashes a new screen over USB, updates
it over Wi-Fi and sends it your tiles. Changing a screen is pick, drag and **Save & send to screen**:
no reflash, no YAML to write, no blueprint, MQTT or token.

**[Install it](#installing-from-home-assistant)** · [Full reference](README_EXTENDED.md) · [What's new](screen_manager/CHANGELOG.md)

## On the screen

<p align="center">
  <img src="docs/images/guition-home.png" width="32%" alt="Guition 4-inch screen: an analog clock with the date, a temperature graph, the weather forecast, a lamp and presence">
  <img src="docs/images/guition-controls.png" width="32%" alt="Double-width tiles with direct control: the heating setpoint, a dimmer and the Sonos volume">
  <img src="docs/images/guition-page-4.png" width="32%" alt="An energy graph, the robot vacuum, a coffee machine, a fan and a good-night script with pastel backgrounds">
</p>
<p align="center">
  <img src="docs/images/guition-weather.png" width="32%" alt="Weather card: current weather, the coming hours and the coming days with chance of rain">
  <img src="docs/images/guition-climate.png" width="32%" alt="Climate card: the target temperature between big minus and plus keys, the mode keys, and fan and swing choices">
  <img src="docs/images/guition-light.png" width="32%" alt="Light control: color, color temperature and brightness">
</p>
<p align="center">
  <img src="docs/images/guition-vacuum.png" width="32%" alt="Vacuum card: docked and charging, start and dock, the cleaning mode vacuum, vac and mop or mop, suction and water">
  <img src="docs/images/guition-blind.png" width="32%" alt="Cover card for a venetian blind: its battery, the position slider with the blind hanging from the top, the tilt slider over slats, and open, stop and close">
  <img src="docs/images/guition-history-touch.png" width="32%" alt="A finger on the history graph: the top of the card shows the average of that hour and its time, the graph stays as it is">
</p>
<p align="center"><sub>Tap a tile to switch it, hold it for the full card. Keys and sliders right on the tile, pastel colors, a clock, the weather and history you can read with a finger. Rendered from the firmware's own LVGL code with a demo home.</sub></p>
<p align="center">
  <img src="docs/images/cyd-home.png" width="32%" alt="CYD 2.8-inch screen: the weather forecast, a kitchen timer, the coffee machine, a lamp and power usage as a large value">
  <img src="docs/images/cyd-page-2.png" width="32%" alt="Second CYD page: Sonos volume, presence, a scene and an energy graph">
  <img src="docs/images/cyd-weather.png" width="32%" alt="The weather card on the CYD: current weather, the coming hours and the coming days">
</p>
<p align="center">
  <img src="docs/images/cyd-tiles-controls.png" width="32%" alt="The CYD with heating mode keys, playback keys for the radio and a ceiling fan's speed slider">
  <img src="docs/images/cyd-vacuum.png" width="32%" alt="The vacuum card on the CYD: state, battery and charging, clean and dock, the cleaning mode, suction and water">
  <img src="docs/images/cyd-history.png" width="32%" alt="The history card on the CYD: power over 24 hours with its highest and lowest moment, an axis in watts and clock times">
</p>
<p align="center"><sub>The same cards on the 2.8-inch CYD, 320 × 240.</sub></p>
<p align="center">
  <img src="docs/images/guition-dark-home.png" width="32%" alt="Dark mode on the Guition: the same home page with a black page, graphite cards and soft white text, the clock, the temperature graph, the weather, a lamp and presence">
  <img src="docs/images/guition-dark-controls.png" width="32%" alt="Dark mode with direct control: the heating setpoint, the dimmer's slider and the Sonos volume keep their colours">
  <img src="docs/images/guition-dark-page-4.png" width="32%" alt="Dark mode with pastel backgrounds: the coffee machine and the good-night script keep their colour, deeper">
</p>
<p align="center"><sub>Dark mode, for a screen beside the bed: the same pages, darker. It is a switch on the screen, in ESP Screens and in Home Assistant, so an automation can turn it on at bedtime.</sub></p>

## Managed from Home Assistant

<p align="center">
  <img src="docs/images/editor.png" width="98%" alt="ESP Screens in Home Assistant: the screens on the left, the top bar and tiles of the selected screen on the right">
</p>
<p align="center"><sub>ESP Screens, a page in Home Assistant: every screen on the left, its top bar and tiles on the right.</sub></p>
<p align="center">
  <img src="docs/images/editor-tiles.png" width="59%" alt="Choosing tiles: every page in the screen preview, next to the entity picker with filters">
  <img src="docs/images/editor-tile-settings.png" width="37%" alt="Tile settings: name, icon, display, width, direct control, tap action and pastel background">
</p>
<p align="center"><sub>Search your home, drop a tile on the preview, tap it for its name, width, control and color. Save, and the screen has it.</sub></p>
<p align="center">
  <img src="docs/images/editor-top-bar.png" width="31%" alt="Add to the top bar: the time, an analog clock, the date and suggestions from your own home">
  <img src="docs/images/editor-settings.png" width="65%" alt="Settings in ESP Screens: New screen and Firmware & USB, the firmware updates, the Alerts cheatsheet, and the Claude skill">
</p>
<p align="center"><sub>A top bar built from your own home, firmware updates over Wi-Fi (every night if you like), and a skill so Claude can rearrange your screens.</sub></p>

## Alerts

<p align="center">
  <img src="docs/images/guition-alert-camera.png" width="41%" alt="An alert on the Guition with the front door camera's picture across the top: someone is at the door, with a Coming button">
  <img src="docs/images/guition-camera.png" width="41%" alt="The front door camera full screen on the Guition, with the round back key and the camera's name at the top">
</p>
<p align="center"><sub>Someone at the door? One event in an automation wakes every screen and shows it. Add the doorbell camera and a Guition shows who is there; tap the picture, or a camera tile, for the camera full screen, refreshed every few seconds. <a href="docs/CAMERA.md">Camera images</a>.</sub></p>
<p align="center">
  <img src="docs/images/guition-alert.png" width="41%" alt="An alert on the Guition: someone is at the door, with a Coming button">
  <img src="docs/images/editor-alerts.png" width="53%" alt="The Alerts cheatsheet in ESP Screens: the action name of every screen, ready to copy">
</p>
<p align="center"><sub>Any alert, on one screen or all of them, in a pastel color of your choice. <a href="README_EXTENDED.md#alert-from-an-automation">How alerts work</a>.</sub></p>

## Installing from Home Assistant

### Which screen

| Screen | Resolution | Display / touch |
| --- | --- | --- |
| CYD ESP32-2432S028 | 320 × 240 | ILI9341 / resistive XPT2046 |
| Guition ESP32-S3-4848S040, 4 inch | 480 × 480 | ST7701S RGB / capacitive GT911 |

Use these exact board variants: similar-looking product names can have different
controllers or connectors. Wallbox relays are not controlled.
The firmware and built-in CLI are tested with **ESPHome 2026.6.2**.

### Do I need ESPHome?

**You don't need to install the separate ESPHome Device Builder app.**
ESP Screen Manager already includes the ESPHome CLI and can build firmware itself,
install it via USB, and later update it wirelessly over OTA.

**You do need to pair the flashed screen via the ESPHome integration in HA.**
That pairing lives under **Settings → Devices & services**, not in the
App store. Add the discovered device there. If it doesn't appear automatically,
choose **Add integration → ESPHome** and enter the screen's IP address.
If asked for a key, use the `api.encryption.key` from your own device YAML,
and grant the device permission to perform Home Assistant actions.

| Component | Needed? | What for? |
| --- | --- | --- |
| ESP Screen Manager app | Yes, for this installation route | Installing firmware, managing tiles, and sending current data to the screen |
| ESPHome Device Builder app | No, optional | Alternative editor and firmware installer; the same CLI is already in ESP Screens |
| ESPHome integration in HA | Yes, pair every screen | The connection between Home Assistant and the physical screen |

So a fresh installation without ESPHome Device Builder also works. If there's
no ESPHome `secrets.yaml` yet, our wizard asks for Wi-Fi once and
stores it locally. Existing Wi-Fi secrets are reused. API and OTA keys
are generated per new screen and stay in that device's own profile.

### Step by step

For Home Assistant Container (Docker) without the App store, follow [ESP Screens with Docker](docs/DOCKER.md).

For Home Assistant OS with Apps/Add-ons on **aarch64 or amd64**:

1. Open the App store and add this repository:
   `https://github.com/MaxGramser/homeassistant_espscreen`.
2. Install **ESP Screen Manager**, start the app, and open **ESP Screens**.
   ESPHome Device Builder is optional: the ESPHome CLI is already in this app.
3. Connect the screen with a USB data cable to the **Home Assistant machine**
   and choose **Settings → New screen**: CYD or Guition, a name, the USB port, and
   **Install**. If Wi-Fi is missing from the ESPHome `secrets.yaml`, the window
   asks for it once and ESP Screens only adds the missing lines. The profile
   with unique API and OTA keys goes into the ESPHome folder; the build
   and flash run in the same window (a first build takes a few minutes on a
   Raspberry Pi). Each screen gets its own profile.
4. **CYD:** go through the calibration on the screen. **Guition:** uses GT911
   without resistive calibration. Then pair the discovered ESPHome device in
   **Settings → Devices & services** using the API key the window
   shows after installation (copy button). Grant the device permission to
   perform Home Assistant actions.
5. Select the screen in ESP Screens, choose your tiles, and click
   **Save & send to screen**. Then test the physical controls.

<p align="center">
  <img src="docs/images/editor-new-screen.png" width="37%" alt="New screen in ESP Screens: choose the board, give it a name, pick the USB port and install">
  <img src="docs/images/editor-tiles.png" width="59%" alt="Choosing tiles: every page in the screen preview, next to the entity picker with filters">
</p>
<p align="center"><sub>New screen (step 3) and choosing your tiles (step 5).</sub></p>

You can install new firmware later from **Settings → Firmware & USB → Wi-Fi / OTA**.
For an existing screen, always use the existing profile; creating a new
installation profile generates new keys.

The [complete installation guide](docs/EASY_SETUP.md) walks through every step in more detail.

## More

- **[Full reference](README_EXTENDED.md):** every card and setting, alerts and wake/sleep from an
  automation, the top bar, the settings page on the screen, and how updates keep your settings.
- [Guition hardware, mounting, and rotation](docs/GUITION.md) ·
  [CYD calibration and USB diagnostics](docs/CALIBRATING.md) ·
  [Troubleshooting](docs/TROUBLESHOOTING.md) ·
  [Release history](screen_manager/CHANGELOG.md)

## Credits and license

The very first version started from Adrian Kuehlewind's
[ESPHome-touch-display-mount](https://github.com/akuehlewind/ESPHome-touch-display-mount).
Little of that code is left, but his repository has 3D-printable desk, under-desk, wall and flush
mounts for the CYD.

ESP Screens is MIT licensed, see [LICENSE](LICENSE).
