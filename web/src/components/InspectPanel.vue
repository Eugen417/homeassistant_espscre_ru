<script setup lang="ts">
// Read current data: what Home Assistant reports for the screen's tiles right now, and how each tile is set.
import { onMounted, ref, watch } from "vue";
import { getJson } from "../api";
import { t } from "../i18n";
import { controlsLabel, displayName, sizeName } from "../model/layout";
import { closeInspector, state, toast } from "../store";

const props = defineProps<{ entity?: string }>();
const summary = ref<any[] | null>(null);
const raw = ref(t("editor.inspect.reading"));
const error = ref("");
async function load() {
  if (!state.selected) return;
  summary.value = null;
  error.value = "";
  raw.value = t("editor.inspect.fetching");
  try {
    const data = await getJson(`screens/${encodeURIComponent(state.selected)}/inspect`);
    const tiles = props.entity ? data.tiles.filter((t: any) => t.entity === props.entity) : data.tiles;
    summary.value = tiles;
    raw.value = JSON.stringify(props.entity ? tiles[0] : data, null, 2);
  } catch (e: any) {
    error.value = e.message;
    raw.value = "";
    toast(e.message);
  }
}
const optionsText = (entity: string) => {
  const options = state.layout?.tiles.find((t) => t.entity === entity)?.options || {};
  return t("editor.inspect.options", {
    slider: t(options.inline === "slider" ? "editor.inspect.yes" : "editor.inspect.no"),
    display: displayName(options.display || "standard"),
    size: sizeName(options.size as string),
    control: controlsLabel({ entity, name: "", slot: 0, options }, state.inventory),
    background: state.inventory.backgrounds?.[options.background || "auto"]?.label || t("editor.inspect.background_default"),
  });
};
onMounted(load);
watch(() => props.entity, load);
</script>

<template>
  <div class="dr-head">
    <span class="av" style="background: var(--seg); color: var(--ink-2); font-size: 16px">⌕</span>
    <span class="tx"><b>{{ t("editor.inspect.title") }}</b><small :class="{ mono: entity }">{{ entity || t("editor.inspect.subtitle") }}</small></span>
    <button type="button" class="icon-btn" :aria-label="t('editor.common.close')" @click="closeInspector">✕</button>
  </div>
  <div class="dr-body" id="inspector-section">
    <div id="inspection-summary">
      <p v-if="error" class="hint warn">{{ error }}</p>
      <p v-else-if="!summary" class="hint">{{ t("editor.inspect.fetching") }}</p>
      <article v-for="(tile, i) in summary || []" :key="i" class="inspection-tile">
        <strong>{{ tile.entity }}</strong>
        <span>{{ t("editor.inspect.status", { status: tile.word || tile.state }) }}</span>
        <small>{{ optionsText(tile.entity) }}</small>
      </article>
    </div>
    <pre id="inspection">{{ raw }}</pre>
  </div>
  <div class="dr-foot">
    <button type="button" class="btn quiet" @click="load">{{ t("editor.inspect.read_again") }}</button>
    <span class="spacer"></span>
  </div>
</template>
