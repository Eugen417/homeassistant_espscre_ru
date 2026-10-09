<script setup lang="ts">
// The settings of the plugins this screen runs (docs/PLUGINS.md), under Screen settings: the ESPHome entities each
// plugin's manifest names, drawn with the rows of the screen's own settings, changed through Home Assistant at once.
// The same rows stand on the screen's own settings page under Plugins.
import { ref, watch } from "vue";
import { getJson, send } from "../api";
import { editorLanguage, languageMarks, numberText, t } from "../i18n";
import { text, type Texts } from "../model/plugins";
import { glyph } from "../model/topbar";
import { nodeOf, plugins, pluginsEnabled } from "../plugin-state";
import { currentScreen, toast } from "../store";

type Row = { entity: string | null; kind: "switch" | "number" | "select" | null; label: Texts; hint?: Texts | null; available: boolean;
  value?: boolean | number | string | null; min?: number; max?: number; step?: number; unit?: string; options?: string[] };
type Group = { plugin: string; name: Texts; rows: Row[] };
const groups = ref<Group[]>([]);
async function load() {
  const screen = currentScreen.value;
  // Only a screen that runs a plugin has rows to ask for.
  if (!pluginsEnabled.value || !screen || screen.virtual || !plugins.installed[nodeOf(screen)]?.length) { groups.value = []; return; }
  try { groups.value = await getJson<Group[]>(`screens/${encodeURIComponent(screen.id)}/plugins/settings`); }
  catch { groups.value = []; }
}
watch(() => [currentScreen.value?.id, currentScreen.value && plugins.installed[nodeOf(currentScreen.value)]?.length], load, { immediate: true });
async function set(row: Row, value: boolean | number | string) {
  const screen = currentScreen.value;
  if (!screen || !row.entity) return;
  row.value = value;   // as the screen's own rows: the change shows at once, Home Assistant squares it
  try { await send(`screens/${encodeURIComponent(screen.id)}/plugins/settings`, "POST", { entity: row.entity, value }); }
  catch (error: any) { toast(error.message); load(); }
}
const number = (value: unknown, unit = "") => `${numberText(Number(value), languageMarks(editorLanguage()))}${unit ? ` ${unit}` : ""}`;
function step(row: Row, direction: number) {
  const now = Number(row.value ?? row.min ?? 0), by = row.step || 1;
  const next = Math.min(row.max ?? Infinity, Math.max(row.min ?? -Infinity, now + direction * by));
  if (next !== now) set(row, next);
}
</script>

<template>
  <section v-for="group in groups" :key="group.plugin" class="set-card plugin-settings" :data-plugin="group.plugin">
    <h4><span class="mdi">{{ glyph("F0A66") }}</span>{{ text(group.name) }}</h4>
    <div v-for="row in group.rows" :key="row.entity || text(row.label)" class="srow" :class="[`setting-${row.kind === 'switch' ? 'toggle' : row.kind === 'select' ? 'choice' : 'number'}`, { inactive: !row.available }]"
      :title="row.available ? '' : t('editor.screen_settings.unavailable')">
      <span class="s-label">{{ text(row.label) }}<small v-if="row.hint" class="help">{{ text(row.hint) }}</small></span>
      <div class="s-control">
        <button v-if="row.kind === 'switch'" type="button" class="switch" role="switch" :aria-checked="row.value ? 'true' : 'false'" :disabled="!row.available"
          :aria-label="text(row.label)" @click="set(row, !row.value)"></button>
        <div v-else-if="row.kind === 'select'" class="seg" role="group" :aria-label="text(row.label)">
          <button v-for="option in row.options" :key="option" type="button" :aria-pressed="row.value === option ? 'true' : 'false'" :disabled="!row.available" @click="set(row, option)">{{ option }}</button>
        </div>
        <div v-else-if="row.kind === 'number'" class="step">
          <button type="button" :aria-label="t('editor.screen_settings.lower', { name: text(row.label) })" :disabled="!row.available || Number(row.value) <= (row.min ?? -Infinity)" @click="step(row, -1)"><span class="mdi">{{ glyph("F0374") }}</span></button>
          <output>{{ row.value === null || row.value === undefined ? "–" : number(row.value, row.unit) }}</output>
          <button type="button" :aria-label="t('editor.screen_settings.higher', { name: text(row.label) })" :disabled="!row.available || Number(row.value) >= (row.max ?? Infinity)" @click="step(row, 1)"><span class="mdi">{{ glyph("F0415") }}</span></button>
        </div>
        <small v-else class="help">{{ t("editor.plugins.settings_missing") }}</small>
      </div>
    </div>
  </section>
</template>
