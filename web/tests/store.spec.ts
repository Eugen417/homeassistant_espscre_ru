// The store: selecting a screen, editing its layout, what's new, progress, copy and import.
import { beforeEach, describe, expect, it, vi } from "vitest";
import {
  addTile, canAlert, copyLayoutFrom, importLayout, layoutJson, liveOf, PHASES, removeTile, retargetPageTile, select, setTileOption, state, tileLimit,
  topbarItems, topbarView, updateProgress, whatsNew,
} from "../src/store";
import type { Inventory, Screen } from "../src/types";

const screen = (id: string, name: string, firmware: string, tiles: any[]): Screen => ({
  id, name, online: true, firmware, board: "guition", layout: { title: name, tiles }, update: { available: true, target: "0.2.62" },
  alert_action: `esphome.${id}_show_alert`,
} as unknown as Screen);

function inventory(): Inventory {
  return {
    csrf: "t", connected: true,
    screens: [
      screen("living", "Living room", "0.2.60", [{ entity: "light.a", name: "", slot: 0 }, { entity: "sensor.t", name: "Temp", slot: 1, options: { display: "graph" } }]),
      screen("kitchen", "Kitchen", "0.2.6", [{ entity: "switch.c", name: "", slot: 0, options: { background: "orange" } }]),
    ],
    entities: [
      { id: "light.a", name: "Lamp A", state: "on", area: "Living room" }, { id: "sensor.t", name: "Temperature", state: "21.5", area: "Living room" },
      { id: "switch.c", name: "Coffee", state: "off", area: "Kitchen" }, { id: "light.b", name: "Lamp B", state: "off", area: "Kitchen" },
    ],
    updates: { target: "0.2.62", changelog: [
      { app: "0.2.74", firmware: "0.2.62", lines: ["Full-page tiles.", "Live values."] },
      { app: "0.2.71", firmware: "0.2.60", lines: ["Slider stays put."] },
      { app: "0.2.40", firmware: "0.2.34", lines: ["English."] },
    ] } as any,
    alerts: { min_firmware: "0.2.31" },
    controls: { light: { default: "toggle", choices: [{ key: "toggle", label: "On/off" }, { key: "brightness", label: "Brightness" }, { key: "none", label: "None" }] } },
    backgrounds: { auto: { label: "Default" }, orange: { label: "Orange", color: "#ffe1c6" } },
  } as unknown as Inventory;
}

beforeEach(() => {
  vi.stubGlobal("fetch", vi.fn(() => Promise.reject(new Error("offline"))));
  vi.spyOn(window, "confirm").mockReturnValue(true);
  state.inventory = inventory();
  state.selected = null;
  state.layout = null;
  state.dirty = false;
  state.liveStates = {};
  state.toast = null;
  state.updating = [];
  state.firmwareJob = null;
});

describe("selecting and editing", () => {
  it("copies the stored layout so edits never touch the inventory until saved", () => {
    select("living");
    expect(state.layout).not.toBe(state.inventory.screens[0].layout);
    expect(state.layout!.tiles.map((t) => t.slot)).toEqual([0, 1]);
    expect(state.dirty).toBe(false);
    state.layout!.tiles[0].name = "Changed";
    expect(state.inventory.screens[0].layout.tiles[0].name).toBe("");
  });
  it("adds a tile to the first free cell once, within the firmware's limit", () => {
    select("living");
    addTile("light.b");
    expect(state.layout!.tiles.map((t) => t.entity)).toEqual(["light.a", "sensor.t", "light.b"]);
    expect(state.layout!.tiles[2].slot).toBe(2);
    expect(state.dirty).toBe(true);
    addTile("light.b");
    expect(state.layout!.tiles).toHaveLength(3);
    expect(tileLimit.value).toBe(20);
  });
  it("keeps a wide tile at the start of a row and lets a display change fix the controls", () => {
    select("living");
    const sensor = state.layout!.tiles[1];
    setTileOption(sensor, "size", "wide");
    expect(sensor.slot).toBe(2);
    expect(state.layout!.tiles.map((t) => t.slot)).toEqual([0, 2]);
    const lamp = state.layout!.tiles[0];
    setTileOption(lamp, "size", "wide");
    setTileOption(lamp, "controls", "brightness");
    setTileOption(lamp, "display", "watch");
    expect(lamp.options).toMatchObject({ size: "wide", display: "watch", controls: "none", inline: "none" });
  });
  it("removes a tile and offers to undo", () => {
    select("living");
    removeTile(state.layout!.tiles[0]);
    expect(state.layout!.tiles.map((t) => t.entity)).toEqual(["sensor.t"]);
    expect(state.toast?.action?.label).toBe("Undo");
    state.toast!.action!.run();
    expect(state.layout!.tiles.map((t) => t.entity)).toEqual(["light.a", "sensor.t"]);
  });
  it("shows the clock of the stored setting when there is no top bar yet", () => {
    select("living");
    expect(topbarItems()).toEqual([{ type: "clock" }]);
    state.now = new Date(2026, 8, 15, 10, 8).getTime();
    expect(topbarView({ type: "clock" }).text).toBe("10:08");
    expect(topbarView({ type: "date" }).text).toBe("Tu 15 Sep");
    expect(topbarView({ type: "analog" }).analog).toBe(true);
  });
});

describe("live values", () => {
  it("falls back to what the inventory knew", () => {
    expect(liveOf("light.a")).toEqual({ state: "on", word: null, a: {} });
    state.liveStates["light.a"] = { state: "off", word: "Off", a: { brightness: 0 } };
    expect(liveOf("light.a")!.word).toBe("Off");
    expect(liveOf("light.nope")).toBeNull();
  });
});

describe("copy, export and import", () => {
  it("copies another screen's tiles and keeps this screen's title", () => {
    select("living");
    copyLayoutFrom("kitchen");
    expect(state.layout!.title).toBe("Living room");
    expect(state.layout!.tiles.map((t) => t.entity)).toEqual(["switch.c"]);
    expect(state.layout!.tiles[0].options).toEqual({ background: "orange" });
    expect(state.dirty).toBe(true);
    expect(JSON.parse(layoutJson())).toMatchObject({ esp_screens_layout: 1, title: "Living room", tiles: [{ entity: "switch.c" }] });
  });
  it("refuses garbage, drops duplicates and trims to the firmware's limit", () => {
    select("kitchen"); // firmware 0.2.6: ten tiles
    importLayout("not json");
    expect(state.toast?.message).toMatch(/isn't JSON/);
    importLayout(JSON.stringify({ hello: 1 }));
    expect(state.toast?.message).toMatch(/No tiles/);
    const many = Array.from({ length: 14 }, (_, i) => ({ entity: `light.l${i}`, name: "", slot: i }));
    importLayout(JSON.stringify({ tiles: [...many, { entity: "light.l0" }, { bogus: true }, { entity: "no-dot" }] }));
    expect(state.layout!.tiles).toHaveLength(10);
    expect(state.toast?.message).toMatch(/10 of 14 tiles/);
  });
});

describe("updates with content", () => {
  it("lists what a screen gets, and nothing it already has", () => {
    const [living, kitchen] = state.inventory.screens;
    expect(whatsNew(living)).toEqual(["Full-page tiles.", "Live values."]);
    expect(whatsNew(kitchen)).toEqual(["Full-page tiles.", "Live values.", "Slider stays put.", "English."]);
    expect(whatsNew({ ...living, firmware: "0.2.62" })).toEqual([]);
  });
  it("turns the phase and the ESPHome stage into a progress bar", () => {
    const living = state.inventory.screens[0];
    expect(updateProgress(living)).toBeNull();
    living.update!.state = "running";
    living.update!.phase = "install";
    expect(updateProgress(living)).toEqual({ percent: 12, text: PHASES.install });
    state.firmwareJob = { job: { stage: "compile" }, logs: [] };
    expect(updateProgress(living)!.percent).toBe(40);
    state.firmwareJob = { job: { stage: "upload" }, logs: [] };
    expect(updateProgress(living)!.percent).toBe(66);
    living.update!.phase = "verify";
    expect(updateProgress(living)!.percent).toBe(78);
    living.update!.phase = "settle";
    expect(updateProgress(living)!.percent).toBe(92);
  });
  it("knows which screen can show an alert", () => {
    const [living, kitchen] = state.inventory.screens;
    expect(canAlert(living)).toBe(true);
    expect(canAlert(kitchen)).toBe(false);
    expect(canAlert(undefined)).toBe(false);
  });
});

describe("full-page and navigation tiles", () => {
  it("moves the other tiles of the page behind a tile that grows to the whole page", () => {
    select("living");
    const [lamp, sensor] = state.layout!.tiles;
    setTileOption(lamp, "size", "full");
    expect(lamp.slot).toBe(0);
    expect(sensor.slot).toBe(6);
    expect(state.layout!.pages).toBe(2);
    expect(state.toast).toBeNull();
  });
  it("takes the first empty page when the others cannot move, and gives up with a toast when none is free", () => {
    select("living");
    state.layout!.tiles = Array.from({ length: 48 }, (_, i) => ({ entity: `light.l${i}`, name: "", slot: i }));
    const first = state.layout!.tiles[0];
    setTileOption(first, "size", "full");
    expect(first.options?.size).toBe("single");
    expect(state.toast?.message).toMatch(/No page is free/);
  });
  it("lets a navigation tile point at another page, once per page", () => {
    select("living");
    addTile("screen.page_2");
    const nav = state.layout!.tiles.find((t) => t.entity === "screen.page_2")!;
    expect(nav.options).toBeUndefined();
    expect(retargetPageTile(nav, 3)).toBe(true);
    expect(nav.entity).toBe("screen.page_3");
    addTile("screen.page_4");
    expect(retargetPageTile(nav, 4)).toBe(false);
    expect(state.toast?.message).toMatch(/already has a tile that goes to page 4/);
    expect(retargetPageTile(nav, 9)).toBe(false);
  });
});
