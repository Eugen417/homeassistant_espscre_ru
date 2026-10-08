<script setup lang="ts">
// Plugins (design for the plugins proposal): every plugin there is, and on which of your screens it is. A plugin lives
// in a screen's firmware, so this page does not install anything for "the app": its details tick the screens it goes
// on, greyed out where it does not fit. A screen's own Plugins tab does the same for that one screen. The list is the
// add-on's: the plugin index, and the plugins someone is making in a folder of Home Assistant's config (a test).
import { computed, ref } from "vue";
import { t } from "../i18n";
import { text, type Plugin } from "../model/plugins";
import { allTests, installedOn, labelOf, loadPlugins, plugins, realScreens, statusOverall } from "../plugin-state";
// The folder as Home Assistant shows it (config/...), not as the app's container mounts it (/homeassistant/...).
const folderShown = (path: string) => path.replace(/^\/(homeassistant|config)\//, "config/");
import { go } from "../store";
import PluginCard from "./PluginCard.vue";
import PluginDetail from "./PluginDetail.vue";
import PluginLink from "./PluginLink.vue";
import Icon from "./ui/Icon.vue";

loadPlugins();
const FILTERS = ["all", "tessera", "community", "in_use"] as const;
const filter = ref<(typeof FILTERS)[number]>("all");
const query = ref("");
const everything = computed(() => [...plugins.index, ...allTests()]);
const inUse = (plugin: Plugin) => realScreens().some((screen) => installedOn(screen, plugin.id));
const keep = (plugin: Plugin, key: (typeof FILTERS)[number]) => key === "all" || (key === "tessera" ? plugin.tessera
  : key === "community" ? labelOf(plugin) === "community" : inUse(plugin));
const shown = computed(() => {
  const words = query.value.toLocaleLowerCase().split(/\s+/).filter(Boolean);
  return everything.value.filter((plugin) => keep(plugin, filter.value) && words.every((word) =>
    `${text(plugin.name)} ${text(plugin.summary)} ${plugin.maintainer} ${plugin.id}`.toLocaleLowerCase().includes(word)));
});
const count = (key: (typeof FILTERS)[number]) => everything.value.filter((plugin) => keep(plugin, key)).length;

const panel = ref<"plugin" | "link" | null>(null);
const openId = ref<string | null>(null);
const open = computed(() => everything.value.find((plugin) => plugin.id === openId.value) || null);
function show(plugin: Plugin) { openId.value = plugin.id; panel.value = "plugin"; }
function close() { panel.value = null; openId.value = null; }
</script>

<template>
  <div class="setup plugins" id="plugins" :class="{ 'with-detail': panel }">
    <header class="setup-head">
      <span class="setup-brand">{{ t("editor.plugins.title") }}</span>
      <span class="setup-steps-spacer"></span>
      <div class="plugins-head-actions">
        <button type="button" class="btn quiet" id="plugin-add-link" :aria-pressed="panel === 'link'" @click="panel = 'link'; openId = null"><Icon name="link-variant" />{{ t("editor.plugins.add_link") }}</button>
        <button type="button" class="icon-btn" id="close-plugins" :aria-label="t('editor.common.close')" :title="t('editor.common.close')" @click="go('')"><Icon name="close" /></button>
      </div>
    </header>

    <div class="plugins-frame">
      <section class="plugins-list">
        <h1>{{ t("editor.plugins.title") }}</h1>
        <p class="setup-lead">{{ t("editor.plugins.intro") }}</p>
        <div class="pick-tools">
          <label class="pick-search"><Icon name="magnify" /><input id="plugin-search" v-model="query" type="search" :placeholder="t('editor.plugins.search')" autocomplete="off" spellcheck="false" /></label>
          <div class="seg" role="group" :aria-label="t('editor.plugins.filter')">
            <button v-for="key in FILTERS" :key="key" type="button" :aria-pressed="filter === key" @click="filter = key">{{ t(`editor.plugins.filters.${key}`) }} <small>{{ count(key) }}</small></button>
          </div>
        </div>
        <div class="plugin-grid" role="list">
          <PluginCard v-for="plugin in shown" :key="plugin.id" :plugin="plugin" :status="statusOverall(plugin)" :chosen="openId === plugin.id" @open="show(plugin)" />
          <p v-if="!shown.length" class="pick-none">{{ filter === "in_use" ? t("editor.plugins.none_in_use") : t("editor.plugins.none_found", { query: query.trim() }) }}</p>
        </div>
        <p class="plugins-make">
          {{ t("editor.plugins.make") }}
          <a href="https://github.com/MaxGramser/tessera-plugins/tree/main/template" target="_blank" rel="noopener">{{ t("editor.plugins.template") }}</a>
        </p>
        <p v-if="plugins.folders.path" class="plugins-make" id="plugins-folder">{{ t("editor.plugins.folder_note", { path: folderShown(plugins.folders.path) }) }}</p>
        <p v-for="(why, folder) in plugins.folders.errors" :key="folder" class="pd-misfit plugins-folder-error"><Icon name="information-outline" />{{ t("editor.plugins.folder_error", { folder, why }) }}</p>
      </section>

      <Transition name="drawer">
        <aside v-if="panel" class="plugin-detail" id="plugin-detail" @click.stop>
          <div class="plugin-detail-inner">
            <PluginDetail v-if="panel === 'plugin' && open" :plugin="open" @close="close" />
            <PluginLink v-else-if="panel === 'link'" @close="close" @found="show" />
          </div>
        </aside>
      </Transition>
    </div>
  </div>
</template>
