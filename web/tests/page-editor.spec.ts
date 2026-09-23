import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { nextTick } from "vue";
import { mount } from "@vue/test-utils";
import LayoutView from "../src/components/LayoutView.vue";
import NavigationPreview from '../src/components/NavigationPreview.vue';
import { addPage, addTile, connectTile, copyLayoutFrom, importLayout, layoutJson, movePage, moveWorkspacePage, redo,
  acceptGridReview, gridChanged, refresh, reviewScreenGrid, save, saveWorkspace, select, setEditorMode, setHomePage, setPageExcluded, setPageTitle, setTopbarItems, state, undo, workspacePositions } from "../src/store";
import { documentFixture, screenFixture } from "./page-fixtures";
import type { PageDocument, Screen } from "../src/types";

const record = () => state.inventory.screens[0].page_document as PageDocument;
const reply = (body: unknown, status = 200) => new Response(JSON.stringify(body), { status });
beforeEach(() => {
  vi.useFakeTimers();
  vi.spyOn(window, "confirm").mockReturnValue(true);
  const preferences = new Map<string, string>();
  vi.stubGlobal("localStorage", { getItem: (key: string) => preferences.get(key) ?? null, setItem: (key: string, value: string) => preferences.set(key, value) });
  state.dirty = false; state.busy = false; state.workspaceDirty = false;
  state.inventory = { connected: true, screens: [screenFixture({ id: "test", name: "Test", firmware: "0.3.0", online: true,
    board: "guition", shape: { columns: 2, rows: 3, width: 480, height: 480 },
    layout: { title: "Home", pages: 2, tiles: [{ entity: "screen.page_2", name: "Controls", slot: 0 }] },
  } as Screen)], entities: [], icons: { groups: [], defaults: {}, weather: {}, sun: {}, controls: {}, fallback: "F0335" } } as any;
  state.selected = null;
  vi.stubGlobal("fetch", vi.fn(async () => reply({ states: {}, previews: [], capabilities: {} })));
  select("test");
});
afterEach(() => { vi.clearAllTimers(); vi.useRealTimers(); });

describe("one draft in both editor modes", () => {
  it('adds a library item only to the selected page and leaves other pages alone when full', () => {
    state.selectedPageId = state.document!.pages[1].id;
    addTile('light.selected');
    expect(state.document!.pages[0].tiles).toHaveLength(1);
    expect(state.document!.pages[1].tiles[0].content).toEqual({ kind: 'entity', entityId: 'light.selected' });
    for (let i = 0; i < 5; i++) addTile(`sensor.filling_${i}`);
    const before = JSON.stringify(state.document);
    addTile('sensor.overflow');
    expect(JSON.stringify(state.document)).toBe(before);
  });
  it('uses a page picker and named routes at phone widths without shrinking the map', async () => {
    const width = window.innerWidth;
    Object.defineProperty(window, 'innerWidth', { configurable: true, value: 390 });
    const view = mount(LayoutView);
    try {
      expect(view.findAll('#layout-preview .device')).toHaveLength(1);
      await view.find('.mobile-page-picker select').setValue(state.document!.pages[1].id);
      expect(view.find('#layout-preview .page').attributes('data-page-id')).toBe(state.document!.pages[1].id);
      setEditorMode('advanced'); await nextTick();
      expect(view.find('.map-list').exists()).toBe(true);
      expect(view.find('.map-scroll').exists()).toBe(false);
      expect(view.findAll('.map-list-route')).toHaveLength(2);
      expect(view.find('#library').exists()).toBe(true);
    } finally { Object.defineProperty(window, 'innerWidth', { configurable: true, value: width }); view.unmount(); }
  });
  it('reviews a changed grid before applying it, and undo restores the source geometry', () => {
    const original = JSON.stringify(state.document), revision = state.documentRevision;
    Object.assign(state.inventory.screens[0].shape!, { columns: 1, rows: 4 });
    expect(gridChanged.value).toBe(true);
    reviewScreenGrid();
    expect(state.gridReview?.target).toEqual({ columns: 1, rows: 4 });
    expect(state.documentGrid).toEqual({ columns: 2, rows: 3 });
    expect(JSON.stringify(state.document)).toBe(original);
    expect(state.dirty).toBe(false);
    acceptGridReview();
    expect(state.documentGrid).toEqual({ columns: 1, rows: 4 });
    expect(state.documentRevision).toBe(revision);
    expect(state.dirty).toBe(true);
    undo();
    expect(state.documentGrid).toEqual({ columns: 2, rows: 3 });
    expect(JSON.stringify(state.document)).toBe(original);
    expect(state.dirty).toBe(false);
    redo(); expect(state.documentGrid).toEqual({ columns: 1, rows: 4 });
  });
  it('previews navigation locally without modifying the draft, selection, or calling HA', async () => {
    vi.spyOn(HTMLDialogElement.prototype, 'showModal').mockImplementation(() => {});
    const document = JSON.stringify(state.document), selected = state.selectedTile;
    const view = mount(NavigationPreview);
    await nextTick();
    vi.mocked(fetch).mockClear();
    expect(view.findAll('.remove')).toHaveLength(0);
    expect(view.find('.page-navigation span').text()).toBe('1 / 2');
    await view.find('.tile').trigger('click');
    expect(view.find('.page-navigation span').text()).toBe('2 / 2');
    await view.find('.preview-home').trigger('click');
    expect(view.find('.page-navigation span').text()).toBe('1 / 2');
    expect(state.selectedTile).toBe(selected);
    expect(JSON.stringify(state.document)).toBe(document);
    expect(state.dirty).toBe(false);
    expect(fetch).not.toHaveBeenCalled();
  });
  it('keeps the footer on detail pages and returns without needing a tile', async () => {
    state.document!.pages[1].navigation.excludeFromPagination = true;
    const view = mount(NavigationPreview);
    expect(view.find('.page-navigation').exists()).toBe(true);
    await view.find('.tile').trigger('click');
    expect(view.find('.page-back').exists()).toBe(true);
    expect(view.find('.page-navigation').text()).not.toContain('/');
    await view.find('.page-back').trigger('click');
    expect(view.find('.page').attributes('data-page-id')).toBe(state.document!.homePageId);
  });
  it('replaces Home with Back when the footer is hidden, even if Home was disabled', async () => {
    state.document!.pages[1].navigation.excludeFromPagination = true;
    state.inventory.screens[0].settings = { values: { page_buttons: false, home_button: false } } as any;
    const view = mount(NavigationPreview);
    expect(view.find('.page-navigation').exists()).toBe(false);
    expect(view.find('.preview-home').exists()).toBe(false);
    await view.find('.tile').trigger('click');
    expect(view.find('.page-navigation').exists()).toBe(false);
    expect(view.find('.preview-home').attributes('aria-label')).toMatch(/Back|Terug/);
    await view.find('.preview-home').trigger('click');
    expect(view.find('.page').attributes('data-page-id')).toBe(state.document!.homePageId);
  });
  it("changes only editor state when switching modes and retains the same undo history", () => {
    const home = state.document!.homePageId;
    setHomePage(state.document!.pages[1].id);
    const draft = JSON.stringify(state.document), count = state.undoCount;
    setEditorMode("advanced"); setEditorMode("simple");
    expect(JSON.stringify(state.document)).toBe(draft);
    expect(state.undoCount).toBe(count);
    undo(); expect(state.document!.homePageId).toBe(home);
    redo(); expect(JSON.stringify(state.document)).toBe(draft);
  });
  it("unmounts every connection and port when returning to Simple", async () => {
    setEditorMode("advanced");
    const view = mount(LayoutView);
    expect(view.find(".map-links").exists()).toBe(true);
    state.connectingTileId = state.layout!.tiles[0].id!;
    setEditorMode("simple"); await nextTick();
    expect(view.find(".map-links").exists()).toBe(false);
    expect(view.find(".connection-port").exists()).toBe(false);
    expect(state.connectingTileId).toBeNull();
    expect(view.find("#library").exists()).toBe(true);
  });
  it("keeps Home, links, exclusions and per-page bars attached through a reorder and undo", () => {
    const [home, controls] = state.document!.pages.map((page) => page.id);
    setHomePage(controls); setPageExcluded(controls, true);
    state.barPage = 1; setTopbarItems([{ type: "date" }]);
    const before = JSON.stringify(state.document);
    movePage(1, 0);
    expect(state.document!.homePageId).toBe(controls);
    expect(state.document!.pages[0]).toMatchObject({ id: controls, navigation: { excludeFromPagination: true }, topbar: { trailing: [{ type: "date" }] } });
    expect(state.document!.pages[1]).toMatchObject({ id: home, tiles: [{ content: { target: { pageId: controls } } }] });
    expect(state.barPage).toBe(0);
    undo(); expect(JSON.stringify(state.document)).toBe(before);
  });
  it("creates a navigation destination and tile together and undoes both together", () => {
    const before = JSON.stringify(state.document);
    addTile("screen.page_5");
    expect(state.document!.pages).toHaveLength(5);
    expect(state.document!.pages[0].tiles[1].content).toEqual({ kind: "navigation", target: { kind: "page", pageId: state.document!.pages[4].id } });
    undo(); expect(JSON.stringify(state.document)).toBe(before);
  });
  it("keeps the saved old-screen notice until the actual capability arrives", async () => {
    state.inventory.screens[0].page_capability = "update_screen";
    const view = mount(LayoutView);
    expect(view.text()).toContain("Update screen to use the new titlebar and layout");
    const before = JSON.stringify(state.document);
    state.inventory.screens[0].page_capability = "ready"; await nextTick();
    expect(view.text()).not.toContain("Update screen to use the new titlebar and layout");
    expect(JSON.stringify(state.document)).toBe(before);
    expect(state.dirty).toBe(false);
  });
  it('preserves an unsaved draft when one of two screens upgrades a week later', async () => {
    const original = JSON.parse(JSON.stringify(state.inventory.screens[0]));
    state.inventory.screens[0].page_capability = 'update_screen';
    setPageTitle(1, 'Unsent title');
    setEditorMode('advanced');
    const selected = state.document!.pages[1].id;
    state.selectedPageId = selected;
    moveWorkspacePage(selected, 3, 2);
    const draft = JSON.stringify(state.document), workspace = JSON.stringify(state.workspace);
    vi.setSystemTime(Date.now() + 7 * 24 * 60 * 60 * 1000);
    const fetch = vi.fn(async (_url: string) => reply({ connected: true, screens: [
      { ...original, firmware: '0.3.0', page_capability: 'ready' },
      { ...original, id: 'other', firmware: '0.2.104', page_capability: 'update_screen' },
    ] }));
    vi.stubGlobal('fetch', fetch);
    await refresh(false);
    expect(JSON.stringify(state.document)).toBe(draft);
    expect(JSON.stringify(state.workspace)).toBe(workspace);
    expect(state.selectedPageId).toBe(selected);
    expect(state.dirty).toBe(true);
    expect(state.workspaceDirty).toBe(true);
    expect(state.inventory.screens.map((screen) => screen.page_capability)).toEqual(['ready', 'update_screen']);
    expect(fetch).toHaveBeenCalledTimes(1);
    expect(fetch.mock.calls[0]?.[0]).toBe('api/inventory?light=1');
  });
});

describe("editor positions never alter firmware configuration", () => {
  it("materializes positions once and keeps them stable when Home, links or order change", () => {
    setEditorMode("advanced");
    const before = workspacePositions(), second = state.document!.pages[1].id;
    setHomePage(second); connectTile(state.layout!.tiles[0].id!, "home"); movePage(0, 1);
    expect(workspacePositions()).toEqual(before);
    addPage();
    expect(Object.fromEntries(Object.entries(workspacePositions()).filter(([id]) => id in before))).toEqual(before);
  });
  it("saves a map move through the workspace endpoint, with both expected revisions and no layout", async () => {
    setEditorMode("advanced");
    const layout = JSON.stringify(state.document), revision = state.documentRevision;
    moveWorkspacePage(state.document!.homePageId, 3, 2);
    expect(state.dirty).toBe(false);
    const fetch = vi.fn(async (url: string, options?: RequestInit) => {
      expect(url).toBe("api/screens/test/workspace");
      const body = JSON.parse(String(options?.body));
      expect(body).toEqual({ revision, workspace: state.workspace });
      return reply({ ...body.workspace, revision: "new-workspace" });
    });
    vi.stubGlobal("fetch", fetch);
    await saveWorkspace();
    expect(fetch).toHaveBeenCalledTimes(1);
    expect(state.workspaceDirty).toBe(false);
    expect(state.documentRevision).toBe(revision);
    expect(JSON.stringify(state.document)).toBe(layout);
  });
  it("rejects overlapping map positions without changing the document or positions", () => {
    setEditorMode("advanced");
    const before = JSON.stringify(state.workspace.positions), [first, second] = state.document!.pages;
    const destination = workspacePositions()[second.id];
    moveWorkspacePage(first.id, destination.x, destination.y);
    expect(JSON.stringify(state.workspace.positions)).toBe(before);
    expect(state.dirty).toBe(false);
  });
});

describe("revisions and portable layouts", () => {
  it("keeps a rejected save as a draft and exposes the competing revision", async () => {
    addPage(); const draft = JSON.stringify(state.document);
    const other = { ...record(), revision: "changed-elsewhere" };
    vi.stubGlobal("fetch", vi.fn(async (_url: string, options?: RequestInit) => options?.method === "PUT"
      ? reply({ error: "Layout changed" }, 409) : reply({ screens: [{ ...state.inventory.screens[0], page_document: other }] })));
    await save();
    expect(JSON.stringify(state.document)).toBe(draft);
    expect(state.dirty).toBe(true); expect(state.conflict).toBe(true);
    expect(state.documentRevision).toBe(record().revision);
  });
  it("recovers a committed save whose HTTP response was lost without resending it", async () => {
    addPage(); const draft = JSON.parse(JSON.stringify(state.document));
    const server = { ...record(), revision: "committed", layout: draft };
    const fetch = vi.fn(async (_url: string, options?: RequestInit) => {
      if (options?.method === "PUT") throw new Error("Connection lost");
      return reply({ screens: [{ ...state.inventory.screens[0], page_document: server }] });
    });
    vi.stubGlobal("fetch", fetch); await save();
    expect(state.dirty).toBe(false); expect(state.documentRevision).toBe("committed");
    expect(fetch.mock.calls.filter(([, options]) => options?.method === "PUT")).toHaveLength(1);
  });
  it("exports only portable layout and editor placement, and copies with fresh IDs", () => {
    (state.inventory.screens[0] as any).api_key = "private-key-never-export";
    state.inventory.screens.push(screenFixture({ ...state.inventory.screens[0], id: "source", name: "Source" }));
    const source = state.inventory.screens[1].page_document as PageDocument;
    copyLayoutFrom("source");
    const exported = JSON.parse(layoutJson());
    expect(Object.keys(exported).sort()).toEqual(["editor", "esp_screens_layout", "layout", "sourceGrid"]);
    expect(layoutJson()).not.toContain("private-key-never-export");
    expect(state.document!.pages[0].id).not.toBe(source.layout.pages[0].id);
    expect(state.document!.pages[0].tiles[0].content).toEqual({ kind: "navigation", target: { kind: "page", pageId: state.document!.pages[1].id } });
  });
  it("does not apply an import response to a different screen opened while it was checked", async () => {
    let resolve!: (value: Response) => void;
    const imported = documentFixture({ title: "Imported", tiles: [] });
    vi.stubGlobal("fetch", vi.fn((url: string) => url.endsWith("/import") ? new Promise<Response>((done) => { resolve = done; }) : Promise.resolve(reply({}))));
    const pending = importLayout(JSON.stringify({ esp_screens_layout: 2, sourceGrid: imported.sourceGrid, layout: imported.layout }));
    state.inventory.screens.push(screenFixture({ ...state.inventory.screens[0], id: "other", name: "Other" }));
    select("other"); const before = JSON.stringify(state.document);
    resolve(reply(imported)); await pending;
    expect(JSON.stringify(state.document)).toBe(before); expect(state.dirty).toBe(false);
  });
});
