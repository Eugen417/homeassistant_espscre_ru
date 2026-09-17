## 0.2.62 (firmware 0.2.53)

On or off at a glance, in Home Assistant's words and colours.

- **A door says Open or Closed.** A binary sensor's tile and card use the words of its kind, the same ones the top bar and the history card show: a door or window Open or Closed, motion Motion or No motion, a leak sensor Wet or Dry, and so on for smoke, battery, connectivity, plugs and the rest. A sensor without a kind still says On or Off.
- **Off looks off.** A light or binary sensor that is off turns grey, as in Home Assistant, and a light without an icon of its own shows a crossed-out bulb while it is off, in the top bar too. An icon you chose stays the same and turns grey, as Home Assistant does with an icon of its own.
- **The history card's heading is right while it loads.** It said On until the history arrived, and a card opened after another one could show that card's word for a moment, such as Open on a motion sensor.
- Needs firmware 0.2.53: press **Update** on the screen. Older firmware keeps On, Off and its colours, and shows an off light in the top bar without an icon.

## 0.2.61 (firmware 0.2.52)

Your own YAML for one screen, kept through updates.

- **Override YAML** on each screen edits a small `<screen>.local.yaml` beside the profile. It is loaded after the shared board package and stays in place when the app or the firmware package updates. New screens get an empty one; an existing profile is attached on the first save, without reformatting the rest of the file.
- The editor has an example that changes the display controller with `!extend` (ESPHome appends package lists, so a bare `id:` would add a second, incomplete display), line numbers, Tab indentation and Cmd/Ctrl-S. **Save & check** runs ESPHome's full validation of the complete profile; a build never starts from an invalid one.
- The screen's name, Wi-Fi, API, OTA, packages, external components and captive portal stay managed and are refused in the override, as are the managed substitutions. Invalid YAML is refused before anything is written, and the file is 12 KB at most.
- No firmware change: firmware stays 0.2.52.

## 0.2.60 (firmware 0.2.52)

Sliders keep their colour while a speaker plays or a blind is open.

- **Coloured sliders for speakers, blinds and numbers.** Since 0.2.42 the volume slider of a playing speaker, the position slider of a blind or curtain and the slider of a number were grey, the colour of a light that is off. Sliders now follow Home Assistant's tile sliders: a speaker that plays, is paused or is idle shows its volume in blue, a cover its position in purple, also when it is closed, and a number its value in teal. A speaker that is off or in standby, and a light or fan that is off, stay grey. This goes for the sliders on double-width tiles and for the small sliders on single tiles.
- Needs firmware 0.2.52: press **Update** on the screen.

## 0.2.59 (firmware 0.2.51)

A history card you can read: axes, an hour, a day or a week, and the value under your finger.

- **History with axes.** Tapping a sensor, a number, a binary sensor or a person, or holding a switch, opens its history the way Home Assistant shows it. Numbers get a line with round values along the side and clock times along the bottom, and their highest and lowest moment as rings with their values. On/off, home and away, zones and a status like a washing machine's get a timeline with the time spent in each state. The value now stays big at the top, with the highest and lowest moment and their times beside it, or how long the state lasted and how often it began: "Open · 22 min, 6 times".
- **An hour, a day or a week.** Three keys below the graph. Every range has the same number of points, just further apart: 24 averages on a line, 96 steps on a timeline. ESP Screens fetches the range when the card opens, from Home Assistant's statistics where the entity has them.
- **Your finger reads the graph.** Hold a finger on the graph and slide: the top of the card shows the value and the time of that moment. On a timeline it shows the state, when it began and ended, and how long it lasted, so a door that stood open for five minutes says 5 min. The graph itself stays as it is, and letting go shows the value now again.
- **Every unit.** Temperatures, percentages, watts, kilowatt-hours, lux and any other unit get an axis in round steps with the decimals the entity shows in Home Assistant. Times follow the screen's 12- or 24-hour clock.
- A switch's card keeps its toggle, top right, and a number's card its slider.
- **The weather card's back button is whole again on the Guition.** Since 0.2.43 the round back button sat half under the card with the current weather.
- Needs firmware 0.2.51: press **Update** on the screen. Screens with older firmware keep their old card.

## 0.2.58 (firmware 0.2.50)

A card for blinds, curtains and garage doors, and three fixes found right after updating to 0.2.57.

- **Cover card.** Tapping a blind, curtain, shutter or garage door opens a card like Home Assistant's own, in the style of the climate and vacuum cards: a tall position slider on which the blind hangs from the top, a tilt slider over slats for venetian blinds, and open, stop and close keys. The key of the direction the cover is moving is filled, and a key that can't move it further is greyed out. A slider shows its value while you drag and sends it when you let go. A battery-powered blind, such as Motionblinds, shows its battery at the top right. The card shows only what the cover supports: a garage door that only opens and closes gets just the keys.
- **Night brightness 0 % now shows in Home Assistant.** A brightness setting whose value is 0 stayed "unknown" in Home Assistant, so ESP Screens could not change it on a screen with firmware 0.2.49.
- **No second layout sensor while a screen restarts.** During a restart ESP Screens took "unavailable" for the screen's device name and wrote `sensor.esp_screens_unavailable`. Home Assistant drops that sensor at its next restart.
- **The entity list is filled when the page opens.** When ESP Screens was busy, such as while building firmware, the page could open a screen before its entities arrived: "No entities found" until you picked a filter, and tile names showing as entity IDs.
- **No stale page after an update.** A browser that keeps old files, as Safari did, could combine the new page with the old script and show an empty Screen settings panel. The page now always loads the script and styles of the version that is running.
- Needs firmware 0.2.50 for the cover card and the brightness fix: press **Update** on the screen.

## 0.2.57 (firmware 0.2.49)

The screen keeps its own settings, and ESP Screens shows them the way the screen does.

- **A setting you change stays changed.** Until now ESP Screens kept its own copy of a screen's settings and sent it along with every full update: after a restart of the app or Home Assistant, every hour and after every save. A change it had missed, because the app or Home Assistant was restarting, came back to the old value, and holding − or + on the screen's settings page could jump back a step. With firmware 0.2.49 the screen owns its settings, and ESP Screens changes them on the screen instead of overwriting them.
- **Every setting is an entity in Home Assistant.** Besides the brightness numbers and Auto standby, each screen now has Night mode, Night starts and Night ends, 24-hour clock, Back to page 1 and after how long, Back to page 1 on standby, Swipe between pages and, on a Guition, Rotation, all under the device's configuration. The settings page, an automation and ESP Screens change the same value, and setting a value the screen already has costs nothing. The Claude skill lists them.
- **New Screen settings panel.** The form under a screen's tiles is now three cards, Brightness, Night and Screen, with the rows of the settings page on the screen: switches, − and + that repeat while you hold them, and chips for the clock and the rotation. A change applies right away, without Save, and a change made on the screen shows up while the panel is open. An offline screen shows its values as unknown until it is back.
- **A missing tile comes back sooner.** The screen now answers ESP Screens' ping directly, so a screen that still lacks its layout or a tile right after an update gets everything again after 30 seconds instead of two minutes.
- **The Settings tile works without Home Assistant.** It did nothing while the screen had no connection, exactly when you want to see "Home Assistant: Not connected" or press Restart.
- **Claude sees the screens again after Home Assistant restarts.** The `sensor.esp_screens_<screen>` layout sensors are written again right after a restart instead of after the next change to a screen.
- **No more forecast errors in the Home Assistant log.** A weather tile only asks for the forecasts its weather entity has. Buienradar has no hourly forecast, and asking for one logged an error about 48 times a day.
- Screens with older firmware work as before, with their settings kept in ESP Screens; a change made on such a screen is no longer sent back to it.
- Needs firmware 0.2.49: press **Update** on the screen.

## 0.2.56 (firmware 0.2.48)

The back button on a card works again.

- **Back works on every card.** Since firmware 0.2.44 the back arrow of the light, colour and climate cards hardly responded, and neither did the button at the top right of the climate card. The invisible strip you hold to open the settings page lay on top of the cards and caught those taps. It now lies under the cards, so holding the top bar only opens the settings page from the tile pages, as intended.
- Needs firmware 0.2.48: press **Update** on the screen. Includes everything from 0.2.55.

## 0.2.55 (firmware 0.2.47)

A colour slider no longer turns red when you let go.

- **Sliders keep the value you let go of.** On a Guition, dragging a light's colour slider and letting go could send red instead of the colour under your finger, and brightness or colour temperature could jump to an end the same way. The touch panel now and then reports a stray touch in the top-left corner just as the finger lifts, and the screen took that as where the finger was. The screen now ignores it. Should a slider still jump when you let go, the ESPHome log says `slider jumped on release`.
- **Setting a screen to what it already has costs nothing.** An automation that sets Standby after or a brightness whenever a light changes runs on every colour and brightness change too, because a state trigger without `to:` also fires on those. Each time, ESP Screens sent the whole screen again, and while the screen redrew its page it briefly stopped reading the touch panel: exactly when you were dragging that light's slider. A value the screen already has is now neither saved on the screen nor sent again by ESP Screens.
- **A repeated layout no longer redraws the page.** The hourly repeat, and a save that changed nothing for this screen, only update the tiles whose state comes in, instead of rebuilding the whole page behind an open card.
- Needs firmware 0.2.47: press **Update** on the screen.

## 0.2.54 (firmware 0.2.46)

The standby code, tidied after a review of Wake and Sleep.

- **One rule for Sleep.** A screen put to sleep with Sleep stays asleep until someone taps it, Wake is pressed or an alert comes in. Switching Auto standby off from Home Assistant used to end it, while the same switch in ESP Screens or on the screen did not; now none of them do. Switching Auto standby off still wakes a screen that its standby time dimmed.
- **Standby closes every card.** The settings page and any open card close when the screen goes into standby, and with "also on standby" on it goes back to page 1 as well. Until now a weather or media card stayed open under a dimmed screen while a light or climate card closed.
- **The backlight light is gone from Home Assistant.** `Display Backlight` (on the CYD `Power Display Backlight`) fought the screen's own brightness: the screen put its level back within a minute, and switching the light off made the screen dark without standby, so taps landed on tiles you couldn't see. Use the brightness numbers and the Wake and Sleep buttons. If Home Assistant still lists the old light as unavailable after the update, you can delete it there.
- Under the hood: standby goes through the same scripts as "back to page 1", every card closes through one call, and a leftover of the old manual profile is gone.
- Needs firmware 0.2.46: press **Update** on the screen.

## 0.2.53 (firmware 0.2.45)

Wake a screen or put it to sleep from Home Assistant.

- **Wake and Sleep buttons.** Every screen has `button.<screen>_wake` and `button.<screen>_sleep` in Home Assistant, pressed with `button.press`. Wake does what a tap does: a screen in standby lights up and the standby time counts again from that moment. Sleep puts the screen in standby right away, just like when the standby time runs out, and also works with Auto standby off: the screen stays in standby until someone taps it, Wake is pressed or an alert comes in. An alert that is showing closes, reported as `remote`.
- **As often as you like.** Neither button saves anything on the screen, unlike the Auto standby switch, so an automation may press Wake on every motion. To reach several screens at once, list their buttons under `entity_id`; don't target an area or a device, because that presses every other button there too.
- **The Claude skill knows them.** Settings → Claude explains both buttons with an example automation. Install the skill again to get it.
- Needs firmware 0.2.45: press **Update** on the screen. Screens on older firmware keep working, without the two buttons.

## 0.2.52 (firmware 0.2.44)

Change a screen's settings on the screen itself.

- **A settings page on the glass.** Hold the top bar of the overview for about a second and a half -- a blue line fills along the top edge while you hold -- and the screen opens its own settings: Brightness, Night, Screen and This screen. Toggles flip on a tap, numbers and times have `-` and `+` (hold them and a time walks whole hours), and a choice like the clock cycles in a chip. A change is saved on the screen, takes effect at once and appears in ESP Screens within a second, so both sides always show the same value.
- **Rotation and swiping are on it too.** Everything you would want to change standing in front of the panel: brightness, standby, night mode and its hours, the 12/24-hour clock, swiping between pages and, on boards that can, the rotation. Tiles and the top bar stay in the editor, where there is a mouse.
- **This screen.** Its name, IP address, firmware version, whether Home Assistant is connected, and a Restart that asks once before it does it.
- **Back to page 1 by itself.** New setting, on by default at two minutes: a card someone opened, or a second page they left behind, closes by itself after that long without a touch. Until now that only happened when the screen went into standby, so with standby off or far away a card could stay up all day. Set the time in ESP Screens or on the screen, or switch it off there.
- **A Settings tile.** Next to the clock card there is now a `Settings` card you can put on a page, for a screen where holding the bar is not obvious. Needs firmware 0.2.44.
- **For Home Assistant.** `esphome.<screen>_open_settings` opens the page (0 menu, 1 Brightness, 2 Night, 3 Screen, 4 This screen, -1 closes it and goes back to page 1), and the Claude skill explains all of it.

## 0.2.51 (firmware 0.2.43)

Ask Claude in Home Assistant to put something on a screen.

- **Tiles from a Home Assistant event.** ESP Screens now listens for `esp_screens_add_tile`, `esp_screens_remove_tile`, `esp_screens_move_tile` and `esp_screens_order_tiles`. An event names the screen (its device name, the name Home Assistant shows, its area or its title) and the entity, and may set the size, the direct control, the display, the icon, the colour and the spot. The app changes the screen and sends it right away, with the same checks as its own editor: at most twenty tiles, an entity once per screen, and a double-width tile in the left column. A tile with a control or a forecast becomes double-width by itself. Every event gets an answer, `esp_screens_tile_result`, with `ok` or the reason it was refused; a refused event changes nothing.
- **A screen you can read.** Every screen also publishes what it shows as `sensor.esp_screens_<device name>`: the number of tiles, and per tile the entity, the page, the row, the column, the width, the control and the display. So an assistant can look before it moves something, and you can use it in a template.
- **The Claude skill knows all of it.** Settings → Claude now also covers tiles: the four events, every field, what each kind of entity can do, how to read a screen first, and the rule to show what it is about to do and wait for a yes. Install it again from the Settings page, then ask for example "put the vacuum on the living room screen", "give the living room lights a brightness slider and make that tile wide" or "order page 1 by how often I use them".
- No new firmware: screens on 0.2.43 need no update.

## 0.2.50 (firmware 0.2.43)

More memory for the CYD, so a busy moment no longer restarts it.

- **Sliders without extra buffers.** The fill of the tile and light card sliders now has the same rounded corners as its track. With the tighter corners it had since 0.2.43, the screen drew every fill into a separate buffer of up to 20 KB on each redraw, and when the CYD ran short of memory it kept retrying until its watchdog restarted it. The end of the fill is now slightly rounder behind the white handle.
- **Leaner tiles.** Climate modes, a select's options, weather, sun and timer times, the media title and the vacuum rows now only take memory on tiles that use them, and a tile keeps a short fingerprint of its last state instead of a full copy. That frees about 16 KB of RAM on the CYD.
- **The old manual setup is gone.** Before ESP Screens existed, tiles were written by hand in the screen's YAML. Nothing used that any more, and it cost a screen memory: 80 Home Assistant subscriptions, its own tap handlers and an old vacuum card it never showed. All of it is out, together with its guides and helper scripts. Choosing tiles, the top bar, the settings and the calibration work exactly as before. The unused sensor **SmartDisplay Action** disappears: Home Assistant shows it as unavailable and you can delete it.
- **No pale flash.** A tap between two tiles, or the tap that wakes a dimmed screen, lit up a pale panel. It stays transparent now, on both boards.
- **Heap Minimum Free.** A new diagnostic sensor shows the lowest free memory since the screen started.
- Firmware 0.2.43 for both boards; the app offers the update.

## 0.2.49 (firmware 0.2.42)

No more flickering stripes on the CYD.

- **CYD panel fix.** The cheap 2.8" panel showed a fine pattern of vertical lines that shimmered when you moved your eyes or the board. The screen now uses frame inversion at the panel's highest refresh rate, which removes the stripes. Colours and contrast are unchanged. Guition screens are not affected.
- Firmware 0.2.42 for both boards (version only on the Guition); the app offers the update.

## 0.2.48 (firmware 0.2.41)

Keep a screen awake from a Home Assistant automation.

- **Auto standby in Home Assistant.** Every screen gets the switch **Auto standby**, the same setting as the checkbox in ESP Screens. Turn it off and a dimmed screen wakes at once and stays on; turn it on and the screen dims again after its standby time, counted from that moment. So an automation can keep the screens on while someone is home and the lights are on or a window is open. A change shows in ESP Screens too and is kept after a restart.
- **The Claude skill knows it.** The skill under Settings → Claude now also covers standby and brightness: the Auto standby switch, Standby after and the three brightness numbers, with an example automation. Install it again from the Settings page to get the new version.
- Firmware 0.2.41 for both boards; the app offers the update.

## 0.2.47 (firmware 0.2.40)

The new climate card comes to the Guition, with fan and swing right on the card.

- **Climate card on the Guition.** The dial is gone here too: the target temperature sits big between two large − / + keys, the number follows every tap and one call goes out when you stop tapping, and holding a key keeps stepping. One row of keys picks the mode in Home Assistant's order and colours.
- **Fan and swing at a glance.** Below the mode keys a white card shows the fan speeds and the swing modes as rows of choices, each behind its icon, like the cleaning settings on the vacuum card. Tap a choice and it shows at once. The card makes room for whatever the device has: with fan and swing everything moves a little closer together, and a thermostat without them keeps a larger setpoint.
- **One mode is no choice.** A thermostat that can only heat no longer shows a single mode key; its setpoint sits in the middle of the card, on both boards. The power key still turns it on and off.
- Firmware 0.2.40 for both boards; the app offers the update. The CYD keeps fan and swing behind ···.

## 0.2.46 (firmware 0.2.39)

Choose between vacuuming, mopping or both on the vacuum card, and a climate card made for the small CYD.

- **Vacuum, vacuum and mop, or mop only.** Robots that offer a cleaning mode in Home Assistant, such as a Roborock (HA 2026.8 or newer), get a row with Vacuum, Vac & mop and Mop on the vacuum card of both screens. ESP Screens finds the mode and the mop intensity on the robot's own device, whatever language your entity IDs are in; you don't have to add them as tiles. Custom (the per-room settings from the robot's app) shows up only while the robot uses it.
- **Only what the mode uses.** Below the mode sit suction power and water as rows of choices: vacuum only shows suction, mop only shows water, both show both. A tap shows your choice at once and the card waits for Home Assistant. Speeds that belong to a mode, such as suction off for mopping, move into the mode row, and Max+ now fits next to Max.
- **A new vacuum card.** The robot, its state and battery on top, with a green bolt while it charges and the room it is in while it cleans; then Start cleaning (or Pause, Resume) and Dock; then how it cleans. On the Guition the cleaning settings share one white card, and Locate became a round button beside the robot. Since HA 2026.8 a vacuum no longer reports its battery itself, so ESP Screens now reads the battery sensor of the robot.
- **Climate card for the CYD.** No more dial to drag on the small resistive screen: the target temperature sits big between two large − / + keys. The number follows every tap at once and one call goes out when you stop tapping; holding a key keeps stepping. One row of keys picks the mode (in Home Assistant's order and colours), and ··· opens fan and swing. A tap only redraws the number or a key, so the card responds right away. The Guition keeps its dial.
- **Less redrawing after a climate change.** A card closed within three seconds of a change left a flag behind that redrew every tile each second; that flag is now cleared.
- Firmware 0.2.39 for both boards; the app offers the update. Update the app and the screens together: with firmware 0.2.39 and an older app the vacuum card falls back to suction only.

## 0.2.45 (firmware 0.2.38)

One alert for every screen, a Claude skill, and a Settings page that clears up the main page.

- **One alert for every screen.** Fire the Home Assistant event `esp_screens_show_alert` with the usual alert fields and ESP Screens shows the alert on every screen that is online, screens added later included. `esp_screens_dismiss_alert` clears it everywhere. Fields you leave out stay empty. A value that doesn't fit, such as a bare `Yes` that YAML turns into true, is left empty with a line in the log instead of losing the alert. The per-screen actions stay as they are.
- **Ask Claude.** Settings → Claude installs an ESP Screens skill for Claude Code in Home Assistant (`/homeassistant/.claude/skills/esp-screens`), or downloads it as a zip for claude.ai. Claude then knows the alert events and every field, color and icon, so "show an alert on all my screens when the mailbox is full" becomes a working automation. Nothing is written until you press the button, and the page says when a newer version of the skill is ready.
- **Settings page.** New screen, Firmware & USB, the firmware updates, Alerts and Claude moved from the header and the sidebar to a page of their own; the header keeps one Settings button. The Update button of each screen stays in My screens.
- **Clearer screen list.** Every screen in My screens sits on a light grey card; the selected screen is white.
- No new firmware: screens on firmware 0.2.31 or newer show alerts for every screen.

## 0.2.44 (firmware 0.2.38)

- **Vacuum card on the Guition.** The state line under the name is gone; the card below already shows it in its badge, and the two sat on top of each other.
- Firmware 0.2.38 for both boards; the app offers the update.

## 0.2.43 (firmware 0.2.37)

A second hand on the analog clock, sliders that look like Home Assistant's, and one top bar for every card.

- **Second hand.** The analog clock card gets a thin red second hand with a short tail that moves every second while the screen is in use. During standby (also at the night level) it is hidden and the clock keeps its once-a-minute redraw, so a sleeping screen does no extra work. Only the hand's own line is redrawn, not the card. The small dial in the top bar stays as it was.
- **Sliders like Home Assistant.** Card sliders (a light's or fan's direct control, the Sonos volume, the brightness row of the colour card) now use the proportions of HA's control slider: softer corners on track and fill, a slimmer white handle a little in from the end of the fill, a track in the fill colour at 20 %, and a short stub instead of a full circle at 0 or 1 %. A light or fan that is off shows only the empty grey track, without fill or handle, exactly as HA does; the colour card's brightness row then reads Off until you move it.
- **A light's slider stops at 1 %.** As in HA, dragging a light's slider all the way down leaves it on at 1 %; tapping the card turns it off.
- **One top bar on every card.** Every card that opens over the tiles (brightness, colour, climate, vacuum, media, timer, switch, sensor history and the others) has the same bar: a round back arrow at the left, big enough for a finger (60 px on the Guition, 40 px on the CYD), and the name in the middle. Only a card with one overriding action keeps a round button at the right, such as the on/off of a climate device; the colour and vacuum cards lose their extra "done" buttons, which only closed the card. The brightness card, which had no button at all, gets the back arrow too.
- **A starting screen says so.** Until the first layout arrives the top bar reads "Connecting to Home Assistant..." and then "Waiting for ESP Screens...", instead of "Choose tiles in HA".
- Firmware 0.2.37 for both boards; the app offers the update.

## 0.2.42 (firmware 0.2.36)

Smoother page swipes and a new colour card for lights.

- **Page swipes that keep up with your finger.** The next page shows up at once as empty cards and fills in two cards at a time, top to bottom, while the screen keeps reading touch in between, so a quick second swipe is no longer lost. On the Guition the longest pause during a swipe went from about 280 ms to about 75 ms, and the page is complete in about a quarter of a second. Swiping past the first or last page no longer redraws the page.
- **Less redrawing overall.** A state change redraws only its own card, and clock cards redraw once a minute instead of every second.
- **Slider handles sit inside the fill**, like in Home Assistant, also at 1 %. A light that is off shows a grey slider end.
- **The busy sheet covers the whole card**, including the controls of a wide card.
- **New colour card for lights.** Colour, colour temperature and brightness each get their own card with a rounded track: a knob in the chosen colour for colour and temperature, and a filled slider with the handle inside for brightness. Lights that only do colour temperature, or only colour, show just their cards. The knob no longer gets cut off at the ends of the track.
- Firmware 0.2.36 for both boards; the app offers the update.

## 0.2.41 (firmware 0.2.35)

See ESP Screens before you install it. What the screens show and what you can set stay the same.

- **Screenshots in the README.** Photos of a Guition at home, both boards with demo layouts (pages, direct controls, the weather, climate, light and vacuum cards, sensor history, an alert) and the ESP Screens page itself: the tiles, tile settings, top bar, Alerts cheatsheet and New screen. The screen images are rendered from the firmware's own LVGL code on a demo home, so they match what the screens draw. The app's page in the App store shows a photo and the editor too.
- Firmware 0.2.35 for both boards, so you can try an update from the app. It only changes the version number.

## 0.2.40 (firmware 0.2.34)

ESP Screens is now in English: the screens, the management page, the Home Assistant entities and the documentation.

- **English everywhere.** Every label on both boards (tiles, detail cards, the climate and vacuum cards, page navigation, calibration), the ESP Screens editor, the Alerts cheatsheet, error messages and logs, and the README and guides. The top bar writes numbers the English way (21.5 °C, 1,249 W). State labels follow Home Assistant's own English names, for example Heat and Cool for climate modes, Heating and Cooling for what the device is doing, Away, Returning to dock, and weather conditions such as Lightning, rainy. Dates use English abbreviations (Mo 14 Sep).
- **English entity names.** Firmware 0.2.34 names its entities Tile settings, Screen firmware, Device name, IP address, Guition screen type, Last boot, Normal brightness, Standby brightness, Night brightness, Standby after and Calibrate touch. Home Assistant registers a renamed ESPHome entity under a new entity id and removes the old one, so `text.kitchen_screen_tegelinstellingen` becomes `text.kitchen_screen_tile_settings`. Dashboards or automations that use one of these diagnostic entities need the new id.
- **Your layouts move along.** ESP Screen Manager recognises a screen that comes back under a new inbox id and moves its tiles, settings, top bar and update history to it, also when the app restarts in between. The **Update** button and the nightly round follow the screen through the rename and report the update as successful.
- **Older firmware keeps working.** Screens on firmware 0.2.33 or older still report the Dutch entity names and status messages; the app recognises both until they are updated.
- Docs use Home Assistant's current menu names (**Settings → Apps**, **App store**, **Settings → Tools**), and the guides are now `docs/TILES.md`, `docs/CALIBRATING.md`, `docs/ACCEPTANCE.md` and `docs/TROUBLESHOOTING.md`.
- Firmware 0.2.34 for both boards; the app offers the update. Besides the English text and entity names it changes nothing on the screen.

## 0.2.39 (firmware 0.2.33)

Zuiniger in dagelijks gebruik, ook met tientallen schermen. Niets verandert aan wat je ziet of instelt.

- **Alleen doorrekenen wat veranderde.** Bij een statuswijziging bouwt de app alleen de tegels (en de bovenbalk) opnieuw waar die entiteit op staat; de andere berichten blijven zoals ze verstuurd zijn. De schermlijst wordt niet meer bij elke wijziging uit het hele Home Assistant-register opgebouwd, maar bewaard tot het register of een schermdiagnose verandert. De live-stream van de beheerpagina volgt dezelfde regel.
- **Historie gebundeld, buiten de lus.** De 24 staafjes van sensortegels komen uit één `recorder/statistics_during_period`-vraag voor alle sensoren tegelijk (uurgemiddelden voor 24 uur, 5-minuutgemiddelden voor 1 en 6 uur), in een achtergrondtaak die elke vijf minuten ververst. Sensoren zonder statistiek (geen state class) houden de oude REST-historie, ook op de achtergrond. De verzendlus wacht nergens meer op de database; een gewijzigde grafiek stuurt alleen die tegel.
- **Keepalive als ping.** Schermen met firmware 0.2.33 krijgen elke twee minuten alleen een klein bericht met de revisie van hun indeling. Komt die niet overeen (herstart, demo-indeling, verloren bericht), dan meldt het scherm `Indeling opnieuw nodig` en stuurt de app alles opnieuw; als vangnet gaat eens per uur toch alles. De inbox-status wisselt niet meer per ronde tussen "Indeling ontvangen" en "Gesynchroniseerd". Oudere firmware houdt de volledige herhaling per twee minuten.
- **Eén actie per bericht.** Firmware 0.2.33 heeft de actie `esphome.<apparaatnaam>_screen_message` die een heel bericht in één keer aanneemt; de app gebruikt die zodra het scherm die firmware meldt. De base64-blokjes van 200 tekens in de tekstentiteit blijven de terugval voor oudere firmware.
- **Rustige diagnostiek.** De diagnostische sensor `Uptime` (elke 15 s een nieuwe waarde) is vervangen door `Opgestart`, een tijdstempel met één waarde per opstart; Home Assistant ruimt de oude entiteit zelf op. De heap-sensoren melden per vijf minuten in plaats van per 30 s, en de vier instellingsgetallen (helderheid, standby) publiceren alleen nog een gewijzigde waarde. Dat scheelt tot honderdduizenden recorder-rijen per dag bij dertig schermen.
- Firmware 0.2.33 voor beide borden; de app biedt de update aan. Alles werkt ook met de vorige firmware, alleen zonder ping en actie.

## 0.2.38 (firmware 0.2.32)

- **Bovenbalk per scherm.** Bovenaan elk scherm staat links de naam en rechts wat jij kiest, tot zes onderdelen: de **tijd**, een **analoge klok**, de **datum**, of een **entiteit met icoon** uit Home Assistant, zoals temperatuur, luchtvochtigheid, verbruik, een deur of raam (open/dicht), het alarmpaneel, een slot, wie er thuis is (`zone.home`) of een telefoon (`device_tracker`). Zonder aanpassing blijft het zoals het was: naam en tijd.
- **Status of "Laatst gewijzigd".** Een onderdeel toont de status zoals Home Assistant hem schrijft (21,3 °C, 65%, 1.249 W, Open/Dicht, Thuis/Weg, Afwezig, de volgende zonsondergang), of hoe lang geleden het veranderde: "Zojuist", "5 min geleden", "Gisteren". Een tijdstempel-sensor telt ook vooruit ("Over 2 uur"). Het scherm telt de minuten zelf, zonder verkeer.
- **Alleen als actief.** Laat een onderdeel alleen verschijnen als het aan, open, thuis of meer dan 0 is, bijvoorbeeld een open deur of een draaiende wasmachine. Actieve onderdelen kleuren zoals in Home Assistant: open deur amber, alarm ingeschakeld groen, alarm afgegaan of slot open rood, iemand thuis groen.
- **Iconen.** Automatisch zoals Home Assistant (een open deur krijgt een open deur-icoon, een temperatuursensor een thermometer), een eigen icoon uit de lijst, of geen icoon.
- **Netjes op het oog.** Alle waarden, de tijd ook, staan in dezelfde letter op de basislijn van de naam; iconen en de analoge klok staan gecentreerd op de cijferhoogte en even groot; de ruimte tussen icoon en waarde wordt gemeten tussen wat je ziet, dus voor elk icoon gelijk. Past niet alles, dan krijgt de naam puntjes en valt het voorste onderdeel weg.
- **Editor.** Nieuw blok **Bovenbalk** met de naam en de onderdelen als kaartjes: slepen om te ordenen, tikken om in te stellen, **＋ Toevoegen** met de tijd, klok en datum, suggesties uit je eigen huis (temperatuur en verbruik uit de ruimte van het scherm, weer, aantal thuis, zon op en onder) en een zoekveld voor elke entiteit. Elke pagina in het schermvoorbeeld toont de balk met Roboto en dezelfde plaatsingsregels als het scherm; onderdelen die niet passen of nu verborgen zijn, zijn gemarkeerd.
- De knoppenbalk **Tegels instellen / Algemene instellingen / Inspector** is weg; de secties staan gewoon onder elkaar. **Klok tonen** is verhuisd naar de bovenbalk (de 24-uursklok blijft bij de scherminstellingen en staat ook bij de klok in de balk).
- Firmware 0.2.32 voor beide borden; de app biedt de update aan. Oudere firmware toont tot de update de naam en, als die in de balk staat, de tijd.
- Voor ontwikkelaars: `tools/render_topbar.py` rendert de echte LVGL-code van de bovenbalk op je computer (ESPHome host + SDL2) naar PNG, voor beide borden, zonder scherm.

## 0.2.37 (firmware 0.2.31)

- **Cheatsheet Alerts in ESP Screens.** De knop **Alerts** bovenaan opent een naslagpagina met alles over `show_alert`: per scherm de exacte actienaam (`esphome.<apparaatnaam>_show_alert`) met kopieerknop en of de firmware het al kan, een kant-en-klaar YAML-voorbeeld per scherm, alle zeven velden met uitleg, voorbeelden en de tekstlimieten per bord, alle iconen met glyph en naam (zoeken, tikken kopieert de naam), de kleuren als stalen, het gedrag (timeout, knop, knipperen, standby, vervangen) en het event `esphome.screen_alert` met een voorbeeldautomatisering die op de knop wacht. Home Assistant toont bij ESPHome-acties zelf geen uitleg of keuzelijsten; deze pagina vult dat gat.
- Geen nieuwe firmware; de doelversie blijft 0.2.31.

## 0.2.36 (firmware 0.2.31)

- **Alert vanuit een automatisering.** Elk scherm heeft nu de actie `esphome.<scherm>_show_alert` met `title`, `subtitle`, `icon`, `color`, `button_text`, `timeout` en `flash`. De kaart ligt op LVGL's toplaag over alles heen (pagina's, kaarten, de standby-laag), wekt het scherm en houdt de backlight op de normale helderheid tot iemand op de knop tikt (`button_text`, standaard Oké), of tot `timeout` seconden (0 is tot de knop; de knop sluit ook een alert met timeout direct). `flash: true` laat de backlight vier keer knipperen bij binnenkomst. Iconen zijn de namen uit de tegelkiezer (ook als `mdi:naam`, of als hex-codepoint van een meegecompileerd glyph); onbekend wordt `alert-outline`. Kleuren zijn de pasteltinten van de tegels; leeg is de witte kaart. Een nieuwe alert vervangt de huidige. Oké, timeout, vervangen en `dismiss_alert` melden zich als event `esphome.screen_alert` met `action`, `title` en `screen`.
- De actie `esphome.<scherm>_dismiss_alert` haalt een alert op afstand weg, bijvoorbeeld als de deur al open is.
- De iconenlijst schrijft nu ook `components/smart_display/tile_icon_names.h` (naam naar codepoint) via `tools/generate_icons.py`.
- Firmware 0.2.31 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.35 (firmware 0.2.30)

- **Snelle randveeg pakt nu ook in één keer (Guition).** Op 0.2.29 wisselde de veeg wel, maar ongeveer de helft van de snelle vegen eindigde met `randveeg niet gevuurd: 5 px naar binnen` terwijl de vinger duidelijk verder ging; elke aanraking duurde in de firmware bijna exact 160 ms en leverde maar één meting op. Een druk in de lege rand landde op de achtergrondcontainer en de pagina, en het thema geeft elk object een ingedrukte-stijl (45% dekking): bijna het hele scherm werd bij indrukken en loslaten opnieuw getekend, wat de touch-polling precies zo lang blokkeerde als een flick duurt. De pagina en de tegelcontainer nemen nu geen drukken meer aan (`LV_OBJ_FLAG_CLICKABLE` uit), dus geen hertekening en de metingen komen elke 20 ms door.
- Zolang een randveeg gewapend is, logt de firmware elke meting (`veeg id=0 st=2 x=.. y=..`) en de eerste druk meldt zijn contact-id, zodat de bemonstering uit het log te lezen is.
- Firmware 0.2.30 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.34 (firmware 0.2.29)

- **Randveeg werkte in 0.2.32 en 0.2.33 helemaal niet, hersteld.** De veeg was daar op de pointer-events van LVGL gezet, maar de LVGL 9.5.0 die ESPHome meelevert stuurt het invoerapparaat wel PRESSED en RELEASED maar geen PRESSING, dus de veeg kreeg nooit positie-updates (live gezien op Studio 1: wel `GT911 press`, verder niets). De Guition volgt de veeg weer via ESPHome's touchscreen-triggers, zoals in 0.2.24 (dat werkte live zodra de kaartfix uit 0.2.32 erbij zat), met de eigen rotatiemapping gelijk aan die van ESPHome's LVGL-component, de 45°-regel, `end()` bij loslaten (een niet-afgemaakte veeg kan daardoor niet meer bij de volgende aanraking afgaan) en de logregels `randveeg genegeerd: …` en `randveeg niet gevuurd: …`.
- Firmware 0.2.29 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.33 (firmware 0.2.28)

- **Guition: loslaten binnen de tegel telt.** De verplaatsingsgrens voor een tik staat op de Guition uit (`TOUCH_MOVE_LIMIT_PX: "0"`); LVGL beslist, zoals iOS: laat je los binnen de tegel waarop je begon, dan is het een tik, verlaat je vinger de tegel, dan niet. Een stevige druk die een centimeter verschoof (73 px op Studio 1) werd anders nog geweigerd. De randveeg neemt zijn eigen tik weg en sliders vangen hun eigen sleep, dus de grens had daar geen taak meer. De CYD houdt zijn 56 px, omdat het resistieve paneel kan springen.
- Firmware 0.2.28 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.32 (firmware 0.2.27)

- **Randveeg die nooit pakte, opgelost.** Live meegelezen op een Guition: de veeg werd herkend maar geblokkeerd door de voorwaarde "kaart open". De lichtkaart, klimaatkaart en stofzuigerkaart wisten bij sluiten (kruisje, tik naast de kaart) en bij wakker worden uit standby de interne "actieve kaart" niet, dus na één keer een kaart openen bleef vegen stil geblokkeerd tot de indeling veranderde. Dat gold ook voor de oude veeg over het hele scherm en voor de CYD. Beide borden sluiten kaarten nu via één script (`close_cards`) dat alles verbergt én die toestand wist; wakker worden gebruikt hetzelfde pad.
- **Randveeg op LVGL's eigen manier.** De Guition volgt de veeg nu via de pointer-events van LVGL 9 (`lv_indev_add_event_cb`, PRESSED/PRESSING) in plaats van ESPHome's touchscreen-`on_update`: de coördinaten komen al gedraaid van LVGL (geen eigen rotatiemapping meer), en `lv_indev_wait_release` wordt vanuit LVGL zelf aangeroepen. De richtingseis is versoepeld van "twee keer zo horizontaal" naar "meer zijwaarts dan verticaal" (45°), zodat een schuine duimveeg vanaf rechts ook in één keer pakt.
- **Log zegt waarom.** Een herkende maar geblokkeerde randveeg meldt de reden (`randveeg genegeerd: kaart open`), en een randveeg die zonder paginawissel eindigt meldt hoe ver de vinger kwam (`randveeg niet gevuurd: 28 px naar binnen, 35 px verticaal`), naast het bestaande `randveeg: pagina 0 -> 1`.
- Firmware 0.2.27 voor beide borden; de app biedt de update aan. Alleen de doelversie verandert in de app.

## 0.2.31 (firmware 0.2.26)

- **Vrij slepen, met lege plekken**: de editor werkt met vaste plekken in plaats van een volgorde die steeds wordt aangeschoven. Elke tegel heeft zijn eigen plek (kolom, rij, pagina) en die verandert alleen als jij hem versleept. Lege plekken zijn gewoon lege plekken: naast een enkele tegel, midden op een pagina, waar je wilt; ze blijven staan zodat je kunt ordenen. Sleep een tegel op een lege plek en hij staat daar. Sleep hem op een andere tegel en die twee wisselen: de ander neemt de plek die vrijkwam, of anders de dichtstbijzijnde vrije plek. Alle overige tegels blijven waar ze staan. Tijdens het slepen laat het voorbeeld al zien waar alles terechtkomt; loslaten bevestigt precies dat, loslaten buiten het voorbeeld verandert niets.
- **Naar een volgende pagina slepen**, ook als de vorige nog niet vol is: onder de laatste pagina staat tijdens het slepen een lege pagina klaar. Met **Pagina toevoegen** maak je zelf een lege pagina, die ook bewaard blijft; een lege pagina heeft **Pagina weghalen** (de pagina's erna schuiven op). Maximaal acht pagina's.
- **Lege plek aanklikken**: klik op een lege plek en de volgende tegel uit de kiezer komt daar ("Volgende tegel komt hier"); zonder keuze vult toevoegen de eerste vrije plek. Een tegel die dubbelbreed wordt houdt zijn rij als de plek ernaast vrij is en gaat anders naar de dichtstbijzijnde vrije rij; de buurtegel blijft staan. Dubbelbreed terug naar normaal laat de rechterplek leeg.
- **Geen tekstselectie meer tijdens het slepen**, en een sleep die snel begint start toch (de muis blijft aan de kaart gekoppeld tot de sleep loopt). Pijltjestoetsen verplaatsen een gefocuste tegel per plek; Enter opent de instellingen.
- Firmware 0.2.26 tekent de tegels op precies die plekken: lege plekken blijven leeg en een lege pagina blijft een pagina. Oudere firmware negeert de nieuwe velden en schuift de tegels aan tot de eerste vrije plek, zoals voorheen; de editor meldt dat onder het voorbeeld zolang zo'n scherm een indeling met gaten heeft. Bestaande indelingen krijgen bij het laden de plekken die ze al hadden, er verandert niets op het scherm.
- Opslag en protocol (voor ontwikkelaars): `tiles[].slot` (0–47, absolute plek: pagina × 6 + rij × 2 + kolom; dubbelbreed altijd op een even plek) en `pages` (1–8) zijn additief in opslagversie 1; het layoutbericht krijgt `slots` en `pages`. Firmware 0.2.26 voor beide borden; de app biedt de update aan.

## 0.2.30 (firmware 0.2.25)

- **Groen bolletje voor online schermen**: in de lijst met schermen is het bolletje voor **Online** nu groen, zodat je in één oogopslag ziet welke schermen bereikbaar zijn. **Offline** blijft een grijs open rondje.
- Firmware 0.2.25 voor beide borden; de app biedt de update aan. Inhoudelijk verandert er in de firmware niets, alleen het versienummer.

## 0.2.29 (firmware 0.2.24)

- **Vegen vanaf de zijrand op de Guition**: met **Vegen tussen pagina's** aan wissel je van pagina door vanaf de linker- of rechterrand naar binnen te vegen, zoals terug-vegen op een telefoon. Vanaf rechts naar links is volgende, vanaf links naar rechts vorige. Langzaam of snel maakt niet uit: de veeg begint in een band van 32 px (circa 5 mm) langs de rand en telt na 40 px (circa 6 mm) naar binnen, duidelijk meer zijwaarts dan omhoog of omlaag (`EDGE_SWIPE_BAND_PX`, `EDGE_SWIPE_TRAVEL_PX`). Werkt in alle vier de rotaties. Een veeg die midden op het scherm begint doet niets meer, zodat tikken en slepen op tegels nooit per ongeluk van pagina wisselen; de tegel onder een randveeg krijgt geen tik. Het log meldt `randveeg: pagina 0 -> 1`.
- De CYD houdt de bestaande snelle veeg over het scherm (LVGL-gesture, firmware 0.2.7+); daar verandert niets, de firmware krijgt alleen het nieuwe versienummer.
- Firmware 0.2.24 voor beide borden; de app biedt de update aan. In de app is alleen de omschrijving van de instelling aangepast.

## 0.2.28 (firmware 0.2.23)

- **Tikken die niet doorkwamen**: de firmware gooide een tik weg zodra de vinger tijdens het drukken meer dan 18 px (nog geen 3 mm) van het eerste contactpunt afweek, ver onder LVGL's eigen veegdrempel en ook met "Vegen tussen pagina's" uit. Een vinger die platter wordt of iets rolt haalde dat al. Nu geldt per bord een grens van ongeveer één centimeter (`TOUCH_MOVE_LIMIT_PX`: Guition 67 px, CYD 56 px), gemeten als afstand tot een referentiepunt dat over de eerste vier metingen (circa 60 ms) settelt in plaats van tot het allereerste punt. Eindigt de vinger op een andere tegel, dan vangt LVGL dat nog steeds op (press lost).
- Alleen het contact dat de aanraking begon telt: een tweede vinger of een duim aan de rand geldt niet meer als verplaatsing (`touch.id` van de touchscreen-component).
- De minimale contactduur is per bord (`TOUCH_MIN_PRESS_MS`): 60 ms op de resistieve CYD (contactdender), 20 ms op de capacitieve Guition, waar een lichte snelle tik anders verloren ging.
- Elke geweigerde tik staat nu in het ESPHome-log (tag `touch`, niveau INFO) met de reden: `verplaatst 73 px (grens 67)`, `te kort (12 ms, minimaal 20)`, `al verwerkt in dit contact` of `dezelfde knop binnen de dendertijd`, zodat een gemiste aanraking uit het log te lezen is.
- Firmware 0.2.23 voor beide borden; de app biedt de update aan. Alleen firmware verandert; de app-kant is ongewijzigd behalve de doelversie.

## 0.2.27 (firmware 0.2.22 blijft actueel)

- **Nieuw scherm in één venster**: bord, naam, USB-poort, **Installeren**. Het venster maakt het profiel met unieke API- en OTA-sleutels in de ESPHome-map, bouwt de firmware en schrijft die via USB, met het ESPHome-log en de fase (bouwen, schrijven) in hetzelfde venster. Na afloop staat de API-sleutel klaar met een kopieerknop plus de koppelstappen voor Home Assistant; mislukt de build, dan staat het log open en is er **Opnieuw proberen**. De apparaatnaam volgt uit de naam (`Keuken` → `keuken`) en is aan te passen. Zonder aangesloten scherm kun je alleen het profiel bewaren; het staat dan in dezelfde map als ESPHome Device Builder. Elk scherm krijgt zijn eigen profiel; het venster zegt dat nu ook.
- **Waar is mijn scherm?** De koppeling gebeurt in Home Assistant zelf, buiten ESP Screens. Het klaar-scherm van het venster zegt dat nu expliciet, met een knop **Open Apparaten & diensten** die je daarheen brengt. Zolang een ESP Screens-profiel nog niet in Home Assistant staat, toont **Mijn schermen** er een kaart voor ("geïnstalleerd, maar nog niet in Home Assistant", of "nog niet geflasht"), met dezelfde knop, **Kopieer API-sleutel** en de herinnering om bij Configureren "Allow the device to perform Home Assistant actions" aan te zetten (anders ziet het scherm alles, maar bedient het niets). De kaart verdwijnt zodra het scherm in de lijst staat. `/api/inventory` levert daarvoor `pending` (profielen die het bordpakket van dit project gebruiken zonder gekoppeld scherm).
- **Wifi zonder handwerk**: ontbreken `wifi_ssid` of `wifi_password` in de ESPHome `secrets.yaml`, of bestaat het bestand niet, dan vraagt het venster ze en zet ESP Screens alleen de ontbrekende regels in het bestand; commentaar en andere secrets blijven staan (de wizard weigerde eerder bij een bestaand bestand zonder wifi). Een ongeldig `secrets.yaml` wordt nog steeds niet aangeraakt.
- Kopiëren en downloaden van de installatie-YAML is vervallen (`POST /api/install` bestaat niet meer): het profiel staat al in de ESPHome-map. `POST /api/firmware/profiles` accepteert `target` (USB-poort) en start dan direct de build en flash; poort en vrije bouwslot worden gecontroleerd vóór er iets wordt geschreven, en het antwoord bevat `api_key`. Het firmwarejob-object meldt de lopende fase in `stage`.
- Alleen de app verandert; firmware 0.2.22 blijft actueel.

## 0.2.26 (firmware 0.2.22)

- **"HA niet verbonden" om de twee minuten opgelost**: sinds 0.2.20 herhaalt de app de indeling elke 120 s, maar de firmware verwachtte binnen 95 s een bericht (gemaakt voor de oude 25 s). Op een rustig scherm stonden alle tegels daardoor 25 tot 45 s per ronde op "Niet beschikbaar". De firmware gebruikt nu ESPHome's eigen API-verbindingsstatus voor "HA niet verbonden" (direct bij wegvallen en terugkeren van Home Assistant) en bewaakt de gegevensstroom apart met het interval dat de app zelf declareert: het layoutbericht bevat `keepalive` (seconden). Pas na twee gemiste rondes plus marge meldt het scherm "ESP Screens niet actief". Zonder het veld (oudere app) rekent de firmware met 120 s. Firmware 0.2.22 voor beide borden; firmware tot 0.2.21 negeert het veld en houdt zijn 95 s.

## 0.2.25 (firmware 0.2.21)

- **− / + in één beweging**: snel drie keer tikken telt drie stappen (van 20 naar 17); de touch-guard hield een tweede tik op dezelfde knop binnen 600 ms tegen. De − / + knoppen gebruiken nu een eigen korte guard (150 ms, alleen tegen stuiteren) en stappen ook door zolang je ze vasthoudt (drie per seconde). Na 700 ms rust gaat nog steeds één opdracht naar Home Assistant.

## 0.2.24 (firmware 0.2.20)

- **Lichter en zuiniger**: de dag- en uurvoorspelling zitten alleen nog in het geheugen van weertegels (was 832 bytes in elk van de twintig tegels, ruim 16 KB op de CYD zonder PSRAM). De mini-slider onderin een tegel (**Kleine slider**) heeft nu dezelfde zichtbare witte greep als de nieuwe schuiven. De licht-, ventilator- en zonweringkaart op de CYD gebruiken het lichte palet van de Guition (de laatste rest van het donkere thema).
- **Wifi zonder modem-slaap** (`power_save_mode: none`) in de installatie-YAML van de wizard en de bordprofielen: statusupdates, tikacties en OTA wachten niet meer op een wifi-beacon. Bestaande schermen: voeg de regel zelf toe onder `wifi:` in je ESPHome-YAML.
- Doorlichting van de rendering: LVGL 9.5 tekent partieel en slaat ongewijzigde posities en maten al over; de winst zat in de stijlzetters (0.2.23) en in niet meer herrenderen tijdens *bezig*. Een paginawissel tekent bewust twee keer (skelet, dan inhoud). Verder geen structurele last gevonden; zie docs/TEST_RESULTS_0224.md.

## 0.2.23 (firmware 0.2.19)

- **Directe bediening op dubbelbrede tegels**, zoals de entiteitsrijen in Home Assistant: rechts op de kaart staan knoppen of een schuif, links blijven icoon, naam en status. Per domein kies je in het tegelpaneel onder **Directe bediening op de tegel** welke set de tegel toont:
  - klimaat: **Temperatuur − / +** (de gewenste temperatuur in een pill; tikken past direct aan, na een korte pauze gaat één opdracht naar HA) of **Uit, verwarmen, koelen** (maximaal drie modusknoppen uit `hvac_modes`, de actieve gekleurd);
  - schakelaar, input_boolean, lamp en ventilator: **Aan/uit-schakelaar** (toggle die direct omklapt); lamp ook **Helderheidsschuif**, ventilator ook **Snelheidsschuif**;
  - stofzuiger: **Start, stop, naar dock** (start wordt pauze tijdens het schoonmaken; dock en stop grijs als ze niet kunnen);
  - zonwering: **Open, stop, dicht** (pijlen horizontaal voor gordijnen, deuren, poorten en zonneschermen; open/dicht grijs aan het eind van de slag; stop alleen als het apparaat het kan) of **Positieschuif**;
  - mediaspeler: **Volume en dempen** (schuif met witte greep en dempknop) of **Vorige, play/pauze, volgende**;
  - getal: **Waarde − / +** of **Schuif**; keuzelijst: **Vorige / volgende keuze**; kookwekker: **Start/pauze en annuleren**; scène, script en knop: één knop **Activeren**, **Uitvoeren** of **Indrukken**.
- Zonder keuze toont een dubbelbrede tegel van zo'n domein de eerste set; **Geen** houdt de gewone kaart. Enkele tegels veranderen niet. De statusregel volgt HA: `Koelen · 21.5°`, `Open · 80%`, `TV · 17%`.
- De mockup in de editor toont de gekozen bediening in het klein; de Inspector meldt de gekozen set. De add-on stuurt alleen de werkelijk getoonde set mee (`o.controls`), plus `device_class`, `hvac_action` en `is_volume_muted`. Firmware 0.2.18 en ouder negeert het veld; het paneel meldt dat.
- Firmware 0.2.19: het paneel wordt alleen opgebouwd voor tegels die het tonen en per set hergebruikt; de render-selftest controleert dat knoppen binnen de kaart en rechts van de tekst blijven. De drie icoonfonts bevatten 18 extra bedieningsglyphs (CYD +3 KB, Guition +5 KB) en het middenpunt `·` in de tekstfonts.
- **Bezig-status als spinner**: een tegel die op Home Assistant wacht krijgt een licht-witte laag met een klein draaiend cirkeltje in plaats van de blauwe voortgangsbalk en de tekst "Bezig...". De laag vangt tikken op zolang de opdracht loopt.
- **Weerkaart** (bediening openen op een weertegel), in twee kaarten: *nu* met groot icoon, temperatuur, omschrijving en een gedempte regel met gevoelstemperatuur, luchtvochtigheid en wind, daaronder de komende zes uren (tijd, icoon, temperatuur, regenkans in blauw); en *Komende dagen* met per dag icoon, omschrijving, regen met druppel (kans en millimeters) en de hoogste temperatuur vet naast de laagste gedempt. De add-on haalt naast de dagvoorspelling nu ook de uurvoorspelling op (`weather.get_forecasts` type `hourly`, elk half uur ververst) en stuurt `x.hours` plus regen (`p` kans, `r` mm) per dag en uur mee. Zonder uurvoorspelling blijft de kaart bij de dagen.
- **Klimaat**: het vinkje rechtsboven in de klimaatkaart is een **aan/uit-knop** geworden (`climate.turn_on`/`turn_off`, licht op als het apparaat draait); het kruisje sluit nog steeds. De moduskiezer toont naast de HVAC-modi ook de **ventilatorstanden** en **zwenkstanden** van het apparaat (`fan_modes`, `swing_modes`, maximaal vier per rij; Nederlands waar bekend). Bij aantikken van een klimaattegel kun je nu ook **Aan / uit** kiezen (`climate.toggle`).
- **Scènes, scripts en knoppen** tonen niet langer "Uit" maar wanneer ze voor het laatst liepen: `Laatst 14:32`, `Gisteren 14:32` of `Laatst 13 sep`, en `Bezig...` zolang een script draait; `Nog niet gestart` als het nog nooit liep. De add-on stuurt daarvoor `x.last` (unix-tijd uit `last_triggered` of de tijdstempel-status).
- **Moduskiezer van de klimaatkaart** opnieuw ingedeeld: titel *Modus*, de kiezer op het schermgrijs met witte chips met randje (contrast), Nederlandse HVAC-chips drie per rij, de actieve chip in het accentblauw met witte tekst, daaronder de rijen *Ventilator* en *Zwenken* met de standen zoals het apparaat ze meldt (alleen een hoofdletter, geen vertaaltabel: werkt met elk apparaat). Het overbodige vinkje is weg; een chip past de stand toe en sluit, het kruisje sluit. *Nu:* en *Gewenst* vervangen de Engelse labels in de kaart.
- **Lichter tekenen**: de firmware zet lettertypen, uitlijning, padding en lijndikte alleen nog als ze echt veranderen (elke `lv_obj_set_style_*` maakt anders het hele object ongeldig), en herrendert tijdens *bezig* niet meer elke 250 ms de hele pagina. Een tikkende klokkaart of een draaiende spinner tekent zo alleen nog zichzelf. Het donkere thema is uit de runtime-code verwijderd; het lichte palet is het enige palet.
- Getallen boven een miljoen gaan niet mee als weergavewaarde, behalve `supported_features` (een bitveld; mediaspelers zitten boven de 8 miljoen), anders hadden Sonos-tegels geen volume- of playbackknoppen.

## 0.2.22 (firmware 0.2.18 blijft actueel)

- De eventstream stuurt het eerste event direct bij openen in plaats van na 3 s. Gemeten op een Home Assistant Yellow na 0.2.21: `/api/inventory` 50 ms (was 7,2 s), lichte poll 2,6 KB, CPU in rust 0,5% van één core (was 8,7%). Alleen de app verandert.

## 0.2.21 (firmware 0.2.18 blijft actueel)

- **Live updates zonder pollen**: de pagina luistert op `/api/events` (server-sent events). De add-on stuurt schermstatus en updatestatus zodra de synchronisatielus of een opslag iets verandert, en controleert elke 3 s op wijzigingen tijdens een firmware-update. Zolang de stream open is pollt de pagina niet meer; valt de stream weg, dan neemt de poll van 10 s het over. De volledige lijst met entiteiten, achtergronden en iconen wordt nog elke 5 minuten ververst.
- Het log meldt bij het wegvallen van de Home Assistant-verbinding hoe lang die stond, de websocket close-code en de fout, ook wanneer de verbinding zonder foutmelding sluit. Het add-on-log op de Yellow toonde herhaalde "Home Assistant verbonden"-regels zonder waarschuwing ervoor; met deze regels is de oorzaak bij de volgende keer uit het log te halen.
- Alleen de app verandert; firmware 0.2.18 blijft actueel.

## 0.2.20 (firmware 0.2.18 blijft actueel)

- **Minder verbruik in rust**: de synchronisatielus wordt alleen nog wakker voor entiteiten die op een indeling staan of bij een scherm horen (Tegelinstellingen, Schermfirmware, Apparaatnaam, IP-adres, Guition schermtype), niet meer bij elke `state_changed` in Home Assistant.
- De entity-, device- en area-registry (circa 1 MB JSON) wordt niet meer elke 30 s opgehaald, maar bij een `*_registry_updated`-event van Home Assistant (1 s gedebounced) en als vangnet elke 10 minuten.
- De volledige keepalive naar de schermen loopt elke 2 minuten in plaats van elke 25 s. Een scherm dat offline en weer online komt krijgt de volledige indeling direct, zoals voorheen.
- **Lichte poll**: de pagina haalt elke 10 s alleen nog schermen en updatestatus op (`/api/inventory?light=1`, enkele KB) en de volledige lijst met entiteiten, achtergronden en iconen bij openen, bij terugkeer naar het tabblad en elke 5 minuten.
- Alleen de app verandert; firmware 0.2.18 blijft actueel.

## 0.2.19 (firmware 0.2.18 blijft actueel)

- **Sneller overzicht**: `/api/inventory` las bij elke aanroep alle ESPHome-profielen opnieuw in (tot vier keer per verzoek), waardoor de add-on op een Pi seconden per verzoek blokkeerde. Profielen worden nu per bestand gecachet op inode, wijzigingstijd en grootte; alleen een gewijzigd profiel wordt opnieuw gelezen en verwijderde profielen verdwijnen direct. Het parsen zelf gebruikt libyaml wanneer die beschikbaar is (circa tien keer sneller).
- Het overzicht en de updatestatus lezen de profiellijst en de HA-inventaris nog één keer per verzoek; de synchronisatielus rekent de inventaris één keer per ronde uit in plaats van per scherm.
- De pagina pollt niet meer in een verborgen tabblad en ververst direct zodra het tabblad weer zichtbaar is.
- Alleen de app verandert; firmware 0.2.18 blijft actueel.

## 0.2.18 (firmware 0.2.18)

- **Eigen icoon per tegel**: in het tegelpaneel staat onder de naam het veld **Icoon**. Kies uit 150 iconen in twaalf groepen (verlichting, ruimtes, klimaat, weer, media en muziek, beveiliging, apparaten, energie, zonwering, tuin en huisdieren, mensen en onderweg, overig), met zoeken op Nederlandse naam, Engelse MDI-naam of groep. De mockup en de kop van het paneel tonen het gekozen icoon direct.
- **Automatisch** gebruikt het icoon dat je in Home Assistant aan de entiteit gaf (`mdi:…`), als het in de set zit; anders het standaardicoon zoals voorheen. De mockup toont nu overal het icoon dat het scherm werkelijk tekent.
- Firmware 0.2.18 bevat alle 150 iconen in de drie icoonfonts van beide borden (CYD +22 KB, Guition +36 KB). Oudere firmware negeert de keuze en houdt het standaardicoon; het paneel meldt dat een update nodig is.
- De weersvoorspelling blijft het icoon van het actuele weer tonen; klok, voorspelling en zonnebaan hebben geen icoonkeuze.

## 0.2.17 (firmware 0.2.17 blijft actueel)

- Het vinkje **Elke nacht automatisch bijwerken** kreeg de algemene invoerstijl (100% breed met padding), waardoor de tekst buiten de zijkolom onder het editorpaneel viel. Het checkboxje heeft nu een vaste maat. Alleen de app verandert.

## 0.2.16 (firmware 0.2.17)

- **Bijwerken met één knop**: een scherm met oudere firmware krijgt in de lijst een badge *Update 0.2.17* en een knop **Bijwerken**. De app bouwt het eigen profiel, installeert draadloos en toont een spinner tot het scherm terug is met de nieuwe versie en een minuut stabiel blijft. Het resultaat blijft een dag zichtbaar bij het scherm.
- **Elke nacht automatisch bijwerken**: vinkje onder *Firmware-updates*. Tussen 03:00 en 06:00 (tijdzone van HA) werkt de app schermen met oudere firmware één voor één bij, met twee minuten pauze ertussen. Mislukt een scherm, dan stopt de ronde en verschijnt een melding in Home Assistant; de overige schermen blijven op hun oude firmware.
- **Alle schermen bijwerken** doet dezelfde ronde direct, bijvoorbeeld na een app-update.
- Nieuwe firmware is er zodra deze app een nieuwe versie heeft: de app kent de bijbehorende firmwareversie en vergelijkt die met `Schermfirmware` per scherm.
- Firmware 0.2.17 meldt twee diagnostische sensors extra: **Apparaatnaam** (de ESPHome-naam, gelijk aan het YAML-profiel) en **IP-adres**. Daarmee vindt de app zelf het profiel en het OTA-adres. Een scherm met oudere firmware wordt op apparaatnaam aan een profiel gekoppeld en vraagt eenmalig het IP-adres.

## 0.2.15 (firmware 0.2.16)

- **Achtergrond: Geen** als extra keuze in het tegelpalet: de kaart en de rand vallen weg en de tegelinhoud staat direct op de schermachtergrond, in dezelfde maat en op dezelfde plek als mét kaart. Werkt voor elke tegel; de mockup toont zo'n tegel met een stippellijn. Vereist schermfirmware 0.2.16; de app bewaart de indeling en meldt het als het scherm ouder is.
- **Analoge klok**: streepjes als index met de cijfers 12, 3, 6 en 9 (de CYD houdt alleen streepjes). Op een enkele tegel staat naast de wijzerplaat een kalenderblok — weekdag, grote dag, maand (CYD: "13 sep"). Dubbelbreed blijft de digitale tijd met datum naast de wijzerplaat.

## 0.2.14 (firmware 0.2.15 blijft actueel)

- Tegelinstellingen openen in een paneel bóven de schermmockup (op mobiel een sheet onderaan): naam, weergave, breedte, tikgedrag, slider, geschiedenis en kleur als knoppen, direct zichtbaar in de mockup erachter. Geen springende pagina meer.
- Verwijderen kan direct in de mockup met het kruisje op een tegel; de melding onderaan heeft **Ongedaan maken**.
- De tegellijst onder de mockup is vervallen; ordenen doe je door te slepen.

## 0.2.13 (firmware 0.2.15)

- Editor: sleep tegels in de schermmockup om te ordenen en sleep entiteiten rechtstreeks uit de lijst naar een plek in de mockup — met muis én touch (even vasthouden). De opslaan-balk blijft altijd in beeld.
- Nieuwe kop met duidelijke acties (**Nieuw scherm**, **Firmware & USB**, **Uitleg**) en een korte uitleg in drie stappen; de firmwaredialoog legt profiel, doel en knoppen uit.
- **Zonnebaan**: `sun.sun` toont dubbelbreed een horizon met de zon op zijn huidige positie tussen opkomst en ondergang (’s nachts onder de horizon), met beide tijden. Nieuwe zon-, weer- en kloktegels starten meteen dubbelbreed in hun mooiste weergave.
- Grafiek: vloeiende curve door dezelfde 24 punten met een zachte vulling eronder, zoals de HA-trendkaart.
- Scherm: de paginering is één geïntegreerde onderbalk — links tikken is vorige, rechts is volgende, het paginanummer staat in het midden.
- Scherm: bij een paginawissel verschijnen direct de kaartkaders van de nieuwe pagina (skeleton) en vult de inhoud een fractie later; geen oude waarden meer in nieuwe kaders.
- CYD: hetzelfde lichte kleurenschema als de Guition — lichtgrijze achtergrond, witte kaarten met een fijne rand, donkere tekst en lichtblauwe accenten, ook in de bedieningskaarten.

## 0.2.12 (firmware 0.2.14)

- Nieuwe kaarten in de kiezer: **Klok** (digitaal of analoog, ingebouwd), **weersvoorspelling** met vijf dagen op een dubbelbrede weerkaart, **grafiek** van de sensorgeschiedenis in de tegel, **zon** (`sun.sun`, opkomst en ondergang in je eigen tijdzone), **kookwekker** (`timer.*`, live aftellen; tikken start of pauzeert, lang indrukken annuleert) en **aanwezigheid** (`person.*`).
- **Dubbelbreed** als breedte-optie voor elke tegel. Een dubbelbrede tegel begint links en telt voor twee vakjes; het schermvoorbeeld toont ook de lege plek ervoor.
- De weerkaart toont het icoon van de actuele weersituatie. Voorspellingen komen van `weather.get_forecasts` en worden elk half uur ververst.
- Nieuwe domeinen en de klok vereisen schermfirmware 0.2.14; de app weigert opslaan voor oudere firmware en bewaart je bestaande indeling. Breedte en weergave-opties negeert oudere firmware gewoon.

## Firmware 0.2.13 (app 0.2.11 blijft bruikbaar)

- Grote waarde: klein domeinicoon naast de titel, titel en waarde verticaal gecentreerd, groter cijferfont (38 px Guition, 22 px CYD). Een te lange waarde krijgt puntjes; de eenheid staat altijd rechts naast het getal.
- Guition: de backlight dimt naar standby via de LEDC-hardwarefader in 1,5 s en wordt in 80 ms wakker. Schermherteken onderbreekt de overgang niet meer, dus geen schokkerig uitfaden.
- Installeer de nieuwe schermfirmware via ESP Screens; geen appupdate of gewijzigde configuratie nodig.

## Firmware 0.2.12 (app 0.2.11 blijft bruikbaar)

- Switches herkennen nu een aan/uit-terugmelding met ongewijzigde attributen. Daarmee verdwijnt de onterechte wachttijd van zes seconden. Na HA-bevestiging geldt voor switches slechts 150 ms minimale feedback.
- Uitgeschakelde switches hebben een grijs icoon en een grijze icoonachtergrond. Een zelfgekozen pastel tegelachtergrond blijft behouden.
- Mini-sliders behouden een compact domeinicoon naast titel en waarde. CYD centreert iconen en tekst verticaal; de renderdiagnose controleert centrering en vrije ruimte boven de slider.
- Lang indrukken op een switch of input_boolean opent een grote native LVGL-schakelaar met de echte HA-status, ook op CYD. Openen verstuurt geen opdracht.
- Installeer de nieuwe schermfirmware via ESP Screens; geen appupdate of gewijzigde configuratie nodig.

## 0.2.11

- Compact raster voor de pastelkleuren in de tegeleditor; voorkomt dat algemene formulierstijlen het palet in een lange kolom zetten. Alleen de app verandert; schermfirmware 0.2.10 blijft actueel.

## 0.2.10

- Kies per tegel een pastel achtergrond: rood, oranje, geel, groen, mint, blauw, paars, roze of grijs. Standaard herstelt de normale kleuren. De keuze is zichtbaar in het schermvoorbeeld en werkt met donkere tekst op Guition én CYD.
- Tegelkleuren blijven behouden bij appupdates en bij opslaan vanuit een oudere beheerpagina. Installeer firmware 0.2.10 op het scherm om de kleuren weer te geven.
- README en Easy Setup beschrijven de huidige één-app-installatie, twintig tegels, inspector, rotatie en updates via de ingebouwde ESPHome-CLI.

## 0.2.9

- Guition: kies 0°, 90°, 180° of 270° onder Scherminstellingen. Na opslaan draait de interface direct mee, inclusief touch. De hoek blijft bewaard na herstart; een nieuwe flash per hoek is niet nodig.
- Installeer eerst Guition-firmware 0.2.9 voor deze optie. De CYD behoudt zijn vaste oriëntatie en kalibratie.

## 0.2.8

- Firmware: vaste tekstbreedtes voor nette afkapping met puntjes. Ondertitels gebruiken de beschikbare ruimte, ook bij korte titels.
- Firmware: op de CYD staat de mini-slider onder beide tekstregels, met minder verticale padding. Zonder icoon krijgen de tekstregels de volledige kaartbreedte.
- De renderdiagnose controleert nu ook de daadwerkelijke tekst- en slidercoördinaten. Installeer de schermfirmware om deze verbeteringen te gebruiken; bestaande tegels, instellingen en sleutels blijven behouden.

## 0.2.7

- Tot twintig tegels op vier vaste pagina’s; installeer eerst firmware 0.2.7. Zes tegelvakken worden hergebruikt om geheugen te sparen.
- Optionele LVGL-swipes tussen pagina’s via scherminstellingen. Sliders en detailmenu’s wisselen niet van pagina; na een swipe wordt de aanraking geconsumeerd.

## 0.2.6

- Nieuw scherm hergebruikt bestaande ESPHome-wifi-secrets automatisch. De wizard vraagt alleen wifi als secrets.yaml nog niet bestaat. Ontbrekende wifi-sleutels in een bestaand bestand worden gemeld zonder het bestand te overschrijven.

## 0.2.5

- Firmware: een vacuumkaart openen toont de echte status, geen opdrachtmelding. Het statuslabel in de robotkaart blijft ook na een actie actueel.

## 0.2.4

- Visuele tegelkiezer met domeiniconen, zachte kleuren en een klikbaar schermvoorbeeld van zes tegels per pagina. Selecteer een tegel om de instellingen te openen; sleep tegels in het voorbeeld om te ordenen. Geen firmware-update nodig.

## 0.2.3

- Firmware: iets donkerdere Guition-achtergrond, zachte domeinkleuren op iconen en de echte HA-lampkleur in iconen/mini-sliders. Aangepaste Lovelace-thema- en kaartkleuren worden niet automatisch geïmporteerd.

## 0.2.2

- Firmware: terug naar pagina 1 bij standby sluit nu ook runtime-detailkaarten, waaronder vacuum, historie en media. Installeer hiervoor de nieuwe schermfirmware via Firmware & USB.

## 0.2.1

- Algemene instellingen en Inspector direct bovenaan bereikbaar.
- Tegel selecteren: klikactie, grote waarde, kleine slider (ja/nee) en historieperiode.
- Inspectie per tegel met actuele HA-status en eigenschappen.
- Firmware: schuifgebaren correct verwerken, brede mini-slider zonder handvat, concrete historie-as.
- Nieuwe vacuumkaart met robotweergave, actieve zuigkracht en druk-/opdrachtfeedback.

# 0.2.0

- ESPHome CLI in de app: bestaande profielen controleren, bouwen en via USB/OTA installeren.
- Nieuwe profielen met unieke sleutels; bestaande YAML, secrets en appdata blijven behouden.
- Tegelopties: klikgedrag, grote waarde, mini-schuif en sensorgeschiedenis.
- Media-, weather-, number- en select-kaarten en vernieuwde vacuumkaart op beide borden.
- Actiefeedback, vaste tekstregels en zwarte sliderknoppen.
- Inspector en helderheid/standby als instellingen bij het HA-apparaat.
- Native Guition ST7701S-configuratie en lichte kaartstijl. Duuracceptatie van het fysieke beeld blijft apart van firmwaretests.

# 0.1.2

- Per scherm standbyduur, normale/standby/nachthelderheid en nachturen instellen.
- Klok aan/uit, 12/24 uur en terug naar pagina 1 na standby.
- Instellingen direct doorsturen; firmware 0.1.2 bewaart ze in preferences.
- Bestaande tegels en sleutels blijven behouden. Eenmalig firmware bijwerken.
- De Guition-displaydriver is ongewijzigd; dit is geen oplossing voor paneelstrepen.

# 0.1.1

- Duidelijke foutmelding bij een ongeldige naam, te veel tegels of dubbele entiteiten.
- Updatepad met behoud van bestaande schermindelingen.

# 0.1.0

- Eerste ESP Screen Manager met Home Assistant Ingress.
- Zoek op entiteit, apparaat of ruimte; tien geordende tegels per scherm.
- Tegelwijzigingen en actuele waarden zonder firmwareflash.
- Installatie-YAML met unieke sleutels voor CYD en Guition.
- Permanente indelingen, automatisch herstel na HA-/schermherstart.
