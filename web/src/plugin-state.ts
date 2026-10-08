// Plugins (docs/PLUGINS.md): what the add-on offers and what each screen runs, shared by the Plugins page (every plugin, and on
// which screens it is) and a screen's own Plugins tab (what this screen has, and what it can add). A plugin lives in a
// screen's firmware, so every change is per screen; the page only sets several screens at once.
import { reactive } from "vue";
import { getJson, send } from "./api";
import { t } from "./i18n";
import { choiceKey, fit, knowTileTypes, pluginTileId, testPlugin, text, type Installed, type Plugin,
  type PluginTileOption, type Texts, pluginTileOf, pluginDefaults} from "./model/plugins";
import { pluginTiles } from "./model/page-validation";
import { computed, watch } from "vue";
import { copyText, state, toast } from "./store";
import type { Screen } from "./types";

export const plugins = reactive({
  index: [] as Plugin[],
  installed: {} as Record<string, Installed[]>,
  // Screen node → the plugins its build is adding or updating right now.
  building: {} as Record<string, string[]>,
  loaded: false,
  // What a person filled in when adding a plugin (inputs) and which optional parts are on, per screen node and plugin.
  // A secret is kept by the add-on and never comes back to the page; here it only says that one is set.
  values: {} as Record<string, Record<string, Record<string, string>>>,
  parts: {} as Record<string, Record<string, string[]>>,
  attached: {} as Record<string, boolean>,
  // From the add-on: which secrets are set (never their values), the screens whose build is running, the plugins file
  // of a screen with its own YAML, and the lists of choices it fetched.
  secrets: {} as Record<string, Record<string, boolean>>,
  jobs: {} as Record<string, { add: string[]; remove: string[]; state?: string }>,
  files: {} as Record<string, { file: string; content: string; line: string }>,
  choices: {} as Record<string, { value: string; label: Texts }[]>,
  // What the add-on drew of a tile's data, by plugin tile and its options (previewFor), and when it was asked.
  previews: {} as Record<string, { items: PreviewRow[]; at: number }>,
  // Entities of the domains the plugins name that the editor's own list lacks (a calendar), from the add-on.
  entities: [] as { id: string; name: string }[],
  // Per plugin: the person agreed to the other rights an update asks for (needsConsent).
  consented: {} as Record<string, boolean>,
  folders: { path: "", errors: {} as Record<string, string> },
});

// Plugins are on in the dev app (editor_features.plugins, docs/PLUGINS.md) and in `npm run dev`: a person with a
// released add-on never sees the page or the tab.
export const pluginsEnabled = computed(() => import.meta.env.DEV || state.inventory.editor_features?.plugins === true);
// The layout model, the memory price and the tile card ask the model's register for a plugin tile; it follows the index.
watch(() => plugins.index, (index) => knowTileTypes(index), { immediate: true });
// The plugins load as soon as they are on, not when the Plugins page opens: a page with a plugin tile needs its type.
watch(pluginsEnabled, (on) => { pluginTiles.enabled = on; if (on) loadPlugins(); }, { immediate: true });

type Payload = {
  plugins: Plugin[]; installed: Record<string, Installed[]>; secrets: typeof plugins.secrets;
  building: typeof plugins.jobs; folders: typeof plugins.folders; entities?: typeof plugins.entities;
};
let polling: ReturnType<typeof setTimeout> | undefined;
// The add-on's plugins (api/plugins), and again every few seconds while a screen builds.
export function reloadPlugins(refresh = false) {
  return getJson<Payload>(refresh ? "plugins?refresh=1" : "plugins").then((data) => {
    plugins.index = data.plugins;
    plugins.installed = data.installed;
    plugins.secrets = data.secrets || {};
    plugins.jobs = data.building || {};
    plugins.folders = data.folders || { path: "", errors: {} };
    plugins.entities = data.entities || [];
    plugins.building = Object.fromEntries(Object.entries(data.installed).map(([id, list]) =>
      [id, list.filter((item) => item.state === "building").map((item) => item.id)]));
    clearTimeout(polling);
    if (Object.values(plugins.building).some((list) => list.length) || Object.keys(plugins.jobs).length)
      polling = setTimeout(() => reloadPlugins(), 5000);
  }).catch(() => undefined);
}
export function loadPlugins() {
  if (plugins.loaded) return;
  plugins.loaded = true;
  reloadPlugins();
}
// The choices of an option: its own, or those of one of the plugin's fetches (a stop's lines), asked of the add-on with
// the tile's other options.
const asking = new Set<string>();
export function choicesFor(plugin: Plugin, option: PluginTileOption, values: Record<string, unknown>) {
  if (!option.options_from) return option.choices || [];
  const key = choiceKey(plugin, option, values);
  if (!(key in plugins.choices) && !asking.has(key)) {
    asking.add(key);
    const query = new URLSearchParams(Object.entries(values).filter(([k, v]) => k !== option.id && v !== "" && v !== undefined)
      .map(([k, v]) => [k, String(v)])).toString();
    getJson<{ choices: { value: string; label: string }[] }>(`plugins/${plugin.id}/choices/${option.options_from}${query ? `?${query}` : ""}`)
      .then((data) => { plugins.choices[key] = data.choices.map((c) => ({ value: c.value, label: { en: c.label } })); })
      .catch(() => { plugins.choices[key] = []; })
      .finally(() => asking.delete(key));
  }
  return plugins.choices[key] || [];
}

// A plugin tile's look in the mockup (the editor cannot run its C++): the manifest's `preview` filled in by the add-on
// from the tile's data, its first rows; asked again once a minute while a page shows it.
export type PreviewRow = { badge?: string; title?: string; value?: string; at?: number };
const drawing = new Set<string>();
export function previewFor(entity: string, options: Record<string, unknown> | undefined, bound?: string): PreviewRow[] | null {
  const kind = pluginTileOf(entity);
  if (!kind?.tile.preview) return null;
  const values = { ...pluginDefaults(kind.tile), ...(options || {}), ...(bound ? { entity: bound } : {}) };
  const key = `${entity}|${JSON.stringify(values)}`;
  const known = plugins.previews[key];
  if ((!known || Date.now() - known.at > 60000) && !drawing.has(key)) {
    drawing.add(key);
    const query = new URLSearchParams(Object.entries(values).filter(([, v]) => v !== "" && v !== undefined).map(([k, v]) => [k, String(v)])).toString();
    getJson<{ items: PreviewRow[] }>(`plugins/${kind.plugin.id}/preview/${kind.tile.id}${query ? `?${query}` : ""}`)
      .then((data) => { plugins.previews[key] = { items: data.items || [], at: Date.now() }; })
      .catch(() => { plugins.previews[key] = { items: [], at: Date.now() }; })
      .finally(() => drawing.delete(key));
  }
  return known ? known.items : null;
}

// The tap actions a tile of this domain can take on this screen: those of the plugins it runs, as tap choices.
export function tapActionsFor(screen: Screen | undefined, domain: string): [string, string][] {
  if (!screen || screen.virtual) return [];
  return plugins.index.filter((plugin) => installedOn(screen, plugin.id)).flatMap((plugin) =>
    (plugin.tap_actions || []).filter((action) => action.domains.includes(domain))
      .map((action) => [`plugin:${plugin.id}.${action.id}`, text(action.label)] as [string, string]));
}

// The top bar items of the plugins this screen runs: [{ item, label, icon, example }].
export function barItemsFor(screen: Screen | undefined) {
  if (!screen || screen.virtual) return [];
  return plugins.index.filter((plugin) => installedOn(screen, plugin.id)).flatMap((plugin) =>
    (plugin.bar_items || []).map((bar) => ({ item: `plugin:${plugin.id}.${bar.id}`, label: text(bar.label), icon: bar.icon,
      example: bar.example ? text(bar.example) : "", plugin: text(plugin.name) })));
}

// Every entity of these domains: the editor's own and those the add-on sent for plugins, each once.
export function entitiesIn(domains: string[] = []) {
  const seen = new Set<string>();
  return [...state.inventory.entities, ...plugins.entities].filter((e) => domains.includes(e.id.split(".")[0]) && !seen.has(e.id) && seen.add(e.id))
    .map((e) => ({ id: e.id, name: e.name || e.id }));
}

// An update of a plugin on this screen that asks for other rights than the person agreed to: the details ask again.
export function needsConsent(screen: Screen, plugin: Plugin) {
  const have = installedOn(screen, plugin.id);
  return Boolean(have && have.consent && plugin.permission_hash && have.consent !== plugin.permission_hash);
}

export const realScreens = () => state.inventory.screens.filter((screen) => !screen.virtual);
// The tile types a screen can place: those of the plugins it runs (the library's Plugins group). A preview screen has none.
export function tilesOn(screen: Screen | undefined) {
  if (!screen || screen.virtual) return [];
  return plugins.index.filter((plugin) => installedOn(screen, plugin.id)).flatMap((plugin) =>
    (plugin.tiles || []).map((tile) => ({ id: pluginTileId(plugin.id, tile.id), name: text(tile.name), plugin: text(plugin.name), tile: true })));
}
// A screen built from its own YAML (in ESPHome Device Builder, with no profile in Tessera): the add-on cannot add a
// plugin to it, so the page shows the lines to paste instead.
export const ownYaml = (screen: Screen) => !screen.update?.profile;
export const partsOn = (screen: Screen, plugin: Plugin) => plugins.parts[screen.node || screen.id]?.[plugin.id]
  ?? (plugin.parts || []).filter((part) => part.default).map((part) => part.id);
export function setParts(screen: Screen, plugin: Plugin, ids: string[]) {
  ((plugins.parts[screen.node || screen.id] ||= {})[plugin.id] = ids);
}
export const valueOf = (screen: Screen, plugin: Plugin, id: string) => plugins.values[screen.node || screen.id]?.[plugin.id]?.[id] ?? "";
export function setValue(screen: Screen, plugin: Plugin, id: string, value: string) {
  const node = screen.node || screen.id;
  ((plugins.values[node] ||= {})[plugin.id] ||= {})[id] = value;
}
// Every screen's plugins live in one file only the add-on writes, <node>.plugins.yaml beside its YAML, attached once by
// one line under `packages:`, the way Override YAML attaches <node>.local.yaml. A screen Tessera installed gets the line
// from the add-on; a screen with its own YAML gets it pasted once. After that ESPHome Device Builder, another computer
// sharing the config folder, or the add-on's own update builds the same plugins.
export const fileOf = (screen: Screen) => `${screen.node || screen.id}.plugins.yaml`;
export const attachLine = (screen: Screen) => `packages:\n  tessera_plugins: !include ${fileOf(screen)}`;
// An own-YAML screen counts as attached once the person says the line is in its YAML (the app cannot build it, so it
// cannot check).
export const needsAttach = (screen: Screen) => ownYaml(screen) && !plugins.attached[screen.node || screen.id];
export function markAttached(screen: Screen) { plugins.attached[screen.node || screen.id] = true; }
export const copyAttach = (screen: Screen) => copyText(attachLine(screen), undefined, "yaml");
// The plugins file of a screen with its own YAML, as the add-on wrote it ("" until it has): the add-on pins each plugin to
// the commit of its release, so the page never writes one of its own.
export const pluginsFile = (screen: Screen) => plugins.files[screen.id]?.content || "";
// Whether everything a plugin asks for is filled in on these screens: the button waits until it is.
export const setupReady = (plugin: Plugin, screens: Screen[]) =>
  screens.every((screen) => (plugin.inputs || []).every((input) => valueOf(screen, plugin, input.id).trim() !== ""
    || (input.kind === "secret" && plugins.secrets[plugin.id]?.[input.id])));
// The room the chosen optional parts add.
export const partsKb = (screen: Screen, plugin: Plugin) =>
  partsOn(screen, plugin).reduce((sum, id) => sum + (plugin.parts?.find((part) => part.id === id)?.flash_kb || 0), 0);
// The add-on keeps a screen's plugins by its inbox, the id every screen route takes.
export const nodeOf = (screen: Screen) => screen.id;
export const installedOn = (screen: Screen, id: string) => plugins.installed[nodeOf(screen)]?.find((item) => item.id === id);
export const buildingOn = (screen: Screen, id: string) => Boolean(plugins.building[nodeOf(screen)]?.includes(id));
// A plugin on a screen the add-on has no manifest of any more (a test folder that went): known only by its record.
export const testsOn = (screen: Screen) => (plugins.installed[nodeOf(screen)] || [])
  .filter((item) => item.source !== "index" && !plugins.index.some((p) => p.id === item.id)).map(testPlugin);
export const allTests = () => {
  const seen = new Map<string, Plugin>();
  for (const screen of realScreens()) for (const plugin of testsOn(screen)) seen.set(plugin.id, plugin);
  return [...seen.values()];
};
// Tessera's own, someone else's from the index, or a test the screen runs from a branch, a folder or a link.
export const labelOf = (plugin: Plugin) => plugin.label
  || (plugin.tessera ? "tessera" : plugins.index.some((p) => p.id === plugin.id) ? "community" : "test");

// ---- One line of state: for one screen (its tab), or over all screens (the page) ----
export type Status = { kind: "installed" | "update" | "building" | "test" | "misfit" | "failed" | ""; label: string };
export function statusOn(plugin: Plugin, screen: Screen): Status {
  // A screen in the add-on's build queue waits its turn; one at a time builds (docs/PLUGINS.md).
  if (buildingOn(screen, plugin.id))
    return { kind: "building", label: t(plugins.jobs[screen.id]?.state === "queued" ? "editor.plugins.state.queued" : "editor.plugins.state.building") };
  const have = installedOn(screen, plugin.id);
  if (have?.state === "failed") return { kind: "failed", label: t("editor.plugins.state.failed") };
  if (have && have.source !== "index") return { kind: "test", label: t(`editor.plugins.source.${have.source}`) };
  if (have && have.version !== plugin.version) return { kind: "update", label: t("editor.plugins.state.update", { version: plugin.version }) };
  if (have) return { kind: "installed", label: t("editor.plugins.state.installed") };
  const result = fit(plugin, screen);
  return result.ok ? { kind: "", label: "" } : { kind: "misfit", label: t(`editor.plugins.misfit_short.${result.reason}`) };
}
export function statusOverall(plugin: Plugin): Status {
  const screens = realScreens();
  if (screens.some((screen) => buildingOn(screen, plugin.id))) return { kind: "building", label: t("editor.plugins.state.building") };
  const on = screens.filter((screen) => installedOn(screen, plugin.id));
  // A test names its screen when it is on one; on more it counts them, so the line fits the card beside its chip.
  if (labelOf(plugin) === "test") return { kind: "test", label: on.length === 1 ? t("editor.plugins.state.test_on", { name: on[0].name })
    : on.length ? t("editor.plugins.state.on_screens", { n: on.length }, on.length) : "" };
  const updates = on.filter((screen) => installedOn(screen, plugin.id)!.version !== plugin.version).length;
  if (updates) return { kind: "update", label: t("editor.plugins.state.updates", { n: updates }, updates) };
  if (on.length) return { kind: "installed", label: t("editor.plugins.state.on_screens", { n: on.length }, on.length) };
  if (screens.length && !screens.some((screen) => fit(plugin, screen).ok)) return { kind: "misfit", label: t("editor.plugins.state.fits_none") };
  return { kind: "", label: "" };
}

// ---- Changes: add, update or remove on one screen ----
// What goes to the add-on for one screen: the plugin, its parts, what was filled in, and the secrets apart.
function addition(screen: Screen, plugin: Plugin) {
  const values: Record<string, string> = {}, secrets: Record<string, string> = {};
  for (const input of plugin.inputs || []) {
    const value = valueOf(screen, plugin, input.id).trim();
    if (value) (input.kind === "secret" ? secrets : values)[input.id] = value;
  }
  return { id: plugin.id, source: plugin.source || "index", parts: partsOn(screen, plugin), values, secrets,
    ...(plugins.consented[plugin.id] ? { consent: true } : {}) };
}
async function change(screen: Screen, body: object) {
  const result = await send<{ own_yaml?: boolean; file?: string; content?: string; line?: string }>(
    `screens/${encodeURIComponent(screen.id)}/plugins`, "POST", body);
  if (result?.own_yaml) plugins.files[screen.id] = { file: result.file || "", content: result.content || "", line: result.line || "" };
}
export async function addPlugin(screens: Screen[], plugin: Plugin) {
  if (!screens.length) return;
  // One build at a time: the add-on builds the screens one after the other as each build ends.
  for (const screen of screens) {
    try {
      (plugins.building[screen.id] ||= []).push(plugin.id);
      await change(screen, { add: [addition(screen, plugin)] });
    } catch (error: any) {
      plugins.building[screen.id] = (plugins.building[screen.id] || []).filter((id) => id !== plugin.id);
      toast(error.message);
      break;
    }
  }
  await reloadPlugins();
}
export async function removePlugin(screens: Screen[], plugin: Plugin) {
  if (!screens.length) return;
  for (const screen of screens) {
    try { await change(screen, { remove: [plugin.id] }); }
    catch (error: any) { toast(error.message); break; }
  }
  await reloadPlugins();
  toast(t("editor.plugins.removed", { name: text(plugin.name), screens: screens.map((s) => s.name).join(", ") }));
}
// A secret for every screen (an API key): kept by the add-on, never shown again.
export async function setSecret(plugin: Plugin, input: string, value: string) {
  await send(`plugins/${plugin.id}/secrets/${input}`, "PUT", { value });
  await reloadPlugins();
}
