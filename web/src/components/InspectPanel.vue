<script setup lang="ts">
// Read current data: what Home Assistant reports for the screen's tiles right now, and how each tile is set.
import { onMounted, ref, watch } from "vue";
import { getJson } from "../api";
import { controlsLabel, displayNames, sizeNames } from "../model/layout";
import { closeInspector, state, toast } from "../store";

const props = defineProps<{ entity?: string }>();
const summary = ref<any[] | null>(null);
const raw = ref("Reading…");
const error = ref("");
async function load() {
  if (!state.selected) return;
  summary.value = null;
  error.value = "";
  raw.value = "Fetching current HA status...";
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
  return `Small slider: ${options.inline === "slider" ? "yes" : "no"} · Display: ${displayNames[options.display || "standard"] || options.display} · Size: ${sizeNames[options.size as string] || "normal"} · Control: ${controlsLabel({ entity, name: "", slot: 0, options }, state.inventory)} · Background: ${state.inventory.backgrounds?.[options.background || "auto"]?.label || "Default"}`;
};
onMounted(load);
watch(() => props.entity, load);
</script>

<template>
  <div class="dr-head">
    <span class="av" style="background: var(--seg); color: var(--ink-2); font-size: 16px">⌕</span>
    <span class="tx"><b>Current data</b><small :class="{ mono: entity }">{{ entity || "Connection, status, and supported properties" }}</small></span>
    <button type="button" class="icon-btn" aria-label="Close" @click="closeInspector">✕</button>
  </div>
  <div class="dr-body" id="inspector-section">
    <div id="inspection-summary">
      <p v-if="error" class="hint warn">{{ error }}</p>
      <p v-else-if="!summary" class="hint">Fetching current HA status...</p>
      <article v-for="(tile, i) in summary || []" :key="i" class="inspection-tile">
        <strong>{{ tile.entity }}</strong>
        <span>Status: {{ tile.word || tile.state }}</span>
        <small>{{ optionsText(tile.entity) }}</small>
      </article>
    </div>
    <pre id="inspection">{{ raw }}</pre>
  </div>
  <div class="dr-foot">
    <button type="button" class="btn quiet" @click="load">Read again</button>
    <span class="spacer"></span>
  </div>
</template>
