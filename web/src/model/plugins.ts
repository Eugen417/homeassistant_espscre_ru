// Plugins (design, docs: the plugins proposal): what the index says about a plugin, and whether it fits a screen.
// Pure functions only, so the store page and its tests share one rule. The index itself comes from the add-on once it
// serves one (tessera-plugins/index.json); until then the page shows EXAMPLE_INDEX and says that it does.
import { editorLanguage } from "../i18n";
import type { Screen } from "../types";

// A plugin's own words per language, "en" always present. In its repo they live in translations/<language>.json, in two
// parts like Tessera's own files: `screen` (built into the firmware in the screen's language) and `app` (what the editor
// shows); its README is README.md with README.<language>.md beside it. The add-on hands the editor this per-language form.
export type Texts = Record<string, string>;
export type PluginInput = { id: string; kind: "secret" | "text" | "gpio"; label: Texts; hint?: Texts; scope: "all" | "screen" };
export type PluginPart = { id: string; label: Texts; hint: Texts; flash_kb: number; default: boolean };
// A tile type of a plugin (docs: the plugins proposal, "Een plugin-tegel"): its sizes as the catalogue names them, its
// price in the screen's memory, the entity it belongs to if any, and the options the inspector draws. An option's
// `options_from` names a fetch of the plugin, whose answer the add-on hands the inspector as a list of choices.
export type PluginTileOption = {
  id: string; kind: "text" | "choice" | "number" | "toggle"; label: Texts; hint?: Texts;
  default?: string | number | boolean; choices?: { value: string; label: Texts }[]; options_from?: string;
  min?: number; max?: number; step?: number; unit?: string;
};
export type PluginTile = {
  id: string; name: Texts; icon?: string; min: string; max: string; memory: number;
  entity?: string[]; options?: PluginTileOption[]; example?: Texts;
  // The add-on draws a preview of it from its data (api/plugins/<id>/preview/<tile>): the manifest names one.
  preview?: boolean;
};
export type PluginSource = "index" | "link" | "branch" | "folder";
export type PluginLabel = "tessera" | "community" | "test";
export type PluginKind = "hardware" | "behaviour";
export type Plugin = {
  id: string;
  name: Texts;
  summary: Texts;
  description: Texts;
  icon: string;                       // a Material Design Icons codepoint the editor's font holds (the add-on resolves the name)
  maintainer: string;
  tessera: boolean;                   // a plugin Tessera ships and reviews itself
  version: string;
  repo: string;
  license: string;
  kind: PluginKind;
  boards: string[] | "any";
  board_names?: string[];             // how a person knows those boards; the add-on fills it from boards.json
  requires: { firmware?: string; psram?: boolean; free_gpio?: number };
  flash_kb: number;
  permissions: { home_assistant: string[]; network: string[]; read_entities?: string[] };
  readme: Texts;                      // markdown; the app shows it in the editor's language, else in English
  languages: string[];                // the languages its own texts are complete in
  inputs?: PluginInput[];             // what a person fills in when adding it: a key, a pin, a name
  parts?: PluginPart[];               // optional parts, on or off per screen, each with its own room
  attributes: string[];               // cloud, commercial, ai-developed, experimental
  // From the add-on (plugins.py): where it comes from (the index, or a folder someone is making it in), its label, the
  // reason it is blocked, whether this app's plugin API takes it, and its privacy statement.
  source?: PluginSource;
  label?: PluginLabel;
  blocked?: string | null;
  fits_api?: boolean;
  privacy?: string;
  // Its cards and its tap actions for tiles of Home Assistant's own (the add-on's payload; docs/PLUGINS.md).
  cards?: { id: string; name: Texts }[];
  tap_actions?: { id: string; label: Texts; domains: string[] }[];
  bar_items?: { id: string; label: Texts; icon: string; example?: Texts | null }[];
  adds: {
    tiles?: PluginTile[];
    tap_actions?: { label: Texts; domains: string[] }[];
    card?: boolean;
    settings?: boolean;
    ha_package?: boolean;
    inputs?: boolean;
  };
};
// A plugin on a screen, as the add-on keeps it (plugins.json): its source and commit, its parts and what was filled in
// (never a secret), and its state: building, active, or failed with the reason.
export type Installed = {
  id: string; version: string; source: PluginSource; ref?: string | null; parts?: string[]; values?: Record<string, string>;
  state?: "building" | "active" | "failed" | "removing"; reason?: string | null;
};

// ---- Plugin tiles in a layout: the tile's entity is plugin:<plugin>.<tile>, the type the screen's protocol carries ----
export const PLUGIN_TILE = /^plugin:([a-z0-9_]+)\.([a-z0-9_]+)$/;
export const pluginTileId = (plugin: string, tile: string) => `plugin:${plugin}.${tile}`;
export const isPluginTile = (entity: string) => PLUGIN_TILE.test(entity);
// The tile types of the plugins the editor knows, kept by the plugin state as its index changes, so the layout model,
// the memory price and the tile card can ask without depending on it.
const tileTypes = new Map<string, { plugin: Plugin; tile: PluginTile }>();
export function knowTileTypes(index: Plugin[]) {
  tileTypes.clear();
  for (const plugin of index) for (const tile of plugin.adds.tiles || []) tileTypes.set(pluginTileId(plugin.id, tile.id), { plugin, tile });
  barTypes.clear();
  for (const plugin of index) for (const bar of plugin.bar_items || []) barTypes.set(`plugin:${plugin.id}.${bar.id}`, { plugin, ...bar });
}
export const pluginTileOf = (entity: string) => tileTypes.get(entity);
// A plugin's top bar item by its key (plugin:<plugin>.<item>), from the same index.
const barTypes = new Map<string, { plugin: Plugin; label: Texts; icon: string; example?: Texts | null }>();
export function barItemOf(key: string | undefined) {
  const known = key ? barTypes.get(key) : undefined;
  return known ? { label: text(known.label), icon: known.icon, example: known.example ? text(known.example) : "", plugin: text(known.plugin.name) } : null;
}
// The choices an option takes from a fetch. Example answers until the add-on runs the plugin's fetches.
export const EXAMPLE_FETCH: Record<string, { value: string; label: Texts }[]> = {
  "bus.lines": [
    { value: "12", label: { en: "12 Central Station", nl: "12 Centraal Station" } },
    { value: "15", label: { en: "15 University", nl: "15 Universiteit" } },
    { value: "N4", label: { en: "N4 Night bus", nl: "N4 Nachtbus" } },
  ],
};
// The key of one list of choices: plugin, fetch, and the other options it is asked with (plugin-state keeps the lists).
export const choiceKey = (plugin: Plugin, option: PluginTileOption, values: Record<string, unknown>) =>
  `${plugin.id}.${option.options_from}.${JSON.stringify(Object.entries(values).filter(([k]) => k !== option.id).sort())}`;
// The example answers, while the page shows the example index.
export const choicesOf = (plugin: Plugin, option: PluginTileOption) =>
  option.choices || (option.options_from ? EXAMPLE_FETCH[`${plugin.id}.${option.options_from}`] || [] : []);
// What a plugin tile's options are when nothing is chosen yet.
export const pluginDefaults = (tile: PluginTile) =>
  Object.fromEntries((tile.options || []).filter((option) => option.default !== undefined).map((option) => [option.id, option.default!]));

// The words in the editor's language, else English.
const own = (texts: Texts) => texts[editorLanguage()] ?? texts[editorLanguage().split("-")[0]];
export const text = (texts: Texts) => own(texts) ?? texts.en ?? "";
// Whether these words exist in the editor's language, or the page falls back to English and says so.
export const inEditorLanguage = (texts: Texts) => own(texts) !== undefined;

// ---- Does it fit this screen ----
// The reasons a plugin is not offered for a screen, in the order a person can do something about them.
export type Misfit = "board" | "psram" | "firmware" | "flash" | "pins" | "blocked";
export type Fit = { ok: true } | { ok: false; reason: Misfit };

const parts = (version: string) => version.replace(/^[^\d]*/, "").split(".").map((part) => Number.parseInt(part, 10) || 0);
export function atLeast(version: string | undefined, wanted: string) {
  if (!version) return false;
  const have = parts(version), need = parts(wanted);
  for (let i = 0; i < Math.max(have.length, need.length); i++) {
    if ((have[i] || 0) !== (need[i] || 0)) return (have[i] || 0) > (need[i] || 0);
  }
  return true;
}

// The 4 MB boards (the classic ESP32: the CYD, its ILI9342 sibling, the Hosyond) run close to the top of their slot.
// What a plugin may add there keeps the image under the 93 % line of docs/RELEASING.md: the CYD's 0.51.0 image is
// 1,853,664 B of 2,031,616 B, which leaves 34 KB below it.
// The add-on reports a screen's own last image and slot (firmware_image) once it builds plugins; until then the CYD's
// numbers stand in for every 4 MB board.
export const SMALL_FLASH = { image: 1_853_664, slot: 2_031_616, ceiling: 0.93 };
const SMALL_FLASH_BOARDS = ["cyd", "cyd9342", "hosyond40"];
const imageOf = (screen: Screen) => screen.firmware_image
  || (screen.board && SMALL_FLASH_BOARDS.includes(screen.board) ? { size: SMALL_FLASH.image, slot: SMALL_FLASH.slot } : null);
// Only a slot this full is worth a meter: on 8 MB and more a plugin of a few hundred KB fits without a thought.
const smallFlash = (screen: Screen) => { const image = imageOf(screen); return Boolean(image && image.slot <= 2_100_000); };
export const headroomKb = (screen?: Screen) => {
  const image = (screen && imageOf(screen)) || { size: SMALL_FLASH.image, slot: SMALL_FLASH.slot };
  return Math.max(0, Math.floor((image.slot * SMALL_FLASH.ceiling - image.size) / 1024));
};
// Free pins per board for a plugin with its own wiring. Example values: boards.json gets a field for them, filled from
// each board's docs (the CYD's CN1 connector, GitHub #189).
export const FREE_PINS: Record<string, string[]> = { cyd: ["GPIO22", "GPIO27"], cyd9342: ["GPIO22", "GPIO27"] };
export const freePins = (screen: Screen) => (screen.board && FREE_PINS[screen.board]) || [];

export function fit(plugin: Plugin, screen: Screen | null): Fit {
  if (!screen) return { ok: true };
  if (plugin.blocked) return { ok: false, reason: "blocked" };
  if (plugin.fits_api === false) return { ok: false, reason: "firmware" };
  if (plugin.boards !== "any" && !(screen.board && plugin.boards.includes(screen.board))) return { ok: false, reason: "board" };
  if (plugin.requires.psram && !screen.pictures) return { ok: false, reason: "psram" };
  if (plugin.requires.firmware && !atLeast(screen.firmware, plugin.requires.firmware.replace(/^>=\s*/, ""))) return { ok: false, reason: "firmware" };
  if ((plugin.requires.free_gpio || 0) > freePins(screen).length) return { ok: false, reason: "pins" };
  if (smallFlash(screen) && plugin.flash_kb > headroomKb(screen)) return { ok: false, reason: "flash" };
  return { ok: true };
}

// What its image would take of the update slot on a 4 MB board, before and after: the meter in the details.
export function flashShare(plugin: Plugin, screen: Screen | null, extra_kb = 0) {
  if (!screen || !smallFlash(screen)) return null;
  const image = imageOf(screen)!;
  return { before: image.size / image.slot, after: (image.size + (plugin.flash_kb + extra_kb) * 1024) / image.slot };
}

// ---- Example index: shown only while the plugins feature is an experiment, and marked as such on the page ----
const HEATING_README_EN = `Edit the week programme of a thermostat on the screen. The programme lives in Home Assistant's own **Schedule helpers**, so the helpers page and the screen edit the same thing.

## Set up

1. Download the package with the button below this text.
2. Put it in \`config/packages/\` of Home Assistant and restart Home Assistant. It adds a Schedule helper for every weekday and one for vacation.
3. Add the plugin to a screen.
4. On a thermostat tile, choose **Tap action → Edit schedule**, or place the **Week schedule** tile.

## Good to know

- Gaps in a day mean the heating is off.
- The vacation timeline wins over the week until you switch it off.
- A change made in Home Assistant while the screen edits is not overwritten: the screen asks again.`;
const HEATING_README_NL = `Pas het weekprogramma van een thermostaat aan op het scherm. Het programma staat in de **Schedule-helpers** van Home Assistant zelf, dus de helperpagina en het scherm passen hetzelfde aan.

## Instellen

1. Download het pakket met de knop onder deze tekst.
2. Zet het in \`config/packages/\` van Home Assistant en herstart Home Assistant. Het maakt een Schedule-helper voor elke weekdag en één voor vakantie.
3. Voeg de plugin toe aan een scherm.
4. Kies bij een thermostaattegel **Tikactie → Schema aanpassen**, of zet de tegel **Weekschema** op een pagina.

## Goed om te weten

- Een gat in een dag betekent: verwarming uit.
- Het vakantieschema gaat voor de week tot je het uitzet.
- Een wijziging in Home Assistant terwijl het scherm bewerkt, wordt niet overschreven: het scherm vraagt opnieuw.`;
const BUS_README_EN = `A tile that shows when the next bus of your line leaves, with your walk to the stop already taken off.

## Set up

1. Ask for a free key at \`example-ov.nl/developers\`.
2. Add the plugin to a screen and fill in the key. It stays in Tessera and never goes to the screen.
3. Place the **Next bus** tile on a page. In the inspector, fill in your stop code (it is on the sign at the stop) and choose your line.
4. Set **Walk to the stop** to the minutes you need. Buses you can no longer catch are skipped.

## Good to know

- Tessera asks for the departures once a minute, for all screens together.
- Works on every board; on a CYD it takes 9 KB.`;
const BUS_README_NL = `Een tegel die laat zien wanneer de volgende bus van je lijn vertrekt, met de reistijd naar de halte er al af.

## Instellen

1. Vraag een gratis sleutel aan op \`example-ov.nl/developers\`.
2. Voeg de plugin toe aan een scherm en vul de sleutel in. Hij blijft in Tessera en gaat niet naar het scherm.
3. Zet de tegel **Eerstvolgende bus** op een pagina. Vul in de inspector je haltecode in (die staat op het bordje bij de halte) en kies je lijn.
4. Stel bij **Lopen naar de halte** in hoeveel minuten je nodig hebt. Bussen die je niet meer haalt, slaat de tegel over.

## Goed om te weten

- Tessera vraagt de vertrektijden één keer per minuut op, voor alle schermen samen.
- Werkt op elk bordje; op een CYD kost hij 9 KB.`;

export const EXAMPLE_INDEX: Plugin[] = [
  {
    id: "heating_schedule", icon: "F00ED", maintainer: "Tessera", tessera: true, version: "1.1.0",
    repo: "https://github.com/MaxGramser/homeassistant_espscreen/tree/main/plugins/heating_schedule", license: "AGPL-3.0",
    kind: "behaviour", boards: "any", requires: { firmware: ">=0.50.0" }, flash_kb: 24, languages: ["en", "nl", "de", "fr", "es", "it", "pl", "pt", "hu"],
    permissions: { home_assistant: ["schedule/list", "schedule/update"], network: [] }, attributes: [],
    name: { en: "Heating schedule", nl: "Verwarmingsschema" },
    summary: { en: "Edit the week programme of a thermostat on the screen.", nl: "Pas het weekprogramma van een thermostaat aan op het scherm." },
    description: { en: "Drag the blocks of a day, set their temperature and save.", nl: "Sleep de blokken van een dag, kies hun temperatuur en bewaar." },
    readme: { en: HEATING_README_EN, nl: HEATING_README_NL },
    adds: {
      tiles: [{
        id: "week", icon: "F00ED", name: { en: "Week schedule", nl: "Weekschema" }, min: "2x1", max: "4x2", memory: 1800, entity: ["climate"],
        example: { en: "Now 20.5° · 22:30 off", nl: "Nu 20,5° · 22:30 uit" },
        options: [{ id: "days", kind: "choice", label: { en: "Shows", nl: "Toont" }, default: "today",
          choices: [{ value: "today", label: { en: "Today", nl: "Vandaag" } }, { value: "week", label: { en: "The week", nl: "De week" } }] }],
      }],
      tap_actions: [{ label: { en: "Edit schedule", nl: "Schema aanpassen" }, domains: ["climate"] }],
      card: true, settings: true, ha_package: true,
    },
  },
  {
    id: "audio", icon: "F057E", maintainer: "Tessera", tessera: true, version: "0.3.0",
    repo: "https://github.com/MaxGramser/homeassistant_espscreen/tree/main/plugins/audio", license: "AGPL-3.0",
    kind: "hardware", boards: ["wavesharep4"], board_names: ["Waveshare ESP32-P4-86-Panel"], requires: { firmware: ">=0.50.0", psram: true }, flash_kb: 60,
    languages: ["en", "nl", "de", "fr", "es", "it", "pl", "pt", "hu"],
    permissions: { home_assistant: [], network: [] }, attributes: [],
    name: { en: "Audio", nl: "Audio" },
    summary: { en: "The microphone and speaker of the Waveshare P4 86 panel.", nl: "De microfoon en luidspreker van het Waveshare P4 86-paneel." },
    description: { en: "Speaker volume, microphone mute, automatic gain and a tap sound.", nl: "Luidsprekervolume, microfoon dempen, automatische versterking en een tikgeluid." },
    readme: {
      en: "Speaker volume, microphone mute, automatic gain and a tap sound, on the screen's settings page and in the editor.\n\n## Hardware tests\n\nTurn on **Hardware tests** when you check a new panel: a tone, a five-second recording played back, and a wake word test. They take 440 KB; leave them off for everyday use.",
      nl: "Luidsprekervolume, microfoon dempen, automatische versterking en een tikgeluid, op de instellingenpagina van het scherm en in de editor.\n\n## Hardwaretests\n\nZet **Hardwaretests** aan als je een nieuw paneel controleert: een toon, een opname van vijf seconden die wordt teruggespeeld, en een wekwoordtest. Ze kosten 440 KB; laat ze uit voor elke dag.",
    },
    parts: [{ id: "tests", label: { en: "Hardware tests", nl: "Hardwaretests" }, hint: { en: "Tone, recording and wake word test", nl: "Toon, opname en wekwoordtest" }, flash_kb: 440, default: false }],
    adds: { settings: true },
  },
  {
    id: "bus", icon: "F034E", maintainer: "example-maker", tessera: false, version: "1.0.0",
    repo: "https://github.com/example-maker/tessera-bus", license: "MIT",
    kind: "behaviour", boards: "any", requires: { firmware: ">=0.50.0" }, flash_kb: 9, languages: ["en", "nl"],
    permissions: { home_assistant: [], network: ["api.example-ov.nl"] }, attributes: ["cloud"],
    name: { en: "Next bus", nl: "Eerstvolgende bus" },
    summary: { en: "When the next bus of your line leaves.", nl: "Wanneer de volgende bus van je lijn vertrekt." },
    description: { en: "A tile with the next departures of one line at your stop, counting down on the screen.", nl: "Een tegel met de volgende vertrekken van één lijn bij je halte, die op het scherm aftelt." },
    readme: { en: BUS_README_EN, nl: BUS_README_NL },
    inputs: [{ id: "api_key", kind: "secret", scope: "all", label: { en: "API key", nl: "API-sleutel" }, hint: { en: "Stays in Tessera; never sent to a screen.", nl: "Blijft in Tessera; gaat nooit naar een scherm." } }],
    adds: { tiles: [{
      id: "next_bus", icon: "F034E", name: { en: "Next bus", nl: "Eerstvolgende bus" }, min: "1x1", max: "2x2", memory: 900,
      example: { en: "12 · in 4 min", nl: "12 · over 4 min" },
      options: [
        { id: "stop", kind: "text", label: { en: "Stop code", nl: "Haltecode" }, hint: { en: "On the sign at the stop", nl: "Staat op het bordje bij de halte" } },
        { id: "line", kind: "choice", label: { en: "Line", nl: "Lijn" }, options_from: "lines" },
        { id: "walk", kind: "number", label: { en: "Walk to the stop", nl: "Lopen naar de halte" }, min: 0, max: 20, step: 1, unit: "min", default: 3 },
      ],
    }] },
  },
  {
    id: "relays_86", icon: "F1A25", maintainer: "example-maker", tessera: false, version: "0.4.1",
    repo: "https://github.com/example-maker/tessera-relays-86", license: "MIT",
    kind: "hardware", boards: ["wavesharep4"], board_names: ["Waveshare ESP32-P4-86-Panel"], requires: { firmware: ">=0.50.0" }, flash_kb: 4, languages: ["en"],
    permissions: { home_assistant: [], network: [] }, attributes: [],
    name: { en: "Panel relays" },
    summary: { en: "The two relays of an 86 panel as switches in Home Assistant." },
    description: { en: "Each relay becomes a switch of the screen in Home Assistant." },
    readme: { en: "Each relay becomes a switch of the screen in Home Assistant, so a switch tile on any screen, or an automation, turns it on and off.\n\n## Set up\n\nAdd the plugin, then name the relays under **Screen settings**. Choose what they do after a power cut: off, on, or as they were." },
    adds: { settings: true },
  },
  {
    id: "ds18b20", icon: "F050F", maintainer: "example-maker", tessera: false, version: "1.2.0",
    repo: "https://github.com/example-maker/tessera-ds18b20", license: "MIT",
    kind: "hardware", boards: "any", requires: { firmware: ">=0.50.0", free_gpio: 1 }, flash_kb: 6, languages: ["en"],
    permissions: { home_assistant: [], network: [] }, attributes: [],
    name: { en: "Temperature probe" },
    summary: { en: "A DS18B20 on a free pin, as a sensor of the screen." },
    description: { en: "Wire a DS18B20 to a free pin and choose that pin here." },
    readme: { en: "Wire a DS18B20 to a free pin of the board: data to the pin, with a 4.7 kΩ resistor to 3.3 V.\n\n## Set up\n\nChoose the pin when you add the plugin. The temperature becomes a sensor of the screen in Home Assistant, and a sensor tile shows it." },
    inputs: [
      { id: "pin", kind: "gpio", scope: "screen", label: { en: "Pin" }, hint: { en: "The pin the probe's data wire is on" } },
      { id: "name", kind: "text", scope: "screen", label: { en: "Name" }, hint: { en: "How the sensor is called in Home Assistant" } },
    ],
    adds: { inputs: true },
  },
  {
    id: "night_light", icon: "F0594", maintainer: "example-labs", tessera: false, version: "0.2.0",
    repo: "https://github.com/example-labs/tessera-night-light", license: "GPL-3.0",
    kind: "behaviour", boards: "any", requires: { firmware: ">=0.50.0" }, flash_kb: 3, languages: ["en"],
    permissions: { home_assistant: [], network: [] }, attributes: ["ai-developed"],
    name: { en: "Night light" },
    summary: { en: "A warm, dim glow on the glass with one tap at night." },
    description: { en: "A tile that turns the whole screen into a soft warm light until the next tap." },
    readme: { en: "A tile that turns the whole screen into a soft warm light until the next tap. Its colour and brightness are set in the inspector." },
    adds: { tiles: [{
      id: "glow", icon: "F0594", name: { en: "Night light" }, min: "1x1", max: "2x1", memory: 400, example: { en: "Tap for a warm glow" },
      options: [
        { id: "colour", kind: "choice", label: { en: "Colour" }, default: "warm",
          choices: [{ value: "warm", label: { en: "Warm" } }, { value: "amber", label: { en: "Amber" } }, { value: "red", label: { en: "Red" } }] },
        { id: "brightness", kind: "number", label: { en: "Brightness" }, min: 5, max: 40, step: 5, unit: "%", default: 15 },
      ],
    }], card: true },
  },
  {
    id: "camera_module", icon: "F07AE", maintainer: "example-labs", tessera: false, version: "0.1.0",
    repo: "https://github.com/example-labs/tessera-camera-module", license: "MIT",
    kind: "hardware", boards: ["esp32s3_ov2640"], board_names: ["ESP32-S3 + OV2640"], requires: { firmware: ">=0.50.0", psram: true }, flash_kb: 85, languages: ["en"],
    permissions: { home_assistant: [], network: [] }, attributes: ["experimental"],
    name: { en: "Camera module" },
    summary: { en: "An OV2640 on an ESP32-S3 board, with a preview to aim it." },
    description: { en: "The camera becomes a camera entity in Home Assistant." },
    readme: { en: "The camera becomes a camera entity in Home Assistant, which every camera tile shows. On the screen itself a preview helps you aim it." },
    adds: { settings: true, card: true },
  },
];

// What the example screens have, by their ESPHome name, so the page shows each state a plugin can be in.
export const EXAMPLE_INSTALLED: Record<string, Installed[]> = {
  "living-room-screen": [{ id: "heating_schedule", version: "1.0.0", source: "index" }],
  "office-panel": [
    { id: "audio", version: "0.3.0", source: "index" },
    { id: "kiosk_buttons", version: "main", source: "branch", ref: "github.com/example-maker/tessera-kiosk-buttons@main" },
  ],
};

// A test plugin has no entry in the index: the page knows it only by what the screen says it runs.
export const testPlugin = (installed: Installed): Plugin => ({
  id: installed.id, icon: "F0A66", maintainer: (installed.ref || "").split("/")[1] || "", tessera: false, version: installed.version,
  repo: installed.ref ? `https://${installed.ref.split("@")[0]}` : "", license: "", kind: "behaviour", boards: "any", requires: {},
  flash_kb: 0, permissions: { home_assistant: [], network: [] }, attributes: [],
  name: { en: installed.id.replace(/_/g, " ").replace(/^./, (c) => c.toUpperCase()) }, summary: { en: "" }, description: { en: "" }, adds: {},
  readme: { en: "" }, languages: [],
});
