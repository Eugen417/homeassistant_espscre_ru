// ---- Grid positions ----
// A screen's page is a grid of cells: two columns and three rows on the boards that shipped first, and
// whatever a newer screen reports for itself (firmware 0.2.9x says "800x480 3x2"). A tile's `slot` is its
// absolute cell (page * cells + row * columns + column); a wide tile starts in a column that has a cell to its
// right and covers both; a full tile (firmware 0.2.62+) starts a page and covers every cell of it. Empty cells
// are allowed and stay exactly where they are.
//
// `setGrid` is called when the screen being edited changes, before anything is drawn or packed: the editor then
// places tiles the way that screen will, instead of the way the first two boards did.
import { reactive } from "vue";
import { t } from "../i18n";
import type { Inventory, Layout, Tile } from "../types";

export const MAX_PAGES = 8;
export const DEFAULT_GRID = { columns: 2, rows: 3 };
// Live bindings: importers see the grid of the screen they are editing (ES module exports update with them).
export let COLUMNS = DEFAULT_GRID.columns;
export let ROWS = DEFAULT_GRID.rows;
export let SLOTS_PER_PAGE = COLUMNS * ROWS;
export let MAX_SLOTS = MAX_PAGES * SLOTS_PER_PAGE;
// The same grid as something Vue can watch. A plain `let` is a live binding for other modules, but a computed()
// that reads one never re-runs when it changes: the mockup of a 3 x 3 screen kept drawing six cells while the
// counter beside it already said nine. Anything reactive reads `grid`, the helpers below keep the constants.
export const grid = reactive({ ...DEFAULT_GRID, slots: SLOTS_PER_PAGE, maxSlots: MAX_SLOTS });
export function setGrid(columns?: number, rows?: number) {
  const cols = Math.min(12, Math.max(1, Math.round(columns || DEFAULT_GRID.columns)));
  const lines = Math.min(12, Math.max(1, Math.round(rows || DEFAULT_GRID.rows)));
  COLUMNS = cols;
  ROWS = lines;
  SLOTS_PER_PAGE = cols * lines;
  MAX_SLOTS = MAX_PAGES * SLOTS_PER_PAGE;
  grid.columns = cols;
  grid.rows = lines;
  grid.slots = SLOTS_PER_PAGE;
  grid.maxSlots = MAX_SLOTS;
}

export type Entry = { tile: Tile; slot: number };
// A tile is single, wide (a row) or full (the whole page); `true` still means wide.
export type Size = "single" | "wide" | "full";
export const SIZES: Size[] = ["single", "wide", "full"];
type SizeLike = Size | boolean;
const asSize = (size: SizeLike): Size => (size === true ? "wide" : size === false ? "single" : size);

// A navigation tile (screen.page_<n>, firmware 0.2.62+) and the page it opens; 0 for any other entity.
export const pageTarget = (id: string) => (/^screen\.page_[1-8]$/.test(id) ? Number(id.slice(-1)) : 0);
export const sizeOf = (tile: Tile): Size => (SIZES.includes(tile.options?.size as Size) ? (tile.options!.size as Size) : "single");
export const isWide = (tile: Tile) => sizeOf(tile) !== "single";
export const isFull = (tile: Tile) => sizeOf(tile) === "full";
export const pageStart = (slot: number) => slot - (slot % SLOTS_PER_PAGE);
export const rowStart = (slot: number) => slot - (slot % COLUMNS);
export const pageOf = (slot: number) => Math.floor(slot / SLOTS_PER_PAGE);
// A wide tile takes two cells, unless the screen has a single column: then it is as wide as the page already.
export const spanOf = (size: SizeLike) => (asSize(size) === "full" ? SLOTS_PER_PAGE : asSize(size) === "wide" ? Math.min(2, COLUMNS) : 1);
export const cellsOf = (slot: number, size: SizeLike) =>
  asSize(size) === "full" ? Array.from({ length: SLOTS_PER_PAGE }, (_, i) => pageStart(slot) + i)
    : Array.from({ length: spanOf(size) }, (_, i) => slot + i);
// The cell a tile of `size` starts at when dropped on `slot`.
export const startOf = (slot: number, size: SizeLike) => (asSize(size) === "full" ? pageStart(slot) : asSize(size) === "wide" ? rowStart(slot) : slot);
export const entriesOf = (layout: Layout): Entry[] => layout.tiles.map((tile) => ({ tile, slot: tile.slot }));

// In-order packing: the rule before positions existed, and what firmware below 0.2.26 still draws.
export function packSlots(tiles: Tile[]) {
  let position = 0;
  return tiles.map((tile) => {
    const size = sizeOf(tile);
    if (size === "full" && position % SLOTS_PER_PAGE) position += SLOTS_PER_PAGE - (position % SLOTS_PER_PAGE);
    else if (size === "wide" && COLUMNS > 1 && position % COLUMNS === COLUMNS - 1) position++;
    const slot = position;
    position += spanOf(size);
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
  for (const { tile, slot } of entries) for (const cell of cellsOf(slot, sizeOf(tile))) taken.add(cell);
  return taken;
}
export const fits = (taken: Set<number>, slot: number, size: SizeLike) =>
  Number.isInteger(slot) && slot >= 0 && slot < MAX_SLOTS &&
  // A wide tile needs a cell beside it in the same row, wherever the screen's columns fall.
  (asSize(size) === "full" ? slot % SLOTS_PER_PAGE === 0
    : asSize(size) === "wide" ? slot + spanOf(size) <= MAX_SLOTS && slot % COLUMNS <= COLUMNS - spanOf(size) : true) &&
  cellsOf(slot, size).every((c) => !taken.has(c));
export function firstFree(taken: Set<number>, size: SizeLike, from = 0) {
  for (let slot = from; slot < MAX_SLOTS; slot++) if (fits(taken, slot, size)) return slot;
  return -1;
}
// The free position closest to `origin`; on a tie the later one, so a nudged tile moves down, not up.
export function nearestFree(taken: Set<number>, size: SizeLike, origin: number) {
  let best = -1;
  for (let slot = 0; slot < MAX_SLOTS; slot++)
    if (fits(taken, slot, size) && (best < 0 || Math.abs(slot - origin) <= Math.abs(best - origin))) best = slot;
  return best;
}
// The arrangement after putting `moving` (a tile on the grid, or a new one) at `target`:
// it lands exactly there; tiles in its way take the cells it left (a swap) or else the
// nearest free cell; everything else stays put. Null when the target is off the grid.
export function arrange(tiles: Tile[], moving: Tile, target: number): Entry[] | null {
  const size = sizeOf(moving);
  target = startOf(target, size);
  if (!fits(new Set(), target, size)) return null;
  const footprint = cellsOf(target, size);
  const vacated = tiles.includes(moving) ? cellsOf(moving.slot, size) : [];
  const result: Entry[] = [{ tile: moving, slot: target }];
  const displaced: Tile[] = [];
  for (const tile of tiles) {
    if (tile === moving) continue;
    if (cellsOf(tile.slot, sizeOf(tile)).some((c) => footprint.includes(c))) displaced.push(tile);
    else result.push({ tile, slot: tile.slot });
  }
  for (const tile of displaced) {
    const w = sizeOf(tile), taken = occupied(result);
    let slot = vacated.map((c) => startOf(c, w)).find((c) => fits(taken, c, w));
    if (slot === undefined) slot = nearestFree(taken, w, tile.slot);
    if (slot < 0) return null;
    result.push({ tile, slot });
  }
  return result.sort((a, b) => a.slot - b.slot);
}
// Pages the tiles need, or more when the user keeps empty pages on purpose (`layout.pages`).
export function pageCount(entries: Entry[], wanted = 1) {
  const last = Math.max(0, ...entries.map(({ tile, slot }) => slot + spanOf(sizeOf(tile))));
  return Math.min(MAX_PAGES, Math.max(1, Math.ceil(last / SLOTS_PER_PAGE), wanted || 1));
}
// With the page buttons and swiping both off (firmware 0.2.69+) only Go to page tiles change the page. Pages count
// from 1: `tiles` is how many Go to page tiles lead to a page the screen has, `targets` the pages they lead to,
// `unreachable` the pages no chain of them reaches from page 1, `noWayBack` the reachable ones they never lead back from.
export function strandedPages(entries: Entry[], pages: number) {
  const links = Array.from({ length: pages }, () => new Set<number>());
  let tiles = 0;
  for (const { tile, slot } of entries) {
    const to = pageTarget(tile.entity) - 1, from = pageOf(slot);
    if (to < 0 || to >= pages || from >= pages) continue;
    tiles++;
    links[from].add(to);
  }
  const reach = (next: (page: number) => number[]) => {
    const seen = new Set([0]), queue = [0];
    while (queue.length) for (const page of next(queue.shift()!)) if (!seen.has(page)) { seen.add(page); queue.push(page); }
    return seen;
  };
  const forward = reach((page) => [...links[page]]);
  const back = reach((page) => links.flatMap((to, from) => (to.has(page) ? [from] : [])));
  const all = Array.from({ length: pages }, (_, page) => page);
  const numbers = (list: number[]) => list.map((page) => page + 1);
  return {
    tiles,
    targets: numbers(all.filter((page) => links.some((to) => to.has(page)))),
    unreachable: numbers(all.filter((page) => !forward.has(page))),
    noWayBack: numbers(all.filter((page) => forward.has(page) && !back.has(page))),
  };
}
// New tiles start with the card that shows the entity best.
export function defaultOptions(id: string): Partial<Tile> {
  const domain = id.split(".")[0];
  if (domain === "sun") return { options: { display: "sunpath", size: "wide" } };
  if (domain === "weather") return { options: { display: "forecast", size: "wide" } };
  if (pageTarget(id)) return {};
  if (domain === "screen") return { options: { display: "digital", size: "wide" } };
  return {};
}
export const newTile = (id: string): Tile => ({ entity: id, name: "", slot: -1, ...defaultOptions(id) } as Tile);

// Same rule as the add-on: only a wide or full card in the standard layout shows direct controls;
// without a choice the domain's first control set applies to a wide card, none to a full one.
export function effectiveControls(tile: Tile, inventory: Inventory): string | null {
  const domain = tile.entity.split(".")[0], catalogue = inventory.controls?.[domain], o = tile.options || {};
  if (!catalogue || !["wide", "full"].includes(o.size as string) || !["standard", "cover"].includes((o.display || "standard") as string) || o.inline === "slider") return null;
  const choice = o.controls ?? (o.size === "full" ? "none" : catalogue.default);
  return choice === "none" ? null : choice;
}
export function controlsLabel(tile: Tile, inventory: Inventory) {
  const key = effectiveControls(tile, inventory);
  if (!key) return t("editor.inspect.control_none");
  // As the choice itself is labelled, like the other values in the summary: German writes its nouns with a capital.
  return inventory.controls?.[tile.entity.split(".")[0]]?.choices.find((c) => c.key === key)?.label || key;
}

export const parseVersion = (v: string | undefined | null) => (/^(\d+)\.(\d+)\.(\d+)$/.exec(v || "") || []).slice(1).map(Number);
export function versionAtLeast(version: string | undefined | null, minimum: string) {
  const [a, b] = [parseVersion(version), parseVersion(minimum)];
  if (a.length !== 3 || b.length !== 3) return false;
  for (let i = 0; i < 3; i++) if (a[i] !== b[i]) return a[i] > b[i];
  return true;
}
export const supportsFirmware = (firmware: string | undefined | null, major: number, minor: number, patch: number) =>
  versionAtLeast(firmware, `${major}.${minor}.${patch}`);
// Firmware 0.2.62 holds one tile per slot (48); 0.2.7 twenty; older firmware ten. The add-on tells the editor per
// screen (tile_limit, app 0.2.78); this rule stays for a screen entry without it.
export const MAX_TILES = MAX_SLOTS;
export function tileLimit(firmware: string | undefined | null) {
  if (parseVersion(firmware).length !== 3) return 10;
  return versionAtLeast(firmware, "0.2.62") ? MAX_TILES : versionAtLeast(firmware, "0.2.7") ? 20 : 10;
}
// What a tile shows and how big it is, in a few words (editor.displays, editor.sizes); a key it doesn't know stays as it is.
export const DISPLAYS = ["standard", "watch", "forecast", "graph", "digital", "analog", "sunpath", "live", "cover"];
export const displayName = (display: string) => (DISPLAYS.includes(display) ? t(`editor.displays.${display}`) : display);
export const sizeName = (size: string | undefined) => t(`editor.sizes.${SIZES.includes(size as Size) ? size : "single"}`);
export const TOGGLE_BEFORE = ["light", "switch", "input_boolean", "fan", "media_player", "climate"];
export const SLIDER_DOMAINS = ["light", "fan", "cover", "number", "input_number", "media_player"];

// Per domain its sign and colours; its name is the text editor.domains.<domain> (app 0.2.90).
export const domains: Record<string, [string, string, string]> = {
  light: ["☀", "#ad7600", "#fff3d3"],
  climate: ["❄", "#c86620", "#ffebdc"],
  vacuum: ["◉", "#008577", "#def3ed"],
  fan: ["✣", "#008aab", "#def5fa"],
  cover: ["▤", "#8053af", "#eee5f8"],
  media_player: ["▶", "#007cad", "#def2fc"],
  sensor: ["⌁", "#3476b1", "#e5effa"],
  binary_sensor: ["◈", "#ad7600", "#fff3d3"],
  switch: ["⏻", "#ad7600", "#fff3d3"],
  input_boolean: ["⏻", "#ad7600", "#fff3d3"],
  scene: ["✦", "#8053af", "#eee5f8"],
  script: ["▷", "#8053af", "#eee5f8"],
  weather: ["☁", "#007cad", "#def2fc"],
  number: ["±", "#008577", "#def3ed"],
  input_number: ["±", "#008577", "#def3ed"],
  select: ["≡", "#5862af", "#eaecfa"],
  input_select: ["≡", "#5862af", "#eaecfa"],
  button: ["↗", "#5862af", "#eaecfa"],
  input_button: ["↗", "#5862af", "#eaecfa"],
  screen: ["◷", "#25282c", "#e9ecf1"],
  sun: ["☼", "#c86620", "#ffebdc"],
  timer: ["⏱", "#008577", "#def3ed"],
  person: ["☺", "#2f7d32", "#e1f2e2"],
  camera: ["◧", "#3d4a57", "#e6ebf0"],
  image: ["◧", "#3d4a57", "#e6ebf0"],
};
// [name, sign, colour, background] of an entity's domain.
export function domainInfo(id: string): [string, string, string, string] {
  const domain = id.split(".")[0], known = domains[domain];
  return known ? [t(`editor.domains.${domain}`), ...known] : [t("editor.domains.entity"), "◇", "#637184", "#edf0f4"];
}
