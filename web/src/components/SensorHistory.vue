<script setup lang="ts">
import { computed, ref, watch } from 'vue';
import { t } from '../i18n';
import { historyGeometry, loadHistory, type HistoryPreview } from '../model/history-preview';
import { state } from '../store';
const props = defineProps<{ entity: string; hours: number }>();
const history = ref<HistoryPreview | null>(null), loading = ref(true);
watch(() => [props.entity, props.hours, Math.floor(state.now / 60000)], async (_, __, cleanup) => {
  let active = true; cleanup(() => { active = false; });
  loading.value = true; history.value = null;
  try { const value = await loadHistory(props.entity, props.hours); if (active) history.value = value; }
  catch { /* No recorder or unavailable history stays explicitly empty. */ }
  finally { if (active) loading.value = false; }
}, { immediate: true });
const geometry = computed(() => history.value ? historyGeometry(history.value) : null);
const caption = computed(() => history.value ? `${new Date(history.value.start * 1000).toLocaleString()} – ${new Date(history.value.end * 1000).toLocaleString()}` : '');
</script>
<template>
  <svg v-if="geometry" class="sensor-history" viewBox="0 0 200 50" role="img" :aria-label="t('editor.pages.history_label', { hours })">
    <title>{{ caption }}</title>
    <path v-for="(path, index) in geometry.paths" :key="index" :d="path" />
    <circle v-for="(point, index) in geometry.points" :key="`point-${index}`" :cx="point.x" :cy="point.y" r="1.8" />
  </svg>
  <small v-else class="history-empty">{{ t(loading ? 'editor.pages.history_loading' : 'editor.pages.history_empty') }}</small>
</template>
<style scoped>
.sensor-history { width: 100%; min-height: 20px; max-height: 65px; color: var(--tile-accent); }
path { stroke: currentColor; stroke-width: 2; stroke-linejoin: round; stroke-linecap: round; fill: none; }
circle { fill: currentColor; }
.history-empty { font-size: 9px; color: #46525e; }
</style>
