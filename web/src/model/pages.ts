/** Page document operations. Neither editor mode nor a live HA value owns configuration.
 *
 * All operations return a complete validated replacement. Tile/card renderers
 * consume a derived view with stable instance IDs. Only the persistence and
 * delivery boundaries translate to another format.
 */
import type { HeaderItem, Layout, Page, PageGrid, PageLayout, PageTarget, PageTile, Tile, TileOptions } from "../types";

export const clone = <T>(value: T): T => JSON.parse(JSON.stringify(value));
export const instanceId = () => [...crypto.getRandomValues(new Uint8Array(8))].map((b) => b.toString(16).padStart(2, "0")).join("");
export const pageLimit = (grid: PageGrid) => Math.min(8, Math.floor(64 / (grid.columns * grid.rows)));
export const sameGrid = (a: PageGrid, b: PageGrid) => a.columns === b.columns && a.rows === b.rows;
const byteLength = (value: string) => new TextEncoder().encode(value).length;

export function emptyPage(bar?: Page["topbar"]): Page {
  return {
    id: instanceId(), navigation: { excludeFromPagination: false }, tiles: [],
    topbar: bar ? copyBar(bar) : {
      leading: [{ id: instanceId(), kind: "home" }], title: { source: "screen" },
      trailing: [{ id: instanceId(), type: "clock" }],
    },
  };
}
export function emptyLayout(title: string): PageLayout {
  const page = emptyPage();
  return { title, homePageId: page.id, pages: [page] };
}
export function copyBar(bar: Page["topbar"]): Page["topbar"] {
  return { title: clone(bar.title), leading: bar.leading.map((item) => ({ ...item, id: instanceId() })),
    trailing: bar.trailing.map((item) => ({ ...clone(item), id: instanceId() })) };
}
export const destination = (layout: PageLayout, target: PageTarget) => target.kind === "home" ? layout.homePageId : target.pageId;
export const titleOf = (layout: PageLayout, page: Page) => page.topbar.title.source === "text" ? page.topbar.title.text : layout.title;
export const pagination = (layout: PageLayout) => layout.pages.filter((page) => !page.navigation.excludeFromPagination).map((page) => page.id);
export function sequentialTarget(layout: PageLayout, current: string, direction: -1 | 1) {
  const sequence = pagination(layout), index = sequence.indexOf(current);
  return index < 0 ? current : sequence[index + direction] ?? current;
}

export type NavigationSettings = { pageButtons: boolean; swipe: boolean; homeButton: boolean };
export type NavigationIntent = { kind: "home" | "back" } | { kind: "tile"; tileId: string }
  | { kind: "previous" | "next" | "swipe-previous" | "swipe-next" };
/** Shared by the preview and reachability checks. Explicit links are independent
 * of the filtered sequence; excluded pages never acquire a sequential neighbour.
 */
export function navigationTarget(layout: PageLayout, current: string, intent: NavigationIntent, settings: NavigationSettings) {
  const page = layout.pages.find((page) => page.id === current);
  if (!page) return layout.homePageId;
  if (intent.kind === "back") return page.navigation.excludeFromPagination ? layout.homePageId : current;
  if (intent.kind === "home") return settings.homeButton && page.topbar.leading.some((item) => item.kind === "home") ? layout.homePageId : current;
  if (intent.kind === "tile") {
    const tile = page.tiles.find((tile) => tile.id === intent.tileId);
    return tile?.content.kind === "navigation" ? destination(layout, tile.content.target) : current;
  }
  const allowed = intent.kind.startsWith("swipe-") ? settings.swipe : settings.pageButtons;
  return allowed ? sequentialTarget(layout, current, intent.kind.endsWith("previous") ? -1 : 1) : current;
}
/** Same eight-ID route as firmware. Back never pushes another return entry. */
export function navigationStep(layout: PageLayout, current: string, history: string[], intent: NavigationIntent, settings: NavigationSettings) {
  const next = history.slice(-8);
  if (intent.kind === 'back') {
    if (!layout.pages.find((page) => page.id === current)?.navigation.excludeFromPagination) return { current, history: next };
    while (next.length) {
      const target = next.pop()!;
      if (target !== current && layout.pages.some((page) => page.id === target)) return { current: target, history: next };
    }
    return { current: layout.homePageId, history: next };
  }
  const target = navigationTarget(layout, current, intent, settings);
  if (intent.kind === 'home' && target === layout.homePageId) next.length = 0;
  if (intent.kind === 'tile' && target !== current) { next.push(current); if (next.length > 8) next.shift(); }
  return { current: target, history: next };
}
export const navigationFooter = (layout: PageLayout, settings: NavigationSettings) => layout.pages.length > 1 && settings.pageButtons;
export function reachability(layout: PageLayout, settings: NavigationSettings) {
  const edges = new Map(layout.pages.map((page) => [page.id, new Set<string>()]));
  for (const page of layout.pages) {
    const intents: NavigationIntent[] = [{ kind: "home" }, { kind: "back" }, { kind: "previous" }, { kind: "next" }, { kind: "swipe-previous" }, { kind: "swipe-next" },
      ...page.tiles.map((tile): NavigationIntent => ({ kind: "tile", tileId: tile.id }))];
    for (const intent of intents) edges.get(page.id)!.add(navigationTarget(layout, page.id, intent, settings));
  }
  function visit(reverse: boolean) {
    const seen = new Set([layout.homePageId]), queue = [layout.homePageId];
    for (let i = 0; i < queue.length; i++) {
      const next = reverse ? [...edges].filter(([, targets]) => targets.has(queue[i])).map(([id]) => id) : [...edges.get(queue[i])!];
      for (const id of next) if (!seen.has(id)) { seen.add(id); queue.push(id); }
    }
    return seen;
  }
  const reachable = visit(false), returns = visit(true);
  return { unreachable: layout.pages.filter((page) => !reachable.has(page.id)).map((page) => page.id),
    noWayHome: layout.pages.filter((page) => reachable.has(page.id) && !returns.has(page.id)).map((page) => page.id) };
}

export function footprintSize(tile: PageTile, grid: PageGrid): "single" | "wide" | "full" {
  const { columns, rows } = tile.placement;
  const presentation = tile.appearance.presentation;
  if (presentation) {
    const sizes = { single: [1, 1], wide: [Math.min(2, grid.columns), 1], full: [grid.columns, grid.rows] };
    const size = sizes[presentation];
    if (!size || size[0] !== columns || size[1] !== rows) throw new Error("This tile presentation and footprint require a future screen capability");
    return presentation;
  }
  if (columns === 1 && rows === 1) return "single";
  if (columns === Math.min(2, grid.columns) && rows === 1) return "wide";
  if (columns === grid.columns && rows === grid.rows) return "full";
  throw new Error("This tile footprint requires a future screen capability");
}
/** An explicit review proposal. It never changes page membership, drops a tile,
 * adds capacity or retargets links. The caller must show it before adoption.
 */
export function adaptGrid(layout: PageLayout, source: PageGrid, target: PageGrid): PageLayout {
  validatePages(layout, source);
  if (layout.pages.length > pageLimit(target)) throw new Error('This layout has more pages than the destination screen supports');
  const draft = clone(layout);
  for (const page of draft.pages) {
    const occupied = new Set<string>(), pending: PageTile[] = [];
    const fits = (tile: PageTile, row: number, column: number) => {
      const { rows, columns } = tile.placement;
      if (row + rows > target.rows || column + columns > target.columns) return false;
      for (let y = row; y < row + rows; y++) for (let x = column; x < column + columns; x++) if (occupied.has(`${y}:${x}`)) return false;
      return true;
    };
    const place = (tile: PageTile, row: number, column: number) => {
      Object.assign(tile.placement, { row, column });
      for (let y = row; y < row + tile.placement.rows; y++) for (let x = column; x < column + tile.placement.columns; x++) occupied.add(`${y}:${x}`);
    };
    for (const tile of page.tiles) {
      const presentation = footprintSize(tile, source);
      tile.appearance.presentation = presentation;
      tile.placement.columns = presentation === 'full' ? target.columns : presentation === 'wide' ? Math.min(2, target.columns) : 1;
      tile.placement.rows = presentation === 'full' ? target.rows : 1;
      if (fits(tile, tile.placement.row, tile.placement.column)) place(tile, tile.placement.row, tile.placement.column);
      else pending.push(tile);
    }
    for (const tile of pending) {
      let found = false;
      for (let row = 0; row < target.rows && !found; row++) for (let column = 0; column < target.columns && !found; column++) {
        if (fits(tile, row, column)) { place(tile, row, column); found = true; }
      }
      if (!found) throw new Error('A page has more tiles than the destination grid can hold. No tiles were removed.');
    }
  }
  return validatePages(draft, target);
}
export function entityOf(layout: PageLayout, tile: PageTile): string {
  const content = tile.content;
  if (content.kind === "entity") return content.entityId;
  if (content.kind === "builtin") return `screen.${content.name}`;
  const index = layout.pages.findIndex((page) => page.id === destination(layout, content.target));
  if (index < 0) throw new Error("The navigation destination no longer exists");
  return `screen.page_${index + 1}`;
}
const appearanceKeys = { display: "display", icon: "icon", background: "background", historyHours: "history_hours", refresh: "refresh", subtitle: "sub" } as const;

/** A render view, never a second saved or editable layout. */
export function projectLayout(layout: PageLayout, grid: PageGrid): Layout {
  const titles = layout.pages.map((page) => page.topbar.title.source === "text" ? page.topbar.title.text : "");
  while (titles.length && !titles.at(-1)) titles.pop();
  return {
    title: layout.title, pages: layout.pages.length,
    ...(titles.length ? { page_titles: titles } : {}),
    tiles: layout.pages.flatMap((page, index) => page.tiles.map((tile): Tile => {
      const options: TileOptions = { ...clone(tile.interaction) }, size = footprintSize(tile, grid);
      for (const [key, wire] of Object.entries(appearanceKeys)) {
        const value = tile.appearance[key as keyof typeof appearanceKeys];
        if (value !== undefined) Object.assign(options, { [wire]: clone(value) });
      }
      if (size !== "single") options.size = size;
      return { id: tile.id, entity: entityOf(layout, tile), name: tile.appearance.label,
        slot: index * grid.columns * grid.rows + tile.placement.row * grid.columns + tile.placement.column,
        ...(Object.keys(options).length ? { options } : {}) };
    })).sort((a, b) => a.slot - b.slot),
  };
}

export function validatePages(layout: PageLayout, grid: PageGrid): PageLayout {
  if (![grid.columns, grid.rows].every((n) => Number.isInteger(n) && n > 0) || grid.columns * grid.rows > 64)
    throw new Error("The screen grid is not known");
  if (!layout.pages.length || layout.pages.length > pageLimit(grid)) throw new Error("This screen has no room for another page");
  if (!layout.pages.some((page) => page.id === layout.homePageId)) throw new Error("Home must be an existing page");
  const ids = new Set<string>(), entities = new Set<string>();
  const identity = (id: string, page = false) => {
    if (!(page ? /^[0-9a-f]{16}$/ : /^[a-zA-Z0-9_-]{1,64}$/).test(id) || ids.has(id)) throw new Error("Invalid or duplicate instance ID");
    ids.add(id);
  };
  for (const page of layout.pages) identity(page.id, true);
  for (const page of layout.pages) {
    const bar = page.topbar;
    if (typeof page.navigation.excludeFromPagination !== "boolean") throw new Error("Invalid page navigation setting");
    if (bar.leading.length > 1 || bar.trailing.length > 6) throw new Error("Too many top-bar items");
    if (bar.title.source !== "screen" && (bar.title.source !== "text" || !bar.title.text.trim() || byteLength(bar.title.text) > 96))
      throw new Error("The page title must contain at most 96 UTF-8 bytes");
    for (const item of bar.leading) {
      identity(item.id);
      if (item.kind !== "home") throw new Error("Unsupported leading control");
    }
    for (const item of bar.trailing) identity(item.id);
    const occupied = new Set<number>();
    for (const tile of page.tiles) {
      identity(tile.id);
      const { row, column, columns, rows } = tile.placement;
      if (![row, column, columns, rows].every(Number.isInteger) || row < 0 || column < 0 || columns < 1 || rows < 1 ||
          row + rows > grid.rows || column + columns > grid.columns) throw new Error("A tile must fit inside its page");
      footprintSize(tile, grid);
      for (let y = row; y < row + rows; y++) for (let x = column; x < column + columns; x++) {
        const cell = y * grid.columns + x;
        if (occupied.has(cell)) throw new Error("Tiles cannot overlap");
        occupied.add(cell);
      }
      const entity = entityOf(layout, tile);
      if (tile.content.kind !== "navigation") {
        if (entities.has(entity)) throw new Error("This entity already has a tile on this screen");
        entities.add(entity);
      }
    }
  }
  return layout;
}

export function changePages(layout: PageLayout, grid: PageGrid, apply: (draft: PageLayout) => void): PageLayout {
  const draft = clone(layout);
  apply(draft);
  return validatePages(draft, grid);
}
export function reorderPage(layout: PageLayout, grid: PageGrid, pageId: string, to: number) {
  return changePages(layout, grid, (draft) => {
    const from = draft.pages.findIndex((page) => page.id === pageId);
    if (from < 0 || !Number.isInteger(to) || to < 0 || to >= draft.pages.length) throw new Error("Invalid page order");
    draft.pages.splice(to, 0, ...draft.pages.splice(from, 1));
  });
}
export function deletePage(layout: PageLayout, grid: PageGrid, pageId: string) {
  return changePages(layout, grid, (draft) => {
    if (draft.pages.length === 1 || !draft.pages.some((page) => page.id === pageId)) throw new Error("Cannot delete this page");
    draft.pages = draft.pages.filter((page) => page.id !== pageId);
    if (draft.homePageId === pageId) draft.homePageId = draft.pages[0].id;
    for (const page of draft.pages) page.tiles = page.tiles.filter((tile) => tile.content.kind !== "navigation" ||
      tile.content.target.kind !== "page" || tile.content.target.pageId !== pageId);
  });
}
export function duplicatePage(layout: PageLayout, grid: PageGrid, pageId: string, empty = false) {
  return changePages(layout, grid, (draft) => {
    const index = draft.pages.findIndex((page) => page.id === pageId);
    if (index < 0) throw new Error("The page no longer exists");
    const source = draft.pages[index], copy = emptyPage(source.topbar);
    if (!empty) {
      copy.navigation = clone(source.navigation);
      copy.tiles = source.tiles.map((tile) => {
        const next = { ...clone(tile), id: instanceId() };
        if (next.content.kind === "navigation" && next.content.target.kind === "page" && next.content.target.pageId === pageId)
          next.content.target.pageId = copy.id;
        return next;
      });
    }
    draft.pages.splice(index + 1, 0, copy);
  });
}
export function replaceBar(layout: PageLayout, grid: PageGrid, source: string, targets: string[], whole = true) {
  return changePages(layout, grid, (draft) => {
    const bar = draft.pages.find((page) => page.id === source)?.topbar;
    if (!bar) throw new Error("The source page no longer exists");
    for (const id of targets) {
      const page = draft.pages.find((item) => item.id === id);
      if (!page) throw new Error("The destination page no longer exists");
      if (id !== source) {
        const copy = copyBar(bar);
        if (whole) page.topbar = copy;
        else page.topbar.trailing = copy.trailing;
      }
    }
  });
}

/** Apply existing card-placement calculations as one document operation.
 * Instance IDs, not entity names or array indexes, identify existing tiles.
 */
export function arrangeTiles(layout: PageLayout, grid: PageGrid, entries: { tile: Tile; slot: number }[]) {
  return changePages(layout, grid, (draft) => {
    const cells = grid.columns * grid.rows, existing = new Map(draft.pages.flatMap((page) => page.tiles.map((tile) => [tile.id, tile] as const)));
    const required = Math.max(draft.pages.length, ...entries.map(({ slot }) => Math.floor(slot / cells) + 1));
    if (required > pageLimit(grid)) throw new Error("This screen has no room for another page");
    while (draft.pages.length < required) draft.pages.push(emptyPage(draft.pages.at(-1)!.topbar));
    for (const page of draft.pages) page.tiles = [];
    for (const { tile, slot } of entries) {
      if (!Number.isInteger(slot) || slot < 0) throw new Error("Invalid tile position");
      const old = tile.id ? existing.get(tile.id) : undefined, options = tile.options || {};
      if (tile.id && !old) throw new Error("The tile no longer exists");
      const target = /^screen\.page_([1-8])$/.exec(tile.entity);
      let content: PageTile["content"];
      if (target) {
        const page = draft.pages[Number(target[1]) - 1];
        if (!page) throw new Error("The navigation destination no longer exists");
        content = old?.content.kind === "navigation" && old.content.target.kind === "home" && entityOf(layout, old) === tile.entity
          ? clone(old.content) : { kind: "navigation", target: { kind: "page", pageId: page.id } };
      } else if (tile.entity === "screen.clock" || tile.entity === "screen.settings") {
        content = { kind: "builtin", name: tile.entity.slice(7) as "clock" | "settings" };
      } else content = { kind: "entity", entityId: tile.entity };
      const size = options.size ?? "single";
      if (!["single", "wide", "full"].includes(size)) throw new Error("Unsupported tile size");
      const appearance: PageTile["appearance"] = { label: tile.name };
      if (size !== "single") appearance.presentation = size as "wide" | "full";
      for (const [key, wire] of Object.entries(appearanceKeys)) {
        if (options[wire] !== undefined) Object.assign(appearance, { [key]: clone(options[wire]) });
      }
      const interaction: PageTile["interaction"] = {};
      for (const key of ["tap", "inline", "controls", "action"] as const) {
        if (options[key] !== undefined) Object.assign(interaction, { [key]: clone(options[key]) });
      }
      draft.pages[Math.floor(slot / cells)].tiles.push({ id: old?.id || instanceId(), content, appearance, interaction,
        placement: { row: Math.floor((slot % cells) / grid.columns), column: slot % grid.columns,
          columns: size === "full" ? grid.columns : size === "wide" ? Math.min(2, grid.columns) : 1,
          rows: size === "full" ? grid.rows : 1 } });
    }
  });
}

export function remapLayout(layout: PageLayout, grid: PageGrid) {
  const result = clone(layout), ids = new Map(result.pages.map((page) => [page.id, instanceId()]));
  result.homePageId = ids.get(result.homePageId)!;
  for (const page of result.pages) {
    page.id = ids.get(page.id)!;
    page.topbar = copyBar(page.topbar);
    for (const tile of page.tiles) {
      tile.id = instanceId();
      if (tile.content.kind === "navigation" && tile.content.target.kind === "page") {
        const id = ids.get(tile.content.target.pageId);
        if (!id) throw new Error("The navigation destination no longer exists");
        tile.content.target.pageId = id;
      }
    }
  }
  return validatePages(result, grid);
}

export type PageConnection = { tileId: string; from: string; to: string; home: boolean };
export function connections(layout: PageLayout): PageConnection[] {
  return layout.pages.flatMap((page) => page.tiles.flatMap((tile) => tile.content.kind === "navigation"
    ? [{ tileId: tile.id, from: page.id, to: destination(layout, tile.content.target), home: tile.content.target.kind === "home" }] : []));
}
export function initialPositions(layout: PageLayout): Record<string, { x: number; y: number }> {
  const routes = connections(layout), levels = new Map([[layout.homePageId, 0]]), queue = [layout.homePageId];
  for (let i = 0; i < queue.length; i++) {
    const id = queue[i];
    for (const route of routes.filter((route) => route.from === id)) if (!levels.has(route.to)) {
      levels.set(route.to, levels.get(id)! + 1); queue.push(route.to);
    }
  }
  const orphanRow = Math.max(...levels.values()) + 1, columns = new Map<number, number>();
  return Object.fromEntries([layout.pages.find((page) => page.id === layout.homePageId)!, ...layout.pages.filter((page) => page.id !== layout.homePageId)]
    .map((page) => {
      const y = levels.get(page.id) ?? orphanRow, x = columns.get(y) ?? 0;
      columns.set(y, x + 1);
      return [page.id, { x, y }];
    }));
}

/** Shared top-bar edit while old firmware is connected, still independent copies. */
export function setBarItems(layout: PageLayout, grid: PageGrid, pageId: string, items: HeaderItem[], allPages = false) {
  return changePages(layout, grid, (draft) => {
    for (const page of draft.pages) if (allPages || page.id === pageId) {
      const owned = new Set(page.topbar.trailing.map((item) => item.id));
      page.topbar.trailing = items.map((item) => ({ ...clone(item), id: item.id && owned.has(item.id) ? item.id : instanceId() }));
    }
  });
}
