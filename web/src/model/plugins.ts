// Plugins (design, docs: the plugins proposal): what the index says about a plugin, and whether it fits a screen.
// Pure functions only, so the store page and its tests share one rule. The index itself comes from the add-on once it
// serves one (tessera-plugins/index.json); until then the page shows EXAMPLE_INDEX and says that it does.
import { editorLanguage } from "../i18n";
import type { Screen } from "../types";

type Texts = Record<string, string>;  // a plugin's own words per language, "en" always present
export type PluginSource = "index" | "link" | "branch" | "folder";
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
  permissions: { home_assistant: string[]; network: string[] };
  attributes: string[];               // cloud, commercial, ai-developed, experimental
  adds: {
    tiles?: { name: Texts; min: string; max: string }[];
    tap_actions?: { label: Texts; domains: string[] }[];
    card?: boolean;
    settings?: boolean;
    ha_package?: boolean;
    inputs?: boolean;
  };
};
export type Installed = { id: string; version: string; source: PluginSource; ref?: string };

// The words in the editor's language, else English.
export const text = (texts: Texts) => texts[editorLanguage()] ?? texts[editorLanguage().split("-")[0]] ?? texts.en ?? "";

// ---- Does it fit this screen ----
// The reasons a plugin is not offered for a screen, in the order a person can do something about them.
export type Misfit = "board" | "psram" | "firmware" | "flash";
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
export const SMALL_FLASH = { image: 1_853_664, slot: 2_031_616, ceiling: 0.93 };
// The boards with 4 MB of flash (boards.yaml); the add-on will say this per board once it serves the index.
const SMALL_FLASH_BOARDS = ["cyd", "cyd9342", "hosyond40"];
const smallFlash = (screen: Screen) => Boolean(screen.board && SMALL_FLASH_BOARDS.includes(screen.board));
export const headroomKb = () => Math.floor((SMALL_FLASH.slot * SMALL_FLASH.ceiling - SMALL_FLASH.image) / 1024);

export function fit(plugin: Plugin, screen: Screen | null): Fit {
  if (!screen) return { ok: true };
  if (plugin.boards !== "any" && !(screen.board && plugin.boards.includes(screen.board))) return { ok: false, reason: "board" };
  if (plugin.requires.psram && !screen.pictures) return { ok: false, reason: "psram" };
  if (plugin.requires.firmware && !atLeast(screen.firmware, plugin.requires.firmware.replace(/^>=\s*/, ""))) return { ok: false, reason: "firmware" };
  if (smallFlash(screen) && plugin.flash_kb > headroomKb()) return { ok: false, reason: "flash" };
  return { ok: true };
}

// What its image would take of the update slot on a 4 MB board, before and after: the meter in the details.
export function flashShare(plugin: Plugin, screen: Screen | null) {
  if (!screen || !smallFlash(screen)) return null;
  const before = SMALL_FLASH.image / SMALL_FLASH.slot;
  return { before, after: (SMALL_FLASH.image + plugin.flash_kb * 1024) / SMALL_FLASH.slot };
}

// ---- Example index: shown until the add-on serves the real one, and marked as such on the page ----
export const EXAMPLE_INDEX: Plugin[] = [
  {
    id: "heating_schedule", icon: "F00ED", maintainer: "Tessera", tessera: true, version: "1.1.0",
    repo: "https://github.com/MaxGramser/homeassistant_espscreen/tree/main/plugins/heating_schedule", license: "AGPL-3.0",
    kind: "behaviour", boards: "any", requires: { firmware: ">=0.50.0" }, flash_kb: 24,
    permissions: { home_assistant: ["schedule/list", "schedule/update"], network: [] }, attributes: [],
    name: { en: "Heating schedule", nl: "Verwarmingsschema" },
    summary: { en: "Edit the week programme of a thermostat on the screen.", nl: "Pas het weekprogramma van een thermostaat aan op het scherm." },
    description: {
      en: "Drag the blocks of a day, set their temperature and save. The programme lives in Home Assistant's own Schedule helpers, so the helpers page and the screen edit the same thing. A Vacation timeline overrides the week until you switch it off.",
      nl: "Sleep de blokken van een dag, kies hun temperatuur en bewaar. Het programma staat in de Schedule-helpers van Home Assistant zelf, dus de helperpagina en het scherm passen hetzelfde aan. Een vakantieschema gaat voor de week tot je het uitzet.",
    },
    adds: {
      tiles: [{ name: { en: "Week schedule", nl: "Weekschema" }, min: "2×1", max: "4×2" }],
      tap_actions: [{ label: { en: "Edit schedule", nl: "Schema aanpassen" }, domains: ["climate"] }],
      card: true, settings: true, ha_package: true,
    },
  },
  {
    id: "audio", icon: "F057E", maintainer: "Tessera", tessera: true, version: "0.3.0",
    repo: "https://github.com/MaxGramser/homeassistant_espscreen/tree/main/plugins/audio", license: "AGPL-3.0",
    kind: "hardware", boards: ["wavesharep4"], board_names: ["Waveshare ESP32-P4-86-Panel"], requires: { firmware: ">=0.50.0", psram: true }, flash_kb: 60,
    permissions: { home_assistant: [], network: [] }, attributes: [],
    name: { en: "Audio", nl: "Audio" },
    summary: { en: "The microphone and speaker of the Waveshare P4 86 panel.", nl: "De microfoon en luidspreker van het Waveshare P4 86-paneel." },
    description: {
      en: "Speaker volume, microphone mute, automatic gain and a tap sound, on the screen's settings page and in the editor. Tests for the speaker and the microphone come with it.",
      nl: "Luidsprekervolume, microfoon dempen, automatische versterking en een tikgeluid, op de instellingenpagina van het scherm en in de editor. Met tests voor de luidspreker en de microfoon.",
    },
    adds: { settings: true },
  },
  {
    id: "relays_86", icon: "F1A25", maintainer: "example-maker", tessera: false, version: "0.4.1",
    repo: "https://github.com/example-maker/tessera-relays-86", license: "MIT",
    kind: "hardware", boards: ["wavesharep4"], board_names: ["Waveshare ESP32-P4-86-Panel"], requires: { firmware: ">=0.50.0" }, flash_kb: 4,
    permissions: { home_assistant: [], network: [] }, attributes: [],
    name: { en: "Panel relays", nl: "Relais van het paneel" },
    summary: { en: "The two relays of an 86 panel as switches in Home Assistant.", nl: "De twee relais van een 86-paneel als schakelaars in Home Assistant." },
    description: {
      en: "Each relay becomes a switch of the screen in Home Assistant, so a switch tile on any screen, or an automation, turns it on and off. Names and the state after a power cut are set in the editor.",
      nl: "Elk relais wordt een schakelaar van het scherm in Home Assistant, zodat een schakelaartegel op elk scherm, of een automatisering, hem aan- en uitzet. Namen en de stand na een stroomstoring kies je in de editor.",
    },
    adds: { settings: true },
  },
  {
    id: "ds18b20", icon: "F050F", maintainer: "example-maker", tessera: false, version: "1.2.0",
    repo: "https://github.com/example-maker/tessera-ds18b20", license: "MIT",
    kind: "hardware", boards: "any", requires: { firmware: ">=0.50.0", free_gpio: 1 }, flash_kb: 6,
    permissions: { home_assistant: [], network: [] }, attributes: [],
    name: { en: "Temperature probe", nl: "Temperatuursensor" },
    summary: { en: "A DS18B20 on a free pin, as a sensor of the screen.", nl: "Een DS18B20 op een vrije pin, als sensor van het scherm." },
    description: {
      en: "Wire a DS18B20 to a free pin of the board and choose that pin here. The temperature becomes a sensor of the screen in Home Assistant, and a sensor tile shows it.",
      nl: "Sluit een DS18B20 aan op een vrije pin van het bordje en kies die pin hier. De temperatuur wordt een sensor van het scherm in Home Assistant, en een sensortegel toont hem.",
    },
    adds: { inputs: true },
  },
  {
    id: "night_light", icon: "F0594", maintainer: "example-labs", tessera: false, version: "0.2.0",
    repo: "https://github.com/example-labs/tessera-night-light", license: "GPL-3.0",
    kind: "behaviour", boards: "any", requires: { firmware: ">=0.50.0" }, flash_kb: 3,
    permissions: { home_assistant: [], network: [] }, attributes: ["ai-developed"],
    name: { en: "Night light", nl: "Nachtlampje" },
    summary: { en: "A warm, dim glow on the glass with one tap at night.", nl: "Een warme, zachte gloed op het scherm met één tik in de nacht." },
    description: {
      en: "A tile that turns the whole screen into a soft warm light until the next tap. Its colour and brightness are set in the editor.",
      nl: "Een tegel die het hele scherm een zacht warm licht maakt tot de volgende tik. Kleur en helderheid kies je in de editor.",
    },
    adds: { tiles: [{ name: { en: "Night light", nl: "Nachtlampje" }, min: "1×1", max: "2×1" }], card: true },
  },
  {
    id: "camera_module", icon: "F07AE", maintainer: "example-labs", tessera: false, version: "0.1.0",
    repo: "https://github.com/example-labs/tessera-camera-module", license: "MIT",
    kind: "hardware", boards: ["esp32s3_ov2640"], board_names: ["ESP32-S3 + OV2640"], requires: { firmware: ">=0.50.0", psram: true }, flash_kb: 85,
    permissions: { home_assistant: [], network: [] }, attributes: ["experimental"],
    name: { en: "Camera module", nl: "Cameramodule" },
    summary: { en: "An OV2640 on an ESP32-S3 board, with a preview to aim it.", nl: "Een OV2640 op een ESP32-S3-bordje, met een voorbeeld om hem te richten." },
    description: {
      en: "The camera becomes a camera entity in Home Assistant, which every camera tile shows. On the screen itself a preview helps you aim it; flip and resolution are settings.",
      nl: "De camera wordt een camera-entiteit in Home Assistant, die elke camerategel toont. Op het scherm zelf helpt een voorbeeld om hem te richten; spiegelen en resolutie zijn instellingen.",
    },
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
});
