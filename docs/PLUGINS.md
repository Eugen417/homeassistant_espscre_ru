# Plugins

A plugin adds something to the screens a person chooses, without the core knowing that plugin exists: a tile with data
from a web service, hardware on one board, a feature not everyone needs. How to make one is in the plugins repository,
[github.com/MaxGramser/tessera-plugins](https://github.com/MaxGramser/tessera-plugins) (its `docs/` and `AGENTS.md`).
This page is the core's side: what the firmware, the add-on and the editor do, and the rules a change here keeps.

Plugins are on dev while the plugin API is 0.x (0.2 now): in an app added from the `#dev` URL, a local copy of the app, and the
editor's development server (`core.plugins_enabled()`). The stable app has no Plugins page, no routes and no loop.

## The pieces

| Where | File | What |
|---|---|---|
| Firmware | `components/smart_display/plugin_api.h` | What a plugin uses: `tessera::Plugin`, `tessera::Tile`, `TileContext`, the drawing helpers, the clock. The API's version, `PLUGIN_API_MAJOR/MINOR`. |
| Firmware | `components/smart_display/plugin_host.h`, `plugin_host.cpp` | The core's side: the register, a plugin tile in a card, the moments, the hello. |
| Firmware | `components/smart_display/__init__.py` | `register_plugin()`, which a plugin's `__init__.py` calls: id, version, tile memory and screen texts from its manifest. `PLUGIN_API`. |
| Add-on | `screen_manager/app/plugin_manifest.py` | Reads and checks a manifest. Standalone: the plugins repository's `tools/check.py` runs this file. `PLUGIN_API`. |
| Add-on | `screen_manager/app/plugin_fetch.py` | The building block `fetch`: fill in, ask, map, cache, back off. |
| Add-on | `screen_manager/app/plugin_store.py` | `/data/plugins.json` (per screen) and `/data/plugin_secrets.json` (0600). |
| Add-on | `screen_manager/app/plugins.py` | The index and test folders, the editor's payload, the plugins file, adding and removing, a plugin tile's state message, the fetch loop. |
| Add-on | `firmware.py` (`PLUGINS_SUFFIX`, `save_plugins`), `page_layout.py`, `core.py` (`plugin_tile`, `PLUGIN_MEMORY`), `page_delivery.py` (`plugins_of`) | The sidecar, the document, the layout check and price, the negotiation. |
| Editor | `web/src/model/plugins.ts`, `web/src/plugin-state.ts`, `web/src/components/Plugin*.vue`, `ScreenPluginsTab.vue` | The Plugins page, the screen's Plugins tab, a plugin tile in the library and the inspector. |

## A plugin tile, end to end

1. **The document** stores `{"kind": "plugin", "plugin": "ov_departures", "tile": "next", "options": {...}}`. Flat, as
   the protocol and the layout check see it, that is `entity: "plugin:ov_departures.next"` with `options.plugin`.
   `core.entity_id()` is unchanged (commands and events use it); `core.plugin_tile()` is the second kind a layout takes.
2. **The layout check** keeps only the shape of a plugin tile: its size, colour, icon, `tap` (auto or none) and at most
   twelve options of its own, each a short text, a number or true/false. A saved layout never becomes unreadable
   because a plugin left a screen.
3. **The price** of a plugin tile is its manifest's `memory` (`core.PLUGIN_MEMORY`, filled by `plugins.Plugins`), plus
   the tile and its extras on a board without PSRAM. The screen counts the same (`plugin_host::bytes`, from the same
   manifest through `register_plugin`). A tile of a plugin the app does not know costs `PLUGIN_PLACEHOLDER_BYTES` (64),
   as on the screen.
4. **Negotiation**: the screen's hello says `plugin_api` and `plugins: [{id, version, tiles}]`. A layout with a plugin
   tile goes only to a screen that says `plugin_api`; one without gets the refusal "update the firmware". No firmware
   number gates it, so dev screens work before a release numbers them.
5. **The state message** of a plugin tile (`Plugins.tile_message`) has `state: "ok"`, no attributes, `o.plugin` (its
   options with the manifest's defaults) and `x`, the mapped answer of its fetch, kept under 2.6 KB.
6. **On the screen** `page_receiver` keeps `o.plugin` and `x` as JSON in the tile's `Extra` (`plugin_options`,
   `plugin_state`). `render_slot` hands a plugin tile the card's extra layer (`plugin_host::render`): a new
   `tessera::Tile` when the card shows another tile, size or options, `on_state` when the data changed, `on_theme` when
   the look did. The once-a-second gate in `tick()` calls `on_tick` for every plugin card on the glass, and a short tap
   goes to `on_tap` through the same guard as every tile's. `end_extra` and `release_kept` delete the object.
7. **In the editor** a plugin tile is drawn from its data when its manifest has a `preview`: the add-on fills in the
   first rows (`GET api/plugins/<id>/preview/<tile>`, `Plugins.preview`) and the mockup counts down to a moment on the
   editor's clock, so a page in the editor looks like the glass. Without a preview it shows the icon, the name and the
   manifest's `example`. The editor loads the plugins as soon as they are on, so a page with a plugin tile knows its
   type before the Plugins page was opened.
8. **A tile whose plugin this screen lacks** is a plain card: its icon, its name and "Plugin missing"
   (`screen.plugin.missing`). Never an error, never a restart.

## A plugin on a screen

- `<name>.plugins.yaml` beside the screen's YAML holds its plugins; only the app writes it. The screen's YAML attaches it
  once under `packages:` as `tessera_plugins: !include <name>.plugins.yaml`, the way Override YAML attaches
  `<name>.local.yaml`, and with the same rules (`_ensure_include`: the file first, then the line; another include under
  that key is refused).
- A plugin from the index is a remote package and a git external component, both pinned to the commit the index names,
  `refresh: never`. A plugin from a test folder (`tessera-plugins/<id>/` beside the ESPHome folder) is an `!include` and
  a local external component, by a path relative to the ESPHome folder, so the app, Device Builder and a shared folder
  build the same.
- Adding or removing writes `plugins.json` and the plugins file, and puts the screen in the queue for its own build and
  update (`firmware.start`, action install). The queue builds one screen at a time, in the order asked, each when the
  app's build slot is free, so ticking three screens on the Plugins page builds them one after the other. A screen
  without a profile in the app gets the file and the line to paste.
- Removing a screen removes its plugins and, when no screen uses a plugin any more, its secrets.

## The fetch loop

`Plugins.loop` runs every 15 s while some layout has a plugin tile: it asks what is due for every plugin tile
(`Fetcher.get`, one ask per distinct URL and headers), and marks the tiles whose answer changed dirty, so the next sync
sends them. The rules (named hosts only, public addresses only, checked again at connect time by `PublicResolver`, no
redirects, 64 KB, 10 s, JSON, every 30 s at most, secrets never logged) are in `plugin_fetch.py` and the plugins
repository's `docs/FETCH.md`.

## Rules a change keeps

- **The core never names a plugin.** A hook a plugin needs goes into `plugin_api.h` for every plugin, and raises
  `PLUGIN_API_MINOR` (`plugin_api.h`, `__init__.py`, `plugin_manifest.py`: a test keeps them equal).
- **No plugin code in the add-on or the editor.** They read the manifest and run their own building blocks.
- **A plugin tile is never an error.** Unknown plugins, missing manifests, failed fetches: a placeholder or a `wait`.
- **Secrets stay in `plugin_secrets.json`**: never in a payload to the editor, a message to a screen, a YAML file or a
  log.
- **Test against real data**: the plugins repository's `tools/check.py` uses this `plugin_manifest.py`; change both in
  step.

## What a plugin can add (plugin API 0.2)

| Part | Firmware | Add-on | Editor |
|---|---|---|---|
| A tile, with data from a fetch | `Tile`, `add_tile` | `Plugins.tile_message`, `plugin_fetch.py` | library, inspector, `preview` drawn from the data |
| A tile of an entity | `TileContext.entity`, `tessera::action` | `Plugins.entity_part` (state, name, named attributes), `related_entities` | entity picker of the manifest's domains, also ones Tessera draws no tile for |
| A card | `Card`, `add_card`, `open_card`; closed by `hide_detail` | nothing | nothing |
| A tap action | `add_tap_action`; `event()` runs a tile's `plugin:` tap | `validate_layout` takes a `plugin:` tap | the tile inspector's tap choices |
| A top bar item | `add_bar_item`; `header_bar::Kind::plugin` | `validate_header` type `plugin`, sent to a screen whose hello says `plugins` | "From plugins" in Top bar, Add |
| Settings rows | `settings(SettingsPage&)`; `settings_screen::plugin_pages` | nothing (the values are ESPHome entities of the plugin's YAML) | nothing |
| A question to Home Assistant | `tessera::send`, `on_message` (op `plugin`) | `Plugins.answer`: only `permissions.ha_commands`, logged, answer bounded | the commands under "What it may do" |
| The moments | `on_ready`, `on_tick`, `on_standby`, `before_update`, `on_cards_closed`, `on_alert` | | |

## Where a plugin comes from

- **The index** (`tessera-plugins/index.json`): Tessera's own plugins (label Tessera) and entries of `community/`
  (label Community), each pinned to a commit, read when the editor opens and at most every ten minutes (ETag).
- **A link** (`POST api/plugins/link`): the newest release of any GitHub repository, pinned to its commit (Community,
  or Tessera for a repository of MaxGramser); a branch to test, which every build takes anew (`refresh: 0s`, Test); or
  a folder in `tessera-plugins/` beside the ESPHome folder (Test).
- **An update** is a newer version in the index: the screen's Plugins tab and the Plugins page offer it, and the build
  queue takes the screens one by one. An update whose rights differ from what the person agreed to
  (`permission_hash`) waits for their yes in the editor; the add-on refuses it without (`consent`).

## What is not built yet

A plugin's own messages beyond Home Assistant commands, pictures of a plugin's own, Python modules of Tessera's own
plugins in the add-on (the design's `module`), and the plugin's settings rows in the editor's Screen settings (they are
entities of the screen in Home Assistant, so an automation and Home Assistant's own device page already reach them).
