<script setup lang="ts">
// Plugins (design for the plugins proposal): what a screen can get beyond the core, one card per plugin, drawn in the
// family of New screen's gallery. A plugin opens its details in a column on the right, the way a tile opens the
// inspector: what it adds, what it may do, the room it takes on this screen, and the one button. "Add with a link"
// takes a plugin that is not in the index, or a test from a branch or a folder. Until the add-on serves the index the
// page shows an example one and says so; nothing here installs anything yet.
import { computed, reactive, ref, watch } from "vue";
import { getJson, send } from "../api";
import { editorLanguage, languageMarks, numberText, t, te } from "../i18n";
import { glyph } from "../model/topbar";
import {
  EXAMPLE_INDEX, EXAMPLE_INSTALLED, fit, flashShare, headroomKb, testPlugin, text,
  type Installed, type Plugin, type PluginSource,
} from "../model/plugins";
import { currentScreen, go, state, toast } from "../store";
import type { Screen } from "../types";
import Icon from "./ui/Icon.vue";

const index = ref<Plugin[]>(EXAMPLE_INDEX);
const example = ref(true);
// The add-on's index, once it has one (fase 4); a 404 keeps the example.
getJson<{ plugins: Plugin[]; installed: Record<string, Installed[]> }>("plugins").then((data) => {
  index.value = data.plugins;
  Object.assign(installedBy, data.installed);
  example.value = false;
}).catch(() => undefined);

const installedBy = reactive<Record<string, Installed[]>>(structuredClone(EXAMPLE_INSTALLED));
const building = reactive<Record<string, string>>({});  // screen node → plugin id, while its build runs (example only)

// ---- For which screen: what fits and what is installed depend on it ----
const screens = computed(() => state.inventory.screens.filter((screen) => !screen.virtual));
const screenId = ref<string>(currentScreen.value?.id || screens.value[0]?.id || "");
watch(screens, (list) => { if (!list.some((screen) => screen.id === screenId.value)) screenId.value = list[0]?.id || ""; });
const screen = computed<Screen | null>(() => screens.value.find((s) => s.id === screenId.value) || null);
const installedHere = computed(() => (screen.value?.node && installedBy[screen.value.node]) || []);
const installedOf = (plugin: Plugin) => installedHere.value.find((item) => item.id === plugin.id);

// ---- The list: search, a filter, every plugin as a card ----
const FILTERS = ["all", "tessera", "community", "installed"] as const;
const filter = ref<(typeof FILTERS)[number]>("all");
const query = ref("");
const tests = computed(() => installedHere.value.filter((item) => item.source !== "index" && !index.value.some((p) => p.id === item.id)).map(testPlugin));
const shown = computed(() => {
  const words = query.value.toLocaleLowerCase().split(/\s+/).filter(Boolean);
  const all = [...index.value, ...tests.value];
  return all.filter((plugin) => {
    if (filter.value === "tessera" && !plugin.tessera) return false;
    if (filter.value === "community" && plugin.tessera) return false;
    if (filter.value === "installed" && !installedOf(plugin)) return false;
    const haystack = `${text(plugin.name)} ${text(plugin.summary)} ${plugin.maintainer} ${plugin.id}`.toLocaleLowerCase();
    return words.every((word) => haystack.includes(word));
  }).sort((a, b) => Number(fit(b, screen.value).ok) - Number(fit(a, screen.value).ok));
});
const count = (key: (typeof FILTERS)[number]) => key === "all" ? index.value.length + tests.value.length
  : key === "installed" ? installedHere.value.length
  : index.value.filter((p) => (key === "tessera") === p.tessera).length;

// One line on the card's right: installed, an update, building, or why it does not fit.
type Status = { kind: "installed" | "update" | "building" | "test" | "misfit" | ""; label: string };
function status(plugin: Plugin): Status {
  const node = screen.value?.node || "";
  if (building[node] === plugin.id) return { kind: "building", label: t("editor.plugins.state.building") };
  const have = installedOf(plugin);
  if (have && have.source !== "index") return { kind: "test", label: t(`editor.plugins.source.${have.source}`) };
  if (have && have.version !== plugin.version) return { kind: "update", label: t("editor.plugins.state.update", { version: plugin.version }) };
  if (have) return { kind: "installed", label: t("editor.plugins.state.installed") };
  const result = fit(plugin, screen.value);
  if (!result.ok) return { kind: "misfit", label: t(`editor.plugins.misfit_short.${result.reason}`) };
  return { kind: "", label: "" };
}

// ---- The details on the right ----
const openId = ref<string | null>(null);
const panel = ref<"plugin" | "link" | null>(null);
const open = computed(() => [...index.value, ...tests.value].find((plugin) => plugin.id === openId.value) || null);
function show(plugin: Plugin) { openId.value = plugin.id; panel.value = "plugin"; trust.value = false; }
function close() { panel.value = null; openId.value = null; }
const trust = ref(false);
const kb = (value: number) => numberText(value, languageMarks(editorLanguage()));
const percent = (share: number) => `${numberText((share * 100).toFixed(1), languageMarks(editorLanguage()))} %`;
// Tessera's own, someone else's from the index, or a test the screen runs from a branch, a folder or a link.
const labelOf = (plugin: Plugin) => plugin.tessera ? "tessera" : index.value.some((p) => p.id === plugin.id) ? "community" : "test";
const openFit = computed(() => (open.value ? fit(open.value, screen.value) : { ok: true as const }));
const openStatus = computed(() => (open.value ? status(open.value) : { kind: "", label: "" }));
const openFlash = computed(() => (open.value ? flashShare(open.value, screen.value) : null));
const openInstalled = computed(() => (open.value ? installedOf(open.value) : undefined));
const works = (plugin: Plugin) => plugin.boards === "any"
  ? t(plugin.requires.psram ? "editor.plugins.works.any_psram" : "editor.plugins.works.any")
  : (plugin.board_names || plugin.boards).join(", ");

async function install(plugin: Plugin) {
  const target = screen.value;
  if (!target?.node) return;
  if (example.value) {
    // The example shows the moment, not a build: the add-on writes the screen's YAML and builds in fase 4.
    building[target.node] = plugin.id;
    toast(t("editor.plugins.example_install", { name: text(plugin.name), screen: target.name }));
    setTimeout(() => {
      delete building[target.node!];
      const list = installedBy[target.node!] ||= [];
      const at = list.findIndex((item) => item.id === plugin.id);
      if (at >= 0) list[at] = { ...list[at], version: plugin.version };
      else list.push({ id: plugin.id, version: plugin.version, source: "index" });
    }, 2400);
    return;
  }
  try { await send(`screens/${encodeURIComponent(target.id)}/plugins`, "POST", { id: plugin.id, version: plugin.version }); }
  catch (error: any) { toast(error.message); }
}
function remove(plugin: Plugin) {
  const node = screen.value?.node;
  if (!node || !installedBy[node]) return;
  installedBy[node] = installedBy[node].filter((item) => item.id !== plugin.id);
  toast(t("editor.plugins.removed", { name: text(plugin.name) }));
}

// ---- Add with a link: a release of any repo, a branch to test, or a folder on this machine ----
const link = reactive({ source: "link" as Exclude<PluginSource, "index">, url: "", branch: "main", folder: "/config/tessera-plugins/" });
function openLink() { panel.value = "link"; openId.value = null; }
function fetchLink() {
  const known = index.value.find((plugin) => link.url && plugin.repo.replace(/\/$/, "").endsWith(link.url.replace(/^https?:\/\//, "").replace(/\/$/, "").split("github.com/")[1] || "\u0000"));
  if (known) { show(known); return; }
  toast(t(example.value ? "editor.plugins.link.example" : "editor.plugins.link.not_found"));
}
const linkReady = computed(() => link.source === "folder" ? link.folder.trim().length > "/config/".length : /^https:\/\/github\.com\/[^/]+\/[^/]+/.test(link.url.trim()));
</script>

<template>
  <div class="setup plugins" id="plugins" :class="{ 'with-detail': panel }">
    <header class="setup-head">
      <span class="setup-brand">{{ t("editor.plugins.title") }}</span>
      <span class="setup-steps-spacer"></span>
      <div class="plugins-head-actions">
        <button type="button" class="btn quiet" id="plugin-add-link" :aria-pressed="panel === 'link'" @click="openLink"><Icon name="link-variant" />{{ t("editor.plugins.add_link") }}</button>
        <button type="button" class="icon-btn" id="close-plugins" :aria-label="t('editor.common.close')" :title="t('editor.common.close')" @click="go('')"><Icon name="close" /></button>
      </div>
    </header>

    <div class="plugins-frame">
      <section class="plugins-list">
        <p v-if="example" class="plugins-example" id="plugins-example"><Icon name="information-outline" />{{ t("editor.plugins.example") }}</p>
        <h1>{{ t("editor.plugins.title") }}</h1>
        <p class="setup-lead">{{ t("editor.plugins.intro") }}</p>

        <div class="pick-tools">
          <label class="pick-search"><Icon name="magnify" /><input id="plugin-search" v-model="query" type="search" :placeholder="t('editor.plugins.search')" autocomplete="off" spellcheck="false" /></label>
          <div class="seg" role="group" :aria-label="t('editor.plugins.filter')">
            <button v-for="key in FILTERS" :key="key" type="button" :aria-pressed="filter === key" @click="filter = key">{{ t(`editor.plugins.filters.${key}`) }} <small>{{ count(key) }}</small></button>
          </div>
          <label class="plugins-for" v-if="screens.length">
            <span>{{ t("editor.plugins.for") }}</span>
            <select id="plugin-screen" v-model="screenId"><option v-for="s in screens" :key="s.id" :value="s.id">{{ s.name }}</option></select>
          </label>
        </div>

        <div class="plugin-grid" role="list">
          <button v-for="plugin in shown" :key="plugin.id" type="button" role="listitem" class="plugin-card"
                  :class="[status(plugin).kind, { chosen: openId === plugin.id }]" :data-plugin="plugin.id" @click="show(plugin)">
            <span class="plugin-icon" :class="{ tessera: plugin.tessera }" aria-hidden="true"><span class="mdi">{{ glyph(plugin.icon) }}</span></span>
            <span class="plugin-words">
              <b>{{ text(plugin.name) }}</b>
              <small>{{ plugin.tessera ? t("editor.plugins.from_tessera") : t("editor.plugins.by", { maker: plugin.maintainer }) }}</small>
            </span>
            <span v-if="text(plugin.summary)" class="plugin-summary">{{ text(plugin.summary) }}</span>
            <span class="plugin-foot">
              <em class="plugin-chip" :class="labelOf(plugin)">{{ t(`editor.plugins.label.${labelOf(plugin)}`) }}</em>
              <span v-if="status(plugin).label" class="plugin-state" :class="status(plugin).kind">
                <Icon v-if="status(plugin).kind === 'installed'" name="check" />
                <Icon v-else-if="status(plugin).kind === 'update'" name="update" />
                <span v-else-if="status(plugin).kind === 'building'" class="spin" aria-hidden="true"></span>
                {{ status(plugin).label }}
              </span>
            </span>
          </button>
          <p v-if="!shown.length" class="pick-none">{{ filter === "installed" ? t("editor.plugins.none_installed") : t("editor.plugins.none_found", { query: query.trim() }) }}</p>
        </div>

        <p class="plugins-make">
          {{ t("editor.plugins.make") }}
          <a href="https://github.com/MaxGramser/tessera-plugin-template" target="_blank" rel="noopener">{{ t("editor.plugins.template") }}</a>
        </p>
      </section>

      <Transition name="drawer">
        <aside v-if="panel" class="plugin-detail" id="plugin-detail" @click.stop>
          <div class="plugin-detail-inner">
            <!-- One plugin: who made it, the one button, what it adds, what it may do, and the room it takes. -->
            <template v-if="panel === 'plugin' && open">
              <div class="pd-top">
                <span class="plugin-icon large" :class="{ tessera: open.tessera }" aria-hidden="true"><span class="mdi">{{ glyph(open.icon) }}</span></span>
                <button type="button" class="icon-btn" :aria-label="t('editor.common.close')" @click="close"><Icon name="close" /></button>
              </div>
              <div class="pd-title">
                <h2 id="plugin-name">{{ text(open.name) }}</h2>
                <p class="pd-by">
                  {{ open.tessera ? t("editor.plugins.from_tessera") : t("editor.plugins.by", { maker: open.maintainer }) }}
                  <template v-if="openInstalled?.source === 'branch' || openInstalled?.source === 'folder'"> · {{ openInstalled.ref }}</template>
                  <template v-else> · {{ t("editor.plugins.version", { version: open.version }) }}</template>
                  <template v-if="open.license"> · {{ open.license }}</template>
                </p>
                <p class="pd-chips">
                  <em class="plugin-chip" :class="labelOf(open)">{{ t(`editor.plugins.label.${labelOf(open)}`) }}</em>
                  <em v-if="labelOf(open) !== 'test'" class="plugin-chip">{{ t(`editor.plugins.kind.${open.kind}`) }}</em>
                  <em v-for="mark in open.attributes" :key="mark" class="plugin-chip attribute">{{ t(`editor.plugins.attribute.${mark}`) }}</em>
                </p>
              </div>
              <p v-if="text(open.description)" class="pd-description">{{ text(open.description) }}</p>

              <!-- The action for the chosen screen, and what stands in its way. -->
              <div class="pd-action" :class="openStatus.kind">
                <p v-if="!openFit.ok" class="pd-misfit" id="plugin-misfit"><Icon name="information-outline" />{{ t(`editor.plugins.misfit.${openFit.reason}`, { screen: screen?.name || "", kb: headroomKb() }) }}</p>
                <template v-else-if="openStatus.kind === 'test'">
                  <p class="pd-note">{{ t("editor.plugins.test_note") }}</p>
                  <button type="button" class="btn quiet" @click="remove(open)">{{ t("editor.plugins.remove", { screen: screen?.name || "" }) }}</button>
                </template>
                <template v-else-if="openStatus.kind === 'building'">
                  <p class="pd-note"><span class="spin" aria-hidden="true"></span>{{ t("editor.plugins.building", { screen: screen?.name || "" }) }}</p>
                </template>
                <template v-else-if="openInstalled">
                  <p class="pd-note"><Icon name="check-circle" />{{ t("editor.plugins.installed_on", { screen: screen?.name || "", version: openInstalled.version }) }}</p>
                  <div class="pd-buttons">
                    <button v-if="openStatus.kind === 'update'" type="button" class="btn primary" id="plugin-update" @click="install(open)">{{ t("editor.plugins.update", { version: open.version }) }}</button>
                    <button type="button" class="btn quiet" id="plugin-remove" @click="remove(open)">{{ t("editor.plugins.remove", { screen: screen?.name || "" }) }}</button>
                  </div>
                </template>
                <template v-else>
                  <!-- A plugin of someone else: what that means, once, before the button does anything. -->
                  <label v-if="!open.tessera" class="pd-trust" id="plugin-trust">
                    <input type="checkbox" v-model="trust" />
                    <span><b>{{ t("editor.plugins.trust.title") }}</b>{{ t("editor.plugins.trust.text") }}</span>
                  </label>
                  <button type="button" class="btn primary pd-install" id="plugin-install" :disabled="!screen || (!open.tessera && !trust)" @click="install(open)">{{ t("editor.plugins.install", { screen: screen?.name || "" }) }}</button>
                </template>
              </div>

              <section v-if="openStatus.kind !== 'test'" class="pd-section">
                <h3>{{ t("editor.plugins.adds.title") }}</h3>
                <ul class="pd-list">
                  <li v-for="tile in open.adds.tiles || []" :key="text(tile.name)"><Icon name="view-dashboard-outline" /><span><b>{{ t("editor.plugins.adds.tile", { name: text(tile.name) }) }}</b><small>{{ t("editor.plugins.adds.tile_sizes", { min: tile.min, max: tile.max }) }}</small></span></li>
                  <li v-for="action in open.adds.tap_actions || []" :key="text(action.label)"><Icon name="gesture-tap" /><span><b>{{ t("editor.plugins.adds.tap", { name: text(action.label) }) }}</b><small>{{ t("editor.plugins.adds.tap_on", { domains: action.domains.map((domain) => te(`editor.domains.${domain}`) ? t(`editor.domains.${domain}`) : domain).join(", ") }) }}</small></span></li>
                  <li v-if="open.adds.card"><Icon name="monitor-eye" /><span><b>{{ t("editor.plugins.adds.card") }}</b></span></li>
                  <li v-if="open.adds.settings"><Icon name="cog-outline" /><span><b>{{ t("editor.plugins.adds.settings") }}</b><small>{{ t("editor.plugins.adds.settings_where") }}</small></span></li>
                  <li v-if="open.adds.inputs"><Icon name="tune-variant" /><span><b>{{ t("editor.plugins.adds.inputs") }}</b></span></li>
                  <li v-if="open.adds.ha_package"><Icon name="tray-arrow-down" /><span><b>{{ t("editor.plugins.adds.ha_package") }}</b><small>{{ t("editor.plugins.adds.ha_package_why") }}</small></span></li>
                </ul>
              </section>

              <section class="pd-section">
                <h3>{{ t("editor.plugins.may.title") }}</h3>
                <ul class="pd-list">
                  <li><Icon name="home-outline" /><span>
                    <b>{{ open.permissions.home_assistant.length ? t("editor.plugins.may.ha") : t("editor.plugins.may.ha_none") }}</b>
                    <small v-if="open.permissions.home_assistant.length"><code v-for="kind in open.permissions.home_assistant" :key="kind">{{ kind }}</code></small>
                  </span></li>
                  <li><Icon name="wifi-strength-off-outline" /><span><b>{{ open.permissions.network.length ? t("editor.plugins.may.network", { hosts: open.permissions.network.join(", ") }) : t("editor.plugins.may.network_none") }}</b></span></li>
                </ul>
                <p v-if="!open.tessera" class="pd-warning" id="plugin-warning">{{ t("editor.plugins.warning") }}</p>
              </section>

              <section v-if="openStatus.kind !== 'test'" class="pd-section">
                <h3>{{ t("editor.plugins.room.title", { screen: screen?.name || "" }) }}</h3>
                <template v-if="openFlash">
                  <div class="pd-meter" :class="{ tight: openFlash.after > 0.9 }" role="meter" aria-valuemin="0" aria-valuemax="100" :aria-valuenow="Math.round(openFlash.after * 100)">
                    <i class="before" :style="{ width: openFlash.before * 100 + '%' }"></i>
                    <i class="added" :style="{ left: openFlash.before * 100 + '%', width: Math.max(0.6, (openFlash.after - openFlash.before) * 100) + '%' }"></i>
                  </div>
                  <p class="pd-room">{{ t("editor.plugins.room.small", { kb: kb(open.flash_kb), before: percent(openFlash.before), after: percent(openFlash.after) }) }}</p>
                </template>
                <p v-else class="pd-room">{{ t("editor.plugins.room.large", { kb: kb(open.flash_kb) }) }}</p>
                <p class="pd-room"><span class="pd-works">{{ t("editor.plugins.works.title") }}</span> {{ works(open) }}</p>
              </section>

              <p class="pd-links">
                <a v-if="open.repo" :href="open.repo" target="_blank" rel="noopener">{{ t("editor.plugins.source_code") }}</a>
                <a v-if="open.repo && !open.tessera" :href="`${open.repo}/issues`" target="_blank" rel="noopener">{{ t("editor.plugins.issues") }}</a>
              </p>
            </template>

            <!-- Add with a link: a release, a branch to test, or a folder on this machine. -->
            <template v-else-if="panel === 'link'">
              <div class="pd-top">
                <span class="plugin-icon large" aria-hidden="true"><Icon name="link-variant" /></span>
                <button type="button" class="icon-btn" :aria-label="t('editor.common.close')" @click="close"><Icon name="close" /></button>
              </div>
              <div class="pd-title"><h2>{{ t("editor.plugins.link.title") }}</h2></div>
              <p class="pd-description">{{ t("editor.plugins.link.intro") }}</p>
              <form class="pd-link" @submit.prevent="fetchLink">
                <div class="seg" role="group" :aria-label="t('editor.plugins.link.title')">
                  <button v-for="source in (['link', 'branch', 'folder'] as const)" :key="source" type="button" :aria-pressed="link.source === source" @click="link.source = source">{{ t(`editor.plugins.link.sources.${source}`) }}</button>
                </div>
                <div v-if="link.source !== 'folder'" class="field">
                  <label class="f-label" for="plugin-url">{{ t("editor.plugins.link.url") }}</label>
                  <input id="plugin-url" v-model="link.url" type="url" placeholder="https://github.com/…/…" autocomplete="off" spellcheck="false" />
                </div>
                <div v-if="link.source === 'branch'" class="field">
                  <label class="f-label" for="plugin-branch">{{ t("editor.plugins.link.branch") }}</label>
                  <input id="plugin-branch" v-model="link.branch" autocomplete="off" spellcheck="false" />
                </div>
                <div v-if="link.source === 'folder'" class="field">
                  <label class="f-label" for="plugin-folder">{{ t("editor.plugins.link.folder") }}</label>
                  <input id="plugin-folder" v-model="link.folder" autocomplete="off" spellcheck="false" />
                </div>
                <p class="pd-note">{{ t(`editor.plugins.link.hints.${link.source}`) }}</p>
                <button type="submit" class="btn primary" id="plugin-fetch" :disabled="!linkReady">{{ t(link.source === "link" ? "editor.plugins.link.fetch" : "editor.plugins.link.test", { screen: screen?.name || "" }) }}</button>
              </form>
              <p class="pd-warning">{{ t("editor.plugins.warning") }}</p>
            </template>
          </div>
        </aside>
      </Transition>
    </div>
  </div>
</template>
