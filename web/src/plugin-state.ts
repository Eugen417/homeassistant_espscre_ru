// Plugins (design): what the index offers and what each screen runs, shared by the Plugins page (every plugin, and on
// which screens it is) and a screen's own Plugins tab (what this screen has, and what it can add). A plugin lives in a
// screen's firmware, so every change is per screen; the page only sets several screens at once.
import { reactive } from "vue";
import { getJson, send } from "./api";
import { t } from "./i18n";
import { EXAMPLE_INDEX, EXAMPLE_INSTALLED, fit, testPlugin, text, type Installed, type Plugin } from "./model/plugins";
import { computed } from "vue";
import { copyText, state, toast } from "./store";
import type { Screen } from "./types";

export const plugins = reactive({
  index: EXAMPLE_INDEX as Plugin[],
  // True until the add-on serves its index (api/plugins): the pages say they show examples.
  example: true,
  installed: structuredClone(EXAMPLE_INSTALLED) as Record<string, Installed[]>,
  // Screen node → the plugins its build is adding or updating right now.
  building: {} as Record<string, string[]>,
  loaded: false,
  // What a person filled in when adding a plugin (inputs) and which optional parts are on, per screen node and plugin.
  // A secret is kept by the add-on and never comes back to the page; here it only says that one is set.
  values: {} as Record<string, Record<string, Record<string, string>>>,
  parts: {} as Record<string, Record<string, string[]>>,
  attached: {} as Record<string, boolean>,
});

// Plugins are an experiment (editor_features.plugins, SCREEN_EDITOR_ENV=development) and always on in `npm run dev`:
// a person with a released add-on never sees the page, the tab or the example index.
export const pluginsEnabled = computed(() => import.meta.env.DEV || state.inventory.editor_features?.plugins === true);

export function loadPlugins() {
  if (plugins.loaded) return;
  plugins.loaded = true;
  getJson<{ plugins: Plugin[]; installed: Record<string, Installed[]> }>("plugins").then((data) => {
    plugins.index = data.plugins;
    plugins.installed = data.installed;
    plugins.example = false;
  }).catch(() => undefined);
}

export const realScreens = () => state.inventory.screens.filter((screen) => !screen.virtual);
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
// An own-YAML screen counts as attached once its YAML has the line: the add-on sees it in the file, or the screen's hello
// names its plugins after a build. In the example the person says so.
export const needsAttach = (screen: Screen) => ownYaml(screen) && !plugins.attached[screen.node || screen.id];
export function markAttached(screen: Screen) { plugins.attached[screen.node || screen.id] = true; }
export const copyAttach = (screen: Screen) => copyText(attachLine(screen), undefined, "yaml");
// What the add-on writes in that file: each plugin pinned to the commit of its release, and what was filled in for it.
function entry(screen: Screen, plugin: Plugin) {
  const commit = "3f9c2a1e7b04d5c6a8e91f2b3c4d5e6f7a8b9c0d";  // example: the add-on writes the commit of the release
  const folder = plugin.repo.match(/\/tree\/main\/(.*)$/)?.[1];
  const path = (file: string) => (folder ? `${folder}/${file}` : file);
  // What was filled in travels as the package's vars, ESPHome's own way to hand a remote file its substitutions.
  const vars = (plugin.inputs || []).filter((input) => input.kind !== "secret")
    .map((input) => `${input.id.toUpperCase()}: "${valueOf(screen, plugin, input.id) || "…"}"`);
  const files = [path("plugin.yaml"), ...partsOn(screen, plugin).map((id) => path(`${id}.yaml`))];
  return [
    `  plugin_${plugin.id}:`,
    `    url: ${plugin.repo.replace(/\/tree\/main\/.*$/, "")}`,
    `    ref: ${commit}  # v${plugin.version}`,
    "    refresh: never",
    "    files:",
    ...files.map((file, i) => (i === 0 && vars.length ? `      - path: ${file}\n        vars: { ${vars.join(", ")} }` : `      - ${file}`)),
  ];
}
export function pluginsFile(screen: Screen, adding?: Plugin) {
  const on = plugins.index.filter((p) => installedOn(screen, p.id) || p === adding);
  return ["# Written by Tessera. Change plugins in Tessera, not here.", "packages:",
    ...(on.length ? on.flatMap((plugin) => entry(screen, plugin)) : ["  {}"])].join("\n");
}
// Whether everything a plugin asks for is filled in on these screens: the button waits until it is.
export const setupReady = (plugin: Plugin, screens: Screen[]) =>
  screens.every((screen) => (plugin.inputs || []).every((input) => valueOf(screen, plugin, input.id).trim() !== ""));
// The room the chosen optional parts add.
export const partsKb = (screen: Screen, plugin: Plugin) =>
  partsOn(screen, plugin).reduce((sum, id) => sum + (plugin.parts?.find((part) => part.id === id)?.flash_kb || 0), 0);
const nodeOf = (screen: Screen) => screen.node || screen.id;
export const installedOn = (screen: Screen, id: string) => plugins.installed[nodeOf(screen)]?.find((item) => item.id === id);
export const buildingOn = (screen: Screen, id: string) => Boolean(plugins.building[nodeOf(screen)]?.includes(id));
// Test plugins: on a screen, but not in the index. The pages know them only by what the screen says it runs.
export const testsOn = (screen: Screen) => (plugins.installed[nodeOf(screen)] || [])
  .filter((item) => item.source !== "index" && !plugins.index.some((p) => p.id === item.id)).map(testPlugin);
export const allTests = () => {
  const seen = new Map<string, Plugin>();
  for (const screen of realScreens()) for (const plugin of testsOn(screen)) seen.set(plugin.id, plugin);
  return [...seen.values()];
};
// Tessera's own, someone else's from the index, or a test the screen runs from a branch, a folder or a link.
export const labelOf = (plugin: Plugin) => plugin.tessera ? "tessera" : plugins.index.some((p) => p.id === plugin.id) ? "community" : "test";

// ---- One line of state: for one screen (its tab), or over all screens (the page) ----
export type Status = { kind: "installed" | "update" | "building" | "test" | "misfit" | ""; label: string };
export function statusOn(plugin: Plugin, screen: Screen): Status {
  if (buildingOn(screen, plugin.id)) return { kind: "building", label: t("editor.plugins.state.building") };
  const have = installedOn(screen, plugin.id);
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
  if (labelOf(plugin) === "test") return { kind: "test", label: t("editor.plugins.state.test_on", { name: on.map((s) => s.name).join(", ") }) };
  const updates = on.filter((screen) => installedOn(screen, plugin.id)!.version !== plugin.version).length;
  if (updates) return { kind: "update", label: t("editor.plugins.state.updates", { n: updates }, updates) };
  if (on.length) return { kind: "installed", label: t("editor.plugins.state.on_screens", { n: on.length }, on.length) };
  if (screens.length && !screens.some((screen) => fit(plugin, screen).ok)) return { kind: "misfit", label: t("editor.plugins.state.fits_none") };
  return { kind: "", label: "" };
}

// ---- Changes: add, update or remove on one screen ----
// The example shows the moment, not a build: with the index the add-on writes the screen's YAML and builds.
function simulate(screen: Screen, plugin: Plugin) {
  const node = nodeOf(screen);
  (plugins.building[node] ||= []).push(plugin.id);
  setTimeout(() => {
    plugins.building[node] = (plugins.building[node] || []).filter((id) => id !== plugin.id);
    const list = (plugins.installed[node] ||= []);
    const at = list.findIndex((item) => item.id === plugin.id);
    if (at >= 0) list[at] = { ...list[at], version: plugin.version };
    else list.push({ id: plugin.id, version: plugin.version, source: "index" });
  }, 2400);
}
export async function addPlugin(screens: Screen[], plugin: Plugin) {
  if (!screens.length) return;
  if (plugins.example) {
    screens.forEach((screen) => simulate(screen, plugin));
    toast(t("editor.plugins.example_install", { name: text(plugin.name), screens: screens.map((s) => s.name).join(", ") }));
    return;
  }
  for (const screen of screens) {
    try { await send(`screens/${encodeURIComponent(screen.id)}/plugins`, "POST", { id: plugin.id, version: plugin.version }); }
    catch (error: any) { toast(error.message); }
  }
}
export function removePlugin(screens: Screen[], plugin: Plugin) {
  for (const screen of screens) {
    const node = nodeOf(screen);
    plugins.installed[node] = (plugins.installed[node] || []).filter((item) => item.id !== plugin.id);
  }
  if (screens.length) toast(t("editor.plugins.removed", { name: text(plugin.name), screens: screens.map((s) => s.name).join(", ") }));
}
