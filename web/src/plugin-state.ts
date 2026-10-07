// Plugins (design): what the index offers and what each screen runs, shared by the Plugins page (every plugin, and on
// which screens it is) and a screen's own Plugins tab (what this screen has, and what it can add). A plugin lives in a
// screen's firmware, so every change is per screen; the page only sets several screens at once.
import { reactive } from "vue";
import { getJson, send } from "./api";
import { t } from "./i18n";
import { EXAMPLE_INDEX, EXAMPLE_INSTALLED, fit, testPlugin, text, type Installed, type Plugin } from "./model/plugins";
import { state, toast } from "./store";
import type { Screen } from "./types";

export const plugins = reactive({
  index: EXAMPLE_INDEX as Plugin[],
  // True until the add-on serves its index (api/plugins): the pages say they show examples.
  example: true,
  installed: structuredClone(EXAMPLE_INSTALLED) as Record<string, Installed[]>,
  // Screen node → the plugins its build is adding or updating right now.
  building: {} as Record<string, string[]>,
  loaded: false,
});

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
