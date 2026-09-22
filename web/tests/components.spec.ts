// The components that draw the state: a tile with live values, the library's filters, the ⌘K search.
import { readFileSync } from "node:fs";
import { flushPromises, mount } from "@vue/test-utils";
import { nextTick } from "vue";
import { beforeEach, describe, expect, it, vi } from "vitest";
import AppSettingsView from "../src/components/AppSettingsView.vue";
import CommandPalette from "../src/components/CommandPalette.vue";
import Library from "../src/components/Library.vue";
import DevicePage from "../src/components/DevicePage.vue";
import InstallerView from "../src/components/InstallerView.vue";
import Sidebar from "../src/components/Sidebar.vue";
import TileCard from "../src/components/TileCard.vue";
import TileInspector from "../src/components/TileInspector.vue";
import TopbarInspector from "../src/components/TopbarInspector.vue";
import { openBar, state } from "../src/store";
import type { Inventory, Tile } from "../src/types";

function inventory(): Inventory {
  return {
    csrf: "t", connected: true,
    screens: [{ id: "living", name: "Living room", online: true, firmware: "0.2.60", board: "guition", layout: { title: "Living room", tiles: [] }, alert_action: "esphome.living_show_alert" } as any],
    entities: [
      { id: "light.a", name: "Lamp A", state: "on", area: "Living room", device: "Hue" },
      { id: "light.b", name: "Lamp B", state: "unavailable", area: "Kitchen" },
      { id: "sensor.t", name: "Temperature", state: "21.5", area: "Living room" },
      { id: "cover.c", name: "Curtains", state: "open", area: "Living room" },
    ],
    builtin: [
      { id: "screen.clock", name: "Clock", device: "Built into the screen", area: "", state: "ok" },
      { id: "screen.page_1", name: "Go to page 1", device: "Built into the screen", area: "", state: "ok" },
    ],
    icons: { groups: [], weather: { partlycloudy: "F0595" }, sun: { below_horizon: "F0594" }, defaults: { light: "F0335", sensor: "F050F", cover: "F1846" }, fallback: "F0335", builtin: { "screen.clock": "F0150" }, controls: { minus: "F0374", plus: "F0415" } },
    backgrounds: { auto: { label: "Default" }, none: { label: "None" } },
    controls: { light: { default: "toggle", choices: [{ key: "toggle", label: "On/off" }, { key: "brightness", label: "Brightness" }, { key: "none", label: "None" }] } },
    alerts: { min_firmware: "0.2.31" },
  } as unknown as Inventory;
}

beforeEach(() => {
  vi.stubGlobal("fetch", vi.fn(() => Promise.reject(new Error("offline"))));
  state.inventory = inventory();
  state.selected = "living";
  state.layout = { title: "Living room", tiles: [] };
  state.liveStates = {};
  state.search = ""; state.filter = ""; state.room = ""; state.hidePlaced = false;
  state.palette = false;
  state.selectedTile = null; state.inspector = null;
  state.dirty = false; state.tab = "layout";
});

function placed(tile: Tile) {
  state.layout!.tiles.push(tile);
  return mount(TileCard, { props: { tile, slot: tile.slot } });
}

describe("TileCard", () => {
  it("shows a sensor's value with its unit and a light that is on as lit", () => {
    state.liveStates["sensor.t"] = { state: "21.4", word: null, a: { unit_of_measurement: "°C" } };
    state.liveStates["light.a"] = { state: "on", word: "On", a: { brightness: 128 } };
    const sensor = placed({ entity: "sensor.t", name: "Temp", slot: 0 });
    expect(sensor.text()).toContain("21.4 °C");
    const lamp = placed({ entity: "light.a", name: "", slot: 1, options: { inline: "slider" } });
    expect(lamp.text()).toContain("Lamp A");
    expect(lamp.find(".ic").classes()).toContain("lit");
    expect(lamp.find(".mini-slider").attributes("style")).toContain("50%");
  });
  it("fills a blind's bar with its closed part, like the screen and its card", () => {
    state.liveStates["cover.c"] = { state: "open", word: "Open", a: { current_position: 30 } };
    const blind = placed({ entity: "cover.c", name: "", slot: 2, options: { inline: "slider" } });
    expect(blind.find(".mini-slider").attributes("style")).toContain("70%");
  });
  it("draws the large value, the wide card's toggle and an unavailable entity in grey", () => {
    state.liveStates["sensor.t"] = { state: "1249", word: null, a: { unit_of_measurement: "W" } };
    const big = placed({ entity: "sensor.t", name: "Power", slot: 0, options: { display: "watch" } });
    // Numbers as the screens write them (app 0.2.90): "1,234.5" until the add-on names another format.
    expect(big.find(".big").text()).toBe("1,249W");
    state.liveStates["light.a"] = { state: "off", word: "Off", a: {} };
    const wide = placed({ entity: "light.a", name: "", slot: 2, options: { size: "wide" } });
    expect(wide.classes()).toContain("wide");
    expect(wide.find(".tog").classes()).toContain("off");
    expect(wide.find(".ic").classes()).not.toContain("lit");
    const gone = placed({ entity: "light.b", name: "", slot: 4 });
    expect(gone.find(".st").text()).toBe("Unavailable");
    expect(gone.find(".st").classes()).toContain("off");
  });
  it("draws a single tile as the screen does: the icon left, the name and value beside it", () => {
    state.liveStates["sensor.t"] = { state: "1249", word: null, a: { unit_of_measurement: "W" } };
    const plain = placed({ entity: "sensor.t", name: "Power", slot: 0 });
    expect(plain.find(".head > .ic").exists()).toBe(true);
    expect(plain.find(".head > .tx > .nm").text()).toBe("Power");
    expect(plain.find(".head > .tx > .st").text()).toBe("1,249 W");
    // A watch card keeps the name next to the icon and puts the big value underneath.
    const watch = placed({ entity: "sensor.t", name: "Power", slot: 1, options: { display: "watch" } });
    expect(watch.find(".head").classes()).toContain("top");
    expect(watch.find(".head .big").exists()).toBe(false);
    expect(watch.find(".head + .big").text()).toBe("1,249W");
    state.liveStates["light.a"] = { state: "on", word: "On", a: { brightness: 255 } };
    const lamp = placed({ entity: "light.a", name: "", slot: 2, options: { inline: "slider" } });
    expect(lamp.find(".head + .mini-slider").exists()).toBe(true);
  });
  it("shows the display name when Home Assistant has no value, and nothing for a scene", () => {
    const graph = placed({ entity: "sensor.x", name: "Unknown sensor", slot: 0, options: { display: "graph" } });
    expect(graph.find(".st").text()).toBe("graph");
    state.liveStates["scene.movie"] = { state: "2026-09-14T19:15:00+00:00", word: null, a: {} };
    const scene = placed({ entity: "scene.movie", name: "Movie", slot: 1 });
    expect(scene.find(".st").exists()).toBe(false);
  });
  it("opens the tile's settings on a click", async () => {
    const lamp = placed({ entity: "light.a", name: "", slot: 0 });
    await lamp.trigger("click");
    expect(state.selectedTile?.entity).toBe("light.a");
    expect(state.inspector).toEqual({ kind: "tile" });
    expect(lamp.classes()).toContain("chosen");
  });
});

describe("Library", () => {
  it("filters by room and hides what is placed, and tints the avatars by state", async () => {
    state.layout!.tiles.push({ entity: "light.a", name: "", slot: 0 });
    const library = mount(Library);
    const names = () => library.findAll(".ent .tx b").map((b) => b.text());
    expect(names()).toEqual(["Clock", "Go to page 1", "Lamp A", "Lamp B", "Temperature", "Curtains"]);
    expect(library.find('.ent[title="light.a"]').attributes("disabled")).toBeDefined();
    expect(library.find('.ent[title="light.a"] .av').classes()).toContain("on");
    expect(library.find('.ent[title="light.b"] .av').classes()).toContain("gone");
    expect(library.findAll("#room option").map((o) => o.text())).toEqual(["All rooms", "Kitchen", "Living room"]);
    await library.find("#room").setValue("Kitchen");
    expect(names()).toEqual(["Lamp B"]);
    await library.find("#room").setValue("");
    await library.find("#hide-placed").trigger("click");
    expect(names()).toEqual(["Clock", "Go to page 1", "Lamp B", "Temperature", "Curtains"]);
    state.filter = "light";
    await library.vm.$nextTick();
    expect(names()).toEqual(["Lamp B"]);
  });
});

// The nineteen domain chips used to sit on one sideways scroller with its scrollbar hidden (app 0.2.74). A trackpad
// swipes such a strip, but an ordinary mouse has no bar to grab and no drag to start, so fifteen of the nineteen could
// not be reached at all. They wrap now, they follow the results the way the list does, and the tail of a long one
// folds behind "More" (app 0.2.116).
describe("Library: every domain chip is reachable without a trackpad", () => {
  // Every chip but All and the More/Fewer one carries its domain's glyph in front of the label.
  const label = (b: { text: () => string }) => b.text().replace(/^[^\p{L}]+/u, "");
  const chips = (library: ReturnType<typeof mount>) => library.findAll("#filters button").map(label);
  const domains = (library: ReturnType<typeof mount>) => chips(library).filter((c) => !/^(More|Fewer)/.test(c));
  const chip = (library: ReturnType<typeof mount>, name: string) =>
    library.findAll("#filters button").find((b) => label(b) === name)!;
  // One entity in each of nine domains, so the strip is longer than the head can hold.
  const manyDomains = () => state.inventory.entities.push(
    { id: "climate.c", name: "Heating", state: "heat" }, { id: "switch.s", name: "Plug", state: "on" },
    { id: "binary_sensor.b", name: "Door", state: "off" }, { id: "script.r", name: "Run", state: "off" },
    { id: "fan.f", name: "Fan", state: "off" }, { id: "scene.n", name: "Night", state: "on" },
    { id: "media_player.m", name: "Sonos", state: "idle" }, { id: "person.p", name: "Sam", state: "home" },
  ) as unknown as void;

  it("offers the domains the results hold, and narrows them as the search narrows the list", async () => {
    const library = mount(Library);
    // Four domains in this home, plus All. A chip for a domain with nothing behind it would filter to an empty list.
    expect(domains(library)).toEqual(["All", "Lights", "Covers", "Sensors", "Screen"]);
    expect(library.find("#more-filters").exists()).toBe(false);

    state.search = "lamp";
    await library.vm.$nextTick();
    expect(domains(library)).toEqual(["All", "Lights"]);
    state.search = "temp";
    await library.vm.$nextTick();
    expect(domains(library)).toEqual(["All", "Sensors"]);
  });

  it("keeps every other chip once one is chosen, so a domain is never a dead end", async () => {
    const library = mount(Library);
    await chip(library, "Lights").trigger("click");
    expect(state.filter).toBe("light");
    expect(library.findAll(".ent .tx b").map((b) => b.text())).toEqual(["Lamp A", "Lamp B"]);
    // Read off the domain filter itself and picking Lights would have taken Sensors and Covers away with it.
    expect(domains(library)).toEqual(["All", "Lights", "Covers", "Sensors", "Screen"]);
    expect(chip(library, "Lights").attributes("aria-pressed")).toBe("true");
  });

  it("keeps the chosen domain in sight when the search leaves nothing of it", async () => {
    const library = mount(Library);
    await chip(library, "Lights").trigger("click");
    state.search = "temp";
    await library.vm.$nextTick();
    expect(library.findAll(".ent").length).toBe(0);
    // An empty list needs the chip that empties it on show, or there is nothing to explain it and nothing to undo.
    expect(domains(library)).toContain("Lights");
    expect(chip(library, "Lights").attributes("aria-pressed")).toBe("true");
  });

  it("folds a long strip behind More and opens the rest in place", async () => {
    manyDomains();
    const library = mount(Library);
    expect(domains(library)).toHaveLength(7);
    expect(chips(library).at(-1)).toBe("More (6)");
    expect(library.find("#more-filters").attributes("aria-expanded")).toBe("false");

    await library.find("#more-filters").trigger("click");
    expect(library.find("#more-filters").attributes("aria-expanded")).toBe("true");
    expect(domains(library)).toEqual([
      "All", "Lights", "Climate", "Switches", "Status", "Scripts", "Fans", "Covers", "Scenes", "Sensors",
      "Media", "People", "Screen",
    ]);
    expect(chips(library).at(-1)).toBe("Fewer");

    await library.find("#more-filters").trigger("click");
    expect(domains(library)).toHaveLength(7);
  });

  it("carries a folded-away chip into the head once it is the chosen one", async () => {
    manyDomains();
    const library = mount(Library);
    await library.find("#more-filters").trigger("click");
    await chip(library, "People").trigger("click");
    await library.find("#more-filters").trigger("click");
    expect(domains(library)).toContain("People");
    expect(chips(library).at(-1)).toBe("More (5)");
    const pressed = library.findAll("#filters button").filter((b) => b.attributes("aria-pressed") === "true");
    expect(pressed.map(label)).toEqual(["People"]);
  });

  it("never hides the strip behind a scrollbar a mouse cannot reach", () => {
    const css = readFileSync("src/styles/app.css", "utf8");
    const rule = css.split("\n").find((line) => line.startsWith(".filters {"))!;
    expect(rule).toContain("flex-wrap: wrap");
    expect(rule).not.toContain("overflow");
    expect(rule).not.toContain("scrollbar-width");
    expect(css).not.toContain(".filters::-webkit-scrollbar");
  });
});

describe("CommandPalette", () => {
  it("lists screens and actions, finds an entity to add, and runs the chosen row", async () => {
    state.palette = true;
    const palette = mount(CommandPalette);
    await palette.vm.$nextTick();
    const labels = () => palette.findAll(".palette-item .tx > span").map((s) => s.text());
    expect(labels()).toContain("Living room");
    expect(labels()).toContain("Save & send");
    expect(labels()).toContain("Identify this screen");
    await palette.find("#palette-input").setValue("curt");
    expect(labels()).toEqual(["Curtains"]);
    await palette.find("#palette-input").trigger("keydown", { key: "Enter" });
    expect(state.layout!.tiles.map((t) => t.entity)).toEqual(["cover.c"]);
    expect(state.palette).toBe(false);
  });
});

describe("full-page and navigation tiles on the mockup", () => {
  it("draws a full tile as one big card and a navigation tile with its page", () => {
    state.liveStates["light.a"] = { state: "on", word: "On", a: {} };
    const full = placed({ entity: "light.a", name: "", slot: 0, options: { size: "full" } });
    expect(full.classes()).toContain("full");
    expect(full.classes()).not.toContain("wide");
    expect(full.find(".ic").classes()).toContain("lit");
    expect(full.find(".tog").exists()).toBe(false);
    const nav = placed({ entity: "screen.page_3", name: "Go to page 3", slot: 6 });
    expect(nav.find(".goto").text()).toBe("Page 3 ›");
  });
});

describe("several tiles that go to the same page in the library (firmware 0.2.65)", () => {
  it("keeps offering a placed navigation tile when the screen takes several, and adds another copy", async () => {
    Object.assign(state.inventory.screens[0], { firmware: "0.2.65", page_tiles_repeat: true });
    state.layout!.tiles.push({ entity: "screen.page_1", name: "", slot: 0 }, { entity: "light.a", name: "", slot: 1 });
    const library = mount(Library);
    const row = () => library.find('.ent[title="screen.page_1"]');
    expect(row().attributes("disabled")).toBeUndefined();
    expect(row().find(".add").text()).toBe("+");
    expect(library.find('.ent[title="light.a"]').attributes("disabled")).toBeDefined();
    await library.find("#hide-placed").trigger("click");
    expect(row().exists()).toBe(true);
    await row().trigger("click");
    expect(state.layout!.tiles.filter((t) => t.entity === "screen.page_1")).toHaveLength(2);
  });
  it("marks it placed when the screen takes one per page", () => {
    Object.assign(state.inventory.screens[0], { page_tiles_repeat: false });
    state.layout!.tiles.push({ entity: "screen.page_1", name: "", slot: 0 });
    const library = mount(Library);
    expect(library.find('.ent[title="screen.page_1"]').attributes("disabled")).toBeDefined();
    expect(library.find('.ent[title="screen.page_1"] .add').text()).toBe("✓");
  });
});

describe("TileInspector: a live picture on a camera tile (app 0.2.91)", () => {
  const row = (wrapper: ReturnType<typeof mount>, label: string) =>
    wrapper.findAll(".f").find((f) => f.find(".f-label").exists() && f.find(".f-label").text() === label)!;
  const choices = (wrapper: ReturnType<typeof mount>, label: string) => row(wrapper, label).findAll(".seg button").map((b) => b.text());
  it("offers the live picture for a camera, with its pace once chosen, and says which firmware it needs", async () => {
    state.inventory.entities.push({ id: "camera.front", name: "Front", state: "idle", area: "Hall" } as any);
    const tile: Tile = { entity: "camera.front", name: "", slot: 0 };
    state.layout!.tiles.push(tile);
    const drawer = mount(TileInspector, { props: { tile } });
    expect(choices(drawer, "Display")).toEqual(["Name and status", "Large value", "Live picture"]);
    expect(row(drawer, "Refresh")).toBeUndefined();
    await row(drawer, "Display").findAll(".seg button")[2].trigger("click");
    expect(tile.options).toEqual({ display: "live" });
    expect(choices(drawer, "Refresh")).toEqual(["Every 15 s", "Every 30 s"]);
    expect(row(drawer, "Refresh").find('[aria-pressed="true"]').text()).toBe("Every 15 s");
    expect(row(drawer, "Display").find("small").text()).toMatch(/firmware 0\.2\.77/);
    expect(row(drawer, "Display").find("small").classes()).toContain("warn");
    await row(drawer, "Refresh").findAll(".seg button")[1].trigger("click");
    expect(tile.options).toEqual({ display: "live", refresh: 30 });
    Object.assign(state.inventory.screens[0], { firmware: "0.2.77" });
    await drawer.vm.$nextTick();
    expect(row(drawer, "Display").find("small").text()).toMatch(/icon's place/);
    expect(row(drawer, "Display").find("small").classes()).not.toContain("warn");
    // A light has no such choice.
    const lamp = mount(TileInspector, { props: { tile: { entity: "light.a", name: "", slot: 1 } } });
    expect(choices(lamp, "Display")).not.toContain("Live picture");
    // The mockup draws the picture's rounded square instead of the icon.
    const card = mount(TileCard, { props: { tile, slot: 0 } });
    expect(card.find(".ic").classes()).toContain("thumb");
  });
  it("offers the album cover for a media player on a Guition, not on a full-page tile, and keeps its controls", async () => {
    Object.assign(state.inventory.screens[0], { firmware: "0.2.78" });
    state.inventory.entities.push({ id: "media_player.sonos", name: "Sonos", state: "playing", area: "Hall" } as any);
    (state.inventory as any).controls.media_player = { default: "volume", choices: [{ key: "volume", label: "Volume" }, { key: "none", label: "None" }] };
    const tile: Tile = { entity: "media_player.sonos", name: "", slot: 0, options: { size: "wide" } };
    state.layout!.tiles.push(tile);
    const drawer = mount(TileInspector, { props: { tile } });
    expect(choices(drawer, "Display")).toEqual(["Name and status", "Large value", "Album cover"]);
    await row(drawer, "Display").findAll(".seg button")[2].trigger("click");
    expect(tile.options).toEqual({ size: "wide", display: "cover" });
    expect(row(drawer, "Display").find("small").text()).toMatch(/icon's place/);
    expect(row(drawer, "Refresh")).toBeUndefined();
    expect(row(drawer, "Direct control on the tile")).toBeDefined();
    const card = mount(TileCard, { props: { tile, slot: 0 } });
    expect(card.find(".ic").classes()).toContain("thumb");
    expect(card.find(".range").exists()).toBe(true);
    // A board without memory for pictures (a CYD) gets no such choice; a tile over the whole page keeps the card's big cover.
    Object.assign(state.inventory.screens[0], { board: "cyd", pictures: false });
    tile.options = { size: "single" };
    await drawer.vm.$nextTick();
    expect(choices(mount(TileInspector, { props: { tile } }), "Display")).toEqual(["Name and status", "Large value"]);
    Object.assign(state.inventory.screens[0], { board: "guition", pictures: true });
    tile.options = { size: "full" };
    expect(choices(mount(TileInspector, { props: { tile } }), "Display")).toEqual(["Name and status", "Large value"]);
  });
});

describe("TileInspector: pages (app 0.2.78)", () => {
  const row = (wrapper: ReturnType<typeof mount>, label: string) =>
    wrapper.findAll(".f").find((f) => f.find(".f-label").exists() && f.find(".f-label").text() === label)!;
  const choices = (wrapper: ReturnType<typeof mount>, label: string) => row(wrapper, label).findAll(".seg button").map((b) => b.text());
  function open(tiles: any[], index: number) {
    state.layout!.tiles.push(...tiles);
    state.layout!.pages = 1;
    const tile = state.layout!.tiles[index];
    return { tile, drawer: mount(TileInspector, { props: { tile } }) };
  }
  it("offers the pages the screen has and the empty one after them as targets, and keeps one beyond those", async () => {
    Object.assign(state.inventory.screens[0], { firmware: "0.2.63", full_page: true });
    const { tile, drawer } = open([
      { entity: "light.a", name: "", slot: 0 }, { entity: "screen.page_2", name: "", slot: 1 }, { entity: "sensor.t", name: "", slot: 6 },
    ], 1);
    expect(choices(drawer, "Goes to page")).toEqual(["1", "2", "3 (empty)"]);
    expect(row(drawer, "Goes to page").find('[aria-pressed="true"]').text()).toBe("2");
    tile.entity = "screen.page_7";
    await drawer.vm.$nextTick();
    expect(choices(drawer, "Goes to page")).toEqual(["1", "2", "3 (empty)", "7 (empty)"]);
    expect(row(drawer, "Goes to page").find("small.warn").text()).toMatch(/no page 7/);
    await row(drawer, "Goes to page").findAll(".seg button")[2].trigger("click");
    expect(tile.entity).toBe("screen.page_3");
    expect(state.layout!.pages).toBe(3);
    expect(choices(drawer, "Goes to page")).toEqual(["1", "2", "3 (empty)", "4 (empty)"]);
    expect(row(drawer, "Goes to page").find("small").classes()).not.toContain("warn");
  });
  it("moves the tile to another page or a new one with a tap", async () => {
    const { tile, drawer } = open([{ entity: "light.a", name: "", slot: 0 }, { entity: "sensor.t", name: "", slot: 6 }], 0);
    expect(choices(drawer, "Page")).toEqual(["1", "2", "New page"]);
    expect(row(drawer, "Page").find('[aria-pressed="true"]').text()).toBe("1");
    await row(drawer, "Page").findAll(".seg button")[1].trigger("click");
    expect(tile.slot).toBe(7);
    expect(state.dirty).toBe(true);
    // Alone on the last page now: a new page would only leave this one empty.
    const sensor = state.layout!.tiles.find((t) => t.entity === "sensor.t")!;
    await row(drawer, "Page").findAll(".seg button")[0].trigger("click");
    expect(tile.slot).toBe(0);
    expect(sensor.slot).toBe(6);
    const other = mount(TileInspector, { props: { tile: sensor } });
    expect(choices(other, "Page")).toEqual(["1", "2"]);
    await row(other, "Page").findAll(".seg button")[0].trigger("click");
    expect(sensor.slot).toBe(1);
    // One tile on one page: nowhere to go, so no row.
    state.layout!.tiles = [];
    const only = open([{ entity: "light.b", name: "", slot: 0 }], 0).drawer;
    expect(row(only, "Page")).toBeUndefined();
  });
});

describe("Sidebar", () => {
  it("marks the open screen with unsaved edits and keeps them when it is chosen again", async () => {
    state.dirty = true;
    const layout = state.layout;
    state.tab = "settings";
    const sidebar = mount(Sidebar);
    const item = sidebar.find("#screens .nav-item");
    expect(item.find(".unsaved").exists()).toBe(true);
    await item.trigger("click");
    expect(state.layout).toBe(layout);
    expect(state.dirty).toBe(true);
    expect(state.tab).toBe("layout");
  });
  it("shows a screen's name and light alone, and its details once it is chosen (app 0.2.108)", async () => {
    const sidebar = mount(Sidebar);
    const item = sidebar.find("#screens .screen-item");
    expect(item.find(".led").classes()).toContain("ok");
    expect(item.find(".sub").exists()).toBe(false);
    expect(item.find(".screen-details").exists()).toBe(false);
    await item.find(".nav-item").trigger("click");
    expect(item.classes()).toContain("open");
    expect(item.findAll(".facts dt").map((dt) => dt.text())).toEqual(["Firmware", "Board"]);
    expect(item.findAll(".facts dd").map((dd) => dd.text())).toEqual(["0.2.60", "Guition · 4 inch"]);
    // Chosen again, the details fold away; a screen that is off shows why in red.
    await item.find(".nav-item").trigger("click");
    expect(item.classes()).not.toContain("open");
    Object.assign(state.inventory.screens[0], { online: false });
    await nextTick();
    expect(item.find(".led").classes()).toContain("down");
    expect(item.find(".sub").text()).toBe("Offline");
  });
  it("asks what goes before it removes a screen, and then removes it (app 0.2.112)", async () => {
    Object.assign(state.inventory.screens[0], { online: false, update: { profile: "living.yaml" } });
    const calls: [string, RequestInit][] = [];
    vi.stubGlobal("fetch", vi.fn((path: string, options: RequestInit) => {
      calls.push([path, options]);
      return Promise.resolve(new Response(JSON.stringify({ removed: true, name: "Living room", kept: [] }), { status: 200 }));
    }));
    const sidebar = mount(Sidebar);
    const item = sidebar.find("#screens .screen-item");
    await item.find(".nav-item").trigger("click");
    await item.find(".remove-screen").trigger("click");
    // What goes, before anything is asked of Home Assistant: the device, the profile and what is kept here.
    const said = item.find(".screen-remove").text();
    expect(said).toContain("Remove Living room?");
    expect(said).toContain("living.yaml");
    expect(calls).toEqual([]);
    await item.find(".btn.danger").trigger("click");
    await flushPromises();
    expect(calls.map(([path, options]) => [path, options.method])).toEqual([
      ["api/screens/living", "DELETE"], ["api/inventory?light=1", undefined]]);
    expect(state.selected).toBeNull();
    expect(state.layout).toBeNull();
    expect(state.toast?.message).toBe("Living room is removed.");
  });
  it("warns that a screen that is still connected comes back (app 0.2.112)", async () => {
    const sidebar = mount(Sidebar);
    const item = sidebar.find("#screens .screen-item");
    await item.find(".nav-item").trigger("click");
    await item.find(".remove-screen").trigger("click");
    expect(item.find(".screen-remove small.warn").text()).toContain("still connected");
    // Nothing goes until it is confirmed: Cancel puts the details back.
    await item.findAll(".screen-remove .btn")[1].trigger("click");
    expect(item.find(".screen-remove").exists()).toBe(false);
    expect(item.find(".facts").exists()).toBe(true);
  });
});

describe("AppSettingsView", () => {
  it("shows what the current firmware brings from the changelog of the full inventory (app 0.2.78)", () => {
    state.inventory.updates = { target: "0.2.65", pending: 0 };
    state.inventory.changelog = [
      { app: "0.2.78", firmware: "0.2.65", lines: ["Several tiles go to the same page."] },
      { app: "0.2.76", firmware: "0.2.63", lines: ["Older."] },
    ];
    const view = mount(AppSettingsView);
    expect(view.find(".whatsnew summary").text()).toBe("What's new in firmware 0.2.65");
    expect(view.findAll(".whatsnew li").map((li) => li.text())).toEqual(["Several tiles go to the same page."]);
    delete state.inventory.changelog;
    expect(mount(AppSettingsView).find(".whatsnew").exists()).toBe(false);
  });
});

describe("the title above a page (app 0.2.105)", () => {
  // You click a page's bar and answer one question: what stands above this page.
  it("asks it for the page whose bar was clicked", async () => {
    state.layout = { title: "Living room", tiles: [], pages: 3 };
    openBar(0, 1);
    const drawer = mount(TopbarInspector, { props: { index: 0 } });
    const field = drawer.find("#page-title");
    expect(drawer.find("label[for='page-title']").text()).toBe("Title above page 2");
    // Empty says what page 1 says, so a page that follows it looks like it does.
    expect((field.element as HTMLInputElement).value).toBe("");
    expect(field.attributes("placeholder")).toBe("Living room");
    expect(drawer.find("#page-title-hint").text()).toBe("Leave empty and this page says the same as page 1.");
    await field.setValue("Music");
    expect(state.layout!.page_titles).toEqual(["", "Music"]);
    // Clearing it hands the page back and leaves nothing behind.
    await field.setValue("");
    expect(state.layout!.page_titles).toBeUndefined();
  });
  it("is the screen's own title on page 1, and stores it there", async () => {
    state.layout = { title: "Living room", tiles: [], pages: 2, page_titles: ["", "Music"] };
    openBar(0, 0);
    const drawer = mount(TopbarInspector, { props: { index: 0 } });
    const field = drawer.find("#page-title");
    expect((field.element as HTMLInputElement).value).toBe("Living room");
    // Page 1 has no line of its own to explain, and no entry of its own to store.
    expect(drawer.find("#page-title-hint").exists()).toBe(false);
    await field.setValue("Downstairs");
    expect(state.layout!.title).toBe("Downstairs");
    expect(state.layout!.page_titles).toEqual(["", "Music"]);
  });
  it("names one field only, whatever the screen has", () => {
    state.layout = { title: "Living room", tiles: [] };
    openBar(0, 0);
    const one = mount(TopbarInspector, { props: { index: 0 } });
    expect(one.findAll("#page-title")).toHaveLength(1);
    expect(one.find("#title").exists()).toBe(false);
    expect(one.find("label[for='page-title']").text()).toBe("Title above the page");
  });
  it("shows the page's own title in that page's mockup bar, and opens that page's field", async () => {
    state.layout = { title: "Living room", tiles: [], pages: 2, page_titles: ["", "Music"] };
    const props = { entries: [], pages: 2, moving: null };
    const second = mount(DevicePage, { props: { page: 1, ...props } });
    expect(second.find(".bar-wrap").text()).toContain("Music");
    expect(mount(DevicePage, { props: { page: 0, ...props } }).find(".bar-wrap").text()).toContain("Living room");
    await second.find(".bar-wrap").trigger("click");
    expect(state.barPage).toBe(1);
  });
});

// New screen: which way the screen will hang (app 0.2.107). The choice is a build choice, so it is made here and
// nowhere else; the numbers beside each way come from the board files through the add-on, never from this page.
describe("the orientation of a new screen", () => {
  const boards = {
    cyd: { square: false, orientations: { landscape: { width: 320, height: 240, columns: 2, rows: 3, rotation: 90 },
                                          portrait: { width: 240, height: 320, columns: 1, rows: 4, rotation: 180 } } },
    guition: { square: true, orientations: { landscape: { width: 480, height: 480, columns: 2, rows: 3, rotation: 0 },
                                             portrait: { width: 480, height: 480, columns: 2, rows: 3, rotation: 0 } } },
    waveshare43: { square: false, orientations: { landscape: { width: 800, height: 480, columns: 3, rows: 3, rotation: 0 },
                                                  portrait: { width: 480, height: 800, columns: 1, rows: 4, rotation: 90 } } },
    waveshare7: { square: false, orientations: { landscape: { width: 800, height: 480, columns: 4, rows: 4, rotation: 0 },
                                                 portrait: { width: 480, height: 800, columns: 2, rows: 7, rotation: 90 } } },
  };
  const answers: any[] = [];
  async function installer() {
    vi.stubGlobal("fetch", vi.fn((url: string, options: any) => {
      if (String(url).endsWith("api/firmware")) {
        return Promise.resolve({ ok: true, json: () => Promise.resolve({ available: true, ports: [], profiles: [], logs: [], wifi: { state: "ready" }, boards }) });
      }
      answers.push(JSON.parse(options.body));
      return Promise.resolve({ ok: true, text: () => Promise.resolve(JSON.stringify({ file: "hall.yaml", api_key: "k" })) });
    }));
    const view = mount(InstallerView);
    await flush();
    return view;
  }
  const flush = async () => { await Promise.resolve(); await Promise.resolve(); await new Promise((done) => setTimeout(done, 0)); };

  it("offers the two ways glass that is not square can hang, with the cells each way gives", async () => {
    const view = await installer();
    const options = view.findAll("#orientation-fields .orient");
    expect(options).toHaveLength(2);
    expect(options.map((option) => option.find("b").text())).toEqual(["Lying down", "Standing up"]);
    expect(options.map((option) => option.find("small").text())).toEqual(["6 tiles a page", "4 tiles a page"]);
    // A picture of the glass each way, with a cell per tile of that page.
    expect(options[0].findAll(".orient-cells i")).toHaveLength(6);
    expect(options[1].findAll(".orient-cells i")).toHaveLength(4);
    expect(options[1].find(".orient-glass").attributes("style")).toContain("240 / 320");
    // Lying down to begin with, and saying so plainly that this is chosen now and not later.
    expect((options[0].find("input").element as HTMLInputElement).checked).toBe(true);
    expect(view.find("#orientation-hint").text()).toContain("build the screen again");
  });

  it("asks nothing about square glass, and asks again about the next board", async () => {
    const view = await installer();
    await view.findAll(".board input")[1].setValue("guition");
    expect(view.find("#orientation-fields").exists()).toBe(false);
    await view.findAll(".board input")[2].setValue("waveshare43");
    const options = view.findAll("#orientation-fields .orient");
    expect(options.map((option) => option.find("small").text())).toEqual(["9 tiles a page", "4 tiles a page"]);
  });

  it("explains experimental Waveshare 7 support and submits its selected orientation", async () => {
    const view = await installer();
    expect(view.text()).not.toContain("backlight stays on");
    await view.find('input[value="waveshare7"]').setValue("waveshare7");
    expect(view.find('input[value="waveshare7"]').element.closest("label")?.textContent).toContain("experimental");
    expect(view.text()).toContain("Not yet tested on this hardware");
    expect(view.text()).toContain("backlight stays on");
    const options = view.findAll("#orientation-fields .orient");
    expect(options.map((option) => option.find("small").text())).toEqual(["16 tiles a page", "14 tiles a page"]);
    await options[1].find("input").setValue("portrait");
    await view.find("#friendly_name").setValue("Hall");
    await view.find("#install-form").trigger("submit");
    await flush();
    expect(answers.pop()).toMatchObject({ board: "waveshare7", orientation: "portrait", name: "hall" });
  });

  it("sends the chosen way with the new screen", async () => {
    const view = await installer();
    await view.findAll("#orientation-fields .orient input")[1].setValue("portrait");
    await view.find("#friendly_name").setValue("Hall");
    await view.find("#install-form").trigger("submit");
    await flush();
    expect(answers.pop()).toMatchObject({ board: "cyd", orientation: "portrait", name: "hall" });
  });
});
