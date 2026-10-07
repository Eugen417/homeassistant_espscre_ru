<script setup lang="ts">
// A screen's Plugins tab, beside Layout and Screen settings: what this screen runs, what it can add, and, folded away,
// what does not fit its board. The same cards and details as the Plugins page, with one button for this screen.
import { computed, ref } from "vue";
import { t } from "../i18n";
import { fit, type Plugin } from "../model/plugins";
import { buildingOn, installedOn, loadPlugins, plugins, statusOn, testsOn } from "../plugin-state";
import { currentScreen, go } from "../store";
import PluginCard from "./PluginCard.vue";
import PluginDetail from "./PluginDetail.vue";
import PluginLink from "./PluginLink.vue";
import Icon from "./ui/Icon.vue";

loadPlugins();
const screen = computed(() => currentScreen.value!);
const here = computed(() => [...plugins.index.filter((p) => installedOn(screen.value, p.id) || buildingOn(screen.value, p.id)), ...testsOn(screen.value)]);
const addable = computed(() => plugins.index.filter((p) => !here.value.includes(p) && fit(p, screen.value).ok));
const misfits = computed(() => plugins.index.filter((p) => !here.value.includes(p) && !fit(p, screen.value).ok));

const panel = ref<"plugin" | "link" | null>(null);
const openId = ref<string | null>(null);
const open = computed(() => [...plugins.index, ...testsOn(screen.value)].find((p) => p.id === openId.value) || null);
function show(plugin: Plugin) { openId.value = plugin.id; panel.value = "plugin"; }
function close() { panel.value = null; openId.value = null; }
</script>

<template>
  <div class="screen-plugins" id="screen-plugins" :class="{ 'with-detail': panel }">
    <div class="sp-list">
      <p v-if="plugins.example" class="plugins-example"><Icon name="information-outline" />{{ t("editor.plugins.example") }}</p>
      <div class="sp-head">
        <p class="sp-intro">{{ t("editor.plugins.tab.intro", { screen: screen.name }) }}</p>
        <div class="sp-actions">
          <button type="button" class="btn quiet" id="screen-plugin-link" :aria-pressed="panel === 'link'" @click="panel = 'link'; openId = null"><Icon name="link-variant" />{{ t("editor.plugins.add_link") }}</button>
          <button type="button" class="btn link" id="screen-plugin-all" @click="go('#plugins')">{{ t("editor.plugins.tab.all") }}<Icon name="arrow-right" /></button>
        </div>
      </div>

      <section class="sp-group">
        <h3>{{ t("editor.plugins.tab.here") }}</h3>
        <div class="plugin-grid" role="list">
          <PluginCard v-for="plugin in here" :key="plugin.id" :plugin="plugin" :status="statusOn(plugin, screen)" :chosen="openId === plugin.id" @open="show(plugin)" />
          <p v-if="!here.length" class="sp-empty">{{ t("editor.plugins.none_installed") }}</p>
        </div>
      </section>

      <section v-if="addable.length" class="sp-group">
        <h3>{{ t("editor.plugins.tab.add") }}</h3>
        <div class="plugin-grid" role="list">
          <PluginCard v-for="plugin in addable" :key="plugin.id" :plugin="plugin" :status="statusOn(plugin, screen)" :chosen="openId === plugin.id" @open="show(plugin)" />
        </div>
      </section>

      <details v-if="misfits.length" class="sp-group sp-misfits" id="screen-plugin-misfits">
        <summary><Icon name="chevron-right" />{{ t("editor.plugins.tab.misfits", { n: misfits.length }, misfits.length) }}</summary>
        <div class="plugin-grid" role="list">
          <PluginCard v-for="plugin in misfits" :key="plugin.id" :plugin="plugin" :status="statusOn(plugin, screen)" :chosen="openId === plugin.id" @open="show(plugin)" />
        </div>
      </details>
    </div>

    <Transition name="drawer">
      <aside v-if="panel" class="plugin-detail" id="plugin-detail" @click.stop>
        <div class="plugin-detail-inner">
          <PluginDetail v-if="panel === 'plugin' && open" :plugin="open" :screen="screen" @close="close" />
          <PluginLink v-else-if="panel === 'link'" :screen="screen" @close="close" @found="show" />
        </div>
      </aside>
    </Transition>
  </div>
</template>
