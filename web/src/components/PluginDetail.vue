<script setup lang="ts">
// The details of one plugin, in the column on the right: who made it, what it adds, what it may do, the room it takes,
// and what to do with it. On the Plugins page (no `screen`) that is a checkbox per screen, ticked where it is on and
// greyed out with its reason where it does not fit; in a screen's Plugins tab it is the one button for that screen.
import { computed, reactive, ref, watch } from "vue";
import { editorLanguage, languageMarks, numberText, t, te } from "../i18n";
import { boardTitle } from "../model/boards";
import { fit, flashShare, headroomKb, text, type Plugin } from "../model/plugins";
import { glyph } from "../model/topbar";
import { addPlugin, buildingOn, installedOn, labelOf, realScreens, removePlugin, statusOn } from "../plugin-state";
import type { Screen } from "../types";
import Icon from "./ui/Icon.vue";

const props = defineProps<{ plugin: Plugin; screen?: Screen | null }>();
defineEmits<{ close: [] }>();

const label = computed(() => labelOf(props.plugin));
const marks = () => languageMarks(editorLanguage());
const kb = (value: number) => numberText(value, marks());
const percent = (share: number) => `${numberText((share * 100).toFixed(1), marks())} %`;
const works = computed(() => props.plugin.boards === "any"
  ? t(props.plugin.requires.psram ? "editor.plugins.works.any_psram" : "editor.plugins.works.any")
  : (props.plugin.board_names || props.plugin.boards).join(", "));
const domainName = (domain: string) => (te(`editor.domains.${domain}`) ? t(`editor.domains.${domain}`) : domain);
const trust = ref(false);
watch(() => props.plugin.id, () => { trust.value = false; });

// ---- One screen (its Plugins tab) ----
const here = computed(() => props.screen || null);
const hereStatus = computed(() => (here.value ? statusOn(props.plugin, here.value) : null));
const hereInstalled = computed(() => (here.value ? installedOn(here.value, props.plugin.id) : undefined));
const hereFit = computed(() => (here.value ? fit(props.plugin, here.value) : { ok: true as const }));
const hereFlash = computed(() => (here.value ? flashShare(props.plugin, here.value) : null));

// ---- Every screen (the Plugins page): a box per screen, applied together ----
const screens = computed(() => realScreens());
const wanted = reactive<Record<string, boolean>>({});
function reset() {
  for (const key of Object.keys(wanted)) delete wanted[key];
  for (const screen of screens.value) wanted[screen.id] = Boolean(installedOn(screen, props.plugin.id));
}
watch([() => props.plugin.id, () => screens.value.map((s) => s.id).join()], reset, { immediate: true });
const has = (screen: Screen) => Boolean(installedOn(screen, props.plugin.id));
const canTick = (screen: Screen) => !buildingOn(screen, props.plugin.id) && (has(screen) || fit(props.plugin, screen).ok);
function rowLine(screen: Screen) {
  const have = installedOn(screen, props.plugin.id);
  if (buildingOn(screen, props.plugin.id)) return t("editor.plugins.state.building");
  if (wanted[screen.id] && !have) return t("editor.plugins.pending.add");
  if (!wanted[screen.id] && have) return t("editor.plugins.pending.remove");
  if (have && have.source !== "index") return t(`editor.plugins.source.${have.source}`);
  if (have && have.version !== props.plugin.version) return t("editor.plugins.screen_update", { from: have.version, to: props.plugin.version });
  if (have) return t("editor.plugins.screen_version", { version: have.version });
  const result = fit(props.plugin, screen);
  if (!result.ok) return t(`editor.plugins.misfit_short.${result.reason}`);
  return screen.shape?.catalog ? boardTitle(screen.shape.catalog) : "";
}
const adding = computed(() => screens.value.filter((s) => wanted[s.id] && !has(s)));
const removing = computed(() => screens.value.filter((s) => !wanted[s.id] && has(s)));
const updatable = computed(() => screens.value.filter((s) => has(s) && installedOn(s, props.plugin.id)!.source === "index"
  && installedOn(s, props.plugin.id)!.version !== props.plugin.version && !buildingOn(s, props.plugin.id)));
const needsTrust = computed(() => label.value === "community" && adding.value.length > 0);
const applyText = computed(() => {
  const add = adding.value.length, drop = removing.value.length;
  if (add && drop) return t("editor.plugins.apply.both", { n: add + drop }, add + drop);
  if (drop) return t("editor.plugins.apply.remove", { n: drop }, drop);
  if (!add) return t("editor.plugins.apply.none");
  return t("editor.plugins.apply.add", { n: add }, add);
});
function apply() {
  addPlugin(adding.value, props.plugin);
  removePlugin(removing.value, props.plugin);
  trust.value = false;
}
watch(() => screens.value.map((s) => `${s.id}:${installedOn(s, props.plugin.id)?.version || ""}`).join(), reset);
</script>

<template>
  <div class="pd-top">
    <span class="plugin-icon large" :class="{ tessera: plugin.tessera }" aria-hidden="true"><span class="mdi">{{ glyph(plugin.icon) }}</span></span>
    <button type="button" class="icon-btn" :aria-label="t('editor.common.close')" @click="$emit('close')"><Icon name="close" /></button>
  </div>
  <div class="pd-title">
    <h2 id="plugin-name">{{ text(plugin.name) }}</h2>
    <p class="pd-by">
      {{ plugin.tessera ? t("editor.plugins.from_tessera") : t("editor.plugins.by", { maker: plugin.maintainer }) }}
      <template v-if="label !== 'test'"> · {{ t("editor.plugins.version", { version: plugin.version }) }}</template>
      <template v-if="plugin.license"> · {{ plugin.license }}</template>
    </p>
    <p class="pd-chips">
      <em class="plugin-chip" :class="label">{{ t(`editor.plugins.label.${label}`) }}</em>
      <em v-if="label !== 'test'" class="plugin-chip">{{ t(`editor.plugins.kind.${plugin.kind}`) }}</em>
      <em v-for="mark in plugin.attributes" :key="mark" class="plugin-chip attribute">{{ t(`editor.plugins.attribute.${mark}`) }}</em>
    </p>
  </div>
  <p v-if="text(plugin.description)" class="pd-description">{{ text(plugin.description) }}</p>

  <!-- In a screen's tab: the action for this screen, and what stands in its way. -->
  <div v-if="here && hereStatus" class="pd-action" :class="hereStatus.kind">
    <p v-if="!hereFit.ok && !hereInstalled" class="pd-misfit" id="plugin-misfit"><Icon name="information-outline" />{{ t(`editor.plugins.misfit.${hereFit.reason}`, { screen: here.name, kb: headroomKb() }) }}</p>
    <template v-else-if="hereStatus.kind === 'test'">
      <p class="pd-note">{{ t("editor.plugins.test_note") }}</p>
      <button type="button" class="btn quiet" @click="removePlugin([here], plugin)">{{ t("editor.plugins.remove", { screen: here.name }) }}</button>
    </template>
    <p v-else-if="hereStatus.kind === 'building'" class="pd-note"><span class="spin" aria-hidden="true"></span>{{ t("editor.plugins.building", { screen: here.name }) }}</p>
    <template v-else-if="hereInstalled">
      <p class="pd-note"><Icon name="check-circle" />{{ t("editor.plugins.installed_on", { screen: here.name, version: hereInstalled.version }) }}</p>
      <div class="pd-buttons">
        <button v-if="hereStatus.kind === 'update'" type="button" class="btn primary" id="plugin-update" @click="addPlugin([here], plugin)">{{ t("editor.plugins.update", { version: plugin.version }) }}</button>
        <button type="button" class="btn quiet" id="plugin-remove" @click="removePlugin([here], plugin)">{{ t("editor.plugins.remove", { screen: here.name }) }}</button>
      </div>
    </template>
    <template v-else>
      <label v-if="label === 'community'" class="pd-trust" id="plugin-trust">
        <input type="checkbox" v-model="trust" />
        <span><b>{{ t("editor.plugins.trust.title") }}</b>{{ t("editor.plugins.trust.text") }}</span>
      </label>
      <button type="button" class="btn primary pd-install" id="plugin-install" :disabled="label === 'community' && !trust" @click="addPlugin([here], plugin); trust = false">{{ t("editor.plugins.install", { screen: here.name }) }}</button>
    </template>
  </div>

  <!-- On the Plugins page: a box per screen; what does not fit stays in the list, greyed out, with its reason. -->
  <div v-else-if="!here" class="pd-action pd-screens-box">
    <p class="pd-label">{{ t("editor.plugins.on_screens") }}</p>
    <ul class="pd-screens" id="plugin-screens">
      <li v-for="s in screens" :key="s.id" :class="{ off: !canTick(s) }">
        <label>
          <input type="checkbox" :checked="wanted[s.id]" :disabled="!canTick(s)" :data-screen="s.name" @change="wanted[s.id] = ($event.target as HTMLInputElement).checked" />
          <span class="pd-screen-words"><b>{{ s.name }}</b><small :class="{ 'pd-pending': Boolean(wanted[s.id]) !== has(s) }">{{ rowLine(s) }}</small></span>
        </label>
        <span v-if="buildingOn(s, plugin.id)" class="spin small" aria-hidden="true"></span>
      </li>
    </ul>
    <label v-if="needsTrust" class="pd-trust" id="plugin-trust">
      <input type="checkbox" v-model="trust" />
      <span><b>{{ t("editor.plugins.trust.title") }}</b>{{ t("editor.plugins.trust.text") }}</span>
    </label>
    <div class="pd-buttons">
      <button type="button" class="btn primary" id="plugin-apply" :disabled="!(adding.length || removing.length) || (needsTrust && !trust)" @click="apply">{{ applyText }}</button>
      <button v-if="updatable.length" type="button" class="btn quiet" id="plugin-update-all" @click="addPlugin(updatable, plugin)">{{ t("editor.plugins.update_all", { n: updatable.length, version: plugin.version }, updatable.length) }}</button>
    </div>
  </div>

  <section v-if="label !== 'test'" class="pd-section">
    <h3>{{ t("editor.plugins.adds.title") }}</h3>
    <ul class="pd-list">
      <li v-for="tile in plugin.adds.tiles || []" :key="text(tile.name)"><Icon name="view-dashboard-outline" /><span><b>{{ t("editor.plugins.adds.tile", { name: text(tile.name) }) }}</b><small>{{ t("editor.plugins.adds.tile_sizes", { min: tile.min, max: tile.max }) }}</small></span></li>
      <li v-for="action in plugin.adds.tap_actions || []" :key="text(action.label)"><Icon name="gesture-tap" /><span><b>{{ t("editor.plugins.adds.tap", { name: text(action.label) }) }}</b><small>{{ t("editor.plugins.adds.tap_on", { domains: action.domains.map(domainName).join(", ") }) }}</small></span></li>
      <li v-if="plugin.adds.card"><Icon name="monitor-eye" /><span><b>{{ t("editor.plugins.adds.card") }}</b></span></li>
      <li v-if="plugin.adds.settings"><Icon name="cog-outline" /><span><b>{{ t("editor.plugins.adds.settings") }}</b><small>{{ t("editor.plugins.adds.settings_where") }}</small></span></li>
      <li v-if="plugin.adds.inputs"><Icon name="tune-variant" /><span><b>{{ t("editor.plugins.adds.inputs") }}</b></span></li>
      <li v-if="plugin.adds.ha_package"><Icon name="tray-arrow-down" /><span><b>{{ t("editor.plugins.adds.ha_package") }}</b><small>{{ t("editor.plugins.adds.ha_package_why") }}</small></span></li>
    </ul>
  </section>

  <section class="pd-section">
    <h3>{{ t("editor.plugins.may.title") }}</h3>
    <ul class="pd-list">
      <li><Icon name="home-outline" /><span>
        <b>{{ plugin.permissions.home_assistant.length ? t("editor.plugins.may.ha") : t("editor.plugins.may.ha_none") }}</b>
        <small v-if="plugin.permissions.home_assistant.length"><code v-for="kind in plugin.permissions.home_assistant" :key="kind">{{ kind }}</code></small>
      </span></li>
      <li><Icon name="wifi-strength-off-outline" /><span><b>{{ plugin.permissions.network.length ? t("editor.plugins.may.network", { hosts: plugin.permissions.network.join(", ") }) : t("editor.plugins.may.network_none") }}</b></span></li>
    </ul>
    <p v-if="!plugin.tessera" class="pd-warning" id="plugin-warning">{{ t("editor.plugins.warning") }}</p>
  </section>

  <section v-if="label !== 'test'" class="pd-section">
    <h3>{{ here ? t("editor.plugins.room.title", { screen: here.name }) : t("editor.plugins.room.title_all") }}</h3>
    <template v-if="hereFlash">
      <div class="pd-meter" :class="{ tight: hereFlash.after > 0.9 }" role="meter" aria-valuemin="0" aria-valuemax="100" :aria-valuenow="Math.round(hereFlash.after * 100)">
        <i class="before" :style="{ width: hereFlash.before * 100 + '%' }"></i>
        <i class="added" :style="{ left: hereFlash.before * 100 + '%', width: Math.max(0.6, (hereFlash.after - hereFlash.before) * 100) + '%' }"></i>
      </div>
      <p class="pd-room">{{ t("editor.plugins.room.small", { kb: kb(plugin.flash_kb), before: percent(hereFlash.before), after: percent(hereFlash.after) }) }}</p>
    </template>
    <p v-else-if="here" class="pd-room">{{ t("editor.plugins.room.large", { kb: kb(plugin.flash_kb) }) }}</p>
    <p v-else class="pd-room">{{ t("editor.plugins.room.per_screen", { kb: kb(plugin.flash_kb) }) }}</p>
    <p class="pd-room"><span class="pd-works">{{ t("editor.plugins.works.title") }}</span> {{ works }}</p>
  </section>

  <p class="pd-links">
    <a v-if="plugin.repo" :href="plugin.repo" target="_blank" rel="noopener">{{ t("editor.plugins.source_code") }}</a>
    <a v-if="plugin.repo && !plugin.tessera" :href="`${plugin.repo}/issues`" target="_blank" rel="noopener">{{ t("editor.plugins.issues") }}</a>
  </p>
</template>
