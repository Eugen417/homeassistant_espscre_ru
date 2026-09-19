// The components that draw the state: a tile with live values, the library's filters, the ⌘K search.
import { mount } from "@vue/test-utils";
import { beforeEach, describe, expect, it, vi } from "vitest";
import AppSettingsView from "../src/components/AppSettingsView.vue";
import CommandPalette from "../src/components/CommandPalette.vue";
import Library from "../src/components/Library.vue";
import Sidebar from "../src/components/Sidebar.vue";
import TileCard from "../src/components/TileCard.vue";
import TileInspector from "../src/components/TileInspector.vue";
import { state } from "../src/store";
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
