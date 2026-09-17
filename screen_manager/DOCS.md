# Installation and everyday use

See [the full installation guide](https://github.com/MaxGramser/homeassistant_espscreen/blob/main/docs/EASY_SETUP.md).

Open **Settings → New screen** to install a screen: connect it via USB to the
Home Assistant machine, choose the board, name and USB port, then click **Install**.
The profile with unique keys goes into the ESPHome folder of the HA configuration,
any missing Wi-Fi goes into `secrets.yaml` (existing secrets are left untouched), and the
build and flash run in the same window; it then shows the API key
for pairing. Is the Home Assistant machine out of reach of the screen, such as a server or a
virtual machine? Choose **Download** under **Install via**: ESP Screens builds the firmware and
you put it on the screen from your own computer with [ESPHome Web](https://web.esphome.io) in
Chrome or Edge. Pairing itself happens in Home Assistant under **Settings → Devices & services** (a button in the window and on the *not yet in Home Assistant* card under My screens). Each screen has its own profile. **Settings → Firmware & USB** is
for existing profiles: check, build or reinstall via USB, the IP address (OTA) or
Download. Existing ESPHome profiles in the HA config folder are
found automatically.

**Settings** (top right) also has the nightly firmware updates, the **Alerts** cheatsheet
(an alert on one screen, or on every screen with the `esp_screens_show_alert` event)
and **Claude**: install the ESP Screens skill for Claude Code in Home Assistant, or
download it for claude.ai, and ask Claude for the alert automation.

At the top of the editor you set the **Top bar** for each screen: the name on the left, up
to six items on the right (time, analog clock, date, or an entity with an icon, such as
temperature, a door, the alarm, or "last changed"). Drag to reorder, tap
to configure; firmware 0.2.32 or newer renders them.

After pairing via the HA ESPHome integration, choose the tiles in ESP Screens.
**Tile settings** has click behavior, larger values and mini-sliders.
**Screen settings** has brightness and standby. **Inspector** helps with
missing attributes or an offline screen. New cards require firmware 0.2.0.
Each screen also has **Override YAML** for a persistent, hardware-specific
`<screen>.local.yaml` file. It is loaded after the shared package and does not
replace the managed name, credentials, Wi-Fi, OTA or package connection.

The screen carries the same settings itself (firmware 0.2.44 or newer): hold the top bar for
about a second and a half, or put the built-in **Settings** card on a page. What is changed
there appears here within a second, and the other way around.

In Home Assistant every screen also has a **Wake** and a **Sleep** button (firmware 0.2.45 or
newer). Press them from an automation with `button.press`: Wake lights the screen up and restarts the
standby time, Sleep puts the screen in standby right away, also with Auto standby off. Wake is not a
touch, so an open card or a later page still goes back to page 1 on time (firmware 0.2.56 or newer).
