// ---- Grid positions ----
// Two columns, three rows per page, at most eight pages. A tile's `slot` is its absolute
// cell (page * 6 + row * 2 + column); a wide tile starts in the left column and also covers
// the cell to its right. Empty cells are allowed and stay exactly where they are.
import type { Inventory, Layout, Tile } from "../types";

export const SLOTS_PER_PAGE = 6;
export const MAX_PAGES = 8;
export const MAX_SLOTS = MAX_PAGES * SLOTS_PER_PAGE;

export type Entry = { tile: Tile; slot: number };

export const isWide = (tile: Tile) => tile.options?.size === "wide";
export const cellsOf = (slot: number, wide: boolean) => (wide ? [slot, slot + 1] : [slot]);
export const rowStart = (slot: number) => slot - (slot % 2);
export const pageOf = (slot: number) => Math.floor(slot / SLOTS_PER_PAGE);
export const entriesOf = (layout: Layout): Entry[] => layout.tiles.map((tile) => ({ tile, slot: tile.slot }));

// In-order packing: the rule before positions existed, and what firmware below 0.2.26 still draws.
export function packSlots(tiles: Tile[]) {
  let position = 0;
  return tiles.map((tile) => {
    const wide = isWide(tile);
    if (wide && position % 2 === 1) position++;
    const slot = position;
    position += wide ? 2 : 1;
    return slot;
  });
}
export function hasGaps(tiles: Tile[]) {
  const packed = packSlots(tiles);
  return tiles.some((tile, i) => tile.slot !== packed[i]);
}
// Every tile gets a position (older layouts pack in order) and the list stays in reading order.
export function normalize(layout: Layout) {
  if (layout.tiles.some((t) => !Number.isInteger(t.slot))) {
    const packed = packSlots(layout.tiles);
    layout.tiles.forEach((t, i) => (t.slot = packed[i]));
  }
  layout.tiles.sort((a, b) => a.slot - b.slot);
}
export function occupied(entries: Entry[]) {
  const taken = new Set<number>();
  for (const { tile, slot } of entries) for (const cell of cellsOf(slot, isWide(tile))) taken.add(cell);
  return taken;
}
export const fits = (taken: Set<number>, slot: number, wide: boolean) =>
  Number.isInteger(slot) && slot >= 0 && slot + (wide ? 1 : 0) < MAX_SLOTS && !(wide && slot % 2) && cellsOf(slot, wide).every((c) => !taken.has(c));
export function firstFree(taken: Set<number>, wide: boolean) {
  for (let slot = 0; slot < MAX_SLOTS; slot++) if (fits(taken, slot, wide)) return slot;
  return -1;
}
// The free position closest to `origin`; on a tie the later one, so a nudged tile moves down, not up.
export function nearestFree(taken: Set<number>, wide: boolean, origin: number) {
  let best = -1;
  for (let slot = 0; slot < MAX_SLOTS; slot++)
    if (fits(taken, slot, wide) && (best < 0 || Math.abs(slot - origin) <= Math.abs(best - origin))) best = slot;
  return best;
}
// The arrangement after putting `moving` (a tile on the grid, or a new one) at `target`:
// it lands exactly there; tiles in its way take the cells it left (a swap) or else the
// nearest free cell; everything else stays put. Null when the target is off the grid.
export function arrange(tiles: Tile[], moving: Tile, target: number): Entry[] | null {
  const wide = isWide(moving);
  if (wide) target = rowStart(target);
  if (!fits(new Set(), target, wide)) return null;
  const footprint = cellsOf(target, wide);
  const vacated = tiles.includes(moving) ? cellsOf(moving.slot, wide) : [];
  const result: Entry[] = [{ tile: moving, slot: target }];
  const displaced: Tile[] = [];
  for (const tile of tiles) {
    if (tile === moving) continue;
    if (cellsOf(tile.slot, isWide(tile)).some((c) => footprint.includes(c))) displaced.push(tile);
    else result.push({ tile, slot: tile.slot });
  }
  for (const tile of displaced) {
    const w = isWide(tile), taken = occupied(result);
    let slot = vacated.map((c) => (w ? rowStart(c) : c)).find((c) => fits(taken, c, w));
    if (slot === undefined) slot = nearestFree(taken, w, tile.slot);
    if (slot < 0) return null;
    result.push({ tile, slot });
  }
  return result.sort((a, b) => a.slot - b.slot);
}
// Pages the tiles need, or more when the user keeps empty pages on purpose (`layout.pages`).
export function pageCount(entries: Entry[], wanted = 1) {
  const last = Math.max(0, ...entries.map(({ tile, slot }) => slot + (isWide(tile) ? 2 : 1)));
  return Math.min(MAX_PAGES, Math.max(1, Math.ceil(last / SLOTS_PER_PAGE), wanted || 1));
}
// New tiles start with the card that shows the entity best.
export function defaultOptions(id: string): Partial<Tile> {
  const domain = id.split(".")[0];
  if (domain === "sun") return { options: { display: "sunpath", size: "wide" } };
  if (domain === "weather") return { options: { display: "forecast", size: "wide" } };
  if (domain === "screen") return { options: { display: "digital", size: "wide" } };
  return {};
}
export const newTile = (id: string): Tile => ({ entity: id, name: "", slot: -1, ...defaultOptions(id) } as Tile);

// Same rule as the add-on: only a wide card in the standard layout shows direct controls;
// without a choice the domain's first control set applies.
export function effectiveControls(tile: Tile, inventory: Inventory): string | null {
  const domain = tile.entity.split(".")[0], catalogue = inventory.controls?.[domain], o = tile.options || {};
  if (!catalogue || o.size !== "wide" || (o.display || "standard") !== "standard" || o.inline === "slider") return null;
  const choice = o.controls ?? catalogue.default;
  return choice === "none" ? null : choice;
}
export function controlsLabel(tile: Tile, inventory: Inventory) {
  const key = effectiveControls(tile, inventory);
  if (!key) return "none";
  return inventory.controls?.[tile.entity.split(".")[0]]?.choices.find((c) => c.key === key)?.label.toLocaleLowerCase() || key;
}

export const parseVersion = (v: string | undefined) => (/^(\d+)\.(\d+)\.(\d+)$/.exec(v || "") || []).slice(1).map(Number);
export function versionAtLeast(version: string | undefined, minimum: string) {
  const [a, b] = [parseVersion(version), parseVersion(minimum)];
  if (a.length !== 3 || b.length !== 3) return false;
  for (let i = 0; i < 3; i++) if (a[i] !== b[i]) return a[i] > b[i];
  return true;
}
export const supportsFirmware = (firmware: string | undefined, major: number, minor: number, patch: number) =>
  versionAtLeast(firmware, `${major}.${minor}.${patch}`);
export function tileLimit(firmware: string | undefined) {
  const match = parseVersion(firmware);
  if (match.length !== 3) return 10;
  return match[0] > 0 || match[1] > 2 || (match[1] === 2 && match[2] >= 7) ? 20 : 10;
}
export const displayNames: Record<string, string> = {
  standard: "standard", watch: "large value", forecast: "weather forecast", graph: "graph",
  digital: "digital clock", analog: "analog clock", sunpath: "sun path",
};
export const TOGGLE_BEFORE = ["light", "switch", "input_boolean", "fan", "media_player", "climate"];
export const SLIDER_DOMAINS = ["light", "fan", "cover", "number", "input_number", "media_player"];

export const domains: Record<string, [string, string, string, string]> = {
  light: ["Light", "☀", "#ad7600", "#fff3d3"],
  climate: ["Climate", "❄", "#c86620", "#ffebdc"],
  vacuum: ["Vacuum", "◉", "#008577", "#def3ed"],
  fan: ["Fan", "✣", "#008aab", "#def5fa"],
  cover: ["Cover", "▤", "#8053af", "#eee5f8"],
  media_player: ["Media", "▶", "#007cad", "#def2fc"],
  sensor: ["Sensor", "⌁", "#3476b1", "#e5effa"],
  binary_sensor: ["Status", "◈", "#ad7600", "#fff3d3"],
  switch: ["Switch", "⏻", "#ad7600", "#fff3d3"],
  input_boolean: ["Switch", "⏻", "#ad7600", "#fff3d3"],
  scene: ["Scene", "✦", "#8053af", "#eee5f8"],
  script: ["Script", "▷", "#8053af", "#eee5f8"],
  weather: ["Weather", "☁", "#007cad", "#def2fc"],
  number: ["Value", "±", "#008577", "#def3ed"],
  input_number: ["Value", "±", "#008577", "#def3ed"],
  select: ["Select", "≡", "#5862af", "#eaecfa"],
  input_select: ["Select", "≡", "#5862af", "#eaecfa"],
  button: ["Action", "↗", "#5862af", "#eaecfa"],
  input_button: ["Action", "↗", "#5862af", "#eaecfa"],
  screen: ["Clock", "◷", "#25282c", "#e9ecf1"],
  sun: ["Sun", "☼", "#c86620", "#ffebdc"],
  timer: ["Timer", "⏱", "#008577", "#def3ed"],
  person: ["Person", "☺", "#2f7d32", "#e1f2e2"],
  camera: ["Camera", "◧", "#3d4a57", "#e6ebf0"],
  image: ["Image", "◧", "#3d4a57", "#e6ebf0"],
};
export const domainInfo = (id: string) => domains[id.split(".")[0]] || (["Entity", "◇", "#637184", "#edf0f4"] as [string, string, string, string]);
