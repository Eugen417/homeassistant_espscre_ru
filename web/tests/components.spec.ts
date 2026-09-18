// The components that draw the state: a tile with live values, the library's filters, the ⌘K search.
import { mount } from "@vue/test-utils";
import { beforeEach, describe, expect, it, vi } from "vitest";
import CommandPalette from "../src/components/CommandPalette.vue";
import Library from "../src/components/Library.vue";
import TileCard from "../src/components/TileCard.vue";
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
    builtin: [{ id: "screen.clock", name: "Clock", device: "Built into the screen", area: "", state: "ok" }],
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
  it("draws the large value, the wide card's toggle and an unavailable entity in grey", () => {
    state.liveStates["sensor.t"] = { state: "1249", word: null, a: { unit_of_measurement: "W" } };
    const big = placed({ entity: "sensor.t", name: "Power", slot: 0, options: { display: "watch" } });
    expect(big.find(".big").text()).toBe("1249W");
    state.liveStates["light.a"] = { state: "off", word: "Off", a: {} };
    const wide = placed({ entity: "light.a", name: "", slot: 2, options: { size: "wide" } });
    expect(wide.classes()).toContain("wide");
    expect(wide.find(".tog").classes()).toContain("off");
    expect(wide.find(".ic").classes()).not.toContain("lit");
    const gone = placed({ entity: "light.b", name: "", slot: 4 });
    expect(gone.find(".st").text()).toBe("Unavailable");
    expect(gone.find(".st").classes()).toContain("off");
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
    expect(state.selectedTile).toBe("light.a");
    expect(state.inspector).toEqual({ kind: "tile" });
    expect(lamp.classes()).toContain("chosen");
  });
});

describe("Library", () => {
  it("filters by room and hides what is placed, and tints the avatars by state", async () => {
    state.layout!.tiles.push({ entity: "light.a", name: "", slot: 0 });
    const library = mount(Library);
    const names = () => library.findAll(".ent .tx b").map((b) => b.text());
    expect(names()).toEqual(["Clock", "Lamp A", "Lamp B", "Temperature", "Curtains"]);
    expect(library.find('.ent[title="light.a"]').attributes("disabled")).toBeDefined();
    expect(library.find('.ent[title="light.a"] .av').classes()).toContain("on");
    expect(library.find('.ent[title="light.b"] .av').classes()).toContain("gone");
    expect(library.findAll("#room option").map((o) => o.text())).toEqual(["All rooms", "Kitchen", "Living room"]);
    await library.find("#room").setValue("Kitchen");
    expect(names()).toEqual(["Lamp B"]);
    await library.find("#room").setValue("");
    await library.find("#hide-placed").trigger("click");
    expect(names()).toEqual(["Clock", "Lamp B", "Temperature", "Curtains"]);
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
